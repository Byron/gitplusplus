#include <git/db/pack_file.h>
#include <git/config.h>			// for doxygen

#include <cstring>

namespace boost {

	void intrusive_ptr_add_ref(git::PackCache::counted_char_const_type* d) {
		gtl::intrusive_ptr_add_ref_array_impl(d);
	}
	
	void intrusive_ptr_release(git::PackCache::counted_char_const_type* d) {
		gtl::intrusive_ptr_release_array_impl(d);
	}

}// end boost namespace


GIT_NAMESPACE_BEGIN

size_t PackCache::gMemoryLimit = 0;
size_t PackCache::gMemory = 0;

PackCache::PackCache()
    : m_mem(0)
    , m_total_importance(0)
#ifdef DEBUG
    , m_hits(0)
    , m_noccupied(0)
    , m_ncollect(0)
    , m_mem_collected(0)
    
#endif
{}


void PackCache::clear() 
{
	m_info.clear();
	m_ofs.clear();
	gMemory -= m_mem;
	m_mem = 0;
	m_total_importance = 0;
	
#ifdef DEBUG
	m_hits = 0;
	m_noccupied = 0;
	m_ncollect = 0;
	m_mem_collected = 0;
#endif
}


void PackCache::cache_info(std::ostream &out) const {
	out << "###-> Pack " << this << " memory = " << m_mem  / 1000 << "kb, structure_mem[kb] = " 
	    << struct_mem() / 1000 << "kb, entries = " << m_ofs.size() << ", queries = " << m_total_importance
#ifdef DEBUG	       
		<< ", occupied = " << m_noccupied << ", hits = " << hits() << ", misses " << misses()
		<< ", hit-ratio = " << (m_total_importance ? m_hits / (float)m_total_importance : 0.f)
		<< ", collects = " << m_ncollect << ", memory collected = " << m_mem_collected / 1000 << "kb"
#endif
		<< std::endl;
}

PackCache::CacheInfo::CacheInfo(uint32 importance, PackCache::size_type size, PackCache::counted_char_const_type* data)
    : importance(importance)
    , size(0)
    , pdata(data)
{}

void PackCache::initialize(const PackIndexFile &index)
{
	if (is_available()){
		return;
	}

	const uint32 ne = index.num_entries();
	m_ofs.reserve(ne);
	m_info.resize(ne);
	
	for (uint32 i = 0; i < ne; ++i) {
		m_ofs.push_back(index.offset(i));
	}
	std::sort(m_ofs.begin(), m_ofs.end());
}

uint32 PackCache::offset_to_entry(uint64 offset) const
{
	assert(is_available());
	
	vec_ofs::const_iterator lo = m_ofs.begin();
	vec_ofs::const_iterator hi = m_ofs.end();
	vec_ofs::const_iterator mi;
	
	while (lo < hi) {
	    mi = lo + (std::distance(lo, hi) / 2);
	    
	    if (*mi > offset) {
		    hi = mi;
	    } else if (*mi==offset) {
		    return std::distance(m_ofs.begin(), mi);
		} else {
		    lo = mi + 1;
		}
	}
	
	assert(false);
	return PackIndexFile::hash_unknown;
	
	// todo: test std implementation performance	
	/*std::iterator_traits<vec_ofs::iterator>::distance_type count, step;
	std::lower_boundvec_ofs::const_iterator first = m_ofs.begin();
	vec_ofs::const_iterator end = m_ofs.end();
	
	std::lower_bound(first, end, offset);*/
}

size_t PackCache::collect(size_t bytes_to_free)
{
#ifdef DEBUG
	m_ncollect += 1;
#endif
	size_t bf = 0;	// bytes freed
	
	// we cut away all entries with an importance less than average one.
	// To be sure we reach our limit, we start smaller, but raise the importance 
	// on each step until we have reached our goal.
	// We always do full runs which means we could possibly truncate quite a lot of our data
	const vec_info::iterator end = m_info.end();
	for (float mult = 0.5f; bf < bytes_to_free && m_mem; mult += 0.5f){
		uint32 avg_importance = static_cast<uint32>(mult * (m_total_importance / (float)m_ofs.size()));
		for (vec_info::iterator beg = m_info.begin(); beg < end; ++beg) {
			if (beg->importance < avg_importance) {
				CacheInfo& info = *beg;
				bf += info.size;
				set_data(info, 0, nullptr);
			}
		}// for each info item
	}// while there are bytes to free
	
#ifdef DEBUG
	m_mem_collected += bf;
#endif
	return bf;
}

