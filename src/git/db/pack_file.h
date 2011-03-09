#ifndef GIT_DB_PACK_FILE_H
#define GIT_DB_PACK_FILE_H

#include <git/config.h>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/device/file.hpp>

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
typedef git_object_traits::char_type					char_type;
//! @} end convenience typedefs



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
	
protected:
	Type		m_type;		//!< version of the index file
	uint32		m_version;	//!< starting at version two, there is an additional version number
	
private:
	//! \return array of 255 integers which are our hex fanout for faster lookups
	inline const uint32*	fanout() const;
	
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
	uint32 num_entries() const;
	
	//! \return checksum of the pack file as hash
	key_type pack_checksum() const;
	
	//! \return checksum of the index file as hash
	key_type index_checksum() const;
	
	//! @} end interface
	
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
  */
class PackFile
{
	
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
