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
	uint64 len = 0;
	const char* x = i;
	char c;
	do {
		c = *x;
		++x;
		len = (len << 7) + (c & 0x7f);
	} while (c & 0x80);
	i = x;
	return len;
}

void PackStream::info_at_offset(cursor_type& cur, uint64 ofs, PackInfo &info) const
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
		c = *i;
		++i;
		info.delta.ofs = c & 0x7f;
		while (c & 0x80) {
			c = *i;
			++i;
			info.delta.ofs += 1;
			info.delta.ofs = (info.delta.ofs << 7) + (c & 0x7f);
		}
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
			err.stream() << "Invalid type in pack - this really shouldn't be possible: " << (int)info.type;
			throw err;
	}
	}// end type switch
	
	info.rofs = i - cur.begin();
}

void PackStream::assure_object_info() const
{
	if (m_type != ObjectType::None) {
		return;
	}
	
	PackInfo info;
	uint64 ofs = m_pack.index().offset(m_entry);
	uint64 next_ofs;
	cursor_type cur = m_pack.cursor();
	bool has_delta_size = false;
	
	for(;;)
	{
		info_at_offset(cur, ofs, info);
		assert(info.type != PackedObjectType::Bad);
		
		// note: could make this a switch for maybe even more performance
		if (info.type == PackedObjectType::OfsDelta) {
			next_ofs = ofs - info.delta.ofs;
		} else if (info.type == PackedObjectType::RefDelta) {
			uint32 entry = m_pack.index().sha_to_entry(info.delta.key);
			assert(entry != PackIndexFile::hash_unknown);
			next_ofs = m_pack.index().offset(entry);
		} else {
			m_type = static_cast<ObjectType>(info.type);
			// if we are not a delta, we have to obtain the original objects size
			if (!has_delta_size) {
				m_size = info.size;
			}
			break;
		}// handle object type
		
		if (!has_delta_size) {
			m_size = delta_target_size(cur, ofs + info.rofs);
			has_delta_size = true;
		}
		
		ofs = next_ofs;
	}// while we are not at the base
}

uint64 PackStream::delta_target_size(cursor_type& cur, uint64 ofs) const
{
	unsigned char delta_header[20];	// can handle two 64 bit numbers
	gtl::zlib_decompressor zstream;
	zstream.next_out = delta_header;
	zstream.avail_out = sizeof(delta_header);
	int status;
	do {
		cur.use_region(ofs, 20);	// just pick a static size to map which is small to prevent unnecessary mapping
		zstream.next_in = const_cast<uchar*>(reinterpret_cast<const uchar*>(cur.begin()));
		zstream.avail_in = cur.size();
		status = zstream.decompress(true);
		ofs += zstream.next_in - const_cast<uchar*>(reinterpret_cast<const uchar*>(cur.begin()));
	} while ((status == Z_OK || status == Z_BUF_ERROR) && 
	         zstream.total_out < sizeof(delta_header));
	if (status != Z_STREAM_END && zstream.total_out != sizeof(delta_header)) {
		throw gtl::zlib_error(status);
	}
	
	const char* data = reinterpret_cast<char*>(&delta_header[0]);
	msb_len(data);				// base offset - ignore
	return msb_len(data);		// target offset
}

GIT_NAMESPACE_END
