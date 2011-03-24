#ifndef TEST_GTL_FIXTURE_HPP
#define TEST_GTL_FIXTURE_HPP

#include <gtl/config.h>
#include <gtl/util.hpp>
#include <boost/filesystem.hpp>

#include <fstream>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace fs = boost::filesystem;

/** Provides a basis with general file based fixture utlities
  */
class fixture_file_base
{
protected:
	//! \return a path pointing to a non-existing temporary file/directory in some temporary space
	fs::path temp_file(const std::string& prefix) const {
		fs::path p;
		gtl::temppath(p, prefix.empty() ? 0 : prefix.c_str());
		return p;
	}
	
	//! Recursively copy a directory into another directory
	//! \return number of copied files
	//! \note if source is not a directory, we will copy a single file (for convenience
	size_t copy_recursive(const fs::path& source, const fs::path& dest) const {
		if (fs::is_directory(source)) {
			size_t num_paths = 0;
			fs::directory_iterator end;
			fs::directory_iterator dit(source);
			
			for (;dit != end; ++dit) {
				const fs::path& p = dit->path();
				const fs::path dest_path(dest/p.filename());
				if (fs::is_directory(dit->status())) {
					fs::create_directory(dest_path);
					copy_recursive(p, dest_path);
				} else {
					fs::copy_file(p, dest_path);
				}// handle directory/file
			}// for each item in directory
			return num_paths;
		} else {
			fs::copy_file(source, dest);
			return 1;
		}
	}
	
	//! remove the given path, which may be empty, a directory a or a file
	void remove_path(const fs::path& path) {
		if (!path.empty()) {
			if (fs::is_regular_file(path)) {
				fs::remove(path);
			} else if (fs::is_directory(path)) {
				fs::remove_all(path);
			}
		}
	}
	
};

/** \brief fixture to create a temporary copy of a of some fixture directory or file.
  * It takes care of cleaning it as well
  */
class file_copyer : public fixture_file_base
{
protected:
	fs::path m_root_path;	//!< root under which to find the fixture files
	
public:
	file_copyer(const fs::path& source_root)
	{
		fs::path dest_root = temp_file(source_root.filename());
		assert(fs::is_directory(source_root));
		fs::create_directory(dest_root);
		m_root_path = dest_root;
		copy_recursive(source_root, dest_root);
	}
	
	~file_copyer()
	{
		remove_path(m_root_path);
	}
	
public:
	//! \return source directory with the temporary writable files
	//! \throws if the rw path does not point to a directory
	const fs::path& rw_dir() const {
		if (!fs::is_directory(m_root_path)) {
			throw fs::filesystem_error("caller expected a directory, which was a file", boost::system::error_code());
		}
		return m_root_path;
	}
	
	//! \return readable and writable source file
	//! \throws if the source file is a directory
	const fs::path& rw_file() const {
		if (!fs::is_regular_file(m_root_path)) {
			throw fs::filesystem_error("source is not a file", boost::system::error_code());
		}
		return m_root_path;
	}
};


/** \brief simple type which manages a file's lifetime. The file can be created by any means
  */
class path_manager_base : public fixture_file_base
{
protected:
	fs::path m_path;
	size_t m_size;
	
protected:
	//! Create a temporary file with the given size in bytes and prefix
	path_manager_base()
		: m_size(0)
	{}
	
	~path_manager_base()
	{
		remove_path(m_path);
	}
	
public:
	const fs::path& file() const {
		assert(fs::is_regular_file(m_path));
		return m_path;
	}
	
	const fs::path& directory() const {
		assert(fs::is_directory(m_path));
		return m_path;
	}
	
	//! \return size of files or amount of items in directory (depending on the implementation)
	size_t size() const {
		return m_size;
	}
};

/** \brief utilty to create a temporary file filled with relatively random memory contents
  */
class file_creator : public path_manager_base
{
public:
	//! Create a temporary file with the given size in bytes and prefix
	file_creator(size_t size, const char* prefix = nullptr)
	{
		const uint32 buflen = 65536;
		char buf[buflen];
		m_path = temp_file(prefix);
		m_size = size;
		std::ofstream of;
		of.open(m_path.string().c_str(), std::ios::out|std::ios::binary|std::ios::trunc);
		
		for (uint32 nb = size / buflen; nb; --nb) {
			of.write(buf, buflen);
		}
		auto remainder = size - ((size / buflen) * buflen);
		if (remainder) {
			of.write(buf, remainder);
		}
		of.close();
		assert(boost::filesystem::file_size(m_path) == size);
	}
	
	//! Create a temporary file from the given initialized memory
	template <class Iterator>
	file_creator(Iterator beg, Iterator end, const char* prefix = nullptr)
	{
		m_path = temp_file(prefix);
		std::ofstream of;
		of.open(m_path.string().c_str(), std::ios::out|std::ios::binary|std::ios::trunc);
		
		for (;beg != end; ++beg) {
			of << *beg;
		}
		
		of.close();
		m_size = file_size(m_path);
	}
};


/** \brief creates a directory and assures it gets deleted on its own destruction
  */
class directory_creator : public path_manager_base
{
public:
	directory_creator(const char* prefix = nullptr) {
		m_path = temp_file(prefix);
		fs::create_directory(m_path);
		
		fs::directory_iterator it(m_path);
		fs::directory_iterator end;
		for (; it != end; ++it, ++m_size);
	}
};

GTL_NAMESPACE_END
GTL_HEADER_END

#endif // TEST_GTL_FIXTURE_HPP
