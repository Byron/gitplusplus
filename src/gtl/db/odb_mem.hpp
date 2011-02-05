#ifndef GTL_ODB_MEM_HPP
#define GTL_ODB_MEM_HPP

#include <gtl/config.h>
#include <gtl/db/odb.hpp>
#include <gtl/db/odb_stream.hpp>
#include <map>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN


template <class Key, class ObjectType>
class mem_input_iterator : public odb_input_iterator<Key, ObjectType>
{
protected:
	typedef std::map<Key, odb_ostream<ObjectType> > map_type;
	typename map_type::const_iterator m_iter;

public:
	template <class Iterator>
	mem_input_iterator(const Iterator& it) : m_iter(it) {}
	
	inline bool operator==(const mem_input_iterator<Key, ObjectType>& rhs) const {
		return m_iter == rhs.m_iter;
	}
	inline bool operator!=(const mem_input_iterator<Key, ObjectType>& rhs) const {
		return m_iter != rhs.m_iter;
	}
	
	inline ObjectType& operator*() {
		return m_iter->second;
	}

	inline const ObjectType& operator*() const {
		return m_iter->second;
	}
	
	inline size_t size() const {
		(*this).size();
	}
	
	inline typename ObjectType::object_type type() const {
		(*this).type();
	}
};


template <class Key, class ObjectType>
class mem_forward_iterator : public mem_input_iterator<Key, ObjectType>
{
public:
	typedef std::map<Key, odb_ostream<ObjectType> > map_type;
	typedef typename map_type::value_type value_type;
	
	template <class Iterator>
	mem_forward_iterator(const Iterator& it)
		: mem_input_iterator<Key, ObjectType>(it) {}
	
	mem_forward_iterator& operator++() {
		++(this->m_iter); return *this;
	}
	mem_forward_iterator operator++(int) {
		mem_forward_iterator cpy(*this); ++(*this); return cpy;
	}
	
	inline size_t size() const {
		(*this).second.size();
	}
	
	inline typename ObjectType::object_type type() const {
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
template <class Key, class ObjectType>
class odb_mem : public odb_base<Key, ObjectType>
{
private:
	std::map<Key, odb_ostream<ObjectType> > m_objs;
	
public:
	typedef const mem_input_iterator<Key, ObjectType> const_input_iterator;
	typedef mem_input_iterator<Key, ObjectType> input_iterator;
	typedef mem_forward_iterator<Key, ObjectType> forward_iterator;
	
	const_input_iterator find(const typename std::add_rvalue_reference<const Key>::type k) const{
		return mem_input_iterator<Key, ObjectType>(m_objs.find(k));
	}
	forward_iterator end() const {
		return mem_forward_iterator<Key, ObjectType>(m_objs.end());
	}
	
};
		
GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_MEM_HPP
