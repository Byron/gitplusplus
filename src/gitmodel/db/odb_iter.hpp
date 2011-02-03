#ifndef GITMODEL_ODB_ITERATOR_HPP
#define GITMODEL_ODB_ITERATOR_HPP
/** \defgroup ODBIter Object Database Iterators
  * Classes modeling the concept of an object database iterator
  */

#include <gitmodel/config.h>
#include <iterator>

GITMODEL_HEADER_BEGIN
GITMODEL_NAMESPACE_BEGIN
	
/** \class odb_input_iterator
  * \ingroup ODBIter
  * \brief Iterator allowing read-only access to the underlying container
  */
template <class Key, class T>
class odb_input_iterator : public std::iterator<typename std::input_iterator_tag, T>
{
	
};
		
GITMODEL_NAMESPACE_END
GITMODEL_HEADER_END

#endif // GITMODEL_ODB_ITERATOR_HPP
