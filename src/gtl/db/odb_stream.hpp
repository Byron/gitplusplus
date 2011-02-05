#ifndef GTL_ODB_STREAM_HPP
#define GTL_ODB_STREAM_HPP
/** \defgroup ODBStream Object Streams
  * Streams as part of the object database which allow streamed access to an object
  */

#include <gtl/config.h>
#include <stddef.h>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN



/** \brief structure providing information about an object
  * \ingroup ODBObject
  * 
  * The object_type identifies the type of this object, which allows
  * to determine how to interpret the stream data.
  */
template <class ObjectType, class SizeType=size_t>
struct odb_info
{
	typedef ObjectType object_type;
	typedef SizeType size_type;

	//! \return type of the object which helps to figure out how to interpret its data
	object_type type() const;
	//! \return the size of the object in a format that relates to its storage requirements
	size_type size() const;
};


/** \brief A handle to a stream of object information for reading
  * \ingroup ODBStream
  * Objects are simple structures which only know their data size, 
  * their type and their value.
  */
template <class ObjectType, class StreamType, class SizeType=size_t>
struct odb_ostream : public odb_info<ObjectType, SizeType>
{
	typedef StreamType stream_type;

	//! \return stream object for reading of data
	stream_type stream() const;
};

		
GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_STREAM_HPP
