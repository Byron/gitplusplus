#define BOOST_TEST_MODULE GitModelODBTest
#include <gitmodel/testutil.hpp>

#include <gitmodel/db/odb.hpp>
#include <gitmodel/db/odb_mem.hpp>

#include <type_traits>
#include <vector>
#include <utility>

using namespace gitmodel;

// force full instantiation
template class odb_base<int, int>;
template class odb_mem<int, int>;

BOOST_AUTO_TEST_CASE(cpp0x)
{
	
	
}
