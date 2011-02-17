#ifndef GIT_ODB_H
#define GIT_ODB_H

#include <git/config.h>
#include <git/db/sha1.h>
#include <git/db/sha1_gen.h>
#include <gtl/db/odb_mem.hpp>
#include <git/obj/multiobj.h>

#include <iostream>

// for serialization only
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <memory>


GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

// forward declaration
struct git_object_policy;

/** \brief Configures output streams used in memory databases
  */
struct git_object_traits
{
	//! Type allowing to classify the stored object
	typedef Object::Type object_type;
	//! max size of serialized objects
	typedef uint64_t size_type;
	//! Type used to return values by reference
	typedef MultiObject& output_reference_type;
	//! Type used to return values by reference
	typedef Object& input_reference_type;

	//! Hash generator to produce keys.
	typedef SHA1Generator hash_generator_type;	
	//! Using SHA1 as key
	typedef SHA1 key_type;
	
	//! use unsigned bytes as general storage
	typedef uchar char_type;
	
	template <class ObjectTraits, class StreamType>
	struct serialization_policy
	{
		//! Serialize the given object into the given stream.
		//! \param object object to be serialized
		//! \param ostream stream to write serialzed object to.
		//! \tparam StreamType typoe of stream
		//! \throw odb_serialization_error
		void serialize(typename ObjectTraits::input_reference_type object, StreamType& ostream);
	};
	
	template <class ObjectTraits, class ObjectType>
	struct deserialization_policy
	{
		//! deserialize the data contained in object to recreate the object it represents
		//! \param output variable to keep the deserialized object
		//! \param object database object containing all information, like type, size, stream
		//! \tparam ObjectType type of database object
		//! \throw odb_deserialization_error
		static void deserialize(typename ObjectTraits::output_reference_type out, const ObjectType& object);
	};
	
	// partial specialization declaration
	template <class StreamType>
	class 
};




/** \ingroup ODB
  * \brief Database storing git objects in memory only
  * 
  * Use this specialization to quickly cache objects in memory to later dump
  * them to disk at once.
  */
class MemoryODB : public gtl::odb_mem<git_object_traits>
{
public:
	
};


GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_ODB_H
