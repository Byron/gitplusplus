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
	
	zlib_error(int status)
	    : status(status)
	{}
	
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
	enum Mode {
		Compress, 
		Decompress
	};
	
protected:
	zlib_stream(Mode mode, const zlib_params& params)
	{
		zalloc = nullptr;
		zfree = nullptr;
		opaque = nullptr;
		                 
		int err;
		int window_bits = params.noheader ? -params.window_bits : params.window_bits;
		if (mode == Compress) {
			err = deflateInit2(this, params.level, params.method, window_bits, params.mem_level, params.strategy);
		} else {
			// Needs to be set with an actual pointer, or 0 to inidcate the actual initialization
			// should be delayed until the first decompression call
			next_in = nullptr;
			err = inflateInit2(this, window_bits);
		}
		check(err);
	}
	
public:
	//! check the given zlib error code and produce the respective exception
	inline static void check(int err) {
		switch (err) {
			case Z_OK:
			case Z_STREAM_END:
				return;
			case Z_MEM_ERROR:
				throw std::bad_alloc();
			default:
				throw zlib_error(err);
		}
	}
	
	//! Prepare the stream for the next input or output operation by setting the respective source 
	//! and destination memory areas
	inline int prepare(const char* src_begin, const char* src_end, char* dest_begin, char* dest_end) {
		next_in = reinterpret_cast<uchar*>(const_cast<char*>(src_begin));
		assert(src_end - src_begin <= std::numeric_limits<uint32>::max());
		avail_in = static_cast<uint32>(src_end - src_begin);
		next_out = reinterpret_cast	<uchar*>(dest_begin);
		assert(dest_end - dest_begin <= std::numeric_limits<uint32>::max());
		avail_out= static_cast<uint32>(dest_end - dest_begin);
	}
};

/** \brief zlib stream to be used for compression
  */
class zlib_compressor : public zlib_stream
{
public:
	zlib_compressor(const zlib_params& params = io::zlib::default_compression)
	    : zlib_stream(Compress, params)
	{}
	
	~zlib_compressor() {
		deflateEnd(this);
	}
	
public:
	
	void reset() {
		deflateReset(this);
	}
	
	//! Perform a compression step and return the error code
	int compress(int flush = true) {
		return deflate(this, flush);
	}
	
};

/** \brief zlib stream for decompression
  */
class zlib_decompressor : public zlib_stream
{
public:
	zlib_decompressor(const zlib_params& params = io::zlib::default_compression)
	    : zlib_stream(Decompress, params)
	    
	{
	}
	
	~zlib_decompressor() {
		inflateEnd(this);
	}
	
public:
	void reset() {
		inflateReset(this);
	}
	
	//! Perform the decompression and return the error code
	int decompress(int flush = false) {
		return inflate(this, flush);
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
	        public io::closable_tag,
	        public io::direct_tag
	{};

protected:
	zlib_stream		m_stream;
	
protected:
	zlib_device_base(zlib_stream::Mode mode, const zlib_params& params = io::zlib::default_compression)
	{}
	
};


/** \brief Implements a memory mapped device which reads or writes a stream using zlib.
  * When reading, it assumes a compressed stream which will be decompressed when reading. Decompression
  * will always be performed in a certain configurable buffer size, decompressed bytes are placed in a buffer, 
  * which will be read by the client. Once it is depleted, the next batch of bytes will be decompressed to fill the buffer.
  * \todo implementation
  */
template <class ManagerType>
class zlib_file_source :	public zlib_device_base<ManagerType>,
							protected managed_mapped_file_source<ManagerType>
{
public:
	typedef ManagerType													memory_manager_type;
	typedef typename memory_manager_type::cursor						cursor_type;
	typedef zlib_device_base<memory_manager_type>						zlib_parent_type;
	typedef managed_mapped_file_source<ManagerType>						file_parent_type;
	
	typedef typename memory_manager_type::mapped_file_source::char_type	char_type;
	typedef typename memory_manager_type::size_type						size_type;

public:
	
	struct category :	public zlib_parent_type::category_base,
				        public io::input
	{};
	
private:
	zlib_file_source(zlib_file_source&& source);
	
public:
	
	zlib_file_source(memory_manager_type& manager,
	                 const zlib_params& params = io::zlib::default_compression)
		: zlib_parent_type(zlib_stream::Mode::Decompress, params)
	    , file_parent_type(manager)
	{}
	
	// zlib_file_source(const zlib_file_source& rhs) = default;
	
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
	//! \param length amount of compresed bytes to map
	//! \param offset offset into the file at which the compressed stream starts.
	//! \throw system error
	template <typename Path>
	void open(const Path& path, size_type length = file_parent_type::max_length, stream_offset offset = 0)
	{
		file_parent_type::open(path, length, offset);
		
		// todo own initialization
	}
	
	//! \return true if we reached the end of our mapping
	bool eof() const {
		// TODO
		return true;
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
		file_parent_type::close();
		// TODO our deinit
	}
	
	std::streamsize read(char_type* s, std::streamsize n) {
		// TODO
		return -1;	// eof
	}
	//! @} end device interface
};


GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_DB_zlib_file_source_HPP
