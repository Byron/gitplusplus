#ifndef TEST_GIT_FIXTURE_HPP
#define TEST_GIT_FIXTURE_HPP

#include <git/config.h>
#include <gtl/fixture.hpp>
#include <cassert>
#include <iostream>
#include <cstdio>
#include <cstring>



namespace fs = boost::filesystem;

/** \file fixture.hpp
  * \brief contains fixture classes and utilties to be used within testcases
  */

class FixtureUtilities : public gtl::fixture_file_base
{
protected:
	//! \return full path to the resource specified by the given relative path
	static fs::path fixture_path(const fs::path& relapath) {
		return fs::path(__FILE__).parent_path() / "fixtures" / relapath;
	}
};


/** \brief fixture to create a temporary copy of a of some fixture directory or file.
  * It takes care of cleaning it as well
  */
class BasicFixtureCopyer :	public gtl::file_copyer,
							public FixtureUtilities
{
public:
	BasicFixtureCopyer(const char* fixture_relapath)
		: file_copyer(fixture_path(fixture_relapath))
	{}
};

//! \brief simple fixture to create a loose object database test
struct GitLooseODBFixture : public BasicFixtureCopyer
{
	GitLooseODBFixture()
	    : BasicFixtureCopyer("loose_odb")
	{}
};

//! \brief simple fixture to create space for use with packed databases
struct GitPackedODBFixture: public BasicFixtureCopyer
{
	GitPackedODBFixture()
	    : BasicFixtureCopyer("packs")
	{}
};

#endif // TEST_GIT_FIXTURE_HPP
