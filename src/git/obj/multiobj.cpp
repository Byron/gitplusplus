#include <git/obj/multiobj.h>
#include <git/config.h>		// doxygen

GIT_NAMESPACE_BEGIN

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
