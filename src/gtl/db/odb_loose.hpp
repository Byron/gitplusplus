#ifndef GTL_ODB_LOOSE_HPP
#define GTL_ODB_LOOSE_HPP

#include <gtl/config.h>

#include <boost/iostreams/filter/zlib.hpp>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace io = boost::iostreams;


/** Default traits for the loose object database.
  * \note by default, it uses the zlib compression library which usually involves
  * external dependencies which must be met for the program to run.
  */
struct odb_loose_traits
{
	//! type suitable to be used for compression within the 
	//! boost iostreams filtering framework.
	typedef io::zlib_compressor compression_filter_type;
	
	//! type compatible to the boost filtering framework to decompress what 
	//! was previously compressed.
	typedef io::zlib_decompressor decompression_filter_type;
	
	//! amount of hash-characters used as directory into which to put the loose object files.
	//! One hash character will translate into two hexadecimal characters
	static uint32_t num_prefix_characters = 1;
	
	//! if true, the header will not be compressed, only the following datastream. This can be advantageous
	//! if you want to reuse that exact stream some place else.
	static bool compress_header = true;
};


/** \brief Model a fully paremeterized database which stores objects as compressed files on disk.
  *
  * The files are named after their key within the database as transformed from binary to hex. The first
  * portion of the resulting string is used as directory, the rest represents the file's name.
  *
  * The files themselves are compressed containers which contain information about the object type, the uncompressed 
  * size as well as the data stream. The compression applies to the header information as well as the stream itself
  * by default, but this behaviour may be changed easily using traits.
  */


GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_LOOSE_HPP
