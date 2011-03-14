#define BOOST_TEST_MODULE gtlODBTest
#include <gtl/testutil.hpp>

#include <gtl/db/odb.hpp>
#include <gtl/db/odb_mem.hpp>
#include <gtl/db/odb_object.hpp>
#include <gtl/util.hpp>
#include <gtl/fixture.hpp>
#include <gtl/db/mapped_memory_manager.hpp>
#include <gtl/db/sliding_mmap_device.hpp>

#include <type_traits>
#include <vector>
#include <utility>

using namespace gtl;

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
	typedef mapped_memory_manager<> man_type;
	file_creator f(1000 * 1000 * 16 + 5195, "window_test");
	// for this test, we want at least 100 windows - the size is not aligned to any page size value
	// which must work anyway (although its not very efficient). The manager should align the maps properly.
	man_type manager(f.size() / 100);
	man_type::cursor c = manager.make_cursor(f.file());
	// still no opened region
	BOOST_REQUIRE(manager.num_open_files() == 0);
	BOOST_REQUIRE(manager.mapped_memory_size() == 0);
	
	// obtain first window
	const size_t base_offset = 5000;
	BOOST_REQUIRE(c.use_region(base_offset, manager.window_size() / 2).is_valid());
	
	
	BOOST_REQUIRE(manager.num_open_files() == 1);
	BOOST_REQUIRE(manager.num_file_handles() == 1);
	BOOST_CHECK(manager.mapped_memory_size() == manager.window_size());
}
