#define BOOST_TEST_MODULE GitSHA1PerformanceTests
#include <gtl/testutil.hpp>

#include <git/db/sha1_gen.h>
#include <boost/scoped_array.hpp>
#include <boost/timer.hpp>

using namespace std;
using namespace git;

BOOST_AUTO_TEST_CASE(sha1_performance)
{
	// GENERATOR
	////////////
	SHA1Generator sgen;
	const size_t MB = 1024*1024;
	static const size_t nb = MB*100;
	boost::scoped_array<const uchar> mem(new uchar[nb]);
	BOOST_CHECK(mem.get());
	boost::timer t;
	sgen.update(mem.get(), nb);
	sgen.finalize();
	cerr << "Generated SHA1 of " << nb / MB << " MB in " << t.elapsed() << " s (" << nb / t.elapsed() / MB << " MB per s)" << endl;	
}
