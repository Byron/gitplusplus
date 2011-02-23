#include "git/obj/commit.h"
#include <git/config.h>			// for doxygen

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/copy.hpp>


GIT_NAMESPACE_BEGIN
		
namespace io = boost::iostreams;

const string Commit::default_encoding("UTF-8");
		
//! \cond
		
// Using assignment for doxygen - lets hope the compiler optimizes thiss
const string t_tree("tree");
const string t_parent("parent");
const string t_author("author");
const string t_committer("committer");
const string t_encoding("encoding");

Commit::Commit() 
	: Object(Object::Type::Commit)
	, m_tree_key(key_type::null)
	, m_encoding(default_encoding)
{}

bool Commit::operator == (const Commit& rhs) const
{
	return 
			m_tree_key == rhs.tree_key() &&
			m_parent_keys == rhs.m_parent_keys && 
			m_committer == rhs.committer() &&
			m_author == rhs.author() && 
			m_message == rhs.message();
}
		
git_basic_ostream& operator << (git_basic_ostream& stream, const Commit& inst) 
{
	// yes, all shas are written in hex
	auto parent_end = inst.parent_keys().end();
	
	stream << t_tree << ' ' << inst.tree_key() << std::endl;
	for (auto i = inst.parent_keys().begin(); i < parent_end; ++i) {
		stream << t_parent << ' ' << *i << std::endl;
	}
	stream << t_author << ' ' << inst.author() << std::endl;
	stream << t_committer << ' ' << inst.committer() << std::endl;
	
	if (inst.encoding() != Commit::default_encoding) {
		stream << t_encoding << ' ' << inst.encoding() << std::endl;
	}
	
	// single newline as message separator
	stream << std::endl << inst.message();
	
	return stream;
}

git_basic_istream& operator >> (git_basic_istream& stream, Commit& inst) 
{
	string tmp;
	tmp.reserve(32);
	// used to skip single character - could use seek, but lets try to only use 
	// simple gets as our own streams might not implement seeking
	char c;
	
	// tree key
	stream >> tmp;
	if (tmp != t_tree) {
		throw DeserializationError();
	}
	stream.get(c);			// space
	stream >> inst.tree_key();
	
	// parents
	for (stream >> tmp; tmp == t_parent; stream >> tmp) {
		Commit::key_type key;
		stream.get(c);		// space
		stream >> key;
		inst.parent_keys().push_back(std::move(key));
	}
	
	// author
	if (tmp != t_author) {
		throw DeserializationError();
	}
	
	// space
	stream.get(c);
	stream >> inst.author();
	
	// committer
	stream >> tmp;
	if (tmp != t_committer) {
		throw DeserializationError();
	}
	
	stream.get(c);				// space
	stream >>inst.committer();
	stream.get(c);				// newline
	
	// encoding
	stream.get(c);				// indicator
	if (c == 'e') {
		stream >> tmp;
		stream >> inst.encoding();
		stream.get(c);			// newline after encoding
		stream.get(c);			// newline to separate message
	} else {
		if (c != '\n') {
			throw DeserializationError();
		}
	}
	
	// message
	// read until eof
	io::stream<io::back_insert_device<string> > dest(inst.message());	
	io::copy(stream, dest);
	
	return stream;
}

//! \endcond
		
GIT_NAMESPACE_END
