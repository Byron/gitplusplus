#define BOOST_TEST_MODULE gtlODBTest
#include <gtl/testutil.hpp>

#include <gtl/db/odb.hpp>
#include <gtl/db/odb_mem.hpp>
#include <gtl/db/odb_stream.hpp>

#include <type_traits>
#include <vector>
#include <utility>

using namespace gtl;

// force full instantiation
typedef odb_ostream<bool> ostream_type;
template class odb_base<int, ostream_type>;
template class odb_mem<int, ostream_type>;


/** Test behaviour of relaxed unions and unions in general
  * the are expected to have an own constructor and destructor.
  */
union U
{
	size_t* count;
	int i;
	float f;
	double d;
	double destructed;
	
	U(size_t* c) : count(c) { ++(*count); }
	~U() { --(*count); }
};

// strongly typed enum, non-int
enum class Type : unsigned char
{
	tTree, 
	tCommit,
	tNone
};


class Object
{
protected:
	Type type;
public:
	Object(Type t) : type(t) {}
	~Object() { type = Type::tNone; }
};

class Tree : public Object
{
	size_t ec;
public:
	Tree() : Object(Type::tTree), ec(0) {}
	
};

class Commit : public Object
{
	const Commit* parent;
public:
	Commit() : Object(Type::tCommit), parent(0) {}
};

/** Unions with non-pod types */
union U2
{
	Type type;
	Tree tree;
	Commit commit;
	
	U2() : type(Type::tNone) {}
	~U2() { 
		if (type == Type::tTree)
			tree.~Tree();
		else if (type == Type::tCommit)
			commit.~Commit();
	}
};

BOOST_AUTO_TEST_CASE(cpp0x)
{
	static_assert(sizeof(U) == 8, "invalid union size");
	
	size_t ucount = 0;
	{
		U u(&ucount);
		BOOST_CHECK(ucount == 1);
	}
	BOOST_CHECK(ucount == 0);
	
	U2 u;
	BOOST_CHECK(u.type == Type::tNone);
	new (&u.tree) Tree;
	BOOST_CHECK(u.type == Type::tTree);
	u.~U2();
	BOOST_CHECK(u.type == Type::tNone);
	new (&u.commit) Commit;
	BOOST_CHECK(u.type == Type::tCommit);
	
}
