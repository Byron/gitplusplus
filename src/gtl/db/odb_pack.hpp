#ifndef GTL_ODB_PACK_HPP
#define GTL_ODB_PACK_HPP
#include <gtl/config.h>
#include <gtl/db/odb.hpp>
#include <gtl/db/odb_object.hpp>
#include <gtl/db/mapped_memory_manager.hpp>

#include <boost/filesystem/operations.hpp>
#include <boost/iterator/iterator_facade.hpp>

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
	//! The type of object being returned by our accessors and iterators
	typedef uint										output_object_type;
	//! An accessor pointing to exactly one object. It conforms to the interface of the 
	//! object database accessor
	typedef uint										accessor;
	//! An iterator to move forward and backward. It provides the accessor interface as well.
	typedef std::iterator<std::bidirectional_iterator_tag, output_object_type> bidirectional_iterator;
	//! Type able to point to and identify all possible pack entries. It must be a type 
	//! which, if initialized with 0, identifies the first entry in a pack.
	typedef uint32										entry_size_type;
	
protected:
	//! @{ Internal Use
	typedef TraitsType											db_traits_type;
	typedef typename db_traits_type::obj_traits_type			obj_traits_type;
	typedef typename db_traits_type::path_type					path_type;
	typedef typename db_traits_type::obj_traits_type::key_type	key_type;
	typedef typename db_traits_type::mapped_memory_manager_type	mapped_memory_manager_type;
	//! @} end internal use typedefs
	
	
public:
	
	//! \return a new instance of this class initialized with the given file
	//! or nullptr if the file is not suitable to be handled by this pack type
	//! The memory is managed by the caller, it will be deleted using a delete call
	//! \param manager memory manager suitable for use in a managed device
	//! \todo maybe allow overriding the deletor function of the unique_ptr
	static odb_pack_file* new_pack(const path_type& file,  mapped_memory_manager_type& manager);
	
	//! \return the pack's path this instance was initialized with
	//! \note the interface requires it to be cached as it needs fast access
	//! to this information in case of a cache update
	const path_type& pack_path() const;
	
	//! \return true if the pack file contains the given object, false otherwise
	bool has_object(const key_type& k) const;
	
	//! \return iterator to the beginning of all output objects in this pack
	//! \note only valid as long as the parent pack is valid
	bidirectional_iterator begin() const;
	
	//! \return iterator to the end of a given pack
	bidirectional_iterator end() const;
	
	//! \return amount of entries in this pack
	entry_size_type num_entries() const;
	
};



/** \brief iterator allowing to step through all objects within the packed object database
  */
template <class PackDBType>
class pack_forward_iterator : public boost::iterator_facade<	pack_forward_iterator<PackDBType>,
			typename PackDBType::db_traits_type::pack_reader_type::bidirectional_iterator::value_type,
																boost::forward_traversal_tag,
			typename PackDBType::db_traits_type::pack_reader_type::bidirectional_iterator::reference>
{
public:
	typedef PackDBType														odb_pack_type;
protected:
	typedef pack_forward_iterator<odb_pack_type>							this_type;
	typedef typename odb_pack_type::vector_pack_readers						vector_pack_readers;
	typedef typename odb_pack_type::db_traits_type							db_traits_type;
	typedef typename db_traits_type::obj_traits_type						obj_traits_type;
	typedef typename obj_traits_type::key_type								key_type;
	typedef typename db_traits_type::pack_reader_type						pack_reader_type;
	typedef typename pack_reader_type::bidirectional_iterator				bidirectional_iterator;
	typedef typename bidirectional_iterator::reference						reference;
	typedef typename pack_reader_type::entry_size_type						entry_size_type;
	
	//typedef typename db_traits_type::pack_reader_type::output_object_type	output_object_type;

private:
	inline void assert_position() const {
		assert(m_ipack != m_ipack_end);
	}
	
	inline void do_assign_entry_iterators() {
		m_ientry = m_ipack->get()->begin();
		m_ientry_last = m_ipack->get()->end();
		--m_ientry_last;
	}
	
protected:
	typename vector_pack_readers::const_iterator		m_ipack;
	const typename vector_pack_readers::const_iterator	m_ipack_end;
	
	bidirectional_iterator								m_ientry;
	bidirectional_iterator								m_ientry_last;
	
protected:
	friend class boost::iterator_core_access;
	bool equal(const this_type& rhs) const {
		return (m_ientry == rhs.m_ientry && 
		        m_ipack == rhs.m_ipack);
	}
	
	//! \note an end iterator becomes invalid once it is incremented once again
	void increment() {
		if (m_ientry == m_ientry_last) {
			if (++m_ipack != m_ipack_end) {
				do_assign_entry_iterators();
			} else {
				// reset, to make it similar to our instance as end iterator
				m_ientry = bidirectional_iterator();
			}
		} else {
			++m_ientry;
		}
	}
	
	const reference dereference() const {
		assert_position();
		return *m_ientry;
	}
	
public:
	pack_forward_iterator(const vector_pack_readers& packs, bool is_end=false) 
	    : m_ipack(packs.begin())
	    , m_ipack_end(packs.end())
	{
		if (is_end) {
			m_ipack = m_ipack_end;
		}
		if (m_ipack != m_ipack_end) {
			do_assign_entry_iterators();
		}
	}
	
public:
	//! @{ \name Accessor Interface
	key_type key() const {
		assert_position();
		return m_ientry.key();
	}
	 
	//! @} end accessor interface
	
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
	
	
	//! Type providing a memory map manager interface.
	typedef mapped_memory_manager<>							mapped_memory_manager_type;
};


