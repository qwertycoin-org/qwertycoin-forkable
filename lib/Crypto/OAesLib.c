/*
 * ---------------------------------------------------------------------------
 * OpenAES License
 * ---------------------------------------------------------------------------
 * Copyright (c) 2012, Nabil S. Al Ramli, www.nalramli.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * ---------------------------------------------------------------------------
 */
#include <stddef.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// OS X, FreeBSD, and OpenBSD don't need malloc.h
#if !defined(__APPLE__) && !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__DragonFly__)
#include <malloc.h>
#endif

// ANDROID, FreeBSD, and OpenBSD also don't need timeb.h
#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__ANDROID__)
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

#ifdef _WIN32
#include <process.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include <Crypto/OAesLib.h>

#ifdef OAES_HAVE_ISAAC
#include "rand.h"
#endif // OAES_HAVE_ISAAC

#define OAES_RKEY_LEN 4
#define OAES_COL_LEN 4
#define OAES_ROUND_BASE 7

// the block is padded
#define OAES_FLAG_PAD 0x01

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif /* min */

// "OAES<8-bit header version><8-bit type><16-bit options><8-bit flags><56-bit reserved>"
static uint8_t oAesHeader[OAES_BLOCK_SIZE] =
{
    0x4f, 0x41, 0x45, 0x53, 0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static uint8_t oAesGf8[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 };

static uint8_t oAesSubByteValue[16][16] =
{
//      0,    1,    2,    3,    4,    5,    6,    7,
//      8,    9,    a,    b,    c,    d,    e,    f,
    /*0*/
    {
        0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
        0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76
    },
    /*1*/
    {
        0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
        0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0
    },
    /*2*/
    {
        0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc,
        0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15
    },
    /*3*/
    {
        0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a,
        0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75
    },
    /*4*/
    {
        0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
        0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84
    },
    /*5*/
    {
        0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
        0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf
    },
    /*6*/
    {
        0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
        0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8
    },
    /*7*/
    {
        0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
        0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2
    },
    /*8*/
    {
        0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17,
        0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73
    },
    /*9*/
    {
        0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
        0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb
    },
    /*a*/
    {
        0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
        0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79
    },
    /*b*/
    {
        0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9,
        0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08
    },
    /*c*/
    {
        0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6,
        0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a
    },
    /*d*/
    {
        0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
        0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e
    },
    /*e*/
    {
        0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94,
        0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf
    },
    /*f*/
    {
        0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68,
        0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
    }
};

static uint8_t oAesInvSubByteValue[16][16] =
{
//      0,    1,    2,    3,    4,    5,    6,    7,
//      8,    9,    a,    b,    c,    d,    e,    f,
    /*0*/
    {
        0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38,
        0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb
    },
    /*1*/
    {
        0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87,
        0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb
    },
    /*2*/
    {
        0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d,
        0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e
    },
    /*3*/
    {
        0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2,
        0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25
    },
    /*4*/
    {
        0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
        0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92
    },
    /*5*/
    {
        0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda,
        0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84
    },
    /*6*/
    {
        0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a,
        0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06
    },
    /*7*/
    {
        0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
        0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b
    },
    /*8*/
    {
        0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea,
        0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73
    },
    /*9*/
    {
        0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85,
        0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e
    },
    /*a*/
    {
        0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89,
        0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b
    },
    /*b*/
    {
        0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20,
        0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4
    },
    /*c*/
    {
        0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31,
        0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f
    },
    /*d*/
    {
        0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
        0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef
    },
    /*e*/
    {
        0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0,
        0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61
    },
    /*f*/
    {
        0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26,
        0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
    },
};

static uint8_t oAesGfMul2[16][16] =
{
//      0,    1,    2,    3,    4,    5,    6,    7,
//      8,    9,    a,    b,    c,    d,    e,    f,
    /*0*/
    {
        0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e,
        0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e
    },
    /*1*/
    {
        0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e,
        0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e
    },
    /*2*/
    {
        0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e,
        0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e
    },
    /*3*/
    {
        0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e,
        0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e
    },
    /*4*/
    {
        0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e,
        0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e
    },
    /*5*/
    {
        0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae,
        0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbe
    },
    /*6*/
    {
        0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce,
        0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde
    },
    /*7*/
    {
        0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee,
        0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa, 0xfc, 0xfe
    },
    /*8*/
    {
        0x1b, 0x19, 0x1f, 0x1d, 0x13, 0x11, 0x17, 0x15,
        0x0b, 0x09, 0x0f, 0x0d, 0x03, 0x01, 0x07, 0x05
    },
    /*9*/
    {
        0x3b, 0x39, 0x3f, 0x3d, 0x33, 0x31, 0x37, 0x35,
        0x2b, 0x29, 0x2f, 0x2d, 0x23, 0x21, 0x27, 0x25
    },
    /*a*/
    {
        0x5b, 0x59, 0x5f, 0x5d, 0x53, 0x51, 0x57, 0x55,
        0x4b, 0x49, 0x4f, 0x4d, 0x43, 0x41, 0x47, 0x45
    },
    /*b*/
    {
        0x7b, 0x79, 0x7f, 0x7d, 0x73, 0x71, 0x77, 0x75,
        0x6b, 0x69, 0x6f, 0x6d, 0x63, 0x61, 0x67, 0x65
    },
    /*c*/
    {
        0x9b, 0x99, 0x9f, 0x9d, 0x93, 0x91, 0x97, 0x95,
        0x8b, 0x89, 0x8f, 0x8d, 0x83, 0x81, 0x87, 0x85
    },
    /*d*/
    {
        0xbb, 0xb9, 0xbf, 0xbd, 0xb3, 0xb1, 0xb7, 0xb5,
        0xab, 0xa9, 0xaf, 0xad, 0xa3, 0xa1, 0xa7, 0xa5
    },
    /*e*/
    {
        0xdb, 0xd9, 0xdf, 0xdd, 0xd3, 0xd1, 0xd7, 0xd5,
        0xcb, 0xc9, 0xcf, 0xcd, 0xc3, 0xc1, 0xc7, 0xc5
    },
    /*f*/
    {
        0xfb, 0xf9, 0xff, 0xfd, 0xf3, 0xf1, 0xf7, 0xf5,
        0xeb, 0xe9, 0xef, 0xed, 0xe3, 0xe1, 0xe7, 0xe5
    },
};

static uint8_t oAesGfMul3[16][16] =
{
//      0,    1,    2,    3,    4,    5,    6,    7,
//      8,    9,    a,    b,    c,    d,    e,    f,
    /*0*/
    {
        0x00, 0x03, 0x06, 0x05, 0x0c, 0x0f, 0x0a, 0x09,
        0x18, 0x1b, 0x1e, 0x1d, 0x14, 0x17, 0x12, 0x11
    },
    /*1*/
    {
        0x30, 0x33, 0x36, 0x35, 0x3c, 0x3f, 0x3a, 0x39,
        0x28, 0x2b, 0x2e, 0x2d, 0x24, 0x27, 0x22, 0x21
    },
    /*2*/
    {
        0x60, 0x63, 0x66, 0x65, 0x6c, 0x6f, 0x6a, 0x69,
        0x78, 0x7b, 0x7e, 0x7d, 0x74, 0x77, 0x72, 0x71
    },
    /*3*/
    {
        0x50, 0x53, 0x56, 0x55, 0x5c, 0x5f, 0x5a, 0x59,
        0x48, 0x4b, 0x4e, 0x4d, 0x44, 0x47, 0x42, 0x41
    },
    /*4*/
    {
        0xc0, 0xc3, 0xc6, 0xc5, 0xcc, 0xcf, 0xca, 0xc9,
        0xd8, 0xdb, 0xde, 0xdd, 0xd4, 0xd7, 0xd2, 0xd1
    },
    /*5*/
    {
        0xf0, 0xf3, 0xf6, 0xf5, 0xfc, 0xff, 0xfa, 0xf9,
        0xe8, 0xeb, 0xee, 0xed, 0xe4, 0xe7, 0xe2, 0xe1
    },
    /*6*/
    {
        0xa0, 0xa3, 0xa6, 0xa5, 0xac, 0xaf, 0xaa, 0xa9,
        0xb8, 0xbb, 0xbe, 0xbd, 0xb4, 0xb7, 0xb2, 0xb1
    },
    /*7*/
    {
        0x90, 0x93, 0x96, 0x95, 0x9c, 0x9f, 0x9a, 0x99,
        0x88, 0x8b, 0x8e, 0x8d, 0x84, 0x87, 0x82, 0x81
    },
    /*8*/
    {
        0x9b, 0x98, 0x9d, 0x9e, 0x97, 0x94, 0x91, 0x92,
        0x83, 0x80, 0x85, 0x86, 0x8f, 0x8c, 0x89, 0x8a
    },
    /*9*/
    {
        0xab, 0xa8, 0xad, 0xae, 0xa7, 0xa4, 0xa1, 0xa2,
        0xb3, 0xb0, 0xb5, 0xb6, 0xbf, 0xbc, 0xb9, 0xba
    },
    /*a*/
    {
        0xfb, 0xf8, 0xfd, 0xfe, 0xf7, 0xf4, 0xf1, 0xf2,
        0xe3, 0xe0, 0xe5, 0xe6, 0xef, 0xec, 0xe9, 0xea
    },
    /*b*/
    {
        0xcb, 0xc8, 0xcd, 0xce, 0xc7, 0xc4, 0xc1, 0xc2,
        0xd3, 0xd0, 0xd5, 0xd6, 0xdf, 0xdc, 0xd9, 0xda
    },
    /*c*/
    {
        0x5b, 0x58, 0x5d, 0x5e, 0x57, 0x54, 0x51, 0x52,
        0x43, 0x40, 0x45, 0x46, 0x4f, 0x4c, 0x49, 0x4a
    },
    /*d*/
    {
        0x6b, 0x68, 0x6d, 0x6e, 0x67, 0x64, 0x61, 0x62,
        0x73, 0x70, 0x75, 0x76, 0x7f, 0x7c, 0x79, 0x7a
    },
    /*e*/
    {
        0x3b, 0x38, 0x3d, 0x3e, 0x37, 0x34, 0x31, 0x32,
        0x23, 0x20, 0x25, 0x26, 0x2f, 0x2c, 0x29, 0x2a
    },
    /*f*/
    {
        0x0b, 0x08, 0x0d, 0x0e, 0x07, 0x04, 0x01, 0x02,
        0x13, 0x10, 0x15, 0x16, 0x1f, 0x1c, 0x19, 0x1a
    },
};

static uint8_t oAesGfMul9[16][16] =
{
//      0,    1,    2,    3,    4,    5,    6,    7,
//      8,    9,    a,    b,    c,    d,    e,    f,
    /*0*/
    {
        0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
        0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77
    },
    /*1*/
    {
        0x90, 0x99, 0x82, 0x8b, 0xb4, 0xbd, 0xa6, 0xaf,
        0xd8, 0xd1, 0xca, 0xc3, 0xfc, 0xf5, 0xee, 0xe7
    },
    /*2*/
    {
        0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
        0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c
    },
    /*3*/
    {
        0xab, 0xa2, 0xb9, 0xb0, 0x8f, 0x86, 0x9d, 0x94,
        0xe3, 0xea, 0xf1, 0xf8, 0xc7, 0xce, 0xd5, 0xdc
    },
    /*4*/
    {
        0x76, 0x7f, 0x64, 0x6d, 0x52, 0x5b, 0x40, 0x49,
        0x3e, 0x37, 0x2c, 0x25, 0x1a, 0x13, 0x08, 0x01
    },
    /*5*/
    {
        0xe6, 0xef, 0xf4, 0xfd, 0xc2, 0xcb, 0xd0, 0xd9,
        0xae, 0xa7, 0xbc, 0xb5, 0x8a, 0x83, 0x98, 0x91
    },
    /*6*/
    {
        0x4d, 0x44, 0x5f, 0x56, 0x69, 0x60, 0x7b, 0x72,
        0x05, 0x0c, 0x17, 0x1e, 0x21, 0x28, 0x33, 0x3a
    },
    /*7*/
    {
        0xdd, 0xd4, 0xcf, 0xc6, 0xf9, 0xf0, 0xeb, 0xe2,
        0x95, 0x9c, 0x87, 0x8e, 0xb1, 0xb8, 0xa3, 0xaa
    },
    /*8*/
    {
        0xec, 0xe5, 0xfe, 0xf7, 0xc8, 0xc1, 0xda, 0xd3,
        0xa4, 0xad, 0xb6, 0xbf, 0x80, 0x89, 0x92, 0x9b
    },
    /*9*/
    {
        0x7c, 0x75, 0x6e, 0x67, 0x58, 0x51, 0x4a, 0x43,
        0x34, 0x3d, 0x26, 0x2f, 0x10, 0x19, 0x02, 0x0b
    },
    /*a*/
    {
        0xd7, 0xde, 0xc5, 0xcc, 0xf3, 0xfa, 0xe1, 0xe8,
        0x9f, 0x96, 0x8d, 0x84, 0xbb, 0xb2, 0xa9, 0xa0
    },
    /*b*/
    {
        0x47, 0x4e, 0x55, 0x5c, 0x63, 0x6a, 0x71, 0x78,
        0x0f, 0x06, 0x1d, 0x14, 0x2b, 0x22, 0x39, 0x30
    },
    /*c*/
    {
        0x9a, 0x93, 0x88, 0x81, 0xbe, 0xb7, 0xac, 0xa5,
        0xd2, 0xdb, 0xc0, 0xc9, 0xf6, 0xff, 0xe4, 0xed
    },
    /*d*/
    {
        0x0a, 0x03, 0x18, 0x11, 0x2e, 0x27, 0x3c, 0x35,
        0x42, 0x4b, 0x50, 0x59, 0x66, 0x6f, 0x74, 0x7d
    },
    /*e*/
    {
        0xa1, 0xa8, 0xb3, 0xba, 0x85, 0x8c, 0x97, 0x9e,
        0xe9, 0xe0, 0xfb, 0xf2, 0xcd, 0xc4, 0xdf, 0xd6
    },
    /*f*/
    {
        0x31, 0x38, 0x23, 0x2a, 0x15, 0x1c, 0x07, 0x0e,
        0x79, 0x70, 0x6b, 0x62, 0x5d, 0x54, 0x4f, 0x46
    },
};

static uint8_t oAesGfMulB[16][16] =
{
//      0,    1,    2,    3,    4,    5,    6,    7,
//      8,    9,    a,    b,    c,    d,    e,    f,
    /*0*/
    {
        0x00, 0x0b, 0x16, 0x1d, 0x2c, 0x27, 0x3a, 0x31,
        0x58, 0x53, 0x4e, 0x45, 0x74, 0x7f, 0x62, 0x69
    },
    /*1*/
    {
        0xb0, 0xbb, 0xa6, 0xad, 0x9c, 0x97, 0x8a, 0x81,
        0xe8, 0xe3, 0xfe, 0xf5, 0xc4, 0xcf, 0xd2, 0xd9
    },
    /*2*/
    {
        0x7b, 0x70, 0x6d, 0x66, 0x57, 0x5c, 0x41, 0x4a,
        0x23, 0x28, 0x35, 0x3e, 0x0f, 0x04, 0x19, 0x12
    },
    /*3*/
    {
        0xcb, 0xc0, 0xdd, 0xd6, 0xe7, 0xec, 0xf1, 0xfa,
        0x93, 0x98, 0x85, 0x8e, 0xbf, 0xb4, 0xa9, 0xa2
    },
    /*4*/
    {
        0xf6, 0xfd, 0xe0, 0xeb, 0xda, 0xd1, 0xcc, 0xc7,
        0xae, 0xa5, 0xb8, 0xb3, 0x82, 0x89, 0x94, 0x9f
    },
    /*5*/
    {
        0x46, 0x4d, 0x50, 0x5b, 0x6a, 0x61, 0x7c, 0x77,
        0x1e, 0x15, 0x08, 0x03, 0x32, 0x39, 0x24, 0x2f
    },
    /*6*/
    {
        0x8d, 0x86, 0x9b, 0x90, 0xa1, 0xaa, 0xb7, 0xbc,
        0xd5, 0xde, 0xc3, 0xc8, 0xf9, 0xf2, 0xef, 0xe4
    },
    /*7*/
    {
        0x3d, 0x36, 0x2b, 0x20, 0x11, 0x1a, 0x07, 0x0c,
        0x65, 0x6e, 0x73, 0x78, 0x49, 0x42, 0x5f, 0x54
    },
    /*8*/
    {
        0xf7, 0xfc, 0xe1, 0xea, 0xdb, 0xd0, 0xcd, 0xc6,
        0xaf, 0xa4, 0xb9, 0xb2, 0x83, 0x88, 0x95, 0x9e
    },
    /*9*/
    {
        0x47, 0x4c, 0x51, 0x5a, 0x6b, 0x60, 0x7d, 0x76,
        0x1f, 0x14, 0x09, 0x02, 0x33, 0x38, 0x25, 0x2e
    },
    /*a*/
    {
        0x8c, 0x87, 0x9a, 0x91, 0xa0, 0xab, 0xb6, 0xbd,
        0xd4, 0xdf, 0xc2, 0xc9, 0xf8, 0xf3, 0xee, 0xe5
    },
    /*b*/
    {
        0x3c, 0x37, 0x2a, 0x21, 0x10, 0x1b, 0x06, 0x0d,
        0x64, 0x6f, 0x72, 0x79, 0x48, 0x43, 0x5e, 0x55
    },
    /*c*/
    {
        0x01, 0x0a, 0x17, 0x1c, 0x2d, 0x26, 0x3b, 0x30,
        0x59, 0x52, 0x4f, 0x44, 0x75, 0x7e, 0x63, 0x68
    },
    /*d*/
    {
        0xb1, 0xba, 0xa7, 0xac, 0x9d, 0x96, 0x8b, 0x80,
        0xe9, 0xe2, 0xff, 0xf4, 0xc5, 0xce, 0xd3, 0xd8
    },
    /*e*/
    {
        0x7a, 0x71, 0x6c, 0x67, 0x56, 0x5d, 0x40, 0x4b,
        0x22, 0x29, 0x34, 0x3f, 0x0e, 0x05, 0x18, 0x13
    },
    /*f*/
    {
        0xca, 0xc1, 0xdc, 0xd7, 0xe6, 0xed, 0xf0, 0xfb,
        0x92, 0x99, 0x84, 0x8f, 0xbe, 0xb5, 0xa8, 0xa3
    },
};

static uint8_t oAesGfMulD[16][16] =
{
//      0,    1,    2,    3,    4,    5,    6,    7,
//      8,    9,    a,    b,    c,    d,    e,    f,
    /*0*/
    {
        0x00, 0x0d, 0x1a, 0x17, 0x34, 0x39, 0x2e, 0x23,
        0x68, 0x65, 0x72, 0x7f, 0x5c, 0x51, 0x46, 0x4b
    },
    /*1*/
    {
        0xd0, 0xdd, 0xca, 0xc7, 0xe4, 0xe9, 0xfe, 0xf3,
        0xb8, 0xb5, 0xa2, 0xaf, 0x8c, 0x81, 0x96, 0x9b
    },
    /*2*/
    {
        0xbb, 0xb6, 0xa1, 0xac, 0x8f, 0x82, 0x95, 0x98,
        0xd3, 0xde, 0xc9, 0xc4, 0xe7, 0xea, 0xfd, 0xf0
    },
    /*3*/
    {
        0x6b, 0x66, 0x71, 0x7c, 0x5f, 0x52, 0x45, 0x48,
        0x03, 0x0e, 0x19, 0x14, 0x37, 0x3a, 0x2d, 0x20
    },
    /*4*/
    {
        0x6d, 0x60, 0x77, 0x7a, 0x59, 0x54, 0x43, 0x4e,
        0x05, 0x08, 0x1f, 0x12, 0x31, 0x3c, 0x2b, 0x26
    },
    /*5*/
    {
        0xbd, 0xb0, 0xa7, 0xaa, 0x89, 0x84, 0x93, 0x9e,
        0xd5, 0xd8, 0xcf, 0xc2, 0xe1, 0xec, 0xfb, 0xf6
    },
    /*6*/
    {
        0xd6, 0xdb, 0xcc, 0xc1, 0xe2, 0xef, 0xf8, 0xf5,
        0xbe, 0xb3, 0xa4, 0xa9, 0x8a, 0x87, 0x90, 0x9d
    },
    /*7*/
    {
        0x06, 0x0b, 0x1c, 0x11, 0x32, 0x3f, 0x28, 0x25,
        0x6e, 0x63, 0x74, 0x79, 0x5a, 0x57, 0x40, 0x4d
    },
    /*8*/
    {
        0xda, 0xd7, 0xc0, 0xcd, 0xee, 0xe3, 0xf4, 0xf9,
        0xb2, 0xbf, 0xa8, 0xa5, 0x86, 0x8b, 0x9c, 0x91
    },
    /*9*/
    {
        0x0a, 0x07, 0x10, 0x1d, 0x3e, 0x33, 0x24, 0x29,
        0x62, 0x6f, 0x78, 0x75, 0x56, 0x5b, 0x4c, 0x41
    },
    /*a*/
    {
        0x61, 0x6c, 0x7b, 0x76, 0x55, 0x58, 0x4f, 0x42,
        0x09, 0x04, 0x13, 0x1e, 0x3d, 0x30, 0x27, 0x2a
    },
    /*b*/
    {
        0xb1, 0xbc, 0xab, 0xa6, 0x85, 0x88, 0x9f, 0x92,
        0xd9, 0xd4, 0xc3, 0xce, 0xed, 0xe0, 0xf7, 0xfa
    },
    /*c*/
    {
        0xb7, 0xba, 0xad, 0xa0, 0x83, 0x8e, 0x99, 0x94,
        0xdf, 0xd2, 0xc5, 0xc8, 0xeb, 0xe6, 0xf1, 0xfc
    },
    /*d*/
    {
        0x67, 0x6a, 0x7d, 0x70, 0x53, 0x5e, 0x49, 0x44,
        0x0f, 0x02, 0x15, 0x18, 0x3b, 0x36, 0x21, 0x2c
    },
    /*e*/
    {
        0x0c, 0x01, 0x16, 0x1b, 0x38, 0x35, 0x22, 0x2f,
        0x64, 0x69, 0x7e, 0x73, 0x50, 0x5d, 0x4a, 0x47
    },
    /*f*/
    {
        0xdc, 0xd1, 0xc6, 0xcb, 0xe8, 0xe5, 0xf2, 0xff,
        0xb4, 0xb9, 0xae, 0xa3, 0x80, 0x8d, 0x9a, 0x97
    },
};

static uint8_t oAesGfMulE[16][16] = {
//      0,    1,    2,    3,    4,    5,    6,    7,
//      8,    9,    a,    b,    c,    d,    e,    f,
    /*0*/
    {
        0x00, 0x0e, 0x1c, 0x12, 0x38, 0x36, 0x24, 0x2a,
        0x70, 0x7e, 0x6c, 0x62, 0x48, 0x46, 0x54, 0x5a
    },
    /*1*/
    {
        0xe0, 0xee, 0xfc, 0xf2, 0xd8, 0xd6, 0xc4, 0xca,
        0x90, 0x9e, 0x8c, 0x82, 0xa8, 0xa6, 0xb4, 0xba
    },
    /*2*/
    {
        0xdb, 0xd5, 0xc7, 0xc9, 0xe3, 0xed, 0xff, 0xf1,
        0xab, 0xa5, 0xb7, 0xb9, 0x93, 0x9d, 0x8f, 0x81
    },
    /*3*/
    {
        0x3b, 0x35, 0x27, 0x29, 0x03, 0x0d, 0x1f, 0x11,
        0x4b, 0x45, 0x57, 0x59, 0x73, 0x7d, 0x6f, 0x61
    },
    /*4*/
    {
        0xad, 0xa3, 0xb1, 0xbf, 0x95, 0x9b, 0x89, 0x87,
        0xdd, 0xd3, 0xc1, 0xcf, 0xe5, 0xeb, 0xf9, 0xf7
    },
    /*5*/
    {
        0x4d, 0x43, 0x51, 0x5f, 0x75, 0x7b, 0x69, 0x67,
        0x3d, 0x33, 0x21, 0x2f, 0x05, 0x0b, 0x19, 0x17
    },
    /*6*/
    {
        0x76, 0x78, 0x6a, 0x64, 0x4e, 0x40, 0x52, 0x5c,
        0x06, 0x08, 0x1a, 0x14, 0x3e, 0x30, 0x22, 0x2c
    },
    /*7*/
    {
        0x96, 0x98, 0x8a, 0x84, 0xae, 0xa0, 0xb2, 0xbc,
        0xe6, 0xe8, 0xfa, 0xf4, 0xde, 0xd0, 0xc2, 0xcc
    },
    /*8*/
    {
        0x41, 0x4f, 0x5d, 0x53, 0x79, 0x77, 0x65, 0x6b,
        0x31, 0x3f, 0x2d, 0x23, 0x09, 0x07, 0x15, 0x1b
    },
    /*9*/
    {
        0xa1, 0xaf, 0xbd, 0xb3, 0x99, 0x97, 0x85, 0x8b,
        0xd1, 0xdf, 0xcd, 0xc3, 0xe9, 0xe7, 0xf5, 0xfb
    },
    /*a*/
    {
        0x9a, 0x94, 0x86, 0x88, 0xa2, 0xac, 0xbe, 0xb0,
        0xea, 0xe4, 0xf6, 0xf8, 0xd2, 0xdc, 0xce, 0xc0
    },
    /*b*/
    {
        0x7a, 0x74, 0x66, 0x68, 0x42, 0x4c, 0x5e, 0x50,
        0x0a, 0x04, 0x16, 0x18, 0x32, 0x3c, 0x2e, 0x20
    },
    /*c*/
    {
        0xec, 0xe2, 0xf0, 0xfe, 0xd4, 0xda, 0xc8, 0xc6,
        0x9c, 0x92, 0x80, 0x8e, 0xa4, 0xaa, 0xb8, 0xb6
    },
    /*d*/
    {
        0x0c, 0x02, 0x10, 0x1e, 0x34, 0x3a, 0x28, 0x26,
        0x7c, 0x72, 0x60, 0x6e, 0x44, 0x4a, 0x58, 0x56
    },
    /*e*/
    {
        0x37, 0x39, 0x2b, 0x25, 0x0f, 0x01, 0x13, 0x1d,
        0x47, 0x49, 0x5b, 0x55, 0x7f, 0x71, 0x63, 0x6d
    },
    /*f*/
    {
        0xd7, 0xd9, 0xcb, 0xc5, 0xef, 0xe1, 0xf3, 0xfd,
        0xa7, 0xa9, 0xbb, 0xb5, 0x9f, 0x91, 0x83, 0x8d
    },
};

static OAES_RET oAesSubByte(uint8_t *byte)
{
    size_t x;
    size_t y;

    if (NULL == byte) {
        return OAES_RET_ARG1;
    }

    x = y = *byte;
    x &= 0x0f;
    y &= 0xf0;
    y >>= 4;
    *byte = oAesSubByteValue[y][x];

    return OAES_RET_SUCCESS;
}

static OAES_RET oAesInvSubByte(uint8_t *byte)
{
    size_t x;
    size_t y;

    if (NULL == byte) {
        return OAES_RET_ARG1;
    }

    x = y = *byte;
    x &= 0x0f;
    y &= 0xf0;
    y >>= 4;
    *byte = oAesInvSubByteValue[y][x];

    return OAES_RET_SUCCESS;
}

static OAES_RET oAesWordRotLeft(uint8_t *word)
{
    uint8_t _temp[OAES_COL_LEN];

    if (NULL == word) {
        return OAES_RET_ARG1;
    }

    memcpy(_temp, word + 1, OAES_COL_LEN - 1);
    _temp[OAES_COL_LEN - 1] = word[0];
    memcpy(word, _temp, OAES_COL_LEN);

    return OAES_RET_SUCCESS;
}

static OAES_RET oAesShiftRows(uint8_t *block)
{
    uint8_t temp[OAES_BLOCK_SIZE];

    if (NULL == block) {
        return OAES_RET_ARG1;
    }

    temp[0x00] = block[0x00];
    temp[0x01] = block[0x05];
    temp[0x02] = block[0x0a];
    temp[0x03] = block[0x0f];
    temp[0x04] = block[0x04];
    temp[0x05] = block[0x09];
    temp[0x06] = block[0x0e];
    temp[0x07] = block[0x03];
    temp[0x08] = block[0x08];
    temp[0x09] = block[0x0d];
    temp[0x0a] = block[0x02];
    temp[0x0b] = block[0x07];
    temp[0x0c] = block[0x0c];
    temp[0x0d] = block[0x01];
    temp[0x0e] = block[0x06];
    temp[0x0f] = block[0x0b];
    memcpy(block, temp, OAES_BLOCK_SIZE);

    return OAES_RET_SUCCESS;
}

static OAES_RET oAesInvShiftRows(uint8_t *block)
{
    uint8_t temp[OAES_BLOCK_SIZE];

    if (NULL == block) {
        return OAES_RET_ARG1;
    }

    temp[0x00] = block[0x00];
    temp[0x01] = block[0x0d];
    temp[0x02] = block[0x0a];
    temp[0x03] = block[0x07];
    temp[0x04] = block[0x04];
    temp[0x05] = block[0x01];
    temp[0x06] = block[0x0e];
    temp[0x07] = block[0x0b];
    temp[0x08] = block[0x08];
    temp[0x09] = block[0x05];
    temp[0x0a] = block[0x02];
    temp[0x0b] = block[0x0f];
    temp[0x0c] = block[0x0c];
    temp[0x0d] = block[0x09];
    temp[0x0e] = block[0x06];
    temp[0x0f] = block[0x03];
    memcpy(block, temp, OAES_BLOCK_SIZE);

    return OAES_RET_SUCCESS;
}

static uint8_t oAesGfMul(uint8_t left, uint8_t right)
{
    size_t x;
    size_t y;

    x = y = left;
    x &= 0x0f;
    y &= 0xf0;
    y >>= 4;

    switch (right) {
    case 0x02:
        return oAesGfMul2[y][x];
        break;
    case 0x03:
        return oAesGfMul3[y][x];
        break;
    case 0x09:
        return oAesGfMul9[y][x];
        break;
    case 0x0b:
        return oAesGfMulB[y][x];
        break;
    case 0x0d:
        return oAesGfMulD[y][x];
        break;
    case 0x0e:
        return oAesGfMulE[y][x];
        break;
    default:
        return left;
        break;
    }
}

static OAES_RET oAesMixCols(uint8_t *word)
{
    uint8_t temp[OAES_COL_LEN];

    if (NULL == word) {
        return OAES_RET_ARG1;
    }

    temp[0] = oAesGfMul(word[0], 0x02) ^ oAesGfMul(word[1], 0x03) ^ word[2] ^ word[3];
    temp[1] = word[0] ^ oAesGfMul(word[1], 0x02) ^ oAesGfMul(word[2], 0x03) ^ word[3];
    temp[2] = word[0] ^ word[1] ^ oAesGfMul(word[2], 0x02) ^ oAesGfMul(word[3], 0x03);
    temp[3] = oAesGfMul(word[0], 0x03) ^ word[1] ^ word[2] ^ oAesGfMul(word[3], 0x02);
    memcpy(word, temp, OAES_COL_LEN);

    return OAES_RET_SUCCESS;
}

static OAES_RET oAesInvMixCols(uint8_t *word)
{
    uint8_t temp[OAES_COL_LEN];

    if (NULL == word) {
        return OAES_RET_ARG1;
    }

    temp[0] = oAesGfMul(word[0], 0x0e)
            ^ oAesGfMul(word[1], 0x0b)
            ^ oAesGfMul(word[2], 0x0d)
            ^ oAesGfMul(word[3], 0x09);
    temp[1] = oAesGfMul(word[0], 0x09)
            ^ oAesGfMul(word[1], 0x0e)
            ^ oAesGfMul(word[2], 0x0b)
            ^ oAesGfMul(word[3], 0x0d);
    temp[2] = oAesGfMul(word[0], 0x0d)
            ^ oAesGfMul(word[1], 0x09)
            ^ oAesGfMul(word[2], 0x0e)
            ^ oAesGfMul(word[3], 0x0b);
    temp[3] = oAesGfMul(word[0], 0x0b)
            ^ oAesGfMul(word[1], 0x0d)
            ^ oAesGfMul(word[2], 0x09)
            ^ oAesGfMul(word[3], 0x0e);
    memcpy(word, temp, OAES_COL_LEN);

    return OAES_RET_SUCCESS;
}

OAES_RET oAesSprintf(char *buf, size_t *bufLength, const uint8_t *data, size_t dataLength)
{
    size_t i;
    size_t bufLengthIn;
    char temp[4];

    if (NULL == bufLength) {
        return OAES_RET_ARG2;
    }

    bufLengthIn = *bufLength;
    *bufLength = dataLength * 3 + dataLength / OAES_BLOCK_SIZE + 1;

    if (NULL == buf) {
        return OAES_RET_SUCCESS;
    }

    if (*bufLength > bufLengthIn) {
        return OAES_RET_BUF;
    }

    if (NULL == data) {
        return OAES_RET_ARG3;
    }

    strcpy(buf, "");

    for (i = 0; i < dataLength; i++) {
        sprintf(temp, "%02x ", data[i]);
        strcat(buf, temp);

        if (i && 0 == (i + 1) % OAES_BLOCK_SIZE) {
            strcat(buf, "\n");
        }
    }

    return OAES_RET_SUCCESS;
}

#ifdef OAES_HAVE_ISAAC
static void oAesGetSeed(char buf[RANDSIZ + 1])
{
#if !defined(__FreeBSD__) && !defined(__OpenBSD__)
    struct timeb timer;
    struct tm *gmTimer;
    char *_test = NULL;

    ftime(&timer);
    gmTimer = gmtime(&timer.time);
    _test = (char *)calloc(sizeof(char), timer.millitm);
    sprintf(buf, "%04d%02d%02d%02d%02d%02d%03d%p%d", gmTimer->tm_year + 1900, gmTimer->tm_mon + 1,
            gmTimer->tm_mday, gmTimer->tm_hour, gmTimer->tm_min, gmTimer->tm_sec, timer.millitm,
            _test + timer.millitm, getpid());
#else
    struct timeval timer;
    struct tm *gmTimer;
    char *_test = NULL;

    gettimeofday(&timer, NULL);
    gmTimer = gmtime(&timer.tv_sec);
    _test = (char *)calloc(sizeof(char), timer.tv_usec / 1000);
    sprintf(buf, "%04d%02d%02d%02d%02d%02d%03d%p%d", gmTimer->tm_year + 1900, gmTimer->tm_mon + 1,
            gmTimer->tm_mday, gmTimer->tm_hour, gmTimer->tm_min, gmTimer->tm_sec,
            timer.tv_usec / 1000, _test + timer.tv_usec / 1000, getpid());
#endif

    if (_test)
        free(_test);
}
#else
static uint32_t oAesGetSeed(void)
{
#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__ANDROID__)
    struct timeb timer;
    struct tm *gmTimer;
    char *test = NULL;
    uint32_t ret = 0;

    ftime(&timer);
    gmTimer = gmtime(&timer.time);
    test = (char *)calloc(sizeof(char), timer.millitm);
    ret = gmTimer->tm_year + 1900 + gmTimer->tm_mon + 1 + gmTimer->tm_mday + gmTimer->tm_hour
            + gmTimer->tm_min + gmTimer->tm_sec + timer.millitm + (uintptr_t)(test + timer.millitm)
            + getpid();
#else
    struct timeval timer;
    struct tm *gmTimer;
    char *test = NULL;
    uint32_t ret = 0;

    gettimeofday(&timer, NULL);
    gmTimer = gmtime(&timer.tv_sec);
    test = (char *)calloc(sizeof(char), timer.tv_usec / 1000);
    ret = gmTimer->tm_year + 1900 + gmTimer->tm_mon + 1 + gmTimer->tm_mday + gmTimer->tm_hour
            + gmTimer->tm_min + gmTimer->tm_sec + timer.tv_usec / 1000
            + (uintptr_t)(test + timer.tv_usec / 1000) + getpid();
#endif

    if (test) {
        free(test);
    }

    return ret;
}
#endif // OAES_HAVE_ISAAC

static OAES_RET oAesKeyDestroy(OAesKey **key)
{
    if (NULL == *key) {
        return OAES_RET_SUCCESS;
    }

    if ((*key)->data) {
        free((*key)->data);
        (*key)->data = NULL;
    }

    if ((*key)->expData) {
        free((*key)->expData);
        (*key)->expData = NULL;
    }

    (*key)->dataLength = 0;
    (*key)->expDataLength = 0;
    (*key)->numKeys = 0;
    (*key)->keyBase = 0;
    free(*key);
    *key = NULL;

    return OAES_RET_SUCCESS;
}

static OAES_RET oAesKeyExpand(OAES_CTX *ctx)
{
    size_t i;
    size_t j;
    OAesCtx *pCtx = (OAesCtx *)ctx;

    if (NULL == pCtx) {
        return OAES_RET_ARG1;
    }

    if (NULL == pCtx->key) {
        return OAES_RET_NOKEY;
    }

    pCtx->key->keyBase = pCtx->key->dataLength / OAES_RKEY_LEN;
    pCtx->key->numKeys = pCtx->key->keyBase + OAES_ROUND_BASE;

    pCtx->key->expDataLength = pCtx->key->numKeys * OAES_RKEY_LEN * OAES_COL_LEN;
    pCtx->key->expData = (uint8_t *)calloc(pCtx->key->expDataLength, sizeof(uint8_t));

    if (NULL == pCtx->key->expData) {
        return OAES_RET_MEM;
    }

    // the first pCtx->key->dataLength are a direct copy
    memcpy(pCtx->key->expData, pCtx->key->data, pCtx->key->dataLength);

    // apply ExpandKey algorithm for remainder
    for (i = pCtx->key->keyBase; i < pCtx->key->numKeys * OAES_RKEY_LEN; i++) {
        uint8_t temp[OAES_COL_LEN];

        memcpy(temp, pCtx->key->expData + (i - 1) * OAES_RKEY_LEN, OAES_COL_LEN);

        // transform key column
        if (0 == i % pCtx->key->keyBase) {
            oAesWordRotLeft(temp);

            for (j = 0; j < OAES_COL_LEN; j++) {
                oAesSubByte(temp + j);
            }

            temp[0] = temp[0] ^ oAesGf8[i / pCtx->key->keyBase - 1];
        } else if (pCtx->key->keyBase > 6 && 4 == i % pCtx->key->keyBase) {
            for (j = 0; j < OAES_COL_LEN; j++) {
                oAesSubByte(temp + j);
            }
        }

        for (j = 0; j < OAES_COL_LEN; j++) {
            pCtx->key->expData[i * OAES_RKEY_LEN + j] =
                    pCtx->key->expData[(i - pCtx->key->keyBase) * OAES_RKEY_LEN + j]
                    ^ temp[j];
        }
    }

    return OAES_RET_SUCCESS;
}

static OAES_RET oAesKeyGen(OAES_CTX *ctx, size_t keySize)
{
    size_t i;
    OAesKey *pKey = NULL;
    OAesCtx *pCtx = (OAesCtx *)ctx;
    OAES_RET rc = OAES_RET_SUCCESS;

    if (NULL == pCtx) {
        return OAES_RET_ARG1;
    }

    pKey = (OAesKey *)calloc(sizeof(OAesKey), 1);

    if (NULL == pKey) {
        return OAES_RET_MEM;
    }

    if (pCtx->key) {
        oAesKeyDestroy(&(pCtx->key));
    }

    pKey->dataLength = keySize;
    pKey->data = (uint8_t *)calloc(keySize, sizeof(uint8_t));

    if (NULL == pKey->data) {
        free(pKey);
        return OAES_RET_MEM;
    }

    for (i = 0; i < keySize; i++)
#ifdef OAES_HAVE_ISAAC
        pKey->data[i] = (uint8_t)rand(pCtx->rctx);
#else
        pKey->data[i] = (uint8_t)rand();
#endif // OAES_HAVE_ISAAC

    pCtx->key = pKey;
    rc = rc || oAesKeyExpand(ctx);

    if (rc != OAES_RET_SUCCESS) {
        oAesKeyDestroy(&(pCtx->key));
        return rc;
    }

    return OAES_RET_SUCCESS;
}

OAES_RET oAesKeyGen128(OAES_CTX *ctx)
{
    return oAesKeyGen(ctx, 16);
}

OAES_RET oAesKeyGen192(OAES_CTX *ctx)
{
    return oAesKeyGen(ctx, 24);
}

OAES_RET oAesKeyGen256(OAES_CTX *ctx)
{
    return oAesKeyGen(ctx, 32);
}

OAES_RET oAesKeyExport(OAES_CTX *ctx, uint8_t *data, size_t *dataLength)
{
    size_t dataLengthIn;
    OAesCtx *pCtx = (OAesCtx *)ctx;

    if (NULL == pCtx) {
        return OAES_RET_ARG1;
    }

    if (NULL == pCtx->key) {
        return OAES_RET_NOKEY;
    }

    if (NULL == dataLength) {
        return OAES_RET_ARG3;
    }

    dataLengthIn = *dataLength;
    // data + header
    *dataLength = pCtx->key->dataLength + OAES_BLOCK_SIZE;

    if (NULL == data) {
        return OAES_RET_SUCCESS;
    }

    if (dataLengthIn < *dataLength) {
        return OAES_RET_BUF;
    }

    // header
    memcpy(data, oAesHeader, OAES_BLOCK_SIZE);
    data[5] = 0x01;
    data[7] = pCtx->key->dataLength;
    memcpy(data + OAES_BLOCK_SIZE, pCtx->key->data, pCtx->key->dataLength);

    return OAES_RET_SUCCESS;
}

OAES_RET oAesKeyExportData(OAES_CTX *ctx, uint8_t *data, size_t *dataLength)
{
    size_t dataLengthIn;
    OAesCtx *pCtx = (OAesCtx *)ctx;

    if (NULL == pCtx) {
        return OAES_RET_ARG1;
    }

    if (NULL == pCtx->key) {
        return OAES_RET_NOKEY;
    }

    if (NULL == dataLength) {
        return OAES_RET_ARG3;
    }

    dataLengthIn = *dataLength;
    *dataLength = pCtx->key->dataLength;

    if (NULL == data) {
        return OAES_RET_SUCCESS;
    }

    if (dataLengthIn < *dataLength) {
        return OAES_RET_BUF;
    }

    memcpy(data, pCtx->key->data, *dataLength);

    return OAES_RET_SUCCESS;
}

OAES_RET oAesKeyImport(OAES_CTX *ctx, const uint8_t *data, size_t dataLength)
{
    OAesCtx *pCtx = (OAesCtx *)ctx;
    OAES_RET rc = OAES_RET_SUCCESS;
    int keyLength;

    if (NULL == pCtx) {
        return OAES_RET_ARG1;
    }

    if (NULL == data) {
        return OAES_RET_ARG2;
    }

    switch (dataLength) {
    case 16 + OAES_BLOCK_SIZE:
    case 24 + OAES_BLOCK_SIZE:
    case 32 + OAES_BLOCK_SIZE:
        break;
    default:
        return OAES_RET_ARG3;
    }

    // header
    if (0 != memcmp(data, oAesHeader, 4)) {
        return OAES_RET_HEADER;
    }

    // header version
    switch (data[4]) {
    case 0x01:
        break;
    default:
        return OAES_RET_HEADER;
    }

    // header type
    switch (data[5]) {
    case 0x01:
        break;
    default:
        return OAES_RET_HEADER;
    }

    // options
    keyLength = data[7];
    switch (keyLength) {
    case 16:
    case 24:
    case 32:
        break;
    default:
        return OAES_RET_HEADER;
    }

    if ((int)dataLength != keyLength + OAES_BLOCK_SIZE) {
        return OAES_RET_ARG3;
    }

    if (pCtx->key) {
        oAesKeyDestroy(&(pCtx->key));
    }

    pCtx->key = (OAesKey *)calloc(sizeof(OAesKey), 1);

    if (NULL == pCtx->key) {
        return OAES_RET_MEM;
    }

    pCtx->key->dataLength = keyLength;
    pCtx->key->data = (uint8_t *)calloc(keyLength, sizeof(uint8_t));

    if (NULL == pCtx->key->data) {
        oAesKeyDestroy(&(pCtx->key));
        return OAES_RET_MEM;
    }

    memcpy(pCtx->key->data, data + OAES_BLOCK_SIZE, keyLength);
    rc = rc || oAesKeyExpand(ctx);

    if (rc != OAES_RET_SUCCESS) {
        oAesKeyDestroy(&(pCtx->key));
        return rc;
    }

    return OAES_RET_SUCCESS;
}

OAES_RET oAesKeyImportData(OAES_CTX *ctx, const uint8_t *data, size_t dataLength)
{
    OAesCtx *pCtx = (OAesCtx *)ctx;
    OAES_RET rc = OAES_RET_SUCCESS;

    if (NULL == pCtx) {
        return OAES_RET_ARG1;
    }

    if (NULL == data) {
        return OAES_RET_ARG2;
    }

    switch (dataLength) {
    case 16:
    case 24:
    case 32:
        break;
    default:
        return OAES_RET_ARG3;
    }

    if (pCtx->key) {
        oAesKeyDestroy(&(pCtx->key));
    }

    pCtx->key = (OAesKey *)calloc(sizeof(OAesKey), 1);

    if (NULL == pCtx->key) {
        return OAES_RET_MEM;
    }

    pCtx->key->dataLength = dataLength;
    pCtx->key->data = (uint8_t *)calloc(dataLength, sizeof(uint8_t));

    if (NULL == pCtx->key->data) {
        oAesKeyDestroy(&(pCtx->key));
        return OAES_RET_MEM;
    }

    memcpy(pCtx->key->data, data, dataLength);
    rc = rc || oAesKeyExpand(ctx);

    if (rc != OAES_RET_SUCCESS) {
        oAesKeyDestroy(&(pCtx->key));
        return rc;
    }

    return OAES_RET_SUCCESS;
}

OAES_CTX *oAesAlloc(void)
{
    OAesCtx *pCtx = (OAesCtx *)calloc(sizeof(OAesCtx), 1);

    if (NULL == pCtx) {
        return NULL;
    }

#ifdef OAES_HAVE_ISAAC
    {
        ub4 _i = 0;
        char _seed[RANDSIZ + 1];

        pCtx->rctx = (randctx *)calloc(sizeof(randctx), 1);

        if (NULL == pCtx->rctx) {
            free(pCtx);
            return NULL;
        }

        oAesGetSeed(_seed);
        memset(pCtx->rctx->randrsl, 0, RANDSIZ);
        memcpy(pCtx->rctx->randrsl, _seed, RANDSIZ);
        randinit(pCtx->rctx, TRUE);
    }
#else
    srand(oAesGetSeed());
#endif // OAES_HAVE_ISAAC

    pCtx->key = NULL;
    oAesSetOption(pCtx, OAES_OPTION_CBC, NULL);

#ifdef OAES_DEBUG
    pCtx->step_cb = NULL;
    oAesSetOption(pCtx, OAES_OPTION_STEP_OFF, NULL);
#endif // OAES_DEBUG

    return (OAES_CTX *)pCtx;
}

OAES_RET oAesFree(OAES_CTX **ctx)
{
    OAesCtx **pCtx = (OAesCtx **)ctx;

    if (NULL == pCtx) {
        return OAES_RET_ARG1;
    }

    if (NULL == *pCtx) {
        return OAES_RET_SUCCESS;
    }

    if ((*pCtx)->key) {
        oAesKeyDestroy(&((*pCtx)->key));
    }

#ifdef OAES_HAVE_ISAAC
    if ((*pCtx)->rctx) {
        free((*pCtx)->rctx);
        (*pCtx)->rctx = NULL;
    }
#endif // OAES_HAVE_ISAAC

    free(*pCtx);
    *pCtx = NULL;

    return OAES_RET_SUCCESS;
}

OAES_RET oAesSetOption(OAES_CTX *ctx, OAES_OPTION option, const void *value)
{
    size_t i;
    OAesCtx *pCtx = (OAesCtx *)ctx;

    if (NULL == pCtx) {
        return OAES_RET_ARG1;
    }

    switch (option) {
    case OAES_OPTION_ECB:
        pCtx->options &= ~OAES_OPTION_CBC;
        memset(pCtx->iv, 0, OAES_BLOCK_SIZE);
        break;

    case OAES_OPTION_CBC:
        pCtx->options &= ~OAES_OPTION_ECB;
        if (value)
            memcpy(pCtx->iv, value, OAES_BLOCK_SIZE);
        else {
            for (i = 0; i < OAES_BLOCK_SIZE; i++)
#ifdef OAES_HAVE_ISAAC
                pCtx->iv[i] = (uint8_t)rand(pCtx->rctx);
#else
                pCtx->iv[i] = (uint8_t)rand();
#endif // OAES_HAVE_ISAAC
        }
        break;

#ifdef OAES_DEBUG

    case OAES_OPTION_STEP_ON:
        if (value) {
            pCtx->options &= ~OAES_OPTION_STEP_OFF;
            pCtx->step_cb = value;
        } else {
            pCtx->options &= ~OAES_OPTION_STEP_ON;
            pCtx->options |= OAES_OPTION_STEP_OFF;
            pCtx->step_cb = NULL;
            return OAES_RET_ARG3;
        }
        break;

    case OAES_OPTION_STEP_OFF:
        pCtx->options &= ~OAES_OPTION_STEP_ON;
        pCtx->step_cb = NULL;
        break;

#endif // OAES_DEBUG

    default:
        return OAES_RET_ARG2;
    }

    pCtx->options |= option;

    return OAES_RET_SUCCESS;
}

static OAES_RET oAesEncryptBlock(OAES_CTX *ctx, uint8_t *c, size_t cLen)
{
    size_t i;
    size_t j;
    OAesCtx *pCtx = (OAesCtx *)ctx;

    if (NULL == pCtx) {
        return OAES_RET_ARG1;
    }

    if (NULL == c) {
        return OAES_RET_ARG2;
    }

    if (cLen != OAES_BLOCK_SIZE) {
        return OAES_RET_ARG3;
    }

    if (NULL == pCtx->key) {
        return OAES_RET_NOKEY;
    }

#ifdef OAES_DEBUG
    if (pCtx->step_cb) {
        pCtx->step_cb(c, "input", 1, NULL);
    }
#endif // OAES_DEBUG

    // AddRoundKey(State, K0)
    for (i = 0; i < cLen; i++) {
        c[i] = c[i] ^ pCtx->key->expData[i];
    }

#ifdef OAES_DEBUG
    if (pCtx->step_cb) {
        pCtx->step_cb(pCtx->key->expData, "k_sch", 1, NULL);
        pCtx->step_cb(c, "k_add", 1, NULL);
    }
#endif // OAES_DEBUG

    // for round = 1 step 1 to Nr–1
    for (i = 1; i < pCtx->key->numKeys - 1; i++) {
        // SubBytes(state)
        for (j = 0; j < cLen; j++) {
            oAesSubByte(c + j);
        }

#ifdef OAES_DEBUG
        if (pCtx->step_cb) {
            pCtx->step_cb(c, "s_box", i, NULL);
        }
#endif // OAES_DEBUG

        // ShiftRows(state)
        oAesShiftRows(c);

#ifdef OAES_DEBUG
        if (pCtx->step_cb) {
            pCtx->step_cb(c, "s_row", i, NULL);
        }
#endif // OAES_DEBUG

        // MixColumns(state)
        oAesMixCols(c);
        oAesMixCols(c + 4);
        oAesMixCols(c + 8);
        oAesMixCols(c + 12);

#ifdef OAES_DEBUG
        if (pCtx->step_cb) {
            pCtx->step_cb(c, "m_col", i, NULL);
        }
#endif // OAES_DEBUG

        // AddRoundKey(state, w[round*Nb, (round+1)*Nb-1])
        for (j = 0; j < cLen; j++) {
            c[j] = c[j] ^ pCtx->key->expData[i * OAES_RKEY_LEN * OAES_COL_LEN + j];
        }

#ifdef OAES_DEBUG
        if (pCtx->step_cb) {
            pCtx->step_cb(pCtx->key->expData
                           + i
                           * OAES_RKEY_LEN
                           * OAES_COL_LEN,
                          "k_sch",
                          i,
                          NULL);
            pCtx->step_cb(c, "k_add", i, NULL);
        }
#endif // OAES_DEBUG
    }

    // SubBytes(state)
    for (i = 0; i < cLen; i++) {
        oAesSubByte(c + i);
    }

#ifdef OAES_DEBUG
    if (pCtx->step_cb) {
        pCtx->step_cb(c, "s_box", pCtx->key->numKeys - 1, NULL);
    }
#endif // OAES_DEBUG

    // ShiftRows(state)
    oAesShiftRows(c);

#ifdef OAES_DEBUG
    if (pCtx->step_cb) {
        pCtx->step_cb(c, "s_row", pCtx->key->numKeys - 1, NULL);
    }
#endif // OAES_DEBUG

    // AddRoundKey(state, w[Nr*Nb, (Nr+1)*Nb-1])
    for (i = 0; i < cLen; i++) {
        c[i] = c[i]
                ^ pCtx->key->expData[(pCtx->key->numKeys - 1) * OAES_RKEY_LEN * OAES_COL_LEN + i];
    }

#ifdef OAES_DEBUG
    if (pCtx->step_cb) {
        pCtx->step_cb(pCtx->key->expData
                              + (pCtx->key->numKeys - 1) * OAES_RKEY_LEN * OAES_COL_LEN,
                      "k_sch", pCtx->key->numKeys - 1, NULL);
        pCtx->step_cb(c, "output", pCtx->key->numKeys - 1, NULL);
    }
#endif // OAES_DEBUG

    return OAES_RET_SUCCESS;
}

static OAES_RET oAesDecryptBlock(OAES_CTX *ctx, uint8_t *c, size_t cLen)
{
    size_t i;
    size_t j;
    OAesCtx *pCtx = (OAesCtx *)ctx;

    if (NULL == pCtx) {
        return OAES_RET_ARG1;
    }

    if (NULL == c) {
        return OAES_RET_ARG2;
    }

    if (cLen != OAES_BLOCK_SIZE) {
        return OAES_RET_ARG3;
    }

    if (NULL == pCtx->key) {
        return OAES_RET_NOKEY;
    }

#ifdef OAES_DEBUG
    if (pCtx->step_cb) {
        pCtx->step_cb(c, "iinput", pCtx->key->numKeys - 1, NULL);
    }
#endif // OAES_DEBUG

    // AddRoundKey(state, w[Nr*Nb, (Nr+1)*Nb-1])
    for (i = 0; i < cLen; i++) {
        c[i] = c[i]
                ^ pCtx->key->expData[(pCtx->key->numKeys - 1) * OAES_RKEY_LEN * OAES_COL_LEN + i];
    }

#ifdef OAES_DEBUG
    if (pCtx->step_cb) {
        pCtx->step_cb(pCtx->key->expData
                              + (pCtx->key->numKeys - 1) * OAES_RKEY_LEN * OAES_COL_LEN,
                      "ik_sch", pCtx->key->numKeys - 1, NULL);
        pCtx->step_cb(c, "ik_add", pCtx->key->numKeys - 1, NULL);
    }
#endif // OAES_DEBUG

    for (i = pCtx->key->numKeys - 2; i > 0; i--) {
        // InvShiftRows(state)
        oAesInvShiftRows(c);

#ifdef OAES_DEBUG
        if (pCtx->step_cb)
            pCtx->step_cb(c, "is_row", i, NULL);
#endif // OAES_DEBUG

        // InvSubBytes(state)
        for (j = 0; j < cLen; j++) {
            oAesInvSubByte(c + j);
        }

#ifdef OAES_DEBUG
        if (pCtx->step_cb) {
            pCtx->step_cb(c, "is_box", i, NULL);
        }
#endif // OAES_DEBUG

        // AddRoundKey(state, w[round*Nb, (round+1)*Nb-1])
        for (j = 0; j < cLen; j++) {
            c[j] = c[j] ^ pCtx->key->expData[i * OAES_RKEY_LEN * OAES_COL_LEN + j];
        }

#ifdef OAES_DEBUG
        if (pCtx->step_cb) {
            pCtx->step_cb(pCtx->key->expData + i * OAES_RKEY_LEN * OAES_COL_LEN, "ik_sch", i,
                          NULL);
            pCtx->step_cb(c, "ik_add", i, NULL);
        }
#endif // OAES_DEBUG

        // InvMixColums(state)
        oAesInvMixCols(c);
        oAesInvMixCols(c + 4);
        oAesInvMixCols(c + 8);
        oAesInvMixCols(c + 12);

#ifdef OAES_DEBUG
        if (pCtx->step_cb) {
            pCtx->step_cb(c, "im_col", i, NULL);
        }
#endif // OAES_DEBUG
    }

    // InvShiftRows(state)
    oAesInvShiftRows(c);

#ifdef OAES_DEBUG
    if (pCtx->step_cb) {
        pCtx->step_cb(c, "is_row", 1, NULL);
    }
#endif // OAES_DEBUG

    // InvSubBytes(state)
    for (i = 0; i < cLen; i++) {
        oAesInvSubByte(c + i);
    }

#ifdef OAES_DEBUG
    if (pCtx->step_cb) {
        pCtx->step_cb(c, "is_box", 1, NULL);
    }
#endif // OAES_DEBUG

    // AddRoundKey(state, w[0, Nb-1])
    for (i = 0; i < cLen; i++) {
        c[i] = c[i] ^ pCtx->key->expData[i];
    }

#ifdef OAES_DEBUG
    if (pCtx->step_cb) {
        pCtx->step_cb(pCtx->key->expData, "ik_sch", 1, NULL);
        pCtx->step_cb(c, "ioutput", 1, NULL);
    }
#endif // OAES_DEBUG

    return OAES_RET_SUCCESS;
}

OAES_RET oAesEncrypt(OAES_CTX *ctx, const uint8_t *m, size_t mLen, uint8_t *c, size_t *cLen)
{
    size_t i, j, cLenIn, cDataLen;
    size_t padLen = mLen % OAES_BLOCK_SIZE == 0 ? 0 : OAES_BLOCK_SIZE - mLen % OAES_BLOCK_SIZE;
    OAesCtx *pCtx = (OAesCtx *)ctx;
    OAES_RET rc = OAES_RET_SUCCESS;
    uint8_t flags = padLen ? OAES_FLAG_PAD : 0;

    if (NULL == pCtx) {
        return OAES_RET_ARG1;
    }

    if (NULL == m) {
        return OAES_RET_ARG2;
    }

    if (NULL == cLen) {
        return OAES_RET_ARG5;
    }

    cLenIn = *cLen;
    // data + pad
    cDataLen = mLen + padLen;
    // header + iv + data + pad
    *cLen = 2 * OAES_BLOCK_SIZE + mLen + padLen;

    if (NULL == c) {
        return OAES_RET_SUCCESS;
    }

    if (cLenIn < *cLen) {
        return OAES_RET_BUF;
    }

    if (NULL == pCtx->key) {
        return OAES_RET_NOKEY;
    }

    // header
    memcpy(c, oAesHeader, OAES_BLOCK_SIZE);
    memcpy(c + 6, &pCtx->options, sizeof(pCtx->options));
    memcpy(c + 8, &flags, sizeof(flags));
    // iv
    memcpy(c + OAES_BLOCK_SIZE, pCtx->iv, OAES_BLOCK_SIZE);
    // data
    memcpy(c + 2 * OAES_BLOCK_SIZE, m, mLen);

    for (i = 0; i < cDataLen; i += OAES_BLOCK_SIZE) {
        uint8_t block[OAES_BLOCK_SIZE];
        size_t blockSize = min(mLen - i, OAES_BLOCK_SIZE);

        memcpy(block, c + 2 * OAES_BLOCK_SIZE + i, blockSize);

        // insert pad
        for (j = 0; j < OAES_BLOCK_SIZE - blockSize; j++) {
            block[blockSize + j] = j + 1;
        }

        // CBC
        if (pCtx->options & OAES_OPTION_CBC) {
            for (j = 0; j < OAES_BLOCK_SIZE; j++) {
                block[j] = block[j] ^ pCtx->iv[j];
            }
        }

        rc = rc || oAesEncryptBlock(ctx, block, OAES_BLOCK_SIZE);
        memcpy(c + 2 * OAES_BLOCK_SIZE + i, block, OAES_BLOCK_SIZE);

        if (pCtx->options & OAES_OPTION_CBC) {
            memcpy(pCtx->iv, block, OAES_BLOCK_SIZE);
        }
    }

    return rc;
}

OAES_RET oAesDecrypt(OAES_CTX *ctx, const uint8_t *c, size_t cLen, uint8_t *m, size_t *mLen)
{
    size_t i;
    size_t j;
    size_t mLenIn;
    OAesCtx *pCtx = (OAesCtx *)ctx;
    OAES_RET rc = OAES_RET_SUCCESS;
    uint8_t iv[OAES_BLOCK_SIZE];
    uint8_t flags;
    OAES_OPTION options;

    if (NULL == ctx) {
        return OAES_RET_ARG1;
    }

    if (NULL == c) {
        return OAES_RET_ARG2;
    }

    if (cLen % OAES_BLOCK_SIZE) {
        return OAES_RET_ARG3;
    }

    if (NULL == mLen) {
        return OAES_RET_ARG5;
    }

    mLenIn = *mLen;
    *mLen = cLen - 2 * OAES_BLOCK_SIZE;

    if (NULL == m) {
        return OAES_RET_SUCCESS;
    }

    if (mLenIn < *mLen) {
        return OAES_RET_BUF;
    }

    if (NULL == pCtx->key) {
        return OAES_RET_NOKEY;
    }

    // header
    if (0 != memcmp(c, oAesHeader, 4)) {
        return OAES_RET_HEADER;
    }

    // header version
    switch (c[4]) {
    case 0x01:
        break;
    default:
        return OAES_RET_HEADER;
    }

    // header type
    switch (c[5]) {
    case 0x02:
        break;
    default:
        return OAES_RET_HEADER;
    }

    // options
    memcpy(&options, c + 6, sizeof(options));
    // validate that all options are valid
    if (options
        & ~(OAES_OPTION_ECB | OAES_OPTION_CBC
#ifdef OAES_DEBUG
            | OAES_OPTION_STEP_ON | OAES_OPTION_STEP_OFF
#endif // OAES_DEBUG
            )) {
        return OAES_RET_HEADER;
    }

    if ((options & OAES_OPTION_ECB) && (options & OAES_OPTION_CBC)) {
        return OAES_RET_HEADER;
    }

    if (options == OAES_OPTION_NONE) {
        return OAES_RET_HEADER;
    }

    // flags
    memcpy(&flags, c + 8, sizeof(flags));
    // validate that all flags are valid
    if (flags & ~(OAES_FLAG_PAD)) {
        return OAES_RET_HEADER;
    }

    // iv
    memcpy(iv, c + OAES_BLOCK_SIZE, OAES_BLOCK_SIZE);
    // data + pad
    memcpy(m, c + 2 * OAES_BLOCK_SIZE, *mLen);

    for (i = 0; i < *mLen; i += OAES_BLOCK_SIZE) {
        if ((options & OAES_OPTION_CBC) && i > 0) {
            memcpy(iv, c + OAES_BLOCK_SIZE + i, OAES_BLOCK_SIZE);
        }

        rc = rc || oAesDecryptBlock(ctx, m + i, min(*mLen - i, OAES_BLOCK_SIZE));

        // CBC
        if (options & OAES_OPTION_CBC) {
            for (j = 0; j < OAES_BLOCK_SIZE; j++) {
                m[i + j] = m[i + j] ^ iv[j];
            }
        }
    }

    // remove pad
    if (flags & OAES_FLAG_PAD) {
        int isPad = 1;
        size_t temp = (size_t)m[*mLen - 1];

        if (temp <= 0x00 || temp > 0x0f) {
            return OAES_RET_HEADER;
        }

        for (i = 0; i < temp; i++) {
            if (m[*mLen - 1 - i] != temp - i) {
                isPad = 0;
            }
        }

        if (isPad) {
            memset(m + *mLen - temp, 0, temp);
            *mLen -= temp;
        } else {
            return OAES_RET_HEADER;
        }
    }

    return OAES_RET_SUCCESS;
}

OAES_API OAES_RET oAesEncryptionRound(const uint8_t *key, uint8_t *c)
{
    size_t i;

    if (NULL == key) {
        return OAES_RET_ARG1;
    }

    if (NULL == c) {
        return OAES_RET_ARG2;
    }

    // SubBytes(state)
    for (i = 0; i < OAES_BLOCK_SIZE; i++) {
        oAesSubByte(c + i);
    }

    // ShiftRows(state)
    oAesShiftRows(c);

    // MixColumns(state)
    oAesMixCols(c);
    oAesMixCols(c + 4);
    oAesMixCols(c + 8);
    oAesMixCols(c + 12);

    // AddRoundKey(State, key)
    for (i = 0; i < OAES_BLOCK_SIZE; i++) {
        c[i] ^= key[i];
    }

    return OAES_RET_SUCCESS;
}

OAES_API OAES_RET oAesPseudoEncryptEcb(OAES_CTX *ctx, uint8_t *c)
{
    size_t i;
    OAesCtx *pCtx = (OAesCtx *)ctx;

    if (NULL == pCtx) {
        return OAES_RET_ARG1;
    }

    if (NULL == c) {
        return OAES_RET_ARG2;
    }

    if (NULL == pCtx->key) {
        return OAES_RET_NOKEY;
    }

    for (i = 0; i < 10; ++i) {
        oAesEncryptionRound(&pCtx->key->expData[i * OAES_RKEY_LEN * OAES_COL_LEN], c);
    }

    return OAES_RET_SUCCESS;
}
