// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2018-2021, The Qwertycoin Group.
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

#include <assert.h>
#include <stdint.h>

#include <Crypto/CryptoOps.h>

/* Predeclarations */

static void feMul(fe, const fe, const fe);
static void feSq(fe, const fe);
static void feToBytes(unsigned char *, const fe);
static void geMAdd(GeP1P1 *r, const GeP3 *p, const GePrecomp *q);
static void geMSub(GeP1P1 *r, const GeP3 *p, const GePrecomp *q);
static void geP20(GeP2 *h);
static void geP3Dbl(GeP1P1 *r, const GeP3 *p);
static void feDivPowM1(fe, const fe, const fe);

/* Common functions */

static uint64_t load3(const unsigned char *in)
{
    uint64_t result;
    result = (uint64_t)in[0];
    result |= ((uint64_t)in[1]) << 8;
    result |= ((uint64_t)in[2]) << 16;

    return result;
}

static uint64_t load4(const unsigned char *in)
{
    uint64_t result;
    result = (uint64_t)in[0];
    result |= ((uint64_t)in[1]) << 8;
    result |= ((uint64_t)in[2]) << 16;
    result |= ((uint64_t)in[3]) << 24;

    return result;
}

/* From fe0.c */

/*
h = 0
*/

static void fe0(fe h)
{
    h[0] = 0;
    h[1] = 0;
    h[2] = 0;
    h[3] = 0;
    h[4] = 0;
    h[5] = 0;
    h[6] = 0;
    h[7] = 0;
    h[8] = 0;
    h[9] = 0;
}

/* From fe1.c */

/*
h = 1
*/

static void fe1(fe h)
{
    h[0] = 1;
    h[1] = 0;
    h[2] = 0;
    h[3] = 0;
    h[4] = 0;
    h[5] = 0;
    h[6] = 0;
    h[7] = 0;
    h[8] = 0;
    h[9] = 0;
}

/* From feAdd.c */

/*
h = f + g
Can overlap h with f or g.

Preconditions:
   |f| bounded by 1.1*2^25,1.1*2^24,1.1*2^25,1.1*2^24,etc.
   |g| bounded by 1.1*2^25,1.1*2^24,1.1*2^25,1.1*2^24,etc.

Postconditions:
   |h| bounded by 1.1*2^26,1.1*2^25,1.1*2^26,1.1*2^25,etc.
*/

static void feAdd(fe h, const fe f, const fe g)
{
    int32_t f0 = f[0];
    int32_t f1 = f[1];
    int32_t f2 = f[2];
    int32_t f3 = f[3];
    int32_t f4 = f[4];
    int32_t f5 = f[5];
    int32_t f6 = f[6];
    int32_t f7 = f[7];
    int32_t f8 = f[8];
    int32_t f9 = f[9];
    int32_t g0 = g[0];
    int32_t g1 = g[1];
    int32_t g2 = g[2];
    int32_t g3 = g[3];
    int32_t g4 = g[4];
    int32_t g5 = g[5];
    int32_t g6 = g[6];
    int32_t g7 = g[7];
    int32_t g8 = g[8];
    int32_t g9 = g[9];
    int32_t h0 = f0 + g0;
    int32_t h1 = f1 + g1;
    int32_t h2 = f2 + g2;
    int32_t h3 = f3 + g3;
    int32_t h4 = f4 + g4;
    int32_t h5 = f5 + g5;
    int32_t h6 = f6 + g6;
    int32_t h7 = f7 + g7;
    int32_t h8 = f8 + g8;
    int32_t h9 = f9 + g9;
    h[0] = h0;
    h[1] = h1;
    h[2] = h2;
    h[3] = h3;
    h[4] = h4;
    h[5] = h5;
    h[6] = h6;
    h[7] = h7;
    h[8] = h8;
    h[9] = h9;
}

/* From feCmov.c */

/*
Replace (f,g) with (g,g) if b == 1;
replace (f,g) with (f,g) if b == 0.

Preconditions: b in {0,1}.
*/

static void feCmov(fe f, const fe g, unsigned int b)
{
    int32_t f0 = f[0];
    int32_t f1 = f[1];
    int32_t f2 = f[2];
    int32_t f3 = f[3];
    int32_t f4 = f[4];
    int32_t f5 = f[5];
    int32_t f6 = f[6];
    int32_t f7 = f[7];
    int32_t f8 = f[8];
    int32_t f9 = f[9];
    int32_t g0 = g[0];
    int32_t g1 = g[1];
    int32_t g2 = g[2];
    int32_t g3 = g[3];
    int32_t g4 = g[4];
    int32_t g5 = g[5];
    int32_t g6 = g[6];
    int32_t g7 = g[7];
    int32_t g8 = g[8];
    int32_t g9 = g[9];
    int32_t x0 = f0 ^ g0;
    int32_t x1 = f1 ^ g1;
    int32_t x2 = f2 ^ g2;
    int32_t x3 = f3 ^ g3;
    int32_t x4 = f4 ^ g4;
    int32_t x5 = f5 ^ g5;
    int32_t x6 = f6 ^ g6;
    int32_t x7 = f7 ^ g7;
    int32_t x8 = f8 ^ g8;
    int32_t x9 = f9 ^ g9;
    assert((((b - 1) & ~b) | ((b - 2) & ~(b - 1))) == (unsigned int)-1);
    b = -(int)b;
    x0 &= b;
    x1 &= b;
    x2 &= b;
    x3 &= b;
    x4 &= b;
    x5 &= b;
    x6 &= b;
    x7 &= b;
    x8 &= b;
    x9 &= b;
    f[0] = f0 ^ x0;
    f[1] = f1 ^ x1;
    f[2] = f2 ^ x2;
    f[3] = f3 ^ x3;
    f[4] = f4 ^ x4;
    f[5] = f5 ^ x5;
    f[6] = f6 ^ x6;
    f[7] = f7 ^ x7;
    f[8] = f8 ^ x8;
    f[9] = f9 ^ x9;
}

/* From feCopy.c */

/*
h = f
*/

static void feCopy(fe h, const fe f)
{
    int32_t f0 = f[0];
    int32_t f1 = f[1];
    int32_t f2 = f[2];
    int32_t f3 = f[3];
    int32_t f4 = f[4];
    int32_t f5 = f[5];
    int32_t f6 = f[6];
    int32_t f7 = f[7];
    int32_t f8 = f[8];
    int32_t f9 = f[9];
    h[0] = f0;
    h[1] = f1;
    h[2] = f2;
    h[3] = f3;
    h[4] = f4;
    h[5] = f5;
    h[6] = f6;
    h[7] = f7;
    h[8] = f8;
    h[9] = f9;
}

/* From feInvert.c */

static void feInvert(fe out, const fe z)
{
    fe t0;
    fe t1;
    fe t2;
    fe t3;
    int i;

    feSq(t0, z);
    feSq(t1, t0);
    feSq(t1, t1);
    feMul(t1, z, t1);
    feMul(t0, t0, t1);
    feSq(t2, t0);
    feMul(t1, t1, t2);
    feSq(t2, t1);
    for (i = 0; i < 4; ++i) {
        feSq(t2, t2);
    }
    feMul(t1, t2, t1);
    feSq(t2, t1);
    for (i = 0; i < 9; ++i) {
        feSq(t2, t2);
    }
    feMul(t2, t2, t1);
    feSq(t3, t2);
    for (i = 0; i < 19; ++i) {
        feSq(t3, t3);
    }
    feMul(t2, t3, t2);
    feSq(t2, t2);
    for (i = 0; i < 9; ++i) {
        feSq(t2, t2);
    }
    feMul(t1, t2, t1);
    feSq(t2, t1);
    for (i = 0; i < 49; ++i) {
        feSq(t2, t2);
    }
    feMul(t2, t2, t1);
    feSq(t3, t2);
    for (i = 0; i < 99; ++i) {
        feSq(t3, t3);
    }
    feMul(t2, t3, t2);
    feSq(t2, t2);
    for (i = 0; i < 49; ++i) {
        feSq(t2, t2);
    }
    feMul(t1, t2, t1);
    feSq(t1, t1);
    for (i = 0; i < 4; ++i) {
        feSq(t1, t1);
    }
    feMul(out, t1, t0);

    return;
}

/* From feIsNegative.c */

/*
return 1 if f is in {1,3,5,...,q-2}
return 0 if f is in {0,2,4,...,q-1}

Preconditions:
   |f| bounded by 1.1*2^26,1.1*2^25,1.1*2^26,1.1*2^25,etc.
*/

static int feIsNegative(const fe f)
{
    unsigned char s[32];
    feToBytes(s, f);

    return s[0] & 1;
}

/* From feIsNonZero.c, modified */

static int feIsNonZero(const fe f)
{
    unsigned char s[32];
    feToBytes(s, f);

    return (
        ((int)(
            s[0] | s[1] | s[2] | s[3] | s[4] | s[5] | s[6] | s[7] | s[8] | s[9] | s[10]
            | s[11] | s[12] | s[13] | s[14] | s[15] | s[16] | s[17] | s[18] | s[19] | s[20]
            | s[21] | s[22] | s[23] | s[24] | s[25] | s[26] | s[27] | s[28] | s[29] | s[30]
            | s[31]) - 1) >> 8) + 1;
}

/* From feMul.c */

/*
h = f * g
Can overlap h with f or g.

Preconditions:
   |f| bounded by 1.65*2^26,1.65*2^25,1.65*2^26,1.65*2^25,etc.
   |g| bounded by 1.65*2^26,1.65*2^25,1.65*2^26,1.65*2^25,etc.

Postconditions:
   |h| bounded by 1.01*2^25,1.01*2^24,1.01*2^25,1.01*2^24,etc.
*/

/*
Notes on implementation strategy:

Using schoolbook multiplication.
Karatsuba would save a little in some cost models.

Most multiplications by 2 and 19 are 32-bit precomputations;
cheaper than 64-bit postcomputations.

There is one remaining multiplication by 19 in the carry chain;
one *19 precomputation can be merged into this,
but the resulting data flow is considerably less clean.

There are 12 carries below.
10 of them are 2-way parallelizable and vectorizable.
Can get away with 11 carries, but then data flow is much deeper.

With tighter constraints on inputs can squeeze carries into int32.
*/

static void feMul(fe h, const fe f, const fe g)
{
    int32_t f0 = f[0];
    int32_t f1 = f[1];
    int32_t f2 = f[2];
    int32_t f3 = f[3];
    int32_t f4 = f[4];
    int32_t f5 = f[5];
    int32_t f6 = f[6];
    int32_t f7 = f[7];
    int32_t f8 = f[8];
    int32_t f9 = f[9];
    int32_t g0 = g[0];
    int32_t g1 = g[1];
    int32_t g2 = g[2];
    int32_t g3 = g[3];
    int32_t g4 = g[4];
    int32_t g5 = g[5];
    int32_t g6 = g[6];
    int32_t g7 = g[7];
    int32_t g8 = g[8];
    int32_t g9 = g[9];
    int32_t g1_19 = 19 * g1; /* 1.959375*2^29 */
    int32_t g2_19 = 19 * g2; /* 1.959375*2^30; still ok */
    int32_t g3_19 = 19 * g3;
    int32_t g4_19 = 19 * g4;
    int32_t g5_19 = 19 * g5;
    int32_t g6_19 = 19 * g6;
    int32_t g7_19 = 19 * g7;
    int32_t g8_19 = 19 * g8;
    int32_t g9_19 = 19 * g9;
    int32_t f1_2 = 2 * f1;
    int32_t f3_2 = 2 * f3;
    int32_t f5_2 = 2 * f5;
    int32_t f7_2 = 2 * f7;
    int32_t f9_2 = 2 * f9;
    int64_t f0g0 = f0 * (int64_t)g0;
    int64_t f0g1 = f0 * (int64_t)g1;
    int64_t f0g2 = f0 * (int64_t)g2;
    int64_t f0g3 = f0 * (int64_t)g3;
    int64_t f0g4 = f0 * (int64_t)g4;
    int64_t f0g5 = f0 * (int64_t)g5;
    int64_t f0g6 = f0 * (int64_t)g6;
    int64_t f0g7 = f0 * (int64_t)g7;
    int64_t f0g8 = f0 * (int64_t)g8;
    int64_t f0g9 = f0 * (int64_t)g9;
    int64_t f1g0 = f1 * (int64_t)g0;
    int64_t f1g1_2 = f1_2 * (int64_t)g1;
    int64_t f1g2 = f1 * (int64_t)g2;
    int64_t f1g3_2 = f1_2 * (int64_t)g3;
    int64_t f1g4 = f1 * (int64_t)g4;
    int64_t f1g5_2 = f1_2 * (int64_t)g5;
    int64_t f1g6 = f1 * (int64_t)g6;
    int64_t f1g7_2 = f1_2 * (int64_t)g7;
    int64_t f1g8 = f1 * (int64_t)g8;
    int64_t f1g9_38 = f1_2 * (int64_t)g9_19;
    int64_t f2g0 = f2 * (int64_t)g0;
    int64_t f2g1 = f2 * (int64_t)g1;
    int64_t f2g2 = f2 * (int64_t)g2;
    int64_t f2g3 = f2 * (int64_t)g3;
    int64_t f2g4 = f2 * (int64_t)g4;
    int64_t f2g5 = f2 * (int64_t)g5;
    int64_t f2g6 = f2 * (int64_t)g6;
    int64_t f2g7 = f2 * (int64_t)g7;
    int64_t f2g8_19 = f2 * (int64_t)g8_19;
    int64_t f2g9_19 = f2 * (int64_t)g9_19;
    int64_t f3g0 = f3 * (int64_t)g0;
    int64_t f3g1_2 = f3_2 * (int64_t)g1;
    int64_t f3g2 = f3 * (int64_t)g2;
    int64_t f3g3_2 = f3_2 * (int64_t)g3;
    int64_t f3g4 = f3 * (int64_t)g4;
    int64_t f3g5_2 = f3_2 * (int64_t)g5;
    int64_t f3g6 = f3 * (int64_t)g6;
    int64_t f3g7_38 = f3_2 * (int64_t)g7_19;
    int64_t f3g8_19 = f3 * (int64_t)g8_19;
    int64_t f3g9_38 = f3_2 * (int64_t)g9_19;
    int64_t f4g0 = f4 * (int64_t)g0;
    int64_t f4g1 = f4 * (int64_t)g1;
    int64_t f4g2 = f4 * (int64_t)g2;
    int64_t f4g3 = f4 * (int64_t)g3;
    int64_t f4g4 = f4 * (int64_t)g4;
    int64_t f4g5 = f4 * (int64_t)g5;
    int64_t f4g6_19 = f4 * (int64_t)g6_19;
    int64_t f4g7_19 = f4 * (int64_t)g7_19;
    int64_t f4g8_19 = f4 * (int64_t)g8_19;
    int64_t f4g9_19 = f4 * (int64_t)g9_19;
    int64_t f5g0 = f5 * (int64_t)g0;
    int64_t f5g1_2 = f5_2 * (int64_t)g1;
    int64_t f5g2 = f5 * (int64_t)g2;
    int64_t f5g3_2 = f5_2 * (int64_t)g3;
    int64_t f5g4 = f5 * (int64_t)g4;
    int64_t f5g5_38 = f5_2 * (int64_t)g5_19;
    int64_t f5g6_19 = f5 * (int64_t)g6_19;
    int64_t f5g7_38 = f5_2 * (int64_t)g7_19;
    int64_t f5g8_19 = f5 * (int64_t)g8_19;
    int64_t f5g9_38 = f5_2 * (int64_t)g9_19;
    int64_t f6g0 = f6 * (int64_t)g0;
    int64_t f6g1 = f6 * (int64_t)g1;
    int64_t f6g2 = f6 * (int64_t)g2;
    int64_t f6g3 = f6 * (int64_t)g3;
    int64_t f6g4_19 = f6 * (int64_t)g4_19;
    int64_t f6g5_19 = f6 * (int64_t)g5_19;
    int64_t f6g6_19 = f6 * (int64_t)g6_19;
    int64_t f6g7_19 = f6 * (int64_t)g7_19;
    int64_t f6g8_19 = f6 * (int64_t)g8_19;
    int64_t f6g9_19 = f6 * (int64_t)g9_19;
    int64_t f7g0 = f7 * (int64_t)g0;
    int64_t f7g1_2 = f7_2 * (int64_t)g1;
    int64_t f7g2 = f7 * (int64_t)g2;
    int64_t f7g3_38 = f7_2 * (int64_t)g3_19;
    int64_t f7g4_19 = f7 * (int64_t)g4_19;
    int64_t f7g5_38 = f7_2 * (int64_t)g5_19;
    int64_t f7g6_19 = f7 * (int64_t)g6_19;
    int64_t f7g7_38 = f7_2 * (int64_t)g7_19;
    int64_t f7g8_19 = f7 * (int64_t)g8_19;
    int64_t f7g9_38 = f7_2 * (int64_t)g9_19;
    int64_t f8g0 = f8 * (int64_t)g0;
    int64_t f8g1 = f8 * (int64_t)g1;
    int64_t f8g2_19 = f8 * (int64_t)g2_19;
    int64_t f8g3_19 = f8 * (int64_t)g3_19;
    int64_t f8g4_19 = f8 * (int64_t)g4_19;
    int64_t f8g5_19 = f8 * (int64_t)g5_19;
    int64_t f8g6_19 = f8 * (int64_t)g6_19;
    int64_t f8g7_19 = f8 * (int64_t)g7_19;
    int64_t f8g8_19 = f8 * (int64_t)g8_19;
    int64_t f8g9_19 = f8 * (int64_t)g9_19;
    int64_t f9g0 = f9 * (int64_t)g0;
    int64_t f9g1_38 = f9_2 * (int64_t)g1_19;
    int64_t f9g2_19 = f9 * (int64_t)g2_19;
    int64_t f9g3_38 = f9_2 * (int64_t)g3_19;
    int64_t f9g4_19 = f9 * (int64_t)g4_19;
    int64_t f9g5_38 = f9_2 * (int64_t)g5_19;
    int64_t f9g6_19 = f9 * (int64_t)g6_19;
    int64_t f9g7_38 = f9_2 * (int64_t)g7_19;
    int64_t f9g8_19 = f9 * (int64_t)g8_19;
    int64_t f9g9_38 = f9_2 * (int64_t)g9_19;
    int64_t h0 = f0g0 + f1g9_38 + f2g8_19 + f3g7_38 + f4g6_19 + f5g5_38 + f6g4_19 + f7g3_38
            + f8g2_19 + f9g1_38;
    int64_t h1 = f0g1 + f1g0 + f2g9_19 + f3g8_19 + f4g7_19 + f5g6_19 + f6g5_19 + f7g4_19 + f8g3_19
            + f9g2_19;
    int64_t h2 = f0g2 + f1g1_2 + f2g0 + f3g9_38 + f4g8_19 + f5g7_38 + f6g6_19 + f7g5_38 + f8g4_19
            + f9g3_38;
    int64_t h3 =
            f0g3 + f1g2 + f2g1 + f3g0 + f4g9_19 + f5g8_19 + f6g7_19 + f7g6_19 + f8g5_19 + f9g4_19;
    int64_t h4 =
            f0g4 + f1g3_2 + f2g2 + f3g1_2 + f4g0 + f5g9_38 + f6g8_19 + f7g7_38 + f8g6_19 + f9g5_38;
    int64_t h5 = f0g5 + f1g4 + f2g3 + f3g2 + f4g1 + f5g0 + f6g9_19 + f7g8_19 + f8g7_19 + f9g6_19;
    int64_t h6 = f0g6 + f1g5_2 + f2g4 + f3g3_2 + f4g2 + f5g1_2 + f6g0 + f7g9_38 + f8g8_19 + f9g7_38;
    int64_t h7 = f0g7 + f1g6 + f2g5 + f3g4 + f4g3 + f5g2 + f6g1 + f7g0 + f8g9_19 + f9g8_19;
    int64_t h8 = f0g8 + f1g7_2 + f2g6 + f3g5_2 + f4g4 + f5g3_2 + f6g2 + f7g1_2 + f8g0 + f9g9_38;
    int64_t h9 = f0g9 + f1g8 + f2g7 + f3g6 + f4g5 + f5g4 + f6g3 + f7g2 + f8g1 + f9g0;
    int64_t carry0;
    int64_t carry1;
    int64_t carry2;
    int64_t carry3;
    int64_t carry4;
    int64_t carry5;
    int64_t carry6;
    int64_t carry7;
    int64_t carry8;
    int64_t carry9;

    /*
    |h0| <= (1.65*1.65*2^52*(1+19+19+19+19)+1.65*1.65*2^50*(38+38+38+38+38))
      i.e. |h0| <= 1.4*2^60; narrower ranges for h2, h4, h6, h8
    |h1| <= (1.65*1.65*2^51*(1+1+19+19+19+19+19+19+19+19))
      i.e. |h1| <= 1.7*2^59; narrower ranges for h3, h5, h7, h9
    */

    carry0 = (h0 + (int64_t)(1 << 25)) >> 26;
    h1 += carry0;
    h0 -= carry0 << 26;
    carry4 = (h4 + (int64_t)(1 << 25)) >> 26;
    h5 += carry4;
    h4 -= carry4 << 26;
    /* |h0| <= 2^25 */
    /* |h4| <= 2^25 */
    /* |h1| <= 1.71*2^59 */
    /* |h5| <= 1.71*2^59 */

    carry1 = (h1 + (int64_t)(1 << 24)) >> 25;
    h2 += carry1;
    h1 -= carry1 << 25;
    carry5 = (h5 + (int64_t)(1 << 24)) >> 25;
    h6 += carry5;
    h5 -= carry5 << 25;
    /* |h1| <= 2^24; from now on fits into int32 */
    /* |h5| <= 2^24; from now on fits into int32 */
    /* |h2| <= 1.41*2^60 */
    /* |h6| <= 1.41*2^60 */

    carry2 = (h2 + (int64_t)(1 << 25)) >> 26;
    h3 += carry2;
    h2 -= carry2 << 26;
    carry6 = (h6 + (int64_t)(1 << 25)) >> 26;
    h7 += carry6;
    h6 -= carry6 << 26;
    /* |h2| <= 2^25; from now on fits into int32 unchanged */
    /* |h6| <= 2^25; from now on fits into int32 unchanged */
    /* |h3| <= 1.71*2^59 */
    /* |h7| <= 1.71*2^59 */

    carry3 = (h3 + (int64_t)(1 << 24)) >> 25;
    h4 += carry3;
    h3 -= carry3 << 25;
    carry7 = (h7 + (int64_t)(1 << 24)) >> 25;
    h8 += carry7;
    h7 -= carry7 << 25;
    /* |h3| <= 2^24; from now on fits into int32 unchanged */
    /* |h7| <= 2^24; from now on fits into int32 unchanged */
    /* |h4| <= 1.72*2^34 */
    /* |h8| <= 1.41*2^60 */

    carry4 = (h4 + (int64_t)(1 << 25)) >> 26;
    h5 += carry4;
    h4 -= carry4 << 26;
    carry8 = (h8 + (int64_t)(1 << 25)) >> 26;
    h9 += carry8;
    h8 -= carry8 << 26;
    /* |h4| <= 2^25; from now on fits into int32 unchanged */
    /* |h8| <= 2^25; from now on fits into int32 unchanged */
    /* |h5| <= 1.01*2^24 */
    /* |h9| <= 1.71*2^59 */

    carry9 = (h9 + (int64_t)(1 << 24)) >> 25;
    h0 += carry9 * 19;
    h9 -= carry9 << 25;
    /* |h9| <= 2^24; from now on fits into int32 unchanged */
    /* |h0| <= 1.1*2^39 */

    carry0 = (h0 + (int64_t)(1 << 25)) >> 26;
    h1 += carry0;
    h0 -= carry0 << 26;
    /* |h0| <= 2^25; from now on fits into int32 unchanged */
    /* |h1| <= 1.01*2^24 */

    h[0] = (int32_t)h0;
    h[1] = (int32_t)h1;
    h[2] = (int32_t)h2;
    h[3] = (int32_t)h3;
    h[4] = (int32_t)h4;
    h[5] = (int32_t)h5;
    h[6] = (int32_t)h6;
    h[7] = (int32_t)h7;
    h[8] = (int32_t)h8;
    h[9] = (int32_t)h9;
}

