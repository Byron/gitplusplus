#ifndef GIT_MODEL_ODB_STREAM_HPP
#define GIT_MODEL_ODB_STREAM_HPP
/** \defgroup ODBStream Streams as part of the object database
  * Classes modeling the object concept
  */

#include <git/model/db/odb_obj.hpp>

GIT_HEADER_BEGIN
GIT_MODEL_NAMESPACE_BEGIN

/** \class odb_ostream
  * \brief A handle to a stream of object information for reading
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

		
GIT_MODEL_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_MODEL_ODB_STREAM_HPP
