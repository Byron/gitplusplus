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

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

namespace io = boost::iostreams;

//! @{ \name Convenience Typedefs
typedef gtl::odb_pack_traits<git_object_traits>			gtl_pack_traits;
typedef typename gtl_pack_traits::path_type				path_type;
typedef typename gtl_pack_traits::key_type				key_type;
typedef typename git_object_traits::size_type			size_type;
typedef git_object_traits::char_type					char_type;
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
	typedef DeltaPackStream				stream_type;
protected:
	const PackFile& m_pack;			//!< pack that contains this object
	uint32			m_entry;				//!< pack entry we refer to

public:
	//! \note we explicitly don't check bounds
	PackOutputObject(const PackFile& pack, uint32 entry)
	    : m_pack(pack)
	    , m_entry(entry)
	{}
	
public:
	//! @{ \name Output Object Interface
	stream_type* new_stream() const;
	void stream(stream_type* stream) const;
	key_type key() const;
	size_type size() const;
	//! @} output object interface
	
	//! @{ Interface
	uint32 entry() const {
		return m_entry;
	}
	
	//! set our instance to the given entry - we don't check bounds
	uint32& entry() {
		return m_entry;
	}

	//! @} end interface
};


/** \brief Iterator over all items within a pack
  */
class PackForwardIterator : public boost::iterator_facade<	PackForwardIterator,
															PackOutputObject,
															std::forward_iterator_tag>
{
protected:
	PackOutputObject			m_obj;
	
protected:
	void advance(difference_type n);
	
public:
	PackForwardIterator(const PackFile& pack, uint32 entry)
	    : m_obj(pack, entry)
	{}
	
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
	typedef PackForwardIterator				accessor;
	typedef PackForwardIterator				forward_iterator;
	
private:
	PackFile(const PackFile&);
	PackFile(PackFile&&);
	
protected:
	const path_type							m_pack_path;		//! original path to the pack
	PackIndexFile							m_index;			//! Our index file
	//boost::iostreams::mapped_file_source	m_pack;				//! portion of the packed file itself
	
protected:
	//! \return true if the given path appears to be a valid pack file
	static bool is_valid_path(const path_type& path);

public:
	PackFile(const path_type& root);
	
public:
	
	//! @{ \name PackFile Interface
	static PackFile* new_pack(const path_type& file);
	
	const path_type& pack_path() const {
		return m_pack_path;
	}
	
	forward_iterator begin() const {
		return PackForwardIterator(*this, 0);
	}
	
	forward_iterator end() const {
		return PackForwardIterator(*this, this->index().num_entries());
	}
	
	//! @} end packfile interface
	
	//! @{ \name Interface
	//! \return our associated index file
	const PackIndexFile& index() const {
		return m_index;
	}
	
	//! @} end interface
	
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_DB_PACK_FILE_H