/* From feNeg.c */

/*
h = -f

Preconditions:
   |f| bounded by 1.1*2^25,1.1*2^24,1.1*2^25,1.1*2^24,etc.

Postconditions:
   |h| bounded by 1.1*2^25,1.1*2^24,1.1*2^25,1.1*2^24,etc.
*/

static void feNeg(fe h, const fe f)
{
    int32_t f0 = f[0];
    int32_t f1 = f[1];
    int32_t f2 = f[2];
    int32_t f3 = f[3];
    int32_t f4 = f[4];
    int32_t f5 = f[5];
    int32_t f6 = f[6];
    int32_t f7 = f[7];
    int32_t f8 = f[8];
    int32_t f9 = f[9];
    int32_t h0 = -f0;
    int32_t h1 = -f1;
    int32_t h2 = -f2;
    int32_t h3 = -f3;
    int32_t h4 = -f4;
    int32_t h5 = -f5;
    int32_t h6 = -f6;
    int32_t h7 = -f7;
    int32_t h8 = -f8;
    int32_t h9 = -f9;
    h[0] = h0;
    h[1] = h1;
    h[2] = h2;
    h[3] = h3;
    h[4] = h4;
    h[5] = h5;
    h[6] = h6;
    h[7] = h7;
    h[8] = h8;
    h[9] = h9;
}

/* From feSq.c */

/*
h = f * f
Can overlap h with f.

Preconditions:
   |f| bounded by 1.65*2^26,1.65*2^25,1.65*2^26,1.65*2^25,etc.

Postconditions:
   |h| bounded by 1.01*2^25,1.01*2^24,1.01*2^25,1.01*2^24,etc.
*/

/*
See feMul.c for discussion of implementation strategy.
*/

static void feSq(fe h, const fe f)
{
    int32_t f0 = f[0];
    int32_t f1 = f[1];
    int32_t f2 = f[2];
    int32_t f3 = f[3];
    int32_t f4 = f[4];
    int32_t f5 = f[5];
    int32_t f6 = f[6];
    int32_t f7 = f[7];
    int32_t f8 = f[8];
    int32_t f9 = f[9];
    int32_t f0_2 = 2 * f0;
    int32_t f1_2 = 2 * f1;
    int32_t f2_2 = 2 * f2;
    int32_t f3_2 = 2 * f3;
    int32_t f4_2 = 2 * f4;
    int32_t f5_2 = 2 * f5;
    int32_t f6_2 = 2 * f6;
    int32_t f7_2 = 2 * f7;
    int32_t f5_38 = 38 * f5; /* 1.959375*2^30 */
    int32_t f6_19 = 19 * f6; /* 1.959375*2^30 */
    int32_t f7_38 = 38 * f7; /* 1.959375*2^30 */
    int32_t f8_19 = 19 * f8; /* 1.959375*2^30 */
    int32_t f9_38 = 38 * f9; /* 1.959375*2^30 */
    int64_t f0f0 = f0 * (int64_t)f0;
    int64_t f0f1_2 = f0_2 * (int64_t)f1;
    int64_t f0f2_2 = f0_2 * (int64_t)f2;
    int64_t f0f3_2 = f0_2 * (int64_t)f3;
    int64_t f0f4_2 = f0_2 * (int64_t)f4;
    int64_t f0f5_2 = f0_2 * (int64_t)f5;
    int64_t f0f6_2 = f0_2 * (int64_t)f6;
    int64_t f0f7_2 = f0_2 * (int64_t)f7;
    int64_t f0f8_2 = f0_2 * (int64_t)f8;
    int64_t f0f9_2 = f0_2 * (int64_t)f9;
    int64_t f1f1_2 = f1_2 * (int64_t)f1;
    int64_t f1f2_2 = f1_2 * (int64_t)f2;
    int64_t f1f3_4 = f1_2 * (int64_t)f3_2;
    int64_t f1f4_2 = f1_2 * (int64_t)f4;
    int64_t f1f5_4 = f1_2 * (int64_t)f5_2;
    int64_t f1f6_2 = f1_2 * (int64_t)f6;
    int64_t f1f7_4 = f1_2 * (int64_t)f7_2;
    int64_t f1f8_2 = f1_2 * (int64_t)f8;
    int64_t f1f9_76 = f1_2 * (int64_t)f9_38;
    int64_t f2f2 = f2 * (int64_t)f2;
    int64_t f2f3_2 = f2_2 * (int64_t)f3;
    int64_t f2f4_2 = f2_2 * (int64_t)f4;
    int64_t f2f5_2 = f2_2 * (int64_t)f5;
    int64_t f2f6_2 = f2_2 * (int64_t)f6;
    int64_t f2f7_2 = f2_2 * (int64_t)f7;
    int64_t f2f8_38 = f2_2 * (int64_t)f8_19;
    int64_t f2f9_38 = f2 * (int64_t)f9_38;
    int64_t f3f3_2 = f3_2 * (int64_t)f3;
    int64_t f3f4_2 = f3_2 * (int64_t)f4;
    int64_t f3f5_4 = f3_2 * (int64_t)f5_2;
    int64_t f3f6_2 = f3_2 * (int64_t)f6;
    int64_t f3f7_76 = f3_2 * (int64_t)f7_38;
    int64_t f3f8_38 = f3_2 * (int64_t)f8_19;
    int64_t f3f9_76 = f3_2 * (int64_t)f9_38;
    int64_t f4f4 = f4 * (int64_t)f4;
    int64_t f4f5_2 = f4_2 * (int64_t)f5;
    int64_t f4f6_38 = f4_2 * (int64_t)f6_19;
    int64_t f4f7_38 = f4 * (int64_t)f7_38;
    int64_t f4f8_38 = f4_2 * (int64_t)f8_19;
    int64_t f4f9_38 = f4 * (int64_t)f9_38;
    int64_t f5f5_38 = f5 * (int64_t)f5_38;
    int64_t f5f6_38 = f5_2 * (int64_t)f6_19;
    int64_t f5f7_76 = f5_2 * (int64_t)f7_38;
    int64_t f5f8_38 = f5_2 * (int64_t)f8_19;
    int64_t f5f9_76 = f5_2 * (int64_t)f9_38;
    int64_t f6f6_19 = f6 * (int64_t)f6_19;
    int64_t f6f7_38 = f6 * (int64_t)f7_38;
    int64_t f6f8_38 = f6_2 * (int64_t)f8_19;
    int64_t f6f9_38 = f6 * (int64_t)f9_38;
    int64_t f7f7_38 = f7 * (int64_t)f7_38;
    int64_t f7f8_38 = f7_2 * (int64_t)f8_19;
    int64_t f7f9_76 = f7_2 * (int64_t)f9_38;
    int64_t f8f8_19 = f8 * (int64_t)f8_19;
    int64_t f8f9_38 = f8 * (int64_t)f9_38;
    int64_t f9f9_38 = f9 * (int64_t)f9_38;
    int64_t h0 = f0f0 + f1f9_76 + f2f8_38 + f3f7_76 + f4f6_38 + f5f5_38;
    int64_t h1 = f0f1_2 + f2f9_38 + f3f8_38 + f4f7_38 + f5f6_38;
    int64_t h2 = f0f2_2 + f1f1_2 + f3f9_76 + f4f8_38 + f5f7_76 + f6f6_19;
    int64_t h3 = f0f3_2 + f1f2_2 + f4f9_38 + f5f8_38 + f6f7_38;
    int64_t h4 = f0f4_2 + f1f3_4 + f2f2 + f5f9_76 + f6f8_38 + f7f7_38;
    int64_t h5 = f0f5_2 + f1f4_2 + f2f3_2 + f6f9_38 + f7f8_38;
    int64_t h6 = f0f6_2 + f1f5_4 + f2f4_2 + f3f3_2 + f7f9_76 + f8f8_19;
    int64_t h7 = f0f7_2 + f1f6_2 + f2f5_2 + f3f4_2 + f8f9_38;
    int64_t h8 = f0f8_2 + f1f7_4 + f2f6_2 + f3f5_4 + f4f4 + f9f9_38;
    int64_t h9 = f0f9_2 + f1f8_2 + f2f7_2 + f3f6_2 + f4f5_2;
    int64_t carry0;
    int64_t carry1;
    int64_t carry2;
    int64_t carry3;
    int64_t carry4;
    int64_t carry5;
    int64_t carry6;
    int64_t carry7;
    int64_t carry8;
    int64_t carry9;

    carry0 = (h0 + (int64_t)(1 << 25)) >> 26;
    h1 += carry0;
    h0 -= carry0 << 26;
    carry4 = (h4 + (int64_t)(1 << 25)) >> 26;
    h5 += carry4;
    h4 -= carry4 << 26;

    carry1 = (h1 + (int64_t)(1 << 24)) >> 25;
    h2 += carry1;
    h1 -= carry1 << 25;
    carry5 = (h5 + (int64_t)(1 << 24)) >> 25;
    h6 += carry5;
    h5 -= carry5 << 25;

    carry2 = (h2 + (int64_t)(1 << 25)) >> 26;
    h3 += carry2;
    h2 -= carry2 << 26;
    carry6 = (h6 + (int64_t)(1 << 25)) >> 26;
    h7 += carry6;
    h6 -= carry6 << 26;

    carry3 = (h3 + (int64_t)(1 << 24)) >> 25;
    h4 += carry3;
    h3 -= carry3 << 25;
    carry7 = (h7 + (int64_t)(1 << 24)) >> 25;
    h8 += carry7;
    h7 -= carry7 << 25;

    carry4 = (h4 + (int64_t)(1 << 25)) >> 26;
    h5 += carry4;
    h4 -= carry4 << 26;
    carry8 = (h8 + (int64_t)(1 << 25)) >> 26;
    h9 += carry8;
    h8 -= carry8 << 26;

    carry9 = (h9 + (int64_t)(1 << 24)) >> 25;
    h0 += carry9 * 19;
    h9 -= carry9 << 25;

    carry0 = (h0 + (int64_t)(1 << 25)) >> 26;
    h1 += carry0;
    h0 -= carry0 << 26;

    h[0] = (int32_t)h0;
    h[1] = (int32_t)h1;
    h[2] = (int32_t)h2;
    h[3] = (int32_t)h3;
    h[4] = (int32_t)h4;
    h[5] = (int32_t)h5;
    h[6] = (int32_t)h6;
    h[7] = (int32_t)h7;
    h[8] = (int32_t)h8;
    h[9] = (int32_t)h9;
}

/* From feSq2.c */

/*
h = 2 * f * f
Can overlap h with f.

Preconditions:
   |f| bounded by 1.65*2^26,1.65*2^25,1.65*2^26,1.65*2^25,etc.

Postconditions:
   |h| bounded by 1.01*2^25,1.01*2^24,1.01*2^25,1.01*2^24,etc.
*/

/*
See feMul.c for discussion of implementation strategy.
*/

