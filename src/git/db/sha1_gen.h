#ifndef SHA1_GEN_H
#define SHA1_GEN_H

#include <stddef.h>
#include <git/db/sha1.h>
#include <git/config.h>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

// You can define the endian mode in your files without modifying the SHA-1
// source files. Just #define SHA1_LITTLE_ENDIAN or #define SHA1_BIG_ENDIAN
// in your files, before including the SHA1.h header file. If you don't
// define anything, the class defaults to little endian.
#if !defined(SHA1_LITTLE_ENDIAN) && !defined(SHA1_BIG_ENDIAN)
#define SHA1_LITTLE_ENDIAN
#endif

/** \class
  * \brief generator which creates SHA1 instances from raw input data
  * \ingroup ODB
  * Each instance of a generator by default produces only one sha1 instance.
  * If you want to produce more, call reset() inbetween the different runs
  * \note performance tests indicate that this algorithm performs only 10% worse than
  * the highly optimized original git version.
  * \note Based on 100% free public domain implementation of the SHA-1 algorithm
  * by Dominik Reichl Web: http://www.dominik-reichl.de/
  */
class SHA1Generator
{
	private:
	union WorkspaceBlock
	{
		uint32 l[16];
	};
		
	public:
		SHA1Generator();
		~SHA1Generator() {}

		//! Prepare generator for new sha
		void reset();

		//! Update the hash value
		//! \param pbData location to read bytes from
		//! \param uLen number of bytes to read
		void update(const uchar* pbData, uint32 uLen);

		//! Finalize hash, call before using digest() method
		void finalize();

		//! \return 20 byte digest buffer
		inline const uchar* digest() const {
			return m_digest;
		}

		//! \return the hash produced so far as SHA1 instance
		//! \param sha1 destination of the 20 byte hash
		const void hash(SHA1& sha1) const {
			sha1 = m_digest;
		}

	private:
		// Private SHA-1 transformation
		inline void transform(const uchar* pBuffer);
		
		uint32 m_state[5];
		uint32 m_count[2];
		uint32 m_reserved0[1]; // Memory alignment padding
		uchar m_buffer[64];
		uchar m_digest[20];
		uint32 m_reserved1[3]; // Memory alignment padding
		
		// Member variables
		uint32 m_workspace[16];
		// Keep this indirection, as it forces proper aliasing
		// This is a side-effect of casting through a union, and speeds up
		// the whole thing by 25%
		WorkspaceBlock* m_block; // SHA1 pointer to the byte array above
};

GIT_HEADER_END
GIT_NAMESPACE_END

#endif // SHA1_GEN_H
