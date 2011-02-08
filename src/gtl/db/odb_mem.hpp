#ifndef GTL_ODB_MEM_HPP
#define GTL_ODB_MEM_HPP

#include <gtl/config.h>
#include <gtl/db/odb.hpp>
#include <gtl/db/odb_stream.hpp>
#include <sstream>
#include <boost/iostreams/copy.hpp>
#include <map>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

template <class ObjectTraits>
class odb_mem_iostream : public odb_ostream<ObjectTraits>
{
public:
	typedef std::stringstream			stream_type;
private:
	typename ObjectTraits::object_type	m_type;
	size_t								m_size;		//! uncompressed size
	stream_type							m_stream;	//! stream for reading and writing
	
public:
	odb_mem_iostream(typename ObjectTraits::object_type type, size_t size)
		: m_size(size)
		, m_type(type)
	{}
	
	typename ObjectTraits::object_type type() const {
		return m_type;
	}
	
	size_t size() const {
		return m_size;
	}
	
	//! \return actual stream of this instance - changes to its head position
	//! will affect the next read of the next client.
	const stream_type& stream() const {
		return m_stream;
	}
};

/** Iterator providing access to a single element of the memory odb.
  * It's value can be queried read-only, and it cannot be used in iterations.
  */
template <class Key, class ObjectTraits>
class mem_input_iterator : public odb_input_iterator<Key, ObjectTraits>
{
protected:
	typedef std::map<Key, odb_mem_iostream<ObjectTraits> > map_type;
	typename map_type::const_iterator m_iter;

public:
	//! initialize the iterator from the underlying mapping type's iterator
	template <class Iterator>
	mem_input_iterator(const Iterator& it) : m_iter(it) {}
	
	//! Equality comparison of compatible iterators
	inline bool operator==(const mem_input_iterator<Key, ObjectTraits>& rhs) const {
		return m_iter == rhs.m_iter;
	}
	
	//! Inequality comparison
	inline bool operator!=(const mem_input_iterator<Key, ObjectTraits>& rhs) const {
		return m_iter != rhs.m_iter;
	}
	
	//! \return ostream type
	inline typename map_type::value_type::second_type& operator*() {
		return m_iter->second;
	}

	//! \return constant ostream type
	inline const typename map_type::value_type::second_type& operator*() const {
		return m_iter->second;
	}
	
	//! \return uncompressed size of the object in bytes
	inline size_t size() const {
		(*this).size();
	}
	
	//! \return type of object stored in the stream
	inline typename ObjectTraits::object_type type() const {
		(*this).type();
	}
};


template <class Key, class ObjectTraits>
class mem_forward_iterator : public mem_input_iterator<Key, ObjectTraits>
{
public:
	typedef std::map<Key, odb_mem_iostream<ObjectTraits> > map_type;
	typedef typename map_type::value_type value_type;
	
	template <class Iterator>
	mem_forward_iterator(const Iterator& it)
		: mem_input_iterator<Key, ObjectTraits>(it) {}
	
	mem_forward_iterator& operator++() {
		++(this->m_iter); return *this;
	}
	mem_forward_iterator operator++(int) {
		mem_forward_iterator cpy(*this); ++(*this); return cpy;
	}
	
	inline size_t size() const {
		(*this).second.size();
	}
	
	inline typename ObjectTraits::object_type type() const {
		(*this).second.type();
	}
	
	const value_type& operator*() const {
		return *(this->m_iter);
	}
};

/** \brief Class providing access to objects which are cached in memory
  * \ingroup ODB
  * The memory object database acts as an adapter to a map which keeps the actual items.
  * Hence it is nothing more than map with different functionality  and special iterators
  */
template <class Key, class ObjectTraits>
class odb_mem : public odb_base<Key, ObjectTraits>
{
private:
	std::map<Key, odb_mem_iostream<ObjectTraits> > m_objs;
	
public:
	typedef const mem_input_iterator<Key, ObjectTraits> const_input_iterator;
	typedef mem_input_iterator<Key, ObjectTraits> input_iterator;
	typedef mem_forward_iterator<Key, ObjectTraits> forward_iterator;
	
	const_input_iterator find(const typename std::add_rvalue_reference<const Key>::type k) const{
		return mem_input_iterator<Key, ObjectTraits>(m_objs.find(k));
	}
	forward_iterator end() const {
		return mem_forward_iterator<Key, ObjectTraits>(m_objs.end());
	}
	
	//! Copy the contents of the given input stream into an output stream
	forward_iterator insert(typename ObjectTraits::object_type type, size_t size, typename ObjectTraits::istream_type& stream);
	
	//! insert the copy's of the contents of the given input iterators into this object database
	//! The inserted items can be queried using the keys from the input iterators
	template <class Iterator>
	void insert(Iterator begin, const Iterator end);
	
	//! Same as above, but will produce the required serialized version of object automatically
	forward_iterator insert(typename ObjectTraits::input_reference_type object);
};


template <class Key, class ObjectTraits>
typename odb_mem<Key, ObjectTraits>::forward_iterator odb_mem<Key, ObjectTraits>::insert(typename ObjectTraits::object_type type, 
																						 size_t size, 
																						 typename ObjectTraits::istream_type& stream)
{
	odb_mem_iostream<ObjectTraits> iostream(type, size);
	boost::iostreams::copy(stream, iostream.stream());
	// ToDo: generate sha1
	return it(m_objs.insert(iostream));
}
		
GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_MEM_HPP
