#pragma once

#include <string>

#include <crypto/hash.h>
#include <crypto/random.h>

#define CHACHA8_KEY_SIZE 32
#define CHACHA8_IV_SIZE 8

namespace Crypto {
void chacha8(const void *data, size_t length, const uint8_t *key, const uint8_t *iv, char *cipher);
void chacha(size_t doubleRound, const void *data, size_t length, const uint8_t *key, const uint8_t *iv, char *cipher);

#pragma pack(push, 1)
struct chacha8_key {
    uint8_t data[CHACHA8_KEY_SIZE];
};

struct chacha8_iv {
    uint8_t data[CHACHA8_IV_SIZE];
};
#pragma pack(pop)

static_assert(sizeof(chacha8_key) == CHACHA8_KEY_SIZE && sizeof(chacha8_iv) == CHACHA8_IV_SIZE,
              "Invalid structure size");

inline void chacha8(const void *data, size_t length, const chacha8_key &key, const chacha8_iv &iv,
                    char *cipher)
{
    chacha8(data, length, reinterpret_cast<const uint8_t *>(&key),
            reinterpret_cast<const uint8_t *>(&iv), cipher);
}

inline void chacha(size_t doubleRounds, const void *data, size_t length, const chacha8_key &key, const chacha8_iv &iv,
                    char *cipher)
{
    chacha(doubleRounds, data, length, reinterpret_cast<const uint8_t *>(&key),
            reinterpret_cast<const uint8_t *>(&iv), cipher);
}
inline void generate_chacha8_key(Crypto::cn_context &context, const std::string &password,
                                 chacha8_key &key)
{
    static_assert(sizeof(chacha8_key) <= sizeof(Hash),
                  "Size of hash must be at least that of chacha8_key");
    Hash pwd_hash;
    cn_slow_hash(context, password.data(), password.size(), pwd_hash);
    memcpy(&key, &pwd_hash, sizeof(key));
    memset(&pwd_hash, 0, sizeof(pwd_hash));
}

/**
 * Generates a random chacha8 IV
 */
inline chacha8_iv randomChachaIV()
{
    chacha8_iv result;
    Random::randomBytes(CHACHA8_IV_SIZE, result.data);
    return result;
}
}
