#ifndef GTL_DB_SLIDING_MMAP_DEVICE_HPP
#define GTL_DB_SLIDING_MMAP_DEVICE_HPP

#include <gtl/config.h>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/device/array.hpp>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN


/** Maintains a list of ranges of mapped memory regions in one or more files and allows to easily 
  * obtain additional regions assuring there is no overlap.
  * Once a certain memory limit is reached globally, or if there cannot be more open file handles 
  * which result from each mem-map call, the least recently used, and currently unused mapped regions
  * are unloaded automatically.
  * \note currently not thread-safe !
  */
template <class MappedFileSource = boost::iostreams::mapped_file_source>
class mapped_memory_manager
{
public:
	typedef MappedFileSource			mapped_file_source;
	
};


/** Defines a device which uses a memory manager to obtain mapped regions and make their memory
  * available using internal array source devices. Whenever a region is depleted, 
  * As we use a 
  */
class managed_mapped_file_source
{
public:
	typedef	boost::iostreams::mapped_file_source::char_type		char_type;
	typedef boost::iostreams::array_source<char_type>			array_device_type;
	
protected:
	// array_device_type			m_ad;				//!< array device
	
};


GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_DB_SLIDING_MMAP_DEVICE_HPP
