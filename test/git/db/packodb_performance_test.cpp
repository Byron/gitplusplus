#define BOOST_TEST_MODULE GitPackODBPerformanceTests
#include <gtl/testutil.hpp>
#include <git/fixture.hpp>
#include <gtl/util.hpp>
#include <git/db/odb_pack.h>
#include <gtl/db/mapped_memory_manager.hpp>

#include <boost/timer.hpp>
#include <boost/scoped_array.hpp>
#include <boost/iostreams/device/null.hpp>
#include <boost/iostreams/copy.hpp>

#include <cstring>
#include <cstdlib>

using namespace std;
using namespace git;
namespace io = boost::iostreams;
using boost::timer;

const size_t mb = 1024*1024;

gtl::mapped_memory_manager<> manager;

BOOST_AUTO_TEST_CASE(read_pack)
{
	typedef gtl::stack_heap<PackODB::output_object_type::stream_type> stream_heap;
	        
	const char* const env_var = "GITPP_PERFORMANCE_PACK_DIR";
	const char* val = getenv(env_var);
	if (!val) {
		std::cerr << "Please set the " << env_var << " environment variable to point to a .git/objects/pack directory with plenty of larget packs" << std::endl;
		BOOST_REQUIRE(false);
	}
	
	const PackODB podb(val, manager);
	BOOST_REQUIRE(podb.packs().size());
	BOOST_REQUIRE(podb.count() > 10000);	// yes, make sure we have at least something
 
	const size_t no = podb.count();			// number of objects
	const PackODB::forward_iterator end = podb.end();
	std::cerr << "Operating on database with " << podb.packs().size() << " packs and " << no << " objects" << std::endl;
	
	
	
	double iteration_elapsed;
	{// ITERATION - no access
		PackODB::forward_iterator beg = podb.begin();
		timer t;
		for(; beg != end; ++beg);
		iteration_elapsed = t.elapsed();
		std::cerr << "Iterated " << no << " objects without any access in " << iteration_elapsed << " s (" << no / iteration_elapsed << " objects/s)" << std::endl;
	}
	
	double query_elapsed;
	{// Query information
		PackODB::forward_iterator beg = podb.begin();
		timer t;
		for (; beg != end; ++beg) {
			beg->type();
			beg->size();
		}
		query_elapsed = t.elapsed();
		std::cerr << "Iterated and queried type+size of " << no  << " objects in " << query_elapsed << " s (" << no / query_elapsed << " queries/s)" << std::endl;
	}

	const size_t bufsize = 4096;
	PackODB::char_type buf[bufsize];
	size_t tbc;					// total byte count
	double streaming_elapsed;
	{// stream data
		PackODB::forward_iterator beg = podb.begin();
		stream_heap pstream;
		timer t;
		for (tbc = 0; beg != end; ++beg) {
			beg->stream(pstream);
			size_t br;
			do {
				pstream->read(buf, bufsize);
				size_t br = static_cast<size_t>(pstream->gcount());
				tbc += br;
			} while (br == bufsize);
			pstream.destroy();
		}
		streaming_elapsed = t.elapsed();
		const double tmb = tbc / mb;
		std::cerr << "Streamed " << no  << " objects totalling " << tmb <<  " MB in " << streaming_elapsed << " s (" << no / streaming_elapsed<< " streams/s & " << tmb / streaming_elapsed  << " MB/s)" << std::endl;
	}
	
	{ // deserialize data
		
	}
		
}
