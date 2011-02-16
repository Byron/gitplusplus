#ifndef GTL_UTIL_HPP
#define GTL_UTIL_HPP

#include <gtl/config.h>
#include <cctype>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN		

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
