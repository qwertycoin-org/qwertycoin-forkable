// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2018-2021, The Qwertycoin Group.
// Copyright (c) 2018 ryo-project
//
// This file is part of Qwertycoin.
//
// Qwertycoin is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Qwertycoin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Qwertycoin.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/param.h>

#if defined(__ANDROID__)
#include <byteswap.h>
#endif

#if defined(_MSC_VER)
#include <stdlib.h>

#define inline __inline

static inline uint32_t rol32(uint32_t x, int r)
{
    static_assert(sizeof(uint32_t) == sizeof(unsigned int), "this code assumes 32-bit integers");
    return _rotl(x, r);
}

static inline uint64_t rol64(uint64_t x, int r)
{
    return _rotl64(x, r);
}
#else
static inline uint32_t rol32(uint32_t x, int r)
{
    return (x << (r & 31)) | (x >> (-r & 31));
}

static inline uint64_t rol64(uint64_t x, int r)
{
    return (x << (r & 63)) | (x >> (-r & 63));
}
#endif

static inline uint64_t hiDword(uint64_t uValue)
{
    return uValue >> 32;
}

static inline uint64_t loDword(uint64_t uValue)
{
    return uValue & 0xFFFFFFFF;
}

static inline uint64_t mul128(uint64_t uMultiplier,
                              uint64_t uMultiplicand,
                              uint64_t *uProductHigh)
{
    // uMultiplier   = ab = a * 2^32 + b
    // uMultiplicand = cd = c * 2^32 + d
    // ab * cd = a * c * 2^64 + (a * d + b * c) * 2^32 + b * d
    uint64_t a = hiDword(uMultiplier);
    uint64_t b = loDword(uMultiplier);
    uint64_t c = hiDword(uMultiplicand);
    uint64_t d = loDword(uMultiplicand);

    uint64_t ac = a * c;
    uint64_t ad = a * d;
    uint64_t bc = b * c;
    uint64_t bd = b * d;

    uint64_t adbc = ad + bc;
    uint64_t adbcCarry = adbc < ad ? 1 : 0;

    // uMultiplier * uMultiplicand = uProductHigh * 2^64 + productLo
    uint64_t productLo = bd + (adbc << 32);
    uint64_t productLoCarry = productLo < bd ? 1 : 0;
    *uProductHigh = ac + (adbc >> 32) + (adbcCarry << 32) + productLoCarry;

    assert(ac <= *uProductHigh);

    return productLo;
}

static inline uint64_t divWithReminder(uint64_t uDividend,
                                       uint32_t uDivisor,
                                       uint32_t *uRemainder)
{
    uDividend |= ((uint64_t)*uRemainder) << 32;
    *uRemainder = uDividend % uDivisor;

    return uDividend / uDivisor;
}

// Long division with 2^32 base
static inline uint32_t div128b32(uint64_t uDividendHigh,
                                 uint64_t uDividendLow,
                                 uint32_t uDivisor,
                                 uint64_t *uQuotientHigh,
                                 uint64_t *uQuotientLow)
{
    uint64_t uDividendDWords[4];
    uint32_t uRemainder = 0;

    uDividendDWords[3] = hiDword(uDividendHigh);
    uDividendDWords[2] = loDword(uDividendHigh);
    uDividendDWords[1] = hiDword(uDividendLow);
    uDividendDWords[0] = loDword(uDividendLow);

    *uQuotientHigh  = divWithReminder(uDividendDWords[3], uDivisor, &uRemainder) << 32;
    *uQuotientHigh |= divWithReminder(uDividendDWords[2], uDivisor, &uRemainder);
    *uQuotientLow  = divWithReminder(uDividendDWords[1], uDivisor, &uRemainder) << 32;
    *uQuotientLow |= divWithReminder(uDividendDWords[0], uDivisor, &uRemainder);

    return uRemainder;
}

#define IDENT32(x) ((uint32_t) (x))
#define IDENT64(x) ((uint64_t) (x))

#define SWAP32(x) ((((uint32_t) (x) & 0x000000ff) << 24) | \
    (((uint32_t) (x) & 0x0000ff00) <<  8) | \
    (((uint32_t) (x) & 0x00ff0000) >>  8) | \
    (((uint32_t) (x) & 0xff000000) >> 24))
#define SWAP64(x) ((((uint64_t) (x) & 0x00000000000000ff) << 56) | \
    (((uint64_t) (x) & 0x000000000000ff00) << 40) | \
    (((uint64_t) (x) & 0x0000000000ff0000) << 24) | \
    (((uint64_t) (x) & 0x00000000ff000000) <<  8) | \
    (((uint64_t) (x) & 0x000000ff00000000) >>  8) | \
    (((uint64_t) (x) & 0x0000ff0000000000) >> 24) | \
    (((uint64_t) (x) & 0x00ff000000000000) >> 40) | \
    (((uint64_t) (x) & 0xff00000000000000) >> 56))

