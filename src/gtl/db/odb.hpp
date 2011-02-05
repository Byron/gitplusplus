#ifndef GTL_ODB_HPP
#define GTL_ODB_HPP
/** \defgroup ODB Object Databases
  * Classes modeling the object database concept
  */

#include <gtl/config.h>
#include <gtl/db/odb_alloc.hpp>
#include <gtl/db/odb_iter.hpp>

#include <type_traits>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN


	
/** \brief Class providing a basic interface for all derived object database implementations
  * \ingroup ODB
  * An object database behaves much like a map, which uses keys to refer to object streams.
  * An stl container interface is provided.
  * 
  * Iterators allow access to the objects of the database. Objects are always immutable.
  *
  * This base serves as ideological basis for derived types which add an actual implementation of their 
  * specialized model.
  */
template <class Key, class ObjectType>
class odb_base
{
public:
	typedef Key key_type;
	typedef ObjectType object_type;
	
	typedef const odb_input_iterator<key_type, object_type> const_input_iterator;
	typedef odb_input_iterator<key_type, object_type> input_iterator;
	typedef odb_forward_iterator<key_type, object_type> forward_iterator;
	 
public:
	//! \return iterator pointing to the first item in the database
	forward_iterator begin() const throw();
	//! \return iterator pointing to the end of the database, which is one past the last item
	const forward_iterator end() const throw();
	//! \return iterator pointing to the object at the given key, or an iterator pointing to the end
	//! of the database
	const_input_iterator find(typename std::add_rvalue_reference<const key_type>::type const k) const throw();
};

GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_HPP
