#ifndef GTL_DB_SLIDING_MMAP_DEVICE_HPP
#define GTL_DB_SLIDING_MMAP_DEVICE_HPP

#include <gtl/config.h>
#include <gtl/util.hpp>

#include <boost/iostreams/categories.hpp>

#include <memory>
#include <ios>
#include <limits>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace io = boost::iostreams;
using boost::iostreams::stream_offset;


/** \brief utilty which adds members to facilite navigation and seeking in a memory area
  * \note the m_nb member must be set to the number of bytes left for reading !
  */
template <class ManagerType>
class seekable_memory_device_mixin
{
public:
	typedef ManagerType													memory_manager_type;
	typedef typename memory_manager_type::size_type						size_type;
	typedef typename memory_manager_type::mapped_file_source::char_type	char_type;
	
protected:
	stream_offset			m_ofs;			//!< absolute offset into the mapping
	size_type				m_nb;			//!< amount of bytes left for reading
	size_type				m_size;			//!< total size of the mapping
	
public:
	static const size_type	max_length = std::numeric_limits<size_type>::max();

protected:
	seekable_memory_device_mixin()
	    : m_ofs(0)
	    , m_nb(0)
	    , m_size(0)
	{}
	
	std::streamsize read(char_type* s, std::streamsize n, const char_type* first) {
		if (this->m_nb == 0){
			return -1;	// eof
		}
		
		size_t bytes_to_copy =	std::min(static_cast<size_type>(n), this->m_nb);
		std::memcpy(s, first, bytes_to_copy);
		this->m_nb -= bytes_to_copy;
		this->m_ofs += bytes_to_copy;
		return bytes_to_copy;
	}
	
	
public:
	
	//! \return amount of bytes left in the mapping
	size_type bytes_left() const {
		return m_nb;
	}
	
	//! \return true if we reached the end of our mapping
	bool eof() const {
		return m_nb == 0;
	}
	
	//! \return initial size of the mapping
	size_type size() const {
		return m_size;
	}
	
	//! \return our current absolute offset into the mapping
	stream_offset tellg() const {
		return m_ofs;
	}
	
	std::streampos seek(stream_offset off, std::ios_base::seekdir way) {
		switch(way)
		{
		case std::ios_base::beg: {
			if (off < 0 || static_cast<size_type>(off) >= m_nb) {
				throw std::ios_base::failure("invalid offset");
			}
			m_nb = m_nb - (off - m_ofs);
			m_ofs = off;
			break;
		}
		case std::ios_base::cur: {
			if (m_ofs + off >= m_ofs + static_cast<stream_offset>(m_nb) || 
			    m_ofs + off < ((m_ofs + static_cast<stream_offset>(m_nb)) - static_cast<stream_offset>(m_size))) {
				throw std::ios_base::failure("offsets puts stream position out of bounds");
			}
			m_ofs += off;
			m_nb -= off;
			break;
		}
		case std::ios_base::end: {
			if (off > 0 || (static_cast<stream_offset>(m_size) + off) < 0) {
				throw std::ios_base::failure("invalid offset");
			}
			m_ofs = m_ofs + (m_nb + off);
			m_nb -= off;
			break;
		}
		default: throw std::ios_base::failure("unknown seek direction");
		}
		
		return m_ofs;
	}
	
};

/** Defines a device which uses a memory manager to obtain mapped regions and make their memory
  * available using internal array source devices. This non-consecutive region of memory can be 
  * read and seeked as if it was consecutive.
  * Whenever a region is depleted, we request a new window, until the actual end-of-file is reached.
  * This device can be copy-constructed, which allows for easy duplication of a file with minimal overhead.
  */
template <class ManagerType>
class managed_mapped_file_source	: public seekable_memory_device_mixin<ManagerType>
{
public:
	typedef ManagerType													memory_manager_type;
	typedef seekable_memory_device_mixin<memory_manager_type>			parent_type;
	typedef typename memory_manager_type::mapped_file_source::char_type	char_type;
	typedef typename memory_manager_type::size_type						size_type;
	typedef typename memory_manager_type::cursor						cursor_type;

public:
	
	struct category : 
	        public io::input_seekable,
	        public io::device_tag,
	        public io::closable_tag,
	        public io::direct_tag
	{};
	
protected:
	memory_manager_type&	m_man;			//!< memory manager to obtain cursors
	cursor_type				m_cur;			//!< memory manager cursor
	
protected:
	//! finalize open operation, assuming our cursor to be set
	inline void do_open(size_type length, stream_offset offset) {
		m_cur.use_region(offset, length);
		if (!m_cur.is_valid()) {
			throw std::ios_base::failure("Could not map given file region");
		}
		// Compute our own size
		this->m_nb = std::min(m_cur.file_size() - static_cast<size_type>(offset), length - static_cast<size_type>(offset));
		this->m_size = this->m_nb;
		this->m_ofs = offset;
	}
	
private:
	managed_mapped_file_source(managed_mapped_file_source&& source);
	
public:
	
