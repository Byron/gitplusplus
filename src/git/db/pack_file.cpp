#include <git/db/pack_file.h>
#include <git/config.h>			// for doxygen

#include <cstring>

GIT_NAMESPACE_BEGIN

PackIndexFile::PackIndexFile()
{
	reset();
	static_assert(key_type::hash_len == 20, "The original pack index requires sha1 hashes");
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

PackFile::PackFile(const path_type& file)
    : m_pack_path(file)
{
	// initialize index
	path_type index_file(file);
	index_file.replace_extension(".idx");
	m_index.open(index_file.string());
}

bool PackFile::is_valid_path(const path_type& file)
{
	static const std::string prefix("pack-");
	static const std::string extension(".pack");
	return (file.extension() == extension && file.filename().substr(0, 5) == prefix);
}

PackFile* PackFile::new_pack(const path_type& file)
{
	if (!is_valid_path(file)) {
		return nullptr;
	}
	
	return new PackFile(file);
}


GIT_NAMESPACE_END
