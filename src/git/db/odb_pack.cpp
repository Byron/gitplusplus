#include <git/db/odb_pack.h>
#include <git/config.h>

GIT_NAMESPACE_BEGIN

void PackODB::set_cache_memory_limit(size_t limit, gtl::cache_access_mode mode) const
{
	PackCache::set_memory_limit(limit);
	
	// setup the caches
	PackODB* _this = const_cast<PackODB*>(this);
	auto it = _this->m_packs.begin();
	const auto end = _this->m_packs.end();
	
	for (;it != end; ++it)  {
		const auto* pack = it->get();
		limit == 0
		        ? pack->cache().clear()
		        : pack->cache().initialize(pack->index(), pack->cursor().file_size(), mode);
	}
}

size_t PackODB::cache_memory_limit() const
{
	return PackCache::memory_limit();
}

size_t PackODB::cache_memory() const {
	return PackCache::total_memory();
}


GIT_NAMESPACE_END 
