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

#include <cstddef>

#include <CryptoTypes.h>

#include <Crypto/GenericOps.h>

namespace Crypto {

extern "C" {
#include <Crypto/HashOps.h>
}

/*
  CryptoNight Hash functions
*/

inline void cnFastHash(const void *data, size_t length, Hash &hash)
{
    cnFastHash(data, length, reinterpret_cast<char *>(&hash));
}

inline Hash cnFastHash(const void *data, size_t length)
{
    Hash h {};
    cnFastHash(data, length, reinterpret_cast<char *>(&h));
    return h;
}

class CnContext
{
public:
    CnContext();
    ~CnContext();
#if !defined(_MSC_VER) || _MSC_VER >= 1800
    CnContext(const CnContext &) = delete;
    void operator=(const CnContext &) = delete;
#endif

private:
    void *data;
    friend inline void cnSlowHash(CnContext &context,
                                  const void *data,
                                  size_t length,
                                  Hash &hash);
};

inline void cnSlowHash(CnContext &context,
                       const void *data,
                       size_t length,
                       Hash &hash)
{
    cnSlowHash(data, length, reinterpret_cast<char *>(&hash));
}

inline void treeHash(const Hash *hashes, size_t count, Hash &root_hash)
{
    treeHash(reinterpret_cast<const char(*)[HASH_SIZE]>(hashes), count,
             reinterpret_cast<char *>(&root_hash));
}

inline void treeBranch(const Hash *hashes, size_t count, Hash *branch)
{
    treeBranch(reinterpret_cast<const char(*)[HASH_SIZE]>(hashes),
               count,
               reinterpret_cast<char(*)[HASH_SIZE]>(branch));
}

inline void treeHashFromBranch(const Hash *branch,
                               size_t depth,
                               const Hash &leaf,
                               const void *path,
                               Hash &root_hash)
{
    treeHashFromBranch(reinterpret_cast<const char(*)[HASH_SIZE]>(branch), depth,
                       reinterpret_cast<const char *>(&leaf), path,
                       reinterpret_cast<char *>(&root_hash));
}

}

CRYPTO_MAKE_HASHABLE(Hash)
CRYPTO_MAKE_HASHABLE(EllipticCurveScalar)
CRYPTO_MAKE_HASHABLE(EllipticCurvePoint)
CRYPTO_MAKE_HASHABLE(PublicKey)
CRYPTO_MAKE_HASHABLE(SecretKey)
CRYPTO_MAKE_HASHABLE(KeyDerivation)
CRYPTO_MAKE_HASHABLE(KeyImage)