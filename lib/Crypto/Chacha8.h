#pragma once

#include <string>

#include <Crypto/Hash.h>
#include <Crypto/Random.h>

#define CHACHA8_KEY_SIZE 32
#define CHACHA8_IV_SIZE 8

namespace Crypto {
void chacha8(const void *data, size_t length, const uint8_t *key, const uint8_t *iv, char *cipher);
void chacha(size_t doubleRound, const void *data, size_t length, const uint8_t *key, const uint8_t *iv, char *cipher);

#pragma pack(push, 1)
struct Chacha8Key
{
    uint8_t data[CHACHA8_KEY_SIZE];
};

struct Chacha8Iv
{
    uint8_t data[CHACHA8_IV_SIZE];
};
#pragma pack(pop)

static_assert(sizeof(Chacha8Key) == CHACHA8_KEY_SIZE && sizeof(Chacha8Iv) == CHACHA8_IV_SIZE,
              "Invalid structure size");

inline void chacha8(const void *data,
                    size_t length,
                    const Chacha8Key &key,
                    const Chacha8Iv &iv,
                    char *cipher)
{
    chacha8(data,
            length,
            reinterpret_cast<const uint8_t *>(&key),
            reinterpret_cast<const uint8_t *>(&iv),
            cipher);
}

inline void chacha(size_t doubleRounds,
                   const void *data,
                   size_t length,
                   const Chacha8Key &key,
                   const Chacha8Iv &iv,
                   char *cipher)
{
    chacha(doubleRounds,
           data,
           length,
           reinterpret_cast<const uint8_t *>(&key),
           reinterpret_cast<const uint8_t *>(&iv),
           cipher);
}

inline void generateChacha8Key(Crypto::CnContext &context,
                               const std::string &password,
                               Chacha8Key &key)
{
    static_assert(sizeof(Chacha8Key) <= sizeof(Hash),
                  "Size of Hash must be at least that of Chacha8Key");
    Hash pwdHash{};
    cnSlowHash(context, password.data(), password.size(), pwdHash);
    memcpy(&key, &pwdHash, sizeof(key));
    memset(&pwdHash, 0, sizeof(pwdHash));
}

/**
 * Generates a random chacha8 IV
 */
inline Chacha8Iv randomChachaIV()
{
    Chacha8Iv result {};
    Random::randomBytes(CHACHA8_IV_SIZE, result.data);
    return result;
}
}
