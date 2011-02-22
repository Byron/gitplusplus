#ifndef GIT_DB_UTIL_HPP
#define GIT_DB_UTIL_HPP

#include <git/config.h>
#include <git/db/traits.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

// Typedefs for convenience
typedef std::basic_ostream<typename git_object_policy_traits::char_type> git_basic_ostream;
typedef std::basic_istream<typename git_object_policy_traits::char_type> git_basic_istream;

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_DB_UTIL_HPP
