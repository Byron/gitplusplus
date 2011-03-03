#include "git/obj/tree.h"
#include <git/config.h>		// for doxygen

GIT_NAMESPACE_BEGIN

typedef git_basic_ostream::char_type char_type;
typedef Tree::Element::mode_type mode_type;
		
Tree::Tree()
	: Object(Object::Type::Tree) 
{}

git_basic_ostream& operator << (git_basic_ostream& stream, const Tree& inst) 
{
	const size_t mbuflen = 6;						
	char_type mbuf[mbuflen];						// buffer for 6 mode literals
	const mode_type mask = 7;						// 3 bits
	
	auto end = inst.elements().end();
	for (auto i = inst.elements().begin(); i != end; ++i)
	{
		// serialize mode as octal string
		const mode_type mode = i->second.mode;
		// if the first mode character would be 0, it gets truncated. This is reflected by an offset
		const uint ofs = (mode >> ((mbuflen-1)*3) & mask) == 0 ? 1 : 0;
		for (uint m = 0; m < mbuflen-ofs; ++m) {
			mbuf[mbuflen-(m+1+ofs)] = (char_type)(((mode >> (m*3)) & mask) + '0');
		}
		
		stream.write(mbuf, mbuflen-ofs);
		stream << ' ' << i->first << '\0';
		stream.write(i->second.key.bytes(), Tree::key_type::hash_len);
		
	}// for each tree element to write
	return stream;
}

git_basic_istream& operator >> (git_basic_istream& stream, Tree& inst)
{
	char_type c;
	string name;		// reuse possibly reserved memory
	name.reserve(64);	// optimize allocation
	
	while (stream.good())
	{
		std::cerr << "reading tree item" << std::endl;
		// parse mode
		try {
			stream.get(c);
			// see whether the stream is depleted (case if we don't throw exceptions)
			if (!stream.good()) {
				break;
			}
		} catch (std::ios_base::failure) {
			// if we are at the end, abort this
			// It depends on the configuration whether this throws
			break;
		}
		std::cerr << "got char: " << (uint)c << " eof = " << stream.eof() << std::endl;
		name.clear();
		Tree::Element elm;	// we will move the element in
		elm.mode = 0;

		for(; c != ' '; stream.get(c)) {
			std::cerr << c ;
			elm.mode = (elm.mode << 3) + (c - '0');
		}
		// parse name
		std::getline(stream, name, '\0');
		
		// parse hash
		stream.read(elm.key.bytes(), Tree::key_type::hash_len);
		
		// add new element
		std::cerr << "added tree item" << std::endl;
		inst.elements().insert(Tree::map_type::value_type(name, std::move(elm)));
	}// while stream is available
	
	if (inst.elements().size() == 0) {
		DeserializationError err;
		err.stream() << "Couldn't deserialize any tree element";
		throw err;
	}
	
	return stream;
}
		

GIT_NAMESPACE_END
