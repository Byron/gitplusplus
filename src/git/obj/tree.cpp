#include "git/obj/tree.h"

GIT_NAMESPACE_BEGIN

git_basic_ostream& operator << (git_basic_ostream& stream, const Tree& inst) 
{
	return stream;
}

git_basic_istream& operator >> (git_basic_istream& stream, Tree& inst) 
{
	return stream;
}
		

GIT_NAMESPACE_END
