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
typedef git_object_traits::char_type					char_type;
//! @} end convenience typedefs


//! @{ \name Exceptions
class IndexVersionError :	public gtl::streaming_exception,
							public gtl::odb_error
{
	const char* what() const throw() {
		return gtl::streaming_exception::what();
	}
};

//! @}


/** \brief type encapsulating a pack index file, making it available for access
  */ 
class PackIndexFile : public boost::iostreams::mapped_file_source
{
protected:
	//! Version identifier
	//! \todo make it derive from uchar, unfortunately qtcreator can't parse that
	enum Version /*: uchar */{
		None = 0,
		One = 1,
		Two = 2
	};
	
protected:
	Version m_version;		//!< version of the index file
	
public:
	PackIndexFile();
	
public:
	
	//! initialize our internal pointers for fast access whenever we are to open a new file
	//! \throw IndexVersionError if the index could not be read
	void open(const path_type& path);
	
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
	
	static PackFile* new_pack(const path_type& file);
	
	const path_type& pack_path() const {
		return m_pack_path;
	}
	
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_DB_PACK_FILE_H
