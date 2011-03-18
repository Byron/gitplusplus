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


/** \brief Base encapsulating virtual methods to obtain object information using a key
  * This base is used to allow pointers to base types to be stored while maintaining 
  * some basic query functionality. It provides a mechanism for runtime polymorphism, as opposed to the
  * compile time polymorphism used throughout the gtl.
  * Only some databases actually implement this if they want to be viable for use in pack databases.
  */
template <class KeyType, class ObjectType, class SizeType>
struct odb_virtual_provider
{
	typedef KeyType				key_type;
	typedef SizeType			size_type;
	typedef ObjectType			object_type;
	
	/** Pure virtual object base mimicing the templated odb_output_object as far as possible.
	  */
	struct output_object 
	{
		//! \return object's type identifier
		virtual object_type type() const = 0;
		//! \return the objects uncompressed serialized size in bytes
		virtual size_type size() const = 0;
		//! \return stream reference allowing to read the serialized object
		//! \note it is intentionally managed by this object, as it allows optimizations
		//! regarding the heap usage to be implemented by the output object.
		virtual std::istream& stream() const = 0;
	};
	
	//! \return a newly allocated output object matching the given key, or nullptr if there 
	//! is no such object.
	virtual output_object* new_object(const key_type& key) const = 0;
};


/** \brief template implementing an odb virtual provider based on any database as parameterized 
  * by its template arguments.
  * \tparam ODBType the database for which this provider should work. It must obey to the default 
  * odb_base interface
  */
template <class ODBType>
class odb_provider : public odb_virtual_provider<	typename ODBType::obj_traits_type::key_type, 
													typename ODBType::obj_traits_type::object_type,
													typename ODBType::obj_traits_type::size_type >
{
public:
	typedef ODBType											odb_type;
	typedef typename odb_type::obj_traits_type				obj_traits_type;
	typedef typename odb_type::output_object_type			output_object_type;
	typedef typename obj_traits_type::key_type				key_type;
	typedef typename obj_traits_type::object_type			object_type;
	typedef typename obj_traits_type::size_type				size_type;
	typedef typename output_object_type::stream_type		stream_type;
	typedef stack_heap<stream_type>							stack_stream_type;
	typedef odb_virtual_provider<key_type, object_type, size_type> parent_type;

protected:
	const odb_type&			m_db;	//!< database we use to retrieve information
	
public:
	odb_provider(const odb_type& db)
	    : m_db(db)
	{}
	
public:
	class output_object_impl : public parent_type::output_object
	{
	protected:
		output_object_type m_obj;
		mutable bool m_has_stream;
		stack_stream_type m_sstream;
	public:
		output_object_impl(const odb_type& db, const key_type& key)
			: m_obj(db.object(key))
		    , m_has_stream(false)
		{}
		
		~output_object_impl() {
			if (m_has_stream) {
				m_sstream.destroy();
			}
		}

	public:
		virtual object_type type() const {
			return m_obj.type();
		}
		
		virtual size_type size() const {
			return m_obj.size();
		}
		
		virtual std::istream& stream() const {
			if (!m_has_stream) {
				m_obj.stream(const_cast<stream_type*>(&*m_sstream));
				m_has_stream = true;
			}
			return const_cast<stream_type&>(*m_sstream);
		}
	};
	
public:
	
	virtual typename parent_type::output_object* new_object(const key_type& key) const {
		return new output_object_impl(m_db, key);
	}
};


/** \brief Mixin to add a virtual provider pointer to the database
  * The lookup provier is used to obtain objects from other database which may serve as basis for 
  * deltification or reference. These objects are hidden behind a runtime polymorphic virtual facade.
  * It is important to note that we store a changable pointer, which may be zero.
  */
template <class VirtualProviderType>
class odb_provider_mixin
{
public:
	typedef VirtualProviderType		provider_type;
	
protected:
	provider_type*			m_provider;
	
public:
	odb_provider_mixin(provider_type* lu_odb = nullptr)
	    : m_provider(lu_odb)
	{}
	
	odb_provider_mixin(const odb_provider_mixin&) = default;
	odb_provider_mixin(odb_provider_mixin&&) = default;
	
public:
	//! \return read-only version of provider
	const provider_type* object_provider() const {
		return m_provider;
	}
	
	//! Set the provider pointer. It is treated as a borrowed pointer
	void set_object_provider(provider_type* provider) {
		m_provider = provider;
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
