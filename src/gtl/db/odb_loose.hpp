#ifndef GTL_ODB_LOOSE_HPP
#define GTL_ODB_LOOSE_HPP

#include <gtl/config.h>
#include <gtl/db/odb.hpp>
#include <gtl/db/odb_object.hpp>
#include <gtl/db/mapped_memory_manager.hpp>
#include <gtl/util.hpp>

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/type_traits/is_same.hpp>

#include <fstream>
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
template <class TraitsType, size_t BufLen>
class header_filter
{
public:
	typedef TraitsType																db_traits_type;
	typedef typename db_traits_type::obj_traits_type								obj_traits_type;
	typedef header_filter<db_traits_type, BufLen>									this_type;
	typedef typename obj_traits_type::char_type										char_type;
	typedef typename obj_traits_type::size_type										size_type;
	typedef typename obj_traits_type::object_type									object_type;
	
    struct category
        : boost::iostreams::dual_use,
          boost::iostreams::filter_tag,
          boost::iostreams::multichar_tag,
          boost::iostreams::optimally_buffered_tag
        { };
	
protected:
	object_type					m_type;		//!< object type as parsed from the header
	size_type					m_size;		//!< uncompressed size in bytes as parsed from the header

private:
	header_filter(header_filter&&) = default;
	
	
private:
	
	template <class Source>
	std::streamsize update_values(Source& src, char_type*& buf)
	{
		// update our header information
		std::streamsize nb = boost::iostreams::read(src, buf, BufLen);
		size_t header_len = typename db_traits_type::policy_type().parse_header(buf, nb, m_type, m_size);
		buf += header_len;
		return nb;
	}
	
	inline bool needs_update() const {
		return m_type == obj_traits_type::null_object_type;
	}
	
	inline bool must_write_header() const {
		return !needs_update();
	}
	
	//! Make this instance ready for a new underlying stream
	void reset() {
		m_type = obj_traits_type::null_object_type;
	}
	
	
public:
	explicit header_filter(object_type type = obj_traits_type::null_object_type, size_type size = 0)
	    : m_type(type)
	    , m_size(size)
	{}
	
	header_filter(const this_type& rhs) {
		// its probably a rare case that we actually have to duplicate an initialized filter
		if (!rhs.needs_update()){
			m_type = rhs.m_type;
			m_size = rhs.m_size;
		} else {
			// otherwise its just a normal construction
			reset();
		}
	}
	
public:
	
	//! @{ \name Interface

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
		// We require this to happen in one step as we do not cache pre-read header data
		assert(n >= this->optimal_buffer_size());
		static_assert(BufLen < 1024*4, "putpack doesn't work if we cannot buffer the whole buflen");
		std::streamsize bytes_handled = -1;
		if (needs_update()) {
			char_type buf[BufLen];
			char_type* bstart(buf);
			
			auto nb = update_values(src, bstart);
			bytes_handled = nb-(bstart-buf);
			std::memcpy(s, bstart, bytes_handled);
			s += bytes_handled;
			n -= nb;
		}
		
		const std::streamsize bytes_read = boost::iostreams::read(src, s, n);
		if (bytes_read > -1){		// handle eof
			bytes_handled += bytes_read;
		}
		
		return bytes_handled;
    }

    template<typename Sink>
    std::streamsize write(Sink& snk, const char_type* s, std::streamsize n)
    {
		if (must_write_header()) {
			// provide a real stream. We will copy its contents into our device
			// As iostream would copy the buffer if we demanded it, we use a back-inserter
			// get pointer stays at 0 of course
			// flag ourselves to indicate the header is written
			typedef std::basic_string<char_type> string_type;
			string_type buf;
			buf.reserve(64);		// a normal git header is much less actually
			io::stream<io::back_insert_device<string_type> > ostream(buf);
			
			typename db_traits_type::policy_type().write_header(ostream, m_type, m_size);
			ostream.flush();
			assert(buf.size());
			// We may not count the bytes written, the caller expects to only get the number
			// of bytes he actually wrote ... otherwise we get all kinds of issues.
			io::write(snk, buf.data(), buf.size());
			
			m_type = obj_traits_type::null_object_type;
		}
		return io::write(snk, s, n);
    }
	
	//! @} stream interface
	
};


