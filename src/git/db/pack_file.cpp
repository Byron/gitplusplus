#include <git/db/pack_file.h>
#include <git/config.h>			// for doxygen
#include <git/db/util.hpp>
#include <gtl/db/hash_generator_filter.hpp>

#include <boost/crc.hpp>

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
    , m_head(nullptr)
    , m_tail(nullptr)
    , m_mode(gtl::cache_access_mode::unspecified)
#ifdef DEBUG
    , m_hits(0)
    , m_ncollect(0)
    , m_mem_collected(0)
    , m_nrequest(0)
#endif
{}


void PackCache::clear() 
{
	m_info.clear();
	gMemory -= m_mem;
	m_head = m_tail = nullptr;
	m_mem = 0;
	
#ifdef DEBUG
	m_hits = 0;
	m_ncollect = 0;
	m_mem_collected = 0;
	m_nrequest = 0;
#endif
}


void PackCache::cache_info(std::ostream &out) const {
	uint32 occupied = 0;
	for (auto beg = m_info.begin(); beg < m_info.end(); ++beg) {
		occupied += beg->size != 0;
	}
	
	out << "###-> Pack " << this << " memory = " << m_mem  / 1000 << "kb, structure_mem[kb] = " 
	    << struct_mem(m_info.size()) / 1000 << "kb, entries = " << m_info.size()
	    << ", occupied = " << occupied << " (" << (occupied /(float)m_info.size()) * 100 << "% full)"
#ifdef DEBUG	       
	    << ", queries = " << m_nrequest
	    << ", hits = " << hits() <<  ", hit-ratio = " << (m_nrequest ? m_hits / (float)m_nrequest : 0.f)
		<< ", collects = " << m_ncollect << ", memory collected = " << m_mem_collected / 1000 << "kb"
#endif
		<< std::endl;
}

PackCache::CacheInfo::CacheInfo()
    : offset(0)
    , size(0)
{}

void PackCache::initialize(const PackIndexFile &index, uint64 pack_size, gtl::cache_access_mode mode)
{
	if (is_available() && mode == m_mode){
		return;
	}
	
	if (mode == gtl::cache_access_mode::unspecified) {
		mode = gtl::cache_access_mode::random;
	}
	
	// Make sure we reinitialize our linked list pointers correctly
	// We could be smarter about this, but for now we aren't 
	if (m_info.size() != 0) {
		clear();
	}
	
	// Build a hash-map with only a reasonable amount of entries to save memory.
	// As the cache will be more efficient if there are more entries, we try to provide
	// plenty of them while staying within our memory range.
	// only use 10 percent of the available cache memory at maximum, but not more than 1/3 of the maximum
	// entries, the hash table seems to perform better if its more crowed, also considering that there
	// will be clashes even if we have more entries due to hash collisions
	const size_type memavail = gMemoryLimit - gMemory;
	uint32 ne;
	
	// The optimial amount of entries depends on the caching mode. In Random access mode, more slots 
	// are better, but too many don't make things better as we are using a hash function which clashes.
	// In sequencial mode, the optimal entry size depends on the average delta chain depth and the average
	// object size. We want to store as many objects as possible without hitting the memory limit. Instead, we want
	// to just replace one cache by another
	if (mode == gtl::cache_access_mode::sequencial) {
		// lets make a rough estimate: the more free memory we have, the more entries we can effort without
		// collecting too often
		const size_t avg_obj_size = std::max((size_t)1, pack_size / index.num_entries());
		ne = std::max((size_t)256, (memavail / avg_obj_size) / (100+40));	// make it a little smaller to reduce entries
	} else {
		ne = std::min(
					  (uint32)(memavail * 0.1f / (float)sizeof(CacheInfo)), 
					  (uint32)(index.num_entries() * 0.75f));
		ne = std::max(ne, std::max(256u, ne));
	}
	
	m_info.resize(ne);
	m_mode = mode;
	
	// initialize the doubly linked list
	m_head = &m_info[0];
	m_tail = &*(m_info.end()-1);
	
	assert(m_head != m_tail);
	m_head->next = m_tail;
	m_head->prev = nullptr;
	m_tail->prev = m_head;
	m_tail->next = nullptr;
	
	m_mem += struct_mem(ne);
	gMemory += m_mem;
	
#ifdef DEBUG
	const size_t kb = 1000;
	std::cerr << "INITIALIZE mode (1=random,2=sequencial): " 
			<< (int)mode << ", Num Entries: " << ne 
			<< ", Memory free/max [kb]" << (gMemoryLimit - gMemory) / kb<< "/" << gMemoryLimit / kb << std::endl;
#endif
}

uint32 PackCache::offset_to_entry(uint64 offset) const
{
	assert(is_available());
	// Protect our static head and tail from being written
	return std::max(
	            (offset + (offset >> 8) + (offset >> 16) + (offset >> 24) + (offset >> 32)) % (m_info.size()-1), 
	            (uint64)1);
}

