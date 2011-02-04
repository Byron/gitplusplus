#ifndef GTL_ODB_STREAM_HPP
#define GTL_ODB_STREAM_HPP
/** \defgroup ODBStream Object Streams
  * Streams as part of the object database which allow streamed access to an object
  */

#include <gtl/db/odb_obj.hpp>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

/** \brief A handle to a stream of object information for reading
  * \ingroup ODBStream
  * Objects are simple structures which only know their data size, 
  * their type and their value.
  */
template <class TypeID, class StreamType, class SizeType=size_t>
struct odb_ostream : public odb_info<TypeID, SizeType>
{
	typedef StreamType stream_type;

	//! \return stream object for reading of data
	stream_type stream() const;
};

		
GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_STREAM_HPP
