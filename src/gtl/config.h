#ifndef GTL_CONFIG_H
#define GTL_CONFIG_H

#include <stdint.h>

/** \defgroup ODBUtil Object Database Utilities
  * Misc. Utilities for use in and around object database
  */

/** \defgroup ODB Object Databases
  * Classes modeling the object database concept
  */

/** \defgroup ODBIter Object Database Iterators
  * Classes modeling the concept of an object database iterator
  */

/** \defgroup ODBObject Object Database Items
  * Items as part of the object database which allow streamed access to an object
  */

/** \defgroup ODBAlloc Object Allocators
  * Classes implementing different allocators for objects
  */


#define GTL_HEADER_BEGIN
#define GTL_HEADER_END

/** \namespace gtl
  * \brief The git template library contains all models which implement git related concepts
  */
#define GTL_NAMESPACE_BEGIN namespace gtl {
#define GTL_NAMESPACE_END }

typedef uint32_t uint32;
typedef uint8_t uchar;

#endif // GTL_CONFIG_H