PackCache::counted_char_ptr_const_type PackCache::cache_at(uint64 offset) const 
{
	uint32 entry = offset_to_entry(offset);
	
	// increment its importance, no matter whether we have it or not
	const CacheInfo& info = m_info[entry];
	const_cast<CacheInfo&>(info).importance += 1;
	m_total_importance += 1;
	
#ifdef DEBUG
	m_hits += !!info.pdata;
#endif
	
	return info.pdata;
}

void PackCache::set_data(CacheInfo& info, size_type size, counted_char_const_type* pdata)
{
	// Note: We explicitly don't remove importance if the entry is zeroed, as we currently
	// unconditionally add importance on each request
	gMemory -= m_mem;
	m_mem -= info.size;
	m_mem += size;
	gMemory += m_mem;
	info.pdata = pdata;
	info.size = size;
	
#ifdef DEBUG
	m_noccupied += pdata ? 1 : -1;
#endif
}

bool PackCache::set_cache_at(uint64 offset, size_type size, counted_char_const_type* pdata) 
{
	if (size + gMemory > gMemoryLimit) {
		collect(size);
		if (size + gMemory > gMemoryLimit) {
			return false;
		}
	}
	set_data(m_info[offset_to_entry(offset)], size, pdata);
	return true;
}

PackIndexFile::PackIndexFile()
{
	reset();
	static_assert(key_type::hash_len == 20, "The original pack index requires sha1 hashes");
	static_assert(sizeof(key_type) == key_type::hash_len, "Keytypes must only contain the raw hash data");
}

void PackIndexFile::reset()
{
	m_type = Type::Undefined;
	m_version = 0;
	m_num_entries = 0;
}

void PackIndexFile::open(const path_type &path)
{
	boost::iostreams::mapped_file_source::open(path);
	reset();
	assert(data()!=nullptr);
	
	if (size() < 4) {
		ParseError err;
		err.stream() << "Index file is too small";
		throw err;
	}
	
	// READ HEADER
	const char* d = data();
	m_type = std::memcmp(d, "\377tOc", 4) == 0 ? Type::Default : Type::Legacy;
	size_t min_size = 0;
	
	if (m_type == Type::Default) {
		min_size = 2*4 + 256*4 + key_type::hash_len + 4 + 4 + 2*key_type::hash_len;
	} else {
		min_size = 256*4 + key_type::hash_len+4 + 2*key_type::hash_len;
	}// end verify size
	
	if (size() < min_size) {
		ParseError err;
		err.stream() << "Index file size was " << size() << " bytes in size, needs to be at least " << min_size << " bytes";
		throw err;
	}
	
	// INITIALIZE DATA
	if (m_type == Type::Default) {
		m_version = ntohl(reinterpret_cast<const uint32*>(d)[1]);
		if (m_version != 2) {
			IndexVersionError err(m_version);
			throw err;
		}
	}
	
	m_num_entries = _num_entries();
}

uint32 PackIndexFile::crc(uint32 entry) const
{
	assert(entry < num_entries());
	if (m_type == Type::Default) {
		return ntohl(reinterpret_cast<const uint32*>(data() + v2ofs_crc(num_entries()))[entry]);
	} else {
		return 0;
	}
}

void PackIndexFile::sha(uint32 entry, key_type &out_hash) const 
{
	assert(entry < num_entries());
	if (m_type == Type::Default) {
		out_hash = data() + v2ofs_sha() + entry*key_type::hash_len;
	} else {
		out_hash = data() + 256*4 + entry*sizeof(OffsetInfo) + 4;
	}
}

