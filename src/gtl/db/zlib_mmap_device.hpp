#ifndef GTL_DB_zlib_file_source_HPP
#define GTL_DB_zlib_file_source_HPP

#include <gtl/config.h>
#include <gtl/util.hpp>

#include <boost/iostreams/filter/zlib.hpp>	// for zlib params
#include <gtl/db/sliding_mmap_device.hpp>
#include <zlib.h>

#include <exception>
#include <memory>
#include <limits>
#include <ios>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace io = boost::iostreams;
using boost::iostreams::stream_offset;
using boost::iostreams::zlib_params;


/** Exception thrown for all zlib related errors. It stores the respected status in its status variable
  */
struct zlib_error :		public std::exception, 
						public streaming_exception
{
	int status;	//!< zlib status we encountered
	
	zlib_error(int status, const char* msg = nullptr)
	    : status(status)
	{
		stream() << (msg ? msg : "ZLib encountered an error") << " (Status = " << status << ")";
	}
	
	const char* what() const throw() {
		return streaming_exception::what();
	}
};


/** Simple stream wrapper to facilitate using the stream structure, to initialize and deinitialize it
  * \note Althogh zlib_params include the crc32 check, this flags is not currently supported
  * \note for now we only support the default allocation methods
  */
class zlib_stream : public z_stream
{
public:
	enum class Mode : uchar {
		Compress, 
		Decompress
	};
	
protected:
	Mode m_mode;
	
	void init(Mode mode, const zlib_params& params)
	{
		zalloc = nullptr;
		zfree = nullptr;
		opaque = nullptr;
		m_mode = mode;
		                 
		int err;
		int window_bits = params.noheader ? -params.window_bits : params.window_bits;
		if (mode == Mode::Compress) {
			err = deflateInit2(this, params.level, params.method, window_bits, params.mem_level, params.strategy);
		} else {
			// Needs to be set with an actual pointer, or 0 to inidcate the actual initialization
			// should be delayed until the first decompression call
			next_in = nullptr;
			err = inflateInit2(this, window_bits);
		}
		check(err);
	}
	
private:
	zlib_stream(zlib_stream&&);
	
public:
	zlib_stream(Mode mode, const zlib_params& params = io::zlib::default_compression) {
		init(mode, params);
	}
	
	zlib_stream(const zlib_stream& rhs) {
		if (m_mode == Mode::Compress) {
			deflateEnd(this);
			deflateCopy(this, const_cast<zlib_stream*>(&rhs));
		} else {
			inflateEnd(this);
			inflateCopy(this, const_cast<zlib_stream*>(&rhs));
		}
	}
	
	~zlib_stream() {
		if (m_mode == Mode::Compress) {
			inflateEnd(this);
		} else {
			deflateEnd(this);
		}
	}
	
public:
	//! check the given zlib error code and produce the respective exception
	inline static void check(int err, const char* msg = nullptr) {
		switch (err) {
			case Z_OK:
			case Z_STREAM_END:
				return;
			case Z_MEM_ERROR:
				throw std::bad_alloc();
			default:
				throw zlib_error(err, msg);
		}
	}
	
	//! Prepare the stream for the next input or output operation by setting the respective source 
	//! and destination memory areas
	inline int prepare(const char* src_begin, const char* src_end, char* dest_begin, char* dest_end) {
		assert(src_end - src_begin <= std::numeric_limits<uint32>::max());
		assert(dest_end - dest_begin <= std::numeric_limits<uint32>::max());
		
		next_in = reinterpret_cast<uchar*>(const_cast<char*>(src_begin));
		avail_in = static_cast<uint32>(src_end - src_begin);
		next_out = reinterpret_cast	<uchar*>(dest_begin);
		avail_out= static_cast<uint32>(dest_end - dest_begin);
	}
	
