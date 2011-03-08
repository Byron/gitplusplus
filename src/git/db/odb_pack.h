#ifndef GIT_DB_ODB_PACK_H
#define GIT_DB_ODB_PACK_H

#include <git/config.h>
#include <git/db/policy.hpp>
#include <gtl/db/odb_pack.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

struct git_pack_odb_traits : public gtl::odb_pack_traits<git_object_traits>
{
	
};

/** \brief Specializes the odb_pack template to read and generate git-like packs
  */
class PackODB : public gtl::odb_pack<git_object_traits, git_pack_odb_traits>
{
public:
    PackODB(const path_type& root)
	    : odb_pack<git_object_traits, git_pack_odb_traits>(root)
	{}
};


GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_DB_ODB_PACK_H
