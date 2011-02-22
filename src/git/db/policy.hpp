#ifndef GIT_POLICY_HPP
#define GIT_POLICY_HPP

#include <git/config.h>
#include <git/db/traits.hpp>
#include <git/obj/multiobj.h>

#include <memory>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

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
	compute_size(const typename TraitsType::input_reference_type object, std::ostream* stream=nullptr)
	{
		switch(object.type()) 
		{
			case Object::Type::Blob: { return static_cast<const Blob&>(object).data().size(); break; }
			case Object::Type::Tag: { return static_cast<const Tag&>(object).size(); }
			case Object::Type::Commit: { return static_cast<const Commit&>(object).size(); }
			case Object::Type::Tree: { return static_cast<const Tree&>(object).size(); }
			default:
			{
				ObjectError err;
				err.stream() << "cannot handle object type in compute_size(): " << (typename TraitsType::char_type)object.type() << std::flush;
				throw err;
			}
		}// end switch 
	}
	
	template <class StreamType>
	void serialize(const typename TraitsType::input_reference_type object, StreamType& ostream)
	{
		switch(object.type())
		{
			case Object::Type::Blob: { ostream << static_cast<const Blob&>(object); break; }
			case Object::Type::Commit: { ostream << static_cast<const Commit&>(object); break; }
			case Object::Type::Tree: { ostream << static_cast<const Tree&>(object); break; }
			case Object::Type::Tag: { ostream << static_cast<const Tag&>(object); break; }
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
				*pstream >> out.blob; 
				break; 
			}
			case Object::Type::Commit: { new (&out.commit) Commit; *pstream >> out.commit; break; }
			case Object::Type::Tree: { new (&out.tree) Tree; *pstream >> out.tree; break; }
		case Object::Type::Tag: { new (&out.tag) Tag; *pstream >> out.tag; break; }
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