static void feSq2(fe h, const fe f)
{
    int32_t f0 = f[0];
    int32_t f1 = f[1];
    int32_t f2 = f[2];
    int32_t f3 = f[3];
    int32_t f4 = f[4];
    int32_t f5 = f[5];
    int32_t f6 = f[6];
    int32_t f7 = f[7];
    int32_t f8 = f[8];
    int32_t f9 = f[9];
    int32_t f0_2 = 2 * f0;
    int32_t f1_2 = 2 * f1;
    int32_t f2_2 = 2 * f2;
    int32_t f3_2 = 2 * f3;
    int32_t f4_2 = 2 * f4;
    int32_t f5_2 = 2 * f5;
    int32_t f6_2 = 2 * f6;
    int32_t f7_2 = 2 * f7;
    int32_t f5_38 = 38 * f5; /* 1.959375*2^30 */
    int32_t f6_19 = 19 * f6; /* 1.959375*2^30 */
    int32_t f7_38 = 38 * f7; /* 1.959375*2^30 */
    int32_t f8_19 = 19 * f8; /* 1.959375*2^30 */
    int32_t f9_38 = 38 * f9; /* 1.959375*2^30 */
    int64_t f0f0 = f0 * (int64_t)f0;
    int64_t f0f1_2 = f0_2 * (int64_t)f1;
    int64_t f0f2_2 = f0_2 * (int64_t)f2;
    int64_t f0f3_2 = f0_2 * (int64_t)f3;
    int64_t f0f4_2 = f0_2 * (int64_t)f4;
    int64_t f0f5_2 = f0_2 * (int64_t)f5;
    int64_t f0f6_2 = f0_2 * (int64_t)f6;
    int64_t f0f7_2 = f0_2 * (int64_t)f7;
    int64_t f0f8_2 = f0_2 * (int64_t)f8;
    int64_t f0f9_2 = f0_2 * (int64_t)f9;
    int64_t f1f1_2 = f1_2 * (int64_t)f1;
    int64_t f1f2_2 = f1_2 * (int64_t)f2;
    int64_t f1f3_4 = f1_2 * (int64_t)f3_2;
    int64_t f1f4_2 = f1_2 * (int64_t)f4;
    int64_t f1f5_4 = f1_2 * (int64_t)f5_2;
    int64_t f1f6_2 = f1_2 * (int64_t)f6;
    int64_t f1f7_4 = f1_2 * (int64_t)f7_2;
    int64_t f1f8_2 = f1_2 * (int64_t)f8;
    int64_t f1f9_76 = f1_2 * (int64_t)f9_38;
    int64_t f2f2 = f2 * (int64_t)f2;
    int64_t f2f3_2 = f2_2 * (int64_t)f3;
    int64_t f2f4_2 = f2_2 * (int64_t)f4;
    int64_t f2f5_2 = f2_2 * (int64_t)f5;
    int64_t f2f6_2 = f2_2 * (int64_t)f6;
    int64_t f2f7_2 = f2_2 * (int64_t)f7;
    int64_t f2f8_38 = f2_2 * (int64_t)f8_19;
    int64_t f2f9_38 = f2 * (int64_t)f9_38;
    int64_t f3f3_2 = f3_2 * (int64_t)f3;
    int64_t f3f4_2 = f3_2 * (int64_t)f4;
    int64_t f3f5_4 = f3_2 * (int64_t)f5_2;
    int64_t f3f6_2 = f3_2 * (int64_t)f6;
    int64_t f3f7_76 = f3_2 * (int64_t)f7_38;
    int64_t f3f8_38 = f3_2 * (int64_t)f8_19;
    int64_t f3f9_76 = f3_2 * (int64_t)f9_38;
    int64_t f4f4 = f4 * (int64_t)f4;
    int64_t f4f5_2 = f4_2 * (int64_t)f5;
    int64_t f4f6_38 = f4_2 * (int64_t)f6_19;
    int64_t f4f7_38 = f4 * (int64_t)f7_38;
    int64_t f4f8_38 = f4_2 * (int64_t)f8_19;
    int64_t f4f9_38 = f4 * (int64_t)f9_38;
    int64_t f5f5_38 = f5 * (int64_t)f5_38;
    int64_t f5f6_38 = f5_2 * (int64_t)f6_19;
    int64_t f5f7_76 = f5_2 * (int64_t)f7_38;
    int64_t f5f8_38 = f5_2 * (int64_t)f8_19;
    int64_t f5f9_76 = f5_2 * (int64_t)f9_38;
    int64_t f6f6_19 = f6 * (int64_t)f6_19;
    int64_t f6f7_38 = f6 * (int64_t)f7_38;
    int64_t f6f8_38 = f6_2 * (int64_t)f8_19;
    int64_t f6f9_38 = f6 * (int64_t)f9_38;
    int64_t f7f7_38 = f7 * (int64_t)f7_38;
    int64_t f7f8_38 = f7_2 * (int64_t)f8_19;
    int64_t f7f9_76 = f7_2 * (int64_t)f9_38;
    int64_t f8f8_19 = f8 * (int64_t)f8_19;
    int64_t f8f9_38 = f8 * (int64_t)f9_38;
    int64_t f9f9_38 = f9 * (int64_t)f9_38;
    int64_t h0 = f0f0 + f1f9_76 + f2f8_38 + f3f7_76 + f4f6_38 + f5f5_38;
    int64_t h1 = f0f1_2 + f2f9_38 + f3f8_38 + f4f7_38 + f5f6_38;
    int64_t h2 = f0f2_2 + f1f1_2 + f3f9_76 + f4f8_38 + f5f7_76 + f6f6_19;
    int64_t h3 = f0f3_2 + f1f2_2 + f4f9_38 + f5f8_38 + f6f7_38;
    int64_t h4 = f0f4_2 + f1f3_4 + f2f2 + f5f9_76 + f6f8_38 + f7f7_38;
    int64_t h5 = f0f5_2 + f1f4_2 + f2f3_2 + f6f9_38 + f7f8_38;
    int64_t h6 = f0f6_2 + f1f5_4 + f2f4_2 + f3f3_2 + f7f9_76 + f8f8_19;
    int64_t h7 = f0f7_2 + f1f6_2 + f2f5_2 + f3f4_2 + f8f9_38;
    int64_t h8 = f0f8_2 + f1f7_4 + f2f6_2 + f3f5_4 + f4f4 + f9f9_38;
    int64_t h9 = f0f9_2 + f1f8_2 + f2f7_2 + f3f6_2 + f4f5_2;
    int64_t carry0;
    int64_t carry1;
    int64_t carry2;
    int64_t carry3;
    int64_t carry4;
    int64_t carry5;
    int64_t carry6;
    int64_t carry7;
    int64_t carry8;
    int64_t carry9;

    h0 += h0;
    h1 += h1;
    h2 += h2;
    h3 += h3;
    h4 += h4;
    h5 += h5;
    h6 += h6;
    h7 += h7;
    h8 += h8;
    h9 += h9;

    carry0 = (h0 + (int64_t)(1 << 25)) >> 26;
    h1 += carry0;
    h0 -= carry0 << 26;
    carry4 = (h4 + (int64_t)(1 << 25)) >> 26;
    h5 += carry4;
    h4 -= carry4 << 26;

    carry1 = (h1 + (int64_t)(1 << 24)) >> 25;
    h2 += carry1;
    h1 -= carry1 << 25;
    carry5 = (h5 + (int64_t)(1 << 24)) >> 25;
    h6 += carry5;
    h5 -= carry5 << 25;

    carry2 = (h2 + (int64_t)(1 << 25)) >> 26;
    h3 += carry2;
    h2 -= carry2 << 26;
    carry6 = (h6 + (int64_t)(1 << 25)) >> 26;
    h7 += carry6;
    h6 -= carry6 << 26;

    carry3 = (h3 + (int64_t)(1 << 24)) >> 25;
    h4 += carry3;
    h3 -= carry3 << 25;
    carry7 = (h7 + (int64_t)(1 << 24)) >> 25;
    h8 += carry7;
    h7 -= carry7 << 25;

    carry4 = (h4 + (int64_t)(1 << 25)) >> 26;
    h5 += carry4;
    h4 -= carry4 << 26;
    carry8 = (h8 + (int64_t)(1 << 25)) >> 26;
    h9 += carry8;
    h8 -= carry8 << 26;

    carry9 = (h9 + (int64_t)(1 << 24)) >> 25;
    h0 += carry9 * 19;
    h9 -= carry9 << 25;

    carry0 = (h0 + (int64_t)(1 << 25)) >> 26;
    h1 += carry0;
    h0 -= carry0 << 26;

    h[0] = (int32_t)h0;
    h[1] = (int32_t)h1;
    h[2] = (int32_t)h2;
    h[3] = (int32_t)h3;
    h[4] = (int32_t)h4;
    h[5] = (int32_t)h5;
    h[6] = (int32_t)h6;
    h[7] = (int32_t)h7;
    h[8] = (int32_t)h8;
    h[9] = (int32_t)h9;
}

/* From feSub.c */

/*
h = f - g
Can overlap h with f or g.

Preconditions:
   |f| bounded by 1.1*2^25,1.1*2^24,1.1*2^25,1.1*2^24,etc.
   |g| bounded by 1.1*2^25,1.1*2^24,1.1*2^25,1.1*2^24,etc.

Postconditions:
   |h| bounded by 1.1*2^26,1.1*2^25,1.1*2^26,1.1*2^25,etc.
*/

static void feSub(fe h, const fe f, const fe g)
{
    int32_t f0 = f[0];
    int32_t f1 = f[1];
    int32_t f2 = f[2];
    int32_t f3 = f[3];
    int32_t f4 = f[4];
    int32_t f5 = f[5];
    int32_t f6 = f[6];
    int32_t f7 = f[7];
    int32_t f8 = f[8];
    int32_t f9 = f[9];
    int32_t g0 = g[0];
    int32_t g1 = g[1];
    int32_t g2 = g[2];
    int32_t g3 = g[3];
    int32_t g4 = g[4];
    int32_t g5 = g[5];
    int32_t g6 = g[6];
    int32_t g7 = g[7];
    int32_t g8 = g[8];
    int32_t g9 = g[9];
    int32_t h0 = f0 - g0;
    int32_t h1 = f1 - g1;
    int32_t h2 = f2 - g2;
    int32_t h3 = f3 - g3;
    int32_t h4 = f4 - g4;
    int32_t h5 = f5 - g5;
    int32_t h6 = f6 - g6;
    int32_t h7 = f7 - g7;
    int32_t h8 = f8 - g8;
    int32_t h9 = f9 - g9;
    h[0] = h0;
    h[1] = h1;
    h[2] = h2;
    h[3] = h3;
    h[4] = h4;
    h[5] = h5;
    h[6] = h6;
    h[7] = h7;
    h[8] = h8;
    h[9] = h9;
}

/* From feToBytes.c */

/*
Preconditions:
  |h| bounded by 1.1*2^26,1.1*2^25,1.1*2^26,1.1*2^25,etc.

Write p=2^255-19; q=floor(h/p).
Basic claim: q = floor(2^(-255)(h + 19 2^(-25)h9 + 2^(-1))).

Proof:
  Have |h|<=p so |q|<=1 so |19^2 2^(-255) q|<1/4.
  Also have |h-2^230 h9|<2^231 so |19 2^(-255)(h-2^230 h9)|<1/4.

  Write y=2^(-1)-19^2 2^(-255)q-19 2^(-255)(h-2^230 h9).
  Then 0<y<1.

  Write r=h-pq.
  Have 0<=r<=p-1=2^255-20.
  Thus 0<=r+19(2^-255)r<r+19(2^-255)2^255<=2^255-1.

  Write x=r+19(2^-255)r+y.
  Then 0<x<2^255 so floor(2^(-255)x) = 0 so floor(q+2^(-255)x) = q.

  Have q+2^(-255)x = 2^(-255)(h + 19 2^(-25) h9 + 2^(-1))
  so floor(2^(-255)(h + 19 2^(-25) h9 + 2^(-1))) = q.
*/

static void feToBytes(unsigned char *s, const fe h)
{
    int32_t h0 = h[0];
    int32_t h1 = h[1];
    int32_t h2 = h[2];
    int32_t h3 = h[3];
    int32_t h4 = h[4];
    int32_t h5 = h[5];
    int32_t h6 = h[6];
    int32_t h7 = h[7];
    int32_t h8 = h[8];
    int32_t h9 = h[9];
    int32_t q;
    int32_t carry0;
    int32_t carry1;
    int32_t carry2;
    int32_t carry3;
    int32_t carry4;
    int32_t carry5;
    int32_t carry6;
    int32_t carry7;
    int32_t carry8;
    int32_t carry9;

    q = (19 * h9 + (((int32_t)1) << 24)) >> 25;
    q = (h0 + q) >> 26;
    q = (h1 + q) >> 25;
    q = (h2 + q) >> 26;
    q = (h3 + q) >> 25;
    q = (h4 + q) >> 26;
    q = (h5 + q) >> 25;
    q = (h6 + q) >> 26;
    q = (h7 + q) >> 25;
    q = (h8 + q) >> 26;
    q = (h9 + q) >> 25;

    /* Goal: Output h-(2^255-19)q, which is between 0 and 2^255-20. */
    h0 += 19 * q;
    /* Goal: Output h-2^255 q, which is between 0 and 2^255-20. */

    carry0 = h0 >> 26;
    h1 += carry0;
    h0 -= carry0 << 26;
    carry1 = h1 >> 25;
    h2 += carry1;
    h1 -= carry1 << 25;
    carry2 = h2 >> 26;
    h3 += carry2;
    h2 -= carry2 << 26;
    carry3 = h3 >> 25;
    h4 += carry3;
    h3 -= carry3 << 25;
    carry4 = h4 >> 26;
    h5 += carry4;
    h4 -= carry4 << 26;
    carry5 = h5 >> 25;
    h6 += carry5;
    h5 -= carry5 << 25;
    carry6 = h6 >> 26;
    h7 += carry6;
    h6 -= carry6 << 26;
    carry7 = h7 >> 25;
    h8 += carry7;
    h7 -= carry7 << 25;
    carry8 = h8 >> 26;
    h9 += carry8;
    h8 -= carry8 << 26;
    carry9 = h9 >> 25;
    h9 -= carry9 << 25;
    /* h10 = carry9 */

    /*
    Goal: Output h0+...+2^255 h10-2^255 q, which is between 0 and 2^255-20.
    Have h0+...+2^230 h9 between 0 and 2^255-1;
    evidently 2^255 h10-2^255 q = 0.
    Goal: Output h0+...+2^230 h9.
    */

    s[0] = h0 >> 0;
    s[1] = h0 >> 8;
    s[2] = h0 >> 16;
    s[3] = (h0 >> 24) | (h1 << 2);
    s[4] = h1 >> 6;
    s[5] = h1 >> 14;
    s[6] = (h1 >> 22) | (h2 << 3);
    s[7] = h2 >> 5;
    s[8] = h2 >> 13;
    s[9] = (h2 >> 21) | (h3 << 5);
    s[10] = h3 >> 3;
    s[11] = h3 >> 11;
    s[12] = (h3 >> 19) | (h4 << 6);
    s[13] = h4 >> 2;
    s[14] = h4 >> 10;
    s[15] = h4 >> 18;
    s[16] = h5 >> 0;
    s[17] = h5 >> 8;
    s[18] = h5 >> 16;
    s[19] = (h5 >> 24) | (h6 << 1);
    s[20] = h6 >> 7;
    s[21] = h6 >> 15;
    s[22] = (h6 >> 23) | (h7 << 3);
    s[23] = h7 >> 5;
    s[24] = h7 >> 13;
    s[25] = (h7 >> 21) | (h8 << 4);
    s[26] = h8 >> 4;
    s[27] = h8 >> 12;
    s[28] = (h8 >> 20) | (h9 << 6);
    s[29] = h9 >> 2;
    s[30] = h9 >> 10;
    s[31] = h9 >> 18;
}

/* From geAdd.c */

void geAdd(GeP1P1 *r, const GeP3 *p, const GeCached *q)
{
    fe t0;
    feAdd(r->X, p->Y, p->X);
    feSub(r->Y, p->Y, p->X);
    feMul(r->Z, r->X, q->YPlusX);
    feMul(r->Y, r->Y, q->YMinusX);
    feMul(r->T, q->T2D, p->T);
    feMul(r->X, p->Z, q->Z);
    feAdd(t0, r->X, r->X);
    feSub(r->X, r->Z, r->Y);
    feAdd(r->Y, r->Z, r->Y);
    feAdd(r->Z, t0, r->T);
    feSub(r->T, t0, r->T);
}

/* From ge_double_scalarmult.c, modified */

static void slide(signed char *r, const unsigned char *a)
{
    int i;
    int b;
    int k;

    for (i = 0; i < 256; ++i) {
        r[i] = 1 & (a[i >> 3] >> (i & 7));
    }

    for (i = 0; i < 256; ++i) {
        if (r[i]) {
            for (b = 1; b <= 6 && i + b < 256; ++b) {
                if (r[i + b]) {
                    if (r[i] + (r[i + b] << b) <= 15) {
                        r[i] += r[i + b] << b;
                        r[i + b] = 0;
                    } else if (r[i] - (r[i + b] << b) >= -15) {
                        r[i] -= r[i + b] << b;
                        for (k = i + b; k < 256; ++k) {
                            if (!r[k]) {
                                r[k] = 1;
                                break;
                            }
                            r[k] = 0;
                        }
                    } else
                        break;
                }
            }
        }
    }
}

