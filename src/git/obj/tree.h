#ifndef GIT_OBJ_TREE_H
#define GIT_OBJ_TREE_H

#include <git/obj/object.hpp>
#include <git/obj/stream.h>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

/** \brief Represents the contents of a directory
  */
class Tree : public Object
{
public:
    Tree() : Object(Object::Type::Tree) {}
};


git_basic_ostream& operator << (git_basic_ostream& stream, const Tree& inst);
git_basic_istream& operator >> (git_basic_istream& stream, Tree& inst);

GIT_NAMESPACE_END
GIT_HEADER_END



#endif // GIT_OBJ_TREE_H
