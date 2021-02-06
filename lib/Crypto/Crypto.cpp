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

#include <Common/Varint.h>

#include <Crypto/Crypto.h>
#include <Crypto/CryptoOps.h>
#include <Crypto/Hash.h>
#include <Crypto/Random.h>

namespace Crypto {

static inline unsigned char *operator&(FEllipticCurvePoint &point)
{
    return &reinterpret_cast<unsigned char &>(point);
}

static inline const unsigned char *operator&(const FEllipticCurvePoint &point)
{
    return &reinterpret_cast<const unsigned char &>(point);
}

static inline unsigned char *operator&(FEllipticCurveScalar &scalar)
{
    return &reinterpret_cast<unsigned char &>(scalar);
}

static inline const unsigned char *operator&(const FEllipticCurveScalar &scalar)
{
    return &reinterpret_cast<const unsigned char &>(scalar);
}

static inline void randomScalar(FEllipticCurveScalar &res)
{
    unsigned char tmp[64];
    Random::randomBytes(64, tmp);
    scReduce(tmp);
    memcpy(&res, tmp, 32);
}

void hashToScalar(const void *data,
                  size_t length,
				  FEllipticCurveScalar &res)
{
    cnFastHash(data, length, reinterpret_cast<FHash &>(res));
    scReduce32(reinterpret_cast<unsigned char *>(&res));
}

void CryptoOps::generateKeys(FPublicKey &pub,
							 FSecretKey &sec)
{
    GeP3 point;
    randomScalar(reinterpret_cast<FEllipticCurveScalar &>(sec));
    geScalarMultBase(&point, reinterpret_cast<unsigned char *>(&sec));
    geP3Tobytes(reinterpret_cast<unsigned char *>(&pub), &point);
}

void CryptoOps::generateDeterministicKeys(FPublicKey &pub,
										  FSecretKey &sec,
										  FSecretKey &second)
{
    GeP3 point;
    sec = second;
    scReduce32(reinterpret_cast<unsigned char *>(
            &sec)); // reduce in case second round of keys (sendkeys)
    geScalarMultBase(&point, reinterpret_cast<unsigned char *>(&sec));
    geP3Tobytes(reinterpret_cast<unsigned char *>(&pub), &point);
}

FSecretKey CryptoOps::generateMKeys(FPublicKey &pub,
									FSecretKey &sec,
									const FSecretKey &recovery_key,
									bool recover)
{
    GeP3 point;
    FSecretKey rng;

    if (recover) {
        rng = recovery_key;
    } else {
        randomScalar(reinterpret_cast<FEllipticCurveScalar &>(rng));
    }

    sec = rng;
    // reduce in case second round of keys (sendkeys)
    scReduce32(reinterpret_cast<unsigned char *>(&sec));
    geScalarMultBase(&point, reinterpret_cast<unsigned char *>(&sec));
    geP3Tobytes(reinterpret_cast<unsigned char *>(&pub), &point);

    return rng;
}

bool CryptoOps::checkKey(const FPublicKey &key)
{
    GeP3 point;

    return geFromBytesVarTime(&point, reinterpret_cast<const unsigned char *>(&key)) == 0;
}

bool CryptoOps::secretKeyToPublicKey(const FSecretKey &sec,
									 FPublicKey &pub)
{
    GeP3 point;

    if (scCheck(reinterpret_cast<const unsigned char *>(&sec)) != 0) {
        return false;
    }

    geScalarMultBase(&point, reinterpret_cast<const unsigned char *>(&sec));
    geP3Tobytes(reinterpret_cast<unsigned char *>(&pub), &point);

    return true;
}

bool CryptoOps::secretKeyMultPublicKey(const FSecretKey &sec,
									   const FPublicKey &pub,
									   FPublicKey &res)
{
    if (scCheck(&sec) != 0) {
        return false;
    }

    GeP3 point;
    if (geFromBytesVarTime(&point, &pub) != 0) {
        return false;
    }

    GeP2 point2;
    geScalarMult(&point2, &sec, &point);
    geToBytes(reinterpret_cast<unsigned char *>(&res), &point2);

    return true;
}

bool CryptoOps::generateKeyDerivation(const FPublicKey &key1,
									  const FSecretKey &key2,
									  FKeyDerivation &derivation)
{
    GeP3 point;
    GeP2 point2;
    GeP1P1 point3;
    assert(scCheck(reinterpret_cast<const unsigned char *>(&key2)) == 0);
    if (geFromBytesVarTime(&point, reinterpret_cast<const unsigned char *>(&key1)) != 0) {
        return false;
    }

    geScalarMult(&point2, reinterpret_cast<const unsigned char *>(&key2), &point);
    geMul8(&point3, &point2);
    geP1P1ToP2(&point2, &point3);
    geToBytes(reinterpret_cast<unsigned char *>(&derivation), &point2);

    return true;
}

static void derivationToScalar(const FKeyDerivation &derivation,
							   size_t outputIndex,
							   FEllipticCurveScalar &res)
{
    struct
    {
        FKeyDerivation derivation;
        char outputIndex[(sizeof(size_t) * 8 + 6) / 7];
    } Buf {};
    char *end = Buf.outputIndex;
    Buf.derivation = derivation;
    Tools::writeVarint(end, outputIndex);
    assert(end <= Buf.outputIndex + sizeof Buf.outputIndex);
    hashToScalar(&Buf, end - reinterpret_cast<char *>(&Buf), res);
}

static void derivationToScalar(const FKeyDerivation &derivation,
							   size_t outputIndex,
							   const uint8_t *suffix,
							   size_t suffixLength,
							   FEllipticCurveScalar &res)
{
    assert(suffixLength <= 32);
    struct
    {
        FKeyDerivation derivation;
        char outputIndex[(sizeof(size_t) * 8 + 6) / 7 + 32];
    } Buf {};
    char *end = Buf.outputIndex;
    Buf.derivation = derivation;
    Tools::writeVarint(end, outputIndex);
    assert(end <= Buf.outputIndex + sizeof Buf.outputIndex);
    size_t bufSize = end - reinterpret_cast<char *>(&Buf);
    memcpy(end, suffix, suffixLength);
    hashToScalar(&Buf, bufSize + suffixLength, res);
}

bool CryptoOps::derivePublicKey(const FKeyDerivation &derivation,
								size_t outputIndex,
								const FPublicKey &base,
								FPublicKey &derivedKey)
{
    FEllipticCurveScalar scalar{};
    GeP3 point1;
    GeP3 point2;
    GeCached point3;
    GeP1P1 point4;
    GeP2 point5;
    if (geFromBytesVarTime(&point1, reinterpret_cast<const unsigned char *>(&base)) != 0) {
        return false;
    }

    derivationToScalar(derivation, outputIndex, scalar);
    geScalarMultBase(&point2, reinterpret_cast<unsigned char *>(&scalar));
    geP3ToCached(&point3, &point2);
    geAdd(&point4, &point1, &point3);
    geP1P1ToP2(&point5, &point4);
    geToBytes(reinterpret_cast<unsigned char *>(&derivedKey), &point5);

    return true;
}

bool CryptoOps::derivePublicKey(const FKeyDerivation &derivation,
								size_t outputIndex,
								const FPublicKey &base,
								const uint8_t *suffix,
								size_t suffixLength,
								FPublicKey &derivedKey)
{
    FEllipticCurveScalar scalar {};
    GeP3 point1;
    GeP3 point2;
    GeCached point3;
    GeP1P1 point4;
    GeP2 point5;
    if (geFromBytesVarTime(&point1, reinterpret_cast<const unsigned char *>(&base)) != 0) {
        return false;
    }

    derivationToScalar(derivation, outputIndex, suffix, suffixLength, scalar);
    geScalarMultBase(&point2, reinterpret_cast<unsigned char *>(&scalar));
    geP3ToCached(&point3, &point2);
    geAdd(&point4, &point1, &point3);
    geP1P1ToP2(&point5, &point4);
    geToBytes(reinterpret_cast<unsigned char *>(&derivedKey), &point5);

    return true;
}

bool CryptoOps::underivePublicKeyAndGetScalar(const FKeyDerivation &derivation,
											  size_t outputIndex,
											  const FPublicKey &derivedKey,
											  FPublicKey &base,
											  FEllipticCurveScalar &hashedDerivation)
{
    GeP3 point1;
    GeP3 point2;
    GeCached point3;
    GeP1P1 point4;
    GeP2 point5;
    if (geFromBytesVarTime(&point1, reinterpret_cast<const unsigned char *>(&derivedKey)) != 0) {
        return false;
    }

    derivationToScalar(derivation, outputIndex, hashedDerivation);
    geScalarMultBase(&point2, reinterpret_cast<unsigned char *>(&hashedDerivation));
    geP3ToCached(&point3, &point2);
    geSub(&point4, &point1, &point3);
    geP1P1ToP2(&point5, &point4);
    geToBytes(reinterpret_cast<unsigned char *>(&base), &point5);

    return true;
}

void CryptoOps::deriveSecretKey(const FKeyDerivation &derivation,
								size_t outputIndex,
								const FSecretKey &base,
								FSecretKey &derivedKey)
{
    FEllipticCurveScalar scalar {};
    assert(scCheck(reinterpret_cast<const unsigned char *>(&base)) == 0);
    derivationToScalar(derivation, outputIndex, scalar);
    scAdd(reinterpret_cast<unsigned char *>(&derivedKey),
          reinterpret_cast<const unsigned char *>(&base),
          reinterpret_cast<unsigned char *>(&scalar));
}

void CryptoOps::deriveSecretKey(const FKeyDerivation &keyDerivation,
								size_t outputIndex,
								const FSecretKey &baseKey,
								const uint8_t *suffix,
								size_t suffixLength,
								FSecretKey &derivedKey)
{
    FEllipticCurveScalar scalar {};
    assert(scCheck(reinterpret_cast<const unsigned char *>(&baseKey)) == 0);
    derivationToScalar(keyDerivation, outputIndex, suffix, suffixLength, scalar);
    scAdd(reinterpret_cast<unsigned char *>(&derivedKey),
          reinterpret_cast<const unsigned char *>(&baseKey),
          reinterpret_cast<unsigned char *>(&scalar));
}

bool CryptoOps::underivePublicKey(const FKeyDerivation &keyDerivation,
								  size_t outputIndex,
								  const FPublicKey &derivedKey,
								  FPublicKey &baseKey)
{
    FEllipticCurveScalar scalar {};
    GeP3 point1;
    GeP3 point2;
    GeCached point3;
    GeP1P1 point4;
    GeP2 point5;
    if (geFromBytesVarTime(&point1, reinterpret_cast<const unsigned char *>(&derivedKey)) != 0) {
        return false;
    }

    derivationToScalar(keyDerivation, outputIndex, scalar);
    geScalarMultBase(&point2, reinterpret_cast<unsigned char *>(&scalar));
    geP3ToCached(&point3, &point2);
    geSub(&point4, &point1, &point3);
    geP1P1ToP2(&point5, &point4);
    geToBytes(reinterpret_cast<unsigned char *>(&baseKey), &point5);

    return true;
}

bool CryptoOps::underivePublicKey(const FKeyDerivation &keyDerivation,
								  size_t outputIndex,
								  const FPublicKey &derivedKey,
								  const uint8_t *suffix,
								  size_t suffixLength,
								  FPublicKey &baseKey)
{
    FEllipticCurveScalar scalar {};
    GeP3 point1;
    GeP3 point2;
    GeCached point3;
    GeP1P1 point4;
    GeP2 point5;
    if (geFromBytesVarTime(&point1, reinterpret_cast<const unsigned char *>(&derivedKey)) != 0) {
        return false;
    }

    derivationToScalar(keyDerivation, outputIndex, suffix, suffixLength, scalar);
    geScalarMultBase(&point2, reinterpret_cast<unsigned char *>(&scalar));
    geP3ToCached(&point3, &point2);
    geSub(&point4, &point1, &point3);
    geP1P1ToP2(&point5, &point4);
    geToBytes(reinterpret_cast<unsigned char *>(&baseKey), &point5);

    return true;
}

struct SComm
{
    FHash h;
    FEllipticCurvePoint key;
    FEllipticCurvePoint comm;
};

struct SComm2
{
    FHash msg;
    FEllipticCurvePoint D;
    FEllipticCurvePoint X;
    FEllipticCurvePoint Y;
};

void CryptoOps::generateSignature(const FHash &prefixHash,
								  const FPublicKey &publicKey,
								  const FSecretKey &secretKey,
								  FSignature &signature)
{
    GeP3 tmp3;
    FEllipticCurveScalar k {};
    SComm buf {};
#if !defined(NDEBUG)
    {
        GeP3 t;
        FPublicKey t2;
        assert(scCheck(reinterpret_cast<const unsigned char *>(&secretKey)) == 0);
        geScalarMultBase(&t, reinterpret_cast<const unsigned char *>(&secretKey));
        geP3Tobytes(reinterpret_cast<unsigned char *>(&t2), &t);
        assert(publicKey == t2);
    }
#endif
    buf.h = prefixHash;
    buf.key = reinterpret_cast<const FEllipticCurvePoint &>(publicKey);
tryAgain:
    randomScalar(k);
    if (((const uint32_t *)(&k))[7] == 0) { // we don't want tiny numbers here
        goto tryAgain;
    }
    geScalarMultBase(&tmp3, reinterpret_cast<unsigned char *>(&k));
    geP3Tobytes(reinterpret_cast<unsigned char *>(&buf.comm), &tmp3);
    hashToScalar(&buf, sizeof(SComm), reinterpret_cast<FEllipticCurveScalar &>(signature));
    if (!scIsNonZero(
                (const unsigned char *)reinterpret_cast<FEllipticCurveScalar &>(signature).uData)) {
        goto tryAgain;
    }
    scMulSub(reinterpret_cast<unsigned char *>(&signature) + 32,
             reinterpret_cast<unsigned char *>(&signature),
             reinterpret_cast<const unsigned char *>(&secretKey),
             reinterpret_cast<unsigned char *>(&k));
    if (!scIsNonZero((const unsigned char *)reinterpret_cast<unsigned char *>(&signature) + 32)) {
        goto tryAgain;
    }
}

bool CryptoOps::checkSignature(const FHash &prefixHash,
                               const FPublicKey &publicKey,
                               const FSignature &signature)
{
    GeP2 tmp2;
    GeP3 tmp3;
    FEllipticCurveScalar c {};
    SComm buf {};
    assert(checkKey(publicKey));
    buf.h = prefixHash;
    buf.key = reinterpret_cast<const FEllipticCurvePoint &>(publicKey);
    if (geFromBytesVarTime(&tmp3, reinterpret_cast<const unsigned char *>(&publicKey)) != 0) {
        abort();
    }

    if (scCheck(reinterpret_cast<const unsigned char *>(&signature)) != 0
        || scCheck(reinterpret_cast<const unsigned char *>(&signature) + 32) != 0
        || !scIsNonZero(reinterpret_cast<const unsigned char *>(&signature))) {
        return false;
    }
    geDoubleScalarmultBaseVartime(&tmp2, reinterpret_cast<const unsigned char *>(&signature), &tmp3,
                                  reinterpret_cast<const unsigned char *>(&signature) + 32);
    geToBytes(reinterpret_cast<unsigned char *>(&buf.comm), &tmp2);
    static const FEllipticCurvePoint infinity =
    {
        {
            1, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0
        }
    };

    if (memcmp(&buf.comm, &infinity, 32) == 0) {
        return false;
    }
    hashToScalar(&buf, sizeof(SComm), c);
    scSub(reinterpret_cast<unsigned char *>(&c), reinterpret_cast<unsigned char *>(&c),
          reinterpret_cast<const unsigned char *>(&signature));

    return scIsNonZero(reinterpret_cast<unsigned char *>(&c)) == 0;
}

void CryptoOps::generateTxProof(const FHash &prefixHash,
								const FPublicKey &R,
								const FPublicKey &A,
								const FPublicKey &D,
								const FSecretKey &r,
								FSignature &signature)
{
    // sanity check
    GeP3 R_p3;
    GeP3 A_p3;
    GeP3 D_p3;
    if (geFromBytesVarTime(&R_p3, reinterpret_cast<const unsigned char *>(&R)) != 0) {
        throw std::runtime_error("tx pubkey is invalid");
    }

    if (geFromBytesVarTime(&A_p3, reinterpret_cast<const unsigned char *>(&A)) != 0) {
        throw std::runtime_error("recipient view pubkey is invalid");
    }

    if (geFromBytesVarTime(&D_p3, reinterpret_cast<const unsigned char *>(&D)) != 0) {
        throw std::runtime_error("key derivation is invalid");
    }

    assert(scCheck(reinterpret_cast<const unsigned char *>(&r)) == 0);
    // check R == r*G
    GeP3 dbg_R_p3;
    geScalarMultBase(&dbg_R_p3, reinterpret_cast<const unsigned char *>(&r));
    FPublicKey dbg_R;
    geP3Tobytes(reinterpret_cast<unsigned char *>(&dbg_R), &dbg_R_p3);
    assert(R == dbg_R);
    // check D == r*A
    GeP2 dbg_D_p2;
    geScalarMult(&dbg_D_p2, reinterpret_cast<const unsigned char *>(&r), &A_p3);
    FPublicKey dbg_D;
    geToBytes(reinterpret_cast<unsigned char *>(&dbg_D), &dbg_D_p2);
    assert(D == dbg_D);

    // pick random k
    FEllipticCurveScalar k {};
    randomScalar(k);

    // compute X = k*G
    GeP3 X_p3;
    geScalarMultBase(&X_p3, reinterpret_cast<unsigned char *>(&k));

    // compute Y = k*A
    GeP2 Y_p2;
    geScalarMult(&Y_p2, reinterpret_cast<unsigned char *>(&k), &A_p3);

    // signature.c = Hs(Msg || D || X || Y)
    SComm2 buf {};
    buf.msg = prefixHash;
    buf.D = reinterpret_cast<const FEllipticCurvePoint &>(D);
    geP3Tobytes(reinterpret_cast<unsigned char *>(&buf.X), &X_p3);
    geToBytes(reinterpret_cast<unsigned char *>(&buf.Y), &Y_p2);
    hashToScalar(&buf, sizeof(SComm2), reinterpret_cast<FEllipticCurveScalar &>(signature));

    // signature.r = k - signature.c*r
    scMulSub(reinterpret_cast<unsigned char *>(&signature) + 32,
             reinterpret_cast<unsigned char *>(&signature),
             reinterpret_cast<const unsigned char *>(&r), reinterpret_cast<unsigned char *>(&k));
}

bool CryptoOps::checkTxProof(const FHash &prefixHash,
                             const FPublicKey &R,
                             const FPublicKey &A,
                             const FPublicKey &D,
                             const FSignature &signature)
{
    // sanity check
    GeP3 R_p3;
    GeP3 A_p3;
    GeP3 D_p3;
    if (geFromBytesVarTime(&R_p3, reinterpret_cast<const unsigned char *>(&R)) != 0) {
        return false;
    }

    if (geFromBytesVarTime(&A_p3, reinterpret_cast<const unsigned char *>(&A)) != 0) {
        return false;
    }

    if (geFromBytesVarTime(&D_p3, reinterpret_cast<const unsigned char *>(&D)) != 0) {
        return false;
    }

    if (scCheck(reinterpret_cast<const unsigned char *>(&signature)) != 0
        || scCheck(reinterpret_cast<const unsigned char *>(&signature) + 32) != 0) {
        return false;
    }

    // compute signature.c*R
    GeP2 cR_p2;
    geScalarMult(&cR_p2, reinterpret_cast<const unsigned char *>(&signature), &R_p3);

    // compute signature.r*G
    GeP3 rG_p3;
    geScalarMultBase(&rG_p3, reinterpret_cast<const unsigned char *>(&signature) + 32);

    // compute signature.c*D
    GeP2 cD_p2;
    geScalarMult(&cD_p2, reinterpret_cast<const unsigned char *>(&signature), &D_p3);

    // compute signature.r*A
    GeP2 rA_p2;
    geScalarMult(&rA_p2, reinterpret_cast<const unsigned char *>(&signature) + 32, &A_p3);

    // compute X = signature.c*R + signature.r*G
    FPublicKey cR;
    geToBytes(reinterpret_cast<unsigned char *>(&cR), &cR_p2);
    GeP3 cR_p3;
    if (geFromBytesVarTime(&cR_p3, reinterpret_cast<const unsigned char *>(&cR)) != 0) {
        return false;
    }
    GeCached rG_cached;
    geP3ToCached(&rG_cached, &rG_p3);
    GeP1P1 X_p1p1;
    geAdd(&X_p1p1, &cR_p3, &rG_cached);
    GeP2 X_p2;
    geP1P1ToP2(&X_p2, &X_p1p1);

    // compute Y = signature.c*D + signature.r*A
    FPublicKey cD;
    FPublicKey rA;
    geToBytes(reinterpret_cast<unsigned char *>(&cD), &cD_p2);
    geToBytes(reinterpret_cast<unsigned char *>(&rA), &rA_p2);
    GeP3 cD_p3;
    GeP3 rA_p3;
    if (geFromBytesVarTime(&cD_p3, reinterpret_cast<const unsigned char *>(&cD)) != 0) {
        return false;
    }

    if (geFromBytesVarTime(&rA_p3, reinterpret_cast<const unsigned char *>(&rA)) != 0) {
        return false;
    }

    GeCached rA_cached;
    geP3ToCached(&rA_cached, &rA_p3);
    GeP1P1 Y_p1p1;
    geAdd(&Y_p1p1, &cD_p3, &rA_cached);
    GeP2 Y_p2;
    geP1P1ToP2(&Y_p2, &Y_p1p1);

    // compute c2 = Hs(Msg || D || X || Y)
    SComm2 buf {};
    buf.msg = prefixHash;
    buf.D = reinterpret_cast<const FEllipticCurvePoint &>(D);
    geToBytes(&buf.X, &X_p2);
    geToBytes(&buf.Y, &Y_p2);
    FEllipticCurveScalar c2 {};
    hashToScalar(&buf, sizeof(SComm2), c2);

    // test if c2 == signature.c
    scSub(reinterpret_cast<unsigned char *>(&c2), reinterpret_cast<unsigned char *>(&c2),
          reinterpret_cast<const unsigned char *>(&signature));

    return scIsNonZero(&c2) == 0;
}

static void hashToEC(const FPublicKey &key, GeP3 &res)
{
    FHash h {};
    GeP2 point;
    GeP1P1 point2;
    cnFastHash(std::addressof(key), sizeof(FPublicKey), h);
    geFromFeFromBytesVarTime(&point, reinterpret_cast<const unsigned char *>(&h));
    geMul8(&point2, &point);
    geP1P1ToP3(&res, &point2);
}

FKeyImage CryptoOps::scalarMultKey(const FKeyImage &P, const FKeyImage &a)
{
    GeP3 A;
    GeP2 R;
    // maybe use assert instead?
    geFromBytesVarTime(&A, reinterpret_cast<const unsigned char *>(&P));
    geScalarMult(&R, reinterpret_cast<const unsigned char *>(&a), &A);
    FKeyImage aP;
    geToBytes(reinterpret_cast<unsigned char *>(&aP), &R);

    return aP;
}

void CryptoOps::hashDataToEC(const uint8_t *data, std::size_t length, FPublicKey &publicKey)
{
    FHash h {};
    GeP2 point;
    GeP1P1 point2;
    cnFastHash(data, length, h);
    geFromFeFromBytesVarTime(&point, reinterpret_cast<const unsigned char *>(&h));
    geMul8(&point2, &point);
    geP1P1ToP2(&point, &point2);
    geToBytes(reinterpret_cast<unsigned char *>(&publicKey), &point);
}

void CryptoOps::generateKeyImage(const FPublicKey &publicKey, const FSecretKey &secretKey,
								 FKeyImage &keyImage)
{
    GeP3 point;
    GeP2 point2;
    assert(scCheck(reinterpret_cast<const unsigned char *>(&secretKey)) == 0);
    hashToEC(publicKey, point);
    geScalarMult(&point2, reinterpret_cast<const unsigned char *>(&secretKey), &point);
    geToBytes(reinterpret_cast<unsigned char *>(&keyImage), &point2);
}

void CryptoOps::generateIncompleteKeyImage(const FPublicKey &publicKey,
										   FEllipticCurvePoint &incompleteKeyImage)
{
    GeP3 point;
    hashToEC(publicKey, point);
    geP3Tobytes(reinterpret_cast<unsigned char *>(&incompleteKeyImage), &point);
}

#ifdef _MSC_VER
#pragma warning(disable : 4200)
#endif

struct RsComm
{
    FHash h;
    struct
    {
        FEllipticCurvePoint a;
        FEllipticCurvePoint b;
    } Ab[];
};

static inline size_t rsCommSize(size_t pubsCount)
{
    return sizeof(RsComm) + pubsCount * sizeof(((RsComm *)0)->Ab[0]);
}

void CryptoOps::generateRingSignature(const FHash &prefixHash,
									  const FKeyImage &keyImage,
									  const FPublicKey *const *pPublicKey,
									  size_t pubsCount,
									  const FSecretKey &secretKey,
									  size_t secIndex,
									  FSignature *signature)
{
    size_t i;
    GeP3 image_unp;
    geDsmp image_pre;
    FEllipticCurveScalar sum {};
    FEllipticCurveScalar k {};
    FEllipticCurveScalar h {};
    RsComm *const buf = reinterpret_cast<RsComm *>(alloca(rsCommSize(pubsCount)));
    assert(secIndex < pubsCount);
#if !defined(NDEBUG)
    {
        GeP3 t;
        FPublicKey t2;
        FKeyImage t3;
        assert(scCheck(reinterpret_cast<const unsigned char *>(&secretKey)) == 0);
        geScalarMultBase(&t, reinterpret_cast<const unsigned char *>(&secretKey));
        geP3Tobytes(reinterpret_cast<unsigned char *>(&t2), &t);
        assert(*pPublicKey[secIndex] == t2);
        generateKeyImage(*pPublicKey[secIndex], secretKey, t3);
        assert(keyImage == t3);
        for (i = 0; i < pubsCount; i++) {
            assert(checkKey(*pPublicKey[i]));
        }
    }
#endif
    if (geFromBytesVarTime(&image_unp, reinterpret_cast<const unsigned char *>(&keyImage)) != 0) {
        abort();
    }

    geDsmPrecomp(image_pre, &image_unp);
    sc0(reinterpret_cast<unsigned char *>(&sum));
    buf->h = prefixHash;
    for (i = 0; i < pubsCount; i++) {
        GeP2 tmp2;
        GeP3 tmp3;
        if (i == secIndex) {
            randomScalar(k);
            geScalarMultBase(&tmp3, reinterpret_cast<unsigned char *>(&k));
            geP3Tobytes(reinterpret_cast<unsigned char *>(&buf->Ab[i].a), &tmp3);
            hashToEC(*pPublicKey[i], tmp3);
            geScalarMult(&tmp2, reinterpret_cast<unsigned char *>(&k), &tmp3);
            geToBytes(reinterpret_cast<unsigned char *>(&buf->Ab[i].b), &tmp2);
        } else {
            randomScalar(reinterpret_cast<FEllipticCurveScalar &>(signature[i]));
            randomScalar(*reinterpret_cast<FEllipticCurveScalar *>(
                    reinterpret_cast<unsigned char *>(&signature[i]) + 32));
            if (geFromBytesVarTime(&tmp3, reinterpret_cast<const unsigned char *>(&*pPublicKey[i]))
                != 0) {
                abort();
            }
            geDoubleScalarmultBaseVartime(&tmp2, reinterpret_cast<unsigned char *>(&signature[i]),
                                          &tmp3,
                                          reinterpret_cast<unsigned char *>(&signature[i]) + 32);
            geToBytes(reinterpret_cast<unsigned char *>(&buf->Ab[i].a), &tmp2);
            hashToEC(*pPublicKey[i], tmp3);
            geDoubleScalarMultPrecompVarTime(
                    &tmp2, reinterpret_cast<unsigned char *>(&signature[i]) + 32, &tmp3,
                    reinterpret_cast<unsigned char *>(&signature[i]), image_pre);
            geToBytes(reinterpret_cast<unsigned char *>(&buf->Ab[i].b), &tmp2);
            scAdd(reinterpret_cast<unsigned char *>(&sum), reinterpret_cast<unsigned char *>(&sum),
                  reinterpret_cast<unsigned char *>(&signature[i]));
        }
    }

    hashToScalar(buf, rsCommSize(pubsCount), h);
    scSub(reinterpret_cast<unsigned char *>(&signature[secIndex]),
          reinterpret_cast<unsigned char *>(&h), reinterpret_cast<unsigned char *>(&sum));
    scMulSub(reinterpret_cast<unsigned char *>(&signature[secIndex]) + 32,
             reinterpret_cast<unsigned char *>(&signature[secIndex]),
             reinterpret_cast<const unsigned char *>(&secretKey),
             reinterpret_cast<unsigned char *>(&k));
}

bool CryptoOps::checkRingSignature(const FHash &prefixHash,
                                   const FKeyImage &keyImage,
                                   const FPublicKey *const *pPublicKey,
                                   size_t pubsCount,
                                   const FSignature *signature)
{
    size_t i;
    GeP3 image_unp;
    geDsmp image_pre;
    FEllipticCurveScalar sum {};
    FEllipticCurveScalar h {};
    auto *const buf = reinterpret_cast<RsComm *>(alloca(rsCommSize(pubsCount)));
#if !defined(NDEBUG)
    for (i = 0; i < pubsCount; i++) {
        assert(checkKey(*pPublicKey[i]));
    }
#endif
    if (geFromBytesVarTime(&image_unp, reinterpret_cast<const unsigned char *>(&keyImage)) != 0) {
        return false;
    }
    geDsmPrecomp(image_pre, &image_unp);
    sc0(reinterpret_cast<unsigned char *>(&sum));
    buf->h = prefixHash;
    for (i = 0; i < pubsCount; i++) {
        GeP2 tmp2;
        GeP3 tmp3;
        if (scCheck(reinterpret_cast<const unsigned char *>(&signature[i])) != 0
            || scCheck(reinterpret_cast<const unsigned char *>(&signature[i]) + 32) != 0) {
            return false;
        }

        if (geFromBytesVarTime(&tmp3, reinterpret_cast<const unsigned char *>(&*pPublicKey[i]))
            != 0) {
            abort();
        }
        geDoubleScalarmultBaseVartime(&tmp2, reinterpret_cast<const unsigned char *>(&signature[i]),
                                      &tmp3,
                                      reinterpret_cast<const unsigned char *>(&signature[i]) + 32);
        geToBytes(reinterpret_cast<unsigned char *>(&buf->Ab[i].a), &tmp2);
        hashToEC(*pPublicKey[i], tmp3);
        geDoubleScalarMultPrecompVarTime(
                &tmp2, reinterpret_cast<const unsigned char *>(&signature[i]) + 32, &tmp3,
                reinterpret_cast<const unsigned char *>(&signature[i]), image_pre);
        geToBytes(reinterpret_cast<unsigned char *>(&buf->Ab[i].b), &tmp2);
        scAdd(reinterpret_cast<unsigned char *>(&sum), reinterpret_cast<unsigned char *>(&sum),
              reinterpret_cast<const unsigned char *>(&signature[i]));
    }
    hashToScalar(buf, rsCommSize(pubsCount), h);
    scSub(reinterpret_cast<unsigned char *>(&h), reinterpret_cast<unsigned char *>(&h),
          reinterpret_cast<unsigned char *>(&sum));

    return scIsNonZero(reinterpret_cast<unsigned char *>(&h)) == 0;
}
}
