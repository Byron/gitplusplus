#ifndef SHA1_H
#define SHA1_H

#include <git/config.h>
#include <gtl/db/hash.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

//! \todo would have to include traits to have a symbol for the char type, but traits already include this file
typedef gtl::basic_hash<20, char> SHA1;

GIT_HEADER_END
GIT_NAMESPACE_END


#endif // SHA1_H
