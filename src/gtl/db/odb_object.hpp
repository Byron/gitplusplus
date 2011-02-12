#ifndef GTL_ODB_STREAM_HPP
#define GTL_ODB_STREAM_HPP

#include <gtl/config.h>
#include <stdint.h>
#include <exception>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/null.hpp>
#include <type_traits>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace io = boost::iostreams;


/** Basic traits type to further specify properties of objects stored in the database
  * Types using odb facilities need to fully specialize this trait for their respective object
  * database type
  * For brevity, the traits contain a small set of functions to handle serialization features.
  * \ingroup ODBObject
  */
struct odb_object_traits
{	
	//! Type allowing to classify the stored object
	typedef int object_type;
	//! Type of the size of an object. This represents the maximum size in bytes an object may have
	typedef uint64_t size_type;
	//! Type used to return values by reference
	typedef void*& output_reference_type;
	//! Type used when objects are made available to a function by references
	typedef void* input_reference_type;
	
	//! hash_generator interface compatible type which is used to generate keys from the contents of streams
	typedef bool hash_generator_type;
	//! type of keys used within the object databases to identify objects
	typedef int key_type;
	 
	//! @{ \name Serialization Policy
	
	//! Serialize the given object into the given stream.
	//! \param object object to be serialized
	//! \param ostream stream to write serialzed object to.
	//! \tparam Stream type of writable stream
	template <class Stream>
	static void serialize(input_reference_type object, Stream& ostream) throw(std::exception){}
	
	//! deserialize the data contained in istream to recreate the object it represents
	//! \param out variable to keep the deserialized object
	//! \param istream stream to read the serialized object data from
	//! \tparam Stream type of readable input stream
	template <class Stream>
	static void deserialize(output_reference_type out, Stream& istream) throw(std::exception) {}
	
	//! @} 
};


/** \ingroup ODBObject
  * Element living in an object database. It provides access to the objects properties
  * being its type, its uncompressed size in bytes, and a stream to access the data.
  * \tparam ObjectTraits traits struct compatible to odb_traits_type
  * \tparam Stream seekable and readable stream which allows access to the data. It is dependent 
  *	on the underlying database implementation.
  */
template <class ObjectTraits, class Stream>
struct odb_basic_object
{
	typedef ObjectTraits traits_type;
	typedef Stream stream_type;
	
	//! \return type of the object which helps to figure out how to interpret its data
	typename traits_type::object_type type() const;
	
	//! \return the size of the uncompressed object stream in a format that relates to its storage 
	//! requirements, by default in bytes.
	//! \note as the object may be stored compressed within its stream, the amount of data in the stream
	//! may differ.
	typename traits_type::size_type size() const {
		return 0;
	}
	
	//! \return data stream which contains the serialized object. Must be readable
	stream_type stream() const;
	
};

/** \ingroup ODBObject
  * \brief type used as input to object databases. It optionally provides access to 
  * a pointer to the key type, which if not 0, provides key-information of the object at hand.
  * If the key is 0, one will be generated using the key generator of the object traits
  */
template <class ObjectTraits, class Stream>
struct odb_input_object : public odb_basic_object<ObjectTraits, Stream>
{
	typedef typename ObjectTraits::key_type key_type;
	
	/** \return pointer to key designated to the object, or 0 if a key does not yet exists
	  */
	const typename std::add_pointer<const key_type>::type key_pointer() const {
		return 0;
	}
};


/** \brief A handle to a stream of object information for reading
  * \ingroup ODBObject
  * Streams are simple structures which only know their data size, 
  * their type and their serialized data stream. It provides methods 
  * to deserialize the stream into an object.
  * An ostream is considered the output of an object database
  */
template <class ObjectTraits, class Stream>
struct odb_output_object : public odb_basic_object<ObjectTraits, Stream>
{
	typedef ObjectTraits traits_type;
	typedef Stream stream_type;
	
	//! construct an object from the deserialized stream and store it in the output reference
	//! \note uses exceptions to indicate failure (i.e. out of memory, corrupt stream)
	void deserialize(typename traits_type::output_reference_type out) const throw(std::exception)
	{
		traits_type::deserialize(out, this->stream());
	}
};


/** \brief adapter to allow output objects and a separate key to be used as input objects.
  * \ingroup ODBObject
  * It is meant to be as lightweight as possible, and only stores references to the respective values
  * \note we only derive for the purpose of inheriting member documentation
  */
template <class OutputObject>
class  odb_output_object_adapter : public odb_input_object<	typename OutputObject::traits_type,
															typename OutputObject::stream_type>
{
public:
	typedef OutputObject output_object_type;
	typedef typename output_object_type::traits_type traits_type;
	typedef typename output_object_type::traits_type::key_type key_type;
	
private:
	typename std::add_lvalue_reference<const output_object_type>::type m_obj;
	typename std::add_lvalue_reference<const key_type>::type m_key;
	
public:
	odb_output_object_adapter(typename std::add_rvalue_reference<const output_object_type>::type obj,
							  typename std::add_rvalue_reference<const key_type>::type key)
		: m_obj(obj)
		, m_key(key)
	{}
	
	const typename std::add_pointer<const key_type>::type key_pointer() const {
		return &m_key;
	}
	
	typename traits_type::object_type type() const {
		return m_obj.type();
	}
	
	typename traits_type::size_type size() const {
		return m_obj.size();
	}
};
		
GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_STREAM_HPP