uint32 PackIndexFile::sha_to_entry(const key_type &sha) const
{
	const char* keys;
	uint32 ofs = 0;
	uint32 ksize = key_type::hash_len;
	const uint32* fo = reinterpret_cast<const uint32*>(data());
	
	// use some helpers to mask the v1/v2 compatability stuff. Performance overhead
	// should hopefully be neglectable, for super-performance one might even want to
	// start fresh and create a new format without legacy support
	if (m_type == Type::Default) {
		keys = data() + v2ofs_sha();
		fo += 2;
	} else {
		ofs = 4;
		ksize = sizeof(OffsetInfo);
		keys = data() + 256*4;
	}
	
	uint32 lo = 0;
	uint32 hi, mi;
	const uchar fb = sha.bytes()[0];	// first byte
	
	if (fb != 0) {
		lo = ntohl(fo[fb-1]);
	}
	hi = ntohl(fo[fb]);
	
	// todo: attempt early abort by approaching the bounds. In the best case, we case
	// we could find a better mid position
	
	while (lo < hi) {
		mi = (lo + hi) / 2;
		auto cmp = memcmp(sha.bytes(), keys + ksize*mi + ofs, key_type::hash_len);
		if (cmp < 0)
			hi = mi;
		else if (cmp==0)
			return mi;
		else
			lo = mi + 1;
	}
	return hash_unknown;
}

key_type PackIndexFile::pack_checksum() const
{
	return key_type(data() + size() - key_type::hash_len*2);
}

key_type PackIndexFile::index_checksum() const 
{
	return key_type(data() + size() - key_type::hash_len);
}

void PackIndexFile::close() 
{
	boost::iostreams::mapped_file_source::close();
	m_type = Type::Undefined;
	m_version = 0;
}

key_type PackOutputObject::key() const
{
   assert(m_ppack);
   key_type k;
   m_ppack->index().sha(m_entry, k);
   return k;
}

PackFile::PackFile(const path_type& file, mapped_memory_manager_type& manager, const provider_mixin_type& db)
    : m_pack_path(file)
    , m_db(db)
{
	// initialize index
	path_type index_file(file);
	index_file.replace_extension(".idx");
	m_index.open(index_file.string());
	
	// initialize pack - we map everything as we assume a sliding window
	mapped_file_source_type pack;
	pack.open(manager.make_cursor(file));
	m_cursor = pack.cursor();
	assert(m_cursor.is_valid());
	
	struct Header
	{
		uint32	type;		// Usually "PACK" as char
		uint32	version;	// Version
		uint32	num_entries;// Amount of entries in the pack
	};
	
	Header hdr;
	if (pack.read(reinterpret_cast<char*>(&hdr), sizeof(Header)) != sizeof(Header)) {
		ParseError err;
		err.stream() << "Pack file at " << file << " does not have a header";
		throw err;
	}
	
	// signature
	hdr.type = ntohl(hdr.type);
	ParseError err;
	err.stream() << "Error File " << file << ": ";
	if (hdr.type != pack_signature) {
		err.stream() << "First bytes are supposed to be PACK, but was " << std::hex << hdr.type;
		throw err;
	}
	
	// version
	hdr.version = ntohl(hdr.version);
	if (hdr.version != 2 && hdr.version != 3) {
		err.stream() << "Cannot handle pack file version: " << hdr.version << ". Consider upgrading to a newer git++ library version";
		throw err;
	}
	
	// entries
	hdr.num_entries = ntohl(hdr.num_entries);
	if (hdr.num_entries != m_index.num_entries()) {
		err.stream() << "Pack claims to have " << hdr.num_entries << " objects, but it has " << m_index.num_entries() << " according to the index file";
		throw err;
	}
	
	// verify sha
	try {
		pack.seek(-key_type::hash_len, std::ios::end);
	} catch (const std::ios_base::failure& exc) {
		err.stream() << "Failed to seek to end of pack to read sha: " << exc.what();
		throw err;
	}
	
	key_type key;
	pack.read(key, key_type::hash_len);
	if (key != m_index.pack_checksum()) {
		err.stream() << "Pack checksum didn't match index checksum: " << key << " vs " << m_index.pack_checksum();
		throw err;
	}
}

bool PackFile::is_valid_path(const path_type& file)
{
	static const std::string prefix("pack-");
	static const std::string extension(".pack");
	return (file.extension() == extension && file.filename().substr(0, 5) == prefix);
}

PackFile* PackFile::new_pack(const path_type& file, mapped_memory_manager_type& manager, const provider_mixin_type& db)
{
	if (!is_valid_path(file)) {
		return nullptr;
	}
	
	return new PackFile(file, manager, db);
}


GIT_NAMESPACE_END
