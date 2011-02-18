#ifndef GTL_ODB_MEM_HPP
#define GTL_ODB_MEM_HPP

#include <gtl/config.h>
#include <gtl/db/odb.hpp>
#include <gtl/db/odb_object.hpp>
#include <gtl/util.hpp>

#include <boost/type_traits/add_pointer.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>

#include <iostream>
#include <sstream>
#include <type_traits>
#include <vector>
#include <map>
#include <assert.h>
#include <utility>
#include <memory>


GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace io = boost::iostreams;

/*! \brief Thrown for errors during serialization
 * \ingroup ODBException
 */
class odb_mem_serialization_error : public odb_serialization_error,
									public streaming_exception
{};


/** \ingroup ODBObject
  */
template <	class ObjectTraits, 
			class StreamType = io::stream<io::basic_array_source<typename ObjectTraits::char_type> > >
class odb_mem_output_object : public odb_output_object<ObjectTraits, StreamType>
{
public:
	typedef StreamType											stream_type;
	typedef ObjectTraits										traits_type;
	typedef std::vector<typename stream_type::char_type>		data_type;
	
private:
	typename traits_type::object_type						m_type;		//! object type
	typename traits_type::size_type							m_size;		//! uncompressed size
	data_type												m_data;		//! actually stored data
	
	odb_mem_output_object(const odb_mem_output_object&);				//! enforce move constructor
	
public:
	odb_mem_output_object(typename traits_type::object_type type, typename traits_type::size_type size) noexcept
		: m_type(type)
		, m_size(size)
		
	{
	}
	
	//! Move constructor allows us to insert temporary objects into our map with only minimal overhead
	//! especially in case of our data.
	odb_mem_output_object(odb_mem_output_object&& rhs) = default;
			
	
	typename traits_type::object_type type() const noexcept {
		return m_type;
	}
	
	typename traits_type::size_type size() const noexcept {
		return m_size;
	}
	
	//! \return stream wrapper around the data held in this object
	void stream(stream_type* out_stream) const noexcept {
		new (out_stream) stream_type(m_data.data(), m_data.size());
	}

	stream_type* new_stream() const {
		return new stream_type(m_data.data(), m_data.size());
	}
	
	//! \return our actual data for manipulation
	data_type& data() noexcept {
		return m_data;
	}
	
	void deserialize(typename traits_type::output_reference_type out) const
	{
		typename traits_type::policy_type().deserialize(out, *this);
	}
};


/** \brief an implementation of memory input objects using a reference to the actual stream.
  * This should be suitable for adding objects to the database as streams cannot be copy-constructed.
  * \ingroup ODBObject
  */
template <class ObjectTraits, class StreamBaseType = std::basic_istream<typename ObjectTraits::char_type> >
class odb_mem_input_object : public odb_input_object<ObjectTraits, StreamBaseType>
{
public:
	typedef ObjectTraits								traits_type;
	typedef typename traits_type::object_type			object_type;
	typedef typename traits_type::size_type				size_type;
	typedef typename traits_type::key_type				key_type;
	typedef StreamBaseType								stream_type;
	
protected:
	bool				m_owns_stream;
	object_type			m_type;
	size_type			m_size;
	stream_type*		m_pstream;
	const key_type*		m_key;
	
public:
	odb_mem_input_object(object_type type, size_type size,
						 stream_type* stream = nullptr,
						 const key_type* key_pointer = nullptr) noexcept
		: m_owns_stream(false)
		, m_type(type)
		, m_size(size)
		, m_pstream(stream)
		, m_key(key_pointer)
	{}
	
	~odb_mem_input_object() {
		if (m_owns_stream && m_pstream){
			delete m_pstream;
			m_owns_stream = false;
			m_pstream = nullptr;
		}
	}
	
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
	
	//! \throw odb_mem_serialization_error
	stream_type& stream() {
		if (m_pstream == nullptr){
			odb_mem_serialization_error err;
			err.stream() << "Did not have a stream - did you forget to put one into the input object, or to call serialize() ?" << std::flush;
			throw err;
		}
		return *m_pstream;
	}
	
	const key_type* key_pointer() const noexcept {
		return m_key;
	}
	
	//! \throw odb_mem_serialization_error
	void serialize(const typename traits_type::input_reference_type object) {
		assert(m_key == 0);	// it makes no sense to serialize an object although our key is set ...
		if (m_pstream && (m_pstream->tellp() != 0 || m_pstream->tellg() != 0)) {
			odb_mem_serialization_error err;
			err.stream() << "Cannot serialize object if the input object's stream is not empty" << std::flush;
			throw err;
		}
		
		if (!m_pstream){
			create_stream();
			assert(m_pstream);
		}
		
		m_type = typename traits_type::policy_type().type(object);
		typename traits_type::policy_type().serialize(object, m_pstream);
		m_size = m_pstream->tellp();
		m_pstream->seekp(0, std::ios_base::beg);
	}
	
	//! @{ \name Memory DB Specific Interface
	//! Create a new stream if we currently don't own one
	//! \return pointer to possibly newly created stream
	//! \note its safe to call this method multiple times or in any state
	stream_type* create_stream() {
		if (!m_pstream) {
			m_pstream = new stream_type;
			m_owns_stream = true;
		}
		return m_pstream;
	}
	//! @}
};


/** Iterator providing access to a single element of the memory odb.
  * It's value can be queried read-only, and it cannot be used in iterations.
  * \ingroup ODBIter
  */
