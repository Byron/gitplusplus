#ifndef GTL_ODB_GENERATOR_HPP
#define GTL_ODB_GENERATOR_HPP

#include <gtl/config.h>
#include <exception>

GTL_HEADER_BEGIN
GTL_NAMESPACE_BEGIN


/** \brief basic error for all hash related issues.
  * \ingroup ODBException
  */
class hash_generator_error: public std::exception
{
	virtual const char* what() const throw() {
		return "general generator error";
	}
};

/** Exception specifying the generator is in an invalid state. Usually this happens
  * if you continue feeding it, after it finalized the hash already. Or of some methods
  * are called several times.
  * \ingroup ODBException
  */
class bad_state : public hash_generator_error
{
	virtual const char* what() const throw() {
		return "invalid call order would have caused an invalid state";
	}
};

/** \brief a type which generates hashes from character streams
  * \ingroup ODBUtil
  * \tparam Hash type which represents and encapsulates a hash value. It must 
  * be copy-constructible/assignable using the return value of digest()
  * \tparam Char type of the character sequence
  * \tparam SizeType type specifying an amount of characters of type Char
  */
template <class Hash, class Char, class SizeType>
class hash_generator
{
public:
	typedef Hash hash_type;
	typedef Char char_type;
	typedef SizeType size_type;
	
public:
	//! Prepare generator for new hash to be generated
	void reset() throw();
	
	//! Update the hash value
	//! \param pdata location to read characters from
	//! \param dlen number of characters to read
	//! \throw bad_state
	void update(const char_type* pdata, size_type dlen);
	
	//! Finalize hash, called automatically before using digest() method the first time
	//! The user may, but is not required to make this call automatically.
	//! Must only be called once after a reset(),
	//! \throw bad_state
	void finalize();
	
	//! \return digest buffer which is the generated hash
	const char_type* digest() throw();
	
	//! \return the hash produced so far as SHA1 instance
	//! \param sha1 destination of the 20 byte hash
	//! \note if called once, you need to call reset() if you want to update the instance with new characters
	//! \note ideally the hash_type has a move-constructor or a move assignment constructor. Compilers 
	//! may otherwise be able to optimize the construction if your hash is assigned directly.
	inline hash_type hash() throw();
	
	//! obtain the hash produced so far
	//! \param sha1 destination for the hash
	//! \note if called once, you need to call reset to use the instance again
	void hash(hash_type& outHash) throw();
};

GTL_NAMESPACE_END
GTL_HEADER_END

#endif // GTL_ODB_GENERATOR_HPP
