#ifndef GIT_OBJ_BASE_HPP
#define GIT_OBJ_BASE_HPP

#include <git/config.h>

#include <string>
#include <boost/iostreams/device/back_inserter.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN


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
