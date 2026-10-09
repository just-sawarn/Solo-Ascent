#ifndef SA_NOISE_H
#define SA_NOISE_H
/* Deterministic hashing, a tiny RNG and smooth value noise. Same output on every platform. */
#include <stdint.h>

uint32_t hash_u32(uint32_t x);
uint32_t hash_str(const char *s);
uint32_t hash_combine(uint32_t a, uint32_t b);

typedef struct { uint32_t s; } Rng;
void     rng_seed(Rng *r, uint32_t seed);
uint32_t rng_next(Rng *r);
float    rng_float(Rng *r);                       /* [0,1) */
float    rng_range(Rng *r, float lo, float hi);
int      rng_int(Rng *r, int lo, int hi);          /* inclusive */

float noise2(uint32_t seed, float x, float y);     /* smooth value noise in [0,1] */
float fbm2(uint32_t seed, float x, float y, int octaves); /* [0,1] */

#endif
