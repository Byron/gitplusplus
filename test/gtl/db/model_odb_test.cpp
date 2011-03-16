#define BOOST_TEST_MODULE gtlODBTest
#include <gtl/testutil.hpp>

#include <gtl/db/odb.hpp>
#include <gtl/db/odb_mem.hpp>
#include <gtl/db/odb_object.hpp>
#include <gtl/util.hpp>
#include <gtl/fixture.hpp>
#include <gtl/db/mapped_memory_manager.hpp>
#include <gtl/db/sliding_mmap_device.hpp>

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/operations.hpp>

#include <type_traits>
#include <vector>
#include <cstring>
#include <fstream>
#include <utility>
#include <random>

using namespace gtl;

typedef mapped_memory_manager<>		man_type;

std::ostream& operator << (std::ostream& s, const typename man_type::cursor& c) {
	return s << (c.is_valid() ? "" : "!") << "< " << c.ofs_begin() << " - " << c.size() << " - " << c.ofs_end() << " >";
}

// force full instantiation
// Doesn't work with bogus types (anymore)
// All templates are instantiated by the git library
//template class odb_base<odb_object_traits>;
//template class odb_mem<odb_object_traits>;
//template class odb_mem_input_object<odb_object_traits>;
//template class odb_mem_output_object<odb_object_traits>;

struct DT /*DestrutionTester*/
{
	bool destroyed;
	int count;
	
	DT()
	    : destroyed(false)
	    , count(5)
	{}
	~DT()
	{
		destroyed = true;
	}
	
	int constmethod() const {
		return 1;
	}
	
	void incr() {
		++count;
	}
	
	bool operator==(const DT& rhs) const {
		return count == rhs.count;
	}
};

DT* test_fun(DT* p) {
	return p;
}

DT& test_fun_2(DT& r) {
	return r;
}

const DT* ctest_fun(const DT* p) {
	 return p;
}

const DT& ctest_fun_2(const DT& r) {
	 return r;
}

BOOST_AUTO_TEST_CASE(util)
{
	typedef stack_heap<DT> stack_heap_type;
	stack_heap_type sh;
	new (sh) DT;
	BOOST_REQUIRE(sh->destroyed == false);
	BOOST_REQUIRE(sh->count == 5);
	sh.destroy();
	sh->incr();
	BOOST_REQUIRE(sh->count == 6);
	
	// Auto-Conversion
	test_fun(sh);
	test_fun_2(sh);
	const stack_heap_type& csh(sh);
	ctest_fun(csh);
	ctest_fun_2(csh);
	
	csh->constmethod();
	
	BOOST_REQUIRE(*csh == *sh);
	BOOST_REQUIRE(sh->destroyed == true);
}


BOOST_AUTO_TEST_CASE(test_sliding_mappe_memory_device)
{
	typedef std::vector<char>			char_vector;	
	char_vector data;
	file_creator f(1000 * 1000 * 8 + 5195, "window_test");
	data.reserve(f.size());
	
	// copy whole file into memory to allow comparisons
	std::ifstream ifile(f.file().string().c_str(), std::ios::in|std::ios::binary);
	boost::iostreams::back_insert_device<char_vector> data_device(data);
	boost::iostreams::copy(ifile, data_device);
	
	BOOST_REQUIRE(data.size() == f.size());
	const char* pdata = &*data.begin();
	
	// Safe empty cursors
	{
		man_type::cursor c;
	}
	
	// for this test, we want at least 100 windows - the size is not aligned to any page size value
	// which must work anyway (although its not very efficient). The manager should align the maps properly.
	man_type manager(f.size() / 100, f.size() / 3, 15);
	man_type::cursor c = manager.make_cursor(f.file());
	// still no opened region
	BOOST_REQUIRE(manager.num_open_files() == 0);
	BOOST_REQUIRE(manager.mapped_memory_size() == 0);
	
	// obtain first window
	int base_offset = 5000;
	const size_t size = manager.window_size() / 2;
	BOOST_REQUIRE(c.use_region(base_offset, size).is_valid());
	const auto* pr = c.region_ptr();
	BOOST_CHECK(pr->client_count() == 1);

	BOOST_REQUIRE(manager.num_open_files() == 1);
	BOOST_REQUIRE(manager.num_file_handles() == 1);
	BOOST_CHECK(manager.mapped_memory_size() == pr->size());
	BOOST_CHECK(c.size() == size);					// should be true as we are not yet at the file's end
	BOOST_CHECK(c.ofs_begin() == base_offset);
	BOOST_CHECK(pr->ofs_begin() == 0);
	
	BOOST_REQUIRE(std::memcmp(c.begin(), pdata+base_offset, size) == 0);
	
	// obtain second window, which spans the first part of the file - it is still the same window
	BOOST_REQUIRE(c.use_region(0, size-10).is_valid());
	BOOST_CHECK(c.region_ptr() == pr);
	BOOST_REQUIRE(manager.num_file_handles() == 1);
	BOOST_REQUIRE(c.size() == size-10);
	BOOST_REQUIRE(c.ofs_begin() == 0);
	BOOST_REQUIRE(std::memcmp(c.begin(), pdata, size-10) == 0);
	
	// Map some part at the end, our requested size cannot be kept
	const size_t overshoot = 4000;
	base_offset = f.size() - size + overshoot;
	BOOST_REQUIRE(c.use_region(base_offset, size).is_valid());
	BOOST_REQUIRE(manager.num_file_handles() == 2);
	BOOST_CHECK(c.size() < size);
	BOOST_CHECK(c.region_ptr() != pr);
	BOOST_CHECK(pr->client_count() == 0);	// old region doesn't have a single cursor
	pr = c.region_ptr();
	BOOST_CHECK(pr->client_count() == 1);
	BOOST_CHECK(pr->ofs_begin() < c.ofs_begin());	// it should have mapped some part to the front
	BOOST_REQUIRE(c.uofs_end() < data.size());	// it extends the whole remaining window size to the left, so it cannot extend to the right anymore
	BOOST_REQUIRE(std::memcmp(pdata+base_offset, c.begin(), c.size())==0);
	
	// unusing a region makes the cursor invalid
	BOOST_CHECK(c.unuse_region());
	BOOST_CHECK(!c.is_valid());
	
	// iterate through the windows, verify data contents
	// This will trigger map collections after a while
	uint num_random_accesses = 10000;
	std::uniform_int_distribution<uint64_t> distribution(0ul, c.file_size());
	std::mt19937 engine;
	try {
		while (num_random_accesses--) {
			base_offset = distribution(engine);
			const auto* r = c.region_ptr();
			if (r) {
				BOOST_REQUIRE(r->client_count() < 2);
			}
			BOOST_REQUIRE(manager.max_mapped_memory_size() >= manager.mapped_memory_size());
			BOOST_REQUIRE(manager.max_file_handles() >= manager.num_file_handles());
			BOOST_REQUIRE(c.use_region(base_offset, size).is_valid());
			BOOST_REQUIRE(std::memcmp(c.begin(), pdata+c.ofs_begin(), c.size()) == 0);
		}
	} catch (const std::exception&) {
		BOOST_REQUIRE(false);
	}
	
	// Request memory beyond the size of the file
	BOOST_REQUIRE(!c.use_region(f.size(), 100).is_valid());
}