void geDsmPrecomp(geDsmp r, const GeP3 *s)
{
    GeP1P1 t;
    GeP3 s2, u;
    geP3ToCached(&r[0], s);
    geP3Dbl(&t, s);
    geP1P1ToP3(&s2, &t);
    geAdd(&t, &s2, &r[0]);
    geP1P1ToP3(&u, &t);
    geP3ToCached(&r[1], &u);
    geAdd(&t, &s2, &r[1]);
    geP1P1ToP3(&u, &t);
    geP3ToCached(&r[2], &u);
    geAdd(&t, &s2, &r[2]);
    geP1P1ToP3(&u, &t);
    geP3ToCached(&r[3], &u);
    geAdd(&t, &s2, &r[3]);
    geP1P1ToP3(&u, &t);
    geP3ToCached(&r[4], &u);
    geAdd(&t, &s2, &r[4]);
    geP1P1ToP3(&u, &t);
    geP3ToCached(&r[5], &u);
    geAdd(&t, &s2, &r[5]);
    geP1P1ToP3(&u, &t);
    geP3ToCached(&r[6], &u);
    geAdd(&t, &s2, &r[6]);
    geP1P1ToP3(&u, &t);
    geP3ToCached(&r[7], &u);
}

/*
r = a * A + b * B
where a = a[0]+256*a[1]+...+256^31 a[31].
and b = b[0]+256*b[1]+...+256^31 b[31].
B is the Ed25519 base point (x,4/5) with x positive.
*/

void geDoubleScalarmultBaseVartime(GeP2 *r,
                                   const unsigned char *a,
                                   const GeP3 *A,
                                   const unsigned char *b)
{
    signed char aSlide[256];
    signed char bSlide[256];
    geDsmp Ai; /* A, 3A, 5A, 7A, 9A, 11A, 13A, 15A */
    GeP1P1 t;
    GeP3 u;
    int i;

    slide(aSlide, a);
    slide(bSlide, b);
    geDsmPrecomp(Ai, A);

    geP20(r);

    for (i = 255; i >= 0; --i) {
        if (aSlide[i] || bSlide[i])
            break;
    }

    for (; i >= 0; --i) {
        geP2Dbl(&t, r);

        if (aSlide[i] > 0) {
            geP1P1ToP3(&u, &t);
            geAdd(&t, &u, &Ai[aSlide[i] / 2]);
        } else if (aSlide[i] < 0) {
            geP1P1ToP3(&u, &t);
            geSub(&t, &u, &Ai[(-aSlide[i]) / 2]);
        }

        if (bSlide[i] > 0) {
            geP1P1ToP3(&u, &t);
            geMAdd(&t, &u, &geBi[bSlide[i] / 2]);
        } else if (bSlide[i] < 0) {
            geP1P1ToP3(&u, &t);
            geMSub(&t, &u, &geBi[(-bSlide[i]) / 2]);
        }

        geP1P1ToP2(r, &t);
    }
}

/* From ge_frombytes.c, modified */

int geFromBytesVarTime(GeP3 *h, const unsigned char *s)
{
    fe u;
    fe v;
    fe vxx;
    fe check;

    /* From fe_frombytes.c */

    int64_t h0 = load4(s);
    int64_t h1 = load3(s + 4) << 6;
    int64_t h2 = load3(s + 7) << 5;
    int64_t h3 = load3(s + 10) << 3;
    int64_t h4 = load3(s + 13) << 2;
    int64_t h5 = load4(s + 16);
    int64_t h6 = load3(s + 20) << 7;
    int64_t h7 = load3(s + 23) << 5;
    int64_t h8 = load3(s + 26) << 4;
    int64_t h9 = (load3(s + 29) & 8388607) << 2;
    int64_t carry0;
    int64_t carry1;
    int64_t carry2;
    int64_t carry3;
    int64_t carry4;
    int64_t carry5;
    int64_t carry6;
    int64_t carry7;
    int64_t carry8;
    int64_t carry9;

    /* Validate the number to be canonical */
    if (h9 == 33554428 && h8 == 268435440 && h7 == 536870880 && h6 == 2147483520 && h5 == 4294967295
        && h4 == 67108860 && h3 == 134217720 && h2 == 536870880 && h1 == 1073741760
        && h0 >= 4294967277) {
        return -1;
    }

    carry9 = (h9 + (int64_t)(1 << 24)) >> 25;
    h0 += carry9 * 19;
    h9 -= carry9 << 25;
    carry1 = (h1 + (int64_t)(1 << 24)) >> 25;
    h2 += carry1;
    h1 -= carry1 << 25;
    carry3 = (h3 + (int64_t)(1 << 24)) >> 25;
    h4 += carry3;
    h3 -= carry3 << 25;
    carry5 = (h5 + (int64_t)(1 << 24)) >> 25;
    h6 += carry5;
    h5 -= carry5 << 25;
    carry7 = (h7 + (int64_t)(1 << 24)) >> 25;
    h8 += carry7;
    h7 -= carry7 << 25;

    carry0 = (h0 + (int64_t)(1 << 25)) >> 26;
    h1 += carry0;
    h0 -= carry0 << 26;
    carry2 = (h2 + (int64_t)(1 << 25)) >> 26;
    h3 += carry2;
    h2 -= carry2 << 26;
    carry4 = (h4 + (int64_t)(1 << 25)) >> 26;
    h5 += carry4;
    h4 -= carry4 << 26;
    carry6 = (h6 + (int64_t)(1 << 25)) >> 26;
    h7 += carry6;
    h6 -= carry6 << 26;
    carry8 = (h8 + (int64_t)(1 << 25)) >> 26;
    h9 += carry8;
    h8 -= carry8 << 26;

    h->Y[0] = (int32_t)h0;
    h->Y[1] = (int32_t)h1;
    h->Y[2] = (int32_t)h2;
    h->Y[3] = (int32_t)h3;
    h->Y[4] = (int32_t)h4;
    h->Y[5] = (int32_t)h5;
    h->Y[6] = (int32_t)h6;
    h->Y[7] = (int32_t)h7;
    h->Y[8] = (int32_t)h8;
    h->Y[9] = (int32_t)h9;

    /* End fe_frombytes.c */

    fe1(h->Z);
    feSq(u, h->Y);
    feMul(v, u, feD);
    feSub(u, u, h->Z); /* u = y^2-1 */
    feAdd(v, v, h->Z); /* v = dy^2+1 */

    feDivPowM1(h->X, u, v); /* x = uv^3(uv^7)^((q-5)/8) */

    feSq(vxx, h->X);
    feMul(vxx, vxx, v);
    feSub(check, vxx, u); /* vx^2-u */
    if (feIsNonZero(check)) {
        feAdd(check, vxx, u); /* vx^2+u */
        if (feIsNonZero(check)) {
            return -1;
        }
        feMul(h->X, h->X, feSqrtm1);
    }

    if (feIsNegative(h->X) != (s[31] >> 7)) {
        /* If x = 0, the sign must be positive */
        if (!feIsNonZero(h->X)) {
            return -1;
        }
        feNeg(h->X, h->X);
    }

    feMul(h->T, h->X, h->Y);
    return 0;
}

/* From geMAdd.c */

/*
r = p + q
*/

static void geMAdd(GeP1P1 *r, const GeP3 *p, const GePrecomp *q)
{
    fe t0;
    feAdd(r->X, p->Y, p->X);
    feSub(r->Y, p->Y, p->X);
    feMul(r->Z, r->X, q->YPlusX);
    feMul(r->Y, r->Y, q->YMinusX);
    feMul(r->T, q->XY2D, p->T);
    feAdd(t0, p->Z, p->Z);
    feSub(r->X, r->Z, r->Y);
    feAdd(r->Y, r->Z, r->Y);
    feAdd(r->Z, t0, r->T);
    feSub(r->T, t0, r->T);
}

/* From geMSub.c */

/*
r = p - q
*/

static void geMSub(GeP1P1 *r, const GeP3 *p, const GePrecomp *q)
{
    fe t0;
    feAdd(r->X, p->Y, p->X);
    feSub(r->Y, p->Y, p->X);
    feMul(r->Z, r->X, q->YMinusX);
    feMul(r->Y, r->Y, q->YPlusX);
    feMul(r->T, q->XY2D, p->T);
    feAdd(t0, p->Z, p->Z);
    feSub(r->X, r->Z, r->Y);
    feAdd(r->Y, r->Z, r->Y);
    feSub(r->Z, t0, r->T);
    feAdd(r->T, t0, r->T);
}

/* From geP1P1ToP2.c */

/*
r = p
*/

void geP1P1ToP2(GeP2 *r, const GeP1P1 *p)
{
    feMul(r->X, p->X, p->T);
    feMul(r->Y, p->Y, p->Z);
    feMul(r->Z, p->Z, p->T);
}

/* From geP1P1ToP3.c */

/*
r = p
*/

void geP1P1ToP3(GeP3 *r, const GeP1P1 *p)
{
    feMul(r->X, p->X, p->T);
    feMul(r->Y, p->Y, p->Z);
    feMul(r->Z, p->Z, p->T);
    feMul(r->T, p->X, p->Y);
}

/* From geP20.c */

static void geP20(GeP2 *h)
{
    fe0(h->X);
    fe1(h->Y);
    fe1(h->Z);
}

/* From geP2Dbl.c */

/*
r = 2 * p
*/

void geP2Dbl(GeP1P1 *r, const GeP2 *p)
{
    fe t0;
    feSq(r->X, p->X);
    feSq(r->Z, p->Y);
    feSq2(r->T, p->Z);
    feAdd(r->Y, p->X, p->Y);
    feSq(t0, r->Y);
    feAdd(r->Y, r->Z, r->X);
    feSub(r->Z, r->Z, r->X);
    feSub(r->X, t0, r->Y);
    feSub(r->T, r->T, r->Z);
}

/* From geP30.c */

static void geP30(GeP3 *h)
{
    fe0(h->X);
    fe1(h->Y);
    fe1(h->Z);
    fe0(h->T);
}

/* From geP3Dbl.c */

/*
r = 2 * p
*/

static void geP3Dbl(GeP1P1 *r, const GeP3 *p)
{
    GeP2 q;
    geP3ToP2(&q, p);
    geP2Dbl(r, &q);
}

/* From geP3ToCached.c */

/*
r = p
*/

void geP3ToCached(GeCached *r, const GeP3 *p)
{
    feAdd(r->YPlusX, p->Y, p->X);
    feSub(r->YMinusX, p->Y, p->X);
    feCopy(r->Z, p->Z);
    feMul(r->T2D, p->T, feD2);
}

/* From geP3ToP2.c */

/*
r = p
*/

void geP3ToP2(GeP2 *r, const GeP3 *p)
{
    feCopy(r->X, p->X);
    feCopy(r->Y, p->Y);
    feCopy(r->Z, p->Z);
}

/* From geP3Tobytes.c */

void geP3Tobytes(unsigned char *s, const GeP3 *h)
{
    fe recip;
    fe x;
    fe y;

    feInvert(recip, h->Z);
    feMul(x, h->X, recip);
    feMul(y, h->Y, recip);
    feToBytes(s, y);
    s[31] ^= feIsNegative(x) << 7;
}

/* From gePrecomp0.c */

static void gePrecomp0(GePrecomp *h)
{
    fe1(h->YPlusX);
    fe1(h->YMinusX);
    fe0(h->XY2D);
}

/* From geScalarMultBase.c */

static unsigned char equal(signed char b, signed char c)
{
    unsigned char ub = b;
    unsigned char uc = c;
    unsigned char x = ub ^ uc; /* 0: yes; 1..255: no */
    uint32_t y = x; /* 0: yes; 1..255: no */
    y -= 1; /* 4294967295: yes; 0..254: no */
    y >>= 31; /* 1: yes; 0: no */

    return y;
}

static unsigned char negative(signed char b)
{
    unsigned long long x = b; /* 18446744073709551361..18446744073709551615: yes; 0..255: no */
    x >>= 63; /* 1: yes; 0: no */

    return (unsigned char)x;
}

static void gePrecompCMov(GePrecomp *t,
                          const GePrecomp *u,
                          unsigned char b)
{
    feCmov(t->YPlusX, u->YPlusX, b);
    feCmov(t->YMinusX, u->YMinusX, b);
    feCmov(t->XY2D, u->XY2D, b);
}

static void select(GePrecomp *t, int pos, signed char b)
{
    GePrecomp minusT;
    unsigned char bNegative = negative(b);
    unsigned char bAbs = b - (((-bNegative) & b) << 1);

    gePrecomp0(t);
    gePrecompCMov(t, &ge_base[pos][0], equal(bAbs, 1));
    gePrecompCMov(t, &ge_base[pos][1], equal(bAbs, 2));
    gePrecompCMov(t, &ge_base[pos][2], equal(bAbs, 3));
    gePrecompCMov(t, &ge_base[pos][3], equal(bAbs, 4));
    gePrecompCMov(t, &ge_base[pos][4], equal(bAbs, 5));
    gePrecompCMov(t, &ge_base[pos][5], equal(bAbs, 6));
    gePrecompCMov(t, &ge_base[pos][6], equal(bAbs, 7));
    gePrecompCMov(t, &ge_base[pos][7], equal(bAbs, 8));
    feCopy(minusT.YPlusX, t->YMinusX);
    feCopy(minusT.YMinusX, t->YPlusX);
    feNeg(minusT.XY2D, t->XY2D);
    gePrecompCMov(t, &minusT, bNegative);
}

/*
h = a * B
where a = a[0]+256*a[1]+...+256^31 a[31]
B is the Ed25519 base point (x,4/5) with x positive.

Preconditions:
  a[31] <= 127
*/

void geScalarMultBase(GeP3 *h, const unsigned char *a)
{
    signed char e[64];
    signed char carry;
    GeP1P1 r;
    GeP2 s;
    GePrecomp t;
    int i;

    for (i = 0; i < 32; ++i) {
        e[2 * i + 0] = (a[i] >> 0) & 15;
        e[2 * i + 1] = (a[i] >> 4) & 15;
    }
    /* each e[i] is between 0 and 15 */
    /* e[63] is between 0 and 7 */

    carry = 0;
    for (i = 0; i < 63; ++i) {
        e[i] += carry;
        carry = e[i] + 8;
        carry >>= 4;
        e[i] -= carry << 4;
    }
    e[63] += carry;
    /* each e[i] is between -8 and 8 */

    geP30(h);
    for (i = 1; i < 64; i += 2) {
        select(&t, i / 2, e[i]);
        geMAdd(&r, h, &t);
        geP1P1ToP3(h, &r);
    }

    geP3Dbl(&r, h);
    geP1P1ToP2(&s, &r);
    geP2Dbl(&r, &s);
    geP1P1ToP2(&s, &r);
    geP2Dbl(&r, &s);
    geP1P1ToP2(&s, &r);
    geP2Dbl(&r, &s);
    geP1P1ToP3(h, &r);

    for (i = 0; i < 64; i += 2) {
        select(&t, i / 2, e[i]);
        geMAdd(&r, h, &t);
        geP1P1ToP3(h, &r);
    }
}

/* From geSub.c */

/*
r = p - q
*/

void geSub(GeP1P1 *r, const GeP3 *p, const GeCached *q)
{
    fe t0;
    feAdd(r->X, p->Y, p->X);
    feSub(r->Y, p->Y, p->X);
    feMul(r->Z, r->X, q->YMinusX);
    feMul(r->Y, r->Y, q->YPlusX);
    feMul(r->T, q->T2D, p->T);
    feMul(r->X, p->Z, q->Z);
    feAdd(t0, r->X, r->X);
    feSub(r->X, r->Z, r->Y);
    feAdd(r->Y, r->Z, r->Y);
    feSub(r->Z, t0, r->T);
    feAdd(r->T, t0, r->T);
}

/* From geToBytes.c */

void geToBytes(unsigned char *s, const GeP2 *h)
{
    fe recip;
    fe x;
    fe y;

    feInvert(recip, h->Z);
    feMul(x, h->X, recip);
    feMul(y, h->Y, recip);
    feToBytes(s, y);
    s[31] ^= feIsNegative(x) << 7;
}

/* From scReduce.c */

/*
Input:
  s[0]+256*s[1]+...+256^63*s[63] = s

Output:
  s[0]+256*s[1]+...+256^31*s[31] = s mod l
  where l = 2^252 + 27742317777372353535851937790883648493.
  Overwrites s in place.
*/

