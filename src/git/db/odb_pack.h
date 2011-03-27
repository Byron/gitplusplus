#ifndef GIT_DB_ODB_PACK_H
#define GIT_DB_ODB_PACK_H

#include <git/config.h>
#include <git/db/policy.hpp>
#include <gtl/db/odb_pack.hpp>
#include <git/db/pack_file.h>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN


/** \brief Defines a git-specific implementation of the packed database
  */
struct git_pack_odb_traits : public gtl::odb_pack_traits<git_object_traits>
{
	typedef PackFile							pack_reader_type;
};


/** \brief Specializes the odb_pack template to read and generate git-like packs
  */
class PackODB : public gtl::odb_pack<git_pack_odb_traits>
{
public:
    PackODB(const path_type& root, mapped_memory_manager_type& manager, provider_type* provider = nullptr)
	    : odb_pack<git_pack_odb_traits>(root, manager, provider)
	{}
	
public:
	void set_cache_memory_limit(size_t limit) const;
	
	size_t cache_memory_limit() const;
	
	size_t cache_memory() const;
};


/** \brief Implementation of a PackDatabase Virtual Provider
  */
typedef gtl::odb_provider<PackODB> PackODBProvider;


GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_DB_ODB_PACK_H
