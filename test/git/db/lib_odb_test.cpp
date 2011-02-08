#define BOOST_TEST_MODULE GitLibraryTest
#include <gtl/testutil.hpp>
#include <git/db/odb.h>

#include <git/db/sha1.h>
#include <git/db/sha1_gen.h>
#include <git/obj/blob.h>

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>

using namespace std;
using namespace git;

BOOST_AUTO_TEST_CASE(lib_sha1_facility)
{
	// Test SHA1 itself
	uchar raw[20] = {'a','b','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a'};
	SHA1 s;
	SHA1 o(raw);
	SHA1 a('x');
	BOOST_CHECK(a[0] == 'x');
	BOOST_CHECK(s == s);
	BOOST_CHECK(s != o);
	s = raw;
	BOOST_CHECK(s == o);
	BOOST_CHECK(s[0] == 'a');
	BOOST_CHECK(s[1] == 'b');
	
	
	// GENERATOR
	////////////
	SHA1Generator sgen;
	const std::string hello_sha("aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d");
	const std::string null_sha("0000000000000000000000000000000000000000");
	std::stringstream buf;
	sgen.update((uchar*)"hello", 5);
	sgen.finalize();
	sgen.hash(s);
	
	buf << s;
	BOOST_CHECK(buf.str() == hello_sha);
	buf.seekp(0, std::ios_base::beg);
	
	sgen.reset();
	// after a reset, the state changes
	sgen.hash(o);
	BOOST_CHECK(s!=o);
	buf << o;
	cerr << buf.str() << endl;
	BOOST_CHECK(o == SHA1::null_sha);
	BOOST_CHECK(buf.str() == null_sha);
	
}

BOOST_AUTO_TEST_CASE(mem_db_test)
{
	MemoryODB modb;
	
	git_output_object_traits::iostream_type stream;
	stream.write("h", 1);
	auto s = stream.tellp();
	BOOST_CHECK(s == 1);
	stream.seekp(0, ios_base::beg);
	
	auto it = modb.insert(git::Object::Type::Blob, s, stream);
	
	
}
