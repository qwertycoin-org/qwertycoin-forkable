// Copyright (c) 2017, SUMOKOIN
// Copyright (c) 2018-2021, The Qwertycoin Group.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Parts of this file are originally copyright (c) 2012-2013, The Cryptonote developers

#include <Crypto/CnSlowHash.h>

extern "C" {
#include <Crypto/Keccak.h>
}

#ifdef HAS_ARM_HW

extern const uint8_t sAesSbox[256];

struct AesKeyData
{
    uint32_t x0;
    uint32_t x1;
    uint32_t x2;
    uint32_t x3;

    AesKeyData(const uint8_t* memory)
    {
	const uint32_t* mem = reinterpret_cast<const uint32_t*>(memory);
	x0 = mem[0];
	x1 = mem[1];
	x2 = mem[2];
	x3 = mem[3];
    }

    inline uint8x16_t store()
    {
	uint32x4_t tmp = {x0, x1, x2, x3};

	return vreinterpretq_u8_u32(tmp);
    }

    inline AesKeyData& operator^=(uint32_t rhs) noexcept
    {
	x0 ^= rhs;
	x1 ^= rhs;
	x2 ^= rhs;
	x3 ^= rhs;

	return *this;
    }
};

// slXor(a1 a2 a3 a4) = a1 (a2^a1) (a3^a2^a1) (a4^a3^a2^a1)
inline void slXor(AesKeyData& x)
{
    x.x1 ^= x.x0;
    x.x2 ^= x.x1;
    x.x3 ^= x.x2;
}

inline uint32_t subWord(uint32_t key)
{
    return (sAesSbox[key >> 24 ] << 24)
            | (sAesSbox[(key >> 16) & 0xff] << 16 )
            | (sAesSbox[(key >> 8)  & 0xff] << 8  )
            | sAesSbox[key & 0xff];
}

inline uint32_t rotr(uint32_t value, uint32_t amount)
{
    return (value >> amount) | (value << ((32 - amount) & 31));
}

template<uint8_t rcon>
inline void softAesGenkeySub(AesKeyData& xout0, aeskeydata& xout2)
{
    uint32_t tmp;
    slXor(xout0);
    xout0 ^= rotr(subWord(xout2.x3), 8) ^ rcon;
    slXor(xout2);
    xout2 ^= subWord(xout0.x3);
}

inline void aesGenkey(const uint8_t* memory,
                      uint8x16_t& k0,
                      uint8x16_t& k1,
                      uint8x16_t& k2,
                      uint8x16_t& k3,
                      uint8x16_t& k4,
	              uint8x16_t& k5,
                      uint8x16_t& k6,
                      uint8x16_t& k7,
                      uint8x16_t& k8,
                      uint8x16_t& k9)
{
    AesKeyData xout0(memory);
    AesKeyData xout2(memory + 16);

    k0 = xout0.store();
    k1 = xout2.store();

    softAesGenkeySub<0x01>(xout0, xout2);
    k2 = xout0.store();
    k3 = xout2.store();

    softAesGenkeySub<0x02>(xout0, xout2);
    k4 = xout0.store();
    k5 = xout2.store();

    softAesGenkeySub<0x04>(xout0, xout2);
    k6 = xout0.store();
    k7 = xout2.store();

    softAesGenkeySub<0x08>(xout0, xout2);
    k8 = xout0.store();
    k9 = xout2.store();
}

inline void aesRound10(uint8x16_t& x,
                       const uint8x16_t& k0,
                       const uint8x16_t& k1,
                       const uint8x16_t& k2,
                       const uint8x16_t& k3,
                       const uint8x16_t& k4,
                       const uint8x16_t& k5,
                       const uint8x16_t& k6,
                       const uint8x16_t& k7,
                       const uint8x16_t& k8,
                       const uint8x16_t& k9)
{
    x = vaesmcq_u8(vaeseq_u8(x, vdupq_n_u8(0)));
    x = vaesmcq_u8(vaeseq_u8(x, k0));
    x = vaesmcq_u8(vaeseq_u8(x, k1));
    x = vaesmcq_u8(vaeseq_u8(x, k2));
    x = vaesmcq_u8(vaeseq_u8(x, k3));
    x = vaesmcq_u8(vaeseq_u8(x, k4));
    x = vaesmcq_u8(vaeseq_u8(x, k5));
    x = vaesmcq_u8(vaeseq_u8(x, k6));
    x = vaesmcq_u8(vaeseq_u8(x, k7));
    x = vaesmcq_u8(vaeseq_u8(x, k8));
    x = veorq_u8(x, k9);
}

inline void xOrShift(uint8x16_t& x0,
                     uint8x16_t& x1,
                     uint8x16_t& x2,
                     uint8x16_t& x3,
                     uint8x16_t& x4,
                     uint8x16_t& x5,
                     uint8x16_t& x6,
                     uint8x16_t& x7)
{
    uint8x16_t tmp0 = x0;
    x0 ^= x1;
    x1 ^= x2;
    x2 ^= x3;
    x3 ^= x4;
    x4 ^= x5;
    x5 ^= x6;
    x6 ^= x7;
    x7 ^= tmp0;
}

inline void memLoad(CnSptr& lPad,
                    size_t i,
                    uint8x16_t& x0,
                    uint8x16_t& x1,
                    uint8x16_t& x2,
                    uint8x16_t& x3,
                    uint8x16_t& x4,
                    uint8x16_t& x5,
                    uint8x16_t& x6,
                    uint8x16_t& x7)
{
    x0 ^= vld1q_u8(lPad.as_byte() + i);
    x1 ^= vld1q_u8(lPad.as_byte() + i + 16);
    x2 ^= vld1q_u8(lPad.as_byte() + i + 32);
    x3 ^= vld1q_u8(lPad.as_byte() + i + 48);
    x4 ^= vld1q_u8(lPad.as_byte() + i + 64);
    x5 ^= vld1q_u8(lPad.as_byte() + i + 80);
    x6 ^= vld1q_u8(lPad.as_byte() + i + 96);
    x7 ^= vld1q_u8(lPad.as_byte() + i + 112);
}

