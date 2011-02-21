#ifndef GIT_OBJ_TAG_H
#define GIT_OBJ_TAG_H

#include <git/config.h>
#include <git/obj/object.hpp>
#include <git/db/traits.hpp>
#include <git/db/sha1.h>
#include <string>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

using std::string;
		
/** \brief represents the contents of a directory, which may be blobs and other trees.
  */
class Tag : public Object
{
public:
	typedef typename git_object_traits_base::key_type key_type;
	
private:
	Object::Type	m_obj_type;		//! Type id of the object
	key_type		m_obj_hash;		//! hash pointing to our object
	string			m_name;			//! own tag name
	ActorDate		m_actor;		//! tagger information
	string			m_message;		//! Message provided when the tag was created
	
public:
    Tag() : Object(Object::Type::Tag) {}
	Tag(const Tag&) = default;
	Tag(Tag&&) = default;
	
public:
	//! \return the type of the object we refer to
	Object::Type object_type() const {
		return m_obj_type;
	}
	
	//! \return modifyable
	Object::Type& object_type() {
		return m_obj_type;
	}
	
	//! \return key which can be used to obtain the object from the object database
	const key_type& object_key() const {
		return m_obj_hash;
	}
	
	//! \return modifyable
	key_type& object_key() {
		return m_obj_hash;
	}
	
	//! \return name of this tag
	const string& name() const {
		return m_name;
	}
	
	//! \return modifyable
	string& name() {
		return m_name;
	}
	
	//! \return tag message
	const string& message() const {
		return m_message;
	}
	
	//! \return modifyable
	string& message() {
		return m_message;
	}
	
	//! \return actor who tagged this object, including the exact time
	const ActorDate& actor() const {
		return m_actor;
	}
	
	//! \return modifyable
	ActorDate& actor() {
		return m_actor;
	}
	
	
public:
	//! @{ Serialization
	
	//! Write a serialized version of the tag into the given stream
	//! \tparam StreamType ostream compatible stream
	template <class StreamType>
	void serialize(StreamType& ostream)
	{
		ostream << "object " << m_obj_hash << std::end;
		ostream << "object " << m_obj_hash << std::end;
	}
	
	//! @} end serialization
	
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_OBJ_TAG_H
