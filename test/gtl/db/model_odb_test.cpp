#define BOOST_TEST_MODULE gtlODBTest
#include <gtl/testutil.hpp>

#include <gtl/db/odb.hpp>
#include <gtl/db/odb_mem.hpp>
#include <gtl/db/odb_object.hpp>
#include <gtl/util.hpp>

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

struct DestructionTester
{
	bool destroyed;
	int count;
	
	DestructionTester()
	    : destroyed(false)
	    , count(5)
	{}
	~DestructionTester()
	{
		destroyed = true;
	}
	
	void constmethod() const {
		int x = 1;
	}
	
	void incr() {
		++count;
	}
	
	bool operator==(const DestructionTester& rhs) const {
		return count == rhs.count;
	}
};

void test_fun(DestructionTester* p) {
	DestructionTester* x = p;
}

void test_fun_2(DestructionTester& r) {
	DestructionTester& x = r;
}

void ctest_fun(const DestructionTester* p) {
	const DestructionTester* x = p;
}

void ctest_fun_2(const DestructionTester& r) {
	const DestructionTester& x = r;
}

BOOST_AUTO_TEST_CASE(util)
{
	typedef stack_heap<DestructionTester> stack_heap_type;
	stack_heap_type sh;
	new (sh) DestructionTester;
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
