#include <git/obj/object.hpp>

#include <sstream>
#include <git/obj/tag.h>
#include <git/obj/blob.h>
#include <git/obj/commit.h>
#include <git/obj/tree.h>

GIT_NAMESPACE_BEGIN

class TypeError : public ObjectError
{
	const char* what() const throw() {
		return "Invalid object type";
	}
};
		
Object::size_type Object::size(git_basic_ostream* pstream) const
{
	std::ostringstream s;
	std::basic_ostream<git_object_policy_traits::char_type>* ps(&s);
	if (pstream != nullptr) {
		ps = pstream;
	}
	
	switch(m_type)
	{
		case Object::Type::Blob: { *ps << static_cast<const Blob&>(*this); break; }
		case Object::Type::Tag: { *ps << static_cast<const Tag&>(*this);  break; }
		case Object::Type::Commit: { *ps << static_cast<const Commit&>(*this); break; }
		case Object::Type::Tree: { *ps << static_cast<const Tree&>(*this); break; }
		default:
		{
			throw TypeError();
		}
	}// end type switch
	
	return (size_type)ps->tellp();
}

GIT_NAMESPACE_END
