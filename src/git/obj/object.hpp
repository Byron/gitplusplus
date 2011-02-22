#ifndef GIT_OBJ_BASE_HPP
#define GIT_OBJ_BASE_HPP

#include <git/config.h>
#include <git/db/traits.hpp>
#include <git/db/util.hpp>

#include <gtl/db/odb_object.hpp>
#include <gtl/util.hpp>

#include <string>
#include <ctime>


GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

using std::string;

//! \brief Thrown for errors during deserialization
class ObjectError		: public gtl::odb_object_error,
						  public gtl::streaming_exception
{};

//! \brief Thrown for errors during deserialization
class DeserializationError : public ObjectError
{};

//! \brief Thrown for errors during serialization
class SerializationError :	public ObjectError
{};


//! Utility storing a name and an email
struct Actor
{
	string name;
	string email;
	
	bool operator == (const Actor& rhs) const {
		return (name == rhs.name && email == rhs.email);
	}
};


//! \brief offset definition (including parsing facilities) to define a timezone offset
//! from universal time
struct TimezoneOffset
{
	short utz_offset;		//! Offset as universal time zone offset
	
	TimezoneOffset(short offset)
		: utz_offset(offset) {}
	TimezoneOffset& operator = (short offset) {
		utz_offset = offset;
		return *this;
	}
	operator short() const{
		return utz_offset;
	}
};

//! Utility acting like a stamp with an id, as it encapsulates a timestamp too
struct ActorDate : public Actor
{
	time_t			time;			//! Time at which a certain event happend (seconds since epoch)
	TimezoneOffset	tz_offset;		//! timezone at which the time was recorded
	
	ActorDate()
		: time(0)
		, tz_offset(0) 
	{}
	
	bool operator == (const ActorDate& rhs) const {
		return static_cast<const Actor&>(rhs) == *this && time == rhs.time && tz_offset == rhs.tz_offset;
	}
};

/** \brief base for all objects git knows. 
  * They don't contain more information than their type and data, which is solely defined in derived types.
  */
class Object
{
public:
	//! typedef for compatability and convenience
	typedef ObjectType Type;
	typedef git_object_policy_traits::size_type size_type;
	
protected:
	Type m_type;
	
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
	
	//! \return uncompressed size of the serialized version
	//! \param pstream stream which, if not 0, allows to catch the serialization result
	//! so that it may be reused right away by the caller.
	//! \note intentionally not virtual as we use it to inherit documentation
	//! \note the base implementation is generically implemented, subclasses may and possibly should
	//! implement specific and hence faster versions.
	size_type size(git_basic_ostream* pstream=nullptr) const;
};

GIT_NAMESPACE_END
GIT_HEADER_END


#endif // GIT_OBJ_BASE_HPP
