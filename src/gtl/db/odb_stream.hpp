#ifndef GTL_ODB_STREAM_HPP
#define GTL_ODB_STREAM_HPP
/** \defgroup ODBItem Object Database Items
  * Items as part of the object database which allow streamed access to an object
  */

#include <gtl/config.h>
#include <stddef.h>
#include <exception>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/null.hpp>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace io = boost::iostreams;

/** Basic traits type to further specify properties of objects stored in the database
  * Types using odb facilities need to fully specialize this trait for their respective object
  * database type
  * For brevity, the traits contain a small set of functions to handle serialization features.
  * \ingroup ODBItem
  */
struct odb_object_traits
{	
	//! Type allowing to classify the stored object
	typedef int object_type;
	//! Type used to return values by reference
	typedef void*& output_reference_type;
	//! Type used when objects are made available to a function by references
	typedef void* input_reference_type;
	//! Type used to represent the object in a serialized format from which data can be read
	typedef io::stream<typename io::basic_null_source<uchar> > istream_type;
	//! Stream to write the object to during serialization
	typedef io::stream<typename io::basic_null_sink<uchar> > ostream_type;
	//! stream suitable to read and write data.
	typedef io::stream<typename io::basic_null_device<uchar, io::seekable> > iostream_type;
	
	//! Serialize the given objectinto the given stream.
	static void serialize(input_reference_type object, ostream_type& ostream){}
	
	//! deserialize the data contained in istream to recreate the object it represents
	static void deserialize(output_reference_type out, istream_type& istream) throw(std::exception) {}
};


/** \brief A handle to a stream of object information for reading
  * \ingroup ODBItem
  * Streams are simple structures which only know their data size, 
  * their type and their serialized data stream. It provides methods 
  * to deserialize the stream into an object.
  * An ostream is considered the output of an object database
  */
template <class ObjectTraits>
struct odb_ostream
{
	typedef ObjectTraits object_traits;
	
	//! \return type of the object which helps to figure out how to interpret its data
	typename object_traits::object_type type() const;
	
	//! \return the size of the object in a format that relates to its storage requirements
	size_t size() const;
	
	//! \return stream object for reading of data
	typename object_traits::istream_type stream() const;
	
	//! construct an object from the deserialized stream and store it in the output reference
	//! \note uses exceptions to indicate failure (i.e. out of memory, corrupt stream)
	void deserialize(typename object_traits::output_reference_type out) const throw(std::exception)
	{
		object_traits::deserialize(out, stream());
	}
};

		
GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_STREAM_HPP
