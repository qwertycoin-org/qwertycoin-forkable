// Copyright (c) 2017, SUMOKOIN
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
// Parts of this file are originally copyright (c) 2014-2017, The Monero Project
// Parts of this file are originally copyright (c) 2012-2013, The Cryptonote developers

#pragma once

#include <cassert>
#include <cinttypes>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <boost/align/aligned_alloc.hpp>

#if defined(_WIN32) || defined(_WIN64)
#include <malloc.h>
#include <intrin.h>
#define HAS_WIN_INTRIN_API
#endif

// Note HAS_INTEL_HW and HAS_ARM_HW only mean we can emit the AES instructions
// check CPU support for the hardware AES encryption has to be done at runtime
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X86) || defined(_M_X64)
#ifdef __GNUC__
#include <x86intrin.h>
#if !defined __APPLE__
#pragma GCC target("aes")
#endif
#if !defined(HAS_WIN_INTRIN_API)
#include <cpuid.h>
#endif // !defined(HAS_WIN_INTRIN_API)
#endif // __GNUC__
#define HAS_INTEL_HW
#endif

#if defined(__aarch64__)
#pragma GCC target("+crypto")
#include <sys/auxv.h>
#include <asm/hwcap.h>
#include <arm_neon.h>
#define HAS_ARM_HW
#endif

#ifdef HAS_INTEL_HW
inline void cpuid(uint32_t eax, int32_t ecx, int32_t val[4])
{
    val[0] = 0;
    val[1] = 0;
    val[2] = 0;
    val[3] = 0;

#if defined(HAS_WIN_INTRIN_API)
    __cpuidex(val, eax, ecx);
#else
    __cpuid_count(eax, ecx, val[0], val[1], val[2], val[3]);
#endif
}

inline bool hwCheckAes()
{
    int32_t cpuInfo[4];
    cpuid(1, 0, cpuInfo);

    return (cpuInfo[2] & (1 << 25)) != 0;
}
#endif

#ifdef HAS_ARM_HW
inline bool hwCheckAes()
{
    return (getauxval(AT_HWCAP) & HWCAP_AES) != 0;
}
#endif

#if !defined(HAS_INTEL_HW) && !defined(HAS_ARM_HW)
inline bool hwCheckAes()
{
    return false;
}
#endif

// This cruft avoids casting-galore and allows us not to worry about sizeof(void*)
class CnSptr
{
public:
    CnSptr() : base_ptr(nullptr) { }
    CnSptr(uint64_t *ptr) { base_ptr = ptr; }
    CnSptr(uint32_t *ptr) { base_ptr = ptr; }
    CnSptr(uint8_t *ptr) { base_ptr = ptr; }
#ifdef HAS_INTEL_HW
    CnSptr(__m128i *ptr) { base_ptr = ptr; }
#endif

    inline void set(void *ptr) { base_ptr = ptr; }
    inline CnSptr offset(size_t i) { return reinterpret_cast<uint8_t *>(base_ptr) + i; }
    inline const CnSptr offset(size_t i) const
    {
        return reinterpret_cast<uint8_t *>(base_ptr) + i;
    }

    inline void *asVoid() { return base_ptr; }
    inline uint8_t &asByte(size_t i) { return *(reinterpret_cast<uint8_t *>(base_ptr) + i); }
    inline uint8_t *asByte() { return reinterpret_cast<uint8_t *>(base_ptr); }
    inline uint64_t &asUqword(size_t i) { return *(reinterpret_cast<uint64_t *>(base_ptr) + i); }
    inline const uint64_t &asUqword(size_t i) const
    {
        return *(reinterpret_cast<uint64_t *>(base_ptr) + i);
    }
    inline uint64_t *asUqword() { return reinterpret_cast<uint64_t *>(base_ptr); }
    inline const uint64_t *asUqword() const { return reinterpret_cast<uint64_t *>(base_ptr); }
    inline int64_t &asQword(size_t i) { return *(reinterpret_cast<int64_t *>(base_ptr) + i); }
    inline int32_t &asDword(size_t i) { return *(reinterpret_cast<int32_t *>(base_ptr) + i); }
    inline uint32_t &asUdword(size_t i) { return *(reinterpret_cast<uint32_t *>(base_ptr) + i); }
    inline const uint32_t &asUdword(size_t i) const
    {
        return *(reinterpret_cast<uint32_t *>(base_ptr) + i);
    }
#ifdef HAS_INTEL_HW
    inline __m128i *asXmm() { return reinterpret_cast<__m128i *>(base_ptr); }
#endif
private:
    void *base_ptr;
};

