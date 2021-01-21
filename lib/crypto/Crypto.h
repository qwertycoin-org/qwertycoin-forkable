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

#include "generic-ops.h"
#include "hash.h"

namespace Crypto {

extern "C" {
#include "random.h"
}

extern std::mutex random_lock;

struct EllipticCurvePoint {
    uint8_t data[32];
};

struct EllipticCurveScalar {
    uint8_t data[32];
};

class crypto_ops
{
    crypto_ops();
    crypto_ops(const crypto_ops &);
    void operator=(const crypto_ops &);
    ~crypto_ops();

    static void generateKeys(PublicKey &, SecretKey &);
    friend void generateKeys(PublicKey &, SecretKey &);
    static void generateDeterministicKeys(PublicKey &pub, SecretKey &sec, SecretKey &second);
    friend void generateDeterministicKeys(PublicKey &pub, SecretKey &sec, SecretKey &second);
    static SecretKey generateMKeys(PublicKey &pub, SecretKey &sec,
                                   const SecretKey &recovery_key = SecretKey(),
                                   bool recover = false);
    friend SecretKey generateMKeys(PublicKey &pub, SecretKey &sec, const SecretKey &recovery_key,
                                   bool recover);
    static bool checkKey(const PublicKey &);
    friend bool checkKey(const PublicKey &);
    static bool secretKeyToPublicKey(const SecretKey &, PublicKey &);
    friend bool secretKeyToPublicKey(const SecretKey &, PublicKey &);
    static bool generateKeyDerivation(const PublicKey &key1, const SecretKey &key2,
                                      KeyDerivation &derivation);
    friend bool generateKeyDerivation(const PublicKey &, const SecretKey &, KeyDerivation &);
    static bool derivePublicKey(const KeyDerivation &, size_t, const PublicKey &, PublicKey &);
    friend bool derivePublicKey(const KeyDerivation &, size_t, const PublicKey &, PublicKey &);
    friend bool derivePublicKey(const KeyDerivation &, size_t, const PublicKey &,
                                const uint8_t *prefix, size_t, PublicKey &);
    static bool derivePublicKey(const KeyDerivation &, size_t, const PublicKey &, const uint8_t *,
                                size_t, PublicKey &);
    // hack for pg
    static bool underivePublicKeyAndGetScalar(const KeyDerivation &, std::size_t, const PublicKey &,
                                              PublicKey &, EllipticCurveScalar &);
    friend bool underivePublicKeyAndGetScalar(const KeyDerivation &, std::size_t, const PublicKey &,
                                              PublicKey &, EllipticCurveScalar &);
    static void generateIncompleteKeyImage(const PublicKey &, EllipticCurvePoint &);
    friend void generateIncompleteKeyImage(const PublicKey &, EllipticCurvePoint &);
    //
    static void deriveSecretKey(const KeyDerivation &, size_t, const SecretKey &, SecretKey &);
    friend void deriveSecretKey(const KeyDerivation &, size_t, const SecretKey &, SecretKey &);
    static void deriveSecretKey(const KeyDerivation &, size_t, const SecretKey &, const uint8_t *,
                                size_t, SecretKey &);
    friend void deriveSecretKey(const KeyDerivation &, size_t, const SecretKey &, const uint8_t *,
                                size_t, SecretKey &);
    static bool underivePublicKey(const KeyDerivation &, size_t, const PublicKey &, PublicKey &);
    friend bool underivePublicKey(const KeyDerivation &, size_t, const PublicKey &, PublicKey &);
    static bool underivePublicKey(const KeyDerivation &, size_t, const PublicKey &, const uint8_t *,
                                  size_t, PublicKey &);
    friend bool underivePublicKey(const KeyDerivation &, size_t, const PublicKey &, const uint8_t *,
                                  size_t, PublicKey &);
    static void generateSignature(const Hash &, const PublicKey &, const SecretKey &, Signature &);
    friend void generateSignature(const Hash &, const PublicKey &, const SecretKey &, Signature &);
    static bool checkSignature(const Hash &, const PublicKey &, const Signature &);
    friend bool checkSignature(const Hash &, const PublicKey &, const Signature &);
    static void generateTxProof(const Hash &, const PublicKey &, const PublicKey &,
                                const PublicKey &, const SecretKey &, Signature &);
    friend void generateTxProof(const Hash &, const PublicKey &, const PublicKey &,
                                const PublicKey &, const SecretKey &, Signature &);
    static bool checkTxProof(const Hash &, const PublicKey &, const PublicKey &, const PublicKey &,
                             const Signature &);
    friend bool checkTxProof(const Hash &, const PublicKey &, const PublicKey &, const PublicKey &,
                             const Signature &);
    static void generateKeyImage(const PublicKey &, const SecretKey &, KeyImage &);
    friend void generateKeyImage(const PublicKey &, const SecretKey &, KeyImage &);
    static KeyImage scalarMultKey(const KeyImage &P, const KeyImage &a);
    friend KeyImage scalarMultKey(const KeyImage &P, const KeyImage &a);
    static void hashDataToEC(const uint8_t *, std::size_t, PublicKey &);
    friend void hashDataToEC(const uint8_t *, std::size_t, PublicKey &);
    static void generateRingSignature(const Hash &, const KeyImage &, const PublicKey *const *,
                                      size_t, const SecretKey &, size_t, Signature *);
    friend void generateRingSignature(const Hash &, const KeyImage &, const PublicKey *const *,
                                      size_t, const SecretKey &, size_t, Signature *);
    static bool checkRingSignature(const Hash &, const KeyImage &, const PublicKey *const *, size_t,
                                   const Signature *);
    friend bool checkRingSignature(const Hash &, const KeyImage &, const PublicKey *const *, size_t,
                                   const Signature *);
};

/* Generate a value filled with random bytes.
 */
template<typename T>
typename std::enable_if<std::is_pod<T>::value, T>::type rand()
{
    typename std::remove_cv<T>::type res;
    std::lock_guard<std::mutex> lock(random_lock);
    generate_random_bytes(sizeof(T), &res);
    return res;
}

/* Random number engine based on crypto::rand()
 */
template<typename T>
class RandomEngine
{
public:
    typedef T result_type;

#ifdef __clang__
    constexpr static T min() { return (std::numeric_limits<T>::min)(); }

