#define BOOST_TEST_MODULE GitLooseODBPerformanceTests
#include <gtl/testutil.hpp>
#include <git/fixture.hpp>
#include <git/db/odb_loose.h>

#include <boost/timer.hpp>
#include <boost/shared_array.hpp>
#include <boost/iostreams/device/array.hpp>

using namespace std;
using namespace git;
namespace io = boost::iostreams;

const size_t mb = 1024*1024;

BOOST_FIXTURE_TEST_CASE(read_and_write, GitLooseODBFixture)
{
	 LooseODB lodb(rw_dir());
	 
	 // big file reading
	 const size_t dlen = 4096;
	 boost::shared_array<char> data(new char[dlen]);
	 BOOST_REQUIRE(data);
	 
	 io::stream<io::basic_array_source<char> > istream(data.get(), dlen);
	 LooseODB::input_object_type iobj(Object::Type::Blob, dlen, istream);
	 
	 boost::timer t;
	 LooseODB::accessor acc = lodb.insert(iobj);
	 double elapsed = t.elapsed();
	 std::cerr << "Added " << dlen / mb << " MiB" << " to database in " << elapsed << " s (" << (dlen/mb) / elapsed << " MiB/s)" << std::endl;
}

