#include <git/db/pack_stream.h>
#include <git/config.h>

#include <git/db/pack_file.h>
#include <cstring>

GIT_NAMESPACE_BEGIN

PackStream::PackStream(const PackFile& pack, uint32 entry)
    : m_pack(pack)
    , m_entry(entry)
    , m_type(ObjectType::None)
    , m_size(0)
{
	
}

uint64 PackStream::msb_len(const char*& i) const
{
	uint64 size = 0;
	uint shift = 0;
	char c;
	do {
		c = *i++;
		size += (c & 0x7f) << shift;
		shift += 7;
	} while (c & 0x80);
	
	return size;
}

void PackStream::info_at_offset(cursor& cur, uint64 ofs, PackInfo &info) const
{
	// 1 type + 8 bytes to encode 57bits of size (quite a lot) + max of 20 bytes for offset or ref + 1 bonus
	// Internally, the implementation is likely to provide more space
	cur.use_region(ofs, 1+8+20+1);
	assert(cur.is_valid());
	const char* i = cur.begin();
	char c = *i++;
	info.type = static_cast<PackedObjectType>((c >> 4) & 7);
	
	info.size = c & 15;
	uint shift = 4;
	while(c & 0x80) {
		assert(i < cur.end());
		c = *i;
		++i;
		info.size += (c & 0x7f) << shift;
		shift += 7;
	}
	
	switch(info.type) 
	{
	case PackedObjectType::Commit:
	case PackedObjectType::Tree:
	case PackedObjectType::Blob:
	case PackedObjectType::Tag:
	{
		break;
	}
	case PackedObjectType::OfsDelta:
	{
		info.ofs = msb_len(i);
		break;
	}
	case PackedObjectType::RefDelta:
	{
		std::memcpy(info.delta.key.bytes(), i, key_type::hash_len);
		i += key_type::hash_len;
		break;
	}
	default: 
	{
			PackParseError err;
			err.stream() << "Invalid type in pack - this really shouldn't be possible";
			throw err;
	}
	}// end type switch
	
	info.ofs = i - cur.begin();
}

void PackStream::assure_object_info() const
{
	if (m_type != ObjectType::None) {
		return;
	}
	
	PackInfo info;
	uint64 ofs = m_pack.index().offset(m_entry);
	cursor cur = m_pack.pack().cursor();
	
	for(;;)
	{
		info_at_offset(cur, ofs, info);
		assert(info.type != PackedObjectType::Bad);
		
		// note: could make this a switch for maybe even more performance
		if (info.type == PackedObjectType::OfsDelta) {
			ofs = info.delta.ofs;
		} else if (info.type == PackedObjectType::RefDelta) {
			uint32 entry = m_pack.index().sha_to_entry(info.delta.key);
			assert(entry != PackIndexFile::hash_unknown);
			ofs = m_pack.index().offset(entry);
		} else {
			m_type = static_cast<ObjectType>(info.type);
			// TODO: The actual size is in the first delta !
			m_size = info.size;
			break;
		}// handle object type
	}// while we are not at the base
}


GIT_NAMESPACE_END
