#ifndef GIT_DB_PACK_STREAM_H
#define GIT_DB_PACK_STREAM_H

#include <git/config.h>
#include <git/db/traits.hpp>

#include <boost/iostreams/filtering_stream.hpp>
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
class PackStream : public boost::iostreams::filtering_stream<boost::iostreams::input>
{
public:
	typedef git_object_traits_base::size_type			size_type;
	typedef git_object_traits_base::object_type			object_type;
	
protected:
	const PackFile& m_pack;			//!< pack that contains this object
	uint32			m_entry;		//!< pack entry we refer to
	
public:
    PackStream(const PackFile& pack, uint32 entry=0);
	
public:
	//! @{ \name Interface
	
	//! read-only access to our pack entry
	uint32 entry() const {
		return m_entry;
	}
	
	//! read-write access to our object
	uint32& entry() {
		return m_entry;
	}
	
	object_type type() const;
	size_type size() const;
	
	//! @} end interface
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_DB_PACK_STREAM_H
