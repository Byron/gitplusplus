#ifndef GIT_OBJ_BLOB_H
#define GIT_OBJ_BLOB_H

#include <git/config.h>
#include <git/obj/object.hpp>
#include <string>
#include <iostream>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

/** \brief A blob represents a piece of data which is accessed through a data pointer.
  * The data is owned by the blob and will be destroyed with it.
  */
class Blob : public Object
{
private:
	typedef std::basic_string<uchar> ustring;
	ustring m_data;
	
public:
	//! default constructor
	Blob();
	//! default destructor
	~Blob(){};
	
public:
	//! \return modifyable data containing the blobs raw character data.
	//! \note this method should be used to fill or modify the blobs data as required.
	ustring& data(){
		return m_data;
	}
	
	//! \return constant data
	const ustring& data() const {
		return m_data;
	}
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_OBJ_BLOB_H
