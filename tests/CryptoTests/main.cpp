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

#include <cstddef>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "crypto/Crypto.h"
#include "crypto/crypto-util.h"
#include "crypto/hash.h"
#include "crypto-tests.h"
#include "../Common/Io.h"

using namespace std;
typedef Crypto::Hash chash;

int main(int argc, char *argv[]) {
  fstream input;
  string cmd;
  size_t test = 0;
  bool error = false;
  setup_random();
  if (argc != 2) {
    cerr << "invalid arguments" << endl;
    return 1;
  }
  input.open(argv[1], ios_base::in);
  for (;;) {
    ++test;
    input.exceptions(ios_base::badbit);
    if (!(input >> cmd)) {
      break;
    }
    input.exceptions(ios_base::badbit | ios_base::failbit | ios_base::eofbit);
    if (cmd == "checkScalar") {
      Crypto::EllipticCurveScalar scalar;
      bool expected, actual;
      get(input, scalar, expected);
      actual = checkScalar(scalar);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "randomScalar") {
      Crypto::EllipticCurveScalar expected{};
      Crypto::EllipticCurveScalar actual{};
      get(input, expected);
      randomScalar(actual);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "hashToScalar") {
      vector<char> data;
      Crypto::EllipticCurveScalar expected{}, actual{};
      get(input, data, expected);
      Crypto::hashToScalar(data.data(), data.size(), actual);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "generateKeys") {
      Crypto::PublicKey expected1, actual1;
      Crypto::SecretKey expected2, actual2;
      get(input, expected1, expected2);
      generateKeys(actual1, actual2);
      if (expected1 != actual1 || expected2 != actual2) {
        goto error;
      }
    } else if (cmd == "checkKey") {
      Crypto::PublicKey key;
      bool expected, actual;
      get(input, key, expected);
      actual = checkKey(key);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "secretKeyToPublicKey") {
      Crypto::SecretKey sec;
      bool expected1, actual1;
      Crypto::PublicKey expected2, actual2;
      get(input, sec, expected1);
      if (expected1) {
        get(input, expected2);
      }
      actual1 = secretKeyToPublicKey(sec, actual2);
      if (expected1 != actual1 || (expected1 && expected2 != actual2)) {
        goto error;
      }
    } else if (cmd == "generateKeyDerivation") {
      Crypto::PublicKey key1;
      Crypto::SecretKey key2;
      bool expected1, actual1;
      Crypto::KeyDerivation expected2, actual2;
      get(input, key1, key2, expected1);
      if (expected1) {
        get(input, expected2);
      }
      actual1 = generateKeyDerivation(key1, key2, actual2);
      if (expected1 != actual1 || (expected1 && expected2 != actual2)) {
        goto error;
      }
    } else if (cmd == "derivePublicKey") {
      Crypto::KeyDerivation derivation;
      size_t output_index;
      Crypto::PublicKey base;
      bool expected1, actual1;
      Crypto::PublicKey expected2, actual2;
      get(input, derivation, output_index, base, expected1);
      if (expected1) {
        get(input, expected2);
      }
      actual1 = derivePublicKey(derivation, output_index, base, actual2);
      if (expected1 != actual1 || (expected1 && expected2 != actual2)) {
        goto error;
      }
    } else if (cmd == "deriveSecretKey") {
      Crypto::KeyDerivation derivation;
      size_t output_index;
      Crypto::SecretKey base;
      Crypto::SecretKey expected, actual;
      get(input, derivation, output_index, base, expected);
      deriveSecretKey(derivation, output_index, base, actual);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "underivePublicKey") {
      Crypto::KeyDerivation derivation;
      size_t output_index;
      Crypto::PublicKey derived_key;
      bool expected1, actual1;
      Crypto::PublicKey expected2, actual2;
      get(input, derivation, output_index, derived_key, expected1);
      if (expected1) {
        get(input, expected2);
      }
      actual1 = underivePublicKey(derivation, output_index, derived_key, actual2);
      if (expected1 != actual1 || (expected1 && expected2 != actual2)) {
        goto error;
      }
    } else if (cmd == "generateSignature") {
      chash prefix_hash;
      Crypto::PublicKey pub;
      Crypto::SecretKey sec;
      Crypto::Signature expected, actual;
      get(input, prefix_hash, pub, sec, expected);
      generateSignature(prefix_hash, pub, sec, actual);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "checkSignature") {
      chash prefix_hash;
      Crypto::PublicKey pub;
      Crypto::Signature sig;
      bool expected, actual;
      get(input, prefix_hash, pub, sig, expected);
      actual = checkSignature(prefix_hash, pub, sig);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "hashToPoint") {
      chash h;
      Crypto::EllipticCurvePoint expected, actual;
      get(input, h, expected);
      hashToPoint(h, actual);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "hashToEC") {
      Crypto::PublicKey key;
      Crypto::EllipticCurvePoint expected, actual;
      get(input, key, expected);
      hashToEC(key, actual);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "generateKeyImage") {
      Crypto::PublicKey pub;
      Crypto::SecretKey sec;
      Crypto::KeyImage expected, actual;
      get(input, pub, sec, expected);
      generateKeyImage(pub, sec, actual);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "generateRingSignature") {
      chash prefix_hash;
      Crypto::KeyImage image;
      vector<Crypto::PublicKey> vpubs;
      vector<const Crypto::PublicKey *> pubs;
      size_t pubs_count;
      Crypto::SecretKey sec;
      size_t sec_index;
      vector<Crypto::Signature> expected, actual;
      size_t i;
      get(input, prefix_hash, image, pubs_count);
      vpubs.resize(pubs_count);
      pubs.resize(pubs_count);
      for (i = 0; i < pubs_count; i++) {
        get(input, vpubs[i]);
        pubs[i] = &vpubs[i];
      }
      get(input, sec, sec_index);
      expected.resize(pubs_count);
      getvar(input, pubs_count * sizeof(Crypto::Signature), expected.data());
      actual.resize(pubs_count);
      generateRingSignature(prefix_hash, image, pubs.data(), pubs_count, sec, sec_index,
                            actual.data());
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "checkRingSignature") {
      chash prefix_hash;
      Crypto::KeyImage image;
      vector<Crypto::PublicKey> vpubs;
      vector<const Crypto::PublicKey *> pubs;
      size_t pubs_count;
      vector<Crypto::Signature> sigs;
      bool expected, actual;
      size_t i;
      get(input, prefix_hash, image, pubs_count);
      vpubs.resize(pubs_count);
      pubs.resize(pubs_count);
      for (i = 0; i < pubs_count; i++) {
        get(input, vpubs[i]);
        pubs[i] = &vpubs[i];
      }
      sigs.resize(pubs_count);
      getvar(input, pubs_count * sizeof(Crypto::Signature), sigs.data());
      get(input, expected);
      actual = checkRingSignature(prefix_hash, image, pubs.data(), pubs_count, sigs.data());
      if (expected != actual) {
        goto error;
      }
    } else {
      throw ios_base::failure("Unknown function: " + cmd);
    }
    continue;
error:
    cerr << "Wrong result on test " << test << endl;
    error = true;
  }
  return error ? 1 : 0;
}
