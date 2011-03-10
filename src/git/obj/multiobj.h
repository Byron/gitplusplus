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
	//! destroy an input object, which might be easier to read compared to a direct destructor call
	inline void destroy() {
		this->~MultiObject();
	}

	~MultiObject();
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_OBJ_MULTIOBJ_H
