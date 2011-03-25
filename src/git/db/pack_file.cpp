#include <git/db/pack_file.h>
#include <git/config.h>			// for doxygen

#include <cstring>

GIT_NAMESPACE_BEGIN

size_t PackCache::gMemoryLimit = 0;


void PackCache::clear() 
{
	m_info.clear();
	m_ofs.clear();
}

void PackCache::initialize(const PackIndexFile &index)
{
	if (is_available()){
		return;
	}
}

const char_type* PackCache::cache_at(uint64 offset) const 
{
	return nullptr;
}

bool PackCache::set_cache_at(uint64 offset, const char_type* data) 
{
	return false;
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
