#ifndef GTL_DB_MAPPED_MEMORY_MANAGER_HPP
#define GTL_DB_MAPPED_MEMORY_MANAGER_HPP

#include <gtl/config.h>
#include <gtl/util.hpp>


#include <boost/filesystem/path.hpp>
#include <boost/iostreams/positioning.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/iterator/filter_iterator.hpp>

#include <algorithm>
#include <limits>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

using boost::iostreams::stream_offset;

//! @{ \name Exceptions 

//! \brief base for all memory manager related errors
struct memory_manager_error : public std::exception
{
};

struct lru_failure :	public memory_manager_error
{
	virtual const char* what() const throw() {
		return "Couldn't find any unused memory region to free up resources";
	}
};

//! @} end exceptions

/** Maintains a list of ranges of mapped memory regions in one or more files and allows to easily 
  * obtain additional regions assuring there is no overlap.
  * Once a certain memory limit is reached globally, or if there cannot be more open file handles 
  * which result from each mem-map call, the least recently used, and currently unused mapped regions
  * are unloaded automatically.
  * \note currently not thread-safe !
  * \note in the current implementation, we will automatically unload windows if we either cannot
  * create more memory maps (as the open file handles limit is hit) or if we have allocated more than 
  * a safe amount of memory already, which would possibly cause memory allocations to fail as our address
  * space is full.
  * \todo find or write a mapped file implementation that doestn't use shared ptrs to make itself 
  * copy constructible, like the boost mapped file does. This overhead we really don't need here.
  */
template <	class MappedFileSource = boost::iostreams::mapped_file_source,
			class PathType = boost::filesystem::path  >
class mapped_memory_manager
{
public:
	typedef MappedFileSource						mapped_file_source;
	typedef PathType								path_type;
	typedef typename mapped_file_source::size_type	size_type;
	typedef mapped_memory_manager<mapped_file_source, path_type>	this_type;
	
#ifdef DEBUG
#define LINK_TYPE boost::intrusive::safe_link
#else
#define LINK_TYPE boost::intrusive::normal_link
#endif

protected:
	
	/** \brief utility type which is used to snap windows towards each other, and  adjust their size
	  */
	struct window
	{
		stream_offset	ofs;	//!< offset into the file
		size_type		size;	//!< size of the window in bytes
		
		window(stream_offset offset, size_type size)
		    : ofs(offset)
		    , size(size)
		{}
		
		template <class RegionIterator>
		inline window& operator = (RegionIterator& i) {
			ofs = i->ofs_begin();
			size = i->size();
			return *this;
		}
		
		inline stream_offset ofs_end() const {
			return ofs+size;
		}
		
		inline void align() {
			ofs = this_type::align(ofs, false);
			size = this_type::align(size, true);
		}
		
		//! Adjust the offset to start where the given window on our left ends
		//! if possible, but don't make yourself larger than max_size
		inline void extend_left_to(const window& w, size_type max_size) {
			assert(w.ofs_end() <= ofs);
			const stream_offset size_left = std::min(size, max_size);
			ofs = std::max(w.ofs_end(), ofs-size_left);
		}
		//! Adjust the size to make our window end where the right window begins, but don't
		//! get larger than max_size
		inline void extend_right_to(const window& w, size_type max_size) {
			assert(ofs <= w.ofs);
			const size_type size_left = std::min(size, max_size);
			size = size + (std::min(static_cast<stream_offset>(ofs_end()+size_left), w.ofs) - ofs_end());
		}
	};
	
	/** Defines a mapped region of memory, aligned to pagesizes.
	  * \note deallocates used region automatically on destruction
	  */
	class region : public boost::intrusive::list_base_hook<boost::intrusive::link_mode<LINK_TYPE> >
	{
	private:
		stream_offset			_b;		//!< beginning of the mapping
		mapped_file_source		_mf;	//!< mapped file
		size_type				_nc;	//!< number of clients using this particular window
		size_type				_uc;	//!< total amount of usages
		
