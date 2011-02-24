#ifndef GTL_ODB_LOOSE_HPP
#define GTL_ODB_LOOSE_HPP

#include <gtl/config.h>
#include <gtl/db/odb.hpp>
#include <gtl/db/odb_object.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/filesystem.hpp>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace io = boost::iostreams;
namespace fs = boost::filesystem;

//! \brief policy providing key implementations for the loose object database
//! \note this struct just defines the interface, the actual implementation needs 
//! to be provided by the derived type.
struct odb_loose_policy
{
	
};

/** Default traits for the loose object database.
  * \note by default, it uses the zlib compression library which usually involves
  * external dependencies which must be met for the program to run.
  */
struct odb_loose_traits
{
	//! type suitable to be used for compression within the 
	//! boost iostreams filtering framework.
	typedef io::zlib_compressor compression_filter_type;
	
	//! type compatible to the boost filtering framework to decompress what 
	//! was previously compressed.
	typedef io::zlib_decompressor decompression_filter_type;
	
	//! type to be used as path. The interface must comply to the boost filesystem path
	typedef boost::filesystem::path path_type;
	
	//! amount of hash-characters used as directory into which to put the loose object files.
	//! One hash character will translate into two hexadecimal characters
	static const uint32_t num_prefix_characters = 1;
	
	//! if true, the header will not be compressed, only the following datastream. This can be advantageous
	//! if you want to reuse that exact stream some place else.
	static const bool compress_header = true;
	
	//! Represents a policy type which provides implementations for key-functionality of the object database
	typedef odb_loose_policy policy_type;
};

/** \brief object providing access to a specific database object. Depending on the actual object format, this 
  * may involve partial decompression of the object's data stream
  */
template <class ObjectTraits, class Traits>
class odb_loose_output_object
{
public:
	typedef ObjectTraits						traits_type;
	typedef Traits								db_traits_type;
	typedef typename db_traits_type::path_type	path_type;
	typedef typename traits_type::size_type		size_type;
	typedef typename traits_type::object_tpye	object_type;
	typedef odb_loose_output_object				this_type;
	
protected:
	path_type			m_path;
	bool				m_initialzed;
	size_type			m_size;
	object_type			m_obj_type;
	
public:
	
	odb_loose_output_object(odb_loose_output_object&&) = default;
	
	odb_loose_output_object()
		: m_initialzed(false) 
	{}
	
	odb_loose_output_object(const path_type& obj_path)
		: m_path(obj_path)
	    , m_initialzed(false)
	{};
	
	this_type& operator = (const this_type& rhs) {
		m_initialzed = rhs.m_initialzed;
		m_path = rhs.m_path;
		
		if (m_initialzed) {
			m_size = rhs.m_size;
			m_obj_type = rhs.m_obj_type;
		}
		return *this;
	}
	
	object_type type() const {
		// todo init info
		return m_obj_type;
	}
	
	size_type size() const {
		// todo initialize
		return m_size;
	}
	
	// todo: stream interface
	
	//! @{ Interface
	
	//! read-only version of our internal path
	const path_type& path() const {
		return m_path;
	}
	
	//! modifyable version of our internal path
	path_type& path() {
		return m_path;
	}
	
	//! @}
};

/** \brief accessor pointing to one item in the database.
  */
template <class ObjectTraits, class Traits>
class loose_accessor :	public odb_accessor<ObjectTraits>
{
public:
	typedef ObjectTraits						traits_type;
	typedef Traits								db_traits_type;
	typedef odb_loose_output_object<ObjectTraits, Traits> output_object_type;
	typedef typename traits_type::key_type						key_type;
	typedef typename traits_type::size_type						size_type;
	typedef typename traits_type::object_tpye					object_type;
	typedef loose_accessor<traits_type, db_traits_type>			this_type;
	
public:
	key_type					m_key;
	mutable output_object_type	m_obj;
	
public:
	//! Initialize this instance with a key to operate upon.
	//! It should, but is not required, to point to a valid object
	loose_accessor(const key_type& key)
		: m_key(key)
	{}
	
	//! Equality comparison of compatible iterators
	inline bool operator==(const this_type& rhs) const {
		return m_key == rhs.m_key;
	}
	
	//! Inequality comparison
	inline bool operator!=(const this_type& rhs) const {
		return !(m_key == rhs.m_key);
	}
	
	//! allows access to the actual input object
	inline const output_object_type& operator*() const {
		// initialize if required
		return m_obj;
	}
	
