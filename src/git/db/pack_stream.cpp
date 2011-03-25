#include <git/db/pack_stream.h>
#include <git/config.h>

#include <git/db/pack_file.h>
#include <iostream>
#include <cstring>

GIT_NAMESPACE_BEGIN


//! @{ Utilties


void PackDevice::decompress_some(cursor_type& cur, stream_offset ofs, char_type* dest, uint64 nb, size_t max_input_chunk_size)
{
	assert(cur.is_valid());
	if(m_stream.mode() != gtl::zlib_stream::Mode::Decompress) {
		m_stream.set_mode(gtl::zlib_stream::Mode::Decompress);
	} else {
		this->m_stream.reset();
	}
	
	this->m_stat = Z_OK;
	
	// As zip can only take 32 bit numbers, we must chop up the operation
	const uint64 chunk_size = 1024*1024*16;
	static_assert(chunk_size <= std::numeric_limits<decltype(this->m_stream.avail_out)>::max(), "chunk too large");
	
	for (uint64 bd = 0; this->m_stat != Z_STREAM_END && bd < nb; bd += chunk_size)
	{
		this->m_stream.next_out = reinterpret_cast<uchar*>(dest + std::min(bd, nb));
		this->m_stream.avail_out = static_cast<uint>(std::min(chunk_size, nb));
		do {
			cur.use_region(ofs, chunk_size / 2);
			if (!cur.is_valid()) {
				ParseError err;
				err.stream() << "Failed to map pack at offset " << ofs;
				throw err;
			}
			this->m_stream.next_in = const_cast<uchar*>(reinterpret_cast<const uchar*>(cur.begin()));
			this->m_stream.avail_in = max_input_chunk_size ? std::min(cur.size(), max_input_chunk_size) : cur.size();
			this->m_stat = this->m_stream.decompress(true);
			ofs += this->m_stream.next_in - reinterpret_cast<const uchar*>(cur.begin());
		} while ((this->m_stat == Z_OK || this->m_stat == Z_BUF_ERROR) && this->m_stream.avail_out);
		
		switch(this->m_stat) 
		{
			case Z_ERRNO:
			case Z_STREAM_ERROR:
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
			case Z_VERSION_ERROR:
			{
				throw gtl::zlib_error(this->m_stat, "ZLib stream error");
			}
		}// end switch
	}// for each chunk
	
	// Most importantly, we produce the amount of bytes we wanted. BUF ERRORs occour
	// if there is not enough output (or input)
	if (this->m_stream.total_out != nb && (this->m_stat != Z_STREAM_END && this->m_stat != Z_BUF_ERROR)) {
		throw gtl::zlib_error(this->m_stat, "Failed to produce enough bytes during decompression");
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
	// Copy constructor is called a lot (3 times ) by iostreams::copy which is quite terrible, 
	// std::cerr << "PACK DEVICE: " << "INVOKED COPY CONSTRUCTOR" << std::endl;
	// don't copy the data, it will be recreated when it is first needed
}

PackDevice::~PackDevice()
{
}

uint64 PackDevice::msb_len(const char_type*& data) const
{
	uint64 len = 0;
	const char_type* x = data;
	char_type c;
	uint s = 0;
	do {
		c = *x++;
		len |= (c & 0x7f) << s;
		s += 7;
	} while (c & 0x80);
	data = x;
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
			// delta size uses our zstream, which is why it is non-const. The stream though is an internal
			// detail that we take care of
			const_cast<PackDevice*>(this)->delta_size(cur, info.ofs + info.rofs,
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

PackDevice::managed_const_char_ptr_array PackDevice::unpack_object_recursive(cursor_type& cur, const PackInfo& info, uint64& out_size)
{
	assert(info.type != PackedObjectType::Bad);
	
	//! NOTE: We allocate the memory late to prevent excessive memory use during recursion
	switch (info.type)
	{
	case PackedObjectType::Commit:
	case PackedObjectType::Tree:
	case PackedObjectType::Blob:
	case PackedObjectType::Tag:
	{
		out_size = info.size;
		return obtain_data(cur, info.ofs, info.rofs, info.size);
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
		managed_const_char_ptr_array base_data(unpack_object_recursive(cur, next_info, out_size));	// base memory
		managed_const_char_ptr_array ddata(obtain_data(cur, info.ofs, info.rofs, info.size));		// delta memory
		const char_type* cpddata = ddata.get(); //!< const pointer to delta data
		
		uint64 base_size = msb_len(cpddata);
		if (base_size != out_size) {
			ParseError err;
			err.stream() << "Base buffer length didn't match the parsed information: " << out_size << " != " << base_size;
			if (info.type == PackedObjectType::RefDelta) {
				err.stream() << std::endl << "Base was " << info.delta.key;
			}
			throw err;
		}
		out_size = msb_len(cpddata);
		
		// Allocate memory to keep the destination and apply delta
		char_type* dest = new char_type[out_size];
		apply_delta(base_data.get(), dest, cpddata, info.size - (cpddata - ddata.get()));
		return managed_const_char_ptr_array(true, dest);
		break;
	}
	default: 
	{
		throw ParseError();
	}
	};// end type switch
	
	// compiler doesn't realize that the control can't ever get here, so we give it what it wants
	return managed_const_char_ptr_array();
}

PackDevice::managed_const_char_ptr_array PackDevice::obtain_data(cursor_type& cur, stream_offset ofs, 
                                                                       uint32 rofs, uint64 nb) 
{
	PackCache& cache = m_pack.cache();
	const char_type* cdata;
	if (cache.is_available() && (cdata = cache.cache_at(ofs)) != nullptr) {
		return managed_const_char_ptr_array(false, cdata);
	}

	char_type* ddata = new char_type[nb];
	decompress_some(cur, ofs+rofs, ddata, nb);
	
	return managed_const_char_ptr_array(cache.is_available() 
	                                    ? !cache.set_cache_at(ofs, ddata)
	                                    : true, ddata);
}

std::streamsize PackDevice::read(char_type* s, std::streamsize n)
{
	if (!m_data && this->m_stream.mode() == gtl::zlib_stream::Mode::None) {
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
		
		// need an initialized zlib stream. In delta mode, we use the base-classes zlib stream for our own 
		// purposes and keep it initialized
		if (!parent_type::is_open()) {
			parent_type::open(cur, parent_type::max_length, info.ofs+info.rofs);
		}
		
		// if we have deltas, there is no other way than extracting it into memory, one way or another.
		if (info.is_delta()) {
			m_data.take_ownership(unpack_object_recursive(cur, info, m_size));
			this->m_nb = this->m_size;
		} else {
			this->set_window(parent_type::max_length, info.ofs+info.rofs);
		}
	} // end initialize data
	
	
	if (!!m_data) { // uncompressed delta
		// seekable device - have to call it directly as protected methods can only be called
		// from directly inherited bases (this is the mapped_file_source)
		return parent_type::parent_type::read(s, n, m_data.get());
	} else { // undeltified object
		assert(parent_type::is_open());
		return parent_type::read(s, n);	// automatic and direct decompression
	}
}

uint PackDevice::delta_size(cursor_type& cur, uint64 ofs, uint64& base_size, uint64& target_size)
{
	// TODO: Try to use the cache manually
	char_type delta_header[20];	// can handle two 64 bit numbers
	decompress_some(cur, ofs, delta_header, 20, 128);	// should process 128 bytes in one go
	
	const char_type* data = delta_header;
	base_size = msb_len(data);
	target_size = msb_len(data);		// target offset
	
	return data - delta_header;
}

GIT_NAMESPACE_END