//! Tag specifying the header is supposed to be compressed, as well as the rest of the data stream
struct compressed_header_tag {};
//! Tag specifying the header is supposed to stay uncompressed, while the data stream is compressed
struct uncompressed_header_tag {};


//! \brief stream suitable for reading any kind of loose object format
//! This is the base template which is partially specialized for the respective header tags
//! \tparam ObjectTraits traits for some general git settings
template <class TraitsType, class HeaderTag=typename TraitsType::header_tag, size_t BufLen=128>
class loose_object_input_stream
{
};

/** \brief stream designed to write object data into a file on disk.
  * 
  * It doesn't particularly care about the location of the file, which is defined by the creator of the instance.
  * instead, it will hash the written bytes on demand, and provide the obtained hash in the end of the write operation
  * when the stream is closed.
  * 
  * The type is specialized for the respective header configurations, as defined by the header tag.
  * Instance of this type are implemented as single-use objects - they are initialized with all information they need, 
  * and must be discarded after usage.
  *
  * If any path is given to an instance of this type for writing, it is assumed to be writable.
  * \todo make filtering_stream use the obj_traits_type::char_type - currently it must use char directly 
  * as it will not allow the component method to be used otherwise
  * \todo compression level should be runtime configurable
  */
template <class TraitsType, class HeaderTag=typename TraitsType::header_tag>
class loose_object_output_stream : public io::filtering_stream<io::output, char>
{
public:
	typedef TraitsType													db_traits_type;
	typedef typename db_traits_type::obj_traits_type					obj_traits_type;
	typedef typename obj_traits_type::object_type						object_type;
	typedef typename db_traits_type::path_type							path_type;
	typedef typename obj_traits_type::size_type							size_type;
	typedef typename obj_traits_type::key_type							key_type;
	typedef typename obj_traits_type::char_type							char_type;
	typedef typename db_traits_type::hash_filter_type					hash_filter_type;
	typedef header_filter<db_traits_type, 128>							header_filter_type;
	typedef loose_object_output_stream<db_traits_type, HeaderTag>		this_type;
	
private:
	loose_object_output_stream(const this_type&);
	
protected:
	std::ofstream			m_os;	//!< file output stream
	
public:
	
	//! Initialize the instance. Additional information is provided to allow writing a header which contains the 
	//! object type as well as its data size
	//! \param gen_hash if True, every byte written through this stream will be used to generate a hash, which can be queried 
	//! using the hash_gen() method
	//! 
	loose_object_output_stream(const path_type& destination, const object_type& type, const size_type& size, bool gen_hash)
	{
		const bool compress_header = boost::is_same<compressed_header_tag, HeaderTag>::value;
		if (compress_header) {
			this->push(header_filter_type(type, size));
		}
		if (gen_hash) {
			this->push(hash_filter_type());
		}
		this->push(typename db_traits_type::compression_filter_type());
		m_os.open(destination.string().c_str(), std::ofstream::out|std::ofstream::binary);
		this->push(m_os);
		
		// handle uncompressed header
		if (!compress_header) {
			// write the header directly, not using our compression setup. We will put the header bytes
			// into the hash generator manually too if necessary.
			assert(false);	// todo
			if (gen_hash) {
				
			}
		}
	}
	
	
public:
	//! @{ Interface
	
	//! \return hash filter used to update our hash (if the respective flag was set during instance creation)
	//! Use it to obtain the underlying hash in a variety of ways (i.e. hash() or hash(out_hash))
	//! \note returns nullptr if there is no hash_filter
	hash_filter_type* hash_filter() {
		return this->component<hash_filter_type>(1);
	}
	
	//! @} interface
};


/** Partial specialization of the base template to allow handling fully compressed files, that is
  * the header and the data stream are part of a single compressed stream.
  * \tparam BufLen amount of bytes to read into a buffer. It must be big enough to hold the enitre header.
  * It will be read into a preallocated array of bytes, and bytes not belonging to the header 
  * will be returned again during normal reads.
  * \note this type is not copy-constructible or movable. It should be movable though ... which is not (yet) the case.
  * \todo fix explicit char type - ObjectTraits::char_type should be used, but the component<>() method changes then its return value
  */
