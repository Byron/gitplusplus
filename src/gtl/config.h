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

/** \defgroup ODBException Object Database Exceptions
  * Classes representing various error states in and around an object database
  */

/** \defgroup ODBPolicy Object Database Policies
  * Structs implementing utility methods to fill in functionality the base template cannot know about
  */


// Its not worth using version 3 just because we would need one function, unique_file
// Note: Now, after the switch to boost 1.45, it would actually possible to do so
// #define BOOST_FILESYSTEM_VERSION 3

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
