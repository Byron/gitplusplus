#include <git/obj/object.hpp>


GIT_NAMESPACE_BEGIN

std::istream& operator >> (std::istream& stream, Object::Type& type) 
{
	std::string s;
	stream >> s;
	if (s.size() < 3) {
		type = Object::Type::None;
		return stream;
	}
	
	if (s == "none") {
		type = Object::Type::None;
	} else if (s == "blob") {
		type = Object::Type::Blob;
	} else if(s == "tree") {
		type = Object::Type::Tree;
	} else if(s == "commit") {
		type = Object::Type::Commit;
	} else if(s == "tag") {
		type = Object::Type::Tag;
	}
	return stream;
}

std::ostream& operator << (std::ostream& stream, Object::Type type) 
{
	switch(type)
	{
		case Object::Type::None:
			{
				stream << "none";
				break;
			}
		case Object::Type::Blob:
			{
				stream << "blob";
				break;
			}
		case Object::Type::Tree:
			{
				stream << "tree";
				break;
			}
		case Object::Type::Commit:
			{
				stream << "commit";
				break;
			}
		case Object::Type::Tag:
			{
				stream << "tag";
				break;
			}
		default:
			{
				stream << "unknown_object_type";
				break;
			}
	}// end type switch
	
	return stream;
}


GIT_NAMESPACE_END
