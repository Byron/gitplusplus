#ifndef GTL_ODB_MEM_HPP
#define GTL_ODB_MEM_HPP

#include <gtl/config.h>
#include <gtl/db/odb.hpp>

#include <map>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN


template <class Key, class T>
class mem_input_iterator : public odb_input_iterator<Key, T>
{
protected:
	typedef std::map<Key, T> map_type;
	typename map_type::const_iterator m_iter;

public:
	template <class Iterator>
	mem_input_iterator(const Iterator& it) : m_iter(it) {}
	
	inline bool operator==(const mem_input_iterator<Key, T>& rhs) const {
		return m_iter == rhs.m_iter;
	}
	inline bool operator!=(const mem_input_iterator<Key, T>& rhs) const {
		return m_iter != rhs.m_iter;
	}
	
	inline T& operator->() {
		return m_iter->second;
	}

	inline const T& operator->() const {
		return m_iter->second;
	}
	
	inline T& operator*() {
		return m_iter->second;
	}

	inline const T& operator*() const {
		return m_iter->second;
	}
	
};

template <class Key, class T>
class mem_forward_iterator : public mem_input_iterator<Key, T>
{
public:
	typedef std::map<Key, T> map_type;
	typedef typename map_type::value_type value_type;
	
	template <class Iterator>
	mem_forward_iterator(const Iterator& it)
		: mem_input_iterator<Key, T>(it) {}
	
	mem_forward_iterator& operator++() {
		++(this->m_iter); return *this;
	}
	mem_forward_iterator operator++(int) {
		mem_forward_iterator cpy(*this); ++(*this); return cpy;
	}
	
	const value_type& operator->() const {
		return *(this->m_iter);
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
template <class Key, class T, class ObjectAllocator=std::allocator<T> >
class odb_mem : public odb_base<Key, T, ObjectAllocator>
{
private:
	std::map<Key, T> m_objs;
	
public:
	typedef const mem_input_iterator<Key, T> const_input_iterator;
	typedef mem_input_iterator<Key, T> input_iterator;
	typedef mem_forward_iterator<Key, T> forward_iterator;
	
	const_input_iterator find(const typename std::add_rvalue_reference<const Key>::type k) const{
		return mem_input_iterator<Key, T>(m_objs.find(k));
	}
	forward_iterator end() const {
		return mem_forward_iterator<Key, T>(m_objs.end());
	}
	
};
		
GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_MEM_HPP
