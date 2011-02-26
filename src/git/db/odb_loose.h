#ifndef GIT_ODB_LOOSE_H
#define GIT_ODB_LOOSE_H

#include <git/config.h>
#include <git/db/policy.hpp>
#include <gtl/db/odb_loose.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

/** \todo still to be implemented/defined
  */
struct git_loose_odb_policy : public gtl::odb_loose_policy
{
	template <class StreamType, class StringType, class ObjectType, class SizeType>
	size_t parse_header(StreamType& in, StringType buf, ObjectType& type, SizeType& size)
	{
		const size_t buflen = 512;
		typename StringType::value_type cbuf[buflen];
		in.read(cbuf, buflen);
		
		return 0;
	}
};

/** \brief configures the loose object database to be conforming with a default 
  * git repository
  */
struct git_loose_odb_traits : public gtl::odb_loose_traits<git_object_traits>
{
	//! override default policy for our implementation
	typedef git_loose_odb_policy policy_type;
};

/** \ingroup ODB
  * \brief git-like implementation of the loose object database
  */
class LooseODB : public gtl::odb_loose<git_object_traits, git_loose_odb_traits>
{
public:
	typedef git_loose_odb_traits::path_type path_type;
	
public:
    LooseODB(const path_type& root);
};


GIT_NAMESPACE_END
GIT_HEADER_END
#endif // GIT_ODB_LOOSE_H
