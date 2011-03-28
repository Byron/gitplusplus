#ifndef GIT_DB_PACK_STREAM_H
#define GIT_DB_PACK_STREAM_H

#include <git/config.h>
#include <git/db/policy.hpp>
#include <gtl/util.hpp>
#include <gtl/db/odb_pack.hpp>			// just for exception and db traits
#include <gtl/db/sliding_mmap_device.hpp>
#include <gtl/db/zlib_mmap_device.hpp>

#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/stream.hpp>

#include <memory>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

namespace io = boost::iostreams;
using boost::iostreams::stream_offset;

//! @{ \name forward declarations
class PackFile;

//! @} end forward declarations


//! @{ \name Exceptions

class PackParseError :	public gtl::pack_parse_error,
						public gtl::streaming_exception
{
public:
	virtual const char* what() const throw() {
		return gtl::streaming_exception::what();
	}
};

//! @} end exceptions

typedef gtl::odb_pack_traits<git_object_traits>			gtl_pack_traits;
typedef gtl_pack_traits::mapped_memory_manager_type		mapped_memory_manager_type;


/** \brief A device providing access to a deltified object
  * It operates by pre-assembling all deltas into a single delta, which will be applied to the base at once.
  * On the first read, the destination will be assembled into a buffer large enough to hold it completely.
  * The memory peaks during buffer reconstruction, when the base and destination buffers need to be 
  * allocated at once, as well as the delta stream which contains instruction on how the base buffer
  * needs to be processed to create the destination buffer.
  * If there are no delta's to be reconstructed, the stream is decompressed directly without memory overhead.
  * For this to work, the stream either uses its zlib device base directly, or just the inherited stream
  * \todo the current implementation cannot handle base files larger than 4GB in size. The git implementation
  * is assumed to know about that, and doesn't produce deltas for files in that size.
  */
class PackDevice : protected gtl::zlib_file_source<mapped_memory_manager_type>
{
public:
	typedef git_object_traits_base::key_type			key_type;
	// packs only use 32 bit max
	typedef uint32										size_type;
	typedef git_object_traits_base::size_type			obj_size_type;
	
	typedef git_object_traits_base::char_type			char_type;
	typedef git_object_traits::hash_generator_type		hash_generator_type;
	typedef std::pair<char_type*, char_type*>			char_range;
	typedef git_object_traits_base::object_type			object_type;
	typedef typename mapped_memory_manager_type::cursor	cursor_type;
	typedef gtl::zlib_file_source<mapped_memory_manager_type> parent_type;
	typedef gtl::intrusive_array_type<char_type>		counted_char_type;
	typedef typename counted_char_type::ptr_type		counted_char_ptr_type;
	typedef typename counted_char_type::ptr_const_type	counted_char_ptr_const_type;
	
	struct category : 
	        public io::input,
	        public io::device_tag
	{};
	
protected:
	/** Small structure to hold the pack header information
	  */
	struct PackInfo
	{
		PackedObjectType	type;	//!< object type
		obj_size_type		size;	//!< uncompressed size in bytes
		uint64				ofs;	//!< absolute offset into the pack at which this information was retrieved
		uchar				rofs;	//!< relative offset from the type byte to the compressed stream
	
		union Additional {
			Additional() {};		//!< Don't initialize anything
			key_type		key;	//!< key to the delta base object in our pack
			uint64			ofs;	//!< interpreted as negative offset from the current delta's offset.
		};
		Additional			delta;	//!< additional delta information
		
		PackInfo()
		    : type(PackedObjectType::Bad)
		{}
		
		
		inline bool is_delta() const {
			return (type == PackedObjectType::OfsDelta) | (type == PackedObjectType::RefDelta);
		}
	};
	

protected:	
	//! Parse object information at the given offset and put it into the info structure
	//! \note info structure must have its offset set already
	void info_at_offset(cursor_type& cur, PackInfo& info) const;
	
	//! Get all object info and follow the delta chain, if there is no object info yet
	//! \param size_only if true, only the size will be obtained, not the type, which would 
	//! require iterating the full delta chain
	void assure_object_info(bool size_only = false) const;

	//! Recursively unpack the object identified by the given info structure
	//! \return memory initialized with the unpacked object data. The caller is responsible
	//! for deallocation, using delete []
	//! \param out_size amount of bytes allocated in the returned buffer
	//! \throw std::bad_alloc() or ParseError
	counted_char_ptr_const_type unpack_object_recursive(cursor_type& cur, const PackInfo& info, obj_size_type& out_size);
	
