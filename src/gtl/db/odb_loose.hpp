#ifndef GTL_ODB_LOOSE_HPP
#define GTL_ODB_LOOSE_HPP

#include <gtl/config.h>
#include <gtl/db/odb.hpp>
#include <gtl/db/odb_object.hpp>
#include <gtl/db/hash_generator_filter.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/filesystem.hpp>
#include <assert.h>
#include <string>
#include <cstring>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

namespace io = boost::iostreams;
namespace fs = boost::filesystem;


/** \brief filter which automatically parses the header of a stream and makes the type and size 
  * information available after the first read operation.
  */
template <class ObjectTraits, class Traits, size_t BufLen>
class header_filter
{
public:
	typedef ObjectTraits														traits_type;
	typedef Traits																db_traits_type;
	typedef header_filter<traits_type, db_traits_type, BufLen>					this_type;
	typedef typename traits_type::char_type										char_type;
	typedef typename traits_type::size_type										size_type;
	typedef typename traits_type::object_type									object_type;
	
    struct category
        : boost::iostreams::input,
          boost::iostreams::filter_tag,
          boost::iostreams::multichar_tag,
          boost::iostreams::optimally_buffered_tag
        { };
	
protected:
	object_type					m_type;		//!< object type as parsed from the header
	size_type					m_size;		//!< uncompressed size in bytes as parsed from the header
	char_type					m_buf[BufLen];		//!< buffer to keep pre-read bytes
	char_type*					m_ibuf;		//!< current position into the buffer

private:
	header_filter(header_filter&&) = default;
	
	
private:
	
	template <class Source>
	void update_values(Source& src)
	{
		// update our header information
		std::streamsize nb = boost::iostreams::read(src, &m_buf[0], BufLen);
		size_t header_len = typename db_traits_type::policy_type().parse_header(m_buf, nb, m_type, m_size);
		m_ibuf = m_buf + header_len;
		std::cerr << m_type << ": header_filter.update_values: hlen = " << header_len << ", read total of " << nb << " bytes" << std::endl;
	}
	
	inline bool needs_update() const {
		return m_type == traits_type::null_object_type;
	}
	
public:
	explicit header_filter()
	{reset();}
	
	header_filter(const this_type& rhs) {
		// its probably a rare case that we actually have to duplicate an initialized filter
		if (!rhs.needs_update()){
			std::memcpy(m_buf, rhs.m_buf, BufLen);
			m_ibuf = m_buf + (rhs.m_ibuf - rhs.m_buf);
			m_type = rhs.m_type;
			m_size = rhs.m_size;
		} else {
			// otherwise its just a normal construction
			reset();
		}
	}
	
public:
	
	//! @{ \name Interface
	//! Make this instance ready for a new underlying stream
	void reset() {
		m_type = traits_type::null_object_type;
		m_ibuf = m_buf;
	}
	
	object_type type() const {
		return m_type;
	}
	
	size_type size() const {
		return m_size;
	}
	
	//! @} interface
	
	
	//! @{ \name Stream Interface
	std::streamsize optimal_buffer_size() const { return 1024*4; }
	
	//! \note we may intentionally accept reads of 0 values to force an update
	//! of our internal values
    template<typename Source>
    std::streamsize read(Source& src, char_type* s, std::streamsize n)
    {
		std::cerr << "READ CALLED: " << n << std::endl;
		const char_type*const bufend = m_buf + BufLen;
		if (m_ibuf == bufend){
			return boost::iostreams::read(src, s, n);
		}
		
		if (needs_update()) {
			update_values(src);
		}
		
		std::streamsize nbytes_to_copy = std::min(n, bufend - m_ibuf);
		memcpy(s, m_ibuf, nbytes_to_copy);
		m_ibuf += nbytes_to_copy;
		
		if(nbytes_to_copy < n) {
			// read remaining bytes from stream, update the actual amount of bytes read
			nbytes_to_copy += boost::iostreams::read(src, s + nbytes_to_copy, n - nbytes_to_copy);
		}
		
		return nbytes_to_copy;
    }

    /*template<typename Sink>
    std::streamsize write(Sink& snk, const char_type* s, std::streamsize n)
    {
        std::streamsize result = boost::iostreams::write(snk, s, n);
    }*/
	