	private:
		region(const region&);	//!< no copy constructor (due to memory map)
		
	public:
		//! Initialize a region, allocate the memory map
		//! \param size if size is larger than the file on disk, the whole file will be allocated
		//! and the size automatically adjusted
		//! \throw std::exception may throw, hence you must catch exceptions and handle them 
		//! exceptions occour if the memory map cannot be allocated
		region(const path_type& p, stream_offset ofs, size_type size) 
		    : _b(ofs)
		    , _nc(0)
		    , _uc(0)
		    
		{
			assert(this_type::align(ofs, false) == ofs);
			assert(this_type::align(size, true) == size);
			_mf.open(p, size, ofs);
		}
		
		//! Move constructors are allowed and should be supported by the memory mapped file
		region(region&&) = default;
		
	public:
		//! beginning of the mapping
		inline stream_offset ofs_begin() const {
			return _b;
		}
		//! size of the mapped region
		inline size_type size() const {
			return _mf.size();
		}
		//! offset to one byte beyond the end of the mapped region
		inline stream_offset ofs_end() const {
			return _b + size();
		}
		//! \return true if the given offset is included in our region
		inline bool includes_ofs(stream_offset ofs) const {
			return (ofs >= ofs_begin()) & (ofs < ofs_end());
		}
		//! number of clients using this window
		inline size_type num_clients() const {
			return _nc;
		}
		//! adjust the number of clients
		inline size_type& num_clients() {
			return _nc;
		}
		//! pointer to first byte of the mapped region
		inline const char* begin() const{
			return _mf.data();
		}
		//! pointer to one-beyond the last valid mapped byte of this region
		inline const char* end() const {
			return begin() + size();
		}
		//! \return amount of usages
		size_type usage_count() const {
			return _uc;
		}
		//! \return modifyable usage count
		size_type& usage_count() {
			return _uc;
		}
	};// end class region
	
	/** Utility type associating a path with a list of regions. It is tailored to be quickly looked up
	  * in a set-like structure.
	  */
	class file_regions : public boost::intrusive::set_base_hook<boost::intrusive::link_mode<LINK_TYPE> >
	{
	public:
		typedef boost::intrusive::list<region> region_dlist;
		typedef typename region_dlist::iterator region_iterator;
		typedef typename region_dlist::const_iterator region_const_iterator;
		
	public:
		//! Compares a path with a node directly
		struct comparator
		{
			bool operator()(const path_type& path, const file_regions& n) const
			{
				return path < n.path();
			}
			
			bool operator()(const file_regions& n, const path_type& path) const
			{
				return n.path() < path;
			}
		};
		
	private:
		typedef file_regions this_type;
		file_regions(const this_type&);
		
	protected:
		path_type				m_path;		//!< path our regions map
		mutable uint64_t		m_file_size;//!< total size of the file in bytes
		region_dlist			m_list;		//!< mapped regions
	
	public:
		file_regions(const path_type& path) 
			: m_path(path)
		    , m_file_size(~0u)
		{}
		
		~file_regions(){
			m_list.clear_and_dispose([](typename region_dlist::pointer p){delete p;});
		}
		
		file_regions(this_type&&) = default;
		
		bool operator < (const this_type& rhs) const {
			return m_path < rhs.m_path;
		}
		
		bool operator > (const this_type& rhs) const {
			return m_path > rhs.m_path;
		}
		
		bool operator == (const this_type& rhs) const {
			return m_path == rhs.m_path;
		}
		
	public:
		
		//! \return path we are managing
		const path_type& path() const {
			return m_path;
		}
		
		//! \return size of our managed file in bytes
		uint64_t file_size() const {
			if (m_file_size == ~0u) {
				m_file_size = boost::filesystem2::file_size(m_path);
			}
			return m_file_size;
		}
		
		region_dlist& list() {
			return m_list;
		}
		
		const region_dlist& list() const {
			return m_list;
		}
		
	};// end class file regions
	
