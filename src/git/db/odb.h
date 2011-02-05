#ifndef GIT_ODB_H
#define GIT_ODB_H

#include <git/config.h>
#include <gtl/db/odb_mem.hpp>
#include <git/obj/object.hpp>

#include <string>
#include <boost/iostreams/device/back_inserter.hpp>

namespace io = boost::iostreams;

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

/** \brief Configures output streams used in memory databases
  */
class git_memory_output_object_traits
{
	//! Type allowing to classify the stored object
	typedef Object::Type object_type;
	//! Type returned from the deserialization process
	typedef Object* return_type;
	//! Type used to return values by reference
	typedef void*& output_reference_type;
	//! Put data directly into memory
	typedef io::stream<io::stream_buffer<io::back_insert_device<std::string> > > stream_type;
};


/** \ingroup ODB
  * \brief Database storing git objects in memory only
  * 
  * Use this specialization to quickly cache objects in memory to later dump
  * them to disk at once.
  */
class MemoryODB : public gtl::odb_mem<int, int>
{
public:
};


GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_ODB_H