template <class TraitsType, size_t BufLen>
class loose_object_input_stream<TraitsType, compressed_header_tag, BufLen> : 
        public io::filtering_stream<io::input, char> // was ObjectTraits::char_type, but then it cannot resolve some types
{
public:
	typedef TraitsType																db_traits_type;
	typedef typename db_traits_type::obj_traits_type								obj_traits_type;
	typedef typename obj_traits_type::char_type										char_type;
	typedef header_filter<db_traits_type, BufLen>									header_filter_type;
	typedef loose_object_input_stream<db_traits_type, compressed_header_tag>		this_type;
	typedef io::filtering_stream<io::input, char_type>								parent_type;
	typedef typename obj_traits_type::size_type										size_type;
	typedef typename obj_traits_type::object_type									object_type;
	typedef typename db_traits_type::path_type										path_type;
	static const size_t																buflen = BufLen;

private:
	loose_object_input_stream(const this_type&);
	
protected:
	std::ifstream	m_is;		//!< input file stream
	
public:
	
	//! default constructor
	loose_object_input_stream(){
	}
	
public:
	
	//! @{ \name Interface
	//! Set the path we should operate on. This interface allows 
	//! set and change the path on demand, which results in an opened file.
	//! \param path empty or non-empty path
	//! \todo make a default file buffer part of our own class, to prevent the smartpointer used in boost file buffers
	void set_path(const path_type& path) {
		if (!path.empty()) {
			// rebuild ourselves, chain elements cannot be reused
			this->reset();		// empty ourselves - this should close the file too
			m_is.close();		// just in case it is still open
			this->push(header_filter_type());
			this->push(typename db_traits_type::decompression_filter_type());
			m_is.open(path.string().c_str(), std::ifstream::in|std::ifstream::binary);
			this->push(m_is);
			this->set_auto_close(true);
			
			// cause an update of the underlying stream, without taking any valueable bytes
			// off the stream. As we have to take at least 1 byte to trigger the chain, we put it 
			// pack into the last buffered filter, which happens to be ours.
			assert(!this->eof());
			
			char_type buf[1];
			this->read(&buf[0], 1);
			this->unget();	// operates on our internal buffer
			
			assert(this->type() != obj_traits_type::null_object_type);
		}
	}
	
	object_type type() const {
		return this->component<header_filter_type>(0)->type();
	}
	
	size_type size() const {
		return this->component<header_filter_type>(0)->size();
	}

	//! @} interface
	
};


/** Partial specialization dealing with partially compressed files.
  * The header is read uncompressed, the stream is compressed.
  * \todo implement uncompressed header stream
  */
template <class TraitsType, size_t BufLen>
class loose_object_input_stream<TraitsType, uncompressed_header_tag, BufLen>
{
	typedef TraitsType		db_traits_type;
	
private:
	loose_object_input_stream(const loose_object_input_stream&);

};

//! \brief policy providing key implementations for the loose object database
//! \note this struct just defines the interface, the actual implementation needs 
//! to be provided by the derived type.
struct odb_loose_policy
{
	/** Parse the header out of the given buffer. The format is known only by the 
	  * implementation, which throws an exception if the header could not be parsed
	  * \return number of bytes that actually belonged to the header.
	  * \param buf character buffer filled with header_read_buf_len bytes pre-read from the 
	  * input stream
	  * \param type parsed object type
	  * \param size parsed size
	  */
	template <class CharType, class ObjectType, class SizeType>
	size_t parse_header(CharType* buf, size_t buflen, ObjectType& type, SizeType& size);
	
	/** Write a header parsable by parse_header into the given output stream compatible object
	  * The information which may be contained are the object's type and its final size.
	  * \param stream to write to
	  * \param type of object
	  * \param size of final object data
	  */
	template <class StreamType, class ObjectType, class SizeType>
	void write_header(StreamType& stream, const ObjectType type, const SizeType size);
};


/** Default traits for the loose object database.
  * \note by default, it uses the zlib compression library which usually involves
  * external dependencies which must be met for the program to run.
  */
