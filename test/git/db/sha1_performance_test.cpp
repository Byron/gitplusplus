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
	boost::scoped_array<const char> mem(new char[nb]);
	BOOST_CHECK(mem.get());
	boost::timer t;
	sgen.update(mem.get(), nb);
	sgen.finalize();
	cerr << "Generated SHA1 of " << nb / MB << " MB in " << t.elapsed() << " s (" << nb / t.elapsed() / MB << " MB per s)" << endl;	
}

void test_heap_allocation(bool with_delete){
	
	size_t num_bytes = 0;
	const size_t num_iterations = 100000;
	boost::timer t;
	for (size_t i = 0; i < num_iterations; ++i) {
		static const size_t sizes[] = {4, 8, 40, 150, 5000};
		const size_t size = sizes[i%5];
		char* mem = new char[size];
		num_bytes += size;
		BOOST_REQUIRE(mem != 0);
	}// for each iteration
	
	cerr << "Dynamically allocated " << num_bytes / 1000 << " KiB (" << num_iterations << " iterations) in " << t.elapsed() << " s (" << num_bytes / t.elapsed() << " KiB/s) - include delete = " << with_delete << endl;
}

BOOST_AUTO_TEST_CASE(heap_performance)
{
	test_heap_allocation(true);
	test_heap_allocation(false);
} 
