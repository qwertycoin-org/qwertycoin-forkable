// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2018-2021, The Qwertycoin Group.
// Copyright (c) 2014-2017, The Monero Project
// Copyright (c) 2016-2018, The Karbo developers
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

#ifndef __FreeBSD__
#include <alloca.h>
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "Common/Varint.h"
#include "Crypto.h"
#include "CryptoOps.h"
#include "hash.h"
#include "random.h"

namespace Crypto {

static inline unsigned char *operator&(EllipticCurvePoint &point)
{
    return &reinterpret_cast<unsigned char &>(point);
}

static inline const unsigned char *operator&(const EllipticCurvePoint &point)
{
    return &reinterpret_cast<const unsigned char &>(point);
}

static inline unsigned char *operator&(EllipticCurveScalar &scalar)
{
    return &reinterpret_cast<unsigned char &>(scalar);
}

static inline const unsigned char *operator&(const EllipticCurveScalar &scalar)
{
    return &reinterpret_cast<const unsigned char &>(scalar);
}

static inline void randomScalar(EllipticCurveScalar &res)
{
    unsigned char tmp[64];
    Random::randomBytes(64, tmp);
    sc_reduce(tmp);
    memcpy(&res, tmp, 32);
}

void hashToScalar(const void *data, size_t length, EllipticCurveScalar &res)
{
    cn_fast_hash(data, length, reinterpret_cast<Hash &>(res));
    sc_reduce32(reinterpret_cast<unsigned char *>(&res));
}

void CryptoOps::generateKeys(PublicKey &pub, SecretKey &sec)
{
    ge_p3 point;
    randomScalar(reinterpret_cast<EllipticCurveScalar &>(sec));
    ge_scalarmult_base(&point, reinterpret_cast<unsigned char *>(&sec));
    ge_p3_tobytes(reinterpret_cast<unsigned char *>(&pub), &point);
}

void CryptoOps::generateDeterministicKeys(PublicKey &pub, SecretKey &sec, SecretKey &second)
{
    ge_p3 point;
    sec = second;
    sc_reduce32(reinterpret_cast<unsigned char *>(
            &sec)); // reduce in case second round of keys (sendkeys)
    ge_scalarmult_base(&point, reinterpret_cast<unsigned char *>(&sec));
    ge_p3_tobytes(reinterpret_cast<unsigned char *>(&pub), &point);
}

SecretKey CryptoOps::generateMKeys(PublicKey &pub, SecretKey &sec, const SecretKey &recovery_key,
                                    bool recover)
{
    ge_p3 point;
    SecretKey rng;
    if (recover) {
        rng = recovery_key;
    } else {
        randomScalar(reinterpret_cast<EllipticCurveScalar &>(rng));
    }
    sec = rng;
    sc_reduce32(reinterpret_cast<unsigned char *>(
            &sec)); // reduce in case second round of keys (sendkeys)
    ge_scalarmult_base(&point, reinterpret_cast<unsigned char *>(&sec));
    ge_p3_tobytes(reinterpret_cast<unsigned char *>(&pub), &point);

    return rng;
}

bool CryptoOps::checkKey(const PublicKey &key)
{
    ge_p3 point;
    return ge_frombytes_vartime(&point, reinterpret_cast<const unsigned char *>(&key)) == 0;
}

bool CryptoOps::secretKeyToPublicKey(const SecretKey &sec, PublicKey &pub)
{
    ge_p3 point;
    if (sc_check(reinterpret_cast<const unsigned char *>(&sec)) != 0) {
        return false;
    }
    ge_scalarmult_base(&point, reinterpret_cast<const unsigned char *>(&sec));
    ge_p3_tobytes(reinterpret_cast<unsigned char *>(&pub), &point);
    return true;
}

bool CryptoOps::secretKeyMultPublicKey(const SecretKey &sec, const PublicKey &pub, PublicKey &res)
{
    if (sc_check(&sec) != 0) {
        return false;
    }
    ge_p3 point;
    if (ge_frombytes_vartime(&point, &pub) != 0) {
        return false;
    }
    ge_p2 point2;
    ge_scalarmult(&point2, &sec, &point);
    ge_tobytes(reinterpret_cast<unsigned char *>(&res), &point2);
    return true;
}

bool CryptoOps::generateKeyDerivation(const PublicKey &key1, const SecretKey &key2,
                                       KeyDerivation &derivation)
{
    ge_p3 point;
    ge_p2 point2;
    ge_p1p1 point3;
    assert(sc_check(reinterpret_cast<const unsigned char *>(&key2)) == 0);
    if (ge_frombytes_vartime(&point, reinterpret_cast<const unsigned char *>(&key1)) != 0) {
        return false;
    }
    ge_scalarmult(&point2, reinterpret_cast<const unsigned char *>(&key2), &point);
    ge_mul8(&point3, &point2);
    ge_p1p1_to_p2(&point2, &point3);
    ge_tobytes(reinterpret_cast<unsigned char *>(&derivation), &point2);
    return true;
}

static void derivationToScalar(const KeyDerivation &derivation, size_t output_index,
                               EllipticCurveScalar &res)
{
    struct {
        KeyDerivation derivation;
        char output_index[(sizeof(size_t) * 8 + 6) / 7];
    } buf;
    char *end = buf.output_index;
    buf.derivation = derivation;
    Tools::writeVarint(end, output_index);
    assert(end <= buf.output_index + sizeof buf.output_index);
    hashToScalar(&buf, end - reinterpret_cast<char *>(&buf), res);
}

static void derivationToScalar(const KeyDerivation &derivation, size_t output_index,
                               const uint8_t *suffix, size_t suffixLength, EllipticCurveScalar &res)
{
    assert(suffixLength <= 32);
    struct {
        KeyDerivation derivation;
        char output_index[(sizeof(size_t) * 8 + 6) / 7 + 32];
    } buf;
    char *end = buf.output_index;
    buf.derivation = derivation;
    Tools::writeVarint(end, output_index);
    assert(end <= buf.output_index + sizeof buf.output_index);
    size_t bufSize = end - reinterpret_cast<char *>(&buf);
    memcpy(end, suffix, suffixLength);
    hashToScalar(&buf, bufSize + suffixLength, res);
}

bool CryptoOps::derivePublicKey(const KeyDerivation &derivation, size_t outputIndex,
                                 const PublicKey &base, PublicKey &derivedKey)
{
    EllipticCurveScalar scalar;
    ge_p3 point1;
    ge_p3 point2;
    ge_cached point3;
    ge_p1p1 point4;
    ge_p2 point5;
    if (ge_frombytes_vartime(&point1, reinterpret_cast<const unsigned char *>(&base)) != 0) {
        return false;
    }
    derivationToScalar(derivation, outputIndex, scalar);
    ge_scalarmult_base(&point2, reinterpret_cast<unsigned char *>(&scalar));
    ge_p3_to_cached(&point3, &point2);
    ge_add(&point4, &point1, &point3);
    ge_p1p1_to_p2(&point5, &point4);
    ge_tobytes(reinterpret_cast<unsigned char *>(&derivedKey), &point5);
    return true;
}

bool CryptoOps::derivePublicKey(const KeyDerivation &derivation, size_t outputIndex,
                                 const PublicKey &base, const uint8_t *suffix, size_t suffixLength,
                                 PublicKey &derivedKey)
{
    EllipticCurveScalar scalar;
    ge_p3 point1;
    ge_p3 point2;
    ge_cached point3;
    ge_p1p1 point4;
    ge_p2 point5;
    if (ge_frombytes_vartime(&point1, reinterpret_cast<const unsigned char *>(&base)) != 0) {
        return false;
    }
    derivationToScalar(derivation, outputIndex, suffix, suffixLength, scalar);
    ge_scalarmult_base(&point2, reinterpret_cast<unsigned char *>(&scalar));
    ge_p3_to_cached(&point3, &point2);
    ge_add(&point4, &point1, &point3);
    ge_p1p1_to_p2(&point5, &point4);
    ge_tobytes(reinterpret_cast<unsigned char *>(&derivedKey), &point5);
    return true;
}

bool CryptoOps::underivePublicKeyAndGetScalar(const KeyDerivation &derivation, size_t outputIndex,
                                               const PublicKey &derivedKey, PublicKey &base,
                                               EllipticCurveScalar &hashedDerivation)
{
    ge_p3 point1;
    ge_p3 point2;
    ge_cached point3;
    ge_p1p1 point4;
    ge_p2 point5;
    if (ge_frombytes_vartime(&point1, reinterpret_cast<const unsigned char *>(&derivedKey)) != 0) {
        return false;
    }
    derivationToScalar(derivation, outputIndex, hashedDerivation);
    ge_scalarmult_base(&point2, reinterpret_cast<unsigned char *>(&hashedDerivation));
    ge_p3_to_cached(&point3, &point2);
    ge_sub(&point4, &point1, &point3);
    ge_p1p1_to_p2(&point5, &point4);
    ge_tobytes(reinterpret_cast<unsigned char *>(&base), &point5);
    return true;
}

void CryptoOps::deriveSecretKey(const KeyDerivation &derivation, size_t outputIndex,
                                 const SecretKey &base, SecretKey &derivedKey)
{
    EllipticCurveScalar scalar;
    assert(sc_check(reinterpret_cast<const unsigned char *>(&base)) == 0);
    derivationToScalar(derivation, outputIndex, scalar);
    sc_add(reinterpret_cast<unsigned char *>(&derivedKey),
           reinterpret_cast<const unsigned char *>(&base),
           reinterpret_cast<unsigned char *>(&scalar));
}

void CryptoOps::deriveSecretKey(const KeyDerivation &keyDerivation, size_t outputIndex,
                                 const SecretKey &baseKey, const uint8_t *suffix,
                                 size_t suffixLength, SecretKey &derivedKey)
{
    EllipticCurveScalar scalar;
    assert(sc_check(reinterpret_cast<const unsigned char *>(&baseKey)) == 0);
    derivationToScalar(keyDerivation, outputIndex, suffix, suffixLength, scalar);
    sc_add(reinterpret_cast<unsigned char *>(&derivedKey),
           reinterpret_cast<const unsigned char *>(&baseKey),
           reinterpret_cast<unsigned char *>(&scalar));
}

bool CryptoOps::underivePublicKey(const KeyDerivation &keyDerivation, size_t outputIndex,
                                   const PublicKey &derivedKey, PublicKey &baseKey)
{
    EllipticCurveScalar scalar;
    ge_p3 point1;
    ge_p3 point2;
    ge_cached point3;
    ge_p1p1 point4;
    ge_p2 point5;
    if (ge_frombytes_vartime(&point1, reinterpret_cast<const unsigned char *>(&derivedKey)) != 0) {
        return false;
    }
    derivationToScalar(keyDerivation, outputIndex, scalar);
    ge_scalarmult_base(&point2, reinterpret_cast<unsigned char *>(&scalar));
    ge_p3_to_cached(&point3, &point2);
    ge_sub(&point4, &point1, &point3);
    ge_p1p1_to_p2(&point5, &point4);
    ge_tobytes(reinterpret_cast<unsigned char *>(&baseKey), &point5);
    return true;
}

bool CryptoOps::underivePublicKey(const KeyDerivation &keyDerivation, size_t outputIndex,
                                   const PublicKey &derivedKey, const uint8_t *suffix,
                                   size_t suffixLength, PublicKey &baseKey)
{
    EllipticCurveScalar scalar;
    ge_p3 point1;
    ge_p3 point2;
    ge_cached point3;
    ge_p1p1 point4;
    ge_p2 point5;
    if (ge_frombytes_vartime(&point1, reinterpret_cast<const unsigned char *>(&derivedKey)) != 0) {
        return false;
    }

    derivationToScalar(keyDerivation, outputIndex, suffix, suffixLength, scalar);
    ge_scalarmult_base(&point2, reinterpret_cast<unsigned char *>(&scalar));
    ge_p3_to_cached(&point3, &point2);
    ge_sub(&point4, &point1, &point3);
    ge_p1p1_to_p2(&point5, &point4);
    ge_tobytes(reinterpret_cast<unsigned char *>(&baseKey), &point5);
    return true;
}

struct sComm {
    Hash h;
    EllipticCurvePoint key;
    EllipticCurvePoint comm;
};

struct sComm2 {
    Hash msg;
    EllipticCurvePoint D;
    EllipticCurvePoint X;
    EllipticCurvePoint Y;
};

void CryptoOps::generateSignature(const Hash &prefixHash, const PublicKey &publicKey,
                                   const SecretKey &secretKey, Signature &signature)
{
    ge_p3 tmp3;
    EllipticCurveScalar k;
    sComm buf;
#if !defined(NDEBUG)
    {
        ge_p3 t;
        PublicKey t2;
        assert(sc_check(reinterpret_cast<const unsigned char *>(&secretKey)) == 0);
        ge_scalarmult_base(&t, reinterpret_cast<const unsigned char *>(&secretKey));
        ge_p3_tobytes(reinterpret_cast<unsigned char *>(&t2), &t);
        assert(publicKey == t2);
    }
#endif
    buf.h = prefixHash;
    buf.key = reinterpret_cast<const EllipticCurvePoint &>(publicKey);
try_again:
    randomScalar(k);
    if (((const uint32_t *)(&k))[7] == 0) // we don't want tiny numbers here
        goto try_again;
    ge_scalarmult_base(&tmp3, reinterpret_cast<unsigned char *>(&k));
    ge_p3_tobytes(reinterpret_cast<unsigned char *>(&buf.comm), &tmp3);
    hashToScalar(&buf, sizeof(sComm), reinterpret_cast<EllipticCurveScalar &>(signature));
    if (!sc_isnonzero(
                (const unsigned char *)reinterpret_cast<EllipticCurveScalar &>(signature).data))
        goto try_again;
    sc_mulsub(reinterpret_cast<unsigned char *>(&signature) + 32,
              reinterpret_cast<unsigned char *>(&signature),
              reinterpret_cast<const unsigned char *>(&secretKey),
              reinterpret_cast<unsigned char *>(&k));
    if (!sc_isnonzero((const unsigned char *)reinterpret_cast<unsigned char *>(&signature) + 32))
        goto try_again;
}

bool CryptoOps::checkSignature(const Hash &prefixHash, const PublicKey &publicKey,
                                const Signature &signature)
{
    ge_p2 tmp2;
    ge_p3 tmp3;
    EllipticCurveScalar c;
    sComm buf;
    assert(checkKey(publicKey));
    buf.h = prefixHash;
    buf.key = reinterpret_cast<const EllipticCurvePoint &>(publicKey);
    if (ge_frombytes_vartime(&tmp3, reinterpret_cast<const unsigned char *>(&publicKey)) != 0) {
        abort();
    }
    if (sc_check(reinterpret_cast<const unsigned char *>(&signature)) != 0
        || sc_check(reinterpret_cast<const unsigned char *>(&signature) + 32) != 0
        || !sc_isnonzero(reinterpret_cast<const unsigned char *>(&signature))) {
        return false;
    }
    ge_double_scalarmult_base_vartime(&tmp2, reinterpret_cast<const unsigned char *>(&signature),
                                      &tmp3,
                                      reinterpret_cast<const unsigned char *>(&signature) + 32);
    ge_tobytes(reinterpret_cast<unsigned char *>(&buf.comm), &tmp2);
    static const EllipticCurvePoint infinity = {
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };
    if (memcmp(&buf.comm, &infinity, 32) == 0)
        return false;
    hashToScalar(&buf, sizeof(sComm), c);
    sc_sub(reinterpret_cast<unsigned char *>(&c), reinterpret_cast<unsigned char *>(&c),
           reinterpret_cast<const unsigned char *>(&signature));
    return sc_isnonzero(reinterpret_cast<unsigned char *>(&c)) == 0;
}

void CryptoOps::generateTxProof(const Hash &prefixHash, const PublicKey &R, const PublicKey &A,
                                 const PublicKey &D, const SecretKey &r, Signature &signature)
{
    // sanity check
    ge_p3 R_p3;
    ge_p3 A_p3;
    ge_p3 D_p3;
    if (ge_frombytes_vartime(&R_p3, reinterpret_cast<const unsigned char *>(&R)) != 0)
        throw std::runtime_error("tx pubkey is invalid");
    if (ge_frombytes_vartime(&A_p3, reinterpret_cast<const unsigned char *>(&A)) != 0)
        throw std::runtime_error("recipient view pubkey is invalid");
    if (ge_frombytes_vartime(&D_p3, reinterpret_cast<const unsigned char *>(&D)) != 0)
        throw std::runtime_error("key derivation is invalid");

    assert(sc_check(reinterpret_cast<const unsigned char *>(&r)) == 0);
    // check R == r*G
    ge_p3 dbg_R_p3;
    ge_scalarmult_base(&dbg_R_p3, reinterpret_cast<const unsigned char *>(&r));
    PublicKey dbg_R;
    ge_p3_tobytes(reinterpret_cast<unsigned char *>(&dbg_R), &dbg_R_p3);
    assert(R == dbg_R);
    // check D == r*A
    ge_p2 dbg_D_p2;
    ge_scalarmult(&dbg_D_p2, reinterpret_cast<const unsigned char *>(&r), &A_p3);
    PublicKey dbg_D;
    ge_tobytes(reinterpret_cast<unsigned char *>(&dbg_D), &dbg_D_p2);
    assert(D == dbg_D);

    // pick random k
    EllipticCurveScalar k;
    randomScalar(k);

    // compute X = k*G
    ge_p3 X_p3;
    ge_scalarmult_base(&X_p3, reinterpret_cast<unsigned char *>(&k));

    // compute Y = k*A
    ge_p2 Y_p2;
    ge_scalarmult(&Y_p2, reinterpret_cast<unsigned char *>(&k), &A_p3);

    // signature.c = Hs(Msg || D || X || Y)
    sComm2 buf;
    buf.msg = prefixHash;
    buf.D = reinterpret_cast<const EllipticCurvePoint &>(D);
    ge_p3_tobytes(reinterpret_cast<unsigned char *>(&buf.X), &X_p3);
    ge_tobytes(reinterpret_cast<unsigned char *>(&buf.Y), &Y_p2);
    hashToScalar(&buf, sizeof(sComm2), reinterpret_cast<EllipticCurveScalar &>(signature));

    // signature.r = k - signature.c*r
    sc_mulsub(reinterpret_cast<unsigned char *>(&signature) + 32,
              reinterpret_cast<unsigned char *>(&signature),
              reinterpret_cast<const unsigned char *>(&r), reinterpret_cast<unsigned char *>(&k));
}

bool CryptoOps::checkTxProof(const Hash &prefixHash, const PublicKey &R, const PublicKey &A,
                              const PublicKey &D, const Signature &signature)
{
    // sanity check
    ge_p3 R_p3;
    ge_p3 A_p3;
    ge_p3 D_p3;
    if (ge_frombytes_vartime(&R_p3, reinterpret_cast<const unsigned char *>(&R)) != 0)
        return false;
    if (ge_frombytes_vartime(&A_p3, reinterpret_cast<const unsigned char *>(&A)) != 0)
        return false;
    if (ge_frombytes_vartime(&D_p3, reinterpret_cast<const unsigned char *>(&D)) != 0)
        return false;
    if (sc_check(reinterpret_cast<const unsigned char *>(&signature)) != 0
        || sc_check(reinterpret_cast<const unsigned char *>(&signature) + 32) != 0)
        return false;

    // compute signature.c*R
    ge_p2 cR_p2;
    ge_scalarmult(&cR_p2, reinterpret_cast<const unsigned char *>(&signature), &R_p3);

    // compute signature.r*G
    ge_p3 rG_p3;
    ge_scalarmult_base(&rG_p3, reinterpret_cast<const unsigned char *>(&signature) + 32);

    // compute signature.c*D
    ge_p2 cD_p2;
    ge_scalarmult(&cD_p2, reinterpret_cast<const unsigned char *>(&signature), &D_p3);

    // compute signature.r*A
    ge_p2 rA_p2;
    ge_scalarmult(&rA_p2, reinterpret_cast<const unsigned char *>(&signature) + 32, &A_p3);

    // compute X = signature.c*R + signature.r*G
    PublicKey cR;
    ge_tobytes(reinterpret_cast<unsigned char *>(&cR), &cR_p2);
    ge_p3 cR_p3;
    if (ge_frombytes_vartime(&cR_p3, reinterpret_cast<const unsigned char *>(&cR)) != 0)
        return false;
    ge_cached rG_cached;
    ge_p3_to_cached(&rG_cached, &rG_p3);
    ge_p1p1 X_p1p1;
    ge_add(&X_p1p1, &cR_p3, &rG_cached);
    ge_p2 X_p2;
    ge_p1p1_to_p2(&X_p2, &X_p1p1);

    // compute Y = signature.c*D + signature.r*A
    PublicKey cD;
    PublicKey rA;
    ge_tobytes(reinterpret_cast<unsigned char *>(&cD), &cD_p2);
    ge_tobytes(reinterpret_cast<unsigned char *>(&rA), &rA_p2);
    ge_p3 cD_p3;
    ge_p3 rA_p3;
    if (ge_frombytes_vartime(&cD_p3, reinterpret_cast<const unsigned char *>(&cD)) != 0)
        return false;
    if (ge_frombytes_vartime(&rA_p3, reinterpret_cast<const unsigned char *>(&rA)) != 0)
        return false;
    ge_cached rA_cached;
    ge_p3_to_cached(&rA_cached, &rA_p3);
    ge_p1p1 Y_p1p1;
    ge_add(&Y_p1p1, &cD_p3, &rA_cached);
    ge_p2 Y_p2;
    ge_p1p1_to_p2(&Y_p2, &Y_p1p1);

    // compute c2 = Hs(Msg || D || X || Y)
    sComm2 buf;
    buf.msg = prefixHash;
    buf.D = reinterpret_cast<const EllipticCurvePoint &>(D);
    ge_tobytes(&buf.X, &X_p2);
    ge_tobytes(&buf.Y, &Y_p2);
    EllipticCurveScalar c2;
    hashToScalar(&buf, sizeof(sComm2), c2);

    // test if c2 == signature.c
    sc_sub(reinterpret_cast<unsigned char *>(&c2), reinterpret_cast<unsigned char *>(&c2),
           reinterpret_cast<const unsigned char *>(&signature));
    return sc_isnonzero(&c2) == 0;
}

static void hashToEC(const PublicKey &key, ge_p3 &res)
{
    Hash h;
    ge_p2 point;
    ge_p1p1 point2;
    cn_fast_hash(std::addressof(key), sizeof(PublicKey), h);
    ge_fromfe_frombytes_vartime(&point, reinterpret_cast<const unsigned char *>(&h));
    ge_mul8(&point2, &point);
    ge_p1p1_to_p3(&res, &point2);
}

KeyImage CryptoOps::scalarMultKey(const KeyImage &P, const KeyImage &a)
{
    ge_p3 A;
    ge_p2 R;
    // maybe use assert instead?
    ge_frombytes_vartime(&A, reinterpret_cast<const unsigned char *>(&P));
    ge_scalarmult(&R, reinterpret_cast<const unsigned char *>(&a), &A);
    KeyImage aP;
    ge_tobytes(reinterpret_cast<unsigned char *>(&aP), &R);
    return aP;
}

void CryptoOps::hashDataToEC(const uint8_t *data, std::size_t length, PublicKey &publicKey)
{
    Hash h;
    ge_p2 point;
    ge_p1p1 point2;
    cn_fast_hash(data, length, h);
    ge_fromfe_frombytes_vartime(&point, reinterpret_cast<const unsigned char *>(&h));
    ge_mul8(&point2, &point);
    ge_p1p1_to_p2(&point, &point2);
    ge_tobytes(reinterpret_cast<unsigned char *>(&publicKey), &point);
}

void CryptoOps::generateKeyImage(const PublicKey &publicKey, const SecretKey &secretKey,
                                  KeyImage &keyImage)
{
    ge_p3 point;
    ge_p2 point2;
    assert(sc_check(reinterpret_cast<const unsigned char *>(&secretKey)) == 0);
    hashToEC(publicKey, point);
    ge_scalarmult(&point2, reinterpret_cast<const unsigned char *>(&secretKey), &point);
    ge_tobytes(reinterpret_cast<unsigned char *>(&keyImage), &point2);
}

void CryptoOps::generateIncompleteKeyImage(const PublicKey &publicKey,
                                            EllipticCurvePoint &incompleteKeyImage)
{
    ge_p3 point;
    hashToEC(publicKey, point);
    ge_p3_tobytes(reinterpret_cast<unsigned char *>(&incompleteKeyImage), &point);
}

#ifdef _MSC_VER
#pragma warning(disable : 4200)
#endif

struct rsComm {
    Hash h;
    struct {
        EllipticCurvePoint a, b;
    } ab[];
};

static inline size_t rsCommSize(size_t pubsCount)
{
    return sizeof(rsComm) + pubsCount * sizeof(((rsComm *)0)->ab[0]);
}

void CryptoOps::generateRingSignature(const Hash &prefixHash, const KeyImage &keyImage,
                                       const PublicKey *const *pPublicKey, size_t pubsCount,
                                       const SecretKey &secretKey, size_t secIndex,
                                       Signature *signature)
{
    size_t i;
    ge_p3 image_unp;
    ge_dsmp image_pre;
    EllipticCurveScalar sum, k, h;
    rsComm *const buf = reinterpret_cast<rsComm *>(alloca(rsCommSize(pubsCount)));
    assert(secIndex < pubsCount);
#if !defined(NDEBUG)
    {
        ge_p3 t;
        PublicKey t2;
        KeyImage t3;
        assert(sc_check(reinterpret_cast<const unsigned char *>(&secretKey)) == 0);
        ge_scalarmult_base(&t, reinterpret_cast<const unsigned char *>(&secretKey));
        ge_p3_tobytes(reinterpret_cast<unsigned char *>(&t2), &t);
        assert(*pPublicKey[secIndex] == t2);
        generateKeyImage(*pPublicKey[secIndex], secretKey, t3);
        assert(keyImage == t3);
        for (i = 0; i < pubsCount; i++) {
            assert(checkKey(*pPublicKey[i]));
        }
    }
#endif
    if (ge_frombytes_vartime(&image_unp, reinterpret_cast<const unsigned char *>(&keyImage)) != 0) {
        abort();
    }
    ge_dsm_precomp(image_pre, &image_unp);
    sc_0(reinterpret_cast<unsigned char *>(&sum));
    buf->h = prefixHash;
    for (i = 0; i < pubsCount; i++) {
        ge_p2 tmp2;
        ge_p3 tmp3;
        if (i == secIndex) {
            randomScalar(k);
            ge_scalarmult_base(&tmp3, reinterpret_cast<unsigned char *>(&k));
            ge_p3_tobytes(reinterpret_cast<unsigned char *>(&buf->ab[i].a), &tmp3);
            hashToEC(*pPublicKey[i], tmp3);
            ge_scalarmult(&tmp2, reinterpret_cast<unsigned char *>(&k), &tmp3);
            ge_tobytes(reinterpret_cast<unsigned char *>(&buf->ab[i].b), &tmp2);
        } else {
            randomScalar(reinterpret_cast<EllipticCurveScalar &>(signature[i]));
            randomScalar(*reinterpret_cast<EllipticCurveScalar *>(
                    reinterpret_cast<unsigned char *>(&signature[i]) + 32));
            if (ge_frombytes_vartime(&tmp3,
                                     reinterpret_cast<const unsigned char *>(&*pPublicKey[i]))
                != 0) {
                abort();
            }
            ge_double_scalarmult_base_vartime(
                    &tmp2, reinterpret_cast<unsigned char *>(&signature[i]), &tmp3,
                    reinterpret_cast<unsigned char *>(&signature[i]) + 32);
            ge_tobytes(reinterpret_cast<unsigned char *>(&buf->ab[i].a), &tmp2);
            hashToEC(*pPublicKey[i], tmp3);
            ge_double_scalarmult_precomp_vartime(
                    &tmp2, reinterpret_cast<unsigned char *>(&signature[i]) + 32, &tmp3,
                    reinterpret_cast<unsigned char *>(&signature[i]), image_pre);
            ge_tobytes(reinterpret_cast<unsigned char *>(&buf->ab[i].b), &tmp2);
            sc_add(reinterpret_cast<unsigned char *>(&sum), reinterpret_cast<unsigned char *>(&sum),
                   reinterpret_cast<unsigned char *>(&signature[i]));
        }
    }
    hashToScalar(buf, rsCommSize(pubsCount), h);
    sc_sub(reinterpret_cast<unsigned char *>(&signature[secIndex]),
           reinterpret_cast<unsigned char *>(&h), reinterpret_cast<unsigned char *>(&sum));
    sc_mulsub(reinterpret_cast<unsigned char *>(&signature[secIndex]) + 32,
              reinterpret_cast<unsigned char *>(&signature[secIndex]),
              reinterpret_cast<const unsigned char *>(&secretKey),
              reinterpret_cast<unsigned char *>(&k));
}

bool CryptoOps::checkRingSignature(const Hash &prefixHash, const KeyImage &keyImage,
                                    const PublicKey *const *pPublicKey, size_t pubsCount,
                                    const Signature *signature)
{
    size_t i;
    ge_p3 image_unp;
    ge_dsmp image_pre;
    EllipticCurveScalar sum, h;
    rsComm *const buf = reinterpret_cast<rsComm *>(alloca(rsCommSize(pubsCount)));
#if !defined(NDEBUG)
    for (i = 0; i < pubsCount; i++) {
        assert(checkKey(*pPublicKey[i]));
    }
#endif
    if (ge_frombytes_vartime(&image_unp, reinterpret_cast<const unsigned char *>(&keyImage)) != 0) {
        return false;
    }
    ge_dsm_precomp(image_pre, &image_unp);
    sc_0(reinterpret_cast<unsigned char *>(&sum));
    buf->h = prefixHash;
    for (i = 0; i < pubsCount; i++) {
        ge_p2 tmp2;
        ge_p3 tmp3;
        if (sc_check(reinterpret_cast<const unsigned char *>(&signature[i])) != 0
            || sc_check(reinterpret_cast<const unsigned char *>(&signature[i]) + 32) != 0) {
            return false;
        }
        if (ge_frombytes_vartime(&tmp3, reinterpret_cast<const unsigned char *>(&*pPublicKey[i]))
            != 0) {
            abort();
        }
        ge_double_scalarmult_base_vartime(
                &tmp2, reinterpret_cast<const unsigned char *>(&signature[i]), &tmp3,
                reinterpret_cast<const unsigned char *>(&signature[i]) + 32);
        ge_tobytes(reinterpret_cast<unsigned char *>(&buf->ab[i].a), &tmp2);
        hashToEC(*pPublicKey[i], tmp3);
        ge_double_scalarmult_precomp_vartime(
                &tmp2, reinterpret_cast<const unsigned char *>(&signature[i]) + 32, &tmp3,
                reinterpret_cast<const unsigned char *>(&signature[i]), image_pre);
        ge_tobytes(reinterpret_cast<unsigned char *>(&buf->ab[i].b), &tmp2);
        sc_add(reinterpret_cast<unsigned char *>(&sum), reinterpret_cast<unsigned char *>(&sum),
               reinterpret_cast<const unsigned char *>(&signature[i]));
    }
    hashToScalar(buf, rsCommSize(pubsCount), h);
    sc_sub(reinterpret_cast<unsigned char *>(&h), reinterpret_cast<unsigned char *>(&h),
           reinterpret_cast<unsigned char *>(&sum));
    return sc_isnonzero(reinterpret_cast<unsigned char *>(&h)) == 0;
}
}
