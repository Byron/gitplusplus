#ifndef GIT_DB_UTIL_HPP
#define GIT_DB_UTIL_HPP

#include <git/config.h>
#include <git/db/traits.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

namespace io = boost::iostreams;

//! @{ \name Typedefs for convenience
typedef std::basic_ostream<typename git_object_policy_traits::char_type> git_basic_ostream;
typedef std::basic_istream<typename git_object_policy_traits::char_type> git_basic_istream;
//! @}

//! Write a loose object header into the given memory pointer
//! \param hdr character pointer with 32 bytes of allocated memory
//! \param type the type of the object
//! \param size uncompressed size in byte of the object data
//! \return amount of bytes written into the header, which includes the terminating \0
uchar loose_object_header(	typename git_object_policy_traits::char_type* hdr, 
							typename git_object_policy_traits::object_type type, 
							typename git_object_policy_traits::size_type size);

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_DB_UTIL_HPP
