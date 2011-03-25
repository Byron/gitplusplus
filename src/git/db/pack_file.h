#ifndef GIT_DB_PACK_FILE_H
#define GIT_DB_PACK_FILE_H

#include <git/config.h>
#include <git/db/pack_stream.h>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/asio.hpp>		// for ntohl
#include <boost/iterator/iterator_facade.hpp>

#include <gtl/util.hpp>
#include <gtl/db/odb_pack.hpp>
#include <git/db/policy.hpp>
#include <gtl/db/sliding_mmap_device.hpp>

#include <memory>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

namespace io = boost::iostreams;

//! @{ \name Convenience Typedefs
typedef gtl::odb_pack_traits<git_object_traits>			gtl_pack_traits;
typedef typename gtl_pack_traits::path_type				path_type;
typedef typename gtl_pack_traits::key_type				key_type;
typedef typename gtl_pack_traits::provider_mixin_type	provider_mixin_type;
typedef typename provider_mixin_type::provider_type		provider_type;
typedef typename git_object_traits::object_type			object_type;
typedef typename git_object_traits::size_type			size_type;
typedef typename git_object_traits::output_reference_type	output_reference_type;
typedef git_object_traits::char_type					char_type;
typedef typename gtl_pack_traits::mapped_memory_manager_type		mapped_memory_manager_type;
typedef gtl::odb_file_mixin<path_type, mapped_memory_manager_type> odb_file_mixin_type;
//! @} end convenience typedefs

// Forward declarations
class PackFile;

//! @{ \name Exceptions

//! \brief General error thrown if parsing of a file fails
struct ParseError :	public gtl::streaming_exception,
					public gtl::pack_parse_error
{
	const char* what() const throw() {
		return gtl::streaming_exception::what();
	}
};


//! \brief thrown if the version of the index file is not understood
//! \note only thrown for new-style index files
class IndexVersionError : public ParseError
{
private:
	uint32 m_type;
public:
	IndexVersionError(uint32 version)
	    : m_type(version)
	{
		stream() << "Cannot handle version " << version;
	}
	
public:
	//! \return version number which was found in the file, but could not be handled
	uint32 version() const {
		return m_type;
	}
};

//! @} end exceptions



/** \brief type encapsulating a pack index file, making it available for access
  * \note one instance may be used to open multiple different files in order
  */ 
class PackIndexFile : public boost::iostreams::mapped_file_source
{
public:
	//! Version identifier
	//! \todo make it derive from uchar, unfortunately qtcreator can't parse that
	enum Type /*: uchar */{
		Undefined = 0,
		Legacy = 1,
		Default = 2
	};
	
	//! 32 bit tag indicating that the given hash is not known to this index
	static const uint32 hash_unknown = ~0;
	
protected:
	//! Helper for legacy packs
	struct OffsetInfo {
		uint32		offset;
		key_type	sha1;
	};
	
	Type		m_type;		//!< version of the index file
	uint32		m_version;	//!< starting at version two, there is an additional version number
	uint32		m_num_entries;	//!< number of entries in the index
	
private:
	//! \return array of 255 integers which are our hex fanout for faster lookups
	inline const uint32*	fanout() const {
		const uint32* fan = reinterpret_cast<const uint32*>(data());
		if (m_type == Type::Default) {
			return fan + 2;	// skip 4 header bytes
		} else {
			return fan;
		}
	}
	
	uint32 _num_entries() const {
		return ntohl(fanout()[255]);
	}
	
	void reset();
		
	inline size_t v2ofs_header() const {
		return 2*4;
	}
	inline size_t v2ofs_sha() const {
		return v2ofs_header() + 256*4;
	}
	inline size_t v2ofs_crc(uint32 ne) const {
		return v2ofs_sha() + ne*key_type::hash_len;
	}
	inline size_t v2ofs_ofs32(uint32 ne) const {
		return v2ofs_crc(ne) + ne*4;
	}
	inline size_t v2ofs_ofs64(uint32 ne) const {
		return v2ofs_ofs32(ne) + ne*4;
	}
	
public:
	PackIndexFile();
	
public:
	
	//! initialize our internal pointers for fast access whenever we are to open a new file
	//! \throw IndexVersionError if the index could not be read
	void open(const path_type& path);
	
	//! Explicitly close our file and handle our internal variables
	void close();
	
public:
	//! @{ \name Interface
	
	//! \return the type of our loaded file or None if there is no file loaded
	Type type() const {
		return m_type;
	}
	
	//! \return sub-version of the index file. In case of version one (see version()), it will always be 0
	//! otherwise it usually is two, but this may be incremented in future.
	uint32 version() const {
		return m_version;
	}
	
	//! \return number of entries, i.e. amount of hashes, in the index
	inline uint32 num_entries() const {
		return m_num_entries;
	}
	
