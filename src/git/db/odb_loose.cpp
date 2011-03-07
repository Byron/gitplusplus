#include <git/db/odb_loose.h>
#include <git/config.h>		// for doxygen


GIT_NAMESPACE_BEGIN

LooseODB::LooseODB(const path_type& root)
    : odb_loose(root)
{
}


GIT_NAMESPACE_END
