#ifndef GIT_DB_PACK_STREAM_H
#define GIT_DB_PACK_STREAM_H

#include <git/config.h>

#include <boost/iostreams/filter/zlib.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

//! @{ \name forward declarations
class PackFile;

//! @} end forward declarations

/** \brief A stream providing access to a deltified object
  * It operates by pre-assembling all deltas into a single delta, which will be applied to the base at once
  * For this to work, the stream must know its pack (to dereference in-pack object refs) as well as a lookup 
  * git repository to obtain base object data.
  */
class DeltaPackStream
{
protected:
	PackFile&				m_pack;
	
public:
    DeltaPackStream(PackFile& pack);
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_DB_PACK_STREAM_H
