//#define BOOST_TEST_MODULE GitLibraryTest
#define BOOST_TEST_MODULE git_lib
#include <gtl/testutil.hpp>
#include <boost/test/test_tools.hpp>
#include <git/db/odb.h>

#include <git/db/sha1.h>
#include <git/db/sha1_gen.h>
#include <git/obj/blob.h>
#include <boost/iostreams/filtering_stream.hpp>

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
	const std::string hello_hex_sha("aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d");
	const std::string null_hex_sha("0000000000000000000000000000000000000000");
	std::stringstream buf;
	const uchar* const phello = (const uchar*)"hello";
	sgen.update(phello, 5);
	sgen.hash(s);
	BOOST_CHECK(SHA1(sgen.digest())==s);	// duplicate call
	BOOST_CHECK_THROW(sgen.finalize(), InvalidGeneratorState);
	BOOST_CHECK_THROW(sgen.update((uchar*)"hi", 2), InvalidGeneratorState);
	
	
	buf << s;
	BOOST_CHECK(buf.str() == hello_hex_sha);
	buf.seekp(0, std::ios_base::beg);
	
	sgen.reset();
	// after a reset, the state changes
	sgen.hash(o);
	BOOST_CHECK(s!=o);
	buf << o;
	BOOST_CHECK(o == SHA1::null);
	BOOST_CHECK(buf.str() == null_hex_sha);
	// after reset, update works
	sgen.update((uchar*)"hi", 2);
	
	
	// TEST FILTER
	BOOST_CHECK(SHA1Filter().hash() == SHA1::null);
	std::basic_stringstream<uchar> null;
	//std::basic_stringstream<uchar> in(phello);
	boost::iostreams::stream<boost::iostreams::basic_array_source<uchar> > in(phello, 5);
	boost::iostreams::filtering_stream<boost::iostreams::input, uchar> filter;
	filter.push(SHA1Filter());
	filter.push(in);
	boost::iostreams::copy(filter, null);
	BOOST_CHECK(std::basic_string<uchar>(phello) == null.str());
	buf.seekp(0, std::ios::beg);
	buf << filter.component<SHA1Filter>(0)->hash();
	BOOST_CHECK(buf.str() == hello_hex_sha);
}

BOOST_AUTO_TEST_CASE(mem_db_test)
{
	MemoryODB modb;
	
	git_output_object_traits::iostream_type stream;
	stream.write("h", 1);
	auto s = stream.tellp();
	BOOST_CHECK(s == 1);
	stream.seekp(0, ios_base::beg);
	
	// auto it = modb.insert(git::Object::Type::Blob, s, stream);
	
}
