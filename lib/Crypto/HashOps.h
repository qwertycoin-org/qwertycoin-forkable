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

#pragma once

#if !defined(__cplusplus)

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <Common/StaticAssert.h>
#include <Common/IntUtil.h>

static inline void *pAdd(void *p, size_t i)
{
    return (char *) p + i;
}

static inline const void *cPAdd(const void *p, size_t i)
{
    return (const char *) p + i;
}

static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8, "size_t must be 4 or 8 bytes long");
static inline void placeLength(uint8_t *buffer, size_t bufsize, size_t length)
{
    if (sizeof(size_t) == 4) {
        *(uint32_t *) pAdd(buffer, bufsize - 4) = swap32be((uint32_t) length);
    } else {
        *(uint64_t *) pAdd(buffer, bufsize - 8) = swap64be(length);
    }
}

#pragma pack(push, 1)
union hashState {
  uint8_t b[200];
  uint64_t w[25];
};
#pragma pack(pop)
static_assert(sizeof(union hashState) == 200, "Invalid structure size");

void hashPermutation(union hashState *state);
void hashProcess(union hashState *state, const uint8_t *buf, size_t count);

#endif

enum
{
  HASH_SIZE = 32,
  HASH_DATA_AREA = 136,
  SLOW_HASH_CONTEXT_SIZE = 2097552
};

void cnFastHash(const void *data, size_t length, char *hash);

void cnSlowHash(const void *data, size_t length, char *hash);

void hashExtraBlake(const void *data, size_t length, char *hash);
void hashExtraGroestl(const void *data, size_t length, char *hash);
void hashExtraJh(const void *data, size_t length, char *hash);
void hashExtraSkein(const void *data, size_t length, char *hash);

void treeHash(const char (*hashes)[HASH_SIZE], size_t count, char *root_hash);
size_t treeDepth(size_t count);
void treeBranch(const char (*hashes)[HASH_SIZE], size_t count, char (*branch)[HASH_SIZE]);
void treeHashFromBranch(const char (*branch)[HASH_SIZE],
                        size_t depth,
                        const char *leaf,
                        const void *path,
                        char *root_hash);
