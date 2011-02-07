#ifndef GIT_OBJ_BLOB_H
#define GIT_OBJ_BLOB_H

#include <git/obj/object.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

class Blob : public Object
{
public:
    Blob() : Object(Object::Type::Blob) {}
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_OBJ_BLOB_H
