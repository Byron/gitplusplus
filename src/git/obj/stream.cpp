#include <git/obj/stream.h>
#include <git/config.h>

#include <cstdio>
#include <assert.h>
#include <string>

GIT_NAMESPACE_BEGIN

git_basic_istream& operator >> (git_basic_istream& stream, Object::Type& type) 
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

git_basic_ostream& operator << (git_basic_ostream& stream, Object::Type type) 
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

git_basic_ostream& operator << (git_basic_ostream& stream, const Actor& inst)
{
	if (inst.name.size() == 0){
		throw DeserializationError();
	}
	stream << inst.name << " <" << inst.email << '>';
	return stream;
}

git_basic_istream& operator >> (git_basic_istream& stream, Actor& inst)
{
	std::string buf;
	std::getline(stream, buf, '>');
	std::string::size_type i;
	for (i = 0; i < buf.size() && buf[i] != '<'; ++i);
	if (i == buf.size()) {
		throw DeserializationError();
	}
	
	inst.name = buf.substr(0, i-1);		// skip trailing space
	inst.email = buf.substr(i+1);		// skip trailing >
	
	return stream;
}

git_basic_ostream& operator << (git_basic_ostream& stream, const ActorDate& inst)
{
	stream << static_cast<const Actor&>(inst);
	return stream << ' ' << inst.time << ' ' << inst.tz_offset;
}

git_basic_istream& operator >> (git_basic_istream& stream, ActorDate& inst)
{
	stream >> static_cast<Actor&>(inst);
	return stream >> inst.time >> inst.tz_offset;
}

git_basic_ostream& operator << (git_basic_ostream& stream, const TimezoneOffset& inst)
{
	char buf[17];
	std::sprintf(buf, "%04i", std::abs((int)inst.utz_offset));
	if (inst.utz_offset < 0){
		stream << '-';
	} else {
		stream << '+';
	}
	
	return stream << buf;
}

git_basic_istream& operator >> (git_basic_istream& stream, TimezoneOffset& inst)
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

