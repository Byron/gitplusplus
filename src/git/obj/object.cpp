#include <git/obj/object.hpp>
#include <cstdio>
#include <assert.h>

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

std::ostream& operator << (std::ostream& stream, const Actor& inst)
{
	stream << inst.name << " <" << inst.email << ">";
	return stream;
}

std::istream& operator >> (std::istream& stream, Actor& inst)
{
	stream >> inst.name;
	stream >> inst.email;
	inst.email = inst.email.substr(1, inst.email.size()-2);
	
	return stream;
}

std::ostream& operator << (std::ostream& stream, const ActorDate& inst)
{
	stream << static_cast<const Actor&>(inst);
	return stream << " " << inst.time << " " << inst.tz_offset;
}

std::istream& operator >> (std::istream& stream, ActorDate& inst)
{
	stream >> static_cast<Actor&>(inst);
	return stream >> inst.time >> inst.tz_offset;
}

std::ostream& operator << (std::ostream& stream, const TimezoneOffset& inst)
{
	char buf[17];
	std::sprintf(buf, "%04i", std::abs((int)inst.utz_offset));
	if (inst.utz_offset < 0){
		stream << "-";
	} else {
		stream << "+";
	}
	
	return stream << buf;
}

std::istream& operator >> (std::istream& stream, TimezoneOffset& inst)
{
	char c;
	stream >> c;
	stream >> inst.utz_offset;
	if (c == '-') {
		inst.utz_offset *= -1;
	}
	return stream;
}

GIT_NAMESPACE_END
