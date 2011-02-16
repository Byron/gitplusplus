#ifndef GTL_ODB_ITERATOR_HPP
#define GTL_ODB_ITERATOR_HPP

#include <gtl/config.h>
#include <iterator>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN
	
/** \ingroup ODBIter
  * \brief input iterator allowing read-only access to the underlying object stream.
  * It is considered to be nothing more than a pointer to a specific stream, the iterator
  * should not contain any additional information.
  */
template <class ObjectTraits>
class odb_input_iterator : public std::iterator<typename std::input_iterator_tag, ObjectTraits>
{
protected:
	odb_input_iterator(){}
	~odb_input_iterator(){}
	
public:
	typedef ObjectTraits traits_type;
	typedef typename traits_type::key_type key_type;
	
public:
	bool operator==(const odb_input_iterator<ObjectTraits>& rhs) const;
	bool operator!=(const odb_input_iterator<ObjectTraits>& rhs) const;
	template <typename OtherIterator>
	bool operator==(const OtherIterator&){ return false; }
	template <typename OtherIterator>
	bool operator!=(const OtherIterator&){ return true; }
	
	
	
	//! \return a new instance of an output object which allows accessing the data
	//! \note the implementor is supposed to decuple the output object as much as possible from 
	//! the iteration, it must remain valid even if the iterator advances.
	//! \note -> semantics not supported intentionally, as querying the object can be expensive,
	//! hence the implementor gets the chance to implement the size() and type() methods
	//! directly inside of the iterator reusing its iteration state.
	template <class OutputObject>
	OutputObject operator*();
	
	//! \return size of the data store in the stream
	typename traits_type::size_type size() const;
	
	//! \return type id identifying the type of object contained in the stream
	typename traits_type::object_type type() const;
	
};

/** \ingroup ODBIter
  * \brief input iterator which can iterate in one direction.
  * The iteration is unordered, which is why it can only be equality compared against the end
  * iterator.
  * Dereferencing the iterator yields a pair whose first item in the key, the second is the actual
  * item in the iteration.
  */
template <class ObjectTraits>
class odb_forward_iterator : public odb_input_iterator<ObjectTraits>
{
public:
	typedef odb_input_iterator<ObjectTraits> parent_type;
	odb_forward_iterator& operator++();		// prefix
	odb_forward_iterator operator++(int);	// postfix
	
	//! \return key identifying the current position in the iteration
	typename parent_type::key_type key() const;
};
		
GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_ITERATOR_HPP
