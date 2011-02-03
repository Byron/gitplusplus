#ifndef GIT_MODEL_ODB_HPP
#define GIT_MODEL_ODB_HPP
/** \defgroup ODB Object Databases
  * Classes implementing an object database
  */

#include <git/model/modelconfig.h>
#include <git/model/db/odb_alloc.hpp>
#include <git/model/db/odb_iter.hpp>

#include <type_traits>

GIT_HEADER_BEGIN
GIT_MODEL_NAMESPACE_BEGIN
	
/** \class odb_base
  * \brief Class providing a basic interface for all derived object database implementations
  * \ingroup ODB
  * An object database behaves much like a map, which uses keys to refer to object streams.
  * An stl container interface is provided.
  * 
  * Iterators allow access to the objects of the database. Objects are always immutable.
  */
template <class Key, class T, class ObjectAllocator=std::allocator<T> >
class odb_base
{
public:
	typedef Key key_type;
	typedef T value_type;
	typedef ObjectAllocator allocator_type;
	
	typedef const odb_input_iterator<Key, T> const_iterator;
	typedef odb_input_iterator<Key, T> iterator;
	 
public:
	const_iterator find(typename std::add_rvalue_reference<const Key>::type const k) const throw();
};

		
GIT_MODEL_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_MODEL_ODB_HPP
