#ifndef GTL_ODB_MEM_HPP
#define GTL_ODB_MEM_HPP

#include <gtl/config.h>
#include <gtl/db/odb.hpp>
#include <gtl/db/odb_object.hpp>
#include <sstream>
#include <boost/type_traits/add_pointer.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <map>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

/** \ingroup ODBObject
  */
template <class ObjectTraits>
class odb_mem_output_object : public odb_output_object<ObjectTraits, std::stringstream>
{
public:
	typedef std::stringstream			stream_type;
private:
	typename ObjectTraits::object_type	m_type;
	typename ObjectTraits::size_type	m_size;		//! uncompressed size
	stream_type							m_stream;	//! stream for reading and writing
	
public:
	odb_mem_output_object(typename ObjectTraits::object_type type, uint64_t size)
		: m_size(size)
		, m_type(type)
	{}
	
	typename ObjectTraits::object_type type() const {
		return m_type;
	}
	
	uint64_t size() const {
		return m_size;
	}
	
	//! \return actual stream of this instance - changes to its head position
	//! will affect the next read of the next client.
	stream_type& stream() const {
		return m_stream;
	}
};


/** Iterator providing access to a single element of the memory odb.
  * It's value can be queried read-only, and it cannot be used in iterations.
  * \ingroup ODBIter
  */
template <class ObjectTraits, class Key>
class mem_input_iterator : public odb_input_iterator<ObjectTraits, Key>
{
protected:
	typedef std::map<Key, odb_mem_output_object<ObjectTraits> > map_type;
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
	inline typename std::add_rvalue_reference<typename map_type::value_type::second_type>::type 
		operator*() {
		return m_iter->second;
	}

	//! \return constant ostream type
	inline typename std::add_rvalue_reference<const typename map_type::value_type::second_type>::type 
		operator*() const {
		return m_iter->second;
	}
	
	//! \return uncompressed size of the object in bytes
	inline uint64_t size() const {
		(*this).size();
	}
	
	//! \return type of object stored in the stream
	inline typename ObjectTraits::object_type type() const {
		(*this).type();
	}
};

/** \ingroup ODBIter
  */
template <class ObjectTraits, class Key>
class mem_forward_iterator : public mem_input_iterator<ObjectTraits, Key>
{
public:
	typedef std::map<Key, odb_mem_output_object<ObjectTraits> > map_type;
	typedef typename map_type::value_type value_type;
	
	template <class Iterator>
	mem_forward_iterator(const Iterator& it)
		: mem_input_iterator<ObjectTraits, Key>(it) {}
	
	mem_forward_iterator& operator++() {
		++(this->m_iter); return *this;
	}
	mem_forward_iterator operator++(int) {
		mem_forward_iterator cpy(*this); ++(*this); return cpy;
	}
	
	inline uint64_t size() const {
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
	std::map<Key, odb_mem_output_object<ObjectTraits> > m_objs;
	
public:
	typedef odb_base<Key, ObjectTraits> parent_type;
	typedef const mem_input_iterator<ObjectTraits, Key> const_input_iterator;
	typedef mem_input_iterator<ObjectTraits, Key> input_iterator;
	typedef mem_forward_iterator<ObjectTraits, Key> forward_iterator;
	
	const_input_iterator find(const typename std::add_rvalue_reference<const Key>::type k) const{
		return input_iterator(m_objs.find(k));
	}
	forward_iterator end() const {
		return forward_iterator(m_objs.end());
	}
	
	//! Copy the contents of the given input stream into an output stream
	//! The istream is a structure keeping information about the possibly existing Key, the type
	//! as well as the actual stream which contains the data to be copied into the memory database.
	template <class InputObject>
	forward_iterator insert(const typename std::add_rvalue_reference<const InputObject>::type object);
	
	//! insert the copy's of the contents of the given input iterators into this object database
	//! The inserted items can be queried using the keys from the input iterators
	template <class Iterator>
	void insert(Iterator begin, const Iterator end);
	
	//! Same as above, but will produce the required serialized version of object automatically
	forward_iterator insert(typename ObjectTraits::input_reference_type object);
};


/*
template <class Key, class ObjectTraits>
typename odb_mem<Key, ObjectTraits>::forward_iterator odb_mem<Key, ObjectTraits>::insert(const std::add_rvalue_reference<const InputObject>::type object)
{
	odb_mem_output_object<ObjectTraits> iostream(type, size);
	boost::iostreams::filtering_stream<boost::iostreams::input> fstream;
	boost::iostreams::copy(stream, iostream.stream());
	// ToDo: generate sha1
	return it(m_objs.insert(iostream));
}
*/

GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_MEM_HPP
