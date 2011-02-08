#ifndef GIT_OBJ_BLOB_H
#define GIT_OBJ_BLOB_H

#include <git/obj/object.hpp>
#include <iostream>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

/** \brief A blob represents a piece of data which is accessed through a stream
  * The stream is owned by the blob and will be deallocated on destruction.
  */
class Blob : public Object
{
private:
	std::istream* m_stream;
	
public:
	//! Initialize the blob with the given stream. It will take ownership and deallocate it
	Blob(std::istream* stream = 0);
	~Blob();
	
public:
	//! \return borrowed stream containing the blobs raw character data. Please note that 
	//! you have to take care of rewinding the stream once you are done with it in 
	//! case you want to re-read the data of the same instance
	std::istream* stream(){
		return m_stream;
	}
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_OBJ_BLOB_H
