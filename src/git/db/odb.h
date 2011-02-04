#ifndef GIT_ODB_H
#define GIT_ODB_H

#include <git/config.h>

#include <gitmodel/db/odb_mem.hpp>

using namespace gitmodel;

/** \class MemoryODB
  * \ingroup ODB
  * \brief Database storing git objects in memory only
  * 
  * Use this specialization to quickly cache objects in memory to later dump
  * them to disk at once.
  */
class MemoryODB : public odb_mem<int, int>
{
public:
};

#endif // GIT_ODB_H