template <class ObjectTraits>
class mem_accessor : public odb_accessor<ObjectTraits>
{
public:
	typedef ObjectTraits traits_type;
	typedef typename traits_type::key_type key_type;
	typedef typename traits_type::size_type size_type;
	typedef std::map<key_type, odb_mem_output_object<traits_type> > map_type;
	typedef mem_accessor<traits_type> this_type;
	typedef typename map_type::value_type value_type;

protected:
	typename map_type::const_iterator m_iter;
	
public:
	//! initialize the iterator from the underlying mapping type's iterator
	template <class Iterator>
	mem_accessor(const Iterator& it) noexcept : m_iter(it) {}
	
	//! Equality comparison of compatible iterators
	inline bool operator==(const this_type& rhs) const noexcept {
		return m_iter == rhs.m_iter;
	}
	
	//! Inequality comparison
	inline bool operator!=(const this_type& rhs) const noexcept {
		return m_iter != rhs.m_iter;
	}
	
	//! \return constant output object
	inline typename std::add_lvalue_reference<const typename map_type::value_type::second_type>::type 
		operator*() const noexcept {
		return m_iter->second;
	}
	
	//! \return key instance which identifyies our object within this map
	//! \note this method is an optional addition, just because our implementation provides this information
	//! for free.
	inline const typename map_type::value_type::first_type& key() const noexcept {
		return m_iter->first;
	}
	
	//! \return uncompressed size of the object in bytes
	inline size_type size() const noexcept {
		return m_iter->second.size();
	}
	
	//! \return type of object stored in the stream
	inline typename traits_type::object_type type() const noexcept {
		return m_iter->second.type();
	}
};

/** \ingroup ODBIter
  */
template <class ObjectTraits>
class mem_forward_iterator : public mem_accessor<ObjectTraits>
{
public:
	typedef mem_accessor<ObjectTraits> parent_type;

	
	template <class Iterator>
	mem_forward_iterator(const Iterator& it)
		: mem_accessor<ObjectTraits>(it) {}
	
	mem_forward_iterator& operator++() noexcept {
		++(this->m_iter); return *this;
	}
	mem_forward_iterator operator++(int) noexcept {
		mem_forward_iterator cpy(*this); ++(*this); return cpy;
	}
};

/** \brief Class providing access to objects which are cached in memory
  * \ingroup ODB
  * The memory object database acts as an adapter to a map which keeps the actual items.
  * Hence it is nothing more than map with different functionality  and special iterators, which 
  * may additionally generate its own keys as it knows its kind of input.
  */
template <class ObjectTraits>
class odb_mem : public odb_base<ObjectTraits>
{
public:
	typedef ObjectTraits										traits_type;
	typedef odb_base<traits_type>								parent_type;
	typedef typename traits_type::key_type						key_type;
	typedef typename mem_accessor<traits_type>::map_type		map_type;
	typedef mem_accessor<traits_type>							accessor;
	typedef mem_forward_iterator<traits_type>					forward_iterator;
	
	typedef odb_hash_error<key_type>							hash_error_type;
	typedef odb_mem_output_object<traits_type>					output_object_type;
	typedef odb_mem_input_object<traits_type>					input_object_type;
	
private:
	map_type m_objs;
	
public:
	
	
	
	bool has_object(const key_type& k) const noexcept{
		return m_objs.find(k) != m_objs.end();
	}
	
	accessor object(const key_type& k) const {
		typename map_type::const_iterator it(m_objs.find(k));
		if (it == m_objs.end()) {
			throw hash_error_type(k);
		}
		return accessor(it);
	}
	
	forward_iterator begin() const noexcept {
		return forward_iterator(m_objs.begin());
	}
	forward_iterator end() const noexcept {
		return forward_iterator(m_objs.end());
	}
	size_t count() const noexcept {
		return m_objs.size();
	}
	
	//! Copy the contents of the given input stream into an output stream
	//! The input object is a structure keeping information about the possibly existing Key, the type
	//! as well as the actual stream which contains the data to be copied into the memory database.
	//! \tparam InputObject type providing a type id, a size and the stream to read the object from.
	//! \warning the iterator stays only valid as long as the database does not change, which is until
	//! you call a non-constant method
	template <class InputObject>
	forward_iterator insert(InputObject& object);
	
	//! insert the copy's of the contents of the given input iterators into this object database
	//! The inserted items can be queried using the keys from the input iterators
	template <class Iterator>
	void insert(Iterator begin, const Iterator end);
	
	//! Same as above, but will produce the required serialized version of object automatically
	forward_iterator insert(typename traits_type::input_reference_type object);
	
};



template <class ObjectTraits>
template <class InputObject>
typename odb_mem<ObjectTraits>::forward_iterator odb_mem<ObjectTraits>::insert(InputObject& iobj)
{
	static_assert(sizeof(typename ObjectTraits::char_type) == sizeof(typename InputObject::stream_type::char_type), "char types incompatible");
	output_object_type oobj(iobj.type(), iobj.size());
	
	typename output_object_type::stream_type::char_type buf[parent_type::gCopyChunkSize];
	typename traits_type::size_type total_bytes_read = 0;
	auto pkey = iobj.key_pointer();
	auto& istream = iobj.stream();
	auto& odata = oobj.data();
	odata.reserve(iobj.size());
	
	for (std::streamsize bytes_read = parent_type::gCopyChunkSize; bytes_read == parent_type::gCopyChunkSize; total_bytes_read += bytes_read) {
		istream.read(buf, parent_type::gCopyChunkSize);
		bytes_read = istream.gcount();
		odata.insert(odata.end(), buf, buf+bytes_read);
	}// for each chunk to copy
	
	
	if (pkey) {
		return forward_iterator(m_objs.insert(typename map_type::value_type(*pkey, std::move(oobj))).first);
	} else {
		typename traits_type::hash_generator_type hashgen;
		hashgen.update(odata.data(), odata.size());
		return forward_iterator(m_objs.insert(typename map_type::value_type(hashgen.hash(), std::move(oobj))).first);
	}// handle key exists
}

GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_MEM_HPP
