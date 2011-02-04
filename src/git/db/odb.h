#ifndef GIT_ODB_H
#define GIT_ODB_H

#include <git/config.h>
#include <gtl/db/odb_mem.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN


/** \ingroup ODB
  * \brief Database storing git objects in memory only
  * 
  * Use this specialization to quickly cache objects in memory to later dump
  * them to disk at once.
  */
class MemoryODB : public gtl::odb_mem<int, int>
{
public:
};


GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_ODB_H
