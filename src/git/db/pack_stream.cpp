#include <git/db/pack_stream.h>
#include <git/config.h>

#include <git/db/pack_file.h>
#include <iostream>
#include <cstring>

GIT_NAMESPACE_BEGIN

PackDevice::PackDevice(const PackFile& pack, uint32 entry)
    : m_pack(pack)
    , m_entry(entry)
    , m_type(ObjectType::None)
    , m_size(0)
{
}

PackDevice::PackDevice(const PackDevice &rhs)
	: parent_type(rhs)
    , m_pack(rhs.m_pack)
    , m_entry(rhs.m_entry)
    , m_type(rhs.m_type)
    , m_size(rhs.m_size)
    
{
	std::cerr << "PACK DEVICE: " << "INVOKED COPY CONSTRUCTOR" << std::endl;
	// don't copy the data, it will be recreated when it is first needed
}

PackDevice::~PackDevice()
{
}

uint64 PackDevice::msb_len(const char*& i) const
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

void PackDevice::info_at_offset(cursor_type& cur, PackInfo &info) const
{
	// 1 type + 8 bytes to encode 57bits of size (quite a lot) + max of 20 bytes for offset or ref + 1 bonus
	// Internally, the implementation is likely to provide more space
	cur.use_region(info.ofs, 1+8+20+1);
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

void PackDevice::assure_object_info(bool size_only) const
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
		info.ofs = ofs;
		info_at_offset(cur, info);
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
		
		if (size_only & has_delta_size) {
			break;
		}
		
		ofs = next_ofs;
	}// while we are not at the base
}

void PackDevice::unpack_object_recursive(cursor_type& cur, const PackInfo& info, const char* base) const
{
}

std::streamsize PackDevice::read(char_type* s, std::streamsize n)
{
	if (!m_data) {
		// Gather all deltas and store their header information. We do this recursively for small objects.
		// For larger objects, we first merge all deltas into one byte stream, to finally generate the final output
		// at once. This way, we do not need two possibly huge buffers in memory, but only one in a moderate size
		// for the merged delta, and the buffer for the final result. The basic source buffer will is memory mapped.
		assure_object_info(true);
		
		cursor_type cur = m_pack.cursor();
		if (m_size < 1024*1024) {
			PackInfo info;
			info.ofs = m_pack.index().offset(m_entry);
			
			const_cast<PackDevice*>(this)->info_at_offset(cur, info);
		} else {
			assert(false);
		}
	}
	assert(!!m_data);
	
	// Finally copy the requested amount of data
	return parent_type::read(s, n, m_data.get());
}

uint64 PackDevice::delta_target_size(cursor_type& cur, uint64 ofs) const
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