	//! \return checksum of the pack file as hash
	key_type pack_checksum() const;
	
	//! \return checksum of the index file as hash
	key_type index_checksum() const;
	
	//! \return crc32 of the given entry or 0 for legacy versions
	uint32 crc(uint32 entry) const;
	
	//! obtain the hash of the given entry
	//! \param out_hash parameter to receive the hash.
	void sha(uint32 entry, key_type& out_hash) const;
	
	//! \return offset into the pack file at which the given entry's stream begins 
	inline uint64 offset(uint32 entry) const {
		assert(entry < num_entries());
		if (m_type == Type::Default) {
			uint32 ofs32 = ntohl(reinterpret_cast<const uint32*>(data() + v2ofs_ofs32(num_entries()))[entry]);
			// if high-bit is set, offset is interpreted as index into the 64 bit offset section of the index
			if (ofs32 & 0x80000000) {
				const char* pofs64 = data() + v2ofs_ofs64(num_entries()) + (ofs32 & ~0x80000000) * sizeof(uint64);
				return (((uint64)ntohl(*((uint32_t*)(pofs64 + 0)))) << 32) |
								 ntohl(*((uint32_t*)(pofs64 + 4)));
			}
			return ofs32;
		} else {
			return ntohl(reinterpret_cast<const OffsetInfo*>(data() + 256*4)[entry].offset);
		}
	}
	
	//! \return id of the entry which contains information related to the given sha or 
	//! ~0 if it was not found. It can easily be compared to the hash_unknown static constant
	//! \param sha to look up
	//! \note this means our implementation can only handle 2^32-2 entries (instead of 2^32-1)
	//! which is probably okay considering the performance it buys as the method signature
	//! is considerably simpler that way. If there was a back with 2^32-1 entries, the last entry
	//! couldn't be accessed
	uint32 sha_to_entry(const key_type& sha) const;
	
	//! @} end interface
	
};


//! \brief object providing access to a specific entry in a pack
class PackOutputObject
{
public:
	typedef PackStream								stream_type;
	typedef gtl::stack_heap_managed<stream_type>	heap_type;
	
protected:
	const PackFile*			m_ppack;	//!< pack that contains this object
	uint32					m_entry;	//!< pack entry we refer to
	mutable heap_type		m_pstream;	//!< on-demand pointer
	
	inline void assure_stream() const {
		if (!m_pstream) {
			assert(m_ppack);
			new (m_pstream) stream_type(*m_ppack, m_entry);
			m_pstream.set_occupied();
		}
		
		// have to make it const explicitly, otherwise it uses the lvalue version of entry !
		if (static_cast<const PackDevice&>(**m_pstream).entry() != m_entry) {
			(*m_pstream)->entry() = m_entry;
		}
	}

public:
	
	//! \note we explicitly don't check bounds
	PackOutputObject(const PackFile* pack=nullptr, uint32 entry=0)
	    : m_ppack(pack)
	    , m_entry(entry)
	{}
	
	PackOutputObject(const PackOutputObject& rhs)
	    : m_ppack(rhs.m_ppack)
	    , m_entry(rhs.m_entry)
	{}
	
	PackOutputObject(PackOutputObject&&) = default;
	
	PackOutputObject& operator = (const PackOutputObject& rhs) {
		m_ppack = rhs.m_ppack;
		m_entry = rhs.m_entry;
		// if the other side is occupied or not, we can't copy the Device currently because
		// of the boost::stream limitation, hence we will recreate it on demand.
		m_pstream.destroy_safely();
		return *this;
	}
	
	bool operator == (const PackOutputObject& rhs) const {
		return (m_ppack == rhs.m_ppack) & (m_entry == rhs.m_entry);
	}
	
	bool operator != (const PackOutputObject& rhs) const {
		return !(*this == rhs);
	}
	
public:
	//! @{ \name Output Object Interface
	stream_type* new_stream() const {
		assert(m_ppack);
		return new stream_type(*m_ppack, m_entry);
	}
	
	void stream(stream_type* stream) const {
		assert(m_ppack);
		new (stream) stream_type(*m_ppack, m_entry);
	}
	
	object_type type() const {
		assure_stream();
		return (*m_pstream)->type();
	}
	size_type size() const {
		assure_stream();
		return (*m_pstream)->size();
	}
	
	void deserialize(output_reference_type out) const {
		git_object_traits::policy_type().deserialize(out, *this);
	}
	
	//! @} output object interface
	
	//! @{ Interface
	//! \return our entry
	uint32 entry() const {
		return m_entry;
	}
	
	//! allow writable access to our entry
	uint32& entry() {
		return m_entry;
	}
	
	key_type key() const;

	//! @} end interface
};



/** \brief Iterator over all items within a pack
  */
