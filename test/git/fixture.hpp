#ifndef TEST_GIT_FIXTURE_HPP
#define TEST_GIT_FIXTURE_HPP

#include <boost/filesystem.hpp>
#include <git/config.h>
#include <gtl/util.hpp>
#include <cassert>
#include <iostream>
#include <cstdio>
#include <cstring>


namespace fs = boost::filesystem;

/** \file fixture.hpp
  * \brief contains fixture classes and utilties to be used within testcases
  */

class FixtureUtilities
{
protected:
	//! \return full path to the resource specified by the given relative path
	fs::path fixture_path(const fs::path& relapath) const {
		return fs::path(__FILE__).parent_path() / "fixtures" / relapath;
	}
	
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
			size_t num_files = 0;
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
			return num_files;
		} else {
			fs::copy_file(source, dest);
			return 1;
		}
	}
};


/** \brief fixture to create a temporary copy of a of some fixture directory or file.
  * It takes care of cleaning it as well
  */
class BasicFixtureCopyer : public FixtureUtilities
{
protected:
	fs::path m_root_path;	//!< root under which to find the fixture files
	
public:
	BasicFixtureCopyer(const char* fixture_relapath)
	{
		fs::path source_root = fixture_path(fixture_relapath);
		fs::path dest_root = temp_file(source_root.filename());
		assert(fs::is_directory(source_root));
		fs::create_directory(dest_root);
		m_root_path = dest_root;
		copy_recursive(source_root, dest_root);
	}
	
	~BasicFixtureCopyer()
	{
		if (!m_root_path.empty()) {
			if (fs::is_regular_file(m_root_path)) {
				fs::remove(m_root_path);
			} else {
				fs::remove_all(m_root_path);
			}
		}
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

//! \brief simple fixture to create a loose object database test
struct GitLooseODBFixture : public BasicFixtureCopyer
{
	GitLooseODBFixture()
	    : BasicFixtureCopyer("loose_odb")
	{}
};

#endif // TEST_GIT_FIXTURE_HPP