void scReduce(unsigned char *s)
{
    int64_t s0 = 2097151 & load3(s);
    int64_t s1 = 2097151 & (load4(s + 2) >> 5);
    int64_t s2 = 2097151 & (load3(s + 5) >> 2);
    int64_t s3 = 2097151 & (load4(s + 7) >> 7);
    int64_t s4 = 2097151 & (load4(s + 10) >> 4);
    int64_t s5 = 2097151 & (load3(s + 13) >> 1);
    int64_t s6 = 2097151 & (load4(s + 15) >> 6);
    int64_t s7 = 2097151 & (load3(s + 18) >> 3);
    int64_t s8 = 2097151 & load3(s + 21);
    int64_t s9 = 2097151 & (load4(s + 23) >> 5);
    int64_t s10 = 2097151 & (load3(s + 26) >> 2);
    int64_t s11 = 2097151 & (load4(s + 28) >> 7);
    int64_t s12 = 2097151 & (load4(s + 31) >> 4);
    int64_t s13 = 2097151 & (load3(s + 34) >> 1);
    int64_t s14 = 2097151 & (load4(s + 36) >> 6);
    int64_t s15 = 2097151 & (load3(s + 39) >> 3);
    int64_t s16 = 2097151 & load3(s + 42);
    int64_t s17 = 2097151 & (load4(s + 44) >> 5);
    int64_t s18 = 2097151 & (load3(s + 47) >> 2);
    int64_t s19 = 2097151 & (load4(s + 49) >> 7);
    int64_t s20 = 2097151 & (load4(s + 52) >> 4);
    int64_t s21 = 2097151 & (load3(s + 55) >> 1);
    int64_t s22 = 2097151 & (load4(s + 57) >> 6);
    int64_t s23 = (load4(s + 60) >> 3);
    int64_t carry0;
    int64_t carry1;
    int64_t carry2;
    int64_t carry3;
    int64_t carry4;
    int64_t carry5;
    int64_t carry6;
    int64_t carry7;
    int64_t carry8;
    int64_t carry9;
    int64_t carry10;
    int64_t carry11;
    int64_t carry12;
    int64_t carry13;
    int64_t carry14;
    int64_t carry15;
    int64_t carry16;

    s11 += s23 * 666643;
    s12 += s23 * 470296;
    s13 += s23 * 654183;
    s14 -= s23 * 997805;
    s15 += s23 * 136657;
    s16 -= s23 * 683901;

    s10 += s22 * 666643;
    s11 += s22 * 470296;
    s12 += s22 * 654183;
    s13 -= s22 * 997805;
    s14 += s22 * 136657;
    s15 -= s22 * 683901;

    s9 += s21 * 666643;
    s10 += s21 * 470296;
    s11 += s21 * 654183;
    s12 -= s21 * 997805;
    s13 += s21 * 136657;
    s14 -= s21 * 683901;

    s8 += s20 * 666643;
    s9 += s20 * 470296;
    s10 += s20 * 654183;
    s11 -= s20 * 997805;
    s12 += s20 * 136657;
    s13 -= s20 * 683901;

    s7 += s19 * 666643;
    s8 += s19 * 470296;
    s9 += s19 * 654183;
    s10 -= s19 * 997805;
    s11 += s19 * 136657;
    s12 -= s19 * 683901;

    s6 += s18 * 666643;
    s7 += s18 * 470296;
    s8 += s18 * 654183;
    s9 -= s18 * 997805;
    s10 += s18 * 136657;
    s11 -= s18 * 683901;

    carry6 = (s6 + (1 << 20)) >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry8 = (s8 + (1 << 20)) >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry10 = (s10 + (1 << 20)) >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;
    carry12 = (s12 + (1 << 20)) >> 21;
    s13 += carry12;
    s12 -= carry12 << 21;
    carry14 = (s14 + (1 << 20)) >> 21;
    s15 += carry14;
    s14 -= carry14 << 21;
    carry16 = (s16 + (1 << 20)) >> 21;
    s17 += carry16;
    s16 -= carry16 << 21;

    carry7 = (s7 + (1 << 20)) >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry9 = (s9 + (1 << 20)) >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry11 = (s11 + (1 << 20)) >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;
    carry13 = (s13 + (1 << 20)) >> 21;
    s14 += carry13;
    s13 -= carry13 << 21;
    carry15 = (s15 + (1 << 20)) >> 21;
    s16 += carry15;
    s15 -= carry15 << 21;

    s5 += s17 * 666643;
    s6 += s17 * 470296;
    s7 += s17 * 654183;
    s8 -= s17 * 997805;
    s9 += s17 * 136657;
    s10 -= s17 * 683901;

    s4 += s16 * 666643;
    s5 += s16 * 470296;
    s6 += s16 * 654183;
    s7 -= s16 * 997805;
    s8 += s16 * 136657;
    s9 -= s16 * 683901;

    s3 += s15 * 666643;
    s4 += s15 * 470296;
    s5 += s15 * 654183;
    s6 -= s15 * 997805;
    s7 += s15 * 136657;
    s8 -= s15 * 683901;

    s2 += s14 * 666643;
    s3 += s14 * 470296;
    s4 += s14 * 654183;
    s5 -= s14 * 997805;
    s6 += s14 * 136657;
    s7 -= s14 * 683901;

    s1 += s13 * 666643;
    s2 += s13 * 470296;
    s3 += s13 * 654183;
    s4 -= s13 * 997805;
    s5 += s13 * 136657;
    s6 -= s13 * 683901;

    s0 += s12 * 666643;
    s1 += s12 * 470296;
    s2 += s12 * 654183;
    s3 -= s12 * 997805;
    s4 += s12 * 136657;
    s5 -= s12 * 683901;
    s12 = 0;

    carry0 = (s0 + (1 << 20)) >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry2 = (s2 + (1 << 20)) >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry4 = (s4 + (1 << 20)) >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry6 = (s6 + (1 << 20)) >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry8 = (s8 + (1 << 20)) >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry10 = (s10 + (1 << 20)) >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;

    carry1 = (s1 + (1 << 20)) >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry3 = (s3 + (1 << 20)) >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry5 = (s5 + (1 << 20)) >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry7 = (s7 + (1 << 20)) >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry9 = (s9 + (1 << 20)) >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry11 = (s11 + (1 << 20)) >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;

    s0 += s12 * 666643;
    s1 += s12 * 470296;
    s2 += s12 * 654183;
    s3 -= s12 * 997805;
    s4 += s12 * 136657;
    s5 -= s12 * 683901;
    s12 = 0;

    carry0 = s0 >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry1 = s1 >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry2 = s2 >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry3 = s3 >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry4 = s4 >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry5 = s5 >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry6 = s6 >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry7 = s7 >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry8 = s8 >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry9 = s9 >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry10 = s10 >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;
    carry11 = s11 >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;

    s0 += s12 * 666643;
    s1 += s12 * 470296;
    s2 += s12 * 654183;
    s3 -= s12 * 997805;
    s4 += s12 * 136657;
    s5 -= s12 * 683901;

    carry0 = s0 >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry1 = s1 >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry2 = s2 >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry3 = s3 >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry4 = s4 >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry5 = s5 >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry6 = s6 >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry7 = s7 >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry8 = s8 >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry9 = s9 >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry10 = s10 >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;

    s[0] = (unsigned char)(s0 >> 0);
    s[1] = (unsigned char)(s0 >> 8);
    s[2] = (unsigned char)((s0 >> 16) | (s1 << 5));
    s[3] = (unsigned char)(s1 >> 3);
    s[4] = (unsigned char)(s1 >> 11);
    s[5] = (unsigned char)((s1 >> 19) | (s2 << 2));
    s[6] = (unsigned char)(s2 >> 6);
    s[7] = (unsigned char)((s2 >> 14) | (s3 << 7));
    s[8] = (unsigned char)(s3 >> 1);
    s[9] = (unsigned char)(s3 >> 9);
    s[10] = (unsigned char)((s3 >> 17) | (s4 << 4));
    s[11] = (unsigned char)(s4 >> 4);
    s[12] = (unsigned char)(s4 >> 12);
    s[13] = (unsigned char)((s4 >> 20) | (s5 << 1));
    s[14] = (unsigned char)(s5 >> 7);
    s[15] = (unsigned char)((s5 >> 15) | (s6 << 6));
    s[16] = (unsigned char)(s6 >> 2);
    s[17] = (unsigned char)(s6 >> 10);
    s[18] = (unsigned char)((s6 >> 18) | (s7 << 3));
    s[19] = (unsigned char)(s7 >> 5);
    s[20] = (unsigned char)(s7 >> 13);
    s[21] = (unsigned char)(s8 >> 0);
    s[22] = (unsigned char)(s8 >> 8);
    s[23] = (unsigned char)((s8 >> 16) | (s9 << 5));
    s[24] = (unsigned char)(s9 >> 3);
    s[25] = (unsigned char)(s9 >> 11);
    s[26] = (unsigned char)((s9 >> 19) | (s10 << 2));
    s[27] = (unsigned char)(s10 >> 6);
    s[28] = (unsigned char)((s10 >> 14) | (s11 << 7));
    s[29] = (unsigned char)(s11 >> 1);
    s[30] = (unsigned char)(s11 >> 9);
    s[31] = (unsigned char)(s11 >> 17);
}

/* New code */

static void feDivPowM1(fe r, const fe u, const fe v)
{
    fe v3, uv7, t0, t1, t2;
    int i;

    feSq(v3, v);
    feMul(v3, v3, v); /* v3 = v^3 */
    feSq(uv7, v3);
    feMul(uv7, uv7, v);
    feMul(uv7, uv7, u); /* uv7 = uv^7 */

    /*fe_pow22523(uv7, uv7);*/

    /* From fe_pow22523.c */

    feSq(t0, uv7);
    feSq(t1, t0);
    feSq(t1, t1);
    feMul(t1, uv7, t1);
    feMul(t0, t0, t1);
    feSq(t0, t0);
    feMul(t0, t1, t0);
    feSq(t1, t0);
    for (i = 0; i < 4; ++i) {
        feSq(t1, t1);
    }
    feMul(t0, t1, t0);
    feSq(t1, t0);
    for (i = 0; i < 9; ++i) {
        feSq(t1, t1);
    }
    feMul(t1, t1, t0);
    feSq(t2, t1);
    for (i = 0; i < 19; ++i) {
        feSq(t2, t2);
    }
    feMul(t1, t2, t1);
    for (i = 0; i < 10; ++i) {
        feSq(t1, t1);
    }
    feMul(t0, t1, t0);
    feSq(t1, t0);
    for (i = 0; i < 49; ++i) {
        feSq(t1, t1);
    }
    feMul(t1, t1, t0);
    feSq(t2, t1);
    for (i = 0; i < 99; ++i) {
        feSq(t2, t2);
    }
    feMul(t1, t2, t1);
    for (i = 0; i < 50; ++i) {
        feSq(t1, t1);
    }
    feMul(t0, t1, t0);
    feSq(t0, t0);
    feSq(t0, t0);
    feMul(t0, t0, uv7);

    /* End fe_pow22523.c */
    /* t0 = (uv^7)^((q-5)/8) */
    feMul(t0, t0, v3);
    feMul(r, t0, u); /* u^(m+1)v^(-(m+1)) */
}

static void geCached0(GeCached *r)
{
    fe1(r->YPlusX);
    fe1(r->YMinusX);
    fe1(r->Z);
    fe0(r->T2D);
}

static void geCachedCmov(GeCached *t, const GeCached *u, unsigned char b)
{
    feCmov(t->YPlusX, u->YPlusX, b);
    feCmov(t->YMinusX, u->YMinusX, b);
    feCmov(t->Z, u->Z, b);
    feCmov(t->T2D, u->T2D, b);
}

/* Assumes that a[31] <= 127 */
void geScalarMult(GeP2 *r,
                  const unsigned char *a,
                  const GeP3 *A)
{
    signed char e[64];
    int carry, carry2, i;
    GeCached Ai[8]; /* 1 * A, 2 * A, ..., 8 * A */
    GeP1P1 t;
    GeP3 u;

    carry = 0; /* 0..1 */
    for (i = 0; i < 31; i++) {
        carry += a[i]; /* 0..256 */
        carry2 = (carry + 8) >> 4; /* 0..16 */
        e[2 * i] = carry - (carry2 << 4); /* -8..7 */
        carry = (carry2 + 8) >> 4; /* 0..1 */
        e[2 * i + 1] = carry2 - (carry << 4); /* -8..7 */
    }
    carry += a[31]; /* 0..128 */
    carry2 = (carry + 8) >> 4; /* 0..8 */
    e[62] = carry - (carry2 << 4); /* -8..7 */
    e[63] = carry2; /* 0..8 */

    geP3ToCached(&Ai[0], A);
    for (i = 0; i < 7; i++) {
        geAdd(&t, A, &Ai[i]);
        geP1P1ToP3(&u, &t);
        geP3ToCached(&Ai[i + 1], &u);
    }

    geP20(r);
    for (i = 63; i >= 0; i--) {
        signed char b = e[i];
        unsigned char bnegative = negative(b);
        unsigned char babs = b - (((-bnegative) & b) << 1);
        GeCached cur, minuscur;
        geP2Dbl(&t, r);
        geP1P1ToP2(r, &t);
        geP2Dbl(&t, r);
        geP1P1ToP2(r, &t);
        geP2Dbl(&t, r);
        geP1P1ToP2(r, &t);
        geP2Dbl(&t, r);
        geP1P1ToP3(&u, &t);
        geCached0(&cur);
        geCachedCmov(&cur, &Ai[0], equal(babs, 1));
        geCachedCmov(&cur, &Ai[1], equal(babs, 2));
        geCachedCmov(&cur, &Ai[2], equal(babs, 3));
        geCachedCmov(&cur, &Ai[3], equal(babs, 4));
        geCachedCmov(&cur, &Ai[4], equal(babs, 5));
        geCachedCmov(&cur, &Ai[5], equal(babs, 6));
        geCachedCmov(&cur, &Ai[6], equal(babs, 7));
        geCachedCmov(&cur, &Ai[7], equal(babs, 8));
        feCopy(minuscur.YPlusX, cur.YMinusX);
        feCopy(minuscur.YMinusX, cur.YPlusX);
        feCopy(minuscur.Z, cur.Z);
        feNeg(minuscur.T2D, cur.T2D);
        geCachedCmov(&cur, &minuscur, bnegative);
        geAdd(&t, &u, &cur);
        geP1P1ToP2(r, &t);
    }
}

void geDoubleScalarMultPrecompVarTime(GeP2 *r,
                                      const unsigned char *a,
                                      const GeP3 *A,
                                      const unsigned char *b,
                                      const geDsmp Bi)
{
    signed char aSlide[256];
    signed char bSlide[256];
    geDsmp Ai; /* A, 3A, 5A, 7A, 9A, 11A, 13A, 15A */
    GeP1P1 t;
    GeP3 u;
    int i;

    slide(aSlide, a);
    slide(bSlide, b);
    geDsmPrecomp(Ai, A);

    geP20(r);

    for (i = 255; i >= 0; --i) {
        if (aSlide[i] || bSlide[i])
            break;
    }

    for (; i >= 0; --i) {
        geP2Dbl(&t, r);

        if (aSlide[i] > 0) {
            geP1P1ToP3(&u, &t);
            geAdd(&t, &u, &Ai[aSlide[i] / 2]);
        } else if (aSlide[i] < 0) {
            geP1P1ToP3(&u, &t);
            geSub(&t, &u, &Ai[(-aSlide[i]) / 2]);
        }

        if (bSlide[i] > 0) {
            geP1P1ToP3(&u, &t);
            geAdd(&t, &u, &Bi[bSlide[i] / 2]);
        } else if (bSlide[i] < 0) {
            geP1P1ToP3(&u, &t);
            geSub(&t, &u, &Bi[(-bSlide[i]) / 2]);
        }

        geP1P1ToP2(r, &t);
    }
}

void geMul8(GeP1P1 *r, const GeP2 *t)
{
    GeP2 u;
    geP2Dbl(r, t);
    geP1P1ToP2(&u, r);
    geP2Dbl(r, &u);
    geP1P1ToP2(&u, r);
    geP2Dbl(r, &u);
}

void geFromFeFromBytesVarTime(GeP2 *r, const unsigned char *s)
{
    fe u, v, w, x, y, z;
    unsigned char sign;

    /* From fe_frombytes.c */

    int64_t h0 = load4(s);
    int64_t h1 = load3(s + 4) << 6;
    int64_t h2 = load3(s + 7) << 5;
    int64_t h3 = load3(s + 10) << 3;
    int64_t h4 = load3(s + 13) << 2;
    int64_t h5 = load4(s + 16);
    int64_t h6 = load3(s + 20) << 7;
    int64_t h7 = load3(s + 23) << 5;
    int64_t h8 = load3(s + 26) << 4;
    int64_t h9 = load3(s + 29) << 2;
    int64_t carry0;
    int64_t carry1;
    int64_t carry2;
    int64_t carry3;
    int64_t carry4;
    int64_t carry5;
    int64_t carry6;
    int64_t carry7;
    int64_t carry8;
    int64_t carry9;

    carry9 = (h9 + (int64_t)(1 << 24)) >> 25;
    h0 += carry9 * 19;
    h9 -= carry9 << 25;
    carry1 = (h1 + (int64_t)(1 << 24)) >> 25;
    h2 += carry1;
    h1 -= carry1 << 25;
    carry3 = (h3 + (int64_t)(1 << 24)) >> 25;
    h4 += carry3;
    h3 -= carry3 << 25;
    carry5 = (h5 + (int64_t)(1 << 24)) >> 25;
    h6 += carry5;
    h5 -= carry5 << 25;
    carry7 = (h7 + (int64_t)(1 << 24)) >> 25;
    h8 += carry7;
    h7 -= carry7 << 25;

    carry0 = (h0 + (int64_t)(1 << 25)) >> 26;
    h1 += carry0;
    h0 -= carry0 << 26;
    carry2 = (h2 + (int64_t)(1 << 25)) >> 26;
    h3 += carry2;
    h2 -= carry2 << 26;
    carry4 = (h4 + (int64_t)(1 << 25)) >> 26;
    h5 += carry4;
    h4 -= carry4 << 26;
    carry6 = (h6 + (int64_t)(1 << 25)) >> 26;
    h7 += carry6;
    h6 -= carry6 << 26;
    carry8 = (h8 + (int64_t)(1 << 25)) >> 26;
    h9 += carry8;
    h8 -= carry8 << 26;

    u[0] = (int32_t)h0;
    u[1] = (int32_t)h1;
    u[2] = (int32_t)h2;
    u[3] = (int32_t)h3;
    u[4] = (int32_t)h4;
    u[5] = (int32_t)h5;
    u[6] = (int32_t)h6;
    u[7] = (int32_t)h7;
    u[8] = (int32_t)h8;
    u[9] = (int32_t)h9;

    /* End fe_frombytes.c */

    feSq2(v, u); /* 2 * u^2 */
    fe1(w);
    feAdd(w, v, w); /* w = 2 * u^2 + 1 */
    feSq(x, w); /* w^2 */
    feMul(y, feMa2, v); /* -2 * A^2 * u^2 */
    feAdd(x, x, y); /* x = w^2 - 2 * A^2 * u^2 */
    feDivPowM1(r->X, w, x); /* (w / x)^(m + 1) */
    feSq(y, r->X);
    feMul(x, y, x);
    feSub(y, w, x);
    feCopy(z, feMa);
    if (feIsNonZero(y)) {
        feAdd(y, w, x);
        if (feIsNonZero(y)) {
            goto negative;
        } else {
            feMul(r->X, r->X, feFffb1);
        }
    } else {
        feMul(r->X, r->X, feFffb2);
    }
    feMul(r->X, r->X, u); /* u * sqrt(2 * A * (A + 2) * w / x) */
    feMul(z, z, v); /* -2 * A * u^2 */
    sign = 0;
    goto setSign;
negative:
    feMul(x, x, feSqrtm1);
    feSub(y, w, x);
    if (feIsNonZero(y)) {
        assert((feAdd(y, w, x), !feIsNonZero(y)));
        feMul(r->X, r->X, feFffb3);
    } else {
        feMul(r->X, r->X, feFffb4);
    }
    /* r->X = sqrt(A * (A + 2) * w / x) */
    /* z = -A */
    sign = 1;
setSign:
    if (feIsNegative(r->X) != sign) {
        assert(feIsNonZero(r->X));
        feNeg(r->X, r->X);
    }
    feAdd(r->Z, z, w);
    feSub(r->Y, z, w);
    feMul(r->X, r->X, r->Z);
#if !defined(NDEBUG)
    {
        fe check_x, check_y, check_iz, check_v;
        feInvert(check_iz, r->Z);
        feMul(check_x, r->X, check_iz);
        feMul(check_y, r->Y, check_iz);
        feSq(check_x, check_x);
        feSq(check_y, check_y);
        feMul(check_v, check_x, check_y);
        feMul(check_v, feD, check_v);
        feAdd(check_v, check_v, check_x);
        feSub(check_v, check_v, check_y);
        fe1(check_x);
        feAdd(check_v, check_v, check_x);
        assert(!feIsNonZero(check_v));
    }
#endif
}

