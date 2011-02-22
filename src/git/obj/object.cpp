#include <git/obj/object.hpp>

#include <sstream>
#include <git/obj/tag.h>
#include <git/obj/blob.h>
#include <git/obj/commit.h>
#include <git/obj/tree.h>

GIT_NAMESPACE_BEGIN
		
Object::size_type Object::size() const
{
	std::ostringstream s;
	switch(m_type) 
	{
		case Object::Type::Blob: { s << static_cast<const Blob&>(*this); break; }
		case Object::Type::Tag: { s << static_cast<const Tag&>(*this);  break; }
		case Object::Type::Commit: { s << static_cast<const Commit&>(*this); break; }
		case Object::Type::Tree: { s << static_cast<const Tree&>(*this); break; }
		default:
		{
			ObjectError err;
			err.stream() << "Invalid object type for size() computation: " << m_type;
			throw err;
		}
	}// end type switch
	
	return (size_type)s.tellp();
}

GIT_NAMESPACE_END
