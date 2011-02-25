#ifndef GTL_ODB_LOOSE_HPP
#define GTL_ODB_LOOSE_HPP

#include <gtl/config.h>
#include <gtl/db/odb.hpp>
#include <gtl/db/odb_object.hpp>
#include <gtl/db/hash_generator_filter.hpp>sh
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/filesystem.hpp>
#include <cstring>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace io = boost::iostreams;
namespace fs = boost::filesystem;

//! Tag specifying the header is supposed to be compressed, as well as the rest of the data stream
struct compress_header_tag {};
//! Tag specifying the header is supposed to stay uncompressed, while the data stream is compressed
struct uncompressed_header_tag {};


//! \brief stream suitable for reading any kind of loose object format
//! This is the base template which is partially specialized for the respective header tags
//! \tparam ObjectTraits traits for some general git settings
template <class ObjectTraits, class Traits, class HeaderTag=typename Traits::header_tag>
class loose_object_input_stream
{
};

/** Partial specialization of the base template to allow handling fully compressed files, that is
  * the header and the data stream are part of a single compressed stream.
  */
template <class ObjectTraits, class Traits>
class loose_object_input_stream<ObjectTraits, Traits, compress_header_tag> : 
        public io::filtering_stream<io::input, typename ObjectTraits::char_type>
{
public:
	typedef Traits			db_traits_type;
	typedef ObjectTraits	traits_type;
	typedef typename db_traits_type::path_type path_type;
	
	//! Initalize the instance for reading the given file.
	loose_object_input_stream()
	{
		// init filter
	}
	
public:
	// interface
	// Set the path we should operate on
	void set_path(const path_type& path) {
		// see if we have a sink already, if so, pop it
		// put in the new sink
	}
	
};

/** Partial specialization dealing with partially compressed files.
  * The header is read uncompressed, the stream is compressed.
  */
template <class ObjectTraits, class Traits>
class loose_object_input_stream<ObjectTraits, Traits, uncompressed_header_tag>
{
	typedef Traits			db_traits_type;
	typedef ObjectTraits	traits_type;
	
	

};

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
	
	//! Tag specifying how the header should be handled
	typedef compress_header_tag header_tag;
	
	//! Represents a policy type which provides implementations for key-functionality of the object database
	typedef odb_loose_policy policy_type;
};

/** \brief object providing access to a specific database object. Depending on the actual object format, this 
  * may involve partial decompression of the object's data stream.
  * \note when you query any information about the object, like its type or size, a stream will be opened
  * and kept open for further reading. This implies you should dispose this object as soon as possible to 
  * release the associated system resources.
  */
template <class ObjectTraits, class Traits>
class odb_loose_output_object
{
public:
	typedef ObjectTraits						traits_type;
	typedef Traits								db_traits_type;
	typedef typename db_traits_type::path_type	path_type;
	typedef typename traits_type::size_type		size_type;
	typedef typename traits_type::object_type	object_type;
	typedef odb_loose_output_object				this_type;
	
private:
	//! Initialize our stream for reading, basically read-in the header information
	//! and keep the stream available for actual data reading
	void init() const {
		m_initialized = true;
	}
	
protected:
	path_type					m_path;
	mutable bool				m_initialized;
	mutable size_type			m_size;
	mutable object_type			m_obj_type;
	
public:
	
	odb_loose_output_object(odb_loose_output_object&&) = default;
	
	odb_loose_output_object()
		: m_initialized(false) 
	{}
	
	odb_loose_output_object(const this_type& rhs)
	{
		*this = rhs;
	}
	
	odb_loose_output_object(const path_type& obj_path)
		: m_path(obj_path)
	    , m_initialized(false)
	{};
	
	this_type& operator = (const this_type& rhs) {
		m_initialized = rhs.m_initialized;
		m_path = rhs.m_path;
		
		if (m_initialized) {
			m_size = rhs.m_size;
			m_obj_type = rhs.m_obj_type;
		}
		return *this;
	}
	
	object_type type() const {
		if (!m_initialized){
			init();
		}
		return m_obj_type;
	}
	
