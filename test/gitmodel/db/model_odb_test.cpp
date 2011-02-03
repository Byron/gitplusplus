#define BOOST_TEST_MODULE GitModelODBTest
#include <gitmodel/testutil.hpp>

#include <gitmodel/db/odb.hpp>
#include <gitmodel/db/odb_mem.hpp>

#include <boost/lambda/lambda.hpp>

#include <type_traits>
#include <vector>

BOOST_AUTO_TEST_CASE(cpp0x)
{
	static_assert(1+2==3, "should be 3");
	static_assert(std::is_pod<int>::value, "need posd");
	
	std::vector<int> v({1,2,3});
	BOOST_CHECK(v.size()==3);
	
	//std::transform(v.begin(), v.end(), v.begin(), [](decltype(v)::value_type i) {return i+1; });
	std::transform(v.begin(), v.end(), v.begin(), boost::lambda::_1 + 1 );
	BOOST_CHECK(v[0] == 1+1);
	
}
