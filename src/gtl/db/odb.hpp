#ifndef GTL_ODB_HPP
#define GTL_ODB_HPP

#include <gtl/config.h>
#include <gtl/util.hpp>
#include <gtl/db/odb_object.hpp>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iostreams/constants.hpp>

#include <exception>
#include <type_traits>
#include <iosfwd>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

/** \brief basic exception for all object database related issues
  * \ingroup ODBException
  */
struct odb_error : public std::exception
{
	virtual const char* what() const throw() {
		return "general object database error";
	}
};

/** \brief thrown if there is no corresponding object in the database
  * \tparam HashType hash compatible type
  */
template <class HashType>
struct odb_hash_error :	public odb_error,
						public streaming_exception
						
						
{
	public:
		odb_hash_error(const HashType& hash) {
			stream() << "object " << hash << " does not exist in database";
		}
		
	virtual const char* what() const throw() {
		return streaming_exception::what();
	}
		
};


/** \ingroup ODBIter
  * \brief input iterator which can iterate in one direction.
  * The iteration is unordered, which is why it can only be equality compared against the end
  * iterator.
  * Derefencing the iterator yields the object itself (or a reference to it). The key() method
  * provides the current key identifying the object.
  */
template <class TraitsType>
struct odb_forward_iterator :	public boost::iterator_facade<	odb_forward_iterator<TraitsType>,
																int, /*Output Object Type*/
																boost::forward_traversal_tag>
{
	typedef TraitsType												db_traits_type;
	typedef typename db_traits_type::obj_traits_type				obj_traits_type;
	typedef typename obj_traits_type::key_type						key_type;
	
	//! \return key of the object we are currently pointing to
	key_type key() const;
};


/** \brief Class providing a basic interface for all derived object database implementations
  * \ingroup ODB
  * An object database behaves much like a map, which uses keys to refer to object streams.
  * An stl container interface is provided.
  * 
  * Iterators allow access to the objects of the database. Objects are always immutable.
  *
  * This base serves as ideological basis for derived types which add an actual implementation of their 
  * specialized model.
  * \tparam TraitsType object database traits which specify object traits as well as database specifics
  */
template <class TraitsType>
class odb_base
{
public:
	typedef TraitsType											db_traits_type;
	typedef typename db_traits_type::obj_traits_type				obj_traits_type;
	typedef typename obj_traits_type::key_type						key_type;
	typedef odb_output_object<obj_traits_type, std::stringstream>	output_object_type;
	typedef odb_forward_iterator<db_traits_type>				forward_iterator;
	
	//! type compatible to odb_input_object
	typedef int													input_object_type;
	typedef odb_hash_error<key_type>							hash_error_type;
	
public:
	static const std::streamsize gCopyChunkSize;
	
protected:
	//! @{ \name Subclass Utilities
	
	//! Slow counting of members by iteration. Use your own implementation if it makes sense
	template <class Iterator>
	size_t _count(Iterator& start,const Iterator& end) const
	{
		size_t out = 0;
		for (; start != end; ++start, ++out);
		return out;
	}
	
	//! @}
	
public:
	//! \return iterator pointing to the first item in the database
	forward_iterator begin() const;
	
	//! \return iterator pointing to the end of the database, which is one past the last item
	const forward_iterator end() const;
	
	//! \return output object matching the given key
	//! \note usually the output object is a thin type which evaluates on demand to gather data from its 
	//! underlying database 
	//! \throw hash_error_type
	output_object_type object(const key_type& k) const;
	
	//! \return true if the object associated with the given key is in this object database
	//! \note although this could internally just catch object(), it should be implemented for maximum 
	//! performance, which is not exactly the case if exceptions cause the stack to be unwound.
	bool has_object(const key_type& k) const;
	
	//! Insert a new item into the database
	//! \param object input object containing all information
	//! \return key allowing retrieval of the recently added object
	key_type insert(const input_object_type& object);
	
	//! Same as above, but will produce the required serialized version of object automatically
	//! \note have to rename it to allow normal operator overloading, in case derived classes
	//! use templates in the insert method - one cannot specialize method templates at all
	//! it seems
	key_type insert_object(typename obj_traits_type::input_reference_type object);
	
	//! \return number of objects within the database.
	//! \note this might involve iterating all objects, which is costly, hence we don't name it size()
	size_t count() const;
};

/** \brief Mixin to add a lookup database pointer to the parent class
  * The lookup database is used to obtain objects from other database which may serve as basis for 
  * deltification or reference.
  * It is important to note that we store a changable pointer, which may be zero
  */
template <class LookupODBType>
class odb_lookup_mixin
{
public:
	typedef LookupODBType		lookup_odb_type;
	
protected:
	lookup_odb_type*			m_lu_odb;
	
public:
	odb_lookup_mixin(lookup_odb_type* lu_odb = nullptr)
	    : m_lu_odb(lu_odb)
	{}
	
	odb_lookup_mixin(const odb_lookup_mixin&) = default;
	odb_lookup_mixin(odb_lookup_mixin&&) = default;
	
public:
	//! \return mutable version of the lookup database
	lookup_odb_type* lu_odb() {
		return m_lu_odb;
	}
	
	//! \return read-only version of the lookup database
	const lookup_odb_type* lu_odb() const {
		return m_lu_odb;
	}
	
	//! Set the stored lookup database pointer
	void set_lu_odb(lookup_odb_type* lu_odb) {
		m_lu_odb = lu_odb;
	}
};

/** \brief Mixin adding functionality required for interaction with files
  * This mixin does not have any requirements, it just defines a simple interface to handle root paths
  */
template <class PathType, class MemoryManagerType>
class odb_file_mixin
{
public:
	typedef PathType			path_type;
	typedef MemoryManagerType	mapped_memory_manager_type;

protected:
	path_type			m_root;
	MemoryManagerType&	m_manager;
	
public:
	//! initialize a database which expects to find its files in the given root directory
	//! \note the root is not required to exist, the base
	odb_file_mixin(const path_type& root, mapped_memory_manager_type& manager)
	    : m_root(root)
	    , m_manager(manager)
	{}
	
	odb_file_mixin(const odb_file_mixin&) = default;
	odb_file_mixin(odb_file_mixin&&) = default;
	
public:
	inline const path_type& root_path() const {
		return m_root;
	}

	inline mapped_memory_manager_type& manager() {
		return m_manager;
	}
	
	inline const mapped_memory_manager_type& manager() const {
		return m_manager;
	}
};


//! usually size of one memory page in bytes 
template <class TraitsType>
const std::streamsize odb_base<TraitsType>::gCopyChunkSize(boost::iostreams::default_device_buffer_size);

GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_HPP