template<size_t MEMORY, size_t ITER, size_t POWVER>
void CnSlowHash<MEMORY,ITER,POWVER>::implodeScratchpadHard()
{
    uint8x16_t x0, x1, x2, x3, x4, x5, x6, x7;
    uint8x16_t k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

    aesGenkey(sPad.as_byte() + 32, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

    x0 = vld1q_u8(sPad.as_byte() + 64);
    x1 = vld1q_u8(sPad.as_byte() + 80);
    x2 = vld1q_u8(sPad.as_byte() + 96);
    x3 = vld1q_u8(sPad.as_byte() + 112);
    x4 = vld1q_u8(sPad.as_byte() + 128);
    x5 = vld1q_u8(sPad.as_byte() + 144);
    x6 = vld1q_u8(sPad.as_byte() + 160);
    x7 = vld1q_u8(sPad.as_byte() + 176);

    for (size_t i = 0; i < MEMORY; i += 128) {
        memLoad(lPad, i, x0, x1, x2, x3, x4, x5, x6, x7);

        aesRound10(x0, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
        aesRound10(x1, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
        aesRound10(x2, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
        aesRound10(x3, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
        aesRound10(x4, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
        aesRound10(x5, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
        aesRound10(x6, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
        aesRound10(x7, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

        if(POWVER > 0) {
            xOrShift(x0, x1, x2, x3, x4, x5, x6, x7);
        }
    }

    for (size_t i = 0; POWVER > 0 && i < MEMORY; i += 128) {
	memLoad(lPad, i, x0, x1, x2, x3, x4, x5, x6, x7);

	aesRound10(x0, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x1, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x2, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x3, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x4, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x5, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x6, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x7, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

	xOrShift(x0, x1, x2, x3, x4, x5, x6, x7);
    }

    for (size_t i = 0; POWVER > 0 && i < 16; i++) {
	aesRound10(x0, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x1, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x2, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x3, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x4, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x5, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x6, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x7, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

	xOrShift(x0, x1, x2, x3, x4, x5, x6, x7);
    }

    vst1q_u8(sPad.as_byte() + 64, x0);
    vst1q_u8(sPad.as_byte() + 80, x1);
    vst1q_u8(sPad.as_byte() + 96, x2);
    vst1q_u8(sPad.as_byte() + 112, x3);
    vst1q_u8(sPad.as_byte() + 128, x4);
    vst1q_u8(sPad.as_byte() + 144, x5);
    vst1q_u8(sPad.as_byte() + 160, x6);
    vst1q_u8(sPad.as_byte() + 176, x7);
}

template<size_t MEMORY, size_t ITER, size_t POWVER>
void CnSlowHash<MEMORY,ITER,POWVER>::explodeScratchpadHard()
{
    uint8x16_t x0, x1, x2, x3, x4, x5, x6, x7;
    uint8x16_t k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

    aesGenkey(sPad.as_byte(), k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

    x0 = vld1q_u8(sPad.as_byte() + 64);
    x1 = vld1q_u8(sPad.as_byte() + 80);
    x2 = vld1q_u8(sPad.as_byte() + 96);
    x3 = vld1q_u8(sPad.as_byte() + 112);
    x4 = vld1q_u8(sPad.as_byte() + 128);
    x5 = vld1q_u8(sPad.as_byte() + 144);
    x6 = vld1q_u8(sPad.as_byte() + 160);
    x7 = vld1q_u8(sPad.as_byte() + 176);

    for (size_t i = 0; POWVER > 0 && i < 16; i++) {
	aesRound10(x0, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x1, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x2, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x3, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x4, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x5, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x6, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x7, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

	xOrShift(x0, x1, x2, x3, x4, x5, x6, x7);
    }

    for(size_t i = 0; i < MEMORY; i += 128) {
	aesRound10(x0, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x1, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x2, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x3, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x4, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x5, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x6, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
	aesRound10(x7, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

	vst1q_u8(lPad.as_byte() + i, x0);
	vst1q_u8(lPad.as_byte() + i + 16, x1);
	vst1q_u8(lPad.as_byte() + i + 32, x2);
	vst1q_u8(lPad.as_byte() + i + 48, x3);
	vst1q_u8(lPad.as_byte() + i + 64, x4);
	vst1q_u8(lPad.as_byte() + i + 80, x5);
	vst1q_u8(lPad.as_byte() + i + 96, x6);
	vst1q_u8(lPad.as_byte() + i + 112, x7);
    }
}

inline uint64_t _umul128(uint64_t a, uint64_t b, uint64_t* hi)
{
    unsigned __int128 r = (unsigned __int128)a * (unsigned __int128)b;
    *hi = r >> 64;

    return (uint64_t)r;
}

extern "C" void blake256Hash(uint8_t*, const uint8_t*, uint64_t);
extern "C" void groestl(const unsigned char*, unsigned long long, unsigned char*);
extern "C" size_t jhHash(int, const unsigned char*, unsigned long long, unsigned char*);
extern "C" size_t skeinHash(int, const unsigned char*, size_t, unsigned char*);

inline uint8x16_t _mm_set_epi64x(const uint64_t a, const uint64_t b)
{
    return vreinterpretq_u8_u64(vcombine_u64(vcreate_u64(b), vcreate_u64(a)));
}

template<size_t MEMORY, size_t ITER, size_t POWVER>
void CnSlowHash<MEMORY,ITER,POWVER>::hardwareHash(const void* in, size_t len, void* out)
{
    keccak((const uint8_t *)in, len, sPad.as_byte(), 200);

    explodeScratchpadHard();

    uint64_t* h0 = sPad.as_uqword();

    uint64_t al0 = h0[0] ^ h0[4];
    uint64_t ah0 = h0[1] ^ h0[5];
    uint8x16_t bx0 = _mm_set_epi64x(h0[3] ^ h0[7], h0[2] ^ h0[6]);

    uint64_t idx0 = h0[0] ^ h0[4];

    const uint8x16_t zero = vdupq_n_u8(0);
    // Optim - 90% time boundary
    for(size_t i = 0; i < ITER; i++) {
	uint8x16_t cx;
	cx = vld1q_u8(scratchpad_ptr(idx0).as_byte());

	cx = vaesmcq_u8(vaeseq_u8(cx, zero)) ^ _mm_set_epi64x(ah0, al0);

	vst1q_u8(scratchpad_ptr(idx0).as_byte(), bx0 ^ cx);

	idx0 = vgetq_lane_u64(vreinterpretq_u64_u8(cx), 0);
	bx0 = cx;

	uint64_t hi, lo, cl, ch;
	cl = scratchpad_ptr(idx0).as_uqword(0);
	ch = scratchpad_ptr(idx0).as_uqword(1);

	lo = _umul128(idx0, cl, &hi);

	al0 += hi;
	ah0 += lo;
	scratchpad_ptr(idx0).as_uqword(0) = al0;
	scratchpad_ptr(idx0).as_uqword(1) = ah0;
	ah0 ^= ch;
	al0 ^= cl;
	idx0 = al0;

	if(POWVER > 0) {
    	    int64_t n  = scratchpad_ptr(idx0).as_qword(0);
	    int32_t d  = scratchpad_ptr(idx0).as_dword(2);
	    int64_t q = n / (d | 5);
	    scratchpad_ptr(idx0).as_qword(0) = n ^ q;
	    idx0 = d ^ q;
	}
    }

    implodeScratchpadHard();

    keccakf(sPad.as_uqword(), 24);

    switch(sPad.as_byte(0) & 3) {
	case 0:
    	    blake256Hash((uint8_t*)out, sPad.as_byte(), 200);
	    break;
	case 1:
	    groestl(sPad.as_byte(), 200 * 8, (uint8_t*)out);
	    break;
	case 2:
	    jhHash(32 * 8, sPad.as_byte(), 8 * 200, (uint8_t*)out);
	    break;
	case 3:
	    skeinHash(8 * 32, sPad.as_byte(), 8 * 200, (uint8_t*)out);
	    break;
	}
}

template class CnSlowHash<2*1024*1024, 0x80000, 0>;
template class CnSlowHash<4*1024*1024, 0x40000, 1>;

#endif

#ifdef HAS_INTEL_HW

#if !defined(_LP64) && !defined(_WIN64)
#define BUILD32
#endif

// slXor(a1 a2 a3 a4) = a1 (a2^a1) (a3^a2^a1) (a4^a3^a2^a1)
inline __m128i slXor(__m128i tmp1)
{
    __m128i tmp4;
    tmp4 = _mm_slli_si128(tmp1, 0x04);
    tmp1 = _mm_xor_si128(tmp1, tmp4);
    tmp4 = _mm_slli_si128(tmp4, 0x04);
    tmp1 = _mm_xor_si128(tmp1, tmp4);
    tmp4 = _mm_slli_si128(tmp4, 0x04);
    tmp1 = _mm_xor_si128(tmp1, tmp4);
    return tmp1;
}

template<uint8_t rcon>
inline void aesGenkeySub(__m128i& xout0, __m128i& xout2)
{
    __m128i xout1 = _mm_aeskeygenassist_si128(xout2, rcon);
    xout1 = _mm_shuffle_epi32(xout1, 0xFF);
    xout0 = slXor(xout0);
    xout0 = _mm_xor_si128(xout0, xout1);
    xout1 = _mm_aeskeygenassist_si128(xout0, 0x00);
    xout1 = _mm_shuffle_epi32(xout1, 0xAA);
    xout2 = slXor(xout2);
    xout2 = _mm_xor_si128(xout2, xout1);
}

inline void aesGenkey(const __m128i* memory,
                      __m128i& k0,
                      __m128i& k1,
                      __m128i& k2,
                      __m128i& k3,
                      __m128i& k4,
                      __m128i& k5,
                      __m128i& k6,
                      __m128i& k7,
                      __m128i& k8,
                      __m128i& k9)
{
    __m128i xout0, xout2;

    xout0 = _mm_load_si128(memory);
    xout2 = _mm_load_si128(memory + 1);
    k0 = xout0;
    k1 = xout2;

    aesGenkeySub<0x01>(xout0, xout2);
    k2 = xout0;
    k3 = xout2;

    aesGenkeySub<0x02>(xout0, xout2);
    k4 = xout0;
    k5 = xout2;

    aesGenkeySub<0x04>(xout0, xout2);
    k6 = xout0;
    k7 = xout2;

    aesGenkeySub<0x08>(xout0, xout2);
    k8 = xout0;
    k9 = xout2;
}

inline void aesRound8(const __m128i& key,
                      __m128i& x0,
                      __m128i& x1,
                      __m128i& x2,
                      __m128i& x3,
                      __m128i& x4,
                      __m128i& x5,
                      __m128i& x6,
                      __m128i& x7)
{
    x0 = _mm_aesenc_si128(x0, key);
    x1 = _mm_aesenc_si128(x1, key);
    x2 = _mm_aesenc_si128(x2, key);
    x3 = _mm_aesenc_si128(x3, key);
    x4 = _mm_aesenc_si128(x4, key);
    x5 = _mm_aesenc_si128(x5, key);
    x6 = _mm_aesenc_si128(x6, key);
    x7 = _mm_aesenc_si128(x7, key);
}

inline void xorShift(__m128i& x0,
                      __m128i& x1,
                      __m128i& x2,
                      __m128i& x3,
                      __m128i& x4,
                      __m128i& x5,
                      __m128i& x6,
                      __m128i& x7)
{
    __m128i tmp0 = x0;
    x0 = _mm_xor_si128(x0, x1);
    x1 = _mm_xor_si128(x1, x2);
    x2 = _mm_xor_si128(x2, x3);
    x3 = _mm_xor_si128(x3, x4);
    x4 = _mm_xor_si128(x4, x5);
    x5 = _mm_xor_si128(x5, x6);
    x6 = _mm_xor_si128(x6, x7);
    x7 = _mm_xor_si128(x7, tmp0);
}

template<size_t MEMORY, size_t ITER, size_t POWVER>
void CnSlowHash<MEMORY,ITER,POWVER>::implodeScratchpadHard()
{
    __m128i x0, x1, x2, x3, x4, x5, x6, x7;
    __m128i k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

    aesGenkey(sPad.asXmm() + 2, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

    x0 = _mm_load_si128(sPad.asXmm() + 4);
    x1 = _mm_load_si128(sPad.asXmm() + 5);
    x2 = _mm_load_si128(sPad.asXmm() + 6);
    x3 = _mm_load_si128(sPad.asXmm() + 7);
    x4 = _mm_load_si128(sPad.asXmm() + 8);
    x5 = _mm_load_si128(sPad.asXmm() + 9);
    x6 = _mm_load_si128(sPad.asXmm() + 10);
    x7 = _mm_load_si128(sPad.asXmm() + 11);

    for (size_t i = 0; i < MEMORY / sizeof(__m128i); i +=8) {
        x0 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 0), x0);
        x1 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 1), x1);
        x2 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 2), x2);
        x3 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 3), x3);
        x4 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 4), x4);
        x5 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 5), x5);
        x6 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 6), x6);
        x7 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 7), x7);

        aesRound8(k0, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k1, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k2, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k3, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k4, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k5, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k6, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k7, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k8, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k9, x0, x1, x2, x3, x4, x5, x6, x7);

        if(POWVER > 0) {
            xorShift(x0, x1, x2, x3, x4, x5, x6, x7);
        }
    }

    for (size_t i = 0; POWVER > 0 && i < MEMORY / sizeof(__m128i); i +=8) {
        x0 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 0), x0);
        x1 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 1), x1);
        x2 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 2), x2);
        x3 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 3), x3);
        x4 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 4), x4);
        x5 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 5), x5);
        x6 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 6), x6);
        x7 = _mm_xor_si128(_mm_load_si128(lPad.asXmm() + i + 7), x7);

        aesRound8(k0, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k1, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k2, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k3, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k4, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k5, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k6, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k7, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k8, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k9, x0, x1, x2, x3, x4, x5, x6, x7);

        xorShift(x0, x1, x2, x3, x4, x5, x6, x7);
    }

    for (size_t i = 0; POWVER > 0 && i < 16; i++) {
        aesRound8(k0, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k1, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k2, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k3, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k4, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k5, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k6, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k7, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k8, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k9, x0, x1, x2, x3, x4, x5, x6, x7);

        xorShift(x0, x1, x2, x3, x4, x5, x6, x7);
    }

    _mm_store_si128(sPad.asXmm() + 4, x0);
    _mm_store_si128(sPad.asXmm() + 5, x1);
    _mm_store_si128(sPad.asXmm() + 6, x2);
    _mm_store_si128(sPad.asXmm() + 7, x3);
    _mm_store_si128(sPad.asXmm() + 8, x4);
    _mm_store_si128(sPad.asXmm() + 9, x5);
    _mm_store_si128(sPad.asXmm() + 10, x6);
    _mm_store_si128(sPad.asXmm() + 11, x7);
}

