#ifndef GTL_ODB_MEM_HPP
#define GTL_ODB_MEM_HPP

#include <gtl/config.h>
#include <gtl/db/odb.hpp>
#include <gtl/db/odb_object.hpp>
#include <sstream>
#include <boost/type_traits/add_pointer.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <type_traits>
#include <vector>
#include <map>
#include <assert.h>
#include <iostream>	// debug

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace io = boost::iostreams;

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
	
public:
	odb_mem_output_object(typename traits_type::object_type type, typename traits_type::size_type size)
		: m_type(type)
		, m_size(size)
		
	{
	}
	
	//! Copy constructor, required to be suitable for use in pairs
	odb_mem_output_object(const odb_mem_output_object& rhs)
		: m_type(rhs.m_type)
		, m_size(rhs.m_size)
		, m_data(rhs.m_data)
	{
		std::cerr << this << " OUTPUT OBJECT COPY CONSTRUCTED" << std::endl;
	}
	
	//! Move constructor allows for more efficient copying in some cases
	odb_mem_output_object(odb_mem_output_object&& rhs) = default;
			
	
	typename traits_type::object_type type() const {
		return m_type;
	}
	
	typename traits_type::size_type size() const {
		return m_size;
	}
	
	//! \return stream wrapper around the data held in this object
	void stream(stream_type* out_stream) const {
		new (out_stream) stream_type(m_data.data(), m_data.size());
	}
	
	//! \return our actual data for manipulation
	data_type& data() {
		return m_data;
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
	typedef ObjectTraits traits_type;
	typedef typename traits_type::object_type object_type;
	typedef typename traits_type::size_type size_type;
	typedef typename traits_type::key_type key_type;
	typedef typename std::add_pointer<key_type>::type key_pointer_type;
	typedef StreamBaseType stream_type;
	
protected:
	const object_type m_type;
	const size_type m_size;
	stream_type& m_stream;
	const key_pointer_type m_key;
	
public:
	odb_mem_input_object(object_type type, size_type size, 
						 stream_type& stream,
						 const key_pointer_type key_pointer=0)
		: m_type(type)
		, m_size(size)
		, m_stream(stream)
		, m_key(key_pointer)
	{}
	
	object_type type() const {
		return m_type;
	}
	
	size_type size() const {
		return m_size;
	}
	
	stream_type& stream() {
		return m_stream;
	}
	
	const key_pointer_type key_pointer() const {
		return m_key;
	}
};


/** Iterator providing access to a single element of the memory odb.
  * It's value can be queried read-only, and it cannot be used in iterations.
  * \ingroup ODBIter
  */
template <class ObjectTraits>
class mem_input_iterator : public odb_input_iterator<ObjectTraits>
{
public:
	typedef ObjectTraits traits_type;
	typedef typename traits_type::key_type key_type;
	typedef typename traits_type::size_type size_type;
	typedef std::map<key_type, odb_mem_output_object<traits_type> > map_type;
	typedef mem_input_iterator<traits_type> this_type;
	typedef typename map_type::value_type value_type;

protected:
	typename map_type::const_iterator m_iter;
	
public:
	//! initialize the iterator from the underlying mapping type's iterator
	template <class Iterator>
	mem_input_iterator(const Iterator& it) : m_iter(it) {}
	
	//! Equality comparison of compatible iterators
	inline bool operator==(const this_type& rhs) const {
		return m_iter == rhs.m_iter;
	}
	
	//! Inequality comparison
	inline bool operator!=(const this_type& rhs) const {
		return m_iter != rhs.m_iter;
	}
	
	//! \return constant output object
	inline typename std::add_lvalue_reference<const typename map_type::value_type::second_type>::type 
		operator*() const {
		return m_iter->second;
	}
	
	//! \return key instance which identifyies our object within this map
	inline typename std::add_lvalue_reference<const typename map_type::value_type::first_type>::type
			key() const {
		return m_iter->first;
	}
	
	//! \return uncompressed size of the object in bytes
	inline size_type size() const {
		return m_iter->second.size();
	}
	
	//! \return type of object stored in the stream
	inline typename traits_type::object_type type() const {
		return m_iter->second.type();
	}
};

/** \ingroup ODBIter
  */
template <class ObjectTraits>
class mem_forward_iterator : public mem_input_iterator<ObjectTraits>
{
public:
	typedef mem_input_iterator<ObjectTraits> parent_type;

	
	template <class Iterator>
	mem_forward_iterator(const Iterator& it)
		: mem_input_iterator<ObjectTraits>(it) {}
	
	mem_forward_iterator& operator++() {
		++(this->m_iter); return *this;
	}
	mem_forward_iterator operator++(int) {
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
	typedef typename mem_input_iterator<traits_type>::map_type	map_type;
	typedef const mem_input_iterator<traits_type>				const_input_iterator;
	typedef mem_input_iterator<traits_type>						input_iterator;
	typedef mem_forward_iterator<traits_type>					forward_iterator;
	typedef odb_mem_output_object<traits_type>					output_object_type;
	typedef odb_mem_input_object<traits_type>					input_object_ref;
	
private:
	map_type m_objs;
	
public:
	
	const_input_iterator find(const typename std::add_rvalue_reference<const key_type>::type k) const throw() {
		return input_iterator(m_objs.find(k));
	}
	forward_iterator begin() const {
		return forward_iterator(m_objs.begin());
	}
	forward_iterator end() const {
		return forward_iterator(m_objs.end());
	}
	size_t count() const {
		return m_objs.size();
	}
	
	//! Copy the contents of the given input stream into an output stream
	//! The input object is a structure keeping information about the possibly existing Key, the type
	//! as well as the actual stream which contains the data to be copied into the memory database.
	//! \tparam InputObject type providing a type id, a size and the stream to read the object from.
	template <class InputObject>
	forward_iterator insert(InputObject& object) throw(std::exception);
	
	//! insert the copy's of the contents of the given input iterators into this object database
	//! The inserted items can be queried using the keys from the input iterators
	template <class Iterator>
	void insert(Iterator begin, const Iterator end);
	
	//! Same as above, but will produce the required serialized version of object automatically
	forward_iterator insert(typename traits_type::input_reference_type object);
	
};



template <class ObjectTraits>
template <class InputObject>
typename odb_mem<ObjectTraits>::forward_iterator odb_mem<ObjectTraits>::insert(InputObject& iobj) throw(std::exception)
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
		return forward_iterator(m_objs.insert(typename map_type::value_type(*pkey, oobj)).first);
	} else {
		typename traits_type::hash_generator_type hashgen;
		hashgen.update(odata.data(), odata.size());
		return forward_iterator(m_objs.insert(typename map_type::value_type(hashgen.hash(), oobj)).first);
	}// handle key exists
}

GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_MEM_HPP
