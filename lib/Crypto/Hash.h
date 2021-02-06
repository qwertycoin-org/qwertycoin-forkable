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

inline void cnFastHash(const void *data, size_t length, FHash &hash)
{
    cnFastHash(data, length, reinterpret_cast<char *>(&hash));
}

inline FHash cnFastHash(const void *data, size_t length)
{
    FHash h {};
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
								  FHash &hash);
};

inline void cnSlowHash(CnContext &context,
                       const void *data,
                       size_t length,
					   FHash &hash)
{
    cnSlowHash(data, length, reinterpret_cast<char *>(&hash));
}

inline void treeHash(const FHash *hashes, size_t count, FHash &root_hash)
{
    treeHash(reinterpret_cast<const char(*)[HASH_SIZE]>(hashes), count,
             reinterpret_cast<char *>(&root_hash));
}

inline void treeBranch(const FHash *hashes, size_t count, FHash *branch)
{
    treeBranch(reinterpret_cast<const char(*)[HASH_SIZE]>(hashes),
               count,
               reinterpret_cast<char(*)[HASH_SIZE]>(branch));
}

inline void treeHashFromBranch(const FHash *branch,
                               size_t depth,
							   const FHash &leaf,
                               const void *path,
							   FHash &root_hash)
{
    treeHashFromBranch(reinterpret_cast<const char(*)[HASH_SIZE]>(branch), depth,
                       reinterpret_cast<const char *>(&leaf), path,
                       reinterpret_cast<char *>(&root_hash));
}

}

CRYPTO_MAKE_HASHABLE(FHash)
CRYPTO_MAKE_HASHABLE(FEllipticCurveScalar)
CRYPTO_MAKE_HASHABLE(FEllipticCurvePoint)
CRYPTO_MAKE_HASHABLE(FPublicKey)
CRYPTO_MAKE_HASHABLE(FSecretKey)
CRYPTO_MAKE_HASHABLE(FKeyDerivation)
CRYPTO_MAKE_HASHABLE(FKeyImage)