void sc0(unsigned char *s)
{
    int i;

    for (i = 0; i < 32; i++) {
        s[i] = 0;
    }
}

void scReduce32(unsigned char *s)
{
    int64_t s0 = 2097151 & load3(s);
    int64_t s1 = 2097151 & (load4(s + 2) >> 5);
    int64_t s2 = 2097151 & (load3(s + 5) >> 2);
    int64_t s3 = 2097151 & (load4(s + 7) >> 7);
    int64_t s4 = 2097151 & (load4(s + 10) >> 4);
    int64_t s5 = 2097151 & (load3(s + 13) >> 1);
    int64_t s6 = 2097151 & (load4(s + 15) >> 6);
    int64_t s7 = 2097151 & (load3(s + 18) >> 3);
    int64_t s8 = 2097151 & load3(s + 21);
    int64_t s9 = 2097151 & (load4(s + 23) >> 5);
    int64_t s10 = 2097151 & (load3(s + 26) >> 2);
    int64_t s11 = (load4(s + 28) >> 7);
    int64_t s12 = 0;
    int64_t carry0;
    int64_t carry1;
    int64_t carry2;
    int64_t carry3;
    int64_t carry4;
    int64_t carry5;
    int64_t carry6;
    int64_t carry7;
    int64_t carry8;
    int64_t carry9;
    int64_t carry10;
    int64_t carry11;

    carry0 = (s0 + (1 << 20)) >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry2 = (s2 + (1 << 20)) >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry4 = (s4 + (1 << 20)) >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry6 = (s6 + (1 << 20)) >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry8 = (s8 + (1 << 20)) >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry10 = (s10 + (1 << 20)) >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;

    carry1 = (s1 + (1 << 20)) >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry3 = (s3 + (1 << 20)) >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry5 = (s5 + (1 << 20)) >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry7 = (s7 + (1 << 20)) >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry9 = (s9 + (1 << 20)) >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry11 = (s11 + (1 << 20)) >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;

    s0 += s12 * 666643;
    s1 += s12 * 470296;
    s2 += s12 * 654183;
    s3 -= s12 * 997805;
    s4 += s12 * 136657;
    s5 -= s12 * 683901;
    s12 = 0;

    carry0 = s0 >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry1 = s1 >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry2 = s2 >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry3 = s3 >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry4 = s4 >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry5 = s5 >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry6 = s6 >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry7 = s7 >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry8 = s8 >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry9 = s9 >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry10 = s10 >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;
    carry11 = s11 >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;

    s0 += s12 * 666643;
    s1 += s12 * 470296;
    s2 += s12 * 654183;
    s3 -= s12 * 997805;
    s4 += s12 * 136657;
    s5 -= s12 * 683901;

    carry0 = s0 >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry1 = s1 >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry2 = s2 >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry3 = s3 >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry4 = s4 >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry5 = s5 >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry6 = s6 >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry7 = s7 >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry8 = s8 >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry9 = s9 >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry10 = s10 >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;

    s[0] = (unsigned char)(s0 >> 0);
    s[1] = (unsigned char)(s0 >> 8);
    s[2] = (unsigned char)((s0 >> 16) | (s1 << 5));
    s[3] = (unsigned char)(s1 >> 3);
    s[4] = (unsigned char)(s1 >> 11);
    s[5] = (unsigned char)((s1 >> 19) | (s2 << 2));
    s[6] = (unsigned char)(s2 >> 6);
    s[7] = (unsigned char)((s2 >> 14) | (s3 << 7));
    s[8] = (unsigned char)(s3 >> 1);
    s[9] = (unsigned char)(s3 >> 9);
    s[10] = (unsigned char)((s3 >> 17) | (s4 << 4));
    s[11] = (unsigned char)(s4 >> 4);
    s[12] = (unsigned char)(s4 >> 12);
    s[13] = (unsigned char)((s4 >> 20) | (s5 << 1));
    s[14] = (unsigned char)(s5 >> 7);
    s[15] = (unsigned char)((s5 >> 15) | (s6 << 6));
    s[16] = (unsigned char)(s6 >> 2);
    s[17] = (unsigned char)(s6 >> 10);
    s[18] = (unsigned char)((s6 >> 18) | (s7 << 3));
    s[19] = (unsigned char)(s7 >> 5);
    s[20] = (unsigned char)(s7 >> 13);
    s[21] = (unsigned char)(s8 >> 0);
    s[22] = (unsigned char)(s8 >> 8);
    s[23] = (unsigned char)((s8 >> 16) | (s9 << 5));
    s[24] = (unsigned char)(s9 >> 3);
    s[25] = (unsigned char)(s9 >> 11);
    s[26] = (unsigned char)((s9 >> 19) | (s10 << 2));
    s[27] = (unsigned char)(s10 >> 6);
    s[28] = (unsigned char)((s10 >> 14) | (s11 << 7));
    s[29] = (unsigned char)(s11 >> 1);
    s[30] = (unsigned char)(s11 >> 9);
    s[31] = (unsigned char)(s11 >> 17);
}

void scAdd(unsigned char *s, const unsigned char *a, const unsigned char *b)
{
    int64_t a0 = 2097151 & load3(a);
    int64_t a1 = 2097151 & (load4(a + 2) >> 5);
    int64_t a2 = 2097151 & (load3(a + 5) >> 2);
    int64_t a3 = 2097151 & (load4(a + 7) >> 7);
    int64_t a4 = 2097151 & (load4(a + 10) >> 4);
    int64_t a5 = 2097151 & (load3(a + 13) >> 1);
    int64_t a6 = 2097151 & (load4(a + 15) >> 6);
    int64_t a7 = 2097151 & (load3(a + 18) >> 3);
    int64_t a8 = 2097151 & load3(a + 21);
    int64_t a9 = 2097151 & (load4(a + 23) >> 5);
    int64_t a10 = 2097151 & (load3(a + 26) >> 2);
    int64_t a11 = (load4(a + 28) >> 7);
    int64_t b0 = 2097151 & load3(b);
    int64_t b1 = 2097151 & (load4(b + 2) >> 5);
    int64_t b2 = 2097151 & (load3(b + 5) >> 2);
    int64_t b3 = 2097151 & (load4(b + 7) >> 7);
    int64_t b4 = 2097151 & (load4(b + 10) >> 4);
    int64_t b5 = 2097151 & (load3(b + 13) >> 1);
    int64_t b6 = 2097151 & (load4(b + 15) >> 6);
    int64_t b7 = 2097151 & (load3(b + 18) >> 3);
    int64_t b8 = 2097151 & load3(b + 21);
    int64_t b9 = 2097151 & (load4(b + 23) >> 5);
    int64_t b10 = 2097151 & (load3(b + 26) >> 2);
    int64_t b11 = (load4(b + 28) >> 7);
    int64_t s0 = a0 + b0;
    int64_t s1 = a1 + b1;
    int64_t s2 = a2 + b2;
    int64_t s3 = a3 + b3;
    int64_t s4 = a4 + b4;
    int64_t s5 = a5 + b5;
    int64_t s6 = a6 + b6;
    int64_t s7 = a7 + b7;
    int64_t s8 = a8 + b8;
    int64_t s9 = a9 + b9;
    int64_t s10 = a10 + b10;
    int64_t s11 = a11 + b11;
    int64_t s12 = 0;
    int64_t carry0;
    int64_t carry1;
    int64_t carry2;
    int64_t carry3;
    int64_t carry4;
    int64_t carry5;
    int64_t carry6;
    int64_t carry7;
    int64_t carry8;
    int64_t carry9;
    int64_t carry10;
    int64_t carry11;

    carry0 = (s0 + (1 << 20)) >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry2 = (s2 + (1 << 20)) >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry4 = (s4 + (1 << 20)) >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry6 = (s6 + (1 << 20)) >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry8 = (s8 + (1 << 20)) >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry10 = (s10 + (1 << 20)) >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;

    carry1 = (s1 + (1 << 20)) >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry3 = (s3 + (1 << 20)) >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry5 = (s5 + (1 << 20)) >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry7 = (s7 + (1 << 20)) >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry9 = (s9 + (1 << 20)) >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry11 = (s11 + (1 << 20)) >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;

    s0 += s12 * 666643;
    s1 += s12 * 470296;
    s2 += s12 * 654183;
    s3 -= s12 * 997805;
    s4 += s12 * 136657;
    s5 -= s12 * 683901;
    s12 = 0;

    carry0 = s0 >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry1 = s1 >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry2 = s2 >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry3 = s3 >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry4 = s4 >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry5 = s5 >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry6 = s6 >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry7 = s7 >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry8 = s8 >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry9 = s9 >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry10 = s10 >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;
    carry11 = s11 >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;

    s0 += s12 * 666643;
    s1 += s12 * 470296;
    s2 += s12 * 654183;
    s3 -= s12 * 997805;
    s4 += s12 * 136657;
    s5 -= s12 * 683901;

    carry0 = s0 >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry1 = s1 >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry2 = s2 >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry3 = s3 >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry4 = s4 >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry5 = s5 >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry6 = s6 >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry7 = s7 >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry8 = s8 >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry9 = s9 >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry10 = s10 >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;

    s[0] = (unsigned char)(s0 >> 0);
    s[1] = (unsigned char)(s0 >> 8);
    s[2] = (unsigned char)((s0 >> 16) | (s1 << 5));
    s[3] = (unsigned char)(s1 >> 3);
    s[4] = (unsigned char)(s1 >> 11);
    s[5] = (unsigned char)((s1 >> 19) | (s2 << 2));
    s[6] = (unsigned char)(s2 >> 6);
    s[7] = (unsigned char)((s2 >> 14) | (s3 << 7));
    s[8] = (unsigned char)(s3 >> 1);
    s[9] = (unsigned char)(s3 >> 9);
    s[10] = (unsigned char)((s3 >> 17) | (s4 << 4));
    s[11] = (unsigned char)(s4 >> 4);
    s[12] = (unsigned char)(s4 >> 12);
    s[13] = (unsigned char)((s4 >> 20) | (s5 << 1));
    s[14] = (unsigned char)(s5 >> 7);
    s[15] = (unsigned char)((s5 >> 15) | (s6 << 6));
    s[16] = (unsigned char)(s6 >> 2);
    s[17] = (unsigned char)(s6 >> 10);
    s[18] = (unsigned char)((s6 >> 18) | (s7 << 3));
    s[19] = (unsigned char)(s7 >> 5);
    s[20] = (unsigned char)(s7 >> 13);
    s[21] = (unsigned char)(s8 >> 0);
    s[22] = (unsigned char)(s8 >> 8);
    s[23] = (unsigned char)((s8 >> 16) | (s9 << 5));
    s[24] = (unsigned char)(s9 >> 3);
    s[25] = (unsigned char)(s9 >> 11);
    s[26] = (unsigned char)((s9 >> 19) | (s10 << 2));
    s[27] = (unsigned char)(s10 >> 6);
    s[28] = (unsigned char)((s10 >> 14) | (s11 << 7));
    s[29] = (unsigned char)(s11 >> 1);
    s[30] = (unsigned char)(s11 >> 9);
    s[31] = (unsigned char)(s11 >> 17);
}

void scSub(unsigned char *s, const unsigned char *a, const unsigned char *b)
{
    int64_t a0 = 2097151 & load3(a);
    int64_t a1 = 2097151 & (load4(a + 2) >> 5);
    int64_t a2 = 2097151 & (load3(a + 5) >> 2);
    int64_t a3 = 2097151 & (load4(a + 7) >> 7);
    int64_t a4 = 2097151 & (load4(a + 10) >> 4);
    int64_t a5 = 2097151 & (load3(a + 13) >> 1);
    int64_t a6 = 2097151 & (load4(a + 15) >> 6);
    int64_t a7 = 2097151 & (load3(a + 18) >> 3);
    int64_t a8 = 2097151 & load3(a + 21);
    int64_t a9 = 2097151 & (load4(a + 23) >> 5);
    int64_t a10 = 2097151 & (load3(a + 26) >> 2);
    int64_t a11 = (load4(a + 28) >> 7);
    int64_t b0 = 2097151 & load3(b);
    int64_t b1 = 2097151 & (load4(b + 2) >> 5);
    int64_t b2 = 2097151 & (load3(b + 5) >> 2);
    int64_t b3 = 2097151 & (load4(b + 7) >> 7);
    int64_t b4 = 2097151 & (load4(b + 10) >> 4);
    int64_t b5 = 2097151 & (load3(b + 13) >> 1);
    int64_t b6 = 2097151 & (load4(b + 15) >> 6);
    int64_t b7 = 2097151 & (load3(b + 18) >> 3);
    int64_t b8 = 2097151 & load3(b + 21);
    int64_t b9 = 2097151 & (load4(b + 23) >> 5);
    int64_t b10 = 2097151 & (load3(b + 26) >> 2);
    int64_t b11 = (load4(b + 28) >> 7);
    int64_t s0 = a0 - b0;
    int64_t s1 = a1 - b1;
    int64_t s2 = a2 - b2;
    int64_t s3 = a3 - b3;
    int64_t s4 = a4 - b4;
    int64_t s5 = a5 - b5;
    int64_t s6 = a6 - b6;
    int64_t s7 = a7 - b7;
    int64_t s8 = a8 - b8;
    int64_t s9 = a9 - b9;
    int64_t s10 = a10 - b10;
    int64_t s11 = a11 - b11;
    int64_t s12 = 0;
    int64_t carry0;
    int64_t carry1;
    int64_t carry2;
    int64_t carry3;
    int64_t carry4;
    int64_t carry5;
    int64_t carry6;
    int64_t carry7;
    int64_t carry8;
    int64_t carry9;
    int64_t carry10;
    int64_t carry11;

    carry0 = (s0 + (1 << 20)) >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry2 = (s2 + (1 << 20)) >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry4 = (s4 + (1 << 20)) >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry6 = (s6 + (1 << 20)) >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry8 = (s8 + (1 << 20)) >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry10 = (s10 + (1 << 20)) >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;

    carry1 = (s1 + (1 << 20)) >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry3 = (s3 + (1 << 20)) >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry5 = (s5 + (1 << 20)) >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry7 = (s7 + (1 << 20)) >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry9 = (s9 + (1 << 20)) >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry11 = (s11 + (1 << 20)) >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;

    s0 += s12 * 666643;
    s1 += s12 * 470296;
    s2 += s12 * 654183;
    s3 -= s12 * 997805;
    s4 += s12 * 136657;
    s5 -= s12 * 683901;
    s12 = 0;

    carry0 = s0 >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry1 = s1 >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry2 = s2 >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry3 = s3 >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry4 = s4 >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry5 = s5 >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry6 = s6 >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry7 = s7 >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry8 = s8 >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry9 = s9 >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry10 = s10 >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;
    carry11 = s11 >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;

    s0 += s12 * 666643;
    s1 += s12 * 470296;
    s2 += s12 * 654183;
    s3 -= s12 * 997805;
    s4 += s12 * 136657;
    s5 -= s12 * 683901;

    carry0 = s0 >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry1 = s1 >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry2 = s2 >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry3 = s3 >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry4 = s4 >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry5 = s5 >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry6 = s6 >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry7 = s7 >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry8 = s8 >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry9 = s9 >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry10 = s10 >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;

    s[0] = (unsigned char)(s0 >> 0);
    s[1] = (unsigned char)(s0 >> 8);
    s[2] = (unsigned char)((s0 >> 16) | (s1 << 5));
    s[3] = (unsigned char)(s1 >> 3);
    s[4] = (unsigned char)(s1 >> 11);
    s[5] = (unsigned char)((s1 >> 19) | (s2 << 2));
    s[6] = (unsigned char)(s2 >> 6);
    s[7] = (unsigned char)((s2 >> 14) | (s3 << 7));
    s[8] = (unsigned char)(s3 >> 1);
    s[9] = (unsigned char)(s3 >> 9);
    s[10] = (unsigned char)((s3 >> 17) | (s4 << 4));
    s[11] = (unsigned char)(s4 >> 4);
    s[12] = (unsigned char)(s4 >> 12);
    s[13] = (unsigned char)((s4 >> 20) | (s5 << 1));
    s[14] = (unsigned char)(s5 >> 7);
    s[15] = (unsigned char)((s5 >> 15) | (s6 << 6));
    s[16] = (unsigned char)(s6 >> 2);
    s[17] = (unsigned char)(s6 >> 10);
    s[18] = (unsigned char)((s6 >> 18) | (s7 << 3));
    s[19] = (unsigned char)(s7 >> 5);
    s[20] = (unsigned char)(s7 >> 13);
    s[21] = (unsigned char)(s8 >> 0);
    s[22] = (unsigned char)(s8 >> 8);
    s[23] = (unsigned char)((s8 >> 16) | (s9 << 5));
    s[24] = (unsigned char)(s9 >> 3);
    s[25] = (unsigned char)(s9 >> 11);
    s[26] = (unsigned char)((s9 >> 19) | (s10 << 2));
    s[27] = (unsigned char)(s10 >> 6);
    s[28] = (unsigned char)((s10 >> 14) | (s11 << 7));
    s[29] = (unsigned char)(s11 >> 1);
    s[30] = (unsigned char)(s11 >> 9);
    s[31] = (unsigned char)(s11 >> 17);
}

