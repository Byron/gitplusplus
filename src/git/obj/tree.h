#ifndef GIT_OBJ_TREE_H
#define GIT_OBJ_TREE_H

#include <git/obj/object.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

/** \brief Represents the contents of a directory
  */
class Tree : public Object
{
public:
    Tree();
};


GIT_NAMESPACE_END
GIT_HEADER_END



#endif // GIT_OBJ_TREE_H
