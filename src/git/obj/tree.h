#ifndef GIT_OBJ_TREE_H
#define GIT_OBJ_TREE_H

#include <git/obj/object.hpp>
#include <git/obj/stream.h>

#include <string>
#include <map>
#include <sys/stat.h>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

using std::string;

/** \brief Represents the contents of a directory
  */
class Tree : public Object
{
public:
	typedef typename git_object_traits_base::key_type key_type;
	
	/** Structure keeping information about a tree element
	  */
	struct Element
	{
		typedef	uint32_t mode_type;	//!< type defining the file mode
		
		mode_type	mode;		//!< stat compatible mode
		key_type	key;		//!< key identifying the corresponding database object
		
		Element(mode_type mode, key_type key)
			: mode(mode)
			, key(key) {}
		Element() = default;
		Element(Element&&) = default;
		
		bool operator == (const Element& rhs) const {
			return mode == mode && key == key;
		}
		
		private:
			Element(const Element&);	//! be sure we only move this item
	};
	typedef std::map<string, Element> map_type;

private:
	map_type m_cache;		//!< tree elements for quick name based access
	
public:
    Tree();
	
	bool operator == (const Tree& rhs) const {
		return m_cache == rhs.elements();
	}
	
public:
	
	//! \return map with our elements for modification
	map_type& elements() {
		return m_cache;
	}
	
	//! \return read-only elements
	const map_type& elements() const {
		return m_cache;
	}
};


git_basic_ostream& operator << (git_basic_ostream& stream, const Tree& inst);
git_basic_istream& operator >> (git_basic_istream& stream, Tree& inst);

GIT_NAMESPACE_END
GIT_HEADER_END



#endif // GIT_OBJ_TREE_H
