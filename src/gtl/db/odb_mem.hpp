#ifndef GTL_ODB_MEM_HPP
#define GTL_ODB_MEM_HPP

#include <gtl/config.h>
#include <gtl/db/odb.hpp>
#include <gtl/db/odb_stream.hpp>
#include <map>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN


template <class Key, class ObjectTraits>
class mem_input_iterator : public odb_input_iterator<Key, ObjectTraits>
{
protected:
	typedef std::map<Key, odb_ostream<ObjectTraits> > map_type;
	typename map_type::const_iterator m_iter;

public:
	template <class Iterator>
	mem_input_iterator(const Iterator& it) : m_iter(it) {}
	
	inline bool operator==(const mem_input_iterator<Key, ObjectTraits>& rhs) const {
		return m_iter == rhs.m_iter;
	}
	inline bool operator!=(const mem_input_iterator<Key, ObjectTraits>& rhs) const {
		return m_iter != rhs.m_iter;
	}
	
	inline typename map_type::value_type::second_type& operator*() {
		return m_iter->second;
	}

	inline const typename map_type::value_type::second_type& operator*() const {
		return m_iter->second;
	}
	
	inline size_t size() const {
		(*this).size();
	}
	
	inline typename ObjectTraits::object_type type() const {
		(*this).type();
	}
};


template <class Key, class ObjectTraits>
class mem_forward_iterator : public mem_input_iterator<Key, ObjectTraits>
{
public:
	typedef std::map<Key, odb_ostream<ObjectTraits> > map_type;
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
	std::map<Key, odb_ostream<ObjectTraits> > m_objs;
	
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
	
};
		
GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_MEM_HPP
