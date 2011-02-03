#ifndef GIT_MODEL_ODB_ITERATOR_HPP
#define GIT_MODEL_ODB_ITERATOR_HPP
/** \defgroup ODBIter Object Database Iterators
  * Iterators to access the object database
  */

#include <git/model/modelconfig.h>
#include <iterator>

GIT_HEADER_BEGIN
GIT_MODEL_NAMESPACE_BEGIN
	
/** \class odb_input_iterator
  * \ingroup ODBIter
  * \brief Iterator allowing read-only access to the underlying container
  */
template <class Key, class T>
class odb_input_iterator : public std::iterator<typename std::input_iterator_tag, T>
{
	
};
		
GIT_MODEL_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_MODEL_ODB_ITERATOR_HPP
