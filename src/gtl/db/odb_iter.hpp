#ifndef GTL_ODB_ITERATOR_HPP
#define GTL_ODB_ITERATOR_HPP

#include <gtl/config.h>
#include <iterator>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN
	
/** \ingroup ODBIter
  * \brief pointer allowing read-only access to the underlying object.
  * It is considered to be nothing more than a pointer to a specific object, the iterator
  * should not contain any additional information, it cannot be moved.
  */
template <class TraitsType>
class odb_accessor : public std::iterator<typename std::input_iterator_tag, TraitsType>
{
protected:
	odb_accessor(){}
	~odb_accessor(){}
	
public:
	typedef TraitsType										db_traits_type;
	typedef typename db_traits_type::obj_traits_type		obj_traits_type;
	typedef typename obj_traits_type::key_type				key_type;
	
public:
	bool operator==(const odb_accessor<db_traits_type>& rhs) const;
	bool operator!=(const odb_accessor<db_traits_type>& rhs) const;
	template <typename OtherIterator>
	bool operator==(const OtherIterator&){ return false; }
	template <typename OtherIterator>
	bool operator!=(const OtherIterator&){ return true; }
	
	
	
	//! \return a new instance of an output object which allows accessing the data
	//! \note the implementor is not required to decuple the output object from the iterator
	//! , but it must be copy-constructible so that the user can create a separate, independent copy
	template <class OutputObject>
	OutputObject operator*();
	
	//! Implements support for -> semantics
	template <class OutputObject>
	OutputObject operator->();
};

/** \ingroup ODBIter
  * \brief input iterator which can iterate in one direction.
  * The iteration is unordered, which is why it can only be equality compared against the end
  * iterator.
  * Derefencing the iterator yields the object itself (or a reference to it). The key() method
  * provides the current key identifying the object.
  */
template <class TraitsType>
class odb_forward_iterator : public odb_accessor<TraitsType>
{
public:
	typedef odb_accessor<TraitsType> parent_type;
	odb_forward_iterator& operator++();		// prefix
	odb_forward_iterator operator++(int);	// postfix
	
	//! \return key identifying the current position in the iteration
	typename parent_type::key_type key() const;
};
		
GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_ITERATOR_HPP
