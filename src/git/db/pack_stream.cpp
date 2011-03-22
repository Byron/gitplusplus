#include <git/db/pack_stream.h>
#include <git/config.h>

#include <git/db/pack_file.h>
#include <iostream>
#include <cstring>

GIT_NAMESPACE_BEGIN


//! @{ Utilties

//! Decompress all bytes from the cursor (it must be set to the correct first byte)
//! and continue decompression until the end of the stream or until our destination buffer
//! is full.
//! \param cur cursor whose offset is pointing to the start of the stream we should decompress
//! \param max_input_chunk_size if not 0, the amount of bytes we should put in per iteration. Use this if you only have a few
//! input bytes  and don't want it to decompress more than necessary
static void decompress_some(PackDevice::cursor_type& cur, PackDevice::char_type* dest, uint64 nb, size_t max_input_chunk_size = 0)
{
	assert(cur.is_valid());
	
	int status = Z_OK;
	gtl::zlib_decompressor z;
	stream_offset ofs = cur.ofs_begin();
	
	// As zip can only take 32 bit numbers, we must chop up the operation
	const uint64 chunk_size = 1024*1024*16;
	static_assert(chunk_size <= std::numeric_limits<decltype(z.avail_out)>::max(), "chunk too large");
	
	for (uint64 bd = 0; status != Z_STREAM_END && bd < nb; bd += chunk_size)
	{
		z.next_out = reinterpret_cast<uchar*>(dest + std::min(bd, nb));
		z.avail_out = static_cast<uint>(std::min(chunk_size, nb));
		do {
			cur.use_region(ofs, chunk_size / 2);
			if (!cur.is_valid()) {
				ParseError err;
				err.stream() << "Failed to map pack at offset " << ofs;
				throw err;
			}
			z.next_in = const_cast<uchar*>(reinterpret_cast<const uchar*>(cur.begin()));
			z.avail_in = max_input_chunk_size ? std::min(cur.size(), max_input_chunk_size) : cur.size();
			status = z.decompress(true);
			ofs += z.next_in - reinterpret_cast<const uchar*>(cur.begin());
		} while ((status == Z_OK || status == Z_BUF_ERROR) && z.avail_out);
		
		switch(status) 
		{
			case Z_ERRNO:
			case Z_STREAM_ERROR:
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
			case Z_VERSION_ERROR:
			{
				throw gtl::zlib_error(status, "ZLib stream error");
			}
		}// end switch
	}// for each chunk
	
	// Most importantly, we produce the amount of bytes we wanted. BUF ERRORs occour
	// if there is not enough output (or input)
	if (z.total_out != nb && (status != Z_STREAM_END && status != Z_BUF_ERROR)) {
		throw gtl::zlib_error(status, "Failed to produce enough bytes during decompression");
	}
}

//! @} end utilities


PackDevice::PackDevice(const PackFile& pack, uint32 entry)
    : m_pack(pack)
    , m_entry(entry)
    , m_type(ObjectType::None)
{
}

PackDevice::PackDevice(const PackDevice &rhs)
	: parent_type(rhs)
    , m_pack(rhs.m_pack)
    , m_entry(rhs.m_entry)
    , m_type(rhs.m_type)
    
{
	std::cerr << "PACK DEVICE: " << "INVOKED COPY CONSTRUCTOR" << std::endl;
	// don't copy the data, it will be recreated when it is first needed
}

PackDevice::~PackDevice()
{
}

uint64 PackDevice::msb_len(const char_type*& i) const
{
	uint64 len = 0;
	const char_type* x = i;
	char_type c;
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
	const char_type* i = cur.begin();
	char_type c = *i++;
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
	info.ofs = m_pack.index().offset(m_entry);
	uint64 next_ofs;
	cursor_type cur = m_pack.cursor();
	bool has_delta_size = false;
	
	for(;;)
	{
		info_at_offset(cur, info);
		assert(info.type != PackedObjectType::Bad);
		
		// note: could make this a switch for maybe even more performance
		if (info.type == PackedObjectType::OfsDelta) {
			next_ofs = info.ofs - info.delta.ofs;
		} else if (info.type == PackedObjectType::RefDelta) {
			uint32 entry = m_pack.index().sha_to_entry(info.delta.key);
			assert(entry != PackIndexFile::hash_unknown);
			next_ofs = m_pack.index().offset(entry);
		} else {
			m_type = static_cast<ObjectType>(info.type);
			// if we are not a delta, we have to obtain the original objects size
			if (!has_delta_size) {
				const_cast<PackDevice*>(this)->m_size = info.size;
			}
			break;
		}// handle object type
		
		if (!has_delta_size) {
			delta_size(cur, info.ofs + info.rofs,
			           const_cast<PackDevice*>(this)->m_size, 
			           const_cast<PackDevice*>(this)->m_size);	 // target size assigned last
			has_delta_size = true;
		}
		
		if (size_only & has_delta_size) {
			break;
		}
		
		info.ofs = next_ofs;
	}// while we are not at the base
	
	// assure our base knows we may read
	const_cast<PackDevice*>(this)->m_nb = m_size;
}

