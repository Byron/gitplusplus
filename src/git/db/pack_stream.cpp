#include <git/db/pack_stream.h>
#include <git/config.h>

#include <git/db/pack_file.h>

GIT_NAMESPACE_BEGIN

PackStream::PackStream(const PackFile& pack, uint32 entry)
    : m_pack(pack)
    , m_entry(entry)
{
	
}



GIT_NAMESPACE_END
