#ifndef GIT_OBJ_TAG_H
#define GIT_OBJ_TAG_H

#include <git/config.h>
#include <git/obj/object.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN
		
/** \brief represents the contents of a directory, which may be blobs and other trees.
  */
class Tag : public Object
{
public:
    Tag() : Object(Object::Type::Tag) {}
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_OBJ_TAG_H