template<size_t MEMORY, size_t ITER, size_t POWVER>
void CnSlowHash<MEMORY,ITER,POWVER>::explodeScratchpadHard()
{
    __m128i x0, x1, x2, x3, x4, x5, x6, x7;
    __m128i k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

    aesGenkey(sPad.asXmm(), k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

    x0 = _mm_load_si128(sPad.asXmm() + 4);
    x1 = _mm_load_si128(sPad.asXmm() + 5);
    x2 = _mm_load_si128(sPad.asXmm() + 6);
    x3 = _mm_load_si128(sPad.asXmm() + 7);
    x4 = _mm_load_si128(sPad.asXmm() + 8);
    x5 = _mm_load_si128(sPad.asXmm() + 9);
    x6 = _mm_load_si128(sPad.asXmm() + 10);
    x7 = _mm_load_si128(sPad.asXmm() + 11);

    for (size_t i = 0; POWVER > 0 && i < 16; i++) {
        aesRound8(k0, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k1, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k2, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k3, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k4, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k5, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k6, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k7, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k8, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k9, x0, x1, x2, x3, x4, x5, x6, x7);

        xorShift(x0, x1, x2, x3, x4, x5, x6, x7);
    }

    for(size_t i = 0; i < MEMORY / sizeof(__m128i); i += 8) {
        aesRound8(k0, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k1, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k2, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k3, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k4, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k5, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k6, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k7, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k8, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k9, x0, x1, x2, x3, x4, x5, x6, x7);

        _mm_store_si128(lPad.asXmm() + i + 0, x0);
        _mm_store_si128(lPad.asXmm() + i + 1, x1);
        _mm_store_si128(lPad.asXmm() + i + 2, x2);
        _mm_store_si128(lPad.asXmm() + i + 3, x3);
        _mm_store_si128(lPad.asXmm() + i + 4, x4);
        _mm_store_si128(lPad.asXmm() + i + 5, x5);
        _mm_store_si128(lPad.asXmm() + i + 6, x6);
        _mm_store_si128(lPad.asXmm() + i + 7, x7);
    }
}

#ifdef BUILD32
inline uint64_t _umul128(uint64_t multiplier, uint64_t multiplicand, uint64_t *product_hi)
{
    // multiplier   = Ab = a * 2^32 + b
    // multiplicand = cd = c * 2^32 + d
    // Ab * cd = a * c * 2^64 + (a * d + b * c) * 2^32 + b * d
    uint64_t a = multiplier >> 32;
    uint64_t b = multiplier & 0xFFFFFFFF;
    uint64_t c = multiplicand >> 32;
    uint64_t d = multiplicand & 0xFFFFFFFF;

    uint64_t ac = a * c;
    uint64_t ad = a * d;
    uint64_t bc = b * c;
    uint64_t bd = b * d;

    uint64_t adbc = ad + bc;
    uint64_t adbc_carry = adbc < ad ? 1 : 0;

    // multiplier * multiplicand = product_hi * 2^64 + product_lo
    uint64_t product_lo = bd + (adbc << 32);
    uint64_t product_lo_carry = product_lo < bd ? 1 : 0;
    *product_hi = ac + (adbc >> 32) + (adbc_carry << 32) + product_lo_carry;

    return product_lo;
}
#else
#if !defined(HAS_WIN_INTRIN_API)
inline uint64_t _umul128(uint64_t a, uint64_t b, uint64_t* hi)
{
    unsigned __int128 r = (unsigned __int128)a * (unsigned __int128)b;
    *hi = r >> 64;
    return (uint64_t)r;
}
#endif
#endif

extern "C" void blake256Hash(uint8_t *, const uint8_t *, uint64_t);
extern "C" void groestl(const unsigned char*, unsigned long long, unsigned char*);
extern "C" size_t jhHash(int, const unsigned char *, unsigned long long, unsigned char *);
extern "C" size_t skeinHash(int, const unsigned char *, unsigned long, unsigned char *);

inline uint64_t xmmExtract64(__m128i x)
{
#ifdef BUILD32
    uint64_t r = uint32_t(_mm_cvtsi128_si32(_mm_shuffle_epi32(x, _MM_SHUFFLE(1,1,1,1))));
	r <<= 32;
	r |= uint32_t(_mm_cvtsi128_si32(x));
	return r;
#else
    return _mm_cvtsi128_si64(x);
#endif
}

template<size_t MEMORY, size_t ITER, size_t POWVER>
void CnSlowHash<MEMORY,ITER,POWVER>::hardwareHash(const void* in, size_t len, void* out)
{
    keccak((const uint8_t *)in, len, sPad.asByte(), 200);

    explodeScratchpadHard();

    uint64_t* h0 = sPad.asUqword();

    uint64_t al0 = h0[0] ^ h0[4];
    uint64_t ah0 = h0[1] ^ h0[5];
    __m128i bx0 = _mm_set_epi64x(h0[3] ^ h0[7], h0[2] ^ h0[6]);

    uint64_t idx0 = h0[0] ^ h0[4];

    // Optim - 90% time boundary
    for(size_t i = 0; i < ITER; i++) {
        __m128i cx;
        cx = _mm_load_si128(scratchpadPtr(idx0).asXmm());

        cx = _mm_aesenc_si128(cx, _mm_set_epi64x(ah0, al0));

        _mm_store_si128(scratchpadPtr(idx0).asXmm(), _mm_xor_si128(bx0, cx));
        idx0 = xmmExtract64(cx);
        bx0 = cx;

        uint64_t hi, lo, cl, ch;
        cl = scratchpadPtr(idx0).asUqword(0);
        ch = scratchpadPtr(idx0).asUqword(1);

        lo = _umul128(idx0, cl, &hi);

        al0 += hi;
        ah0 += lo;
        scratchpadPtr(idx0).asUqword(0) = al0;
        scratchpadPtr(idx0).asUqword(1) = ah0;
        ah0 ^= ch;
        al0 ^= cl;
        idx0 = al0;

        if(POWVER > 0) {
            int64_t n  = scratchpadPtr(idx0).asQword(0);
            int32_t d  = scratchpadPtr(idx0).asDword(2);
            int64_t q = n / (d | 5);
            scratchpadPtr(idx0).asQword(0) = n ^ q;
            idx0 = d ^ q;
        }
    }

    implodeScratchpadHard();

    keccakf(sPad.asUqword(), 24);

    switch(sPad.asByte(0) & 3) {
    case 0:
        blake256Hash((uint8_t *)out, sPad.asByte(), 200);
        break;
    case 1:
        groestl(sPad.asByte(), 200 * 8, (uint8_t*)out);
        break;
    case 2:
        jhHash(32 * 8, sPad.asByte(), 8 * 200, (uint8_t *)out);
        break;
    case 3:
        skeinHash(8 * 32, sPad.asByte(), 8 * 200, (uint8_t *)out);
        break;
    }
}

template class CnSlowHash<2*1024*1024, 0x80000, 0>;
template class CnSlowHash<4*1024*1024, 0x40000, 1>;

#endif


/*
AES Tables Implementation is
---------------------------------------------------------------------------
Copyright (c) 1998-2013, Brian Gladman, Worcester, UK. All rights reserved.

The redistribution and use of this software (with or without changes)
is allowed without the payment of fees or royalties provided that:

  source code distributions include the above copyright notice, this
  list of conditions and the following disclaimer;

  binary distributions include the above copyright notice, this list
  of conditions and the following disclaimer in their documentation.

This software is provided 'as is' with no explicit or implied warranties
in respect of its operation, including, but not limited to, correctness
and fitness for purpose.
---------------------------------------------------------------------------
*/

#if !defined(_LP64) && !defined(_WIN64)
#define BUILD32
#endif

#define sAesData(w) {\
	w(0x63), w(0x7c), w(0x77), w(0x7b), w(0xf2), w(0x6b), w(0x6f), w(0xc5),\
	w(0x30), w(0x01), w(0x67), w(0x2b), w(0xfe), w(0xd7), w(0xab), w(0x76),\
	w(0xca), w(0x82), w(0xc9), w(0x7d), w(0xfa), w(0x59), w(0x47), w(0xf0),\
	w(0xad), w(0xd4), w(0xa2), w(0xaf), w(0x9c), w(0xa4), w(0x72), w(0xc0),\
	w(0xb7), w(0xfd), w(0x93), w(0x26), w(0x36), w(0x3f), w(0xf7), w(0xcc),\
	w(0x34), w(0xa5), w(0xe5), w(0xf1), w(0x71), w(0xd8), w(0x31), w(0x15),\
	w(0x04), w(0xc7), w(0x23), w(0xc3), w(0x18), w(0x96), w(0x05), w(0x9a),\
	w(0x07), w(0x12), w(0x80), w(0xe2), w(0xeb), w(0x27), w(0xb2), w(0x75),\
	w(0x09), w(0x83), w(0x2c), w(0x1a), w(0x1b), w(0x6e), w(0x5a), w(0xa0),\
	w(0x52), w(0x3b), w(0xd6), w(0xb3), w(0x29), w(0xe3), w(0x2f), w(0x84),\
	w(0x53), w(0xd1), w(0x00), w(0xed), w(0x20), w(0xfc), w(0xb1), w(0x5b),\
	w(0x6a), w(0xcb), w(0xbe), w(0x39), w(0x4a), w(0x4c), w(0x58), w(0xcf),\
	w(0xd0), w(0xef), w(0xaa), w(0xfb), w(0x43), w(0x4d), w(0x33), w(0x85),\
	w(0x45), w(0xf9), w(0x02), w(0x7f), w(0x50), w(0x3c), w(0x9f), w(0xa8),\
	w(0x51), w(0xa3), w(0x40), w(0x8f), w(0x92), w(0x9d), w(0x38), w(0xf5),\
	w(0xbc), w(0xb6), w(0xda), w(0x21), w(0x10), w(0xff), w(0xf3), w(0xd2),\
	w(0xcd), w(0x0c), w(0x13), w(0xec), w(0x5f), w(0x97), w(0x44), w(0x17),\
	w(0xc4), w(0xa7), w(0x7e), w(0x3d), w(0x64), w(0x5d), w(0x19), w(0x73),\
	w(0x60), w(0x81), w(0x4f), w(0xdc), w(0x22), w(0x2a), w(0x90), w(0x88),\
	w(0x46), w(0xee), w(0xb8), w(0x14), w(0xde), w(0x5e), w(0x0b), w(0xdb),\
	w(0xe0), w(0x32), w(0x3a), w(0x0a), w(0x49), w(0x06), w(0x24), w(0x5c),\
	w(0xc2), w(0xd3), w(0xac), w(0x62), w(0x91), w(0x95), w(0xe4), w(0x79),\
	w(0xe7), w(0xc8), w(0x37), w(0x6d), w(0x8d), w(0xd5), w(0x4e), w(0xa9),\
	w(0x6c), w(0x56), w(0xf4), w(0xea), w(0x65), w(0x7a), w(0xae), w(0x08),\
	w(0xba), w(0x78), w(0x25), w(0x2e), w(0x1c), w(0xa6), w(0xb4), w(0xc6),\
	w(0xe8), w(0xdd), w(0x74), w(0x1f), w(0x4b), w(0xbd), w(0x8b), w(0x8a),\
	w(0x70), w(0x3e), w(0xb5), w(0x66), w(0x48), w(0x03), w(0xf6), w(0x0e),\
	w(0x61), w(0x35), w(0x57), w(0xb9), w(0x86), w(0xc1), w(0x1d), w(0x9e),\
	w(0xe1), w(0xf8), w(0x98), w(0x11), w(0x69), w(0xd9), w(0x8e), w(0x94),\
	w(0x9b), w(0x1e), w(0x87), w(0xe9), w(0xce), w(0x55), w(0x28), w(0xdf),\
	w(0x8c), w(0xa1), w(0x89), w(0x0d), w(0xbf), w(0xe6), w(0x42), w(0x68),\
	w(0x41), w(0x99), w(0x2d), w(0x0f), w(0xb0), w(0x54), w(0xbb), w(0x16) }

