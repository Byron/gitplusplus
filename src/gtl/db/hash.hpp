#ifndef ODB_HASH_HPP
#define ODB_HASH_HPP

#include <gtl/config.h>
#include <gtl/util.hpp>
#include <string>
#include <cstring>
#include <iostream>
#include <exception>
#include <algorithm>
#include <cctype>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

struct bad_hex_string : public std::exception 
{
	virtual const char* what() const noexcept {
		return "Invalid string format to create hash from";
	}
};

/** \brief represents a basic hash and provides common functionality.
  * \ingroup ODB
  *
  * The instance represents a hash and provides common functionality to convert
  * it into human readable formats. It contains an array of predefined size
  * 
  * \tparam HashLen number of bytes used to store the hash
  * \tparam char_type type representing the hash, needs to be of size 1
  */
template <size_t HashLen, class CharType=uchar>
class basic_hash
{
	public:
		typedef CharType		char_type;
		static const size_t		hash_len;
	
		//! hash with all bytes set to 0
		static const basic_hash null;
		
	private:
		char_type m_hash[HashLen];			//!< 20 bytes basic_hash hash
		
	private:
		// Can't delegate constructors :(
		inline void init_from_hex(const char_type* data, size_t len)
		{
			if (len != HashLen*2) {
				throw bad_hex_string();
			}
			typedef hex_char<char_type> _hex_char;
			
			const _hex_char* hdata(reinterpret_cast<const _hex_char*>(data));
			std::transform(hdata, hdata+HashLen, m_hash, fromhex<char_type>);
		}
		
		
	public:
		
		//! Leave hash data uninitialized
		basic_hash() {}
		
		//! Create a hash from a properly sized character array. It is assumed to be of size HashLen
		explicit basic_hash(const char_type* hash) {
			memcpy(m_hash, hash, HashLen);
		}
		
		//! Initializes each character hash with the given value
		explicit basic_hash(char_type val) {
			memset(m_hash, val, HashLen);
		}
		
		//! Initialize a hash from its hexadecimal representation
		//! \throw bad_hex_string
		explicit basic_hash(const char_type* data, size_t len) {
			init_from_hex(data, len);
		}
		
		//! Initialize a hash from a string
		//! \throw bad_hex_string
		explicit basic_hash(const std::string& data) {
			init_from_hex(reinterpret_cast<const char_type*>(data.c_str()), data.size());
		}

		basic_hash(const basic_hash& rhs) {
			memcpy(m_hash, rhs.m_hash, HashLen);
		}

	public:

		//! \addtogroup ops
		//! Operators
		//! @{

		//! Use a basic_hash instance as unsigned char pointer
		inline operator char_type* () {
			return m_hash;
		}
		
		//! Use a basic_hash instance as char pointer
		inline operator char_type const * () const {
			return m_hash;
		}

		//! \return true if two sha instances are equal
		inline bool operator ==(const basic_hash& rhs) const {
			return memcmp(m_hash, rhs.m_hash, HashLen) == 0;
		}
		
		//! compare for inequality
		inline bool operator !=(const basic_hash& rhs) const {
			return !(*this == rhs);
		}

		//! Assign a raw hash
		inline void operator=(const char_type* hash) {
			memcpy(m_hash, hash, HashLen);
		}

		//! @}
		
};

template <size_t HashLen, class CharType>
const basic_hash<HashLen, CharType> basic_hash<HashLen, CharType>::null((uchar)0);

template <size_t HashLen, class CharType>
const size_t basic_hash<HashLen, CharType>::hash_len(HashLen);

GTL_HEADER_END
GTL_NAMESPACE_END

//! Output basic_hash as hex into a stream
template <size_t HashLen, class CharType>
std::ostream& operator << (std::ostream& out, const gtl::basic_hash<HashLen, CharType>& rhs)
{
	// Only blocks at maxStrl bytes
	for(uint32 i = 0; i < HashLen; ++i) {
		auto hc(gtl::tohex(rhs[i]));
		out << hc[0];
		out << hc[1];
	}
	return out;
}

#endif // ODB_HASH_HPP
