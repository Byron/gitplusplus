#ifndef GTL_UTIL_HPP
#define GTL_UTIL_HPP

#include <gtl/config.h>
#include <cctype>
#include <sstream>
#include <memory>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN

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
	static const char map[] = {'0', '1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	static const char rpart = 0x0F;
	
	hex_char<CharType> out;
	out[0] = map[(c >> 4) & rpart];
	out[1] = map[c & rpart];
	
	return out;
}

/** Convert two characters from hexadecimal form to binary 
  * \param c2 pointerto array of at least two characters
  * \return character representing the binary value of c2
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
	out = map[c2[0]] << 4;
	out |= map[c2[1]];
	
	return out;
}


GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_UTIL_HPP
