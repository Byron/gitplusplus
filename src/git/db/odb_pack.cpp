#include <git/db/odb_pack.h>
#include <git/config.h>

GIT_NAMESPACE_BEGIN

void PackODB::set_cache_memory_limit(size_t limit) const
{
	PackCache::set_memory_limit(limit);
	
	// setup the caches
	PackODB* _this = const_cast<PackODB*>(this);
	auto it = _this->m_packs.begin();
	const auto end = _this->m_packs.end();
	
	for (;it != end; ++it)  {
		limit == 0
		        ? it->get()->cache().clear()
		        : it->get()->cache().initialize(it->get()->index());
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
