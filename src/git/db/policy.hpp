#ifndef GIT_POLICY_HPP
#define GIT_POLICY_HPP

#include <git/config.h>
#include <git/db/traits.hpp>
#include <git/obj/multiobj.h>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <memory>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

namespace io = boost::iostreams;

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
				err.stream() << "cannot handle object type: " << (typename TraitsType::char_type)object.type() << std::flush;
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
			case Object::Type::Commit:
			{
				break;
			}
			case Object::Type::Tree:
			{
				break;				
			}
			case Object::Type::Tag:
			{
				const Tag& tag = static_cast<const Tag&>(object);
				ostream << tag;
				break;
			}
			default:
			{
				SerializationError err;
				err.stream() << "invalid object type given for serialization: " << (typename TraitsType::char_type)object.type() << std::flush;
				throw err;
			}
		}// end type switch
	}
	
	template <class ODBObjectType>
	void deserialize(typename TraitsType::output_reference_type out, const ODBObjectType& object)
	{
		std::unique_ptr<typename ODBObjectType::stream_type> pstream(object.new_stream());
		switch(object.type())
		{
			case Object::Type::Blob:
			{
				new (&out.blob) Blob;
				out.blob.data().reserve(object.size());
				io::back_insert_device<Blob::data_type> insert_stream(out.blob.data());
				io::copy(*pstream, insert_stream);
				break;
			}
			case Object::Type::Commit:
			{
				break;
			}
			case Object::Type::Tree:
			{
				break;				
			}
			case Object::Type::Tag:
			{
				new (&out.tag) Tag;
				*pstream >> out.tag;
				break;
			}
			default:
			{
				DeserializationError err;
				err.stream() << "invalid object type given for deserialization: " << (typename TraitsType::char_type)object.type() << std::flush;
				throw err;
			}
		}// end type switch
	}
};



/** \brief final git object traits including the policy itself
  * \note it is put into the policy implementation to adjust the dependencies in order to allow
  * objects to include vital type definitions
  */
struct git_object_traits : public git_object_traits_base
{
	//! serialization policy
	typedef git_object_policy<git_object_policy_traits> policy_type;
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_POLICY_HPP
