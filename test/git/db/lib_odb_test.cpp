//#define BOOST_TEST_MODULE GitLibraryTest
#define BOOST_TEST_MODULE git_lib
#include <gtl/testutil.hpp>
#include <boost/test/test_tools.hpp>
#include <git/db/odb_loose.h>
#include <git/db/odb_mem.h>

#include <git/db/sha1.h>
#include <git/db/sha1_gen.h>
#include <git/obj/blob.h>
#include <boost/iostreams/filtering_stream.hpp>
#include <utility>
#include <git/fixture.hpp>

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <cstring>

using namespace std;
using namespace git;
namespace io = boost::iostreams;

const char* const phello = "hello";
const size_t lenphello = 5;
const std::string hello_hex_sha("AAF4C61DDCC5E8A2DABEDE0F3B482CD9AEA9434D");
const std::string hello_hex_sha_lc("aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d");
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
	
	d1.name = "name lastname";
	d1.email = "e@mail";
	d1.time = 40;
	d1.tz_offset = 800;
	
	BOOST_CHECK(!(d1 == d2));
	d2 = d1;
	BOOST_CHECK(d1 == d2);
	
	std::stringstream s;
	s << d1;
	s >> d3;
	BOOST_REQUIRE(d3 == d1);
	
	// empty names are not allowed
	ActorDate d4;
	BOOST_REQUIRE_THROW(s << d4, DeserializationError);
}

BOOST_AUTO_TEST_CASE(lib_commit)
{
	for (uint enc = 0; enc < 2; enc++) {
		for (uint multi = 0; multi < 2; multi++) {
			Commit c, oc;
			BOOST_CHECK(c == oc);
			
			c.message() = "\nstarts with newline\nhello there ends with newline\n\n";
			c.author().name = "authoria";
			c.committer().name = "commiteria";
			
			c.parent_keys().push_back(SHA1(SHA1::null));
			if (multi) {
				c.parent_keys().push_back(SHA1(hello_hex_sha));
			}
			if (enc) {
				c.encoding() = "myencoding";
			}
			
			BOOST_CHECK(!(c == oc));
			
			std::stringstream s;
			s << c;
			s >> oc;
			
			BOOST_REQUIRE(c == oc);
		}// end multi-parent mode
	}// end encoding loop
}

BOOST_AUTO_TEST_CASE(lib_tag)
{
	Tag tag;
	Tag otag;
	BOOST_REQUIRE(tag == otag);
	tag.object_type() = Object::Type::Commit;
	tag.name() = "name with spaces";
	tag.message() = "12\nmessage21\n2ndline\n";
	
	tag.actor().email = "email@something";
	tag.actor().name = "firstname lastname";
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
	
	// empty message
	tag.message().clear();
	for (uint exc = 0; exc < 2; exc++) {
		std::stringstream s2;
		s2.exceptions(exc ? std::ios_base::eofbit : std::ios_base::goodbit);
		s2 << tag;
		s2 >> otag;
		BOOST_CHECK(tag == otag);
	}// verify exception handling
	
	
	//! \todo maybe, add some fuzzing to be sure we don't unconditionally read garbage
}

