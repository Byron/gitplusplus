#ifndef GTL_UTIL_HPP
#define GTL_UTIL_HPP

#include <gtl/config.h>
#include <boost/filesystem/path.hpp>
#include <cctype>
#include <sstream>
#include <cstring>
#include <memory>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

//! produce a path to a tempfile which is most likely to be unique
//! \param path filesystem path to be altered
//! \throw filesystem error if no temppath could be produced
//! \note template is only used to allow it to be defined inline
//! \todo provide alternative signature which returns the path. This could be more efficient
//! if PathType has a move constructor
template <class PathType>
void temppath(PathType& path, const char* prefix = 0) 
{
#ifndef WIN32
		char* res = tempnam(nullptr, prefix);
		if (!res || std::strlen(res) == 0) {
			throw boost::filesystem::filesystem_error("mktemp failed", boost::system::error_code());
		}
		path = PathType(res);
		free(res);					// we are responsible for deallocating the string
#else
		error "to be done: mktemp on windows, GetTmpFile or something";
#endif
}


/** \brief exception base class which provides a string-stream for detailed errors
  * \note as it has a stream as its member, it might fail itself in low-memory situations.
  * In these cases though, an out-of-memory exceptions should have already been thrown.
  * \note use this class by inheriting from it in addition to your ordinary base
  * \ingroup ODBException
  */
class streaming_exception
{
	protected:
		mutable std::string _buf;
		std::unique_ptr<std::stringstream> _pstream;
	
	public:
		streaming_exception() = default;
		streaming_exception(const streaming_exception&) = default;
		streaming_exception(streaming_exception&&) = default;
		
		//! definition required to simulate no-throw, which seems to be failing
		//! due to our class member
		virtual ~streaming_exception() noexcept {}
		
		std::stringstream& stream() {
			if (!_pstream){
				_pstream.reset(new std::stringstream);
			}
			return *_pstream;
		}
		
		virtual const char* what() const throw() 
		{
			// Produce buffer
			if (_buf.size() == 0 && _pstream){
				try {
					_buf = _pstream->str();
				} catch (...) {
					return "failed to bake exception information into string buffer";
				}
			}
			return _buf.c_str();
		}
};

/** \brief an exception base keeping a static message detailing the cause of the exception being thrown.
  * \note as the c character string is copied, and if the allocation fails, a static information message about the
  * incident will be shown instead.
  */
class message_exception
{
private:
	char* m_msg;
	
protected:
	message_exception(const char* msg = nullptr) {
		m_msg = nullptr;
		if (msg) {
			const size_t msglen(std::strlen(msg));
			m_msg = new char[msglen+1];
			if (m_msg != nullptr) {
				std::memcpy(m_msg, msg, msglen+1);	// copy terminating 0
			} 
		}
	}
	
	~message_exception() {
		if (m_msg){
			delete [] m_msg;
		}
	}
	
	virtual const char* what() const throw() {
		return m_msg == nullptr ? "failed to allocated memory to keep exception message" : m_msg;
	}
};



/** \brief manually managed heap which allows placement new (i.e. new (heap) type()) to be used to create 
  * an object on a stack-allocated area, at the time when it is needed, and not when the constructor gets called 
  * automatically.
  * Call the destroy() method to deallocate the object gracefully.
  * The management of new and delete is entirely in the hands of the caller.
  * Use it to create memory for objects which have to be constructed later, but should still be part of the own type,
  * yet don't have a copy constructor.
  * \tparam Type the type we should provide memory for
  * \note currently, no copy constructor or move constructor is supported. This could be implemented though
  */
template <class Type>
class stack_heap
{
public:
	typedef stack_heap<Type>	this_type;	
	typedef Type				type;
	static const size_t			type_size = sizeof(Type);	//!< length of the internal buffer to store the object
	
protected:
	char _inst_buf[type_size];
	
	stack_heap(const this_type&);
	stack_heap(this_type&&);
	this_type& operator = (const this_type&);

protected:
	type* get() {
		return reinterpret_cast<type*>(_inst_buf);
	}
	
