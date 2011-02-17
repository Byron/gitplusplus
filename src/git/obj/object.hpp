#ifndef GIT_OBJ_BASE_HPP
#define GIT_OBJ_BASE_HPP

#include <git/config.h>

#include <gtl/db/odb_object.hpp>
#include <gtl/util.hpp>

#include <string>


GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

//! \brief Thrown for errors during deserialization
class DeserializationError : public gtl::odb_deserialization_error,
							 public gtl::streaming_exception
{};

//! \brief Thrown for errors during serialization
class SerializationError :	public gtl::odb_serialization_error,
							public gtl::streaming_exception
{};


/** \brief base for all objects git knows. 
  * They don't contain more information than their type and data, which is solely defined in derived types.
  */
class Object
{
	
public:
	/** \brief Enumeration specifying codes representing object type known to git
	  */
	enum class Type : uchar
	{
		None,
		Blob,
		Tree,
		Commit,
		Tag
	};
	
protected:
	//! Initialize the object with its type. It must be defined by derived classes and and should not be none
	Object(Type t) : m_type(t) {}
	//! Resets the type information to assure multiple objects behave correctly in unions
	//! \todo maybe setting the type is not required, its currently just needed for the union to work better
	~Object() { m_type = Type::None; }
	
public:
	
	//! \return object type
	inline Type type() const {
		return m_type;
	}
	
protected:
	Type m_type;
};


GIT_NAMESPACE_END
GIT_HEADER_END


#endif // GIT_OBJ_BASE_HPP