#define SAES_WPOLY           0x011b

#define sAesB2w(b0, b1, b2, b3)                 \
    (((uint32_t)(b3) << 24)                     \
    | ((uint32_t)(b2) << 16)                    \
    | ((uint32_t)(b1) << 8)                     \
    | (b0))

#define sAesF2(x)   ((x<<1) ^ (((x>>7) & 1) * SAES_WPOLY))
#define sAesF3(x)   (sAesF2(x) ^ x)
#define sAesH0(x)   (x)

#define sAesU0(p) sAesB2w(sAesF2(p),          p,          p, sAesF3(p))
#define sAesU1(p) sAesB2w(sAesF3(p), sAesF2(p),          p,          p)
#define sAesU2(p) sAesB2w(         p, sAesF3(p), sAesF2(p),          p)
#define sAesU3(p) sAesB2w(         p,          p, sAesF3(p), sAesF2(p))

alignas(16) const uint32_t sAesTable[4][256] =
{
    sAesData(sAesU0),
    sAesData(sAesU1),
    sAesData(sAesU2),
    sAesData(sAesU3)
};
alignas(16) extern const uint8_t sAesSbox[256] = sAesData(sAesH0);

#ifdef _NOEXCEPT
#define noexcept _NOEXCEPT
#endif

struct AesData
{
    uint64_t v64x0;
    uint64_t v64x1;

