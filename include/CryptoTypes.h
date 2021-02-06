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

struct FEllipticCurvePoint {
    uint8_t uData[32];
};

struct FEllipticCurveScalar {
    uint8_t uData[32];
};

struct FHash {
    uint8_t uData[32];
};

// This structure can be used to store Hashes in ordered containers
// like std::set<FHash, FHashCompare>
struct FHashCompare {
    bool operator()(const FHash &lh, const FHash &rh) const
    {
        return memcmp(lh.uData, rh.uData, 32) > 0;
    }
};

struct FPublicKey : public FEllipticCurvePoint {
};

struct FSecretKey : public FEllipticCurveScalar {
    ~FSecretKey() { sodiumMemZero(uData, sizeof(uData)); }
};

struct FKeyDerivation : public FEllipticCurvePoint {
};

struct FKeyImage : public FEllipticCurvePoint {
};

struct FSignature {
    FEllipticCurveScalar c, r;
};

#pragma pack(pop)

static_assert(sizeof(FEllipticCurvePoint) == 32
			  && sizeof(FEllipticCurveScalar) == 32,
              "Invalid structure size");

static_assert(sizeof(FHash) == 32
			  && sizeof(FPublicKey) == 32
			  && sizeof(FSecretKey) == 32
			  && sizeof(FKeyDerivation) == 32
			  && sizeof(FKeyImage) == 32
			  && sizeof(FSignature) == 64,
              "Invalid structure size");

// identity (a zero elliptic curve point)
const struct FEllipticCurveScalar I = {{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

// curve order
const struct FEllipticCurveScalar L = {{0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58,
                                         0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 } };

// zero scalar
const struct FEllipticCurveScalar Z = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

// curve basepoint
const struct FEllipticCurveScalar G = {{0x58, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                                         0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                                         0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                                         0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66 } };

} // namespace Crypto