	//! Map the path to a file_regions instance
	typedef boost::intrusive::set<file_regions> file_regions_set;

#undef LINK_TYPE
	
public:
	/** Pointer into the memory manager, keeping the current window alive until it is destroyed.
	  */
	class cursor
	{
		this_type*		m_manager;	//! pointer to our parent type
		file_regions*	m_regions;	//! item holding our regions
		region*			m_region;	//! region we are currently looking at
		stream_offset	m_ofs;		//! relative offset from the actually mapped area to our start area
		size_type		m_size;		//! maximum size we should provide
		
	public:
		cursor(this_type* manager=nullptr, file_regions* regions=nullptr)
		    : m_manager(manager)
		    , m_regions(regions)
		    , m_region(nullptr)
		    , m_ofs(0)
		    , m_size(0)
		{}
		
	public:
		//! @{ \name Interface
		
		//! assure we point to a window which allows access to the given offset into the file
		//! \param offset offset in bytes into the file
		//! \param size amount of bytes to map
		//! \return this instance - it should be queried for whether it points to a valid memory region.
		//! This is not the case if the mapping failed because we reached the end of the file
		//! \note The size actually mapped may be smaller than the given size. If that is the case
		//! either the file has reached its end, or the map was created between two existing regions
		cursor& use_region(stream_offset offset, size_type size) {
			assert(m_manager);
			assert(m_regions);
			bool need_region = true;
			
			// clamp size to window size
			size = std::min(size, m_manager->window_size());
			
			// reuse existing if possible
			if (m_region) {
				// still within a valid region ? Unuse it otherwise
				if (m_region->includes_ofs(offset)) {
					need_region = false;
				} else {
					assert(m_region->num_clients() != 0);
					m_region->num_clients() -= 1;
					m_region = nullptr;
				}
			}
			
			if (need_region) {
				typedef typename file_regions::region_const_iterator region_const_iterator;
				assert(m_region==nullptr);
				// find an existing region
				auto& regions = m_regions->list();
				auto rend = regions.end();
				auto rbeg = regions.begin();
				auto it = std::find_if(rbeg, rend, 
				                       [offset](const region& r){return r.includes_ofs(offset);});
				if (it == regions.end()) {
					// adjust windows to be at optimal start, and maximium end
					window left(0, 0);
					window mid(offset, size);
					window right(m_regions->file_size(), 0);
					
					m_manager->collect_one_lru_region(size);
					// we assume the list remains sorted by offset
					region_const_iterator insertpos;
					switch (regions.size()) 
					{
					case 0: 
					{// insert as first item
						insertpos = rend;
						break;	
					}
					case 1:
					{// insert before existing item ?
						if (rbeg->ofs_begin() > offset) {
							insertpos = rbeg;
						} else {
							insertpos = rend;
						}
						break;
					}
					default:
					{
						// find surrounding iterators, that is the first one which is offset further than us
						insertpos = std::find_if(regions.begin(), regions.end(), 
						                         [offset](const region& r){return r.ofs_begin() > offset;});
					}
					}// end size switch
				
					// adjust actual offset and size values to create the largest possible mapping
					if (insertpos == rbeg) {
						if (regions.size() > 0) {
							right = insertpos;
						}
						// handles empty list case too, as begin() == end()
					}
					else {
						// empty case was handled above
						assert(regions.size()>0);
						region_const_iterator onebefore(insertpos);
						--onebefore;
						if (insertpos != rend) {
							right = insertpos;
						}
						left = onebefore;
					}// handle windows
					
					mid.extend_left_to(left, m_manager->window_size());
					mid.extend_right_to(right, m_manager->window_size());
					mid.align();
					assert(left.ofs_end()<=mid.ofs);
					assert(right.ofs >= mid.ofs_end());
				
					// insert it at the right spot to keep order
					try {
						if (m_manager->num_file_handles() >= m_manager->max_file_handles()) {
							throw std::exception();
						}
						m_region = new region(m_regions->path(), mid.ofs, mid.size);
					} catch (const std::exception&) {
						// apparently, we are out of system resources. As many more operations
						// are likely to fail in that condition (like reading a file from disk, etc)
						// we free up as much as possible
						try {
							while (true) {
								m_manager->collect_one_lru_region(0);
							}
						} catch (const lru_failure&) {}
						m_region = new region(m_regions->path(), mid.ofs, mid.size);
					}

					m_manager->m_handles += 1;
					m_manager->m_memory_size += m_region->size();
					regions.insert(insertpos, *m_region);
				} else {
					m_region = &*it;
				}// end create region
	
				assert(m_region);
				m_region->num_clients() += 1;
			}// end need region
			
			assert(m_region);
			// recomute size and relative offset
			m_region->usage_count() += 1;
			m_ofs = offset - m_region->ofs_begin();
			m_size = std::min(size, (size_type)(m_region->ofs_end()-offset));
			return *this;
		}
		
