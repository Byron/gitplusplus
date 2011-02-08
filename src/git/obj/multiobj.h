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

union MultiObject 
{
	Object::Type type;
	
	Tree tree;
	Blob blob;
	Commit commit;
	Tag tag;
	
	MultiObject() : type(Object::Type::None) {}
	~MultiObject();
};


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
}

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_OBJ_MULTIOBJ_H