    inline void load(const CnSptr mem)
    {
        v64x0 = mem.asUqword(0);
        v64x1 = mem.asUqword(1);
    }

    inline void xOrLoad(const CnSptr mem)
    {
        v64x0 ^= mem.asUqword(0);
        v64x1 ^= mem.asUqword(1);
    }

    inline void write(CnSptr mem)
    {
        mem.asUqword(0) = v64x0;
        mem.asUqword(1) = v64x1;
    }

    inline AesData & operator=(const AesData & rhs) noexcept
    {
        v64x0 = rhs.v64x0;
        v64x1 = rhs.v64x1;
        return *this;
    }

    inline AesData & operator^=(const AesData & rhs) noexcept
    {
        v64x0 ^= rhs.v64x0;
        v64x1 ^= rhs.v64x1;
        return *this;
    }

    inline AesData & operator^=(uint32_t rhs) noexcept
    {
        uint64_t t = (uint64_t(rhs) << 32) | uint64_t(rhs);
        v64x0 ^= t;
        v64x1 ^= t;
        return *this;
    }

    inline void getQuad(uint32_t& x0,
                        uint32_t& x1,
                        uint32_t& x2,
                        uint32_t& x3)
    {
        x0 = v64x0;
        x1 = v64x0 >> 32;
        x2 = v64x1;
        x3 = v64x1 >> 32;
    }