		inline bool is_valid() const {
			return m_region != nullptr;
		}
		
		inline stream_offset ofs_begin() const {
			assert(is_valid());
			return m_region->ofs_begin() + m_ofs;
		}
		
		inline stream_offset ofs_end() const {
			assert(is_valid());
			return ofs_begin() + m_size;
		}
		
		//! \return first byte we point to
		inline const char* begin() const {
			assert(is_valid());
			return m_region->begin() + m_ofs;
		}
		
		//! \return one beyond last byte we point to
		inline const char* end() const {
			assert(m_region);
			return begin() + m_size;
		}
		
		//! \return amount of bytes we point to
		inline size_type size() const {
			return m_size;
		}
		
		//! \return read-only mapped region which defines the underlying memory frme
		//! \note the internal region may not be set yet, a nullptr can be returned
		//! \note as region is protected, you can only use it directly after the call
		//! or store a pointer using auto
		const region* region_ptr() const {
			return m_region;
		}
		
		//! \return total size of the unerlying mapped file
		uint64_t file_size() const {
			assert(m_regions);
			return m_regions->file_size();
		}
		
		//! @} end interface
		
	};// end class cursor
	
	friend class cursor;
	
private:
	mapped_memory_manager(const mapped_memory_manager&);
	mapped_memory_manager(mapped_memory_manager&&);

protected:
	file_regions_set		m_files;			//! set of file regions with all mappings
	size_type				m_window_size;		//! size of the window for allocations
	 size_type				m_max_memory_size;	//! maximum amount of memory we may allocate
	const uint32			m_max_handles;		//! maximum amount of handles to keep open
	
