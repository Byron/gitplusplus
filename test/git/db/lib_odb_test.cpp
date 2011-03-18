//#define BOOST_TEST_MODULE GitLibraryTest
#define BOOST_TEST_MODULE git_lib
#include <gtl/testutil.hpp>
#include <boost/test/test_tools.hpp>
#include <git/fixture.hpp>
#include <git/db/odb_loose.h>
#include <git/db/odb_mem.h>
#include <git/db/odb_pack.h>
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
const std::string hello_hex_sha_lc("aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d");
const std::string null_hex_sha("0000000000000000000000000000000000000000");

gtl::mapped_memory_manager<> manager;


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
	
	// copy construction
	SHA1Generator sgencpy(sgen);
	BOOST_REQUIRE(sgencpy.hash() == s);
	BOOST_CHECK_THROW(sgencpy.update("hi", 2), gtl::bad_state);
	
	
	buf << s;
	BOOST_CHECK(buf.str() == hello_hex_sha_lc);
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
	BOOST_CHECK(buf.str() == hello_hex_sha_lc);
}


BOOST_AUTO_TEST_CASE(mem_db_test)
{
	MemoryODB modb;
	
	std::basic_stringstream<typename MemoryODB::obj_traits_type::char_type> stream;
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
	auto obj_key = modb.insert(object);
	const auto& obj = modb.object(obj_key);
	BOOST_REQUIRE(modb.count() == 1);
	
	MemoryODB::forward_iterator fit = modb.begin();
	BOOST_CHECK(++fit == modb.end());
	
	// This cannot be verified as the implementation adds a header
	// BOOST_CHECK(it.key() ==  SHA1(hello_hex_sha));
	BOOST_CHECK(obj.type() == Object::Type::Blob);
	BOOST_CHECK(obj.size() == lenphello);
	
	// stream verification
	{
		gtl::stack_heap<typename MemoryODB::output_object_type::stream_type> ostream;
		obj.stream(ostream);
		
		char buf[lenphello];
		ostream->read(buf, lenphello);
		
		BOOST_REQUIRE((size_t)ostream->gcount() == lenphello);
		BOOST_REQUIRE(ostream->gcount() == ostream->tellg());
		BOOST_CHECK(std::memcmp(buf, phello, lenphello)==0);
		
		ostream.destroy();
	}
	
	// Access the item using the key
	BOOST_CHECK(modb.has_object(obj_key));
	BOOST_REQUIRE(!modb.has_object(MemoryODB::key_type::null));
	BOOST_REQUIRE_THROW(modb.object(MemoryODB::key_type::null), gtl::odb_error);
	
	fit = modb.begin();
	BOOST_CHECK(fit->type() == obj.type());
	BOOST_CHECK(fit->size() == obj.size());
	BOOST_CHECK(fit.key() == obj_key);
	
	// test adapter
	/////////////////
	gtl::odb_output_object_adapter<typename MemoryODB::output_object_type> objadapt(obj, obj_key);
	
	// duplicate items should not be added - hence count remains equal
	modb.insert(objadapt);
	BOOST_CHECK(modb.count() == 1);
	
	
	// BLOB SERIALIZATION AND DESERIALIZATION
	//////////////////////////////////////////
	MultiObject mobj;
	BOOST_CHECK(mobj.type == Object::Type::None);
	
	obj.deserialize(mobj);
	
	BOOST_CHECK(mobj.type == Object::Type::Blob);
	BOOST_CHECK(mobj.blob.data().size() == obj.size());
	
	
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
		
		obj_key = modb.insert_object(tag);
		const auto& obj = modb.object(obj_key);
		BOOST_REQUIRE(modb.count() == 2);
		
		mobj.destroy();
		obj.deserialize(mobj);
		
		BOOST_REQUIRE(tag == mobj.tag);
	}
	
	// TREE SERIALIZATION AND DESERIALIZATION
	///////////////////////////////////////////
	{
		Tree tree;
		tree.elements().insert(Tree::map_type::value_type("hi.txt", Tree::Element(0140644, SHA1(hello_hex_sha))));
		obj_key = modb.insert_object(tree);
		const auto& obj = modb.object(obj_key);
		BOOST_REQUIRE(modb.count() == 3);
		
		mobj.destroy();
		obj.deserialize(mobj);
		
		BOOST_REQUIRE(mobj.tree == tree);
	}
	
	// COMMIT SERIALIZATION AND DESERIALIZATION
	///////////////////////////////////////////
	{
		Commit commit;
		commit.author().name = "this";
		commit.committer().name = "that";
		commit.message() = "hi";
		
		obj_key = modb.insert_object(commit);
		const auto& obj = modb.object(obj_key);
		BOOST_REQUIRE(modb.count() == 4);
		
		mobj.destroy();
		obj.deserialize(mobj);
		
		BOOST_REQUIRE(mobj.commit == commit);
	}
	
	// TODO: test object copy from one database to anotherone
	
}