    inline void setQuad(uint32_t x0,
                        uint32_t x1,
                        uint32_t x2,
                        uint32_t x3)
    {
        v64x0 = uint64_t(x0) | uint64_t(x1) << 32;
        v64x1 = uint64_t(x2) | uint64_t(x3) << 32;
    }
};

inline uint32_t subWord(uint32_t key)
{
    return (sAesSbox[key >> 24 ] << 24)
            | (sAesSbox[(key >> 16) & 0xff] << 16 )
            | (sAesSbox[(key >> 8)  & 0xff] << 8  )
            | sAesSbox[key & 0xff];
}

#if defined(__clang__) || defined(__arm__) || defined(__aarch64__)
inline uint32_t rotr(uint32_t value, uint32_t amount)
{
    return (value >> amount)
            | (value << ((32 - amount) & 31));
}
#else
inline uint32_t rotr(uint32_t value, uint32_t amount)
{
    return _rotr(value, amount);
}
#endif

// slXor(a1 a2 a3 a4) = a1 (a2^a1) (a3^a2^a1) (a4^a3^a2^a1)
inline void slXor(AesData & x)
{
    uint32_t x0, x1, x2, x3;
    x.getQuad(x0, x1, x2, x3);
    x1 ^= x0;
    x2 ^= x1;
    x3 ^= x2;
    x.setQuad(x0, x1, x2, x3);
}