	size_type				m_memory_size;		//! currently allocated memory size
	uint32					m_handles;			//! amount of currently allocated handles
	
protected:
	//! Unmap the region which was least-recently used and has no client
	//! \param size of the region we want to map next (assuming its not already mapped paritally or fully
	//! if 0, we try to free any available region
	//! \throw lru_failure
	//! \todo implement a case where all unused regions are dicarded efficiently. Currently its all brute force
	inline void collect_one_lru_region(size_type size) {
		if ((size != 0) & (m_memory_size+size < m_max_memory_size)) {
			return;
		}
		// find least recently used (or least often used) region
		static auto no_clients = [](const region& r)->bool {return r.num_clients()==0;};
		typedef typename file_regions::region_dlist::iterator region_iterator;
		typedef boost::filter_iterator<decltype(no_clients), region_iterator> iter_no_clients;
		
		const region_iterator no_region;		//!< point to no region
		region_iterator lru_region(no_region);	//!< the actual most recently used region
		typename file_regions::region_dlist* lru_list=nullptr;		//! ptr to the list containing the lru_region iterator
		auto fend = m_files.end();
		
		for (auto fiter = m_files.begin(); fiter != fend; ++fiter) {
			auto& rlist = fiter->list();
			iter_no_clients rbeg(no_clients, rlist.begin(), rlist.end());
			const iter_no_clients rend(no_clients, rlist.end(), rlist.end());
			for (; rbeg != rend; ++rbeg) {
				if (lru_region == no_region || (*rbeg).usage_count() < lru_region->usage_count()) {
					lru_region = rbeg.base();
					lru_list = &rlist;
				}
			}// for each region
		}// for each file regions manager
		
		if (lru_region == no_region) {
			throw lru_failure();
		}
		
		region* r = &*lru_region;
		lru_list->erase(lru_region);
		m_memory_size -= r->size();
		m_handles -= 1;
		delete r;
	}
	
public:
	//! initialize the manager with the given window size. It is used to determine the size of each mapping
	//! whenever a new region is requested
	//! \param window_size if 0, a default window size will be chosen depending on the operating system's architecture
	//! It will internally be quantified to a multiple of the page-size
	//! \param max_memory_size maximium amount of memory we may map at once before releasing mapped regions
	//! if 0, a viable default will be set depending on the system's architecture
	//! \param max_open_handles if not ~0, limit the amount of open file handles to the given number. Otherwise
	//! the amount is only limited by the system itself. If a system or soft limit is hit, the manager will free
	//! as many handles as possible
	mapped_memory_manager(size_type window_size = 0, size_type max_memory_size = 0, uint32 max_open_handles = ~0) 
	    : m_memory_size(0)
	    , m_handles(0)
	    , m_max_handles(max_open_handles)
	{
		m_window_size = window_size != 0 ? window_size : 
								sizeof(void*) < 8 ? 32 * 1024 * 1024	// moderate sizes on 32 bit systems
		                                          : 1024 * 1024 * 1024;	// go for it on 64 bit, we have plenty of address space
		m_max_memory_size = max_memory_size != 0 ? max_memory_size : 
		                                           ((1024L * 1024L) * 
														(sizeof(void*) < 8 
																	? 512 
		                                                            : 8192));
	}
	
	//! Release as many handles as possible, spit out warnings if some maps are still opened
	~mapped_memory_manager() {
		m_files.clear_and_dispose([](typename file_regions_set::pointer p){delete p;});
	}
	
public:

	//! \return a cursor pointing to the given path. It can be used to map new memory regions
	cursor make_cursor(const path_type& path) 
	{
		static typename file_regions::comparator comp;
		typename file_regions_set::iterator it = m_files.find(path, comp);
		if (it == m_files.end()) {
			return cursor(this, &*(m_files.insert(*(new file_regions(path))).first));
		} else {
			return cursor(this, &*it);
		}
	}
	
	//! \return amount of managed files with at least one open memory region
	size_type num_open_files() const {
		return static_cast<size_type>(std::count_if(m_files.begin(), m_files.end(), 
												 [](const file_regions& r)->bool{return r.list().size()>0;}));
	}
	
	//! \return amount of file handles used. Each mapped region uses one file handle
	uint32 num_file_handles() const {
		return m_handles;
	}
	
	//! \return maximum amount of concurrently open file handles
	uint32 max_file_handles() const {
		return m_max_handles;
	}
	
	//! \return desired size each window should have upon allocation
	size_type window_size() const {
		return m_window_size;
	}
	
	//! \return amount of bytes currently mapped in total
	size_type mapped_memory_size() const {
		return m_memory_size;
	}
	
	//! \return maximum amount of memory we may allocate
	size_type max_mapped_memory_size() const {
		return m_max_memory_size;
	}
	
	//! aligns the given offset with the machine's pagesize
	//! \return aligned value
	//! \param offset_is_size if true, offset is seen as size, so that values will be rounded up
	template <class OffsetType>
	static OffsetType align(OffsetType ofs, bool offset_is_size) {
		static const int al = page_size();
		OffsetType nofs = (ofs / al) * al;
		if (offset_is_size & (nofs != ofs)) {
			nofs += al;
		}
		return nofs;
	}
	
	//! \return the system's pagesize to which we align
	static int page_size() {
		return mapped_file_source::alignment();
	}
	
};

GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_DB_MAPPED_MEMORY_MANAGER_HPP
