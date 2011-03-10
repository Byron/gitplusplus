#ifndef GTL_DB_SLIDING_MMAP_DEVICE_HPP
#define GTL_DB_SLIDING_MMAP_DEVICE_HPP

#include <gtl/config.h>
#include <gtl/util.hpp>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/device/array.hpp>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace io = boost::iostreams;

/** Maintains a list of ranges of mapped memory regions in one or more files and allows to easily 
  * obtain additional regions assuring there is no overlap.
  * Once a certain memory limit is reached globally, or if there cannot be more open file handles 
  * which result from each mem-map call, the least recently used, and currently unused mapped regions
  * are unloaded automatically.
  * \note currently not thread-safe !
  */
template <class MappedFileSource = io::mapped_file_source>
class mapped_memory_manager
{
public:
	/** Pointer into the memory manager, keeping the current window alive until it is destroyed.
	  */
	class cursor
	{
	public:
	};

public:
	typedef MappedFileSource			mapped_file_source;
	
private:
	mapped_memory_manager(const mapped_memory_manager&);
	mapped_memory_manager(mapped_memory_manager&&);

protected:
	size_t	m_window_size;		//! size of the window for allocations
	
public:
	//! initialize the manager with the given window size. It is used to determine the size of each mapping
	//! whenever a new region is requested
	//! \param window_size if 0, a default window size will be chosen depending on the operating system's architecture
	mapped_memory_manager(size_t window_size = 0) 
	    : m_window_size(window_size)
	{
		m_window_size = m_window_size != 0 ? m_window_size : 
								sizeof(void*) < 8 ? 32 * 1024 * 1024	// moderate sizes on 32 bit systems
		                                          : 1024 * 1024*1024;	// go for it on 64 bit, we have plenty of address space
	}
	
	//! Release as many handles as possible, spit out warnings if some maps are still opened
	~mapped_memory_manager() {
		
	}
	
};


/** Defines a device which uses a memory manager to obtain mapped regions and make their memory
  * available using internal array source devices. 
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
	
	void close() {
		
	}
	
	std::streamsize read(char_type* s, std::streamsize n) {
		return -1;
	}

	std::streampos seek(boost::iostreams::stream_offset off, std::ios_base::seekdir way) {
		return -1;
	}
	
};


GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_DB_SLIDING_MMAP_DEVICE_HPP