	const type* get() const {
		return reinterpret_cast<const type*>(_inst_buf);
	}
	
public:
	stack_heap() {};
	
	operator type*() {
		return get();
	}
	
	operator type&() {
		return *get();
	}
	
	type* operator-> () {
		return get();
	}
	
	type& operator* () {
		return *get();
	}
	
	operator const type*() const {
		return get();
	}
	
	operator const type&() const {
		return *get();
	}
	
	const type* operator-> () const {
		return get();
	}
	
	const type& operator* () const {
		return *get();
	}
	
	void destroy() {
		get()->type::~type();
	}
};

/** \brief Same as stack_heap, but destroys itself unconditionally during its own destruction
  * \tparam auto_destruct if true, we will unconditionally destroy ourselves in our destructor. Use this only
  * if you unconditionally initialize an instance exactly once.
  */
template <class Type>
struct stack_heap_autodestruct : public stack_heap<Type>
{
	~stack_heap_autodestruct() {
		this->destroy();
	}
};


/** \brief scoped_ptr like type which allows the user to determine whether he wants to auto-delete the associated
  * memory or not.
  * The given pointer must be an array so it can be deleted with delete [] accordingly
  */
template <class T>
class managed_ptr_array : public boost::noncopyable
{
protected:
	bool managed_;
	T* p_;
	
public:
	typedef managed_ptr_array<T>	this_type;
	typedef T						element_type;
	
	//! Initialize the instance with a pointer. If managed is true, the pointer will be deleted upon our destruction
	managed_ptr_array(bool managed = false, T* p = nullptr)
	    : managed_(managed)
	    , p_(p) {}
	
	~managed_ptr_array() {
		reset();
	}
	
	managed_ptr_array(this_type&& rhs) 
	    : managed_(rhs.managed_) 
	    , p_(rhs.p_)
	{
		rhs.p_ = nullptr;	// we now own the data, in case rhs was managed
	}
	
	
public:
	//! possibly delete the contained pointer and store the given pointer instead
	void reset(T* p = 0) {
		if (managed_ && p_ != nullptr) {
			delete [] p_;
		}
		p_ = p;
	}
	
	//! \return stored pointer and stop managing it
	T* release() {
		T* tmp = p_;
		p_ = nullptr;
		return tmp;
	}
	
	//! \return stored pointer, and stop managing its lifetime. The pointer will remain within this instance though
	T* unmanage() {
		managed_ = false;
		return p_;
	}
	
	//! Take the pointer from rhs and copy its management state, while unmanaging the pointer on rhs.
	void take_ownership(this_type&& rhs) {
		bool rhs_managed = rhs.is_managed();
		reset(rhs.unmanage());
		managed_ = rhs_managed;
	}
	
	//! \return true if this instance manages the associated data, hence it deletes it upon destruction
	bool is_managed() const {
		return managed_;
	}
	
	//! \return our managed pointer
	T* get() const {
		return const_cast<this_type*>(this)->p_;
	}
	
	T& operator[] (ptrdiff_t i) const {
		return const_cast<this_type*>(this)->p_[i];
	}
	
	//! \return true if our pointer is not a nullptr
	operator bool () const {
		return p_ != nullptr;
	}
};

/** \brief Heap which, as opposed to the stack_heap_autodestruct, allows you to set a flag to enable or disable
  * whether it should auto-destroy itself. After each destruction, it will reset its flag to allow it to be filled
  * (and destroyed) once again
  */

template <class Type>
class stack_heap_managed : public stack_heap<Type>
{
public:
	typedef stack_heap_managed<Type>	this_type;
	typedef stack_heap<Type>			parent_type;
	typedef Type						type;
	
protected:
	bool _occupied;
	
public:
	stack_heap_managed()
	    : _occupied(false)
	{}
	
	~stack_heap_managed() {
		destroy_safely();
	}
	
	// byte-copy occupied instances over into ours
	stack_heap_managed(this_type&& rhs)
	{
		_occupied = rhs._occupied;
		if (_occupied) {
			std::memcpy(this->_inst_buf, rhs._inst_buf, parent_type::type_size);
			rhs._occupied = false;
		}
	}
	
