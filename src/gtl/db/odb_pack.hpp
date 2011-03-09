#ifndef GTL_ODB_PACK_HPP
#define GTL_ODB_PACK_HPP
#include <gtl/config.h>
#include <gtl/db/odb.hpp>
#include <gtl/db/odb_object.hpp>

#include <boost/filesystem/operations.hpp>

#include <memory>
#include <vector>
#include <algorithm>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace io = boost::iostreams;
namespace fs = boost::filesystem;


//! @{ \name Exceptions

//! \brief General error thrown if parsing of a pack file fails
struct pack_parse_error :	public odb_error
{};

//! @}

/** \brief Defines the interface of all pack files which is used by the pack object database.
  * \note the template parameters are used only to have meaningful types in the method signatures, 
  * the implementor is not obgliged to use templates at all.
  * A pack file is always created on the heap by the pack database as it is assumed that there 
  * are non-copyable members (like streams, or memory mapped files). The pack database is taking
  * ownership of the instance and will dispose it as required.
  * \tparam TraitsType traits identifying pack database specific types
  */
template <class TraitsType>
class odb_pack_file
{
public:
	//! An accessor pointing to exactly one object
	typedef uint										accessor;
	//! An iterator to more forward through the objects in the database
	typedef uint										forward_iterator;
	
protected:
	//! @{ Internal Use
	typedef TraitsType									db_traits_type;
	typedef typename db_traits_type::obj_traits_type	obj_traits_type;
	typedef typename db_traits_type::path_type			path_type;
	//! @} end internal use typedefs
	
public:
		//! Instantiate this instance with the path to a pack file
	odb_pack_file(const path_type& path);
	
public:
	
	//! \return a new instance of this class initialized with the given file
	//! or nullptr if the file is not suitable to be handled by this pack type
	//! The memory is managed by the caller, it will be deleted using a delete call
	//! \todo maybe allow overriding the deletor function of the unique_ptr
	static odb_pack_file* new_pack(const path_type& file);
	
	//! \return the pack's path this instance was initialized with
	//! \note the interface requires it to be cached as it needs fast access
	//! to this information in case of a cache update
	const path_type& pack_path() const;
	
	//! \return iterator to the beginning of all output objects in this pack
	//! \note only valid as long as the parent pack is valid
	forward_iterator begin() const;
	
	//! \return iterator to the end of a given pack
	forward_iterator end() const;
};


//! \brief an accessor providing access to a single entry in a single pack file
template <class TraitsType>
class pack_accessor : public odb_accessor<TraitsType>
{
public:
	typedef TraitsType												db_traits_type;
	typedef typename db_traits_type::obj_traits_type				obj_traits_type;
	typedef typename obj_traits_type::key_type						key_type;
	typedef typename obj_traits_type::size_type						size_type;
	typedef typename obj_traits_type::object_type					object_type;
	typedef pack_accessor<db_traits_type>							this_type;
	
public:
	pack_accessor()
	{}
	
	pack_accessor(this_type&&) = default;
	
	//! Equality comparison of compatible iterators
	inline bool operator==(const this_type& rhs) const {
		return true;
	}
	
	//! Inequality comparison
	inline bool operator!=(const this_type& rhs) const {
		return !(*this == rhs);
	}
	
	//! allows access to the actual input object
	inline const this_type& operator*() const {
		return *this;
	}
	
	//! allow -> semantics
	inline const this_type* operator->() const {
		return this;
	}
	
public:
	key_type key() const { 
		return key_type();
	}
};


/** \brief iterator allowing to step through all objects within the packed object database
  */
template <class TraitsType>
class pack_forward_iterator : public pack_accessor<TraitsType>
{
public:
	pack_forward_iterator() {}
	
};

/** \brief Defines types used specifically in a packed object database.
  */
template <class ObjectTraits>
struct odb_pack_traits : public odb_file_traits<typename ObjectTraits::key_type,
												typename ObjectTraits::hash_generator_type>
{
	typedef ObjectTraits									obj_traits_type;
	
	//! Type used to handle the reading of packs
	//! \note see the dummy type for a complete interface, which includes typedefs
	typedef odb_pack_file<odb_pack_traits>					pack_reader_type;
	
	
};