static inline uint32_t ident32(uint32_t x)
{
    return x;
}

static inline uint64_t ident64(uint64_t x)
{
    return x;
}

#ifndef __OpenBSD__
#  if defined(__ANDROID__) && defined(__swap32) && !defined(swap32)
#      define swap32 __swap32
#  elif !defined(swap32)
static inline uint32_t swap32(uint32_t x)
{
    x = ((x & 0x00ff00ff) << 8) | ((x & 0xff00ff00) >> 8);
    return (x << 16) | (x >> 16);
}
#  endif
#  if defined(__ANDROID__) && defined(__swap64) && !defined(swap64)
#      define swap64 __swap64
#  elif !defined(swap64)
static inline uint64_t swap64(uint64_t x)
{
    x = ((x & 0x00ff00ff00ff00ff) << 8) | ((x & 0xff00ff00ff00ff00) >> 8);
    x = ((x & 0x0000ffff0000ffff) << 16) | ((x & 0xffff0000ffff0000) >> 16);
    return (x << 32) | (x >> 32);
}
#  endif
#endif /* __OpenBSD__ */

#if defined(__GNUC__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif
static inline void memInplaceIdent(void *mem UNUSED, size_t n UNUSED) { }
#undef UNUSED

static inline void memInplaceSwap32(void *memory, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        ((uint32_t *) memory)[i] = swap32(((const uint32_t *) memory)[i]);
    }
}

static inline void memInplaceSwap64(void *memory, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        ((uint64_t *) memory)[i] = swap64(((const uint64_t *) memory)[i]);
    }
}

static inline void memCpyIdent32(void *destination, const void *source, size_t n)
{
    memcpy(destination, source, 4 * n);
}

static inline void memCpyIdent64(void *destination, const void *source, size_t n)
{
    memcpy(destination, source, 8 * n);
}

static inline void memCpySwap32(void *destination, const void *source, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        ((uint32_t *) destination)[i] = swap32(((const uint32_t *) source)[i]);
    }
}

static inline void memCpySwap64(void *destination, const void *source, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        ((uint64_t *) destination)[i] = swap64(((const uint64_t *) source)[i]);
    }
}

/**
 * @brief Calculate ln(p) of Poisson distribution
 * https://github.com/ryo-currency/ryo-writeups/blob/master/poisson-writeup.md
 * Original idea : https://stackoverflow.com/questions/30156803/implementing-poisson-distribution-in-c
 * Using logarithms avoids dealing with very large (uBlockAmount!) and very small (p < 10^-44) numbers
 *
 * dLambda       - dLambda parameter - in our case, how many blocks, on average,
 *                 you would expect to see in the interval
 * uBlockAmount  - uBlockAmount parameter - in our case, how many blocks we have actually seen
 *                 !!! uBlockAmount must not be zero
 *
 * @param dLambda
 * @param uBlockAmount
 * @return ln(p)
 */
static inline double calcPoissonLn(double dLambda, uint64_t uBlockAmount)
{
    double dLogX = -dLambda + uBlockAmount * log(dLambda);
    do {
        dLogX -= log(uBlockAmount); // this can be tabulated
    } while (--uBlockAmount > 0);

    return dLogX;
}

#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN) || !defined(BIG_ENDIAN)
static_assert(false, "BYTE_ORDER is undefined. Perhaps, GNU extensions are not enabled!");
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
#define SWAP32LE IDENT32
#define SWAP32BE SWAP32
#define swap32le ident32
#define swap32be swap32
#define memInplaceSwap32le memInplaceIdent
#define memInplaceSwap32be memInplaceSwap32
#define memCpySwap32le memCpyIdent32
#define memCpySwap32be memCpySwap32
#define SWAP64LE IDENT64
#define SWAP64BE SWAP64
#define swap64le ident64
#define swap64be swap64
#define memInplaceSwap64le memInplaceIdent
#define memInplaceSwap64be memInplaceSwap64
#define memCpySwap64le memCpyIdent64
#define memCpySwap64be memCpySwap64
#endif

#if BYTE_ORDER == BIG_ENDIAN
#define SWAP32LE IDENT32
#define SWAP32BE SWAP32
#define swap32le ident32
#define swap32be swap32
#define memInplaceSwap32le memInplaceIdent
#define memInplaceSwap32be memInplaceSwap32
#define memCpySwap32le memCpyIdent32
#define memCpySwap32be memCpySwap32
#define SWAP64LE IDENT64
#define SWAP64BE SWAP64
#define swap64le ident64
#define swap64be swap64
#define memInplaceSwap64le memInplaceIdent
#define memInplaceSwap64be memInplaceSwap64
#define memCpySwap64le memCpyIdent64
#define memCpySwap64be memCpySwap64
#endif
