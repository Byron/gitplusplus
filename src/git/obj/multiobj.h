#ifndef GIT_OBJ_MULTIOBJ_H
#define GIT_OBJ_MULTIOBJ_H

#include <git/config.h>
#include <git/obj/tree.h>
#include <git/obj/commit.h>
#include <git/obj/blob.h>
#include <git/obj/tag.h>
#include <assert.h>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

//! \brief Utility to encapsulte one of many possible object types, which helps
//! allocation efficiency as the MultiObject can just be created on the stack, reducing 
//! heap usage. 
union MultiObject
{
	Object::Type	type;		//!< type of the currently assigned object or None
	
	//! @{ \name Objects
	Tree			tree;
	Blob			blob;
	Commit			commit;
	Tag				tag;
	//! @}
	
	//! Initialize the instance with a None type. In this state the union doesn't contain
	//! any object
	MultiObject() : type(Object::Type::None) {}
	~MultiObject();
};


//! Destroy the union by calling the destructor of the currently assigned object
//! \note you can reuse the same union for multiple objects of different kinds if
//! you call its destructor manually - this will reset the object into the state
//! it had right after construction.
MultiObject::~MultiObject()
{
	switch(type)
	{
	case Object::Type::None:
		{
			break;
		}
	case Object::Type::Tree:
		{
			tree.~Tree();
			break;
		}
	case Object::Type::Blob:
		{
			blob.~Blob();
			break;
		}
	case Object::Type::Commit:
		{
			commit.~Commit();
			break;
		}
	case Object::Type::Tag:
		{
			tag.~Tag();
			break;
		}
	default:
		{
			assert(false);
		}
	}// switch Type
	
	// Set the type, which allows to use multiobject multiple times in a row
	// when the destructor is called manually
	type = Object::Type::None;
}

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_OBJ_MULTIOBJ_H