	/** Initialize this instance with a memory manager.
	  * \param cursor this optional paramenter assures the file is already opened at the location
	  * of the cursor. The cursor must be associated, otherwise undefined behaviour occurs.
	  * \param length of bytes to map if cursor is given
	  * \param offset into the file if cursor is given
	  */ 
	managed_mapped_file_source(memory_manager_type& manager, 
	                           const cursor_type* cursor = nullptr, size_type length=parent_type::max_length, stream_offset offset = 0)
		: m_man(manager)
	    
	{
		if (cursor && cursor->is_associated()) {
			m_cur = *cursor;
			do_open(length, offset);
		}
	}
	
	managed_mapped_file_source(const managed_mapped_file_source& rhs) = default;
	
	
public:
	
	//! @{ mapped file like interface 
	bool is_open() const {
		return m_cur.is_valid();
	}
	
	//! Open the file at the given path and create a read-only memory mapped region from it
	//! \param path to file to open
	//! \param length amount of bytes to map
	//! \param offset offset into the file. Internally, the offset is adjusted to be divisable by the system's
	//! page size.
	//! \throw system error
	template <typename Path>
	void open(const Path& path, size_type length = parent_type::max_length, stream_offset offset = 0)
	{
		if (is_open()) {
			close();
		}
		if (!m_cur.is_associated() || m_cur.path() != path) {
			m_cur = m_man.make_cursor(path);
		}
		do_open(length, offset);
	}
	
	//! Special version allowing to quickly open a file from a cursor
	//! \param cursor cursor initialized to a path
	//! \throw ios_base::failure if cursor is not associated
	//! \note see first open version for additional argument descriptions
	void open(const typename memory_manager_type::cursor& cursor, size_type length = parent_type::max_length, stream_offset offset = 0)
	{
		if (!cursor.is_associated()) {
			throw std::ios_base::failure("cannot open from an un-initialized cursor");
		}
		
		if (is_open()) {
			close();
		}
		
		m_cur = cursor;
		do_open(length, offset);
	}
	
	
	//! \return total size of the file on disk
	size_type file_size() const {
		if (!is_open()) {
			return 0;
		}
		return m_cur.file_size();
	}
	
	//! \return the system's page size
	static int alignment() {
		return memory_manager_type::page_size();
	}
	
	//! \return reference to our memory manager
	//! \note we are const as the manager cannot interfere with our constness at all
	ManagerType& manager() const {
		return const_cast<ManagerType&>(m_man);
	}
	
	//! \return our cursor initialized to point to our file
	//! \note the cursor is a handle to a region within a memory mapped file.
	//! Use this method to create a copy allowing you free random access on your own.
	const cursor_type& cursor() const {
		return m_cur;
	}
	
	//! @} end mapped file like interface
	
	//! @{ \name Device Interface
	
	void close() {
		if (is_open()) {
			m_cur.unuse_region();
			this->m_ofs = 0;
			this->m_nb = 0;
			this->m_size = 0;
		}
	}
	
	std::streamsize read(char_type* s, std::streamsize n) {
		if (this->m_nb == 0){
			return -1;	// eof
		}
		
		std::streamsize br = 0; // bytes read
		while ((br != n) & (this->m_nb != 0)) {
			// Could use br as size, but it doesn't really matter as we have to verify the acutal cursor size anyway
			if (!m_cur.use_region(this->m_ofs, n).is_valid()) {
				// end of file
				assert(this->m_nb == 0);
				return br;
			}

			size_t bytes_to_copy =	std::min(m_cur.size(), std::min(static_cast<size_type>(n - br), this->m_nb));
			std::memcpy(s, m_cur.begin(), bytes_to_copy);
			br += bytes_to_copy;
			s += bytes_to_copy;
			this->m_nb -= bytes_to_copy;
			this->m_ofs += bytes_to_copy;
		}// while there are bytes to read*/
		return br;
	}
	
	std::streampos seek(stream_offset off, std::ios_base::seekdir way) {
		if (!is_open()) {
			throw std::ios_base::failure("Cannot seek a closed device");
		}
		
		parent_type::seek(off, way);
		
		// use the region to assure we free the current one, and require the one at the new offset
		m_cur.use_region(this->m_ofs, this->m_nb);
		return this->m_ofs;
	}

	//! @} end device interface
	
};


GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_DB_SLIDING_MMAP_DEVICE_HPP
