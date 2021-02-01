#ifndef _BLAKE256_H_
#define _BLAKE256_H_

#include <stdint.h>

typedef struct {
  uint32_t h[8], s[4], t[2];
  int buflen, nullt;
  uint8_t buf[64];
} state;

typedef struct {
  state inner;
  state outer;
} hmac_state;

void blake256Init(state *);
void blake224Init(state *);

void blake256Update(state *, const uint8_t *, uint64_t);
void blake224Update(state *, const uint8_t *, uint64_t);

void blake256Final(state *, uint8_t *);
void blake224Final(state *, uint8_t *);

void blake256Hash(uint8_t *, const uint8_t *, uint64_t);
void blake224Hash(uint8_t *, const uint8_t *, uint64_t);

/* HMAC functions: */

void hmacBlake256Init(hmac_state *, const uint8_t *, uint64_t);
void hmacBlake224Init(hmac_state *, const uint8_t *, uint64_t);

void hmacBlake256Update(hmac_state *, const uint8_t *, uint64_t);
void hmacBlake224Update(hmac_state *, const uint8_t *, uint64_t);

void hmacBlake256Final(hmac_state *, uint8_t *);
void hmacBlake224Final(hmac_state *, uint8_t *);

void hmacBlake256Hash(uint8_t *, const uint8_t *, uint64_t, const uint8_t *, uint64_t);
void hmacBlake224Hash(uint8_t *, const uint8_t *, uint64_t, const uint8_t *, uint64_t);

#endif /* _BLAKE256_H_ */
