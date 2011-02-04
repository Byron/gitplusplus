#ifndef SHA1_H
#define SHA1_H

#include <git/config.h>
#include <cstring>
#include <iostream>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

/** \brief represents a SHA1 hash and provides common functionality.
  * \ingroup ODB
  *
  * The instance represents a 20 byte sha1 hash and provides common functionality to convert
  * the sha into human readable formats
  */
class SHA1
{
	private:
		uchar m_hash[20];	//!< 20 bytes sha1 hash
		
	public:
		//! sha with all bytes set to 0
		static const SHA1 null_sha;

	public:
		
		//! Leave hash data uninitialized
		SHA1() {}
		explicit SHA1(const uchar* hash) {
			memcpy(m_hash, hash, 20);
		}
		
		//! Initializes each character hash with the given value
		explicit SHA1(uchar val) {
			memset(m_hash, val, 20);
		}

		SHA1(const SHA1& rhs) {
			memcpy(m_hash, rhs.m_hash, 20);
		}

	public:

		//! \addtogroup ops
		//! Operators
		//! @{

		//! Use a SHA1 instance as unsigned char pointer
		inline operator uchar* () {
			return m_hash;
		}
		
		//! Use a SHA1 instance as unsigned char pointer
		inline operator uchar const * () const {
			return m_hash;
		}

		//! return true if two sha instances are equal
		inline bool operator ==(const SHA1& rhs) const {
			return memcmp(m_hash, rhs.m_hash, 20) == 0;
		}

		//! compare for inequality
		inline bool operator !=(const SHA1& rhs) const {
			return !(*this == rhs);
		}

		//! Assign a raw hash
		inline void operator=(const uchar* hash) {
			memcpy(m_hash, hash, 20);
		}

		//! @}
		
};

GIT_HEADER_END
GIT_NAMESPACE_END

//! Output sha1 as hex into a stream
std::ostream& operator << (std::ostream& s, const git::SHA1& rhs);

#endif // SHA1_H
