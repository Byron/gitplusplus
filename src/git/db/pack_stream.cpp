#include <git/db/pack_stream.h>
#include <git/config.h>
#include <git/db/pack_file.h>

#include <iostream>
#include <cstring>
#include <array>

GIT_NAMESPACE_BEGIN


//! @{ Utilties


void PackDevice::decompress_some(cursor_type& cur, stream_offset ofs, char_type* dest, size_type nb, size_type max_input_chunk_size)
{
	assert(cur.is_valid());
	if(m_stream.mode() != gtl::zlib_stream::Mode::Decompress) {
		m_stream.set_mode(gtl::zlib_stream::Mode::Decompress);
	} else {
		this->m_stream.reset();
	}
	
	this->m_stat = Z_OK;
	
	// Zip can only take 32 bit numbers, but the pack doesn't try to pack anything larger than 32 bit anyway
	this->m_stream.next_out = reinterpret_cast<uchar*>(dest);
	this->m_stream.avail_out = static_cast<uint>(nb);
	do {
		cur.use_region(ofs, nb / 2);
		if (!cur.is_valid()) {
			ParseError err;
			err.stream() << "Failed to map pack at offset " << ofs;
			throw err;
		}
		this->m_stream.next_in = const_cast<uchar*>(reinterpret_cast<const uchar*>(cur.begin()));
		this->m_stream.avail_in = max_input_chunk_size ? std::min(cur.size(), static_cast<decltype(cur.size())>(max_input_chunk_size)) : cur.size();
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
	assert(len <= std::numeric_limits<size_type>::max());
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
				const_cast<PackDevice*>(this)->m_obj_size = info.size;
			}
			break;
		}// handle object type
		
		if (!has_delta_size) {
			// delta size uses our zstream, which is why it is non-const. The stream though is an internal
			// detail that we take care of
			size_type size;
			const_cast<PackDevice*>(this)->delta_size(cur, info.ofs + info.rofs,
			           size, size);	 // target size assigned last
			const_cast<PackDevice*>(this)->m_obj_size = size;
			has_delta_size = true;
		}
		
		if (size_only & has_delta_size) {
			break;
		}
		
		info.ofs = next_ofs;
	}// while we are not at the base
}

void PackDevice::apply_delta(const char_type* base, char_type* dest, const char_type* delta, size_type deltalen) const
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

PackDevice::counted_char_ptr_const_type PackDevice::unpack_object_recursive(cursor_type& cur, const PackInfo& info, obj_size_type& out_size)
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
		m_type = static_cast<ObjectType>(info.type);		// cache our type right away
		return obtain_data(cur, info, true);
		break;
	}
	case PackedObjectType::OfsDelta:
	case PackedObjectType::RefDelta:
	{
		// see if we have our final object already
		counted_char_ptr_const_type base_data;
		PackCache::size_type tmp_size;
		PackedObjectType tmp_type;
		PackCache& cache = m_pack.cache();
		bool sequencial_caching = cache.mode() == gtl::cache_access_mode::sequencial;
		
		// It makes not much sense to keep a cache in random access mode, as the only chance 
		// we have to make good use of the cache is to store as many objects as possible, hence
		// storing the uncompressed delta's is using the limited cache more efficiently.
		// In sequencial mode, we know that our base is going to be used more orderly, hence we can 
		// gain a lot of performance if we can just keep a level 50 delta instead of reapplying 50 cached deltas.
		if (sequencial_caching) {
			if (cache.is_available() && (base_data = cache.cache_at(info.ofs, &tmp_type, &tmp_size)).get() != nullptr) {
				assert(tmp_type != PackedObjectType::Bad 
					   && tmp_type != PackedObjectType::RefDelta 
					   && tmp_type != PackedObjectType::OfsDelta);
				out_size = static_cast<size_type>(tmp_size);
				m_type = static_cast<ObjectType>(tmp_type);
				return base_data;
			}
		}
		
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
		// See if we have the full base stored already
		base_data = unpack_object_recursive(cur, next_info, out_size);
		// store the delta in non-sequencial mode
		counted_char_ptr_const_type ddata(obtain_data(cur, info, !sequencial_caching));		// delta memory
		const char_type* cpddata = *ddata; //!< const pointer to delta data
		
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
		counted_char_type* dest = new counted_char_type[out_size];
		apply_delta(*base_data, *dest, cpddata, info.size - (cpddata - *ddata));
		// in sequencial mode, we store the result using our offset as key. Otherwise we use
		// our offset to store the decompressed delta.
		if (sequencial_caching) {
			cache.set_cache_at(info.ofs, static_cast<PackedObjectType>(m_type), out_size, dest);
		}
		return counted_char_ptr_const_type(dest);
		break;
	}
	default: 
	{
		throw ParseError();
	}
	};// end type switch
	
	// compiler doesn't realize that the control can't ever get here, so we give it what it wants
	return counted_char_ptr_type();
}