/** \brief Implements a database which reads objects from highly compressed packs.
  * A pack is a file with a collection of objects which are either stored as compressed object 
  * or as a delta against another object ( which may itself be deltified as well ). A base to a delta
  * object is specified either as relative offset within the current pack file, or as key identifying
  * the object in some database, which is not necessarily this one.
  * A pack file comes with an index, which provides the information necessary to quickly find objects within the pack.
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
  * \todo make the implementation thread-safe
  */ 
template <class TraitsType>
class odb_pack :	public odb_base<TraitsType>,
					public odb_file_mixin<typename TraitsType::path_type, typename TraitsType::mapped_memory_manager_type>
{
public:
	typedef TraitsType									db_traits_type;
	typedef typename db_traits_type::obj_traits_type	obj_traits_type;
	typedef odb_pack<db_traits_type>					this_type;
	
	typedef typename obj_traits_type::key_type			key_type;
	typedef typename obj_traits_type::char_type			char_type;
	typedef odb_hash_error<key_type>					hash_error_type;
	
	typedef typename db_traits_type::path_type			path_type;
	typedef typename db_traits_type::pack_reader_type	pack_reader_type;
	typedef typename db_traits_type::mapped_memory_manager_type mapped_memory_manager_type;
	
	typedef pack_forward_iterator<this_type>			accessor;
	typedef accessor									forward_iterator;
	
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
	odb_pack(const path_type& root, mapped_memory_manager_type& manager)
	    : odb_file_mixin<path_type, mapped_memory_manager_type>(root, manager)
	{}
	
public:
	//! @{ \name Pack Interface
	
	//! Update the internal cache of pack file instances. Call this method if the underlying 
	//! set of files has changed after the database was first instantiated.
	//! \todo implement handling for deleted packs - currently we will just add new ones
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
		auto fun = [&k](typename vector_pack_readers::value_type& p)->bool{ return p->has_object(k); };
		auto res = std::find_if(m_packs.begin(), m_packs.end(), fun);
		return res != m_packs.end();
	}
	
	accessor object(const key_type& k) const {
		assure_update();
		return accessor();
	}
	
	forward_iterator begin() const {
		return forward_iterator(this->packs());
	}
	
	forward_iterator end() const {
		return forward_iterator(this->packs(), true);
	}
	
	//! \todo fast implementation
	size_t count() const {
		size_t count = 0;
		std::for_each(packs().begin(), packs().end(), 
		              [&count](const typename vector_pack_readers::value_type& p){count += p.get()->num_entries();});
		return count;
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
									[&path](const typename vector_pack_readers::value_type& uptr) -> bool
									{return (uptr.get()->pack_path() == path);});
		if (packit != m_packs.end()) {
			continue;
		}
		
		pack_reader_type* pack = pack_reader_type::new_pack(path, this->manager());
		if (pack == nullptr) {
			continue;
		}
		
		m_packs.push_back(typename vector_pack_readers::value_type(pack));
	}// for each directory entry

	// todo: remove all packs which do not exist on disk anymore
}

GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_PACK_HPP
