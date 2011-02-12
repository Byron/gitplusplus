#ifndef GTL_ODB_HPP
#define GTL_ODB_HPP

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
template <class ObjectTraits>
class odb_base
{
public:
	typedef ObjectTraits traits_type;
	
	typedef const odb_input_iterator<traits_type> const_input_iterator;
	typedef odb_input_iterator<traits_type> input_iterator;
	typedef odb_forward_iterator<traits_type> forward_iterator;
	 
public:
	//! \return iterator pointing to the first item in the database
	forward_iterator begin() const throw();
	//! \return iterator pointing to the end of the database, which is one past the last item
	const forward_iterator end() const throw();
	//! \return iterator pointing to the object at the given key, or an iterator pointing to the end
	//! of the database. Dereferencing the iterator yields access to an output object, which remains valid
	//! only as long as the iterator exists.
	const_input_iterator find(typename std::add_rvalue_reference<const typename traits_type::key_type>::type const k) const throw();
	
	//! Insert a new item into the database
	//! \param type identifying the object
	//! \param size size of the object in bytes
	//! \param stream whose data is to be inserted
	//! \return iterator pointing to the newly inserted item in the database. It can be used to obtain the generated object key
	//!	as well.
	template <class Stream>
	forward_iterator insert(typename traits_type::object_type type, size_t size, Stream stream);
	
	//! Same as above, but will produce the required serialized version of object automatically
	forward_iterator insert(typename traits_type::input_reference_type object);
};


GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_HPP
