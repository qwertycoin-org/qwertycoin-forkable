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

#include "crypto/Crypto.cpp"

#include "crypto-tests.h"


bool checkScalar(const Crypto::EllipticCurveScalar &scalar) {
  return sc_check(reinterpret_cast<const unsigned char*>(&scalar)) == 0;
}

void randomScalar(Crypto::EllipticCurveScalar &res) {
    Crypto::randomScalar(res);
}

void hashToScalar(const void *data, size_t length, Crypto::EllipticCurveScalar &res) {
    Crypto::hashToScalar(data, length, res);
}

void hashToPoint(const Crypto::Hash &h, Crypto::EllipticCurvePoint &res) {
  ge_p2 point;
  ge_fromfe_frombytes_vartime(&point, reinterpret_cast<const unsigned char *>(&h));
  ge_tobytes(reinterpret_cast<unsigned char*>(&res), &point);
}

void hashToEC(const Crypto::PublicKey &key, Crypto::EllipticCurvePoint &res) {
  ge_p3 tmp;
  Crypto::hashToEC(key, tmp);
  ge_p3_tobytes(reinterpret_cast<unsigned char*>(&res), &tmp);
}