	//! Change the mode of the stream to the given one. Does nothing if we are at the mode already, 
	//! and deinitializes the existing stream as required.
	inline void set_mode(Mode new_mode, const zlib_params& params = io::zlib::default_compression) {
		if (new_mode == m_mode) {
			return;
		}
		this->~zlib_stream();
		init(new_mode, params);
	}
	
	//! \return mode
	inline Mode mode() const {
		return m_mode;
	}
	
	//! Reset the stream to allow a now compression/decompression operation
	inline void reset() {
		if (m_mode == Mode::Compress) {
			deflateReset(this);
		} else {
			inflateReset(this);
		}
	}
	
	//! Perform a compression step and return the error code
	inline int compress(int flush = true) {
		assert(m_mode == Mode::Compress);
		return deflate(this, flush);
	}
	
	//! Perform the decompression and return the error code
	inline int decompress(int flush = false) {
		assert(m_mode == Mode::Decompress);
		return inflate(this, flush ? Z_FINISH : Z_NO_FLUSH);
	}
};

/** Base class with common initialization routines and members to be used by both input and output devices
  * \todo implementation
  */
template <class ManagerType>
class zlib_device_base
{
public:
	typedef ManagerType													memory_manager_type;
	typedef typename memory_manager_type::mapped_file_source::char_type	char_type;
	typedef typename memory_manager_type::size_type						size_type;
	
protected:
	struct category_base : 
	        public io::device_tag,
	        public io::closable_tag
	{};

protected:
	zlib_stream		m_stream;
	
protected:
	zlib_device_base(zlib_stream::Mode mode, const zlib_params& params = io::zlib::default_compression)
		: m_stream(mode, params)
	{}
	
};


/** \brief Implements a memory mapped device which reads or writes a stream using zlib.
  * When reading, it assumes a compressed stream which will be decompressed when reading. Decompression
  * will always be performed in a certain configurable buffer size, decompressed bytes are placed in a buffer, 
  * which will be read by the client. Once it is depleted, the next batch of bytes will be decompressed to fill the buffer.
  * \todo implementation - this could be useful for the loose object database
  */
