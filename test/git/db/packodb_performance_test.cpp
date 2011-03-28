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
 
	const size_t no = podb.count();			// number of objects
	const PackODB::forward_iterator end = podb.end();
	std::cerr << "Operating on database with " << podb.packs().size() << " packs and " << no << " objects" << std::endl;
	
	
	/*
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
	}*/
	
	// Test cache performance
	uint64 max_size = 0;
	for (auto it = podb.packs().begin(); it < podb.packs().end(); ++it) {
		max_size = std::max(max_size, it->get()->cursor().file_size());
	}
	BOOST_REQUIRE(max_size);
	
	size_t min_cache_size = 8*mb;
	std::vector<size_t> cache_sizes = {/*std::max((size_t)(max_size * 0.25f), min_cache_size),*/ 
	                                   std::max(size_t(max_size * 0.7f), min_cache_size*3), 
	                                   /*0*/};
	for (auto cache_size = cache_sizes.begin(); cache_size < cache_sizes.end(); ++cache_size)
	{
		std::cerr << "########################################" << std::endl;
		std::cerr << "CACHE SIZE == " << *cache_size / mb << " mb" << std::endl;
		std::cerr << "########################################" << std::endl;
		
		podb.set_cache_memory_limit(*cache_size);
		BOOST_CHECK(podb.cache_memory_limit() == *cache_size);
		
		{
			for (auto pit = podb.packs().begin(); pit != podb.packs().end(); ++pit) {
				timer t;
				bool res = pit->get()->verify(std::cerr);
				double elapsed = t.elapsed();
				uint64 pack_size = pit->get()->cursor().file_size();
				std::cerr << "Verified pack at " << pit->get()->pack_path() << " of " << pack_size / mb << "mb in " << elapsed << "s (" << (pack_size / mb) / elapsed << "mb / s)" << std::endl;
				BOOST_REQUIRE(res);
			}
		}// end verify pack
		/*
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
				std::for_each(podb.packs().begin(), podb.packs().end(), [](const PackODB::vector_pack_readers::value_type& p){p.get()->cache().cache_info(std::cerr);});
			}// end for each object type
			
			std::cerr << "Deserialized " << sno << " of " << no << " objects in " << deserialization_elapsed << " s (" << sno / deserialization_elapsed << " objects/s)" << std::endl;
		}// end deserialize objects
	
		// This run currently benefits from the cache build during the previous runs ! Its okay
		// as this gices us a good idea about the maximum speed we can achieve, versus the speed we
		// get when we have to deserialize the objects
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
		}// stream data
		*/
		std::cerr << "--------> TOTAL CACHE SIZE == " << podb.cache_memory() / mb << " mb" << std::endl;
	}// end cache size
}