    /*template<typename Sink>
    void close(Sink& sink, BOOST_IOS::openmode which)
    {
		m_ibuf = m_buf+BufLen;
    }*/
	
	//! @} stream interface
	
};


//! Tag specifying the header is supposed to be compressed, as well as the rest of the data stream
struct compress_header_tag {};
//! Tag specifying the header is supposed to stay uncompressed, while the data stream is compressed
struct uncompressed_header_tag {};


//! \brief stream suitable for reading any kind of loose object format
//! This is the base template which is partially specialized for the respective header tags
//! \tparam ObjectTraits traits for some general git settings
template <class ObjectTraits, class Traits, class HeaderTag=typename Traits::header_tag, size_t BufLen=512>
class loose_object_input_stream
{
};


/** Partial specialization of the base template to allow handling fully compressed files, that is
  * the header and the data stream are part of a single compressed stream.
  * \tparam BufLen amount of bytes to read into a buffer. It must be big enough to hold the enitre header.
  * It will be read into a preallocated array of bytes, and bytes not belonging to the header 
  * will be returned again during normal reads.
  * \note this type is not copy-constructible or movable. It should be movable though ... which is not (yet) the case.
  */
template <class ObjectTraits, class Traits, size_t BufLen>
class loose_object_input_stream<ObjectTraits, Traits, compress_header_tag, BufLen> : 
        public io::filtering_stream<io::input, char> // was ObjectTraits::CharTraits
{
public:
	typedef Traits																db_traits_type;
	typedef ObjectTraits														traits_type;
	typedef typename traits_type::char_type										char_type;
	typedef header_filter<traits_type, db_traits_type, BufLen>					header_filter_type;
	typedef loose_object_input_stream<ObjectTraits, Traits, compress_header_tag> this_type;
	typedef io::filtering_stream<io::input, char_type>							parent_type;
	typedef typename traits_type::size_type										size_type;
	typedef typename traits_type::object_type									object_type;
	typedef typename db_traits_type::path_type									path_type;
	typedef typename db_traits_type::decompression_filter_type					decompression_filter_type;
	static const size_t															buflen = BufLen;


public:
	
	//! default constructor
	loose_object_input_stream(){
		this->push(decompression_filter_type());
		this->push(header_filter_type());
	}
	
public:
	
	//! @{ \name Interface
	//! Set the path we should operate on. This interface allows 
	//! set and change the path on demand, which results in an opened file.
	//! \param path empty or non-empty path
	void set_path(const path_type& path) {
		// see if we have a sink already, if so, pop it
		// put in the new sink.
		assert(parent_type::size() < 4);
		if (parent_type::size() == 3) {
			this->pop();
		}
		if (!path.empty()) {
			this->push(io::basic_file_source<char_type>(path.string()));
			// zero reads trigger an update of the underlying stream
			char_type buf[1];
			this->component<header_filter_type>(1)->reset();
			this->read(buf, 0);
			assert(this->type() != traits_type::null_object_type);
		}
	}
	
	object_type type() const {
		return this->component<header_filter_type>(1)->type();
	}
	
	size_type size() const {
		return this->component<header_filter_type>(1)->size();
	}

	//! @} interface
	
};

/** Partial specialization dealing with partially compressed files.
  * The header is read uncompressed, the stream is compressed.
  * \todo implement uncompressed header stream
  */
template <class ObjectTraits, class Traits, size_t BufLen>
class loose_object_input_stream<ObjectTraits, Traits, uncompressed_header_tag, BufLen>
{
	typedef Traits			db_traits_type;
	typedef ObjectTraits	traits_type;

};

//! \brief policy providing key implementations for the loose object database
//! \note this struct just defines the interface, the actual implementation needs 
//! to be provided by the derived type.
struct odb_loose_policy
{
	/** Parse the header from the given stream. The format is known only by the 
	  * implementation, which throws an exception if the header could not be parsed
	  * \return number of bytes that actually belonged to the header.
	  * \param buf character buffer filled with header_read_buf_len bytes pre-read from the 
	  * input stream
	  * \param type parsed object type
	  * \param size parsed size
	  */
	template <class CharType, class ObjectType, class SizeType>
	size_t parse_header(CharType* buf, size_t buflen, ObjectType& type, SizeType& size);
};


