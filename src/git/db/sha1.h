#ifndef SHA1_H
#define SHA1_H

#include <git/config.h>
#include <gtl/db/hash.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

typedef gtl::basic_hash<20, uchar> SHA1;

GIT_HEADER_END
GIT_NAMESPACE_END


#endif // SHA1_H