	//! allow -> semantics
	inline const output_object_type* operator->() const {
		// initialize if required
		return &m_obj;
	}
	
public:
	const key_type& key() const {
		return m_key;
	}
	
};

/** \brief iterator for all loose objects in the database.
  * It iterates folders and files within these folders until the folder interation is depleted.
  * \todo derive from boost::iterator_facade, which would allow to remove plenty of boilerplate
  */
template <class ObjectTraits, class Traits>
class loose_forward_iterator : public loose_accessor<ObjectTraits, Traits>
{
public:
	typedef ObjectTraits						traits_type;
	typedef Traits								db_traits_type;
	typedef odb_loose_output_object<ObjectTraits, Traits> output_object_type;
	typedef typename traits_type::key_type		key_type;
	typedef typename traits_type::size_type		size_type;
	typedef typename traits_type::object_tpye	object_type;
	typedef typename db_traits_type::path_type	path_type;
	typedef loose_forward_iterator				this_type;
	
protected:
	boost::filesystem::recursive_directory_iterator m_iter;
	output_object_type								m_obj;	// object at current iteration
	
protected:
	//! increment our iterator to the next object file
	void next() {
		boost::filesystem::recursive_directory_iterator end;
		for (;m_iter != end; ++m_iter){
			if (fs::is_regular_file(m_iter->status())) {
				const fs::path& path = m_iter->path();
				// verify path tokens are composed to be one of our known files (it must be possible to 
				// construct a key from it)
				if (path.filename().size()/2 == key_type::hash_len - db_traits_type::num_prefix_characters &&
				    (*(--(--path.end()))).size()/2 == db_traits_type::num_prefix_characters)
				{
					m_obj = output_object_type(m_iter->path());
					break;
				}
			}
		}// scrub iterator until we have a file
	}
	
public:
	loose_forward_iterator(const path_type& root)
	    : m_iter(root)
	{next();}
	
	//! default constructor, used as end iterator
	loose_forward_iterator()
	{next();}
	
	//! copy constructor
	loose_forward_iterator(const this_type& rhs)
	    : m_iter(rhs.m_iter)
		, m_obj(rhs.m_obj) 
	{}
	
	//! Equality comparison of compatible iterators
	inline bool operator==(const this_type& rhs) const {
		return m_iter == rhs.m_iter;
	}
	
	//! Inequality comparison
	inline bool operator!=(const this_type& rhs) const {
		return m_iter != rhs.m_iter;
	}
	
	this_type& operator++() {
		next(); return *this;
	}
	
	this_type operator++(int) {
		this_type cpy(*this); next(); return cpy;
	}
	
public:
	
	object_type type() const {
		return m_obj.type();
	}
	
	size_type size() const {
		return m_obj.size();
	}
	
	const output_object_type& operator*() const {
		return m_obj;
	}
	
};


/** \brief Model a fully paremeterized database which stores objects as compressed files on disk.
  *
  * The files are named after their key within the database as transformed from binary to hex. The first
  * portion of the resulting string is used as directory, the rest represents the file's name.
  *
  * The files themselves are compressed containers which contain information about the object type, the uncompressed 
  * size as well as the data stream. The compression applies to the header information as well as the stream itself
  * by default, but this behaviour may be changed easily using traits.
  */
template <class ObjectTraits, class Traits>
class odb_loose : public odb_base<ObjectTraits>
{
public:
	typedef ObjectTraits											traits_type;
	typedef Traits													db_traits_type;
	typedef typename ObjectTraits::key_type							key_type;
	typedef typename db_traits_type::path_type						path_type;
	
	typedef odb_loose_output_object<traits_type, db_traits_type>	output_object_type;
	typedef odb_ref_input_object<traits_type>						input_object_type;
	
	typedef loose_accessor<traits_type, db_traits_type>				accessor;
	typedef loose_forward_iterator<traits_type, db_traits_type>		forward_iterator;

	typedef odb_hash_error<key_type>								hash_error_type;
	
protected:
	path_type		m_root;							//!< root path containing all loose object files
	
public:
	odb_loose(const path_type& root)
		: m_root(root)
	{
	}
	
public:
	bool has_object(const key_type& k) const {
		return false;
	}
	
	accessor object(const key_type& k) const {
		
	}
	
	forward_iterator begin() const {
		return forward_iterator(m_root);
	}
	
	forward_iterator end() const {
		return forward_iterator();
	}
	
	size_t count() const {
		return _count(begin(), end());
	}
};


GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_LOOSE_HPP
