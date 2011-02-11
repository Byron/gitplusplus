#include <git/db/sha1.h>
#include <git/config.h>	// for doxygen

GIT_NAMESPACE_BEGIN

const SHA1 SHA1::null((uchar)0);
		
GIT_NAMESPACE_END

std::ostream& operator << (std::ostream& out, const git::SHA1& rhs)
{
	static const char map[] = {'0', '1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
	const char  rpart = 0x0F;
	
	// Only blocks at maxStrl bytes
	for(uint32 i = 0; i < 20; ++i){
			out << map[(rhs[i] >> 4) & rpart];
			out << map[rhs[i] & rpart];
	}
	return out;
}