BOOST_FIXTURE_TEST_CASE(loose_db_test, GitLooseODBFixture)
{
	//! \todo templated base version of this implementation should go into the gtl tests
	typedef typename git_object_traits::char_type char_type;
	typedef typename LooseODB::input_stream_type input_stream_type;
	typedef gtl::stack_heap<input_stream_type> stack_input_stream_type;
	LooseODB lodb(rw_dir(), manager);
	const uint num_objects = 10;
	BOOST_REQUIRE(lodb.count() == num_objects);
	
	const size_t buflen = 512;
	char_type buf[buflen];
	
	auto end = lodb.end();
	uint count=0;
	for (auto it=lodb.begin(); it != end; ++it, ++count) {
		BOOST_REQUIRE(lodb.has_object(it.key()));
		{
			LooseODB::output_object_type obj = lodb.object(it.key());
			BOOST_REQUIRE(obj.size() == it->size());
			BOOST_REQUIRE(obj.type() == it->type());
		}// end object lifetime
		
		// test new stream
		{
			input_stream_type* stream = it->new_stream();
			assert(stream != 0);
			stream->read(buf, buflen);
			delete stream;
		}
		
		// test custom memory location
		{
			stack_input_stream_type sstream;
			it->stream(sstream);
			sstream->read(buf, buflen);
			sstream.destroy();
		}
		
		// test deserialization
		MultiObject mobj;
		it->deserialize(mobj);
		BOOST_REQUIRE(it->type() == mobj.type);
		
		// re-insert the object, it must have the same key
		// Cannot just keep the accessor, as it is not assignable or default-constructible
		LooseODB::key_type key;
		switch(mobj.type)
		{
		case Object::Type::Blob: { 
			key = lodb.insert_object(mobj.blob);
			break;
		}
		case Object::Type::Commit: { 
			key = lodb.insert_object(mobj.commit);
			break;
		}
		case Object::Type::Tree: { 
			key = lodb.insert_object(mobj.tree);
			break;
		}
		case Object::Type::Tag: { 
			key = lodb.insert_object(mobj.tag);
			break;
		}
		default: BOOST_REQUIRE(false);
		};//end type switch
		if (key != it.key()){
			auto obj = lodb.object(key);
			std::cerr << "Reserialzation of object of type/size: " << it->type() << "/" << it->size()
			          << " failed, got type/size: " << obj.type() << "/" << obj.size() << endl;
		}
	}// for each object in looseodb
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
		phello_key = lodb.insert(iobj);
		BOOST_REQUIRE(lodb.has_object(phello_key));
		auto* stream = lodb.object(phello_key).new_stream();
		std::string buf;
		*stream >> buf;
		BOOST_REQUIRE(buf == phello);
		delete stream;
	}
	
	// insert with key
	istream.seekg(0, std::ios::beg);
	{
		LooseODB::input_object_type iobj(Object::Type::Blob, lenphello, istream, &phello_key);
		BOOST_REQUIRE(lodb.insert(iobj) == phello_key);
	}
	
	// insert object
	{
		Commit c;
		c.author().name = c.committer().name = "sebastian";
		c.message() = "hi";
		
		auto key = lodb.insert_object(c);
		
		MultiObject mobj;
		lodb.object(key).deserialize(mobj);
		BOOST_REQUIRE(mobj.type == c.type());
		BOOST_REQUIRE(mobj.commit == c);
	}
}

BOOST_FIXTURE_TEST_CASE(packed_db_test_db_test, GitPackedODBFixture)
{
	//! \todo templated base version of this implementation should go into the gtl tests
	typedef PackODB::vector_pack_readers::const_iterator const_pack_iterator;
	gtl::mapped_memory_manager<> manager;
	
	const size_t pack_count = 3;
	PackODB podb(rw_dir(), manager);
	podb.set_lu_odb(&podb);
	BOOST_REQUIRE(podb.packs().size() == pack_count);
	
	// update calls don't change amount of items, if there was no local change
	podb.update_cache();
	BOOST_REQUIRE(podb.packs().size() == pack_count);
	
	// iterate old and new style packs
	const_pack_iterator pend = podb.packs().end();
	uint64 obj_count = 0;
	for (const_pack_iterator piter = podb.packs().begin(); piter != pend; ++piter) {
		const PackFile* pack = piter->get();
		const PackIndexFile& pack_index = pack->index();
		BOOST_CHECK(pack_index.type() != PackIndexFile::Type::Undefined);
		
		std::cerr << pack_index.type() << " - " << pack_index.version() << " - " << pack_index.num_entries() << " == " << pack->pack_path() << std::endl;
		
		// make a few calls
		BOOST_REQUIRE(pack_index.num_entries() != 0);
		obj_count += pack_index.num_entries();
		
		PackODB::key_type hash;
		for (uint32 eid = 0; eid < pack_index.num_entries(); ++eid) {
			pack_index.crc(eid);
			pack_index.sha(eid, hash);
			pack_index.offset(eid);
			
			BOOST_REQUIRE(pack_index.sha_to_entry(hash) == eid);
		}
		
		// Test pack iterators
		auto pit = pack->begin();	// pack iterator
		const auto pit_end = pack->end();
		BOOST_REQUIRE(pit == pit);
		BOOST_REQUIRE(pit_end == pit_end);
		BOOST_REQUIRE(pit != pit_end);
		for (;pit != pit_end; ++pit) {
			BOOST_REQUIRE(pack->has_object(pit.key()));
			PackOutputObject obj = *pit;
			BOOST_REQUIRE(obj.key() == pit.key());
			BOOST_REQUIRE(podb.object(obj.key()) == obj);
		}
		
		pack_index.index_checksum();
		pack_index.pack_checksum();
	}
	
	BOOST_REQUIRE(obj_count == podb.count());
	

	// ITERATION AND VALUE QUERY	
	PackODB::forward_iterator begin = podb.begin();
	const PackODB::forward_iterator end = podb.end();
	obj_count = 0;
	BOOST_REQUIRE(begin == begin);
	BOOST_REQUIRE(end == end);
	BOOST_REQUIRE(begin != end);
	for (; begin != end; ++begin, ++obj_count) {
		BOOST_REQUIRE(podb.has_object(begin.key()));
		BOOST_REQUIRE(podb.object(begin.key()) == *begin);
		
		//begin->size();
	}
	BOOST_REQUIRE(podb.count() == obj_count);
	
	
	BOOST_CHECK(false); // verify iterators in empty pack databases
	
	// TODO: Verify invalid pack reading/handling
}
