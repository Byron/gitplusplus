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
	
	//! serialization policy
	typedef git_object_policy policy_type;
};

//! \brief define standard git object serialization and deserialization
//! \ingroup ODBPolicy
//! \note inheriting base just for docs
struct git_object_policy : public gtl::odb_object_policy<git_object_traits::input_reference_type, 
														 git_object_traits::output_reference_type>
{
	template <class StreamType>
	void serialize(typename git_object_traits::input_reference_type object, StreamType& ostream)
	{
		switch(object.type()) 
		{
			case Object::Type::Blob:
			{
				break;
			}
			default:
			{
				SerializationError err;
				err.stream() << "invalid object type given for serialization: " << (uchar)object.type() << std::flush;
				throw err;
			}
		}
	}
	
	template <class ODBObjectType>
	void deserialize(typename git_object_traits::output_reference_type out, const ODBObjectType& object)
	{
		switch(object.type())
		{
			case Object::Type::Blob:
			{
				new (&out.blob) Blob;
				out.blob.data().reserve(object.size());
				std::unique_ptr<typename ODBObjectType::stream_type> pstream(object.new_stream());
				boost::iostreams::back_insert_device<Blob::data_type> insert_stream(out.blob.data());
				boost::iostreams::copy(*pstream, insert_stream);
				break;
			}
			
			default:
			{
				DeserializationError err;
				err.stream() << "invalid object type given for deserialization: " << (uchar)object.type() << std::flush;
				throw err;
			}
		}	
	}
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
