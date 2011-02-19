#ifndef GIT_OBJ_BLOB_H
#define GIT_OBJ_BLOB_H

#include <git/config.h>
#include <git/obj/object.hpp>
#include <git/db/traits.hpp>
#include <vector>
#include <iostream>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

/** \brief A blob represents a piece of data which is accessed through a data pointer.
  * The data is owned by the blob and will be destroyed with it.
  */
class Blob : public Object
{
public:
	typedef typename git_object_policy_traits::char_type	char_type;
	typedef std::vector<char_type>							data_type;	//!< type we use to store our data
	
private:
	data_type m_data;
	
public:
	//! default constructor
	Blob();
	Blob(const Blob&) = default;
	Blob(Blob&&) = default;
	
	//! default destructor
	~Blob(){}
	
public:
	//! \return modifyable data containing the blobs raw character data.
	//! \note this method should be used to fill or modify the blobs data as required.
	data_type& data() {
		return m_data;
	}
	
	//! \return constant data
	const data_type& data() const {
		return m_data;
	}
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_OBJ_BLOB_H
