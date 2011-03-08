#include <git/db/pack_file.h>
#include <git/config.h>			// for doxygen

GIT_NAMESPACE_BEGIN

PackFile::PackFile(const path_type& file)
    : m_pack_path(file)
{
	
}

bool PackFile::is_valid_path(const path_type& file)
{
	return false;
}

PackFile* PackFile::new_pack(const path_type& file)
{
	static const std::string prefix("pack-");
	static const std::string extension(".pack");
	if (file.extension() != extension || file.filename().substr(0, 5) != prefix) {
		return nullptr;
	}
	
	return new PackFile(file);
}


GIT_NAMESPACE_END
