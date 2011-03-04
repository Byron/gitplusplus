#ifndef GIT_ODB_LOOSE_H
#define GIT_ODB_LOOSE_H

#include <git/config.h>
#include <git/db/policy.hpp>
#include <gtl/db/odb_loose.hpp>
#include <git/db/util.hpp>

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

/** \todo still to be implemented/defined
  */
struct git_loose_odb_policy : public gtl::odb_loose_policy
{
	template <class CharType, class ObjectType, class SizeType>
	size_t parse_header(CharType* buf, size_t buflen, ObjectType& type, SizeType& size)
	{
		boost::iostreams::stream<boost::iostreams::basic_array_source<CharType> > stream(buf, buflen);
		stream >> type;
		stream >> size;
		return (size_t)stream.tellg() + 1;	// includes terminating 0
	}
	
	template <class StreamType, class ObjectType, class SizeType>
	void write_header(StreamType& stream, const ObjectType type, const SizeType size)
	{
		typename git_object_policy_traits::char_type buf[32];
		uchar nb = loose_object_header(buf, type, size);
		// maybe a stream wrapper could be more efficient, who knows
		for (uchar i = 0; i < nb; ++i) {
			stream << buf[i];
		}
	}
};

/** \brief configures the loose object database to be conforming with a default 
  * git repository
  */
struct git_loose_odb_traits : public gtl::odb_loose_traits<git_object_traits>
{
	//! override default policy for our implementation
	typedef git_loose_odb_policy policy_type;
};

/** \ingroup ODB
  * \brief git-like implementation of the loose object database
  */
class LooseODB : public gtl::odb_loose<git_object_traits, git_loose_odb_traits>
{
public:
	typedef git_loose_odb_traits::path_type path_type;
	
public:
    LooseODB(const path_type& root);
};


GIT_NAMESPACE_END
GIT_HEADER_END
#endif // GIT_ODB_LOOSE_H
