//#define BOOST_TEST_MODULE GitLibraryTest
#define BOOST_TEST_MODULE git_lib
#include <gtl/testutil.hpp>
#include <boost/test/test_tools.hpp>
#include <git/db/odb.h>

#include <git/db/sha1.h>
#include <git/db/sha1_gen.h>
#include <git/obj/blob.h>
#include <boost/iostreams/filtering_stream.hpp>
#include <utility>

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <cstring>

using namespace std;
using namespace git;

const char* const phello = "hello";
const size_t lenphello = 5;
const std::string hello_hex_sha("AAF4C61DDCC5E8A2DABEDE0F3B482CD9AEA9434D");
const std::string null_hex_sha("0000000000000000000000000000000000000000");


BOOST_AUTO_TEST_CASE(object_type_conversion)
{
	Object::Type t(Object::Type::Tag);
	const Object::Type vals[] = { Object::Type::None, Object::Type::Blob, Object::Type::Tree, 
								Object::Type::Commit, Object::Type::Tag };
	const size_t num_vals = 5;
	for (const Object::Type* i = vals; i < vals + num_vals; ++i) {
		stringstream s;
		s << *i;
		s >> t;
		BOOST_CHECK(t == *i);
	}
}

BOOST_AUTO_TEST_CASE(lib_actor)
{
	ActorDate d1, d2, d3;
	BOOST_CHECK(d1.time == 0 && d1.tz_offset == 0);
	BOOST_CHECK(d1 == d2);
	
	d1.name = "name";
	d1.email = "email";
	d1.time = 40;
	d1.tz_offset = 800;
	
	BOOST_CHECK(!(d1 == d2));
	d2 = d1;
	BOOST_CHECK(d1 == d2);
	
	std::stringstream s;
	s << d1;
	s >> d3;
	BOOST_REQUIRE(d3 == d1);
}

BOOST_AUTO_TEST_CASE(lib_tag)
{
	Tag tag;
	Tag otag;
	BOOST_REQUIRE(tag == otag);
	tag.object_type() = Object::Type::Commit;
	tag.object_key() = SHA1();
	tag.name() = "name";
	tag.message() = "12\nmessage21\n2ndline\n";
	
	tag.actor().email = "email";
	tag.actor().name = "name";
	tag.actor().time = 1;
	tag.actor().tz_offset = 800;
	
	std::stringstream s;
	s << tag;
	BOOST_REQUIRE((size_t)s.tellp() == tag.size());
	s >> otag;
	BOOST_CHECK(tag.name() == otag.name());
	BOOST_CHECK(tag.object_type() == otag.object_type());
	BOOST_CHECK(tag.object_key() == otag.object_key());
	BOOST_CHECK(tag.actor() == otag.actor());
	BOOST_CHECK(tag.message() == otag.message());
	
	BOOST_REQUIRE(tag == otag);
	
	//! \todo maybe, add some fuzzing to be sure we don't unconditionally read garbage
}

BOOST_AUTO_TEST_CASE(lib_sha1_facility)
{
	// Test SHA1 itself
	char raw[20] = {'a','b','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a'};
	SHA1 s;
	SHA1 o(raw);
	SHA1 a('x');
	BOOST_CHECK(a[0] == 'x');
	BOOST_CHECK(s == s);
	BOOST_CHECK(s != o);
	BOOST_CHECK(o == SHA1(o));
	s = raw;
	BOOST_CHECK(s == o);
	BOOST_CHECK(s[0] == 'a');
	BOOST_CHECK(s[1] == 'b');
	
	
	// GENERATOR
	////////////
	SHA1Generator sgen;
	std::stringstream buf;
	sgen.update(phello, lenphello);
	sgen.hash(s);
	BOOST_CHECK(SHA1(sgen.digest())==s);	// duplicate call
	BOOST_CHECK_THROW(sgen.finalize(), gtl::bad_state);
	BOOST_CHECK_THROW(sgen.update("hi", 2), gtl::bad_state);
	
	
	buf << s;
	BOOST_CHECK(buf.str() == hello_hex_sha);
	buf.seekp(0, std::ios_base::beg);
	
	BOOST_CHECK(SHA1(hello_hex_sha) == s);
	BOOST_CHECK(SHA1(hello_hex_sha.c_str(), 40) == s);
	
	sgen.reset();
	// after a reset, the state changes
	sgen.hash(o);
	BOOST_CHECK(s!=o);
	buf << o;
	BOOST_CHECK(o == SHA1::null);
	BOOST_CHECK(buf.str() == null_hex_sha);
	// after reset, update works
	sgen.update("hi", 2);
	
	
	// TEST FILTER
	BOOST_CHECK(SHA1Filter().hash() == SHA1::null);
	std::stringstream null;
	boost::iostreams::stream<boost::iostreams::basic_array_source<char> > in(phello, lenphello);
	boost::iostreams::filtering_stream<boost::iostreams::input, char> filter;
	filter.push(SHA1Filter());
	filter.push(in);
	boost::iostreams::copy(filter, null);
	BOOST_CHECK(std::string(phello) == null.str());
	buf.seekp(0, std::ios::beg);
	buf << filter.component<SHA1Filter>(0)->hash();
	BOOST_CHECK(buf.str() == hello_hex_sha);
}



