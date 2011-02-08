#ifndef GIT_ODB_H
#define GIT_ODB_H

#include <git/config.h>
#include <git/db/sha1.h>
#include <gtl/db/odb_mem.hpp>
#include <git/obj/multiobj.h>
#include <sstream>


GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

/** \brief Configures output streams used in memory databases
  */
struct git_output_object_traits
{
	//! Type allowing to classify the stored object
	typedef Object::Type object_type;
	//! Type used to return values by reference
	typedef MultiObject& output_reference_type;
	//! Type used to return values by reference
	typedef Object* input_reference_type;
	
	
	//! Read data from streams
	typedef std::stringstream istream_type;
	//! Put data directly into memory
	typedef std::stringstream ostream_type;
	//! Put data directly into memory
	typedef std::stringstream iostream_type;
	
	//! 
};


/** \ingroup ODB
  * \brief Database storing git objects in memory only
  * 
  * Use this specialization to quickly cache objects in memory to later dump
  * them to disk at once.
  */
class MemoryODB : public gtl::odb_mem<SHA1, git_output_object_traits>
{
public:
};


GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_ODB_H