void PackDevice::apply_delta(const char_type* base, char_type* dest, const char_type* delta, size_t deltalen) const
{
	const uchar* data = reinterpret_cast<const uchar*>(delta);
	const uchar* const dend = data + deltalen;
	
	while (data < dend)
	{
		const uchar cmd = *data++;
		if (cmd & 0x80) 
		{
			unsigned long cp_off = 0, cp_size = 0;
			if (cmd & 0x01) cp_off = *data++;
			if (cmd & 0x02) cp_off |= (*data++ << 8);
			if (cmd & 0x04) cp_off |= (*data++ << 16);
			if (cmd & 0x08) cp_off |= ((unsigned) *data++ << 24);
			if (cmd & 0x10) cp_size = *data++;
			if (cmd & 0x20) cp_size |= (*data++ << 8);
			if (cmd & 0x40) cp_size |= (*data++ << 16);
			if (cp_size == 0) cp_size = 0x10000;
			
			memcpy(dest, base + cp_off, cp_size); 
			dest += cp_size;
			
		} else if (cmd) {
			memcpy(dest, data, cmd);
			dest += cmd;
			data += cmd;
		} else {                                                                               
			PackParseError err;
			err.stream() << "Encountered an unknown data operation";
			throw err;
		}// end handle opcode
	}// end while there are data opcodes
	
}

char_type* PackDevice::unpack_object_recursive(cursor_type& cur, const PackInfo& info, uint64& out_size) const
{
	assert(info.type != PackedObjectType::Bad);
	typedef boost::scoped_array<char_type> pchar_type;
	
	//! NOTE: We allocate the memory late to prevent excessive memory use during recursion
	char_type* dest = nullptr;
	
	switch (info.type)
	{
	case PackedObjectType::Commit:
	case PackedObjectType::Tree:
	case PackedObjectType::Blob:
	case PackedObjectType::Tag:
	{
		dest = new char_type[info.size];
		out_size = info.size;
		// base object, just decompress the stream  and return the information
		// Size doesn't really matter as the window will slide
		cur.use_region(info.ofs+info.rofs, info.size/2);
		decompress_some(cur, dest, info.size);
		break;
	}
	case PackedObjectType::OfsDelta:
	case PackedObjectType::RefDelta:
	{
		PackInfo next_info;
		if (info.type == PackedObjectType::OfsDelta) {
			next_info.ofs = info.ofs - info.delta.ofs;
		} else {
			uint32 entry = m_pack.index().sha_to_entry(info.delta.key);
			assert(entry != PackIndexFile::hash_unknown);
			next_info.ofs = m_pack.index().offset(entry);
		}
		
		// obtain base and decompress the delta to apply it
		info_at_offset(cur, next_info);
		pchar_type base_data(unpack_object_recursive(cur, next_info, out_size));	// base memory
		
		pchar_type ddata(new char_type[info.size]);		// delta memory
		char_type* pddata = ddata.get();		//!< pointer to delta data
		const char_type* cpddata = ddata.get();
		cur.use_region(info.ofs+info.rofs, info.size/2);
		decompress_some(cur, pddata, info.size);
		
		auto base_size = msb_len(cpddata);
		if (base_size != out_size) {
			ParseError err;
			err.stream() << "Base buffer length didn't match the parsed information: " << out_size << " != " << base_size;
			throw err;
		}
		out_size = msb_len(cpddata);
		
		// Allocate memory to keep the destination and apply delta
		dest = new char_type[out_size];
		apply_delta(base_data.get(), dest, cpddata, info.size - (cpddata - pddata));
		
		break;
	}
	default: 
	{
		throw ParseError();
	}
	};// end type switch
	
	return dest;
}

std::streamsize PackDevice::read(char_type* s, std::streamsize n)
{
	if (!m_data) {
		// Gather all deltas and store their header information. We do this recursively for small objects.
		// For larger objects, we first merge all deltas into one byte stream, to finally generate the final output
		// at once. This way, we do not need two possibly huge buffers in memory, but only one in a moderate size
		// for the merged delta, and the buffer for the final result. The basic source buffer will is memory mapped.
		// NOTE: We will need random access to the base buffer, which is only available in compressed form.
		// The only advantage would be to keep the delta separate, and only produce the requested amount of 
		// output bytes. Unfortunately, this would mean a larger persistent memory footprint. Otherwise we would
		// have two buffer, base and target, and the decompressed delta stream allocated.
		// A good way to keep memory demands reasonable is to actually stream big objects which may not even 
		// be deltified.
		cursor_type cur = m_pack.cursor();
		PackInfo info;
		info.ofs = m_pack.index().offset(m_entry);
		info_at_offset(cur, info);
		
		m_data.reset(unpack_object_recursive(cur, info, m_size));
		this->m_nb = this->m_size;
	}
	assert(!!m_data);
	
	// Finally copy the requested amount of data
	return parent_type::read(s, n, m_data.get());
}

uint PackDevice::delta_size(cursor_type& cur, uint64 ofs, uint64& base_size, uint64& target_size) const
{
	char_type delta_header[20];	// can handle two 64 bit numbers
	cur.use_region(ofs, 20);
	decompress_some(cur, delta_header, 20, 128);	// should process 128 bytes in one go
	
	const char_type* data = delta_header;
	base_size = msb_len(data);
	target_size = msb_len(data);		// target offset
	
	return data - delta_header;
}

GIT_NAMESPACE_END