size_t PackCache::collect(size_t bytes_to_free)
{
	size_t bf = 0;	// bytes freed
	bool sequencial_mode = m_mode == gtl::cache_access_mode::sequencial;
	// In random mode, we are better off getting rid of a lot of data at once to cut down
	// the amount of collects on possibly large amount of entries. As access is random, no entry is
	// more valuable than the other.
	// In sequencial mode, we want to be done as fast as possible. Cutting away the first entries
	// is the right thing to do and allowed by the linked list implementation of the entries.
	// Also, we try to be done faster by focussing on the blobs first
	if (!sequencial_mode) {
		bytes_to_free = std::max(bytes_to_free, (size_t)(m_mem * 0.5f));
	}
	
#ifdef DEBUG
	m_ncollect += 1;
	//static const size_t mb = 1000 * 1000;
	// std::cerr << "Have Collect: " << m_mem / mb << "mb - bytes to free " << bytes_to_free / mb << "mb";
#endif
	
	if (sequencial_mode) {
		for (CacheInfo* pinfo = m_head->next; bf < bytes_to_free && pinfo != m_tail; pinfo = pinfo->next) {
			if (pinfo->type == PackedObjectType::Blob) {
				bf += pinfo->size;
				set_data(*pinfo, 0, PackedObjectType::Bad, 0, nullptr);
			}
		}
	}
	
	for (CacheInfo* pinfo = m_head->next; bf < bytes_to_free && pinfo != m_tail; pinfo = pinfo->next) {
		bf += pinfo->size;
		set_data(*pinfo, 0, PackedObjectType::Bad, 0, nullptr);
	}
	
#ifdef DEBUG
	m_mem_collected += bf;
	//std::cerr << " - End of collect: " << "bytes freed " << bf / mb << "mb -- mem " << m_mem / mb << "mb"
	//		  << std::endl;
#endif
	return bf;
}

PackCache::counted_char_ptr_const_type PackCache::cache_at(uint64 offset, PackedObjectType* out_type, size_type* out_size) const 
{
	static const counted_char_ptr_const_type nullp;
	const uint32 entry = offset_to_entry(offset);
	
#ifdef DEBUG
	m_nrequest += 1;
#endif
	
	const CacheInfo& info = m_info[entry];
	if (info.offset != offset || info.size == 0) {
		return nullp;
	}

#ifdef DEBUG
	m_hits += !!info.pdata;
#endif
	if (out_size) {
		*out_size = info.size;
	}
	if (out_type) {
		*out_type = info.type;
	}
	return info.pdata;
}

void PackCache::set_data(CacheInfo& info, uint64 offset, PackedObjectType type, size_type size, counted_char_const_type* pdata)
{
	gMemory -= m_mem;
	m_mem -= info.size;
	m_mem += size;
	gMemory += m_mem;
	bool was_set = (bool)info.size;
	info.pdata = pdata;
	info.type = type;
	info.size = size;
	info.offset = offset;
	
	if (was_set & !size) {
		// deletion
		info.next->prev = info.prev;
		info.prev->next = info.next;
		// Must not reset pointers, on deletion we rely on the fact that the next pointer still points
		// to the right next entry (and is not null)
	} else if (!was_set & !!size) {
		// first add
		info.next = m_tail;
		info.prev = m_tail->prev;
		m_tail->prev->next = &info;
		m_tail->prev = &info;
	}
	// Otherwise just keep the entry if the data/offset changes
}

bool PackCache::set_cache_at(uint64 offset, PackedObjectType type, size_type size, counted_char_const_type* pdata) 
{
	// refuse to cache items which are too large !
	if (size*2 > gMemoryLimit) {
		return false;
	}
	
	// Collect first to assure we have enough memory.
	CacheInfo& info = m_info[offset_to_entry(offset)];
	uint32 diff = static_cast<uint32>(std::min((size_type)0, size - info.size));
	if (diff + gMemory > gMemoryLimit) {
		collect(size);
		if (size + gMemory > gMemoryLimit) {
			return false;
		}
	}
	
	// store data unconditionally
	set_data(m_info[offset_to_entry(offset)], offset, type, size, pdata);
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

bool PackFile::verify(std::ostream &output) const 
{
	// Sort entries by offset, to help the caching !
	struct OffsetInfo {
		uint64 offset;			// offset into packfile
		uint32 entry;		// entry of index file matching the offset
	};

	typedef std::vector<OffsetInfo> vec_info;
	
	const uint32 ne = m_index.num_entries();
	vec_info ofs;
	ofs.reserve(ne);
	
	OffsetInfo info;
	for (uint32 i = 0; i < ne; ++i) {
		info.offset = m_index.offset(i);
		info.entry = i;
		ofs.push_back(info);
	}
	std::sort(ofs.begin(), ofs.end(), 
	          [](const OffsetInfo& l, const OffsetInfo& r)->bool{return l.offset < r.offset;});
	
	bool													res = true;
	const vec_info::const_iterator							end = ofs.end();
	key_type												hash;
	boost::crc_32_type										crc;
	PackDevice::hash_generator_type							hgen;
	PackDevice												pd(*this);
	
	
	for (vec_info::const_iterator it = ofs.begin(); it < end; ++it) {
		// CRC
		if (m_index.version() > PackIndexFile::Type::Legacy) {
			uint64 len = it+1 < end 
							? (it+1)->offset - it->offset 
			                : m_cursor.file_size() - key_type::hash_len - it->offset;
			uint64 ofs = it->offset;
			do {
				const_cast<PackFile*>(this)->m_cursor.use_region(ofs, len);
				assert(m_cursor.is_valid());
				size_t actual_size = m_cursor.size();
				if (actual_size > len){
					actual_size = static_cast<size_t>(len);
				}
				crc.process_bytes(m_cursor.begin(), actual_size);
				len -= actual_size;
			} while (len);
			
			if (m_index.crc(it->entry) != crc.checksum()) {
				res = false;
				output << "object at entry " << it->entry << " doesn't match its index crc32 " << crc.checksum() << std::endl;
			}
			crc.reset();
		}// handle crc check

		// SHA		
		pd.entry() = it->entry;
		m_index.sha(it->entry, hash);
		if (!pd.verify_hash(hash, hgen)) {
			res = false;
			output << "object at entry " << it->entry << " doesn't match its index sha1 " << hash << std::endl;
		}
		hgen.reset();
	}// end for each object to verify

	return res;
}


GIT_NAMESPACE_END
