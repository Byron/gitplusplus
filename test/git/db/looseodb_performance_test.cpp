#define BOOST_TEST_MODULE GitLooseODBPerformanceTests
#include <gtl/testutil.hpp>
#include <git/fixture.hpp>
#include <git/db/odb_loose.h>

#include <boost/timer.hpp>
#include <boost/shared_array.hpp>
#include <boost/iostreams/device/array.hpp>

#include <cstring>

using namespace std;
using namespace git;
namespace io = boost::iostreams;

const size_t mb = 1024*1024;

BOOST_FIXTURE_TEST_CASE(read_and_write, GitLooseODBFixture)
{
	 LooseODB lodb(rw_dir());
	 
	 // big file writing - its probably quite random as we don't initialze it
	 {
		const size_t dlen = 50*mb;
		boost::shared_array<char> data(new char[dlen]);
		BOOST_REQUIRE(data);
		
		io::stream<io::basic_array_source<char> > istream(data.get(), dlen);
		LooseODB::input_object_type iobj(Object::Type::Blob, dlen, istream);
		
		// ONE BIG FILE
		boost::timer t;
		LooseODB::accessor acc = lodb.insert(iobj);
		double elapsed = t.elapsed();
		cerr << "Added " << dlen / mb << " MiB" << " to  loose object database in " << elapsed << " s (" << (dlen/mb) / elapsed << " MiB/s)" << endl;
	 }
	 
	 
	 // MANY SMALL FILES
	 // Here the filesystem is expected to be the limiting factor
	 {
		const size_t nsf = 500;	// number of small files
		const size_t nbf = 8192;	// number of bytes per file
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
}

