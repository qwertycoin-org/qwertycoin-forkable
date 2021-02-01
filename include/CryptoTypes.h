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

#include <cstdint>
#include <cstring>
#include <Crypto/CryptoUtil.h>

namespace Crypto {

#pragma pack(push, 1)

struct EllipticCurvePoint {
    uint8_t data[32];
};

struct EllipticCurveScalar {
    uint8_t data[32];
};

struct Hash {
    uint8_t data[32];
};

// This structure can be used to store Hashes in ordered containers
// like std::set<Hash, HashCompare>
struct HashCompare {
    bool operator()(const Hash &lh, const Hash &rh) const
    {
        return memcmp(lh.data, rh.data, 32) > 0;
    }
};

struct PublicKey : public EllipticCurvePoint {
};

struct SecretKey : public EllipticCurveScalar {
    ~SecretKey() { sodiumMemZero(data, sizeof(data)); }
};

struct KeyDerivation : public EllipticCurvePoint {
};

struct KeyImage : public EllipticCurvePoint {
};

struct Signature {
    EllipticCurveScalar c, r;
};

#pragma pack(pop)

static_assert(sizeof(EllipticCurvePoint) == 32 && sizeof(EllipticCurveScalar) == 32,
              "Invalid structure size");

static_assert(sizeof(Hash) == 32 && sizeof(PublicKey) == 32 && sizeof(SecretKey) == 32
                      && sizeof(KeyDerivation) == 32 && sizeof(KeyImage) == 32
                      && sizeof(Signature) == 64,
              "Invalid structure size");

// identity (a zero elliptic curve point)
const struct EllipticCurveScalar I = { { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

// curve order
const struct EllipticCurveScalar L = { { 0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58,
                                         0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 } };

// zero scalar
const struct EllipticCurveScalar Z = { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

// curve basepoint
const struct EllipticCurveScalar G = { { 0x58, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                                         0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                                         0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                                         0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66 } };

} // namespace Crypto