/** Default traits for the loose object database.
  * \note by default, it uses the zlib compression library which usually involves
  * external dependencies which must be met for the program to run.
  */
template <class ObjectTraits>
struct odb_loose_traits
{
	typedef ObjectTraits traits_type;
	
	//! type suitable to be used for compression within the 
	//! boost iostreams filtering framework.
	//! \todo there should be a way to define the char_type, but its a bit occluded
	typedef io::zlib_compressor compression_filter_type;
	
	//! type compatible to the boost filtering framework to decompress what 
	//! was previously compressed.
	//! \todo there should be a way to define the char_type, but its a bit occluded
	typedef io::zlib_decompressor decompression_filter_type;
	
	//! type generating a filter based on the hash_filter template, using the predefined hash generator
	typedef generator_filter<typename traits_type::key_type, typename traits_type::hash_generator_type> hash_filter_type;
	
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
	typedef loose_object_input_stream<traits_type, db_traits_type, typename db_traits_type::header_tag> stream_type;
	typedef typename db_traits_type::path_type	path_type;
	typedef typename traits_type::size_type		size_type;
	typedef typename traits_type::object_type	object_type;
	typedef odb_loose_output_object				this_type;
	
private:
	//! Initialize our stream for reading, basically read-in the header information
	//! and keep the stream available for actual data reading
	void init() const {
		if (m_pstream.get() == nullptr) {
			m_pstream.reset(new stream_type);
		}
		m_pstream->set_path(m_path);
		m_initialized = true;
	}
	
protected:
	path_type								m_path;
	mutable bool							m_initialized;
	mutable std::unique_ptr<stream_type>	m_pstream;		//!< use of unique ptr as it is movable
	
public:
	
	odb_loose_output_object(odb_loose_output_object&&) = default;
	
	odb_loose_output_object()
		: m_initialized(false)
	{}
	
	/*odb_loose_output_object(const this_type& rhs)
	    : m_path(rhs.m_path)
	    , m_initialized(rhs.m_initialized)
	{
		if (m_initialized){
			// todo: set our stream to the same state as the other stream. Usually streams 
			// are not copyable, which makes it more difficult.
		}
	}*/
	
	odb_loose_output_object(const path_type& obj_path)
		: m_path(obj_path)
	    , m_initialized(false)
	{};
	
	object_type type() const {
		if (!m_initialized){
			init();
		}
		return m_pstream->type();
	}
	
	size_type size() const {
		if (!m_initialized){
			init();
		}
		return m_pstream->size();
	}
	
	void stream(stream_type* out_stream) const {
		new (out_stream) stream_type;
		out_stream->set_path(m_path);
	}
	
	stream_type* new_stream() const {
		// release our own one, otherwise create a new one
		if (m_pstream.get() != nullptr) {
			m_initialized = false;
			return m_pstream.release();
		}
		stream_type* stream = new stream_type;
		stream->set_path(m_path);
		return stream;
	}
	
	void destroy_stream(stream_type* stream) const {
		stream->~loose_object_input_stream();
	}
	
	void deserialize(typename traits_type::output_reference_type out) const {
		typename traits_type::policy_type().deserialize(out, *this);
	}
	
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
	typedef ObjectTraits										traits_type;
	typedef Traits												db_traits_type;
	typedef odb_loose_output_object<ObjectTraits, Traits>		output_object_type;
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
	
private:
	//! only allow move construction due to our file stream
	loose_accessor(const this_type& rhs) 
	    : m_obj(rhs.m_obj)
	{}
	
public:
	//! Initialize this instance with a key to operate upon.
	//! It should, but is not required, to point to a valid object
	loose_accessor(const path_type& objpath)
		: m_obj(objpath)
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

private:
	//! copy constructor - currently we only allow move semantics due to our file stream
	loose_forward_iterator(const this_type& rhs)
	    : loose_accessor<ObjectTraits, Traits>(rhs)
	    , m_iter(rhs.m_iter)
	{}
	
public:
	loose_forward_iterator(const path_type& root)
	    : m_iter(root)
	{next();}
	
	//! default constructor, used as end iterator
	loose_forward_iterator()
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
	typedef typename output_object_type::stream_type				input_stream_type;
	
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
