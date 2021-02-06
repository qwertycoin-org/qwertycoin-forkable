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

#pragma once

#include <cstddef>
#include <limits>
#include <mutex>
#include <type_traits>
#include <vector>

#include <CryptoTypes.h>

#include <Crypto/CryptoOps.h>
#include <Crypto/CryptoUtil.h>
#include <Crypto/GenericOps.h>
#include <Crypto/Hash.h>
#include <Crypto/Random.h>

namespace Crypto {

class CryptoOps
{
    CryptoOps();
    CryptoOps(const CryptoOps &);
    void operator=(const CryptoOps &);
    ~CryptoOps();

    static void generateKeys(FPublicKey &, FSecretKey &);
    friend void generateKeys(FPublicKey &, FSecretKey &);
    static void generateDeterministicKeys(FPublicKey &pub,
										  FSecretKey &sec,
										  FSecretKey &second);
    friend void generateDeterministicKeys(FPublicKey &pub,
										  FSecretKey &sec,
										  FSecretKey &second);
    static FSecretKey generateMKeys(FPublicKey &pub,
									FSecretKey &sec,
									const FSecretKey &recovery_key = FSecretKey(),
									bool recover = false);
    friend FSecretKey generateMKeys(FPublicKey &pub,
									FSecretKey &sec,
									const FSecretKey &recovery_key,
									bool recover);
    static bool checkKey(const FPublicKey &);
    friend bool checkKey(const FPublicKey &);
    static bool secretKeyToPublicKey(const FSecretKey &, FPublicKey &);
    friend bool secretKeyToPublicKey(const FSecretKey &, FPublicKey &);
    static bool secretKeyMultPublicKey(const FSecretKey &,
                                       const FPublicKey &,
                                       FPublicKey &);
    friend bool secretKeyMultPublicKey(const FSecretKey &,
                                       const FPublicKey &,
                                       FPublicKey &);
    static bool generateKeyDerivation(const FPublicKey &key1,
									  const FSecretKey &key2,
									  FKeyDerivation &derivation);
    friend bool generateKeyDerivation(const FPublicKey &,
									  const FSecretKey &,
									  FKeyDerivation &);
    static bool derivePublicKey(const FKeyDerivation &,
                                size_t,
                                const FPublicKey &,
                                FPublicKey &);
    friend bool derivePublicKey(const FKeyDerivation &,
                                size_t,
                                const FPublicKey &,
                                FPublicKey &);
    friend bool derivePublicKey(const FKeyDerivation &,
                                size_t,
                                const FPublicKey &,
                                const uint8_t *prefix,
                                size_t, FPublicKey &);
    static bool derivePublicKey(const FKeyDerivation &,
                                size_t,
                                const FPublicKey &,
                                const uint8_t *,
                                size_t, FPublicKey &);
    // hack for pg
    static bool underivePublicKeyAndGetScalar(const FKeyDerivation &,
                                              std::size_t,
                                              const FPublicKey &,
                                              FPublicKey &,
                                              FEllipticCurveScalar &);
    friend bool underivePublicKeyAndGetScalar(const FKeyDerivation &,
                                              std::size_t,
                                              const FPublicKey &,
                                              FPublicKey &,
                                              FEllipticCurveScalar &);
    static void generateIncompleteKeyImage(const FPublicKey &, FEllipticCurvePoint &);
    friend void generateIncompleteKeyImage(const FPublicKey &, FEllipticCurvePoint &);
    static void deriveSecretKey(const FKeyDerivation &,
								size_t, const FSecretKey &,
								FSecretKey &);
    friend void deriveSecretKey(const FKeyDerivation &,
								size_t,
								const FSecretKey &,
								FSecretKey &);
    static void deriveSecretKey(const FKeyDerivation &,
								size_t,
								const FSecretKey &,
								const uint8_t *,
								size_t, FSecretKey &);
    friend void deriveSecretKey(const FKeyDerivation &,
								size_t,
								const FSecretKey &,
								const uint8_t *,
								size_t, FSecretKey &);
    static bool underivePublicKey(const FKeyDerivation &,
                                  size_t,
                                  const FPublicKey &,
                                  FPublicKey &);
    friend bool underivePublicKey(const FKeyDerivation &,
                                  size_t,
                                  const FPublicKey &,
                                  FPublicKey &);
    static bool underivePublicKey(const FKeyDerivation &,
                                  size_t,
                                  const FPublicKey &,
                                  const uint8_t *,
                                  size_t, FPublicKey &);
    friend bool underivePublicKey(const FKeyDerivation &,
                                  size_t,
                                  const FPublicKey &,
                                  const uint8_t *,
                                  size_t, FPublicKey &);
    static void generateSignature(const FHash &,
								  const FPublicKey &,
								  const FSecretKey &,
								  FSignature &);
    friend void generateSignature(const FHash &,
								  const FPublicKey &,
								  const FSecretKey &,
								  FSignature &);
    static bool checkSignature(const FHash &,
                               const FPublicKey &,
                               const FSignature &);
    friend bool checkSignature(const FHash &,
                               const FPublicKey &,
                               const FSignature &);
    static void generateTxProof(const FHash &,
								const FPublicKey &,
								const FPublicKey &,
								const FPublicKey &,
								const FSecretKey &,
								FSignature &);
    friend void generateTxProof(const FHash &,
								const FPublicKey &,
								const FPublicKey &,
								const FPublicKey &,
								const FSecretKey &,
								FSignature &);
    static bool checkTxProof(const FHash &,
                             const FPublicKey &,
                             const FPublicKey &,
                             const FPublicKey &,
                             const FSignature &);
    friend bool checkTxProof(const FHash &,
                             const FPublicKey &,
                             const FPublicKey &,
                             const FPublicKey &,
                             const FSignature &);
    static void generateKeyImage(const FPublicKey &,
								 const FSecretKey &,
								 FKeyImage &);
    friend void generateKeyImage(const FPublicKey &,
								 const FSecretKey &,
								 FKeyImage &);
    static FKeyImage scalarMultKey(const FKeyImage &P, const FKeyImage &a);
    friend FKeyImage scalarMultKey(const FKeyImage &P, const FKeyImage &a);
    static void hashDataToEC(const uint8_t *,
                             std::size_t,
                             FPublicKey &);
    friend void hashDataToEC(const uint8_t *,
                             std::size_t,
                             FPublicKey &);
    static void generateRingSignature(const FHash &,
									  const FKeyImage &,
									  const FPublicKey *const *,
									  size_t,
									  const FSecretKey &,
									  size_t, FSignature *);
    friend void generateRingSignature(const FHash &,
									  const FKeyImage &,
									  const FPublicKey *const *,
									  size_t, const FSecretKey &,
									  size_t, FSignature *);
    static bool checkRingSignature(const FHash &,
                                   const FKeyImage &,
                                   const FPublicKey *const *,
                                   size_t,
                                   const FSignature *);
    friend bool checkRingSignature(const FHash &,
                                   const FKeyImage &,
                                   const FPublicKey *const *,
                                   size_t,
                                   const FSignature *);
};

void hashToScalar(const void *data,
				  size_t length,
				  FEllipticCurveScalar &res);

/* Generate a new key pair
 */
inline void generateKeys(FPublicKey &publicKey, FSecretKey &secretKey)
{
    CryptoOps::generateKeys(publicKey, secretKey);
}

inline void generateDeterministicKeys(FPublicKey &pub,
									  FSecretKey &sec,
									  FSecretKey &second)
{
    CryptoOps::generateDeterministicKeys(pub, sec, second);
}

inline FSecretKey generateMKeys(FPublicKey &pub,
								FSecretKey &sec,
								const FSecretKey &recovery_key = FSecretKey(),
								bool recover = false)
{
    return CryptoOps::generateMKeys(pub, sec, recovery_key, recover);
}

/* Check a public key. Returns true if it is valid, false otherwise.
 */
inline bool checkKey(const FPublicKey &publicKey)
{
    return CryptoOps::checkKey(publicKey);
}

/* Checks a private key and computes the corresponding public key.
 */
inline bool secretKeyToPublicKey(const FSecretKey &secretKey, FPublicKey &publicKey)
{
    return CryptoOps::secretKeyToPublicKey(secretKey, publicKey);
}

/* Multiply secret key to public key
 */
inline bool secretKeyMultPublicKey(const FSecretKey &sec,
                                   const FPublicKey &pub,
                                   FPublicKey &res)
{
    return CryptoOps::secretKeyMultPublicKey(sec, pub, res);
}

/* To generate an ephemeral key used to send money to:
 * * The sender generates a new key pair, which becomes the transaction key. The public transaction
 * key is included in "extra" field.
 * * Both the sender and the receiver generate key derivation from the transaction key and the
 * receivers' "view" key.
 * * The sender uses key derivation, the output index, and the receivers' "spend" key to derive an
 * ephemeral public key.
 * * The receiver can either derive the public key (to check that the transaction is addressed to
 * him) or the private key (to spend the money).
 */
inline bool generateKeyDerivation(const FPublicKey &key1,
								  const FSecretKey &key2,
								  FKeyDerivation &derivation)
{
    return CryptoOps::generateKeyDerivation(key1, key2, derivation);
}

inline bool derivePublicKey(const FKeyDerivation &derivation,
                            size_t outputIndex,
                            const FPublicKey &base,
                            const uint8_t *prefix,
                            size_t prefixLength,
                            FPublicKey &derivedKey)
{
    return CryptoOps::derivePublicKey(derivation,
                                      outputIndex,
                                      base,
                                      prefix,
                                      prefixLength,
                                      derivedKey);
}

inline bool derivePublicKey(const FKeyDerivation &derivation,
                            size_t outputIndex,
                            const FPublicKey &base,
                            FPublicKey &derivedKey)
{
    return CryptoOps::derivePublicKey(derivation, outputIndex, base, derivedKey);
}

inline bool underivePublicKeyAndGetScalar(const FKeyDerivation &keyDerivation,
                                          std::size_t outputIndex,
                                          const FPublicKey &derivedKey,
                                          FPublicKey &baseKey,
                                          FEllipticCurveScalar &hashedDerivation)
{
    return CryptoOps::underivePublicKeyAndGetScalar(keyDerivation,
                                                    outputIndex,
                                                    derivedKey,
                                                    baseKey,
                                                    hashedDerivation);
}

inline void deriveSecretKey(const FKeyDerivation &keyDerivation,
							std::size_t outputIndex,
							const FSecretKey &baseKey,
							const uint8_t *prefix,
							size_t prefixLength,
							FSecretKey &derivedKey)
{
    CryptoOps::deriveSecretKey(keyDerivation,
                               outputIndex,
                               baseKey,
                               prefix,
                               prefixLength,
                               derivedKey);
}

inline void deriveSecretKey(const FKeyDerivation &keyDerivation,
							std::size_t outputIndex,
							const FSecretKey &baseKey,
							FSecretKey &derivedKey)
{
    CryptoOps::deriveSecretKey(keyDerivation, outputIndex, baseKey, derivedKey);
}

/* Inverse function of derivePublicKey. It can be used by the receiver to find which "spend" key was
 * used to generate a transaction. This may be useful if the receiver used multiple addresses which
 * only differ in "spend" key.
 */
inline bool underivePublicKey(const FKeyDerivation &keyDerivation,
                              size_t outputIndex,
                              const FPublicKey &derivedKey,
                              const uint8_t *prefix,
                              size_t prefixLength,
                              FPublicKey &baseKey)
{
    return CryptoOps::underivePublicKey(keyDerivation,
                                        outputIndex,
                                        derivedKey,
                                        prefix,
                                        prefixLength,
                                        baseKey);
}

inline bool underivePublicKey(const FKeyDerivation &keyDerivation,
                              size_t outputIndex,
                              const FPublicKey &derivedKey,
                              FPublicKey &baseKey)
{
    return CryptoOps::underivePublicKey(keyDerivation, outputIndex, derivedKey, baseKey);
}

/* Generation and checking of a standard signature.
 */
inline void generateSignature(const FHash &prefixHash,
							  const FPublicKey &publicKey,
							  const FSecretKey &secretKey,
							  FSignature &signature)
{
    CryptoOps::generateSignature(prefixHash, publicKey, secretKey, signature);
}
inline bool checkSignature(const FHash &prefixHash,
                           const FPublicKey &publicKey,
                           const FSignature &signature)
{
    return CryptoOps::checkSignature(prefixHash, publicKey, signature);
}

/* Generation and checking of a tx proof; given a tx pubkey R, the recipient's view pubkey A, and
 * the key derivation D, the signature proves the knowledge of the tx secret key r such that R=r*G
 * and D=r*A
 */
inline void generateTxProof(const FHash &prefixHash,
							const FPublicKey &R,
							const FPublicKey &A,
							const FPublicKey &D,
							const FSecretKey &r,
							FSignature &signature)
{
    CryptoOps::generateTxProof(prefixHash, R, A, D, r, signature);
}
inline bool checkTxProof(const FHash &prefixHash,
                         const FPublicKey &R,
                         const FPublicKey &A,
                         const FPublicKey &D,
                         const FSignature &signature)
{
    return CryptoOps::checkTxProof(prefixHash, R, A, D, signature);
}

/* To send money to a key:
 * * The sender generates an ephemeral key and includes it in transaction output.
 * * To spend the money, the receiver generates a key image from it.
 * * Then he selects a bunch of outputs, including the one he spends, and uses them to generate a
 * ring signature. To check the signature, it is necessary to collect all the keys that were used to
 * generate it. To detect double spends, it is necessary to check that each key image is used at
 * most once.
 */
inline void generateKeyImage(const FPublicKey &publicKey,
							 const FSecretKey &secretKey,
							 FKeyImage &keyImage)
{
    CryptoOps::generateKeyImage(publicKey, secretKey, keyImage);
}

inline FKeyImage scalarMultKey(const FKeyImage &P, const FKeyImage &a)
{
    return CryptoOps::scalarMultKey(P, a);
}

inline void hashDataToEC(const uint8_t *data,
                         std::size_t length,
                         FPublicKey &publicKey)
{
    CryptoOps::hashDataToEC(data, length, publicKey);
}

inline void generateRingSignature(const FHash &prefixHash,
								  const FKeyImage &keyImage,
								  const FPublicKey *const *pPublicKey,
								  std::size_t publicKeyCount,
								  const FSecretKey &secretKey,
								  std::size_t secretKeyIndex,
								  FSignature *signature)
{
    CryptoOps::generateRingSignature(prefixHash,
                                     keyImage,
                                     pPublicKey,
                                     publicKeyCount,
                                     secretKey,
                                     secretKeyIndex,
                                     signature);
}
inline bool checkRingSignature(const FHash &prefixHash,
                               const FKeyImage &keyImage,
                               const FPublicKey *const *pPublicKey,
                               size_t publicKeyCount,
                               const FSignature *signature)
{
    return CryptoOps::checkRingSignature(prefixHash,
                                         keyImage,
                                         pPublicKey,
                                         publicKeyCount,
                                         signature);
}

/* Variants with vector<const FPublicKey *> parameters.
 */
inline void generateRingSignature(const FHash &prefixHash,
								  const FKeyImage &keyImage,
								  const std::vector<const FPublicKey *> &publicKeys,
								  const FSecretKey &secretKey,
								  size_t secretKeyIndex,
								  FSignature *signature)
{
    generateRingSignature(prefixHash,
                          keyImage,
                          publicKeys.data(),
                          publicKeys.size(),
                          secretKey,
                          secretKeyIndex,
                          signature);
}
inline bool checkRingSignature(const FHash &prefixHash,
                               const FKeyImage &keyImage,
                               const std::vector<const FPublicKey *> &publicKeys,
                               const FSignature *signature)
{
    return checkRingSignature(prefixHash,
                              keyImage,
                              publicKeys.data(),
                              publicKeys.size(),
                              signature);
}

static inline const FKeyImage &EllipticCurveScalar2KeyImage(const FEllipticCurveScalar &k)
{
    return (const FKeyImage &)k;
}

static inline const FPublicKey &EllipticCurveScalar2PublicKey(const FEllipticCurveScalar &k)
{
    return (const FPublicKey &)k;
}

static inline const FSecretKey &EllipticCurveScalar2SecretKey(const FEllipticCurveScalar &k)
{
    return (const FSecretKey &)k;
}

} // namespace Crypto

CRYPTO_MAKE_COMPARABLE(Crypto::FHash, std::memcmp)
CRYPTO_MAKE_COMPARABLE(Crypto::FEllipticCurveScalar, sodiumCompare)
CRYPTO_MAKE_COMPARABLE(Crypto::FEllipticCurvePoint, std::memcmp)
CRYPTO_MAKE_COMPARABLE(Crypto::FPublicKey, std::memcmp)
CRYPTO_MAKE_COMPARABLE(Crypto::FSecretKey, sodiumCompare)
CRYPTO_MAKE_COMPARABLE(Crypto::FKeyDerivation, std::memcmp)
CRYPTO_MAKE_COMPARABLE(Crypto::FKeyImage, std::memcmp)
CRYPTO_MAKE_COMPARABLE(Crypto::FSignature, std::memcmp)
