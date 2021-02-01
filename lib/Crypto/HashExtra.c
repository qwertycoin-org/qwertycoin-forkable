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
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <Crypto/Blake256.h>
#include <Crypto/Groestl.h>
#include <Crypto/Jh.h>
#include <Crypto/HashOps.h>
#include <Crypto/Skein.h>

void hashExtraBlake(const void *data, size_t length, char *hash)
{
    blake256Hash((uint8_t *)hash, data, length);
}

void hashExtraGroestl(const void *data, size_t length, char *hash)
{
    groestl(data, length * 8, (uint8_t *)hash);
}

void hashExtraJh(const void *data, size_t length, char *hash)
{
    int r = jhHash(HASH_SIZE * 8, data, 8 * length, (uint8_t *)hash);
    assert(SUCCESS == r);
}

void hashExtraSkein(const void *data, size_t length, char *hash)
{
    int r = skeinHash(8 * HASH_SIZE, data, 8 * length, (uint8_t *)hash);
    assert(SKEIN_SUCCESS == r);
}