template <class ObjectTraits>
struct odb_loose_traits : public odb_file_traits<	typename ObjectTraits::key_type, 
													typename ObjectTraits::hash_generator_type>
{
	//! Traits defininig the object itself and how to deal with it
	typedef ObjectTraits									obj_traits_type;
	
	//! Memory manager used to handle all open files
	typedef mapped_memory_manager<>							mapped_memory_manager_type;
	
	//! amount of hash-characters used as directory into which to put the loose object files.
	//! One hash character will translate into two hexadecimal characters
	static const uint32_t									num_prefix_characters = 1;
	
	//! Tag specifying how the header should be handled
	typedef compressed_header_tag							header_tag;
	
	//! Represents a policy type which provides implementations for key-functionality of the object database
	typedef odb_loose_policy								policy_type;
};

/** \brief object providing access to a specific database object. Depending on the actual object format, this 
  * may involve partial decompression of the object's data stream.
  * \note when you query any information about the object, like its type or size, a stream will be opened
  * and kept open for further reading. This implies you should dispose this object as soon as possible to 
  * release the associated system resources.
  */
template <class TraitsType>
class odb_loose_output_object
{
public:
	typedef TraitsType										db_traits_type;
	typedef typename db_traits_type::obj_traits_type		obj_traits_type;
	typedef typename obj_traits_type::key_type				key_type;
	typedef loose_object_input_stream<db_traits_type, typename db_traits_type::header_tag> stream_type;
	typedef typename db_traits_type::path_type				path_type;
	typedef typename obj_traits_type::size_type				size_type;
	typedef typename obj_traits_type::object_type			object_type;
	typedef odb_loose_output_object							this_type;
	
private:
	//! Initialize our stream for reading, basically read-in the header information
	//! and keep the stream available for actual data reading
	void init() const {
		if (m_pstream.get() == nullptr) {
			m_pstream.reset(new stream_type);
		}
		m_pstream->set_path(m_path);
	}
	
	bool is_initialized() const {
		return m_pstream.get() != nullptr;
	}
	
protected:
	path_type								m_path;
	mutable std::unique_ptr<stream_type>	m_pstream;		//!< use of unique ptr as it is movable
	
public:
	
	odb_loose_output_object(odb_loose_output_object&&) = default;
	
	odb_loose_output_object()
	{}
	
	odb_loose_output_object(const path_type& obj_path)
		: m_path(obj_path)
	{};
	
	object_type type() const {
		if (!is_initialized()){
			init();
		}
		return m_pstream->type();
	}
	
	size_type size() const {
		if (!is_initialized()){
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
		// It will be at zero position as we don't use it except for initial header parsing
		if (m_pstream.get() != nullptr) {
			return m_pstream.release();
		}
		stream_type* stream = new stream_type;
		stream->set_path(m_path);
		return stream;
	}
	
	void deserialize(typename obj_traits_type::output_reference_type out) const {
		typename obj_traits_type::policy_type().deserialize(out, *this);
	}
	
	//! @{ Interface
	
	//! read-only version of our internal path
	const path_type& path() const {
		return m_path;
	}
	
	//! \return key matching our path
	//! \note this generates the key instance from our object's path
	//! \todo implementation could be more efficient by manually parsing the path's buffer - if we 
	//! make plenty of assumptions, this would be easy to write too.
	key_type key() const {
		// convert path to temporary key
		typedef typename path_type::string_type::value_type char_type;
		typedef typename path_type::string_type string_type;
		assert(!path().empty());
		
		string_type filename = path().filename();
		string_type parent_dir = path().parent_path().filename();
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
	
	//! modifyable version of our internal path
	void set_path(const path_type& path) {
		if (m_path == path) {
			return;
		}
		m_path = path;
		init();	// creates a stream if necessary and updates the path
	}
	
	//! @}
};



/** \brief iterator for all loose objects in the database.
  * It iterates folders and files within these folders until the folder interation is depleted.
  * \tparam ObjectTraits traits specifying general git parameters and types
  * \tparam Traits traits to configure the database implentation
  * \note the iterator keeps an internal object which is changed on each iteration step. If you queried 
  * its information once, it will use system resources. On the next step, these are being released automatically
  * as the object then points to a different path.
  */
template <class TraitsType>
class loose_forward_iterator : public boost::iterator_facade<	loose_forward_iterator<TraitsType>,
																const odb_loose_output_object<TraitsType>,
																boost::forward_traversal_tag>
{
public:
	typedef TraitsType									db_traits_type;
	typedef typename db_traits_type::obj_traits_type	obj_traits_type;
	typedef odb_loose_output_object<db_traits_type>		output_object_type;
	typedef typename obj_traits_type::key_type			key_type;
	typedef typename db_traits_type::path_type			path_type;
	typedef loose_forward_iterator<db_traits_type>		this_type;
	
protected:
	friend class boost::iterator_core_access;
	
	output_object_type								m_obj;
	boost::filesystem::recursive_directory_iterator m_iter;
	
	//! increment our iterator to the next object file
	inline void next() {
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
					this->m_obj.set_path(path);
					break;
				}
			}
		}// scrub iterator until we have a file
	}
	
	bool equal(const this_type& rhs) const {
		return m_iter == rhs.m_iter;
	}
	
	const output_object_type& dereference() const {
		return this->m_obj;
	}
	
	void increment() {
		next();
	}
	