/*
Input:
  a[0]+256*a[1]+...+256^31*a[31] = a
  b[0]+256*b[1]+...+256^31*b[31] = b
  c[0]+256*c[1]+...+256^31*c[31] = c

Output:
  s[0]+256*s[1]+...+256^31*s[31] = (c-Ab) mod l
  where l = 2^252 + 27742317777372353535851937790883648493.
*/

void scMulSub(unsigned char *s,
              const unsigned char *a,
              const unsigned char *b,
              const unsigned char *c)
{
    int64_t a0 = 2097151 & load3(a);
    int64_t a1 = 2097151 & (load4(a + 2) >> 5);
    int64_t a2 = 2097151 & (load3(a + 5) >> 2);
    int64_t a3 = 2097151 & (load4(a + 7) >> 7);
    int64_t a4 = 2097151 & (load4(a + 10) >> 4);
    int64_t a5 = 2097151 & (load3(a + 13) >> 1);
    int64_t a6 = 2097151 & (load4(a + 15) >> 6);
    int64_t a7 = 2097151 & (load3(a + 18) >> 3);
    int64_t a8 = 2097151 & load3(a + 21);
    int64_t a9 = 2097151 & (load4(a + 23) >> 5);
    int64_t a10 = 2097151 & (load3(a + 26) >> 2);
    int64_t a11 = (load4(a + 28) >> 7);
    int64_t b0 = 2097151 & load3(b);
    int64_t b1 = 2097151 & (load4(b + 2) >> 5);
    int64_t b2 = 2097151 & (load3(b + 5) >> 2);
    int64_t b3 = 2097151 & (load4(b + 7) >> 7);
    int64_t b4 = 2097151 & (load4(b + 10) >> 4);
    int64_t b5 = 2097151 & (load3(b + 13) >> 1);
    int64_t b6 = 2097151 & (load4(b + 15) >> 6);
    int64_t b7 = 2097151 & (load3(b + 18) >> 3);
    int64_t b8 = 2097151 & load3(b + 21);
    int64_t b9 = 2097151 & (load4(b + 23) >> 5);
    int64_t b10 = 2097151 & (load3(b + 26) >> 2);
    int64_t b11 = (load4(b + 28) >> 7);
    int64_t c0 = 2097151 & load3(c);
    int64_t c1 = 2097151 & (load4(c + 2) >> 5);
    int64_t c2 = 2097151 & (load3(c + 5) >> 2);
    int64_t c3 = 2097151 & (load4(c + 7) >> 7);
    int64_t c4 = 2097151 & (load4(c + 10) >> 4);
    int64_t c5 = 2097151 & (load3(c + 13) >> 1);
    int64_t c6 = 2097151 & (load4(c + 15) >> 6);
    int64_t c7 = 2097151 & (load3(c + 18) >> 3);
    int64_t c8 = 2097151 & load3(c + 21);
    int64_t c9 = 2097151 & (load4(c + 23) >> 5);
    int64_t c10 = 2097151 & (load3(c + 26) >> 2);
    int64_t c11 = (load4(c + 28) >> 7);
    int64_t s0;
    int64_t s1;
    int64_t s2;
    int64_t s3;
    int64_t s4;
    int64_t s5;
    int64_t s6;
    int64_t s7;
    int64_t s8;
    int64_t s9;
    int64_t s10;
    int64_t s11;
    int64_t s12;
    int64_t s13;
    int64_t s14;
    int64_t s15;
    int64_t s16;
    int64_t s17;
    int64_t s18;
    int64_t s19;
    int64_t s20;
    int64_t s21;
    int64_t s22;
    int64_t s23;
    int64_t carry0;
    int64_t carry1;
    int64_t carry2;
    int64_t carry3;
    int64_t carry4;
    int64_t carry5;
    int64_t carry6;
    int64_t carry7;
    int64_t carry8;
    int64_t carry9;
    int64_t carry10;
    int64_t carry11;
    int64_t carry12;
    int64_t carry13;
    int64_t carry14;
    int64_t carry15;
    int64_t carry16;
    int64_t carry17;
    int64_t carry18;
    int64_t carry19;
    int64_t carry20;
    int64_t carry21;
    int64_t carry22;

    s0 = c0 - a0 * b0;
    s1 = c1 - (a0 * b1 + a1 * b0);
    s2 = c2 - (a0 * b2 + a1 * b1 + a2 * b0);
    s3 = c3 - (a0 * b3 + a1 * b2 + a2 * b1 + a3 * b0);
    s4 = c4 - (a0 * b4 + a1 * b3 + a2 * b2 + a3 * b1 + a4 * b0);
    s5 = c5 - (a0 * b5 + a1 * b4 + a2 * b3 + a3 * b2 + a4 * b1 + a5 * b0);
    s6 = c6 - (a0 * b6 + a1 * b5 + a2 * b4 + a3 * b3 + a4 * b2 + a5 * b1 + a6 * b0);
    s7 = c7 - (a0 * b7 + a1 * b6 + a2 * b5 + a3 * b4 + a4 * b3 + a5 * b2 + a6 * b1 + a7 * b0);
    s8 = c8
            - (a0 * b8 + a1 * b7 + a2 * b6 + a3 * b5 + a4 * b4 + a5 * b3 + a6 * b2 + a7 * b1
               + a8 * b0);
    s9 = c9
            - (a0 * b9 + a1 * b8 + a2 * b7 + a3 * b6 + a4 * b5 + a5 * b4 + a6 * b3 + a7 * b2
               + a8 * b1 + a9 * b0);
    s10 = c10
            - (a0 * b10 + a1 * b9 + a2 * b8 + a3 * b7 + a4 * b6 + a5 * b5 + a6 * b4 + a7 * b3
               + a8 * b2 + a9 * b1 + a10 * b0);
    s11 = c11
            - (a0 * b11 + a1 * b10 + a2 * b9 + a3 * b8 + a4 * b7 + a5 * b6 + a6 * b5 + a7 * b4
               + a8 * b3 + a9 * b2 + a10 * b1 + a11 * b0);
    s12 = -(a1 * b11 + a2 * b10 + a3 * b9 + a4 * b8 + a5 * b7 + a6 * b6 + a7 * b5 + a8 * b4
            + a9 * b3 + a10 * b2 + a11 * b1);
    s13 = -(a2 * b11 + a3 * b10 + a4 * b9 + a5 * b8 + a6 * b7 + a7 * b6 + a8 * b5 + a9 * b4
            + a10 * b3 + a11 * b2);
    s14 = -(a3 * b11 + a4 * b10 + a5 * b9 + a6 * b8 + a7 * b7 + a8 * b6 + a9 * b5 + a10 * b4
            + a11 * b3);
    s15 = -(a4 * b11 + a5 * b10 + a6 * b9 + a7 * b8 + a8 * b7 + a9 * b6 + a10 * b5 + a11 * b4);
    s16 = -(a5 * b11 + a6 * b10 + a7 * b9 + a8 * b8 + a9 * b7 + a10 * b6 + a11 * b5);
    s17 = -(a6 * b11 + a7 * b10 + a8 * b9 + a9 * b8 + a10 * b7 + a11 * b6);
    s18 = -(a7 * b11 + a8 * b10 + a9 * b9 + a10 * b8 + a11 * b7);
    s19 = -(a8 * b11 + a9 * b10 + a10 * b9 + a11 * b8);
    s20 = -(a9 * b11 + a10 * b10 + a11 * b9);
    s21 = -(a10 * b11 + a11 * b10);
    s22 = -a11 * b11;
    s23 = 0;

    carry0 = (s0 + (1 << 20)) >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry2 = (s2 + (1 << 20)) >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry4 = (s4 + (1 << 20)) >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry6 = (s6 + (1 << 20)) >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry8 = (s8 + (1 << 20)) >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry10 = (s10 + (1 << 20)) >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;
    carry12 = (s12 + (1 << 20)) >> 21;
    s13 += carry12;
    s12 -= carry12 << 21;
    carry14 = (s14 + (1 << 20)) >> 21;
    s15 += carry14;
    s14 -= carry14 << 21;
    carry16 = (s16 + (1 << 20)) >> 21;
    s17 += carry16;
    s16 -= carry16 << 21;
    carry18 = (s18 + (1 << 20)) >> 21;
    s19 += carry18;
    s18 -= carry18 << 21;
    carry20 = (s20 + (1 << 20)) >> 21;
    s21 += carry20;
    s20 -= carry20 << 21;
    carry22 = (s22 + (1 << 20)) >> 21;
    s23 += carry22;
    s22 -= carry22 << 21;

    carry1 = (s1 + (1 << 20)) >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry3 = (s3 + (1 << 20)) >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry5 = (s5 + (1 << 20)) >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry7 = (s7 + (1 << 20)) >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry9 = (s9 + (1 << 20)) >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry11 = (s11 + (1 << 20)) >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;
    carry13 = (s13 + (1 << 20)) >> 21;
    s14 += carry13;
    s13 -= carry13 << 21;
    carry15 = (s15 + (1 << 20)) >> 21;
    s16 += carry15;
    s15 -= carry15 << 21;
    carry17 = (s17 + (1 << 20)) >> 21;
    s18 += carry17;
    s17 -= carry17 << 21;
    carry19 = (s19 + (1 << 20)) >> 21;
    s20 += carry19;
    s19 -= carry19 << 21;
    carry21 = (s21 + (1 << 20)) >> 21;
    s22 += carry21;
    s21 -= carry21 << 21;

    s11 += s23 * 666643;
    s12 += s23 * 470296;
    s13 += s23 * 654183;
    s14 -= s23 * 997805;
    s15 += s23 * 136657;
    s16 -= s23 * 683901;

    s10 += s22 * 666643;
    s11 += s22 * 470296;
    s12 += s22 * 654183;
    s13 -= s22 * 997805;
    s14 += s22 * 136657;
    s15 -= s22 * 683901;

    s9 += s21 * 666643;
    s10 += s21 * 470296;
    s11 += s21 * 654183;
    s12 -= s21 * 997805;
    s13 += s21 * 136657;
    s14 -= s21 * 683901;

    s8 += s20 * 666643;
    s9 += s20 * 470296;
    s10 += s20 * 654183;
    s11 -= s20 * 997805;
    s12 += s20 * 136657;
    s13 -= s20 * 683901;

    s7 += s19 * 666643;
    s8 += s19 * 470296;
    s9 += s19 * 654183;
    s10 -= s19 * 997805;
    s11 += s19 * 136657;
    s12 -= s19 * 683901;

    s6 += s18 * 666643;
    s7 += s18 * 470296;
    s8 += s18 * 654183;
    s9 -= s18 * 997805;
    s10 += s18 * 136657;
    s11 -= s18 * 683901;

    carry6 = (s6 + (1 << 20)) >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry8 = (s8 + (1 << 20)) >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry10 = (s10 + (1 << 20)) >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;
    carry12 = (s12 + (1 << 20)) >> 21;
    s13 += carry12;
    s12 -= carry12 << 21;
    carry14 = (s14 + (1 << 20)) >> 21;
    s15 += carry14;
    s14 -= carry14 << 21;
    carry16 = (s16 + (1 << 20)) >> 21;
    s17 += carry16;
    s16 -= carry16 << 21;

    carry7 = (s7 + (1 << 20)) >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry9 = (s9 + (1 << 20)) >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry11 = (s11 + (1 << 20)) >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;
    carry13 = (s13 + (1 << 20)) >> 21;
    s14 += carry13;
    s13 -= carry13 << 21;
    carry15 = (s15 + (1 << 20)) >> 21;
    s16 += carry15;
    s15 -= carry15 << 21;

    s5 += s17 * 666643;
    s6 += s17 * 470296;
    s7 += s17 * 654183;
    s8 -= s17 * 997805;
    s9 += s17 * 136657;
    s10 -= s17 * 683901;

    s4 += s16 * 666643;
    s5 += s16 * 470296;
    s6 += s16 * 654183;
    s7 -= s16 * 997805;
    s8 += s16 * 136657;
    s9 -= s16 * 683901;

    s3 += s15 * 666643;
    s4 += s15 * 470296;
    s5 += s15 * 654183;
    s6 -= s15 * 997805;
    s7 += s15 * 136657;
    s8 -= s15 * 683901;

    s2 += s14 * 666643;
    s3 += s14 * 470296;
    s4 += s14 * 654183;
    s5 -= s14 * 997805;
    s6 += s14 * 136657;
    s7 -= s14 * 683901;

    s1 += s13 * 666643;
    s2 += s13 * 470296;
    s3 += s13 * 654183;
    s4 -= s13 * 997805;
    s5 += s13 * 136657;
    s6 -= s13 * 683901;

    s0 += s12 * 666643;
    s1 += s12 * 470296;
    s2 += s12 * 654183;
    s3 -= s12 * 997805;
    s4 += s12 * 136657;
    s5 -= s12 * 683901;
    s12 = 0;

    carry0 = (s0 + (1 << 20)) >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry2 = (s2 + (1 << 20)) >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry4 = (s4 + (1 << 20)) >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry6 = (s6 + (1 << 20)) >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry8 = (s8 + (1 << 20)) >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry10 = (s10 + (1 << 20)) >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;

    carry1 = (s1 + (1 << 20)) >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry3 = (s3 + (1 << 20)) >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry5 = (s5 + (1 << 20)) >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry7 = (s7 + (1 << 20)) >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry9 = (s9 + (1 << 20)) >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry11 = (s11 + (1 << 20)) >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;

    s0 += s12 * 666643;
    s1 += s12 * 470296;
    s2 += s12 * 654183;
    s3 -= s12 * 997805;
    s4 += s12 * 136657;
    s5 -= s12 * 683901;
    s12 = 0;

    carry0 = s0 >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry1 = s1 >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry2 = s2 >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry3 = s3 >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry4 = s4 >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry5 = s5 >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry6 = s6 >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry7 = s7 >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry8 = s8 >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry9 = s9 >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry10 = s10 >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;
    carry11 = s11 >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;

    s0 += s12 * 666643;
    s1 += s12 * 470296;
    s2 += s12 * 654183;
    s3 -= s12 * 997805;
    s4 += s12 * 136657;
    s5 -= s12 * 683901;

    carry0 = s0 >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry1 = s1 >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry2 = s2 >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry3 = s3 >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry4 = s4 >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry5 = s5 >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry6 = s6 >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry7 = s7 >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry8 = s8 >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry9 = s9 >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry10 = s10 >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;

    s[0] = (unsigned char)(s0 >> 0);
    s[1] = (unsigned char)(s0 >> 8);
    s[2] = (unsigned char)((s0 >> 16) | (s1 << 5));
    s[3] = (unsigned char)(s1 >> 3);
    s[4] = (unsigned char)(s1 >> 11);
    s[5] = (unsigned char)((s1 >> 19) | (s2 << 2));
    s[6] = (unsigned char)(s2 >> 6);
    s[7] = (unsigned char)((s2 >> 14) | (s3 << 7));
    s[8] = (unsigned char)(s3 >> 1);
    s[9] = (unsigned char)(s3 >> 9);
    s[10] = (unsigned char)((s3 >> 17) | (s4 << 4));
    s[11] = (unsigned char)(s4 >> 4);
    s[12] = (unsigned char)(s4 >> 12);
    s[13] = (unsigned char)((s4 >> 20) | (s5 << 1));
    s[14] = (unsigned char)(s5 >> 7);
    s[15] = (unsigned char)((s5 >> 15) | (s6 << 6));
    s[16] = (unsigned char)(s6 >> 2);
    s[17] = (unsigned char)(s6 >> 10);
    s[18] = (unsigned char)((s6 >> 18) | (s7 << 3));
    s[19] = (unsigned char)(s7 >> 5);
    s[20] = (unsigned char)(s7 >> 13);
    s[21] = (unsigned char)(s8 >> 0);
    s[22] = (unsigned char)(s8 >> 8);
    s[23] = (unsigned char)((s8 >> 16) | (s9 << 5));
    s[24] = (unsigned char)(s9 >> 3);
    s[25] = (unsigned char)(s9 >> 11);
    s[26] = (unsigned char)((s9 >> 19) | (s10 << 2));
    s[27] = (unsigned char)(s10 >> 6);
    s[28] = (unsigned char)((s10 >> 14) | (s11 << 7));
    s[29] = (unsigned char)(s11 >> 1);
    s[30] = (unsigned char)(s11 >> 9);
    s[31] = (unsigned char)(s11 >> 17);
}

