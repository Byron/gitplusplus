#include "git/obj/tag.h"
#include <git/config.h>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/copy.hpp>
#include <string>

GIT_NAMESPACE_BEGIN

Tag::Tag()
	: Object(Object::Type::Tag)
	, m_obj_type(Object::Type::None)
	, m_obj_hash(key_type::null)
{
}

const string t_object("object");
const string t_type("type");
const string t_tag("tag");
const string t_tagger("tagger");

git_basic_ostream& operator << (git_basic_ostream& stream, const git::Tag& tag)
{
	stream << t_object << " " << tag.object_key() << std::endl;
	stream << t_type << " " <<  tag.object_type() << std::endl;
	stream << t_tag << " " << tag.name() << std::endl;
	stream << t_tagger << " " << tag.actor() << std::endl;
	// empty line only given if we have a message
	if (tag.message().size()) {
		stream << std::endl << tag.message();
	}
	return stream;
}

git_basic_istream& operator >> (git_basic_istream& stream, git::Tag& tag) 
{
	std::string tmp;
	stream >> tmp;
	
	// OBJECT
	if (tmp != t_object) {
		throw git::TagDeserializationError();
	}
	//! \todo revise this, as it could be problematic for other implementations which 
	//! are more than just a byte array.
	stream >> tmp;
	new (&tag.object_key()) typename git::Tag::key_type(tmp);	
	
	// OBJECT TYPE
	stream >> tmp;
	if (tmp != t_type) {
		throw git::TagDeserializationError();
	}
	stream >> tag.object_type();
	if (tag.object_type() == git::Object::Type::None && tag.object_key() != git::Tag::key_type::null) {
		throw git::TagDeserializationError();
	}
	
	// TAG NAME 
	stream >> tmp;
	if (tmp != t_tag) {
		throw git::TagDeserializationError();
	}
	
	stream.seekg(1, std::ios_base::cur); // skip whitespace
	std::getline(stream, tag.name(), '\n');
	
	// TAGGER
	stream >> tmp;
	if (tmp != t_tagger){
		throw git::TagDeserializationError();
	}
	stream.seekg(1, std::ios_base::cur); // skip whitespace
	stream >> tag.actor();

	// read \n at end of tagger line
	char c[1];
	stream.readsome(c, 1);
	
	// MESSAGE	
	if (!stream.eof()) {
		// read all the rest, which should be the message
		// read newline between tagger and message
		stream.readsome(c, 1);
		
		boost::iostreams::back_insert_device<std::string> msgstream(tag.message());
		boost::iostreams::copy(stream, msgstream);
	}
	
	return stream;	
}

/*Object::size_type Tag::size() const 
{
	return t_object.size() + t_type.size() + t_tag.size() + t_tagger.size()	// all header tag sizes
			+ key_type::hash_len * 2		// hex hash len
			+ m_name.size()
			// + name len of object type
			+ 1*4							// 1 space after header tag
			+ 1*4							// 1 newline after single tag line
			// todo: actordate size, + individual sizes
			+ m_message.size() ? m_message.size() + 1 : 0;
}*/


GIT_NAMESPACE_END