	//! Apply the encoded delta stream using the base buffer and write the result into the target buffer
	//! The base buffer is assumed to be able to serve all requests from the delta stream, the destination
	//! buffer must have the correct final size.
	void apply_delta(const char_type* base, char_type* dest, const char_type* delta, size_type deltalen) const;
	
	//! Resolve all deltas and store the result in memory
	void assure_data() const;
	
	//! \return most siginificant bit encoded length starting at begin
	//! \param begin pointer which is advanced during reading
	//! \note we assume we can read enough bytes, without overshooting any bounds
	inline uint64 msb_len(const char_type*& begin) const;
	
	//! retrieve the base and target size of the delta identified by the given PackInfo
	//! \param cur cursor to acquire read acess to the pack
	//! \param ofs absolute offset of the delta's entry into the pack, to where the zstream starts
	//! \return number of bytes read from the data at offset
	//! \note this partly decompresses the stream
	uint delta_size(cursor_type& cur, uint64 ofs, size_type& base_size, size_type& target_size);
	
	//! Obtains the data at the given offset either from the cache or by decompressing it.
	//! Data will be put into the cache in case we decompressed it.
	//! \param cur cursor to use when reading pack data
	//! \param ofs absolute offset to the beginning of this objects header
	//! \param rofs relative offset to the absolute offset offset at which the zlib stream begins
	//! \param nb amount of bytes to read. The amount is assumed to be the target size of the fully 
	//! \param allow_cache if true, the cache will be queried and set, otherwise the object will always be decompressed
	//! decompressed zstream.
	//! \return managed_ptr_array with the data. It will deal with the deallocation of the included pointer as needed
	inline counted_char_ptr_const_type obtain_data(cursor_type& cur, const PackInfo& info, bool allow_cache);
	
	//! Decompress all bytes from the cursor (it must be set to the correct first byte)
	//! and continue decompression until the end of the stream or until our destination buffer
	//! is full.
	//! \param cur cursor to use to obtain pack data
	//! \param ofs absolute offset to the beginning of the zlib stream
	//! \param nb amount of bytes to decompress
	//! \param max_input_chunk_size if not 0, the amount of bytes we should put in per iteration. Use this if you only have a few
	//! input bytes  and don't want it to decompress more than necessary
	void decompress_some(cursor_type& cur, stream_offset ofs, char_type* dest, size_type nb, size_type max_input_chunk_size = 0);
	
	//! Decompress our object data into memory and update our state to allow us to be read
	inline void unpack_data(cursor_type& cur, const PackInfo& info);
	
protected:
	const PackFile&			m_pack;				//!< pack that contains this object
	uint32					m_entry;			//!< pack entry we refer to
	mutable object_type		m_type;				//!< type of the underlying object, None by default
	mutable obj_size_type	m_obj_size;			//!< uncompressed size of the object
	mutable counted_char_ptr_const_type	m_data;	//!< pointer to fully undeltified object data.
	
public:
    PackDevice(const PackFile& pack, uint32 entry = 0);
	PackDevice(const PackDevice& rhs);
	PackDevice(PackDevice&&) = default;
	~PackDevice();
	
public:
	//! @{ \name Interface
	
	//! read-only access to our pack entry
	uint32 entry() const {
		return m_entry;
	}
	
	//! read-write access to our object
	uint32& entry() {
		m_type = ObjectType::None;	// reset our cache
		m_data.reset(nullptr);
		parent_type::close();
		return m_entry;
	}
	
	object_type type() const {
		assure_object_info();
		return m_type;
	}
	
	size_type size() const {
		assure_object_info();
		return m_obj_size;
	}
	
	//! \return amount of deltas required to compute this object. 0 if the object is not deltified
	//! \todo implementation. This method should be used to analyse a big pack like git-verify-pack
	uint32 chain_length() const;
	
	
	//! \return true if the hash of the object stream matches the given one
	bool verify_hash(const key_type& hash, hash_generator_type& hgen);
	
	//! @} end interface
	
	
	//! @{ \name Device Interface
	
	//! We are always open, but it is possible to change our entry.
	bool is_open() const {
		return true;
	}
	
	std::streamsize read(char_type* s, std::streamsize n);
	
	
	//! @} end device interface
};

//!< Main type to allow using the delta as a stream
//! \todo maybe make this a class and assure the stream is unbuffered. Actually, we have to write an
//! own stream buffer which uses a device directly. The PackDevice is 48 bytes in size, with the stream 
//! wrapper it uses 400 bytes more !!! A stream buffer still takes 120 bytes for itself
typedef boost::iostreams::stream<PackDevice> PackStream;


GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_DB_PACK_STREAM_H
