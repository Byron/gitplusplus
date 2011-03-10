#define BOOST_TEST_MODULE GitLooseODBPerformanceTests
#include <gtl/testutil.hpp>
#include <git/fixture.hpp>
#include <git/db/odb_loose.h>

#include <boost/timer.hpp>
#include <boost/shared_array.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/null.hpp>
#include <boost/iostreams/copy.hpp>

#include <cstring>

using namespace std;
using namespace git;
namespace io = boost::iostreams;

const size_t mb = 1024*1024;

BOOST_FIXTURE_TEST_CASE(read_and_write, GitLooseODBFixture)
{
	 LooseODB lodb(rw_dir());
	 
	 // big file writing - its probably quite random as we don't initialze it
	 LooseODB::key_type big_file_key;
	 const size_t dlen = 50*mb;
	 const size_t nsf = 500;	// number of small files
	 const size_t nbf = 8192;	// number of bytes per file
	 io::basic_null_sink<LooseODB::char_type> null;
	 {
		boost::shared_array<char> data(new char[dlen]);
		BOOST_REQUIRE(data);
		
		io::stream<io::basic_array_source<char> > istream(data.get(), dlen);
		LooseODB::input_object_type iobj(Object::Type::Blob, dlen, istream);
		
		// ONE BIG FILE
		boost::timer t;
		big_file_key = lodb.insert(iobj).key();
		double elapsed = t.elapsed();
		cerr << "Added " << dlen / mb << " MiB" << " to  loose object database in " << elapsed << " s (" << (dlen/mb) / elapsed << " MiB/s)" << endl;
	 }
	 
	 // READ BIG FILE
	 {
		 boost::timer t;
		 LooseODB::output_object_type::stream_type* stream = lodb.object(big_file_key)->new_stream();
		 io::copy(*stream, null);
		 delete stream;
		 double elapsed = t.elapsed();
		 cerr << "read file with " << dlen / mb << " MiB in " << elapsed << " s (" << dlen/mb/elapsed << " MiB/s)" << std::endl;
	 }
	 
	 // MANY SMALL FILES
	 // Here the filesystem is expected to be the limiting factor
	 {
		boost::shared_array<char> data(new char[nsf * nbf]);		// every file is a few kb
		char* id = data.get();		// data iterator
		
		boost::timer t;
		for (size_t i  = 0; i < nsf; ++i, id+=nbf) {
			io::stream<io::basic_array_source<char> > istream(id, nbf);
			*reinterpret_cast<size_t*>(id) = i;	// alter memory a bit
			LooseODB::input_object_type iobj(Object::Type::Blob, nbf, istream);
			
			lodb.insert(iobj);
		}
		double elapsed = t.elapsed();
		cerr << "Added " << nsf << " files of size " << nbf << " in " << elapsed << " s (" << (double)nsf / elapsed << " files / s), database now has " << lodb.count() << " unique objects" << endl;
	 }
	 
	 // QUERY OBJECT INFORMATION ONLY
	 {
		 boost::timer t;
		 const auto end = lodb.end();
		 size_t count = 0;
		 for (auto i = lodb.begin(); i != end; ++i, ++count) {
			 i->type();
			 i->size();
		 }
		 double elapsed = t.elapsed();
		 cerr << "Queried information of " << count << " objects in " << elapsed << " s (" << (double)count / elapsed << " objects/s)" << endl;
	 }
	 
	 // READ SMALL FILES STREAMS 
	 {
		 typedef LooseODB::output_object_type::stream_type istream_type;
		 boost::timer t;
		 const auto end = lodb.end();
		 size_t count = 0;
		 size_t total = 0;
		 gtl::stack_heap<istream_type> stream;
		 for (auto i = lodb.begin(); i != end; ++i, ++count) {
			 i->type();
			 if (i->size() > nbf)
				 continue;
			 total += i->size();
			 i->stream(stream);
			 io::copy(*stream, null);
			 stream.destroy();
		 }
		 double elapsed = t.elapsed();
		 cerr << "Read " << count << " objects with total size of " << total / mb << " MiB in " << elapsed << " s (" << (double)count / elapsed << " objects/s)" << endl;
	 }
}