template<uint8_t rcon>
inline void softAesGenKeySub(AesData & xout0, AesData &  xout2)
{
    slXor(xout0);
    xout0 ^= rotr(subWord(xout2.v64x1 >> 32), 8) ^ rcon;
    slXor(xout2);
    xout2 ^= subWord(xout0.v64x1 >> 32);
}

inline void aesGenKey(const CnSptr memory,
                      AesData & k0,
                      AesData & k1,
                      AesData & k2,
                      AesData & k3,
                      AesData & k4,
                      AesData & k5,
                      AesData & k6,
                      AesData & k7,
                      AesData & k8,
                      AesData & k9)
{
    AesData xout0, xout2;

    xout0.load(memory);
    xout2.load(memory.offset(16));
    k0 = xout0;
    k1 = xout2;

    softAesGenKeySub<0x01>(xout0, xout2);
    k2 = xout0;
    k3 = xout2;

    softAesGenKeySub<0x02>(xout0, xout2);
    k4 = xout0;
    k5 = xout2;

    softAesGenKeySub<0x04>(xout0, xout2);
    k6 = xout0;
    k7 = xout2;

    softAesGenKeySub<0x08>(xout0, xout2);
    k8 = xout0;
    k9 = xout2;
}

inline void aesRound(AesData & val, const AesData & key)
{
    uint32_t x0, x1, x2, x3;
    val.getQuad(x0, x1, x2, x3);
    val.setQuad(sAesTable[0][x0 & 0xff] ^ sAesTable[1][(x1 >> 8) & 0xff]
                        ^ sAesTable[2][(x2 >> 16) & 0xff] ^ sAesTable[3][x3 >> 24],
                sAesTable[0][x1 & 0xff] ^ sAesTable[1][(x2 >> 8) & 0xff]
                        ^ sAesTable[2][(x3 >> 16) & 0xff] ^ sAesTable[3][x0 >> 24],
                sAesTable[0][x2 & 0xff] ^ sAesTable[1][(x3 >> 8) & 0xff]
                        ^ sAesTable[2][(x0 >> 16) & 0xff] ^ sAesTable[3][x1 >> 24],
                sAesTable[0][x3 & 0xff] ^ sAesTable[1][(x0 >> 8) & 0xff]
                        ^ sAesTable[2][(x1 >> 16) & 0xff] ^ sAesTable[3][x2 >> 24]);

    val ^= key;
}

inline void aesRound8(const AesData & key,
                      AesData & x0,
                      AesData & x1,
                      AesData & x2,
                      AesData & x3,
                      AesData & x4,
                      AesData & x5,
                      AesData & x6,
                      AesData & x7)
{
    aesRound(x0, key);
    aesRound(x1, key);
    aesRound(x2, key);
    aesRound(x3, key);
    aesRound(x4, key);
    aesRound(x5, key);
    aesRound(x6, key);
    aesRound(x7, key);
}

inline void xOrShift(AesData & x0, AesData & x1, AesData & x2, AesData & x3, AesData & x4, AesData & x5,
                      AesData & x6, AesData & x7)
{
    AesData tmp = x0;
    x0 ^= x1;
    x1 ^= x2;
    x2 ^= x3;
    x3 ^= x4;
    x4 ^= x5;
    x5 ^= x6;
    x6 ^= x7;
    x7 ^= tmp;
}

template<size_t MEMORY, size_t ITER, size_t POWVER>
void CnSlowHash<MEMORY, ITER, POWVER>::implodeScratchpadSoft()
{
    AesData x0, x1, x2, x3, x4, x5, x6, x7;
    AesData k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

    aesGenKey(sPad.asUqword() + 4,
              k0,
              k1,
              k2,
              k3,
              k4,
              k5,
              k6,
              k7,
              k8,
              k9);

    x0.load(sPad.asUqword() + 8);
    x1.load(sPad.asUqword() + 10);
    x2.load(sPad.asUqword() + 12);
    x3.load(sPad.asUqword() + 14);
    x4.load(sPad.asUqword() + 16);
    x5.load(sPad.asUqword() + 18);
    x6.load(sPad.asUqword() + 20);
    x7.load(sPad.asUqword() + 22);

    for (size_t i = 0; i < MEMORY / sizeof(uint64_t); i += 16) {
        x0.xOrLoad(lPad.asUqword() + i + 0);
        x1.xOrLoad(lPad.asUqword() + i + 2);
        x2.xOrLoad(lPad.asUqword() + i + 4);
        x3.xOrLoad(lPad.asUqword() + i + 6);
        x4.xOrLoad(lPad.asUqword() + i + 8);
        x5.xOrLoad(lPad.asUqword() + i + 10);
        x6.xOrLoad(lPad.asUqword() + i + 12);
        x7.xOrLoad(lPad.asUqword() + i + 14);

        aesRound8(k0, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k1, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k2, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k3, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k4, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k5, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k6, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k7, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k8, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k9, x0, x1, x2, x3, x4, x5, x6, x7);

        if (POWVER > 0) {
            xOrShift(x0, x1, x2, x3, x4, x5, x6, x7);
        }
    }

    // Note, this loop is only executed if POWVER > 0
    for (size_t i = 0; POWVER > 0 && i < MEMORY / sizeof(uint64_t); i += 16) {
        x0.xOrLoad(lPad.asUqword() + i + 0);
        x1.xOrLoad(lPad.asUqword() + i + 2);
        x2.xOrLoad(lPad.asUqword() + i + 4);
        x3.xOrLoad(lPad.asUqword() + i + 6);
        x4.xOrLoad(lPad.asUqword() + i + 8);
        x5.xOrLoad(lPad.asUqword() + i + 10);
        x6.xOrLoad(lPad.asUqword() + i + 12);
        x7.xOrLoad(lPad.asUqword() + i + 14);

        aesRound8(k0, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k1, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k2, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k3, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k4, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k5, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k6, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k7, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k8, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k9, x0, x1, x2, x3, x4, x5, x6, x7);

        xOrShift(x0, x1, x2, x3, x4, x5, x6, x7);
    }

    // Note, this loop is only executed if POWVER > 0
    for (size_t i = 0; POWVER > 0 && i < 16; i++) {
        aesRound8(k0, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k1, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k2, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k3, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k4, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k5, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k6, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k7, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k8, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k9, x0, x1, x2, x3, x4, x5, x6, x7);

        xOrShift(x0, x1, x2, x3, x4, x5, x6, x7);
    }

    x0.write(sPad.asUqword() + 8);
    x1.write(sPad.asUqword() + 10);
    x2.write(sPad.asUqword() + 12);
    x3.write(sPad.asUqword() + 14);
    x4.write(sPad.asUqword() + 16);
    x5.write(sPad.asUqword() + 18);
    x6.write(sPad.asUqword() + 20);
    x7.write(sPad.asUqword() + 22);
}

