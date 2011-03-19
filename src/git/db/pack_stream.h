#ifndef GIT_DB_PACK_STREAM_H
#define GIT_DB_PACK_STREAM_H

#include <git/config.h>
#include <git/db/policy.hpp>
#include <gtl/util.hpp>
#include <gtl/db/odb_pack.hpp>			// just for exception and db traits
#include <gtl/db/zlib_mmap_device.hpp>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

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


/** \brief A stream providing access to a deltified object
  * It operates by pre-assembling all deltas into a single delta, which will be applied to the base at once
  * For this to work, the stream must know its pack (to dereference in-pack object refs) as well as a lookup 
  * git repository to obtain base object data.
  */
class PackStream : public boost::iostreams::filtering_stream<boost::iostreams::input>
{
public:
	typedef git_object_traits_base::key_type			key_type;
	typedef git_object_traits_base::size_type			size_type;
	typedef git_object_traits_base::object_type			object_type;
	typedef typename mapped_memory_manager_type::cursor	cursor_type;
	
public:
	/** Small structure to hold the pack header information
	  */
	struct PackInfo
	{
		PackedObjectType	type;	//!< object type
		size_type			size;	//!< uncompressed size in bytes
		uchar				rofs;	//!< relative offset from the type byte to the compressed stream
	
		union Additional {
			Additional() {};		//!< Don't initialize anything
			key_type		key;	//!< key to the delta base object in our pack
			uint64			ofs;	//!< interpreted as negative offset from the current delta's offset.
		};
		Additional			delta;	//! additional delta information
		
		PackInfo()
		    : type(PackedObjectType::Bad)
		{}
		
		
		inline bool is_delta() const {
			return (type == PackedObjectType::OfsDelta) | (type == PackedObjectType::RefDelta);
		}
	};
	

protected:	
	//! Parse object information at the given offset and put it into the info structure
	void info_at_offset(cursor_type& cur, uint64 ofs, PackInfo& info) const;
	
	//! Get all object info and follow the delta chain, if there is no object info yet
	void assure_object_info() const;
	
	//! \return most siginificant bit encoded length starting at begin
	//! \param begin pointer which is advanced during reading
	//! \note we assume we can read enough bytes, without overshooting any bounds
	inline uint64 msb_len(const char*& begin) const;
	
	
protected:
	const PackFile&			m_pack;			//!< pack that contains this object
	uint32					m_entry;		//!< pack entry we refer to
	mutable object_type		m_type;			//!< type of the underlying object, None by default
	mutable size_type		m_size;			//!< uncompressed final size of our object
	
public:
    PackStream(const PackFile& pack, uint32 entry=0);
	
public:
	//! @{ \name Interface
	
	//! read-only access to our pack entry
	uint32 entry() const {
		return m_entry;
	}
	
	//! read-write access to our object
	uint32& entry() {
		m_type = ObjectType::None;	// reset our cache
		return m_entry;
	}
	
	object_type type() const {
		assure_object_info();
		return m_type;
	}
	
	size_type size() const {
		assure_object_info();
		return m_size;
	}
	
	//! \return amount of deltas required to compute this object. 0 if the object is not deltified
	//! \todo implementation. This method should be used to analyse a big pack like git-verify-pack
	uint32 chain_length() const;
	
	//! @} end interface
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_DB_PACK_STREAM_H