template<size_t MEMORY, size_t ITER, size_t POWVER>
class CnSlowHash;
using cnPowHashV1 = CnSlowHash<2 * 1024 * 1024, 0x80000, 0>;
using cnPowHashV2 = CnSlowHash<4 * 1024 * 1024, 0x40000, 1>;

template<size_t MEMORY, size_t ITER, size_t POWVER>
class CnSlowHash
{
public:
    CnSlowHash() : borrowedPad(false)
    {
        lPad.set(boost::alignment::aligned_alloc(4096, MEMORY));
        sPad.set(boost::alignment::aligned_alloc(4096, 4096));
    }

    CnSlowHash(CnSlowHash &&other) noexcept
        : lPad(other.lPad.asByte()), sPad(other.sPad.asByte()), borrowedPad(other.borrowedPad)
    {
        other.lPad.set(nullptr);
        other.sPad.set(nullptr);
    }

    // Factory function enabling to temporally turn v2 object into v1
    // It is caller's responsibility to ensure that v2 object is not hashing at the same time!!
    static cnPowHashV1 makeBorrowed(cnPowHashV2 &t)
    {
        return cnPowHashV1(t.lPad.asVoid(), t.sPad.asVoid());
    }

    CnSlowHash &operator=(CnSlowHash &&other) noexcept
    {
        if (this == &other)
            return *this;

        freeMem();
        lPad.set(other.lPad.asVoid());
        sPad.set(other.sPad.asVoid());
        borrowedPad = other.borrowedPad;

        return *this;
    }

    // Copying is going to be really inefficient
    CnSlowHash(const CnSlowHash &other) = delete;
    CnSlowHash &operator=(const CnSlowHash &other) = delete;

    ~CnSlowHash() { freeMem(); }

    void hash(const void *in, size_t len, void *out)
    {
        if (hwCheckAes() && !checkOverride())
            hardwareHash(in, len, out);
        else
            softwareHash(in, len, out);
    }

    void softwareHash(const void *in, size_t len, void *out);

#if !defined(HAS_INTEL_HW) && !defined(HAS_ARM_HW)
    inline void hardwareHash(const void *in, size_t len, void *out) { assert(false); }
#else
    void hardwareHash(const void *in, size_t len, void *out);
#endif

private:
    static constexpr size_t MASK = ((MEMORY - 1) >> 4) << 4;
    friend cnPowHashV1;
    friend cnPowHashV2;

    // Constructor enabling v1 Hash to borrow v2's buffer
    CnSlowHash(void *lPtr, void *sPtr)
    {
        lPad.set(lPtr);
        sPad.set(sPtr);
        borrowedPad = true;
    }

    inline bool checkOverride()
    {
        const char *env = getenv("SUMO_USE_SOFTWARE_AES");
        if (!env) {
            return false;
        } else if (!strcmp(env, "0") || !strcmp(env, "no")) {
            return false;
        } else {
            return true;
        }
    }

    inline void freeMem()
    {
        if (!borrowedPad) {
            if (lPad.asVoid() != nullptr) {
                boost::alignment::aligned_free(lPad.asVoid());
            }
            if (lPad.asVoid() != nullptr) {
                boost::alignment::aligned_free(sPad.asVoid());
            }
        }

        lPad.set(nullptr);
        sPad.set(nullptr);
    }

    inline CnSptr scratchpadPtr(uint32_t idx)
    {
        return lPad.asByte() + (idx & MASK);
    }

#if !defined(HAS_INTEL_HW) && !defined(HAS_ARM_HW)
    inline void explodeScratchpadHard() { assert(false); }
    inline void implodeScratchpadHard() { assert(false); }
#else
    void explodeScratchpadHard();
    void implodeScratchpadHard();
#endif

    void explodeScratchpadSoft();
    void implodeScratchpadSoft();

    CnSptr lPad;
    CnSptr sPad;
    bool borrowedPad;
};

extern template class CnSlowHash<2 * 1024 * 1024, 0x80000, 0>;
extern template class CnSlowHash<4 * 1024 * 1024, 0x40000, 1>;
