#include "git/db/sha1_gen.h"
#include <git/config.h>	// for doxygen

#include "memory.h"

GIT_NAMESPACE_BEGIN
		
#ifndef ROL32
#ifdef _MSC_VER
#define ROL32(_val32,_nBits) _rotl(_val32,_nBits)
#else
#define ROL32(_val32,_nBits) (((_val32)<<(_nBits))|((_val32)>>(32-(_nBits))))
#endif
#endif

#ifdef SHA1_LITTLE_ENDIAN
#define SHABLK0(i) (m_block->l[i] = \
	(ROL32(m_block->l[i],24) & 0xFF00FF00) | (ROL32(m_block->l[i],8) & 0x00FF00FF))
#else
#define SHABLK0(i) (m_block->l[i])
#endif

#define SHABLK(i) (m_block->l[i&15] = ROL32(m_block->l[(i+13)&15] ^ m_block->l[(i+8)&15] \
	^ m_block->l[(i+2)&15] ^ m_block->l[i&15],1))

// SHA-1 rounds
#define _R0(v,w,x,y,z,i) {z+=((w&(x^y))^y)+SHABLK0(i)+0x5A827999+ROL32(v,5);w=ROL32(w,30);}
#define _R1(v,w,x,y,z,i) {z+=((w&(x^y))^y)+SHABLK(i)+0x5A827999+ROL32(v,5);w=ROL32(w,30);}
#define _R2(v,w,x,y,z,i) {z+=(w^x^y)+SHABLK(i)+0x6ED9EBA1+ROL32(v,5);w=ROL32(w,30);}
#define _R3(v,w,x,y,z,i) {z+=(((w|x)&y)|(w&x))+SHABLK(i)+0x8F1BBCDC+ROL32(v,5);w=ROL32(w,30);}
#define _R4(v,w,x,y,z,i) {z+=(w^x^y)+SHABLK(i)+0xCA62C1D6+ROL32(v,5);w=ROL32(w,30);}

SHA1Generator::SHA1Generator()
	: m_block((WorkspaceBlock*)m_workspace)
{
	static_assert(sizeof(uint32) == 4, "int must be 32 bit");
	static_assert(sizeof(uchar) == 1, "char must be 8 bit");
	reset();
}

void SHA1Generator::reset() throw()
{
	m_finalized = 0;
	m_update_called = 0;
	
	// SHA1 initialization constants
	m_state[0] = 0x67452301;
	m_state[1] = 0xEFCDAB89;
	m_state[2] = 0x98BADCFE;
	m_state[3] = 0x10325476;
	m_state[4] = 0xC3D2E1F0;

	m_count[0] = 0;
	m_count[1] = 0;

	memset(m_digest, 0, 20);
}

