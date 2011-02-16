#ifndef SHA1_GEN_H
#define SHA1_GEN_H

#include <stddef.h>
#include <git/db/sha1.h>
#include <git/config.h>
#include <gtl/db/hash_generator_filter.hpp>
#include <gtl/db/hash_generator.hpp>
#include <exception>
#include <assert.h>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

// You can define the endian mode in your files without modifying the SHA-1
// source files. Just #define SHA1_LITTLE_ENDIAN or #define SHA1_BIG_ENDIAN
// in your files, before including the SHA1.h header file. If you don't
// define anything, the class defaults to little endian.
#if !defined(SHA1_LITTLE_ENDIAN) && !defined(SHA1_BIG_ENDIAN)
#define SHA1_LITTLE_ENDIAN
#endif


class BadSHA1GenState : public gtl::bad_state
{
public:
	virtual const char* what() const throw(){
		return "Cannot update generator once hash() or digest() was called";
	}
};


/** \brief generator which creates SHA1 instances from raw input data
  * \ingroup ODB
  * Each instance of a generator by default produces only one sha1 instance.
  * If you want to produce more, call reset() inbetween the different runs
  * \note performance tests indicate that this algorithm performs only 10% worse than
  * the highly optimized original git version.
  * \note Based on 100% free public domain implementation of the SHA-1 algorithm
  * by Dominik Reichl Web: http://www.dominik-reichl.de/
  */
class SHA1Generator : public gtl::hash_generator<SHA1, uchar, uint32>
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
		//! \note will return 0-sha if update was not yet called
		void reset() throw();

		//! Update the hash value
		//! \param pbData location to read bytes from
		//! \param uLen number of bytes to read
		void update(const uchar* pbData, uint32 uLen) throw(gtl::bad_state);
		
		//! Finalize hash, called before using digest() method the first time
		//! The user may, but is not required to make this call automatically.
		//! Once it is called, it will have no effect anymore.
		void finalize() throw(gtl::bad_state);

		//! \return 20 byte digest buffer
		inline const uchar* digest() throw() {
			if (m_update_called & (!m_finalized)){
				finalize();
				assert(m_finalized);
			}
			return m_digest;
		}
		
		//! \return the hash produced so far as SHA1 instance
		//! \param sha1 destination of the 20 byte hash
		//! \note if called once, you need to call reset to use the instance again
		void hash(SHA1& sha1) throw() {
			sha1 = digest();
		}
		
		inline hash_type hash() throw() {
			return SHA1(digest());
		}

	private:
		
		// Private SHA-1 transformation
		inline void transform(const uchar* pBuffer);
		
		uint32 m_state[5];
		uint32 m_count[2];
		uint32 m_finalized; // Memory alignment padding - used as flag too
		uchar m_buffer[64];
		uchar m_digest[20];
		uint32 m_update_called;// memory alignment and flag to indicate update was called
		
		// Member variables
		uint32 m_workspace[16];
		// Keep this indirection, as it forces proper aliasing
		// This is a side-effect of casting through a union, and speeds up
		// the whole thing by 25%
		WorkspaceBlock* m_block; // SHA1 pointer to the byte array above
};


/** Declares a filter to be used within the stream framework
  */
class SHA1Filter : public gtl::generator_filter<SHA1, SHA1Generator>
{
public:
};

GIT_HEADER_END
GIT_NAMESPACE_END

#endif // SHA1_GEN_H
