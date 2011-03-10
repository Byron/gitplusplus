#ifndef GTL_ODB_ITERATOR_HPP
#define GTL_ODB_ITERATOR_HPP

#include <gtl/config.h>
#include <boost/iterator/iterator_facade.hpp>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN
	
/** \ingroup ODBIter
  * \brief pointer allowing read-only access to the underlying object.
  * It is considered to be nothing more than a pointer to a specific object, the iterator
  * should not contain any additional information, it cannot be moved.
  */
template <class TraitsType>
class odb_accessor
{
protected:
	odb_accessor(){}
	~odb_accessor(){}
	
public:
	typedef TraitsType										db_traits_type;
	typedef typename db_traits_type::obj_traits_type		obj_traits_type;
	typedef typename obj_traits_type::key_type				key_type;
	
public:
	//! \return key identifying the current position in the iteration
	key_type key() const;
};

/** \ingroup ODBIter
  * \brief input iterator which can iterate in one direction.
  * The iteration is unordered, which is why it can only be equality compared against the end
  * iterator.
  * Derefencing the iterator yields the object itself (or a reference to it). The key() method
  * provides the current key identifying the object.
  */
template <class TraitsType>
struct odb_forward_iterator :	public boost::iterator_facade<	odb_forward_iterator<TraitsType>,
																int, /*Output Object Type*/
																boost::forward_traversal_tag>,
								public odb_accessor<TraitsType>
{
};
		
GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_ITERATOR_HPP
