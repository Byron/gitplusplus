#define BOOST_TEST_MODULE gtlODBTest
#include <gtl/testutil.hpp>

#include <gtl/db/odb.hpp>
#include <gtl/db/odb_mem.hpp>
#include <gtl/db/odb_object.hpp>

#include <type_traits>
#include <vector>
#include <utility>

using namespace gtl;

// force full instantiation
template class odb_base<odb_object_traits>;
template class odb_mem<odb_object_traits>;



BOOST_AUTO_TEST_CASE(cpp0x)
{
}