    constexpr static T max() { return (std::numeric_limits<T>::max)(); }
#else
    static T(min)() { return (std::numeric_limits<T>::min)(); }

    static T(max)() { return (std::numeric_limits<T>::max)(); }
#endif
    typename std::enable_if<std::is_unsigned<T>::value, T>::type operator()() { return rand<T>(); }
};

/* Generate a new key pair
 */
inline void generateKeys(PublicKey &publicKey, SecretKey &secretKey)
{
    crypto_ops::generateKeys(publicKey, secretKey);
}

inline void generateDeterministicKeys(PublicKey &pub, SecretKey &sec, SecretKey &second)
{
    crypto_ops::generateDeterministicKeys(pub, sec, second);
}

inline SecretKey generateMKeys(PublicKey &pub, SecretKey &sec,
                               const SecretKey &recovery_key = SecretKey(), bool recover = false)
{
    return crypto_ops::generateMKeys(pub, sec, recovery_key, recover);
}

/* Check a public key. Returns true if it is valid, false otherwise.
 */
inline bool checkKey(const PublicKey &publicKey)
{
    return crypto_ops::checkKey(publicKey);
}

/* Checks a private key and computes the corresponding public key.
 */
inline bool secretKeyToPublicKey(const SecretKey &secretKey, PublicKey &publicKey)
{
    return crypto_ops::secretKeyToPublicKey(secretKey, publicKey);
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
inline bool generateKeyDerivation(const PublicKey &key1, const SecretKey &key2,
                                  KeyDerivation &derivation)
{
    return crypto_ops::generateKeyDerivation(key1, key2, derivation);
}

inline bool derivePublicKey(const KeyDerivation &derivation, size_t outputIndex,
                            const PublicKey &base, const uint8_t *prefix, size_t prefixLength,
                            PublicKey &derivedKey)
{
    return crypto_ops::derivePublicKey(derivation, outputIndex, base, prefix, prefixLength,
                                       derivedKey);
}

inline bool derivePublicKey(const KeyDerivation &derivation, size_t outputIndex,
                            const PublicKey &base, PublicKey &derivedKey)
{
    return crypto_ops::derivePublicKey(derivation, outputIndex, base, derivedKey);
}

inline bool underivePublicKeyAndGetScalar(const KeyDerivation &keyDerivation,
                                          std::size_t outputIndex,
                                          const PublicKey &derivedKey,
                                          PublicKey &baseKey,
                                          EllipticCurveScalar &hashedDerivation)
{
    return crypto_ops::underivePublicKeyAndGetScalar(keyDerivation, outputIndex, derivedKey, baseKey,
                                                     hashedDerivation);
}

inline void deriveSecretKey(const KeyDerivation &keyDerivation,
                            std::size_t outputIndex,
                            const SecretKey &baseKey,
                            const uint8_t *prefix,
                            size_t prefixLength,
                            SecretKey &derivedKey)
{
    crypto_ops::deriveSecretKey(keyDerivation, outputIndex, baseKey, prefix, prefixLength, derivedKey);
}

inline void deriveSecretKey(const KeyDerivation &keyDerivation,
                            std::size_t outputIndex,
                            const SecretKey &baseKey,
                            SecretKey &derivedKey)
{
    crypto_ops::deriveSecretKey(keyDerivation, outputIndex, baseKey, derivedKey);
}

/* Inverse function of derivePublicKey. It can be used by the receiver to find which "spend" key was
 * used to generate a transaction. This may be useful if the receiver used multiple addresses which
 * only differ in "spend" key.
 */
inline bool underivePublicKey(const KeyDerivation &keyDerivation,
                              size_t outputIndex,
                              const PublicKey &derivedKey,
                              const uint8_t *prefix,
                              size_t prefixLength,
                              PublicKey &baseKey)
{
    return crypto_ops::underivePublicKey(keyDerivation, outputIndex, derivedKey, prefix,
                                         prefixLength, baseKey);
}

inline bool underivePublicKey(const KeyDerivation &keyDerivation,
                              size_t outputIndex,
                              const PublicKey &derivedKey,
                              PublicKey &baseKey)
{
    return crypto_ops::underivePublicKey(keyDerivation, outputIndex, derivedKey, baseKey);
}

/* Generation and checking of a standard signature.
 */
inline void generateSignature(const Hash &prefixHash,
                              const PublicKey &publicKey,
                              const SecretKey &secretKey,
                              Signature &signature)
{
    crypto_ops::generateSignature(prefixHash, publicKey, secretKey, signature);
}
inline bool checkSignature(const Hash &prefixHash,
                           const PublicKey &publicKey,
                           const Signature &signature)
{
    return crypto_ops::checkSignature(prefixHash, publicKey, signature);
}

/* Generation and checking of a tx proof; given a tx pubkey R, the recipient's view pubkey A, and
 * the key derivation D, the signature proves the knowledge of the tx secret key r such that R=r*G
 * and D=r*A
 */
inline void generateTxProof(const Hash &prefixHash,
                            const PublicKey &R,
                            const PublicKey &A,
                            const PublicKey &D,
                            const SecretKey &r,
                            Signature &signature)
{
    crypto_ops::generateTxProof(prefixHash, R, A, D, r, signature);
}
inline bool checkTxProof(const Hash &prefixHash,
                         const PublicKey &R,
                         const PublicKey &A,
                         const PublicKey &D,
                         const Signature &signature)
{
    return crypto_ops::checkTxProof(prefixHash, R, A, D, signature);
}

/* To send money to a key:
 * * The sender generates an ephemeral key and includes it in transaction output.
 * * To spend the money, the receiver generates a key image from it.
 * * Then he selects a bunch of outputs, including the one he spends, and uses them to generate a
 * ring signature. To check the signature, it is necessary to collect all the keys that were used to
 * generate it. To detect double spends, it is necessary to check that each key image is used at
 * most once.
 */
inline void generateKeyImage(const PublicKey &publicKey, const SecretKey &secretKey, KeyImage &keyImage)
{
    crypto_ops::generateKeyImage(publicKey, secretKey, keyImage);
}

inline KeyImage scalarMultKey(const KeyImage &P, const KeyImage &a)
{
    return crypto_ops::scalarMultKey(P, a);
}

inline void hashDataToEC(const uint8_t *data, std::size_t length, PublicKey &publicKey)
{
    crypto_ops::hashDataToEC(data, length, publicKey);
}

inline void generateRingSignature(const Hash &prefixHash,
                                  const KeyImage &keyImage,
                                  const PublicKey *const *pPublicKey,
                                  std::size_t publicKeyCount,
                                  const SecretKey &secretKey,
                                  std::size_t secretKeyIndex,
                                  Signature *signature)
{
    crypto_ops::generateRingSignature(prefixHash,
                                      keyImage,
                                      pPublicKey,
                                      publicKeyCount,
                                      secretKey,
                                      secretKeyIndex,
                                      signature);
}
inline bool checkRingSignature(const Hash &prefixHash,
                               const KeyImage &keyImage,
                               const PublicKey *const *pPublicKey,
                               size_t publicKeyCount,
                               const Signature *signature)
{
    return crypto_ops::checkRingSignature(prefixHash,
                                          keyImage,
                                          pPublicKey,
                                          publicKeyCount,
                                          signature);
}

/* Variants with vector<const PublicKey *> parameters.
 */
inline void generateRingSignature(const Hash &prefixHash,
                                  const KeyImage &keyImage,
                                  const std::vector<const PublicKey *> &publicKeys,
                                  const SecretKey &secretKey,
                                  size_t secretKeyIndex,
                                  Signature *signature)
{
    generateRingSignature(prefixHash,
                          keyImage,
                          publicKeys.data(),
                          publicKeys.size(),
                          secretKey,
                          secretKeyIndex,
                          signature);
}
inline bool checkRingSignature(const Hash &prefixHash,
                               const KeyImage &keyImage,
                               const std::vector<const PublicKey *> &publicKeys,
                               const Signature *signature)
{
    return checkRingSignature(prefixHash, keyImage, publicKeys.data(), publicKeys.size(),
                              signature);
}

}

CRYPTO_MAKE_HASHABLE(PublicKey)
CRYPTO_MAKE_HASHABLE(KeyImage)
CRYPTO_MAKE_COMPARABLE(Signature)
CRYPTO_MAKE_COMPARABLE(SecretKey)