PackDevice::counted_char_ptr_const_type PackDevice::obtain_data(cursor_type& cur, const PackInfo& info, bool allow_cache) 
{
	if (allow_cache) {
		PackCache& cache = m_pack.cache();
		counted_char_ptr_const_type cdata;
		if (cache.is_available() && (cdata = cache.cache_at(info.ofs)).get() != nullptr) {
			return cdata;
		}
	
		counted_char_type* pddata(new counted_char_type[info.size]);
		decompress_some(cur, info.ofs+info.rofs, *pddata, info.size);
		
		if (cache.is_available()) {
			cache.set_cache_at(info.ofs, info.type, info.size, pddata);
		}
		return counted_char_ptr_const_type(pddata);
	} else {
		counted_char_type* pddata(new counted_char_type[info.size]);
		decompress_some(cur, info.ofs+info.rofs, *pddata, info.size);
		return counted_char_ptr_const_type(pddata);
	}
}

void PackDevice::unpack_data(cursor_type& cur, const PackInfo& info)
{
	m_data = unpack_object_recursive(cur, info, m_obj_size);
	this->m_ofs = stream_offset(0);
	this->m_nb = m_obj_size;
	this->m_size = m_obj_size;
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
			unpack_data(cur, info);
		} else {
			this->set_window(parent_type::max_length, info.ofs+info.rofs);
		}
	} // end initialize data
	
	
	if (!!m_data) { // uncompressed delta
		// seekable device - have to call it directly as protected methods can only be called
		// from directly inherited bases (this is the mapped_file_source)
		return parent_type::parent_type::read(s, n, *m_data);
	} else { // undeltified object
		assert(parent_type::is_open());
		return parent_type::read(s, n);	// automatic and direct decompression
	}
}

bool PackDevice::verify_hash(const key_type& hash, hash_generator_type& hgen) {
	cursor_type cur = m_pack.cursor();
	PackInfo info;
	info.ofs = m_pack.index().offset(m_entry);
	info_at_offset(cur, info);
	
	if (!parent_type::is_open()) {
		parent_type::open(cur, parent_type::max_length, info.ofs+info.rofs);
	}
	
	std::streamsize											br;		// bytes read
	std::array<char_type, 8192>								buf;
	
	if (info.is_delta()) {
		unpack_data(cur, info);
		assert(m_type != ObjectType::None);
		br = loose_object_header(buf.data(), m_type, m_obj_size);
		hgen.update(buf.data(), static_cast<uint32>(br));
		hgen.update(*m_data, static_cast<uint32>(m_obj_size));
	} else {
		this->set_window(parent_type::max_length, info.ofs+info.rofs);
		br = loose_object_header(buf.data(), static_cast<ObjectType>(info.type), info.size);
		hgen.update(buf.data(), br);
		
		do {
			br = parent_type::read(buf.data(), buf.size());
			if (br > -1) {
				hgen.update(buf.data(), static_cast<uint32>(br));
			}
		} while(static_cast<size_t>(br) == buf.size());
		return true;
	}
	
	return hgen.hash() == hash;
}

uint PackDevice::delta_size(cursor_type& cur, uint64 ofs, size_type& base_size, size_type& target_size)
{
	// TODO: Try to use the cache manually
	char_type delta_header[20];	// can handle two 64 bit numbers
	decompress_some(cur, ofs, delta_header, 20, 128);	// should process 128 bytes in one go
	
	const char_type* data = delta_header;
	base_size = static_cast<size_type>(msb_len(data));
	target_size = static_cast<size_type>(msb_len(data));		// target offset
	
	return data - delta_header;
}

GIT_NAMESPACE_END