class PackBidirectionalIterator : public boost::iterator_facade<PackBidirectionalIterator,
															PackOutputObject,
															boost::bidirectional_traversal_tag,
															const PackOutputObject&>
{
protected:
	PackOutputObject			m_obj;
	
protected:
	friend class boost::iterator_core_access;
	inline void increment() {
		++m_obj.entry();
	}
	inline void decrement() {
		--m_obj.entry();
	}
	inline bool equal(const PackBidirectionalIterator& rhs) const {
		return m_obj == rhs.m_obj;
	}
	
	inline const PackOutputObject& dereference() const {
		return m_obj;
	}
	
public:
	//! \note this iterator becomes an end iterator if our entry is hash_unknown
	PackBidirectionalIterator(const PackFile* pack=nullptr, uint32 entry=0)
	    : m_obj(pack, entry)
	{}
	
public:
	
	//! @{ \name Interface
	inline key_type key() const {
		return m_obj.key();
	}
	//! @} end interface
	
};


/** \brief cache containing decompressed delta streams and base objects
  * During decompression of extremely deltified packs, the actual decompression takes the most
  * time in the process. Hence it is viable to cache decompressed deltas and bases as defined by 
  * certain limits.
  * The cache, once initialized, contains one entry per index entry which keeps the data and a hit
  * count to signal importance. Also you can quickly map an offset to the respective index, as we maintain
  * an offset-sorted list we can bisect in. The index of the offset denotes the entry at which we can find
  * additional information.
  * The client, before decompressing anything, queries the cache for decompressed data. If it exists, it will
  * be used. Otherwise, it decompresses the data itself and afterwards calls the cache to take a copy of the 
  * decompressed data so it may be found in future.
  * The type uses a global counter to set shared (global) memory limits. If the limit is reached, we prune 
  * out least recently used items, and advance all others to the next generation, increasing our own generation
  * gap.
  * By default, the cache is deactivated
  */ 
class PackCache
{
protected:
	struct CacheInfo {
		CacheInfo(uint32 usage_count = 0, size_t size = 0, const char_type* data = nullptr)
		    : usage_count(usage_count)
		    , size(0)
		    , pdata(data)
		{}
		
		CacheInfo(CacheInfo&&) = default;
		
		uint32								usage_count;	//! amount of time we have been required/used
		size_t								size;			//! amount of bytes we store
		std::unique_ptr<const char_type>	pdata;			//! stored inflated data
	};
	
	typedef std::vector<uint64>						vec_ofs;
	typedef std::vector<CacheInfo>					vec_info;
	
	static size_t									gMemoryLimit;
	static size_t									gMemory;		// current used memory
	
	
protected:
	vec_ofs		m_ofs;
	vec_info	m_info;
	mutable size_t		m_mem;			//!< current memory allocation in bytes
	
#ifdef DEBUG
	mutable uint32		m_hits;			// amount of overall cache hits
	mutable uint32		m_calls;		// amount of calls, which is hits + misses
	uint32				m_noccupied;	// amount of occupied entries
#endif
	
protected:
	//! convert an offset into an entry. Entry is hash_error if the offset wasn't found, but this shouldn't happen
	inline uint32 offset_to_entry(uint64 offset) const;
	
	//! sets data, handling our memory counter correctly
	inline void set_data(CacheInfo& info, size_t size, const char_type* data);
	
public:
	PackCache();
	
public:
	static size_t memory_limit() {
		return gMemoryLimit;
	}
	
	//! Set the memory limit
	//! \param limit if 0, the cache is effectively deactivated
	//! \note if the limit is reduced, the collection will start when the next one tries to insert
	//! data. If you want to clear the cache, use the clear() method and set the memory limit to 0
	static void set_memory_limit(size_t limit) {
		gMemoryLimit = limit;
	}
	
public:
	
	//! Empty the cache completely to reduce its memory footprint. Does nothing if the cache
	//! is not initialized.
	void clear();
	
	//! \return true if the cache is available for use. Before querying the cache
	//! or trying to put in data, you have to query for the caches availability.
	inline bool is_available() const {
		return m_ofs.size() != 0;
	}
	
	//! Initialize the data required to run the cache. Should only be called once to make
	//! the cache available, so that is_available() return true.
	//! If the cache is initialized, it does nothing
	void initialize(const PackIndexFile& index);
	
	//! \return amount of used memory in bytes
	size_t memory() const {
		return m_mem;
	}
	
	//! \return data pointer to the decompressed cache matching the given offset, or 0
	//! if there is no such cache entry
	//! \note behaviour undefined if !is_available()
	const char_type* cache_at(uint64 offset) const;
	