protected:
	
	
public:
	loose_forward_iterator(const path_type& root)
	    : m_iter(root)
	{next();}
	
	//! default constructor, used as end iterator
	loose_forward_iterator()
	{}
	
	//! copy constructor
	//! this is required by the facade interface, but shouldn't actually be used
	loose_forward_iterator(const this_type& rhs)
	    : m_obj(rhs.m_obj.path())
	    , m_iter(rhs.m_iter)
	{}
	
	loose_forward_iterator(this_type&&) = default;
	
public:
	
	key_type key() const {
		return m_obj.key();
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
template <class TraitsType>
class odb_loose :	public odb_base<TraitsType>, 
					public odb_file_mixin<typename TraitsType::path_type, typename TraitsType::mapped_memory_manager_type>
{
public:
	typedef TraitsType													db_traits_type;
	typedef typename db_traits_type::mapped_memory_manager_type			mapped_memory_manager_type;
	typedef typename db_traits_type::obj_traits_type					obj_traits_type;
	typedef typename obj_traits_type::key_type							key_type;
	typedef typename obj_traits_type::char_type							char_type;
	typedef odb_hash_error<key_type>									hash_error_type;
	typedef typename db_traits_type::path_type							path_type;
	
	typedef odb_loose_output_object<db_traits_type>						output_object_type;
	typedef odb_ref_input_object<obj_traits_type>						input_object_type;
	typedef typename output_object_type::stream_type					input_stream_type;
	typedef loose_object_output_stream<db_traits_type>					output_stream_type;
	
	typedef loose_forward_iterator<db_traits_type>						forward_iterator;

protected:
	//! Generate a path for the given key - it doesn't necessarily exist
	void path_from_key(const key_type& key, path_type& out_path) const 
	{
		out_path = this->m_root;
		typename obj_traits_type::char_type buf[key_type::hash_len*2+1];
		
		for (uint i = 0; i < db_traits_type::num_prefix_characters; ++i) {
			auto hc(gtl::tohex(key.bytes()[i]));
			buf[i*2+0] = std::tolower(hc[0]);
			buf[i*2+1] = std::tolower(hc[1]);
		}
		buf[db_traits_type::num_prefix_characters*2] = '\0';
		out_path /= &buf[0];		// cstring style append
		
		for (uint i = db_traits_type::num_prefix_characters; i < key_type::hash_len; ++i) {
			auto hc(gtl::tohex(key.bytes()[i]));
			buf[i*2+0] = std::tolower(hc[0]);
			buf[i*2+1] = std::tolower(hc[1]);
		}
		buf[key_type::hash_len*2] = '\0';
		out_path /= &buf[db_traits_type::num_prefix_characters*2];
	}
	
	//! Utilty to unify object insertion
	inline path_type temppath() const {
		path_type tmp_path;
		gtl::temppath(tmp_path, "tmploose_obj");
		tmp_path = this->m_root / *(--tmp_path.end());
		return tmp_path;
	}
	
	//! moves a temporary file into the final place, assuring the destination directory exists
	inline void move_tmp_to_final(const path_type& tmp_file, const path_type& destination_file) const {
		// assure directory exists - throws on error, we only expect one directory level to be created at most
		// Then rename on top of each other, boost removes existing file, which is required on windows at least
		fs::create_directory(destination_file.parent_path());
		try {
			fs::rename(tmp_file, destination_file);
		} catch (boost::filesystem::basic_filesystem_error<path_type>& e) {
			// ignore existing files, they have the same content as they have the same hash
			// retrow if it is anything else
			if (e.code() != boost::system::errc::file_exists) {
				throw;
			} else {
				fs::remove(tmp_file);
			}
		}
	}
	
public:
	odb_loose(const path_type& root, mapped_memory_manager_type& manager)
		: odb_file_mixin<path_type, mapped_memory_manager_type>(root, manager)
	{}
	
public:
	bool has_object(const key_type& k) const {
		path_type path;
		path_from_key(k, path);
		return fs::is_regular_file(path);
	}
	
	output_object_type object(const key_type& k) const {
		path_type path;
		path_from_key(k, path);
		if (!fs::is_regular_file(path)){
			throw hash_error_type(k);
		}
		// this one should be moved automatically
		return output_object_type(std::move(path));
	}
	
	forward_iterator begin() const {
		return forward_iterator(this->m_root);
	}
	
	forward_iterator end() const {
		return forward_iterator();
	}
	
	size_t count() const {
		auto start(begin());
		return _count(start, end());
	}
	
	//! Insert the given object into the database
	//! \tparam InputObject input object compatible type
	template <class InputObject>
	key_type insert(InputObject& object);
	key_type insert_object(typename obj_traits_type::input_reference_type object);
};

template <class TraitsType>
typename odb_loose<TraitsType>::key_type odb_loose<TraitsType>::insert_object(typename TraitsType::obj_traits_type::input_reference_type object)
{
	auto policy = typename obj_traits_type::policy_type();
	std::basic_stringstream<char_type> tmp_stream;
	path_type tmp_path = this->temppath();
	
	output_stream_type ostream(tmp_path, policy.type(object), policy.compute_size(object, &tmp_stream), true);
	
	if (tmp_stream.tellp()) {
		io::copy(tmp_stream, ostream);
	} else {
		policy.serialize(object, ostream);
	}
	ostream.flush();
	// YES ! Have to do that to actually flush everything ... WTF ??
	// Also causes file to be closed
	ostream.pop();
	
	path_type final_path;
	this->path_from_key(ostream.hash_filter()->hash(), final_path);
	assert(ostream.hash_filter()->hash() != obj_traits_type::key_type::null);
	
	move_tmp_to_final(tmp_path, final_path);
	
	return ostream.hash_filter()->hash();
}

template <class TraitsType>
template <class InputObject>
typename odb_loose<TraitsType>::key_type odb_loose<TraitsType>::insert(InputObject& object)
{
	// do nothing if we have the object already
	if (object.key_pointer()) {
		try {
			if (this->has_object(*object.key_pointer())) {
				return *object.key_pointer();
			}
		} catch (odb_error&) {
			
		}
	}// handle existing items
	
	// git itself hashes the data first, then checks for existence in the db, and possible does nothing.
	// We will assume that adding an existing object is a corner case, hence we don't check for it and just
	// do the work right away.
	
	// make sure this path points into our database, tempfiles might be on another partition which would make  
	// moves expensive
	
	path_type tmp_path = this->temppath();
	output_stream_type ostream(tmp_path, object.type(), object.size(), object.key_pointer() == nullptr);
	
	io::copy(object.stream(), ostream);
	ostream.flush();
	// Make sure it is truly being flushed ! Its more like a bug that we have to do that.
	// Also causes closing of the file
	ostream.pop();			
	
	path_type final_path;
	key_type key;
	if (object.key_pointer()) {
		key = *object.key_pointer();
		this->path_from_key(key, final_path);
	} else {
		key = ostream.hash_filter()->hash();
		this->path_from_key(key, final_path);
	}
	
	move_tmp_to_final(tmp_path, final_path);
	return key;
}




GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_LOOSE_HPP
