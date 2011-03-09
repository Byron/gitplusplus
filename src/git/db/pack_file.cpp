#include <git/db/pack_file.h>
#include <git/config.h>			// for doxygen

GIT_NAMESPACE_BEGIN

PackIndexFile::PackIndexFile()
    : m_version(PackIndexFile::Version::None)
{
}

void PackIndexFile::open(const path_type &path){
	boost::iostreams::mapped_file_source::open(path);
	
	
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
