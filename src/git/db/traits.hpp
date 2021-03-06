#ifndef GIT_TRAITS_H
#define GIT_TRAITS_H

#include <git/config.h>
#include <git/db/sha1.h>
#include <git/db/sha1_gen.h>

GIT_HEADER_BEGIN
GIT_NAMESPACE_BEGIN

class Object;
union MultiObject;

/** \brief Enumeration specifying codes representing object type known to git
  */
enum class ObjectType : uchar
{
	None,
	Blob,
	Tree,
	Commit,
	Tag
};

struct git_object_policy_traits
{
	//! Type allowing to classify the stored object
	typedef ObjectType object_type;
	//! None type for objects
	static const object_type null_object_type = object_type::None;
	//! max size of serialized objects
	typedef uint64_t size_type;
	//! use unsigned bytes as general storage
	typedef char char_type;
	//! Type used to return values by reference
	typedef MultiObject& output_reference_type;
	//! Type used to provide unpacked object values.
	typedef Object& input_reference_type;
};


/** \brief Configures additonal types which are not used in the policy, as well as the policy itself
  */
struct git_object_traits_base : public git_object_policy_traits
{
	//! Hash generator to produce keys.
	typedef SHA1Generator hash_generator_type;	
	//! Using SHA1 as key
	typedef SHA1 key_type;
};

GIT_NAMESPACE_END
GIT_HEADER_END

#endif // GIT_TRAITS_H
