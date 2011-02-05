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
  * \ingroup ODBItem
  */
template <class ObjectType>
struct odb_output_object_traits
{	
	//! Type allowing to classify the stored object
	typedef ObjectType object_type;
	//! Type returned from the deserialization process
	typedef void* return_type;
	//! Type used to return values by reference
	typedef void*& output_reference_type;
	//! Type used to represent the object in a serialized format
	typedef io::stream<io::stream_buffer<typename io::basic_null_source<uchar> > > stream_type;
};


/** \brief A handle to a stream of object information for reading
  * \ingroup ODBItem
  * Streams are simple structures which only know their data size, 
  * their type and their serialized data stream. It provides methods 
  * to deserialize the stream into an object.
  * Memory allocation is left to the user entirely.
  */
template <class ObjectType>
struct odb_ostream
{
	typedef ObjectType object_type;
	typedef odb_output_object_traits<ObjectType> traits_type;
	
	//! \return type of the object which helps to figure out how to interpret its data
	object_type type() const;
	
	//! \return the size of the object in a format that relates to its storage requirements
	size_t size() const;
	
	//! \return stream object for reading of data
	typename traits_type::stream_type stream() const;
	
	//! \return deserialized object
	//! \note may use return type to indicate failure, or may throw in that case
	typename traits_type::return_type deserialize() const throw(std::exception);
	
	//! construct an object from the deserialized stream and store it in the output reference
	//! \note uses exceptions to indicate failure (i.e. out of memory, corrupt stream)
	void deserialize(typename traits_type::output_reference_type out) const throw(std::exception);
};

		
GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_STREAM_HPP
