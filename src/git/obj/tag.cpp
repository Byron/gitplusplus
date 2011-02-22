#include "git/obj/tag.h"
#include <git/config.h>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/copy.hpp>

GIT_NAMESPACE_BEGIN

Tag::Tag()
	: Object(Object::Type::Tag)
	, m_obj_type(Object::Type::None)
	, m_obj_hash(key_type::null)
{
}

git_basic_ostream& operator << (git_basic_ostream& stream, const git::Tag& tag)
{
	stream << "object " << tag.object_key() << std::endl;
	stream << "type " << tag.object_type() << std::endl;
	stream << "tag " << tag.name() << std::endl;
	stream << "tagger " << tag.actor() << std::endl;
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
	if (tmp != "object") {
		throw git::TagDeserializationError();
	}
	//! \todo revise this, as it could be problematic for other implementations which 
	//! are more than just a byte array.
	stream >> tmp;
	new (&tag.object_key()) typename git::Tag::key_type(tmp);	
	
	// OBJECT TYPE
	stream >> tmp;
	if (tmp != "type") {
		throw git::TagDeserializationError();
	}
	stream >> tag.object_type();
	if (tag.object_type() == git::Object::Type::None) {
		throw git::TagDeserializationError();
	}
	
	
	// TAG NAME 
	stream >> tmp;
	if (tmp != "tag") {
		throw git::TagDeserializationError();
	}
	
	stream >> tag.name();
	
	// TAGGER
	stream >> tmp;
	if (tmp != "tagger"){
		throw git::TagDeserializationError();
	}
	
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

GIT_NAMESPACE_END
