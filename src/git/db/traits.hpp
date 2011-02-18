#ifndef GIT_TRAITS_H
#define GIT_TRAITS_H

#include <git/config.h>
#include <git/db/sha1.h>
#include <git/db/sha1_gen.h>
#include <git/obj/multiobj.h>


// following includes are for serialization only
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <memory>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

// forward declaration
namespace io = boost::iostreams;

struct git_object_policy_traits
{
	//! Type allowing to classify the stored object
	typedef Object::Type object_type;
	//! max size of serialized objects
	typedef uint64_t size_type;
	//! use unsigned bytes as general storage
	typedef default_char_type char_type;
	//! Type used to return values by reference
	typedef MultiObject& output_reference_type;
	//! Type used to provide unpacked object values.
	typedef Object& input_reference_type;
};


//! \brief define standard git object serialization and deserialization
//! \ingroup ODBPolicy
//! \note inheriting base just for docs
template <class TraitsType>
struct git_object_policy : public gtl::odb_object_policy<TraitsType>
{
	typename TraitsType::object_type type(const typename TraitsType::input_reference_type object)
	{
		return object.type();
	}
	
	typename TraitsType::size_type 
	compute_size(const typename TraitsType::input_reference_type object, std::ostream* stream=0)
	{
		switch(object.type()) 
		{
			case Object::Type::Blob:
			{
				return static_cast<const Blob&>(object).data().size();
				break;
			}
			
			default:
			{
				ObjectError err;
				err.stream() << "cannot handle object type: " << (default_char_type)object.type() << std::flush;
				throw err;
			}
		}// end switch 
	}
	
	template <class StreamType>
	void serialize(const typename TraitsType::input_reference_type object, StreamType& ostream)
	{
		switch(object.type())
		{
			case Object::Type::Blob:
			{
				// just copy the data into the target stream
				const Blob& blob = static_cast<const Blob&>(object);
				io::basic_array_source<typename Blob::char_type> source_stream(&blob.data()[0], blob.data().size());
				io::copy(source_stream, ostream);
				break;
			}
			default:
			{
				SerializationError err;
				err.stream() << "invalid object type given for serialization: " << (default_char_type)object.type() << std::flush;
				throw err;
			}
		}// end type switch
	}
	
	template <class ODBObjectType>
	void deserialize(typename TraitsType::output_reference_type out, const ODBObjectType& object)
	{
		switch(object.type())
		{
			case Object::Type::Blob:
			{
				new (&out.blob) Blob;
				out.blob.data().reserve(object.size());
				std::unique_ptr<typename ODBObjectType::stream_type> pstream(object.new_stream());
				io::back_insert_device<Blob::data_type> insert_stream(out.blob.data());
				io::copy(*pstream, insert_stream);
				break;
			}
			
			default:
			{
				DeserializationError err;
				err.stream() << "invalid object type given for deserialization: " << (default_char_type)object.type() << std::flush;
				throw err;
			}
		}// end type switch
	}
};

/** \brief Configures output streams used in memory databases
  */
struct git_object_traits : public git_object_policy_traits
{
	//! Hash generator to produce keys.
	typedef SHA1Generator hash_generator_type;	
	//! Using SHA1 as key
	typedef SHA1 key_type;
	//! serialization policy
	typedef git_object_policy<git_object_policy_traits> policy_type;
};



GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_TRAITS_H
