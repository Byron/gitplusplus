#ifndef GITMODEL_ODB_OBJ_HPP
#define GITMODEL_ODB_OBJ_HPP
/** \defgroup ODBObject Objects for Object Databases
  * Classes modeling the object concept
  */

#include <gitmodel/config.h>
#include <stddef.h>

GITMODEL_HEADER_BEGIN
GITMODEL_NAMESPACE_BEGIN


/** \class odb_info
  * \brief structure providing information about an object
  * \ingroup ODBObject
  */
template <class TypeID, class SizeType=size_t>
struct odb_info
{
	typedef TypeID type_id;
	typedef SizeType size_type;

	//! \return type of the object which helps to figure out how to interpret its data
	type_id type() const;
	//! \return the size of the object in a format that relates to its storage requirements
	size_type size() const;
};


/** \class odb_object
  * \brief An object as possible member of an object database
  * \ingroup ODBObject
  * Objects are simple structures which only know their data size, 
  * their type and their value.
  */
template <class TypeID, class ValueType, class SizeType=size_t>
struct odb_object : public odb_info<TypeID, SizeType>
{
	typedef ValueType value_type;

	//! \return immutable copy of the object's value
	const value_type value() const;
	//! \return mutable copy of the object's value
	value_type value();
};


GITMODEL_NAMESPACE_END
GITMODEL_HEADER_END

#endif // GITMODEL_ODB_OBJ_HPP
