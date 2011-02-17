#include <git/db/odb.h>
#include <git/config.h>

GIT_NAMESPACE_BEGIN

template <class StreamType>
void git_object_traits::serialization_policy<StreamType>::serialize(typename git_object_traits::input_reference_type object, StreamType& ostream)
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

template <class ObjectType>
void git_object_traits::deserialization_policy<ObjectType>::deserialize(typename git_object_traits::output_reference_type out, const ObjectType& object)
{
	switch(object.type())
	{
		case Object::Type::Blob:
		{
			new (&out.blob) Blob;
			out.blob.data().reserve(object.size());
			std::unique_ptr<typename ObjectType::stream_type> pstream(object.new_stream());
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

GIT_NAMESPACE_END