	// copy constructor which allows initialization of one stack heap with another
	// If rhs is not initialized, we won't be either
	// It requires a copy constructor on the parent type
	stack_heap_managed(const this_type& rhs) 
		: _occupied(rhs._occupied) 
	{
		if (occupied()) {
			new (*this) type(*rhs);
		}
	}
	
	// assign ourselves to have the value rhs value. If rhs is not occupied, we 
	// will not be either
	this_type& operator = (const this_type& rhs) {
		if (occupied()) {
			if (rhs.occupied()) {
				**this = *rhs;
			} else {
				destroy();
			}
		} else {
			if (rhs.occupied()){
				new (*this) type(*rhs);
				_occupied = true;
			}
		}
		return *this;
	}
	
public:
	
	//! @{ Interface
	
	//! In a boolean context, we are true if occupied
	operator bool() const {
		return _occupied;
	}
	
	bool operator!() const {
		return !_occupied;
	}
	
	//! indicate that our memory is currently occupied by an actual instance
	//! \note may only be called once after our memory was filled using new
	inline void set_occupied() {
		assert(!_occupied);
		_occupied = true;
	}
	
	inline bool occupied() const {
		return _occupied;
	}
	
	//! Destroy this instance only if we are occupied. Does not change the object's state otherwise
	inline void destroy_safely() {
		if (occupied()) {
			destroy();
		}
	}
	
	//! Occupies this instance with a default constructed version of our type.
	//! Use this method to assure you do not forget to set this instance occupied
	//! \note may only be used if the instance is not yet occupied, otherwise you leak memory
	//! \return pointer to our newly created instance which now occupies our memory
	inline type* occupy() {
		assert(!occupied());
		type* res = new (*this) type;
		_occupied = true;
		return res;
	}
	
	//! @} end interface
	
	//! Destroy the instance contained in our stack heap memory.
	inline void destroy() {
		assert(_occupied);
		parent_type::destroy();
		_occupied = false;
	}
	
};

/** Class representing two ascii characters in the range of 0-F
  * \note currently represented directly as baked character values, in fact it could 
  * do all conversions dynamically.
  */ 
template <class CharType>
struct hex_char
{
	typedef CharType char_type;
	
	char_type hex[2];
	
	inline operator char_type* () {
		return hex;
	}
	inline operator char_type const * () const {
		return hex;
	}
};

/** Convert a character to its binary ascii representation
  * \return hexadecimal representation in the form of two characters as hex_char
  */
template <class CharType>
inline 
hex_char<CharType> tohex(CharType c) 
{
	static_assert(sizeof(CharType) == 1, "need 1 byte character");
	static const char map[] = {'0', '1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
	static const char rpart = 0x0F;
	
	hex_char<CharType> out;
	out[0] = map[(c >> 4) & rpart];
	out[1] = map[c & rpart];
	
	return out;
}

/** Convert two characters from hexadecimal form to binary 
  * \param c2 pointerto array of at least two characters
  * \return character representing the binary value of c2
  * \todo get rid of the map which is initialized every time 
  * this function is called !
  */
template <class CharType>
inline
CharType fromhex(const CharType* c2)
{
	static_assert(sizeof(CharType) == 1, "need 1 byte character");
	
	static char map[71];
	map[48] = 0;	map[53] = 5;	map[65] = 10;// a
	map[49] = 1;	map[54] = 6;	map[66] = 11;// b
	map[50] = 2;	map[55] = 7;	map[67] = 12;
	map[51] = 3;	map[56] = 8;	map[68] = 13;
	map[52] = 4;	map[57] = 9;	map[69] = 14;
									map[70] = 15;// f
										
	hex_char<CharType> hc;
	hc[0] = std::toupper(c2[0]);
	hc[1] = std::toupper(c2[1]);
	
	CharType out;
	out = map[(uchar)hc[0]] << 4;
	out |= map[(uchar)hc[1]];
	
	return out;
}


GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_UTIL_HPP
