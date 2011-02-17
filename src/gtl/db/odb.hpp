#ifndef GTL_ODB_HPP
#define GTL_ODB_HPP

#include <gtl/config.h>
#include <gtl/db/odb_alloc.hpp>
#include <gtl/db/odb_iter.hpp>

#include <exception>
#include <type_traits>
#include <boost/iostreams/constants.hpp>
#include <vector>
#include <sstream>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN


/** \brief basic exception for all object database related issues
  * \ingroup ODBException
  */
class odb_error : public std::exception
{
	virtual const char* what() const throw() {
		return "general object database error";
	}
};

/** \brief thrown if there is no corresponding object in the database
  * \tparam HashType hash compatible type
  */
template <class HashType>
class odb_hash_error : public odb_error
{
	protected:
		std::string _info;
	
	public:
		odb_hash_error(const HashType& hash) {
			std::stringstream s;
			s << "object " << hash << " does not exist in database";
			_info = s.str();
		}
		
		//! definition required to simulate no-throw, which seems to be failing
		//! due to our class member
		virtual ~odb_hash_error() noexcept {}
		
		virtual const char* what() const throw() {
			return _info.c_str();
		}
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
  */
template <class ObjectTraits>
class odb_base
{
public:
	typedef ObjectTraits										traits_type;
	typedef typename traits_type::key_type						key_type;
	typedef const odb_accessor<traits_type>						accessor;
	typedef odb_forward_iterator<traits_type>					forward_iterator;
	
	typedef odb_hash_error<key_type>							hash_error_type;
	
public:
	static const std::streamsize gCopyChunkSize;
	
protected:
	//! @{ \name Subclass Utilities
	
	//! Slow counting of members by iteration. Use your own implementation if it makes sense
	template <class Iterator>
	size_t _count(Iterator& start, Iterator& end) const noexcept 
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
	
	//! \return accessor pointing to the object at the given key,
	//! Dereferencing the accessor yields access to an output object, which remains valid
	//! only as long as the iterator exists.
	//! \throw hash_error_type
	accessor object(const key_type& k) const;
	
	//! \return true if the object associated with the given key is in this object database
	//! \note although this could internally just catch object(), it should be implemented for maximum 
	//! performance, which is not exactly the case if exceptions cause the stack to be unwound.
	bool has_object(const key_type& k) const;
	
	//! Insert a new item into the database
	//! \param type identifying the object
	//! \param size size of the object in bytes
	//! \param stream whose data is to be inserted
	//! \return iterator pointing to the newly inserted item in the database. It can be used to obtain the generated object key
	//!	as well.
	template <class Stream>
	forward_iterator insert(typename traits_type::object_type type, size_t size, Stream stream);
	
	//! Same as above, but will produce the required serialized version of object automatically
	forward_iterator insert(typename traits_type::input_reference_type object);
	
	//! \return number of objects within the database.
	//! \note this might involve iterating all objects, which is costly, hence we don't name it size()
	size_t count() const;
};


//! usually size of one memory page in bytes 
template <class ObjectTraits>
const std::streamsize odb_base<ObjectTraits>::gCopyChunkSize(boost::iostreams::default_device_buffer_size);

GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_HPP
