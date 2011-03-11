#ifndef GTL_DB_MAPPED_MEMORY_MANAGER_HPP
#define GTL_DB_MAPPED_MEMORY_MANAGER_HPP

#include <gtl/config.h>

#include <boost/iostreams/positioning.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

using boost::iostreams::stream_offset;

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

GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_DB_MAPPED_MEMORY_MANAGER_HPP