BOOST_AUTO_TEST_CASE(mem_db_test)
{
	MemoryODB modb;
	
	std::basic_stringstream<typename MemoryODB::traits_type::char_type> stream;
	stream.write(phello, lenphello);
	auto s = stream.tellp();
	BOOST_CHECK(s == lenphello);
	stream.seekp(0, ios_base::beg);
	
	// INSERT OPERATION
	////////////////////
	MemoryODB::input_object_type object(Object::Type::Blob, lenphello, stream);
	BOOST_CHECK(&object.stream() == &stream);
	BOOST_CHECK(object.size() == lenphello);
	BOOST_CHECK(object.type() == Object::Type::Blob);
	BOOST_CHECK(object.key_pointer() == 0);

	BOOST_REQUIRE(modb.count() == 0);	
	auto it = modb.insert(object);
	BOOST_REQUIRE(it != modb.end());
	BOOST_REQUIRE(modb.count() == 1);
	
	MemoryODB::forward_iterator fit = modb.begin();
	BOOST_CHECK(++fit == modb.end());
	
	BOOST_CHECK(it.key() ==  SHA1(hello_hex_sha));
	BOOST_CHECK(it.type() == Object::Type::Blob);
	BOOST_CHECK(it.size() == lenphello);
	
	// stream verification
	{
		MemoryODB::output_object_type::stream_type ostream;
		(*it).destroy_stream(&ostream);	// pretend the object was never constructed
		(*it).stream(&ostream);
		
		char buf[lenphello];
		ostream.read(buf, lenphello);
		
		BOOST_REQUIRE((size_t)ostream.gcount() == lenphello);
		BOOST_REQUIRE(ostream.gcount() == ostream.tellg());
		BOOST_CHECK(std::memcmp(buf, phello, lenphello)==0);
	}
	
	// Access the item using the key
	BOOST_CHECK(modb.has_object(it.key()));
	MemoryODB::accessor acc = modb.object(it.key());
	BOOST_CHECK(acc.type() == it.type());
	BOOST_CHECK(acc.size() == it.size());
	
	fit = modb.begin();
	BOOST_CHECK(fit.type() == it.type());
	BOOST_CHECK(fit.size() == it.size());
	BOOST_CHECK(fit.key() == it.key());
	
	// test adapter
	/////////////////
	gtl::odb_output_object_adapter<typename MemoryODB::output_object_type> objadapt(*it, it.key());
	
	// duplicate items should not be added - hence count remains equal
	modb.insert(objadapt);
	BOOST_CHECK(modb.count() == 1);
	
	
	// BLOB SERIALIZATION AND DESERIALIZATION
	//////////////////////////////////////////
	MultiObject mobj;
	BOOST_CHECK(mobj.type == Object::Type::None);
	
	(*it).deserialize(mobj);
	
	BOOST_CHECK(mobj.type == Object::Type::Blob);
	BOOST_CHECK(mobj.blob.data().size() == it.size());
	
	
	// insert directly
	modb.insert_object(mobj.blob);
	BOOST_REQUIRE(modb.count() == 1);
	
	// TAG SERIALIZATION AND DESERIALIZATION
	/////////////////////////////////////////
	
	
}


