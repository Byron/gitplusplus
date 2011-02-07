#ifndef GIT_OBJ_TAG_H
#define GIT_OBJ_TAG_H

#include <git/obj/object.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN
		
class Tag : public Object
{
public:
    Tag() : Object(Object::Type::Tag) {}
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_OBJ_TAG_H
