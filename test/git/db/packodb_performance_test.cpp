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

const size_t mb = 1000*1000;

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
	
	double deserialization_elapsed = 0.;
	{// deserialize data
		PackODB::forward_iterator beg = podb.begin();
		MultiObject	mobj;
		std::vector<ObjectType> types = {ObjectType::Tag, ObjectType::Commit, ObjectType::Tree, ObjectType::Blob};	
		size_t sno = 0;	// successful deserializations
		
		for (auto type = types.begin(); type != types.end(); ++type) {
			size_t no = 0;	// num objects
			size_t tbc = 0;	// total size of each object type
			size_t failures = 0;
			double elapsed = 0.0;
			timer t;
			for (PackODB::forward_iterator beg = podb.begin(); beg != end; ++beg) {
				if (beg->type() != *type) continue;
				BOOST_REQUIRE(beg->type() != ObjectType::None);
				try {
					beg->deserialize(mobj);
					tbc += beg->size();
				} catch (const std::exception& err) {
					std::cerr << beg.key() << " failed: " << err.what() << std::endl;
					++failures;
					BOOST_REQUIRE(mobj.type == ObjectType::None);
					continue;
				}
				++no;
				mobj.destroy();
			}// for each pack object
			elapsed = t.elapsed();
			deserialization_elapsed += elapsed;
			sno += no;
			
			if (failures) {
				std::cerr << "Failed to deserialized " << failures << " " << *type << " objects" << std::endl;
			}
			tbc = tbc / mb;
			std::cerr << "Deserialized " << no << " " << *type << " objects totalling " << tbc <<   " MB in " << elapsed << " s (" << no / elapsed<< " objects/s and " << tbc / elapsed << " mb/s)" << std::endl;
		}// end for each object type
		
		std::cerr << "Deserialized " << sno << " of " << no << " objects in " << deserialization_elapsed << " s (" << sno / deserialization_elapsed << " objects/s)" << std::endl;
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
				br = static_cast<size_t>(pstream->gcount());
				tbc += br;
			} while (br == bufsize);
			pstream.destroy();
		}
		streaming_elapsed = t.elapsed();
		const double tmb = tbc / mb;
		std::cerr << "Streamed " << no  << " objects totalling " << tmb <<  " mb in " << streaming_elapsed << " s (" << no / streaming_elapsed<< " streams/s & " << tmb / streaming_elapsed  << " mb/s)" << std::endl;
	}
}
