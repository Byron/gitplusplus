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
template <class Key, class ObjectTraits>
class odb_base
{
public:
	typedef Key key_type;
	typedef ObjectTraits object_traits;
	
	typedef const odb_input_iterator<key_type, object_traits> const_input_iterator;
	typedef odb_input_iterator<key_type, object_traits> input_iterator;
	typedef odb_forward_iterator<key_type, object_traits> forward_iterator;
	 
public:
	//! \return iterator pointing to the first item in the database
	forward_iterator begin() const throw();
	//! \return iterator pointing to the end of the database, which is one past the last item
	const forward_iterator end() const throw();
	//! \return iterator pointing to the object at the given key, or an iterator pointing to the end
	//! of the database
	const_input_iterator find(typename std::add_rvalue_reference<const key_type>::type const k) const throw();
	
	//! Insert a new item into the database
	//! \param type identifying the object
	//! \param size size of the object in bytes
	//! \param stream whose data is to be inserted
	//! \return iterator pointing to the newly inserted item in the database. It can be used to obtain the generated object key
	//!	as well.
	forward_iterator insert(typename object_traits::object_type type, size_t size, typename object_traits::istream_type stream);
};


GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_HPP
