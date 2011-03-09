#include <git/db/pack_file.h>
#include <git/config.h>			// for doxygen

#include <boost/asio.hpp>		// for ntohl

#include <cstring>

GIT_NAMESPACE_BEGIN

PackIndexFile::PackIndexFile()
    : m_type(PackIndexFile::Type::Undefined)
    , m_version(0)
{
}

void PackIndexFile::open(const path_type &path)
{
	boost::iostreams::mapped_file_source::open(path);
	assert(data()!=nullptr);
	m_type = Type::Undefined;
	
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
	} else {
		m_version = 0;
	}
}

const uint32* PackIndexFile::fanout() const 
{
	const uint32* fan = reinterpret_cast<const uint32*>(data());
	if (m_type == Type::Default) {
		return fan + 2;	// skip 4 header bytes
	} else {
		return fan;
	}
}

uint32 PackIndexFile::num_entries() const
{
	return fanout()[255];
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
