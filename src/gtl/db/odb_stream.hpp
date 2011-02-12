#ifndef GTL_ODB_STREAM_HPP
#define GTL_ODB_STREAM_HPP
/** \defgroup ODBObject Object Database Items
  * Items as part of the object database which allow streamed access to an object
  */

#include <gtl/config.h>
#include <stdint.h>
#include <exception>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/null.hpp>
#include <type_traits>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace io = boost::iostreams;

/** Basic traits type to further specify properties of objects stored in the database
  * Types using odb facilities need to fully specialize this trait for their respective object
  * database type
  * For brevity, the traits contain a small set of functions to handle serialization features.
  * \ingroup ODBObject
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
	
	//! generator interface compatible type which is used to generate keys from the contents of streams
	 
	//! @{ \addtogroup Policy Policy Functions
	//! Serialize the given objectinto the given stream.
	static void serialize(input_reference_type object, ostream_type& ostream){}
	
	//! deserialize the data contained in istream to recreate the object it represents
	static void deserialize(output_reference_type out, istream_type& istream) throw(std::exception) {}
	//! @} 
};


/** \ingroup ODBObject
  * Element living in an object database. It provides access to the objects properties
  * being its type, its uncompressed size in bytes, and a stream to access the data.
  * \param ObjectTraits traits struct compatible to odb_object_traits
  * \param Stream seekable and readable stream which allows access to the data. It is dependent 
  *	on the underlying database implementation.
  */
template <class ObjectTraits, class Stream>
struct odb_basic_object
{
	typedef ObjectTraits object_traits;
	typedef Stream stream_type;
	
	//! \return type of the object which helps to figure out how to interpret its data
	typename object_traits::object_type type() const;
	
	//! \return the size of the uncompressed object stream in a format that relates to its storage 
	//! requirements, by default in bytes.
	//! \note as the object may be stored compressed within its stream, the amount of data in the stream
	//! may differ.
	uint64_t size() const {
		return 0;
	}
	
	//! \return data stream which contains the serialized object. Must be readable
	stream_type stream() const;
	
};

/** \ingroup ODBObject
  * \brief type used as input to object databases. It optionally provides access to 
  * a pointer to the key type, which if not 0, provides key-information of the object at hand.
  * If the key is 0, one will be generated using the key generator of the object traits
  */
template <class ObjectTraits, class Stream, class Key>
struct odb_input_object : public odb_basic_object<ObjectTraits, Stream>
{
	typedef Key key_type;
	
	/** \return pointer to key designated to the object, or 0 if a key does not yet exists
	  */
	const typename std::add_pointer<const Key>::type key() const {
		return 0;
	}
};


/** \brief A handle to a stream of object information for reading
  * \ingroup ODBObject
  * Streams are simple structures which only know their data size, 
  * their type and their serialized data stream. It provides methods 
  * to deserialize the stream into an object.
  * An ostream is considered the output of an object database
  */
template <class ObjectTraits, class Stream>
struct odb_output_object : public odb_basic_object<ObjectTraits, Stream>
{
	typedef ObjectTraits object_traits;
	typedef Stream stream_type;
	
	//! construct an object from the deserialized stream and store it in the output reference
	//! \note uses exceptions to indicate failure (i.e. out of memory, corrupt stream)
	void deserialize(typename object_traits::output_reference_type out) const throw(std::exception)
	{
		object_traits::deserialize(out, this->stream());
	}
};

		
GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_STREAM_HPP
