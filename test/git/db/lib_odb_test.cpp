#define BOOST_TEST_MODULE GitLibraryTest
#include <gitmodel/testutil.hpp>
#include <git/db/odb.h>

#include <git/db/sha1.h>
#include <git/db/sha1_gen.h>

using namespace git;

BOOST_AUTO_TEST_CASE(lib_sha1_facility)
{
	// Test SHA1 itself
	uchar raw[20] = {'a','b','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a'};
	SHA1 s;
	SHA1 o(raw);
	BOOST_CHECK(s == s);
	BOOST_CHECK(s != o);
	s = raw;
	BOOST_CHECK(s == o);
	BOOST_CHECK(s[0] == 'a');
	BOOST_CHECK(s[1] == 'b');
	
	
	// Test sha1 generator
	SHA1Generator sgen;
	sgen.update((uchar*)"hello", 4);
	sgen.finalize();
	sgen.hash(s);
	sgen.reset();
	// after a reset, the state changes
	sgen.hash(o);
	
	BOOST_CHECK(s!=o);
	
}

BOOST_AUTO_TEST_CASE(lib_base_test)
{
	int i = 0;
	auto x = i;
}