/** \brief Implements a database which reads objects from highly compressed packs.
  * A pack is a file with a collection of objects which are either stored as compressed object 
  * or as a delta against another object ( which may itself be deltified as well ). A base to a delta
  * object is specified either as relative offset within the current pack file, or as key identifying
  * the object in some database, which is not necessarily this one.
  * A pack file comes with a header, which provides the index necessary to quickly find objects within the pack.
  * The details of how the format of the pack looks like, are encapsulated in the PackFile's interface.
  * 
  * Every pack implementation must support reading of existing packs, writing of packs is optional.
  * When writing a new pack, a list of object keys is provided which in turn gets congested into a new pack file.
  *
  * The pack is implemented using delay-loading mechanism, so that actual drive access will be delayed until the
  * first time the instance is used.
  * 
  * \tparam ObjectTraits traits to define the objects to be stored within the database
  * \tparam Traits traits specific to the database itself, which includes the actual pack format
  */ 
template <class TraitsType>
class odb_pack :	public odb_base<TraitsType>,
					public odb_file_mixin<typename TraitsType::path_type>
{
public:
	typedef TraitsType									db_traits_type;
	typedef typename db_traits_type::obj_traits_type	obj_traits_type;
	
	typedef typename obj_traits_type::key_type			key_type;
	typedef typename obj_traits_type::char_type			char_type;
	typedef odb_hash_error<key_type>					hash_error_type;
	
	typedef typename db_traits_type::path_type			path_type;
	typedef typename db_traits_type::pack_reader_type	pack_reader_type;
	
	typedef pack_accessor<db_traits_type>				accessor;
	typedef pack_forward_iterator<db_traits_type>		forward_iterator;
	
	typedef std::vector<std::unique_ptr<pack_reader_type> > vector_pack_readers;
	
protected:
	mutable vector_pack_readers m_packs;
	
protected:
	//! \return true if we have no cached packs, and need an update
	inline bool needs_update() const {
		return m_packs.size() == 0;
	}
	
	//! update our cache if we didn't initialize it yet
	inline void assure_update() const {
		if (needs_update()) {
			const_cast<odb_pack<db_traits_type>*>(this)->update_cache();
		}
	}
	
public:
	odb_pack(const path_type& root)
	    : odb_file_mixin<path_type>(root)
	{}
	
public:
	//! @{ \name Pack Interface
	
	//! Update the internal cache of pack file instances. Call this method if the underlying 
	//! set of files has changed after the database was first instantiated.
	void update_cache();
	
	//! \return vector of unique pointers of packs
	const vector_pack_readers& packs() const {
		assure_update();
		return m_packs;
	}
	
	//! @} end pack interface
	
	//! @{ odb interface
	
	bool has_object(const key_type& k) const {
		assure_update();
		return false;
	}
	
	accessor object(const key_type& k) const {
		assure_update();
		return accessor();
	}
	
	forward_iterator begin() const {
		assure_update();
		return forward_iterator();
	}
	
	forward_iterator end() const {
		assure_update();
		return forward_iterator();
	}
	
	//! \todo fast implementation
	size_t count() const {
		auto start(begin());
		return _count(start, end());
	}
	//! @} end odb interface
	
};


template <class TraitsType>
void odb_pack<TraitsType>::update_cache()
{
	typedef	fs::basic_directory_iterator<path_type> dir_iter_type;
	
	dir_iter_type iter(this->m_root);
	const dir_iter_type dir_end;
	
	for (; iter != dir_end; ++iter) {
		// skip packs we already have 
		const path_type& path = iter->path();
		auto packit = std::find_if(m_packs.begin(), m_packs.end(), 
									[path](const typename vector_pack_readers::value_type& uptr) -> bool
									{return (uptr.get()->pack_path() == path);});
		if (packit != m_packs.end()) {
			continue;
		}
		
		pack_reader_type* pack = pack_reader_type::new_pack(path);
		if (pack == nullptr) {
			continue;
		}
		
		m_packs.push_back(typename vector_pack_readers::value_type(pack));
	}// for each directory entry
}

GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_PACK_HPP
