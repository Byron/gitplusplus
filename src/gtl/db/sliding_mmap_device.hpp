#ifndef GTL_DB_SLIDING_MMAP_DEVICE_HPP
#define GTL_DB_SLIDING_MMAP_DEVICE_HPP

#include <gtl/config.h>
#include <gtl/util.hpp>

#include <boost/iostreams/device/array.hpp>

#include <memory>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace io = boost::iostreams;
using boost::iostreams::stream_offset;


/** Defines a device which uses a memory manager to obtain mapped regions and make their memory
  * available using internal array source devices. This non-consecutive region of memory can be 
  * read and seeked as if it was consecutive.
  * Whenever a region is depleted, we request a new window, until the actual end-of-file is reached.
  * This device can be copy-constructed, which allows for easy duplication of a file with minimal overhead.
  */
template <class ManagerType>
class managed_mapped_file_source
{
public:
	typedef ManagerType													memory_manager_type;
	typedef typename memory_manager_type::mapped_file_source::char_type	char_type;
	typedef boost::iostreams::basic_array_source<char_type>				array_device_type;
	typedef stack_heap<array_device_type>								stack_array_device_type;	
	typedef std::size_t													size_type;

	static const size_type												max_length =  ~static_cast<size_type>(0);
	
public:
	
	struct category : 
	        public io::input_seekable,
	        public io::device_tag,
	        public io::closable_tag,
	        public io::direct_tag
	{};
	
protected:
	typename memory_manager_type::cursor					m_cur;			//!< memory manager cursor
	stack_array_device_type									m_sad;			//!< stack array device
	
public:
	
	managed_mapped_file_source(memory_manager_type& manager)
	{
		
	}
	
	managed_mapped_file_source(const managed_mapped_file_source& rhs);
	managed_mapped_file_source(managed_mapped_file_source&& source);
	
public:
	
	//! @{ mapped file like interface 
	bool is_open() const {
		return false;
	}
	
	//! Open the file at the given path and create a read-only memory mapped region from it
	//! \param path to file to open
	//! \param length amount of bytes to map
	//! \param offset offset into the file. Internally, the offset is adjusted to be divisable by the system's
	//! page size.
	template <typename Path>
	void open(const Path& path, size_type length = max_length, stream_offset offset = 0) 
	{
		
	}
	
	
	//! @} end mapped file like interface
	
	//! @{ \name Device Interface
	
	void close() {
		
	}
	
	std::streamsize read(char_type* s, std::streamsize n) {
		return -1;
	}

	std::streampos seek(stream_offset off, std::ios_base::seekdir way) {
		return -1;
	}
	
	//! @} end device interface
	
};


GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_DB_SLIDING_MMAP_DEVICE_HPP
