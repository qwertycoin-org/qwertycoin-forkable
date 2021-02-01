// Copyright (c) 2012-2016, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2016-2020, The Karbo developers
// Copyright (c) 2018-2021, The Qwertycoin Group.
//
// This file is part of Qwertycoin.
//
// Karbo is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Karbo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Karbo.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

/* From fe.h */

typedef int32_t fe[10];

/* From ge.h */

typedef struct
{
  fe X;
  fe Y;
  fe Z;
} GeP2;

typedef struct
{
  fe X;
  fe Y;
  fe Z;
  fe T;
} GeP3;

typedef struct
{
  fe X;
  fe Y;
  fe Z;
  fe T;
} GeP1P1;

typedef struct
{
  fe YPlusX;
  fe YMinusX;
  fe XY2D;
} GePrecomp;

typedef struct
{
  fe YPlusX;
  fe YMinusX;
  fe Z;
  fe T2D;
} GeCached;

/* From ge_add.c */

void geAdd(GeP1P1 *, const GeP3 *, const GeCached *);

/* From ge_double_scalarmult.c, modified */

typedef GeCached geDsmp[8];
extern const GePrecomp geBi[8];
void geDsmPrecomp(geDsmp r, const GeP3 *s);
void geDoubleScalarmultBaseVartime(GeP2 *, const unsigned char *, const GeP3 *, const unsigned char *);

/* From ge_frombytes.c, modified */

extern const fe feSqrtm1;
extern const fe feD;
int geFromBytesVarTime(GeP3 *, const unsigned char *);

/* From geP1P1ToP2.c */

void geP1P1ToP2(GeP2 *, const GeP1P1 *);

/* From geP1P1ToP3.c */

void geP1P1ToP3(GeP3 *, const GeP1P1 *);

/* From geP2Dbl.c */

void geP2Dbl(GeP1P1 *, const GeP2 *);

/* From geP3ToCached.c */

extern const fe feD2;
void geP3ToCached(GeCached *, const GeP3 *);

/* From geP3ToP2.c */

void geP3ToP2(GeP2 *, const GeP3 *);

/* From geP3Tobytes.c */

void geP3Tobytes(unsigned char *, const GeP3 *);

/* From geScalarMultBase.c */

extern const GePrecomp ge_base[32][8];
void geScalarMultBase(GeP3 *, const unsigned char *);

/* From geSub.c */

void geSub(GeP1P1 *, const GeP3 *, const GeCached *);

/* From geToBytes.c */

void geToBytes(unsigned char *, const GeP2 *);

/* From scReduce.c */

void scReduce(unsigned char *);

/* New code */

void geScalarMult(GeP2 *, const unsigned char *, const GeP3 *);
void geDoubleScalarMultPrecompVarTime(GeP2 *,
                                      const unsigned char *,
                                      const GeP3 *,
                                      const unsigned char *,
                                      const geDsmp);
void geMul8(GeP1P1 *, const GeP2 *);

extern const fe feMa;
extern const fe feMa2;
extern const fe feFffb1;
extern const fe feFffb2;
extern const fe feFffb3;
extern const fe feFffb4;
void geFromFeFromBytesVarTime(GeP2 *, const unsigned char *);
void sc0(unsigned char *);
void scReduce32(unsigned char *);
void scAdd(unsigned char *, const unsigned char *, const unsigned char *);
void scSub(unsigned char *, const unsigned char *, const unsigned char *);
void scMulSub(unsigned char *, const unsigned char *, const unsigned char *, const unsigned char *);
void scMul(unsigned char *, const unsigned char *, const unsigned char *);
int scCheck(const unsigned char *);
int scIsNonZero(const unsigned char *); /* Doesn't normalize */

#if defined(__cplusplus)
}
#endif
