#ifndef GIT_DB_ODB_PACK_H
#define GIT_DB_ODB_PACK_H

#include <git/config.h>
#include <git/db/policy.hpp>
#include <gtl/db/odb_pack.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

/** \brief Specializes the odb_pack template to read and generate git-like packs
  */
class PackODB
{
public:
    PackODB();
};


GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_DB_ODB_PACK_H