template<size_t MEMORY, size_t ITER, size_t POWVER>
void CnSlowHash<MEMORY,ITER,POWVER>::explodeScratchpadSoft()
{
    AesData x0, x1, x2, x3, x4, x5, x6, x7;
    AesData k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

    aesGenKey(sPad.asUqword(),
              k0,
              k1,
              k2,
              k3,
              k4,
              k5,
              k6,
              k7,
              k8,
              k9);

    x0.load(sPad.asUqword() + 8);
    x1.load(sPad.asUqword() + 10);
    x2.load(sPad.asUqword() + 12);
    x3.load(sPad.asUqword() + 14);
    x4.load(sPad.asUqword() + 16);
    x5.load(sPad.asUqword() + 18);
    x6.load(sPad.asUqword() + 20);
    x7.load(sPad.asUqword() + 22);

    // Note, this loop is only executed if POWVER > 0
    for (size_t i = 0; POWVER > 0 && i < 16; i++) {
        aesRound8(k0, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k1, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k2, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k3, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k4, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k5, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k6, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k7, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k8, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k9, x0, x1, x2, x3, x4, x5, x6, x7);

        xOrShift(x0, x1, x2, x3, x4, x5, x6, x7);
    }

    for (size_t i = 0; i < MEMORY / sizeof(uint64_t); i += 16) {
        aesRound8(k0, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k1, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k2, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k3, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k4, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k5, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k6, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k7, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k8, x0, x1, x2, x3, x4, x5, x6, x7);
        aesRound8(k9, x0, x1, x2, x3, x4, x5, x6, x7);

        x0.write(lPad.asUqword() + i + 0);
        x1.write(lPad.asUqword() + i + 2);
        x2.write(lPad.asUqword() + i + 4);
        x3.write(lPad.asUqword() + i + 6);
        x4.write(lPad.asUqword() + i + 8);
        x5.write(lPad.asUqword() + i + 10);
        x6.write(lPad.asUqword() + i + 12);
        x7.write(lPad.asUqword() + i + 14);
    }
}

extern "C" void blake256Hash(uint8_t *, const uint8_t *, uint64_t);
extern "C" void groestl(const unsigned char*, unsigned long long, unsigned char*);
extern "C" size_t jhHash(int, const unsigned char *, unsigned long long, unsigned char *);

template<size_t MEMORY, size_t ITER, size_t POWVER>
void CnSlowHash<MEMORY,ITER,POWVER>::softwareHash(const void* in, size_t len, void* out)
{
    keccak((const uint8_t *)in, len, sPad.asByte(), 200);

    explodeScratchpadSoft();

    uint64_t* h0 = sPad.asUqword();

    AesData ax;
    ax.v64x0 = h0[0] ^ h0[4];
    ax.v64x1 = h0[1] ^ h0[5];

    AesData bx;
    bx.v64x0 = h0[2] ^ h0[6];
    bx.v64x1 = h0[3] ^ h0[7];

    AesData cx;
    CnSptr idx = scratchpadPtr(ax.v64x0);

    for(size_t i = 0; i < ITER/2; i++) {
        uint64_t hi, lo;
        cx.load(idx);

        aesRound(cx, ax);

        bx ^= cx;
        bx.write(idx);
        idx = scratchpadPtr(cx.v64x0);
        bx.load(idx);

        lo = _umul128(cx.v64x0, bx.v64x0, &hi);

        ax.v64x0 += hi;
        ax.v64x1 += lo;
        ax.write(idx);

        ax ^= bx;
        idx = scratchpadPtr(ax.v64x0);
        if(POWVER > 0) {
            int64_t n  = idx.asQword(0);
            int32_t d  = idx.asDword(2);

#if defined(__arm__)
            asm volatile ("nop"); //Fix for RasPi3 ARM - maybe needed on armv8
#endif

            int64_t q = n / (d | 5);
            idx.asQword(0) = n ^ q;
            idx = scratchpadPtr(d ^ q);
        }

        bx.load(idx);

        aesRound(bx, ax);

        cx ^= bx;
        cx.write(idx);
        idx = scratchpadPtr(bx.v64x0);
        cx.load(idx);

        lo = _umul128(bx.v64x0, cx.v64x0, &hi);

        ax.v64x0 += hi;
        ax.v64x1 += lo;
        ax.write(idx);
        ax ^= cx;
        idx = scratchpadPtr(ax.v64x0);
        if(POWVER > 0) {
            int64_t n  = idx.asQword(0); // read bytes 0 - 7
            int32_t d  = idx.asDword(2); // read bytes 8 - 11

#if defined(__arm__)
            asm volatile ("nop"); //Fix for RasPi3 ARM - maybe needed on armv8
#endif

            int64_t q = n / (d | 5);
            idx.asQword(0) = n ^ q;
            idx = scratchpadPtr(d ^ q);
        }
    }

    implodeScratchpadSoft();

    keccakf(sPad.asUqword(), 24);

    switch(sPad.asByte(0) & 3) {
    case 0:
        blake256Hash((uint8_t *)out, sPad.asByte(), 200);
        break;
    case 1:
        groestl(sPad.asByte(), 200 * 8, (uint8_t*)out);
        break;
    case 2:
        jhHash(32 * 8, sPad.asByte(), 8 * 200, (uint8_t *)out);
        break;
    case 3:
        skeinHash(8 * 32, sPad.asByte(), 8 * 200, (uint8_t *)out);
        break;
    }
}
