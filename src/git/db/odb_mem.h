#ifndef GIT_ODB_MEM_H
#define GIT_ODB_MEM_H

#include <git/config.h>
#include <git/db/policy.hpp>
#include <gtl/db/odb_mem.hpp>
#include <git/db/util.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

struct git_memory_odb_traits
{
	typedef git_object_traits			obj_traits_type;
};

/** \ingroup ODB
  * \brief Database storing git objects in memory only
  * 
  * Use this specialization to quickly cache objects in memory to later dump
  * them to disk at once.
  */
class MemoryODB : public gtl::odb_mem<git_memory_odb_traits>
{
	public:
		virtual void header_hash(	typename obj_traits_type::hash_generator_type& gen, 
									const output_object_type& obj) const 
		{
			typename git_object_policy_traits::char_type hdr[32];
			ushort hdrlen = loose_object_header(hdr, obj.type(), obj.size());
			gen.update(hdr, hdrlen);
		}
};


GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_ODB_MEM_H
