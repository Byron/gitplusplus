#ifndef GIT_OBJ_COMMIT_H
#define GIT_OBJ_COMMIT_H

#include <git/config.h>
#include <git/obj/object.hpp>


GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN
		
/** \brief object representing a commit, which keeps information about a distinct repository state
  */
class Commit : public Object
{
public:
    Commit() : Object(Object::Type::Commit) {}
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_OBJ_COMMIT_H
