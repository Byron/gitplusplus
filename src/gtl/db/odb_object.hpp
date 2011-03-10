#ifndef GTL_ODB_STREAM_HPP
#define GTL_ODB_STREAM_HPP

#include <gtl/config.h>
#include <gtl/db/hash_generator_filter.hpp>

#include <boost/filesystem.hpp>
#include <boost/iostreams/filter/zlib.hpp>

#include <stdint.h>
#include <exception>
#include <iostream>
#include <type_traits>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

/** \brief base error for all database objects related actions
  * \ingroup ODBException
  */
class odb_object_error : public std::exception {};

/*! \brief Thrown for errors during deserialization
 * \ingroup ODBException
 */
class odb_deserialization_error :virtual public odb_object_error {};

/*! \brief Thrown for errors during serialization
 * \ingroup ODBException
 */
class odb_serialization_error : virtual public odb_object_error {};


/** \brief traits for object policies.
  */
struct odb_object_policy_traits
{
	//! Type allowing to classify the stored object
	typedef int object_type;
	//! value which equals null or nullptr if it was a pointer, in terms of an object type
	static const object_type null_object_type = 0;
	//! Type used to return values by reference
	typedef void*& output_reference_type;
	//! Type used when objects are made available to a function by references
	typedef void* input_reference_type;
	//! Type of the size of an object. This represents the maximum size in bytes an object may have
	typedef uint64_t size_type;
	//! Character type to be used within streams and data storage defined in object databases
	typedef char char_type;
};

/** \brief policy defining functions to be used in certain situations
  * Its template parameters should be used to partially specialize the 
  * methods for the respective serialization and deserializion types used.
  * The StreamType and ObjectType remain templates, they obey their designated interfaces.
  * \tparam TraitsType policy traits for configuration
  * \ingroup ODBPolicy
  */
template <class TraitsType>
struct odb_object_policy
{
	//! @{ \name Serialization Policy
	
	//! \return the type of the given object
	//! \throw odb_object_error
	typename TraitsType::object_type type(const typename TraitsType::input_reference_type object);
	
	//! Compute the uncompressed serialized size of the given object as quickly as possible.
	//! \param object to compute the size for
	//! \param pointer stream output stream which can be used to serialize the object into in case the 
	//! only feasible way to determine the object's size is the actual serialization
	//! Its encouraged though to compute the size whenever possible, especially if the object is large.
	//! The stream is only provided to improve efficiency of implementation which have to serialize the object
	//! to obtain the size. After all, it depends on the database implementation and storage format whether 
	//! the serialized object size must be known in advance.
	//! If the pointer is 0, which is valid, the implementation has to solve the issue itself, for example using 
	//! an own temporary stream
	//! \throw odb_object_error
	typename TraitsType::size_type compute_size(const typename TraitsType::input_reference_type object, std::ostream* stream = nullptr);
	
	//! Serialize the given object into the given stream.
	//! \param object object to be serialized
	//! \param outObject input object to fill with data, that is type, size and the actual serialized data stream.
	//! \tparam StreamType ostream compatible type
	//! \throw odb_serialization_error
	template <class StreamType>
	void serialize(const typename TraitsType::input_reference_type object, StreamType& stream);
	
	//! deserialize the data contained in object to recreate the object it represents
	//! \param output variable to keep the deserialized object
	//! \param object database object containing all information, like type, size, stream
	//! \tparam OutputObjectType obd_output_object compatible type
	//! \throw odb_deserialization_error
	template <class OutputObjectType>
	static void deserialize(typename TraitsType::output_reference_type out, const OutputObjectType& object);
	
	//! @}
};



/** Basic traits type to further specify properties of objects stored in the database
  * Types using odb facilities need to fully specialize this trait for their respective object
  * database type
  * For brevity, the traits contain a small set of functions to handle serialization features.
  * \ingroup ODBObject
  */
struct odb_object_traits : public odb_object_policy_traits
{	
	//! hash_generator interface compatible type which is used to generate keys from the contents of streams
	typedef bool hash_generator_type;
	//! type of keys used within the object databases to identify objects
	typedef int key_type;
	//! Policy struct providing additional functionality
	typedef odb_object_policy<odb_object_policy_traits> policy_type;
};


/** Basic traits suitable for a all databases which use files to store objects in one way or another
  * \note by default, it uses the zlib compression library which usually involves
  * external dependencies which must be met for the program to run.
  */