	//! provide cache information for the given offset
	//! \param offset at which the data should be set
	//! \param size of the data to be set
	//! \param data to be taken into the cache. If 0, the cache will be deleted if it exists
	//! The data will be owned by the cache, you must not deallocate it ! If 0 is passed in, 
	//! size must be null as well.
	//! \note has no effect if the cache entry is already set
	//! \return true if the data is used by the cache and false if it was rejected as a memory limit was hit
	//! if the cache was rejected, you remain responsible for your data
	bool set_cache_at(uint64 offset, size_t size, const char_type* data);
	
#ifdef DEBUG
	uint32 hits() const {
		return m_hits;
	}
	
	uint32 misses() const {
		return m_calls - m_hits;
	}
	
	uint32 requests() const {
		return m_calls;
	}
	
	uint32 num_occupied() const {
		return m_noccupied;
	}
	
	size_t struct_mem() const {
		return sizeof(PackCache)
		        + sizeof(vec_info::value_type)	*	m_info.size()
		        + sizeof(vec_ofs::value_type)	*	m_ofs.size();
	}
	
	void cache_info(std::ostream& out) const;
#endif
};

/** \brief implementation of the gtl::odb_pack_file interface with support for git packs version 1 and 2
  * 
  * For the supported formats, see http://book.git-scm.com/7_the_packfile.html
  * A packfile keeps a memory mapped index file for fast access, as well as a seekable stream to the 
  * pack
  * \todo for now, streaming the pack is okay, but at some point we might want to implement sliding windows
  * mapped_file device. This should be well possible using the read(...) interface, question is whether this
  * will be any faster than streaming as the data has to be copied in that case as well. Probably the only
  * way to make the implementation any faster is to manually implement the sliding window and the decompression.
  * \todo might want to use a shared pointer for packs, and use a shared_ptr_iterator from boost to keep packs alive
  * when giving them away
  */
class PackFile
{
public:
	typedef PackOutputObject					output_object_type;
	typedef PackBidirectionalIterator			bidirectional_iterator;
	typedef uint32								entry_size_type;
	typedef gtl::odb_base<gtl_pack_traits>		parent_db_type;
	
	typedef gtl_pack_traits::mapped_memory_manager_type	mapped_memory_manager_type;
	
	//! Type to be used as memory mapped device to read bytes from. It must be source device
	//! compatible to the boost io-streams framework.
	typedef gtl::managed_mapped_file_source<mapped_memory_manager_type>	mapped_file_source_type;
	typedef typename mapped_file_source_type::cursor_type				cursor_type;
	
	
	
	static const uint32						pack_signature = 0x5041434b;	// "PACK" in host format
	
private:
	PackFile(const PackFile&);
	PackFile(PackFile&&);
	
protected:
	const path_type							m_pack_path;		//! original path to the pack
	PackIndexFile							m_index;			//! Our index file
	cursor_type								m_cursor;			//! cursor into our pack
	const provider_mixin_type&				m_db;				//! reference to the database owning us
	PackCache								m_cache;			//! cache implementation
	
	
protected:
	//! \return true if the given path appears to be a valid pack file
	static bool is_valid_path(const path_type& path);

public:
	PackFile(const path_type& root, mapped_memory_manager_type& manager,const provider_mixin_type& db);
	
public:
	
	//! @{ \name PackFile Interface
	static PackFile* new_pack(const path_type& file, mapped_memory_manager_type& manager, const provider_mixin_type& db);
	
	const path_type& pack_path() const {
		return m_pack_path;
	}
	
	bidirectional_iterator begin() const {
		return PackBidirectionalIterator(this, 0);
	}
	
	bidirectional_iterator end() const {
		return PackBidirectionalIterator(this, this->index().num_entries());
	}
	
	entry_size_type num_entries() const {
		return index().num_entries();
	}
	
	inline bool has_object(const key_type& k) const {
		return index().sha_to_entry(k) != PackIndexFile::hash_unknown;
	}
	
	inline PackOutputObject object(const key_type& k) const {
		// although we should raise, if key was not found, we trust the base implementation
		assert(has_object(k));
		return PackOutputObject(this, index().sha_to_entry(k));
	}
	
	//! @} end packfile interface
	
	//! @{ \name Interface
	//! \return our associated index file
	
	//! the cache is considered a mutable part of an otherwise constant pack file
	inline PackCache& cache() const {
		return const_cast<PackFile*>(this)->m_cache;
	}
	
	inline const PackIndexFile& index() const {
		return m_index;
	}
	
	//! \return provider interface pointer or 0 if no object provder was set
	//! The provider pointer is borrowed and must not be deallocated.
	const provider_type* provider() const {
		return m_db.object_provider();
	}
	
	//! \return cursor into our pack
	//! It should be copied for own use
	const cursor_type& cursor() const {
		return m_cursor;
	}
	
	
	
	//! @} end interface
	
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_DB_PACK_FILE_H