void scMul(unsigned char *s,
           const unsigned char *a,
           const unsigned char *b)
{
    int64_t a0 = 2097151 & load3(a);
    int64_t a1 = 2097151 & (load4(a + 2) >> 5);
    int64_t a2 = 2097151 & (load3(a + 5) >> 2);
    int64_t a3 = 2097151 & (load4(a + 7) >> 7);
    int64_t a4 = 2097151 & (load4(a + 10) >> 4);
    int64_t a5 = 2097151 & (load3(a + 13) >> 1);
    int64_t a6 = 2097151 & (load4(a + 15) >> 6);
    int64_t a7 = 2097151 & (load3(a + 18) >> 3);
    int64_t a8 = 2097151 & load3(a + 21);
    int64_t a9 = 2097151 & (load4(a + 23) >> 5);
    int64_t a10 = 2097151 & (load3(a + 26) >> 2);
    int64_t a11 = (load4(a + 28) >> 7);
    int64_t b0 = 2097151 & load3(b);
    int64_t b1 = 2097151 & (load4(b + 2) >> 5);
    int64_t b2 = 2097151 & (load3(b + 5) >> 2);
    int64_t b3 = 2097151 & (load4(b + 7) >> 7);
    int64_t b4 = 2097151 & (load4(b + 10) >> 4);
    int64_t b5 = 2097151 & (load3(b + 13) >> 1);
    int64_t b6 = 2097151 & (load4(b + 15) >> 6);
    int64_t b7 = 2097151 & (load3(b + 18) >> 3);
    int64_t b8 = 2097151 & load3(b + 21);
    int64_t b9 = 2097151 & (load4(b + 23) >> 5);
    int64_t b10 = 2097151 & (load3(b + 26) >> 2);
    int64_t b11 = (load4(b + 28) >> 7);
    int64_t s0;
    int64_t s1;
    int64_t s2;
    int64_t s3;
    int64_t s4;
    int64_t s5;
    int64_t s6;
    int64_t s7;
    int64_t s8;
    int64_t s9;
    int64_t s10;
    int64_t s11;
    int64_t s12;
    int64_t s13;
    int64_t s14;
    int64_t s15;
    int64_t s16;
    int64_t s17;
    int64_t s18;
    int64_t s19;
    int64_t s20;
    int64_t s21;
    int64_t s22;
    int64_t s23;
    int64_t carry0;
    int64_t carry1;
    int64_t carry2;
    int64_t carry3;
    int64_t carry4;
    int64_t carry5;
    int64_t carry6;
    int64_t carry7;
    int64_t carry8;
    int64_t carry9;
    int64_t carry10;
    int64_t carry11;
    int64_t carry12;
    int64_t carry13;
    int64_t carry14;
    int64_t carry15;
    int64_t carry16;
    int64_t carry17;
    int64_t carry18;
    int64_t carry19;
    int64_t carry20;
    int64_t carry21;
    int64_t carry22;

    s0 = a0 * b0;
    s1 = (a0 * b1 + a1 * b0);
    s2 = (a0 * b2 + a1 * b1 + a2 * b0);
    s3 = (a0 * b3 + a1 * b2 + a2 * b1 + a3 * b0);
    s4 = (a0 * b4 + a1 * b3 + a2 * b2 + a3 * b1 + a4 * b0);
    s5 = (a0 * b5 + a1 * b4 + a2 * b3 + a3 * b2 + a4 * b1 + a5 * b0);
    s6 = (a0 * b6 + a1 * b5 + a2 * b4 + a3 * b3 + a4 * b2 + a5 * b1 + a6 * b0);
    s7 = (a0 * b7 + a1 * b6 + a2 * b5 + a3 * b4 + a4 * b3 + a5 * b2 + a6 * b1 + a7 * b0);
    s8 = (a0 * b8 + a1 * b7 + a2 * b6 + a3 * b5 + a4 * b4 + a5 * b3 + a6 * b2 + a7 * b1 + a8 * b0);
    s9 = (a0 * b9 + a1 * b8 + a2 * b7 + a3 * b6 + a4 * b5 + a5 * b4 + a6 * b3 + a7 * b2 + a8 * b1
          + a9 * b0);
    s10 = (a0 * b10 + a1 * b9 + a2 * b8 + a3 * b7 + a4 * b6 + a5 * b5 + a6 * b4 + a7 * b3 + a8 * b2
           + a9 * b1 + a10 * b0);
    s11 = (a0 * b11 + a1 * b10 + a2 * b9 + a3 * b8 + a4 * b7 + a5 * b6 + a6 * b5 + a7 * b4 + a8 * b3
           + a9 * b2 + a10 * b1 + a11 * b0);
    s12 = (a1 * b11 + a2 * b10 + a3 * b9 + a4 * b8 + a5 * b7 + a6 * b6 + a7 * b5 + a8 * b4 + a9 * b3
           + a10 * b2 + a11 * b1);
    s13 = (a2 * b11 + a3 * b10 + a4 * b9 + a5 * b8 + a6 * b7 + a7 * b6 + a8 * b5 + a9 * b4
           + a10 * b3 + a11 * b2);
    s14 = (a3 * b11 + a4 * b10 + a5 * b9 + a6 * b8 + a7 * b7 + a8 * b6 + a9 * b5 + a10 * b4
           + a11 * b3);
    s15 = (a4 * b11 + a5 * b10 + a6 * b9 + a7 * b8 + a8 * b7 + a9 * b6 + a10 * b5 + a11 * b4);
    s16 = (a5 * b11 + a6 * b10 + a7 * b9 + a8 * b8 + a9 * b7 + a10 * b6 + a11 * b5);
    s17 = (a6 * b11 + a7 * b10 + a8 * b9 + a9 * b8 + a10 * b7 + a11 * b6);
    s18 = (a7 * b11 + a8 * b10 + a9 * b9 + a10 * b8 + a11 * b7);
    s19 = (a8 * b11 + a9 * b10 + a10 * b9 + a11 * b8);
    s20 = (a9 * b11 + a10 * b10 + a11 * b9);
    s21 = (a10 * b11 + a11 * b10);
    s22 = a11 * b11;
    s23 = 0;

    carry0 = (s0 + (1 << 20)) >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry2 = (s2 + (1 << 20)) >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry4 = (s4 + (1 << 20)) >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry6 = (s6 + (1 << 20)) >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry8 = (s8 + (1 << 20)) >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry10 = (s10 + (1 << 20)) >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;
    carry12 = (s12 + (1 << 20)) >> 21;
    s13 += carry12;
    s12 -= carry12 << 21;
    carry14 = (s14 + (1 << 20)) >> 21;
    s15 += carry14;
    s14 -= carry14 << 21;
    carry16 = (s16 + (1 << 20)) >> 21;
    s17 += carry16;
    s16 -= carry16 << 21;
    carry18 = (s18 + (1 << 20)) >> 21;
    s19 += carry18;
    s18 -= carry18 << 21;
    carry20 = (s20 + (1 << 20)) >> 21;
    s21 += carry20;
    s20 -= carry20 << 21;
    carry22 = (s22 + (1 << 20)) >> 21;
    s23 += carry22;
    s22 -= carry22 << 21;

    carry1 = (s1 + (1 << 20)) >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry3 = (s3 + (1 << 20)) >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry5 = (s5 + (1 << 20)) >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry7 = (s7 + (1 << 20)) >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry9 = (s9 + (1 << 20)) >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry11 = (s11 + (1 << 20)) >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;
    carry13 = (s13 + (1 << 20)) >> 21;
    s14 += carry13;
    s13 -= carry13 << 21;
    carry15 = (s15 + (1 << 20)) >> 21;
    s16 += carry15;
    s15 -= carry15 << 21;
    carry17 = (s17 + (1 << 20)) >> 21;
    s18 += carry17;
    s17 -= carry17 << 21;
    carry19 = (s19 + (1 << 20)) >> 21;
    s20 += carry19;
    s19 -= carry19 << 21;
    carry21 = (s21 + (1 << 20)) >> 21;
    s22 += carry21;
    s21 -= carry21 << 21;

    s11 += s23 * 666643;
    s12 += s23 * 470296;
    s13 += s23 * 654183;
    s14 -= s23 * 997805;
    s15 += s23 * 136657;
    s16 -= s23 * 683901;

    s10 += s22 * 666643;
    s11 += s22 * 470296;
    s12 += s22 * 654183;
    s13 -= s22 * 997805;
    s14 += s22 * 136657;
    s15 -= s22 * 683901;

    s9 += s21 * 666643;
    s10 += s21 * 470296;
    s11 += s21 * 654183;
    s12 -= s21 * 997805;
    s13 += s21 * 136657;
    s14 -= s21 * 683901;

    s8 += s20 * 666643;
    s9 += s20 * 470296;
    s10 += s20 * 654183;
    s11 -= s20 * 997805;
    s12 += s20 * 136657;
    s13 -= s20 * 683901;

    s7 += s19 * 666643;
    s8 += s19 * 470296;
    s9 += s19 * 654183;
    s10 -= s19 * 997805;
    s11 += s19 * 136657;
    s12 -= s19 * 683901;

    s6 += s18 * 666643;
    s7 += s18 * 470296;
    s8 += s18 * 654183;
    s9 -= s18 * 997805;
    s10 += s18 * 136657;
    s11 -= s18 * 683901;

    carry6 = (s6 + (1 << 20)) >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry8 = (s8 + (1 << 20)) >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry10 = (s10 + (1 << 20)) >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;
    carry12 = (s12 + (1 << 20)) >> 21;
    s13 += carry12;
    s12 -= carry12 << 21;
    carry14 = (s14 + (1 << 20)) >> 21;
    s15 += carry14;
    s14 -= carry14 << 21;
    carry16 = (s16 + (1 << 20)) >> 21;
    s17 += carry16;
    s16 -= carry16 << 21;

    carry7 = (s7 + (1 << 20)) >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry9 = (s9 + (1 << 20)) >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry11 = (s11 + (1 << 20)) >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;
    carry13 = (s13 + (1 << 20)) >> 21;
    s14 += carry13;
    s13 -= carry13 << 21;
    carry15 = (s15 + (1 << 20)) >> 21;
    s16 += carry15;
    s15 -= carry15 << 21;

    s5 += s17 * 666643;
    s6 += s17 * 470296;
    s7 += s17 * 654183;
    s8 -= s17 * 997805;
    s9 += s17 * 136657;
    s10 -= s17 * 683901;

    s4 += s16 * 666643;
    s5 += s16 * 470296;
    s6 += s16 * 654183;
    s7 -= s16 * 997805;
    s8 += s16 * 136657;
    s9 -= s16 * 683901;

    s3 += s15 * 666643;
    s4 += s15 * 470296;
    s5 += s15 * 654183;
    s6 -= s15 * 997805;
    s7 += s15 * 136657;
    s8 -= s15 * 683901;

    s2 += s14 * 666643;
    s3 += s14 * 470296;
    s4 += s14 * 654183;
    s5 -= s14 * 997805;
    s6 += s14 * 136657;
    s7 -= s14 * 683901;

    s1 += s13 * 666643;
    s2 += s13 * 470296;
    s3 += s13 * 654183;
    s4 -= s13 * 997805;
    s5 += s13 * 136657;
    s6 -= s13 * 683901;

    s0 += s12 * 666643;
    s1 += s12 * 470296;
    s2 += s12 * 654183;
    s3 -= s12 * 997805;
    s4 += s12 * 136657;
    s5 -= s12 * 683901;
    s12 = 0;

    carry0 = (s0 + (1 << 20)) >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry2 = (s2 + (1 << 20)) >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry4 = (s4 + (1 << 20)) >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry6 = (s6 + (1 << 20)) >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry8 = (s8 + (1 << 20)) >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry10 = (s10 + (1 << 20)) >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;

    carry1 = (s1 + (1 << 20)) >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry3 = (s3 + (1 << 20)) >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry5 = (s5 + (1 << 20)) >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry7 = (s7 + (1 << 20)) >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry9 = (s9 + (1 << 20)) >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry11 = (s11 + (1 << 20)) >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;

    s0 += s12 * 666643;
    s1 += s12 * 470296;
    s2 += s12 * 654183;
    s3 -= s12 * 997805;
    s4 += s12 * 136657;
    s5 -= s12 * 683901;
    s12 = 0;

    carry0 = s0 >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry1 = s1 >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry2 = s2 >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry3 = s3 >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry4 = s4 >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry5 = s5 >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry6 = s6 >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry7 = s7 >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry8 = s8 >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry9 = s9 >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry10 = s10 >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;
    carry11 = s11 >> 21;
    s12 += carry11;
    s11 -= carry11 << 21;

    s0 += s12 * 666643;
    s1 += s12 * 470296;
    s2 += s12 * 654183;
    s3 -= s12 * 997805;
    s4 += s12 * 136657;
    s5 -= s12 * 683901;

    carry0 = s0 >> 21;
    s1 += carry0;
    s0 -= carry0 << 21;
    carry1 = s1 >> 21;
    s2 += carry1;
    s1 -= carry1 << 21;
    carry2 = s2 >> 21;
    s3 += carry2;
    s2 -= carry2 << 21;
    carry3 = s3 >> 21;
    s4 += carry3;
    s3 -= carry3 << 21;
    carry4 = s4 >> 21;
    s5 += carry4;
    s4 -= carry4 << 21;
    carry5 = s5 >> 21;
    s6 += carry5;
    s5 -= carry5 << 21;
    carry6 = s6 >> 21;
    s7 += carry6;
    s6 -= carry6 << 21;
    carry7 = s7 >> 21;
    s8 += carry7;
    s7 -= carry7 << 21;
    carry8 = s8 >> 21;
    s9 += carry8;
    s8 -= carry8 << 21;
    carry9 = s9 >> 21;
    s10 += carry9;
    s9 -= carry9 << 21;
    carry10 = s10 >> 21;
    s11 += carry10;
    s10 -= carry10 << 21;

    s[0] = (unsigned char)(s0 >> 0);
    s[1] = (unsigned char)(s0 >> 8);
    s[2] = (unsigned char)((s0 >> 16) | (s1 << 5));
    s[3] = (unsigned char)(s1 >> 3);
    s[4] = (unsigned char)(s1 >> 11);
    s[5] = (unsigned char)((s1 >> 19) | (s2 << 2));
    s[6] = (unsigned char)(s2 >> 6);
    s[7] = (unsigned char)((s2 >> 14) | (s3 << 7));
    s[8] = (unsigned char)(s3 >> 1);
    s[9] = (unsigned char)(s3 >> 9);
    s[10] = (unsigned char)((s3 >> 17) | (s4 << 4));
    s[11] = (unsigned char)(s4 >> 4);
    s[12] = (unsigned char)(s4 >> 12);
    s[13] = (unsigned char)((s4 >> 20) | (s5 << 1));
    s[14] = (unsigned char)(s5 >> 7);
    s[15] = (unsigned char)((s5 >> 15) | (s6 << 6));
    s[16] = (unsigned char)(s6 >> 2);
    s[17] = (unsigned char)(s6 >> 10);
    s[18] = (unsigned char)((s6 >> 18) | (s7 << 3));
    s[19] = (unsigned char)(s7 >> 5);
    s[20] = (unsigned char)(s7 >> 13);
    s[21] = (unsigned char)(s8 >> 0);
    s[22] = (unsigned char)(s8 >> 8);
    s[23] = (unsigned char)((s8 >> 16) | (s9 << 5));
    s[24] = (unsigned char)(s9 >> 3);
    s[25] = (unsigned char)(s9 >> 11);
    s[26] = (unsigned char)((s9 >> 19) | (s10 << 2));
    s[27] = (unsigned char)(s10 >> 6);
    s[28] = (unsigned char)((s10 >> 14) | (s11 << 7));
    s[29] = (unsigned char)(s11 >> 1);
    s[30] = (unsigned char)(s11 >> 9);
    s[31] = (unsigned char)(s11 >> 17);
}

/* Assumes that a != INT64_MIN */
static int64_t sigNum(int64_t a)
{
    return (a >> 63) - ((-a) >> 63);
}

int scCheck(const unsigned char *s)
{
    int64_t s0 = load4(s);
    int64_t s1 = load4(s + 4);
    int64_t s2 = load4(s + 8);
    int64_t s3 = load4(s + 12);
    int64_t s4 = load4(s + 16);
    int64_t s5 = load4(s + 20);
    int64_t s6 = load4(s + 24);
    int64_t s7 = load4(s + 28);

    return (int)((sigNum(1559614444 - s0)
                  + (sigNum(1477600026 - s1) << 1)
                  + (sigNum(2734136534 - s2) << 2)
                  + (sigNum(350157278 - s3) << 3)
                  + (sigNum(-s4) << 4)
                  + (sigNum(-s5) << 5)
                  + (sigNum(-s6) << 6)
                  + (sigNum(268435456 - s7) << 7))
                 >> 8);
}

int scIsNonZero(const unsigned char *s)
{
    return (
        ((int)(
            s[0] | s[1] | s[2] | s[3] | s[4] | s[5] | s[6] | s[7] | s[8] | s[9] | s[10]
            | s[11] | s[12] | s[13] | s[14] | s[15] | s[16] | s[17] | s[18] | s[19] | s[20]
            | s[21] | s[22] | s[23] | s[24] | s[25] | s[26] | s[27] | s[28] | s[29] | s[30]
            | s[31])
        - 1) >> 8) + 1;
}
