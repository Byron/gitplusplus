#include <git/config.h>
#include "git/obj/blob.h"

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

GIT_NAMESPACE_BEGIN
		
namespace io = boost::iostreams;

Blob::Blob()
	: Object(Object::Type::Blob)
{}

git_basic_ostream& operator << (git_basic_ostream& stream, const Blob& inst)
{
	// just copy the data into the target stream
	io::basic_array_source<typename Blob::char_type> source_stream(&inst.data()[0], inst.data().size());
	io::copy(source_stream, stream);
	
	return stream;
}

git_basic_istream& operator >> (git_basic_istream& stream, Blob& inst)
{
	io::back_insert_device<Blob::data_type> insert_stream(inst.data());
	io::copy(stream, insert_stream);
	
	return stream;
}

GIT_NAMESPACE_END