void SHA1Generator::transform(const uchar* pBuffer)
{
	uint32 a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3], e = m_state[4];
	memcpy(m_block, pBuffer, 64);

	// 4 rounds of 20 operations each. Loop unrolled.
	_R0(a,b,c,d,e, 0) _R0(e,a,b,c,d, 1) _R0(d,e,a,b,c, 2) _R0(c,d,e,a,b, 3)
	_R0(b,c,d,e,a, 4) _R0(a,b,c,d,e, 5) _R0(e,a,b,c,d, 6) _R0(d,e,a,b,c, 7)
	_R0(c,d,e,a,b, 8) _R0(b,c,d,e,a, 9) _R0(a,b,c,d,e,10) _R0(e,a,b,c,d,11)
	_R0(d,e,a,b,c,12) _R0(c,d,e,a,b,13) _R0(b,c,d,e,a,14) _R0(a,b,c,d,e,15)
	_R1(e,a,b,c,d,16) _R1(d,e,a,b,c,17) _R1(c,d,e,a,b,18) _R1(b,c,d,e,a,19)
	_R2(a,b,c,d,e,20) _R2(e,a,b,c,d,21) _R2(d,e,a,b,c,22) _R2(c,d,e,a,b,23)
	_R2(b,c,d,e,a,24) _R2(a,b,c,d,e,25) _R2(e,a,b,c,d,26) _R2(d,e,a,b,c,27)
	_R2(c,d,e,a,b,28) _R2(b,c,d,e,a,29) _R2(a,b,c,d,e,30) _R2(e,a,b,c,d,31)
	_R2(d,e,a,b,c,32) _R2(c,d,e,a,b,33) _R2(b,c,d,e,a,34) _R2(a,b,c,d,e,35)
	_R2(e,a,b,c,d,36) _R2(d,e,a,b,c,37) _R2(c,d,e,a,b,38) _R2(b,c,d,e,a,39)
	_R3(a,b,c,d,e,40) _R3(e,a,b,c,d,41) _R3(d,e,a,b,c,42) _R3(c,d,e,a,b,43)
	_R3(b,c,d,e,a,44) _R3(a,b,c,d,e,45) _R3(e,a,b,c,d,46) _R3(d,e,a,b,c,47)
	_R3(c,d,e,a,b,48) _R3(b,c,d,e,a,49) _R3(a,b,c,d,e,50) _R3(e,a,b,c,d,51)
	_R3(d,e,a,b,c,52) _R3(c,d,e,a,b,53) _R3(b,c,d,e,a,54) _R3(a,b,c,d,e,55)
	_R3(e,a,b,c,d,56) _R3(d,e,a,b,c,57) _R3(c,d,e,a,b,58) _R3(b,c,d,e,a,59)
	_R4(a,b,c,d,e,60) _R4(e,a,b,c,d,61) _R4(d,e,a,b,c,62) _R4(c,d,e,a,b,63)
	_R4(b,c,d,e,a,64) _R4(a,b,c,d,e,65) _R4(e,a,b,c,d,66) _R4(d,e,a,b,c,67)
	_R4(c,d,e,a,b,68) _R4(b,c,d,e,a,69) _R4(a,b,c,d,e,70) _R4(e,a,b,c,d,71)
	_R4(d,e,a,b,c,72) _R4(c,d,e,a,b,73) _R4(b,c,d,e,a,74) _R4(a,b,c,d,e,75)
	_R4(e,a,b,c,d,76) _R4(d,e,a,b,c,77) _R4(c,d,e,a,b,78) _R4(b,c,d,e,a,79)

	// Add the working vars back into state
	m_state[0] += a;
	m_state[1] += b;
	m_state[2] += c;
	m_state[3] += d;
	m_state[4] += e;
}

// Use this function to hash in binary data and strings
void SHA1Generator::update(const uchar* pbData, uint32 uLen) throw(gtl::bad_state)
{
	if (m_finalized){
		throw BadSHA1GenState();
	}
	m_update_called = 1;
	
	uint32 j = ((m_count[0] >> 3) & 0x3F);

	if ((m_count[0] += (uLen << 3)) < (uLen << 3)) {
		++m_count[1];        // Overflow
	}

	m_count[1] += (uLen >> 29);
	uint32 i;
	if ((j + uLen) > 63) {
		i = 64 - j;
		memcpy(&m_buffer[j], pbData, i);
		transform(m_buffer);
		for (; (i + 63) < uLen; i += 64) {
			transform(&pbData[i]);
		}

		j = 0;
	} else {
		i = 0;
	}

	if ((uLen - i) != 0) {
		memcpy(&m_buffer[j], &pbData[i], uLen - i);
	}
}

void SHA1Generator::finalize() throw(gtl::bad_state)
{
	if (m_finalized){
		throw BadSHA1GenState();
	}
	
	uint32 i;
	uchar finalcount[8];
	for (i = 0; i < 8; ++i) {
		finalcount[i] = (uchar)((m_count[((i >= 4) ? 0 : 1)]
		                         >> ((3 - (i & 3)) * 8)) & 255);  // Endian independent
	}

	update((uchar*)"\200", 1);
	while ((m_count[0] & 504) != 448) {
		update((uchar*)"\0", 1);
	}

	update(finalcount, 8); // Cause a SHA1transform()
	for (i = 0; i < 20; ++i) {
		m_digest[i] = (uchar)((m_state[i >> 2] >> ((3 - (i & 3)) * 8)) & 0xFF);
	}
	
	m_finalized = 1;
}

GIT_NAMESPACE_END