template <class KeyType, class HashGeneratorType>
struct odb_file_traits
{
	typedef KeyType key_type;
	typedef HashGeneratorType hash_generator_type;
	
	//! type suitable to be used for compression within the 
	//! boost iostreams filtering framework.
	//! \todo there should be a way to define the char_type, but its a bit occluded
	//! \note this type may not be changed as it is the default git compressor
	typedef boost::iostreams::zlib_compressor							compression_filter_type;
	
	//! type compatible to the boost filtering framework to decompress what 
	//! was previously compressed.
	//! \todo there should be a way to define the char_type, but its a bit occluded
	//! \note this type may not be changed as it is the default git decompressor
	typedef boost::iostreams::zlib_decompressor							decompression_filter_type;
	
	//! type generating a filter based on the hash_filter template, using the predefined hash generator
	typedef generator_filter<key_type, hash_generator_type>				hash_filter_type;
	
	//! type to be used as path. The interface must comply to the boost filesystem path
	typedef boost::filesystem::path										path_type;

};


/** \ingroup ODBObject
  * Element living in an object database. It provides access to the objects properties
  * being its type, its uncompressed size in bytes, and a stream to access the data.
  * \tparam ObjectTraits traits struct compatible to odb_traits_type
  * \tparam Stream seekable and readable stream which allows access to the data. It is dependent 
  *	on the underlying database implementation.
  */
template <class ObjectTraits>
struct odb_basic_object
{
	typedef ObjectTraits obj_traits_type;
	
	//! \return type of the object which helps to figure out how to interpret its data
	typename obj_traits_type::object_type type() const;
	
	//! \return the size of the uncompressed object stream in a format that relates to its storage 
	//! requirements, by default in bytes.
	//! \note as the object may be stored compressed within its stream, the amount of data in the stream
	//! may differ.
	typename obj_traits_type::size_type size() const {
		return 0;
	}
};

/** \ingroup ODBObject
  * \brief type used as input to object databases. It optionally provides access to 
  * a pointer to the key type, which if not 0, provides key-information of the object at hand.
  * If the key is 0, one will be generated using the key generator of the object traits
  * The key, if provided, is never owned by the input object, and must be kept alive during the insertion.
  * 
  * The input object is to be used as non-constant object as its stream will be read in the course
  * of the process of inserting it into the database - it must be rewound by the caller if desired.
  * 
  * The whole purpose of the input object is to provide a simple way to add any object to the database
  * without actually requiring an instance of such an object. In the usual case though, it might be easiest
  * to use the insert() method of the database which takes an input_reference_type
  */
template <class ObjectTraits, class StreamType>
struct odb_input_object : public odb_basic_object<ObjectTraits>
{
	typedef ObjectTraits obj_traits_type;
	typedef typename ObjectTraits::key_type key_type;
	typedef StreamType stream_type;
	
	//! Initialize the instance
	//! \fn odb_input_object(typename obj_traits_type::object_type, typename obj_traits_type::size_type, stream_type& stream, const key_type* key_pointer = 0)
	//! \param object_type type of the object to store
	//! \param size_type the size of the serialized and uncompressed object, may be a null value
	//! if the instance is initialized later
	//! \param stream own stream to use as object data
	//! \param key_pointer if not 0, the key that was previously generated by hashing the object stream.
	//! it should be provided if objects are copied from one database from another to prevent duplicate
	//! hashing of streams. If it is not provided, a key will be generated during the insertion.
	
	//! Set this instance to use the given size
	void set_size(typename obj_traits_type::size_type size);
	
	//! Set this instance to the given object type
	void set_type(typename obj_traits_type::object_type type);
	
	//! \return reference to the contained stream
	stream_type& stream();
	
	//! \return pointer to key designated to the object, or 0 if a key does not yet exists
	const typename std::add_pointer<const key_type>::type key_pointer() const {
		return nullptr;
	}
};


/** \brief an implementation of an input objects using a reference to the actual stream.
  * This should be suitable for adding objects to the database as streams cannot be copy-constructed.
  * \ingroup ODBObject
  */
