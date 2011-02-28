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
#include <boost/iostreams/device/back_inserter.hpp>

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
	
	void destroy_stream(stream_type* stream) const {
		stream->~stream();
	}
	
	//! \return our actual data for manipulation
	data_type& data() noexcept {
		return m_data;
	}
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
	
	inline typename std::add_pointer<const typename map_type::value_type::second_type>::type 
		operator->() const noexcept {
		return &m_iter->second;
	}
	
	//! \return key instance which identifyies our object within this map
	//! \note this method is an optional addition, just because our implementation provides this information
	//! for free.
	inline const typename map_type::value_type::first_type& key() const noexcept {
		return m_iter->first;
	}
};

/** \ingroup ODBIter
  * \todo derive from boost::iterator_facade, which would allow to remove plenty of boilerplate
  */
template <class ObjectTraits>
class mem_forward_iterator : public mem_accessor<ObjectTraits>
{
public:
	typedef mem_accessor<ObjectTraits> parent_type;

	
	template <class Iterator>
	mem_forward_iterator(const Iterator& it)
		: mem_accessor<ObjectTraits>(it) {}
	
	mem_forward_iterator& operator++() {
		++(this->m_iter); return *this;
	}
	mem_forward_iterator operator++(int) {
		mem_forward_iterator cpy(*this); ++(this->m_iter); return cpy;
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
	typedef odb_ref_input_object<traits_type>					input_object_type;
	
protected:
	map_type m_objs;
	
public:

	//! @{ \name Subclass Implementation
	
	//! Update the given hash with header data bytes, as generated from information contained in the 
	//! output object. This method is called before any input to the hash generator was provided.
	//! \param gen hash generator to update with header information
	//! \param obj output object to be stored in the database, which keeps the type and the size of the object
	//! \todo maybe put this into a database specific policy instead. As long as we only call one method though, 
	//! it might not be worth an own policy.
	virtual void header_hash(typename traits_type::hash_generator_type& gen, const output_object_type& obj) const {}
	
	//! @}
	
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
	//! \note the iterator is guaranteed to stay valid even if the database is changed in the meanwhile, but
	//! not, of course, if the element it points to is deleted.
	//! \tparam InputObject odb_input_object compatible type
	template <class InputObject>
	forward_iterator insert(InputObject& object);

	//! Same as above, but will produce the required serialized version of object automatically
	forward_iterator insert_object(const typename traits_type::input_reference_type object);
	
	//! insert the copy's of the contents of the given input iterators into this object database
	//! The inserted items can be queried using the keys from the input iterators
	template <class Iterator>
	void insert(Iterator begin, const Iterator end);
	
	//! Same as above, but will produce the required serialized version of object automatically
	forward_iterator insert(typename traits_type::input_reference_type object);
	
};

template <class ObjectTraits>
typename odb_mem<ObjectTraits>::forward_iterator odb_mem<ObjectTraits>::insert_object(const typename ObjectTraits::input_reference_type inobj)
{
	auto policy = typename traits_type::policy_type();
	output_object_type oobj(policy.type(inobj), policy.compute_size(inobj));
	auto& odata = oobj.data();
	odata.reserve(oobj.size());
	
	io::stream<io::back_insert_device<typename output_object_type::data_type> > dest(odata);
	policy.serialize(inobj, dest);
	dest << std::flush;
	
	assert(odata.size() == oobj.size());
	typename traits_type::hash_generator_type hashgen;
	header_hash(hashgen, oobj);
	hashgen.update(odata.data(), odata.size());
	return forward_iterator(m_objs.insert(typename map_type::value_type(hashgen.hash(), std::move(oobj))).first);
}

template <class ObjectTraits>
template <class InputObject>
typename odb_mem<ObjectTraits>::forward_iterator odb_mem<ObjectTraits>::insert(InputObject& iobj)
{
	static_assert(sizeof(typename traits_type::char_type) == sizeof(typename InputObject::stream_type::char_type), "char types incompatible");
	output_object_type oobj(iobj.type(), iobj.size());
	
	auto& odata = oobj.data();
	odata.reserve(oobj.size());
	io::back_insert_device<typename output_object_type::data_type> dest(odata);
	io::copy(iobj.stream(), dest);
	
	
	auto pkey = iobj.key_pointer();
	if (pkey) {
		return forward_iterator(m_objs.insert(typename map_type::value_type(*pkey, std::move(oobj))).first);
	} else {
		typename traits_type::hash_generator_type hashgen;
		header_hash(hashgen, oobj);
		hashgen.update(odata.data(), odata.size());
		return forward_iterator(m_objs.insert(typename map_type::value_type(hashgen.hash(), std::move(oobj))).first);
	}// handle key exists
}

GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_MEM_HPP
