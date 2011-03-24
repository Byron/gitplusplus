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


/** Simple stream wrapper to facilitate using the stream structure, to initialize, deinitialize and copy it
  * A stream instance is in one of 3 modes: either setup  for compression, decompression or not set.
  * It may only be used if it is set to a mode which is not None. In all other cases it will fail in debug mode.
  * \note Althogh zlib_params include the crc32 check, this flags is not currently supported
  * \note for now we only support the default allocation methods
  * \note we could get rid of the mode variable (1 byte) by templating this class, so we would loose the runtime
  * adjutability of the compression mode. For now, we keep it as it shouldn't make any difference.	
  */
class zlib_stream : public z_stream
{
public:
	enum class Mode : uchar {
		Compress, 
		Decompress, 
		None
	};
	
protected:
	Mode m_mode;
	
	void init(Mode mode, const zlib_params& params)
	{
		zalloc = nullptr;
		zfree = nullptr;
		opaque = nullptr;
		// init doesn't null the next_out and avail_out, so we do it not to confuse anybody
		next_out = next_in = nullptr;
		avail_out = avail_in = 0;
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
	
	//! a custom copy constructor
	inline void clone_from(const zlib_stream& rhs) {
		m_mode = rhs.m_mode;
		
		switch(m_mode){
		case Mode::Compress: deflateCopy(this, const_cast<zlib_stream*>(&rhs)); break;
		case Mode::Decompress: inflateCopy(this, const_cast<zlib_stream*>(&rhs)); break;
		case Mode::None: break;
		}
	}
	
private:
	zlib_stream(zlib_stream&&);
	
public:
	//! Initialize the stream. If the mode is not None, the params become effective.
	//! otherwise initialize your stream later using set_mode(...)
	zlib_stream(Mode mode = Mode::None, const zlib_params& params = io::zlib::default_compression)
	{
		if (mode != Mode::None) {
			init(mode, params);
		} else {
			m_mode = mode;
		}
	}
	
	zlib_stream(const zlib_stream& rhs) {
		this->clone_from(rhs);
	}
	
	~zlib_stream() {
		switch(m_mode){
		case Mode::Compress: deflateEnd(this); break;
		case Mode::Decompress: inflateEnd(this); break;
		case Mode::None: break;
		}
	}
	
	zlib_stream& operator = (const zlib_stream& rhs) {
		this->~zlib_stream();
		this->clone_from(rhs);
		return *this;
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
	inline void prepare(const char* src_begin, const char* src_end, char* dest_begin, char* dest_end) {
		assert(m_mode != Mode::None);
		assert(src_end - src_begin <= std::numeric_limits<uint32>::max());
		assert(dest_end - dest_begin <= std::numeric_limits<uint32>::max());
		
		next_in = reinterpret_cast<uchar*>(const_cast<char*>(src_begin));
		avail_in = static_cast<uint32>(src_end - src_begin);
		next_out = reinterpret_cast	<uchar*>(dest_begin);
		avail_out= static_cast<uint32>(dest_end - dest_begin);
	}
	
	//! Change the mode of the stream to the given one. Does nothing if we are at the mode already, 
	//! and deinitializes the existing stream as required.
	//! if the Mode is none, the stream will be deinitialized and frees its allocated memory
	inline void set_mode(Mode new_mode, const zlib_params& params = io::zlib::default_compression) {
		if (new_mode == m_mode) {
			return;
		}
		
		this->~zlib_stream();
		
		if (new_mode != Mode::None) {
			init(new_mode, params);
		} else {
			m_mode = new_mode;	// None
		}
	}
	
	//! \return mode
	inline Mode mode() const {
		return m_mode;
	}
	
	//! Reset the stream to allow a now compression/decompression operation
	inline void reset() {
		switch(m_mode){
		case Mode::Compress: deflateReset(this); break;
		case Mode::Decompress: inflateReset(this); break;
		case Mode::None: break;
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



/** \brief Implements a memory mapped device which reads or writes a stream using zlib.
  * When reading, it assumes a compressed stream which will be decompressed when reading. Decompression
  * will always be performed in a certain configurable buffer size, decompressed bytes are placed in a buffer, 
  * which will be read by the client. Once it is depleted, the next batch of bytes will be decompressed to fill the buffer.
  * \todo implementation - this could be useful for the loose object database
  */
template <class ManagerType>
class zlib_file_source : protected managed_mapped_file_source<ManagerType>
{
public:
	typedef ManagerType													memory_manager_type;
	typedef typename memory_manager_type::cursor						cursor_type;
	typedef zlib_file_source<memory_manager_type>						this_type;
	typedef managed_mapped_file_source<ManagerType>						file_parent_type;
	typedef typename memory_manager_type::mapped_file_source::char_type	char_type;
	typedef typename memory_manager_type::size_type						size_type;
	
	static const size_t buf_size = 1024;				//!< internal static buffer size

public:
	
	struct category :	public io::device_tag,
						public io::closable_tag,
				        public io::input
	{};
	
protected:
	
	
	int										m_stat;			//!< last zlib status
	zlib_stream								m_stream;		//!< inflate stream
	char_type								m_buf[buf_size];
	
protected:
	zlib_file_source(this_type&& source);	// disabled
	
	
	//! \return first byte of the input buffer
	inline uchar* ibegin() {
		return reinterpret_cast<uchar*>(m_buf);
	}
	
	//! size of the input buffer
	static std::streamsize ilen() {
		return static_cast<std::streamsize>(buf_size);
	}
	
	
public:
	
	//! Initialize this instance, optionally open the given region right away
	zlib_file_source(const cursor_type* cursor = nullptr, size_type length=file_parent_type::max_length, stream_offset offset = 0)
	    : m_stat(Z_STREAM_END)
	    
	{
		if (cursor) {
			open(*cursor, length, offset);
		}
	}
	
	//! required by boost 
	zlib_file_source(const this_type& rhs)
	    : file_parent_type(rhs)
		, m_stat(rhs.m_stat)
		, m_stream(rhs.m_stream)
	    
	{
		std::memcpy(m_buf, rhs.m_buf, buf_size);
	}
	
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
	void open(const cursor_type& cursor, 
	          size_type length = file_parent_type::max_length, stream_offset offset = 0)
	{
		file_parent_type::open(cursor, length, offset);
		if (m_stream.mode() != zlib_stream::Mode::Decompress) {
			m_stream.set_mode(zlib_stream::Mode::Decompress);
		}
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
			m_stream.set_mode(zlib_stream::Mode::None);
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
		if (n > std::numeric_limits<uint32>::max()) {
			throw zlib_error(Z_STREAM_ERROR, "Cannot currently handle read sizes larger than 2**32-1");
		}
		
		// fill output buffer
		assert(m_stream.avail_out == 0);
		m_stream.avail_out = static_cast<uint32>(n);
		m_stream.next_out = reinterpret_cast<uchar*>(s);
		
		
		while (m_stat != Z_STREAM_END && m_stream.avail_out) {
			// Is there still space in the input buffer ? If not, replenish it
			if (m_stream.avail_in == 0) {
				// read n bytes, assuming worst case compression ration. This honors the desired amount of 
				// output bytes, but also assures we don't try sizes which are too small for zip
				std::streamsize ibr = file_parent_type::read(reinterpret_cast<char_type*>(ibegin()), 
				                                             std::min(ilen(), std::max(n, std::streamsize(128))));
				if (ibr == -1) {
					// it is possible that we hit the end of compressed bytes, before the stream is done
					// This is an error, as we can never finish reading the stream, and we indicate this.
					// But we leave ourselves in a valid state
					this->close();
					assert(!is_open());
					throw zlib_error(Z_BUF_ERROR, "Zlib source buffer depleted before stream end");
				}
				m_stream.next_in = ibegin();
				m_stream.avail_in = static_cast<uint32>(ibr);
			}
			
			m_stat = m_stream.decompress(true);
			
			switch(m_stat) {
			case Z_OK:				// fallthrough
			case Z_STREAM_END:		// fallthrough
			case Z_BUF_ERROR:
				break;
			default: throw zlib_error(m_stat, m_stream.msg);
			}
		}// read-loop
		
		return n - static_cast<std::streamsize>(m_stream.avail_out);
	}
	//! @} end device interface
};


GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_DB_zlib_file_source_HPP