template <class ObjectTraits, class StreamBaseType = std::basic_istream<typename ObjectTraits::char_type> >
class odb_ref_input_object : public odb_input_object<ObjectTraits, StreamBaseType>
{
public:
	typedef ObjectTraits								obj_traits_type;
	typedef typename obj_traits_type::object_type		object_type;
	typedef typename obj_traits_type::size_type			size_type;
	typedef typename obj_traits_type::key_type			key_type;
	typedef StreamBaseType								stream_type;
	
protected:
	object_type			m_type;
	size_type			m_size;
	stream_type&		m_stream;
	const key_type*		m_key;
	
public:
	odb_ref_input_object(object_type type, size_type size,
						 stream_type& stream,
						 const key_type* key_pointer = nullptr)
		: m_type(type)
		, m_size(size)
		, m_stream(stream)
		, m_key(key_pointer)
	{}
	
	
	object_type type() const noexcept {
		return m_type;
	}
	
	void set_type(object_type type) noexcept {
		m_type = type;
	}
	
	size_type size() const noexcept {
		return m_size;
	}
	
	void set_size(size_type size) noexcept {
		m_size = size;
	}
	
	stream_type& stream() noexcept {
		return m_stream;
	}
	
	const key_type* key_pointer() const noexcept {
		return m_key;
	}
};


/** \brief A handle to a stream of object information for reading
  * \ingroup ODBObject
  * Streams are simple structures which only know their data size, 
  * their type and their serialized data stream. It provides methods 
  * to deserialize the stream into an object.
  * An ostream is considered the output of an object database
  * \tparam ObjectTraits traits specifying details about the stored objects, see odb_object_traits
  * \tparam StreamType type of stream to be used for outputting the data	
  */
template <class ObjectTraits, class StreamType>
struct odb_output_object : public odb_basic_object<ObjectTraits>
{
	typedef ObjectTraits					obj_traits_type;
	
	//! the type of streams which allow reading the serialized object data
	typedef StreamType						stream_type;
	
	//! create data stream which allows access to the serialized object data
	//! As streams are generally non-copyable, but yet a unique stream needs to be
	//! returned each time of query, it must be created at the given memory location.
	//! \param out_stream memory able to hold a stream_type object. It must be allocated by the 
	//! caller. Its critical that the constructor was not yet called on out_stream, hence it must
	//! be an uninitialized memory location. The caller is responsible for disposing the memory after use
	//! and assuring the destructor gets called. The new stream will be created using placement new at the 
	//! given location.
	//! \note it is easiest to use a stack_heap<stream_type> instance to deal with the memory, as well as with 
	//! the destructor.
	void stream(stream_type* out_stream) const;
	
	//! create a data stream on the heap and return it as a pointer. The caller is responsible
	//! for disposing the stream using delete or similar means (like std::unique_ptr)
	//! \return heap-allocated stream
	//! \note use this method if streams need to life longer than the current stack frame
	stream_type* new_stream() const;
	
	//! construct an object from the deserialized stream and store it in the output reference
	//! \note uses exceptions to indicate failure (i.e. out of memory, corrupt stream)
	//! \throw odb_deserialization_error
	void deserialize(typename obj_traits_type::output_reference_type out) const;
};


/** \brief adapter to allow output objects and a separate key to be used as input objects.
  * \ingroup ODBObject
  * It is meant to be as lightweight as possible, and only stores references to the respective values
  * \note we only derive for the purpose of inheriting member documentation
  */
template <class OutputObject>
class  odb_output_object_adapter : public odb_input_object<	typename OutputObject::obj_traits_type,
															typename OutputObject::stream_type>
{
public:
	typedef OutputObject output_object_type;
	typedef typename std::add_lvalue_reference<const output_object_type>::type const_output_object_ref_type;
	typedef typename output_object_type::stream_type stream_type;
	typedef typename output_object_type::obj_traits_type obj_traits_type;
	typedef typename output_object_type::obj_traits_type::key_type key_type;
	
private:
	const_output_object_ref_type m_obj;
	const key_type& m_key;
	stream_type* m_stream;
	
public:
	odb_output_object_adapter(const_output_object_ref_type obj,
							  const key_type& key)
		: m_obj(obj)
		, m_key(key)
		, m_stream(nullptr)
	{}
	
	~odb_output_object_adapter() {
		if (m_stream) {
			delete m_stream;
			m_stream = nullptr;
		}
	}
	
	const key_type* key_pointer() const {
		return &m_key;
	}
	
	typename obj_traits_type::object_type type() const {
		return m_obj.type();
	}
	
	typename obj_traits_type::size_type size() const {
		return m_obj.size();
	}
	
	 stream_type& stream() {
		if (m_stream == nullptr) {
			 m_stream = m_obj.new_stream();
		}
		return *m_stream;
	}
};
		
GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_STREAM_HPP