template <class ManagerType>
class zlib_file_source :	public zlib_device_base<ManagerType>,
							protected managed_mapped_file_source<ManagerType>
{
public:
	typedef ManagerType													memory_manager_type;
	typedef typename memory_manager_type::cursor						cursor_type;
	typedef zlib_file_source<memory_manager_type>						this_type;
	typedef zlib_device_base<memory_manager_type>						zlib_parent_type;
	typedef managed_mapped_file_source<ManagerType>						file_parent_type;
	
	typedef typename memory_manager_type::mapped_file_source::char_type	char_type;
	typedef typename memory_manager_type::size_type						size_type;

public:
	
	struct category :	public zlib_parent_type::category_base,
				        public io::input
	{};
	
protected:
	
	int										m_stat;			//!< last zlib status
	char_type								m_buf[this_type::total_buf_size()];
	zlib_file_source(this_type&& source);
	
	static size_t total_buf_size() {
		static_assert(memory_manager_type::page_size() >= 2048, "page size too small on this platform");
		return memory_manager_type::page_size();
	}
	
	//! Offset to the read-section of our buffer. It contains compressed input bytes
	static size_t iofs() {
		return 0;
	}
	
	//! pointer to one past the last valid byte in the buffer
	inline char_type* iend() {
		return m_buf + iofs() + (ilen() - iremain());
	}
	
	//! remaining amount of bytes in input buffer
	inline size_t iremain() const {
		return this->m_stream.avail_in;
	}
	
	inline bool ifull() const {
		return iremain() == 0;
	}
	
	//! size of the input buffer
	static size_t ilen() {
		return oofs();
	}
	
	//! Offset to the write-section of our buffer. It contains the decompressed bytes
	static size_t oofs() {
		return 1024;
	}
	
	inline char_type* oend() {
		return m_buf + oofs() + (olen() - oremain());
	}
	
	inline size_t oremain() const {
		return this->m_stream.avail_out;
	}
	
	//! \return pointer to region which was not copied to the client yet
	//! \note it assumes that total_in was manipulated to mark the spot
	inline char_type* ofrom() {
		return m_buf + oofs() + this->m_stream.total_out;
	}
	
	inline bool ofull() const {
		return oremain() == 0;
	}
	
	//! size of the write (output) buffer
	static size_t olen() {
		return total_buf_size();
	}
	
public:
	
	//! Initialize this instance with a manager and zlib configuration paramters
	//! \note only a fraction of the params are useful in decompression mode
	zlib_file_source(memory_manager_type& manager,
	                 const zlib_params& params = io::zlib::default_compression)
		: zlib_parent_type(zlib_stream::Mode::Decompress, params)
	    , file_parent_type(manager)
	    , m_stat(Z_STREAM_END)
	{}
	
	//! required by boost 
	zlib_file_source(const zlib_file_source& rhs) = default;
	
public:
	
	//! @{ mapped file like interface 
	bool is_open() const {
		return file_parent_type::is_open();
	}
	
	//! Open the given file at the given offset with the given length. The offset and length identify the 
	//! compressed stream that we should read in as maximum. If the stream ends before we reach the physical 
	//! end of our mapping, we report eof accordingly. If the stream doesn't include the logical end of the zlib stream,
	//! we report an error. Hence you should provide a worst-case length which is an uncompressed stream + the header of a zip 
	//! archive. If unsure, just map the whole file, the underlying sliding memory mapped file will assure that this is efficient.
	//! \param path to file to open
	//! \param length amount of compressed bytes to map
	//! \param offset offset into the file at which the compressed stream starts.
	//! \throw system error
	template <typename Path>
	void open(const Path& path, size_type length = file_parent_type::max_length, stream_offset offset = 0)
	{
		file_parent_type::open(path, length, offset);
		m_stat = Z_OK;
	}
	
	//! \return true if we reached the end of our mapping
	bool eof() const {
		return m_stat == Z_STREAM_END;
	}
	
	//! \return total size of the file on disk
	size_type file_size() const {
		return file_parent_type::file_size();
	}
	
	//! \return our current absolute offset into the compressed stream
	stream_offset tellg() const {
		return file_parent_type::tellg();
	}
	
	//! \return reference to our memory manager
	ManagerType& manager() const {
		return file_parent_type::manager();
	}
	
	//! \return our cursor initialized to point to our file
	const cursor_type& cursor() const {
		return file_parent_type::cursor();
	}
	
	//! @} end mapped file like interface
	
	//! @{ \name Device Interface
	
	void close() {
		if (is_open()) {
			inflateReset(&this->m_stream);
			m_stat = Z_STREAM_END;
			file_parent_type::close();
		}
	}
	
	//! Read the given amount of uncompressed bytes - consumes
	//! any required amount of compressed bytes on the way
	//! \throw zlib_error indicating the error in question
	std::streamsize read(char_type* s, std::streamsize n) {
		if (this->eof()) {
			return -1;
		}
		
		std::streamsize bw = 0;			// amount of bytes written to user output
		while (m_stat != Z_STREAM_END && bw != n) {
			/*std::streamsize bl = file_parent_type::read(&buf[0], ManagerType::page_size());
			
			// it is possible that we hit the end of compressed bytes, before the stream is done
			// This is an error, as we can never finish reading the stream, and we indicate this.
			// But we leave ourselves in a valid state
			if (bl == -1) {
				// provide as many bytes as possible from our remaining buffer
				this->close();
				throw zlib_error(Z_BUF_ERROR);
			}
			
			decltype(this->m_stream.total_out) prev_total_out = this->m_stream.total_out;
			this->m_stream.total_out = 0;				*/
			
			// m_stream.next_in
			return -1;
		}// read-loop
		return -1;	// eof
	}
	//! @} end device interface
};


GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_DB_zlib_file_source_HPP
