#ifndef GIT_OBJ_COMMIT_H
#define GIT_OBJ_COMMIT_H

#include <git/obj/object.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN
		
class Commit : public Object
{
public:
    Commit() : Object(Object::Type::Commit) {}
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_OBJ_COMMIT_H