	size_type size() const {
		if (!m_initialized){
			init();
		}
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
		m_initialized = false;	// could change the path, and usually does !
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
	typedef typename traits_type::object_type					object_type;
	typedef typename db_traits_type::path_type					path_type;
	typedef loose_accessor<traits_type, db_traits_type>			this_type;
	
protected:
	mutable output_object_type	m_obj;
	
protected:
	//! Default constructor, only for derived types
	loose_accessor(){}
	
public:
	//! Initialize this instance with a key to operate upon.
	//! It should, but is not required, to point to a valid object
	loose_accessor(const path_type& objpath)
		: m_obj(objpath)
	{}
	
	loose_accessor(const this_type& rhs) 
	    : m_obj(rhs.m_obj)
	{}
	
	loose_accessor(this_type&&) = default;
	
	//! Equality comparison of compatible iterators
	inline bool operator==(const this_type& rhs) const {
		return m_obj == rhs.m_obj;
	}
	
	//! Inequality comparison
	inline bool operator!=(const this_type& rhs) const {
		return !(m_obj == rhs.m_obj);
	}
	
	//! allows access to the actual input object
	inline const output_object_type& operator*() const {
		return m_obj;
	}
	
	//! allow -> semantics
	inline const output_object_type* operator->() const {
		return &m_obj;
	}
	
public:
	//! \return key matching our path
	//! \note this generates the key instance from our object's path
	//! \todo implementation could be more efficient by manually parsing the path's buffer - if we 
	//! make plenty of assumptions, this would be easy to write too.
	key_type key() const {
		// convert path to temporary key
		typedef typename path_type::string_type::value_type char_type;
		typedef typename path_type::string_type string_type;
		assert(!m_obj.path().empty());
		
		string_type filename = m_obj.path().filename();
		string_type parent_dir = m_obj.path().parent_path().filename();
		assert(filename.size()/2 == key_type::hash_len - db_traits_type::num_prefix_characters);
		assert(parent_dir.size()/2 == db_traits_type::num_prefix_characters);
		
		// try to be more efficient regarding allocation by reserving the pre-determined
		// mount of bytes. Could be static buffer.
		string_type tmp;
		tmp.reserve(key_type::hash_len*2);
		tmp.insert(tmp.end(), parent_dir.begin(), parent_dir.end());
		tmp.insert(tmp.end(), filename.begin(), filename.end());
		
		return key_type(tmp);
	}
	
};


/** \brief iterator for all loose objects in the database.
  * It iterates folders and files within these folders until the folder interation is depleted.
  * \tparam ObjectTraits traits specifying general git parameters and types
  * \tparam Traits traits to configure the database implentation
  * \todo derive from boost::iterator_facade, which would allow to remove plenty of boilerplate
  * \note the iterator keeps an internal object which is changed on each iteration step. If you queried 
  * its information once, it will use system resources. On the next step, these are being released automatically
  * as the object then points to a different path.
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
	typedef typename traits_type::object_type	object_type;
	typedef typename db_traits_type::path_type	path_type;
	typedef loose_forward_iterator				this_type;
	
protected:
	boost::filesystem::recursive_directory_iterator m_iter;
	
protected:
	//! increment our iterator to the next object file
	void next() {
		boost::filesystem::recursive_directory_iterator end;
		// have to increment m_iter prior to looping as we broke out the previous loop,
		// which fails to increment m_iter. This is good, as comparisons with end (from the user side)
		// would be false-positive if we increment m_iter at the end of our iteration
		for (++m_iter; m_iter != end; ++m_iter){
			if (fs::is_regular_file(m_iter->status())) {
				const path_type& path = m_iter->path();
				
				// verify path tokens are composed to be one of our known files (it must be possible to 
				// construct a key from it)
				if (path.filename().size()/2 == key_type::hash_len - db_traits_type::num_prefix_characters &&
				    (*(--(--path.end()))).size()/2 == db_traits_type::num_prefix_characters)
				{
					this->m_obj.path() = path;
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
	{}
	
	//! copy constructor
	loose_forward_iterator(const this_type& rhs)
	    : loose_accessor<ObjectTraits, Traits>(rhs)
	    , m_iter(rhs.m_iter)
	{}
	
	loose_forward_iterator(this_type&&) = default;
	
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
		auto start(begin());
		return _count(start, end());
	}
};


GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_LOOSE_HPP
