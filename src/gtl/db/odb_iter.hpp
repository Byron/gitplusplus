#ifndef GTL_ODB_ITERATOR_HPP
#define GTL_ODB_ITERATOR_HPP
/** \defgroup ODBIter Object Database Iterators
  * Classes modeling the concept of an object database iterator
  */

#include <gtl/config.h>
#include <iterator>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN
	
/** \ingroup ODBIter
  * \brief input iterator allowing read-only access to the underlying value.
  */
template <class Key, class T>
class odb_input_iterator : public std::iterator<typename std::input_iterator_tag, T>
{
protected:
	odb_input_iterator(){}
	~odb_input_iterator(){}
	
public:
	bool operator==(const odb_input_iterator<Key, T>& rhs) const;
	bool operator!=(const odb_input_iterator<Key, T>& rhs) const;
	template <typename OtherIterator>
	bool operator==(const OtherIterator&){ return false; }
	template <typename OtherIterator>
	bool operator!=(const OtherIterator&){ return true; }
	
	T& operator->();
	const T& operator->() const;
	T& operator*();
	const T& operator*() const;
	
};

/** \ingroup ODBIter
  * \brief input iterator which can iterate in one direction.
  * The iteration is unordered, which is why it can only be equality compared against the end
  * iterator.
  * Dereferencing the iterator yields a pair whose first item in the key, the second is the actual
  * item in the iteration.
  */
template <class Key, class T>
class odb_forward_iterator : public odb_input_iterator<Key, T>
{
	typedef std::pair<Key, T> value_type;
	
	odb_forward_iterator& operator++();		// prefix
	odb_forward_iterator operator++(int);	// postfix
	
	const value_type& operator->() const;
	const value_type& operator*() const;
};
		
GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_ITERATOR_HPP
