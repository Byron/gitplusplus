#include "git/obj/commit.h"

GIT_NAMESPACE_BEGIN

git_basic_ostream& operator << (git_basic_ostream& stream, const Commit& inst) 
{
	return stream;
}

git_basic_istream& operator >> (git_basic_istream& stream, Commit& inst) 
{
	return stream;
}
		
GIT_NAMESPACE_END