BOOST_AUTO_TEST_CASE(lib_tree)
{
	Tree tree, otree;
	BOOST_REQUIRE(tree == otree);
	
	
	Tree::map_type& elms = tree.elements();
	elms.insert(Tree::map_type::value_type("hi there", Tree::Element(0100644, SHA1())));
	elms.insert(Tree::map_type::value_type("hello world", Tree::Element(0040755, SHA1())));
	
	for (uint exc = 0; exc < 2; exc++) {
		std::stringstream s;
		s.exceptions(exc ? std::ios_base::eofbit : std::ios_base::goodbit);
		s << tree;
		s >> otree;
		BOOST_CHECK(otree.elements().size() == tree.elements().size());
		BOOST_CHECK(otree.elements().begin()->first == tree.elements().begin()->first);
		BOOST_REQUIRE(otree == tree);
	}// for each exception state
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
	
	// upper/lower case hex input yields same results
	BOOST_REQUIRE(SHA1(hello_hex_sha) == SHA1(hello_hex_sha_lc));
	
	
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
	
	// This cannot be verified as the implementation adds a header
	// BOOST_CHECK(it.key() ==  SHA1(hello_hex_sha));
	BOOST_CHECK(it->type() == Object::Type::Blob);
	BOOST_CHECK(it->size() == lenphello);
	
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
	BOOST_REQUIRE(!modb.has_object(MemoryODB::key_type::null));
	MemoryODB::accessor acc = modb.object(it.key());
	BOOST_REQUIRE_THROW(modb.object(MemoryODB::key_type::null), gtl::odb_error);
	BOOST_CHECK(acc->type() == it->type());
	BOOST_CHECK(acc->size() == it->size());
	
	fit = modb.begin();
	BOOST_CHECK(fit->type() == it->type());
	BOOST_CHECK(fit->size() == it->size());
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
	BOOST_CHECK(mobj.blob.data().size() == it->size());
	
	
	// insert directly
	modb.insert_object(mobj.blob);
	BOOST_REQUIRE(modb.count() == 1);
	
	// TAG SERIALIZATION AND DESERIALIZATION
	/////////////////////////////////////////
	{
		Tag tag;
		tag.name() = "my tag";
		tag.message() = "my message";
		tag.actor().name = "me";
		tag.actor().email = "me@you.com";
		
		it = modb.insert_object(tag);
		BOOST_REQUIRE(modb.count() == 2);
		
		mobj.~MultiObject();
		(*it).deserialize(mobj);
		
		BOOST_REQUIRE(tag == mobj.tag);
	}
	
	// TREE SERIALIZATION AND DESERIALIZATION
	///////////////////////////////////////////
	{
		Tree tree;
		tree.elements().insert(Tree::map_type::value_type("hi.txt", Tree::Element(0140644, SHA1(hello_hex_sha))));
		it = modb.insert_object(tree);
		BOOST_REQUIRE(modb.count() == 3);
		
		mobj.~MultiObject();
		(*it).deserialize(mobj);
		
		BOOST_REQUIRE(mobj.tree == tree);
	}
	
	// COMMIT SERIALIZATION AND DESERIALIZATION
	///////////////////////////////////////////
	{
		Commit commit;
		commit.author().name = "this";
		commit.committer().name = "that";
		commit.message() = "hi";
		
		it = modb.insert_object(commit);
		BOOST_REQUIRE(modb.count() == 4);
		
		mobj.~MultiObject();
		(*it).deserialize(mobj);
		
		BOOST_REQUIRE(mobj.commit == commit);
	}
	
	// TODO: test object copy from one database to anotherone
	
}


BOOST_FIXTURE_TEST_CASE(loose_db_test, GitLooseODBFixture)
{
	typedef typename git_object_traits::char_type char_type;
	typedef typename LooseODB::input_stream_type input_stream_type;
	LooseODB lodb(rw_dir());
	const uint num_objects = 10;
	BOOST_REQUIRE(lodb.count() == num_objects);
	
	const size_t buflen = 512;
	char_type buf[buflen];
	
	auto end = lodb.end();
	uint count=0;
	for (auto it=lodb.begin(); it != end; ++it, ++count) {
		BOOST_REQUIRE(lodb.has_object(it.key()));
		{
			LooseODB::accessor acc = lodb.object(it.key());
			BOOST_REQUIRE(acc->size() == it->size());
			BOOST_REQUIRE(acc->type() == it->type());
		}// end accessor lifetime
		
		// test new stream
		input_stream_type* stream = it->new_stream();
		assert(stream != 0);
		stream->read(buf, buflen);
		delete stream;
		
		// test custom memory location
		char sbuf[sizeof(input_stream_type)];
		stream = (input_stream_type*)sbuf;
		it->stream(stream);
		stream->read(buf, buflen);
		it->destroy_stream(stream);
		
		// test deserialization
		MultiObject mobj;
		it->deserialize(mobj);
		BOOST_REQUIRE(it->type() == mobj.type);
	}
	BOOST_REQUIRE(count == num_objects);
	BOOST_REQUIRE(!lodb.has_object(LooseODB::key_type::null));
	BOOST_REQUIRE_THROW(lodb.object(LooseODB::key_type::null), gtl::odb_error);
	
	// OBJECT INSERTION
	///////////////////
	// insert without key
	io::stream<io::basic_array_source<char_type> > istream(phello, lenphello);
	LooseODB::key_type phello_key;
	{
		LooseODB::input_object_type iobj(Object::Type::Blob, lenphello, istream);
		phello_key = lodb.insert(iobj).key();
	}
	
	// insert with key
	istream.seekg(0, std::ios::beg);
	{
		LooseODB::input_object_type iobj(Object::Type::Blob, lenphello, istream, &phello_key);
	}
	
	
}
