#include "core/noise.h"
#include <math.h>

uint32_t hash_u32(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
    return x;
}
uint32_t hash_str(const char *s) {
    uint32_t h = 2166136261u;
    for (; *s; ++s) { h ^= (uint8_t)*s; h *= 16777619u; }
    return hash_u32(h);
}
uint32_t hash_combine(uint32_t a, uint32_t b) { return hash_u32(a * 0x9e3779b9u + b + 0x7f4a7c15u); }

void rng_seed(Rng *r, uint32_t seed) { r->s = hash_u32(seed ? seed : 0x1234567u); }
uint32_t rng_next(Rng *r) { r->s ^= r->s << 13; r->s ^= r->s >> 17; r->s ^= r->s << 5; return r->s; }
float rng_float(Rng *r) { return (float)(rng_next(r) >> 8) * (1.0f / 16777216.0f); }
float rng_range(Rng *r, float lo, float hi) { return lo + (hi - lo) * rng_float(r); }
int rng_int(Rng *r, int lo, int hi) { return hi <= lo ? lo : lo + (int)(rng_next(r) % (uint32_t)(hi - lo + 1)); }

static float lattice(uint32_t seed, int x, int y) {
    return (float)(hash_combine(hash_combine(seed, (uint32_t)x), (uint32_t)y) >> 8) * (1.0f / 16777216.0f);
}
float noise2(uint32_t seed, float x, float y) {
    float fx = floorf(x), fy = floorf(y);
    int ix = (int)fx, iy = (int)fy;
    float tx = x - fx, ty = y - fy;
    tx = tx * tx * (3.0f - 2.0f * tx); ty = ty * ty * (3.0f - 2.0f * ty);
    float a = lattice(seed, ix, iy), b = lattice(seed, ix + 1, iy), c = lattice(seed, ix, iy + 1), d = lattice(seed, ix + 1, iy + 1);
    return (a + (b - a) * tx) + ((c + (d - c) * tx) - (a + (b - a) * tx)) * ty;
}
float fbm2(uint32_t seed, float x, float y, int octaves) {
    float sum = 0, amp = 0.5f, norm = 0;
    for (int i = 0; i < octaves; ++i) {
        sum += amp * noise2(seed + (uint32_t)i * 101u, x, y); norm += amp;
        x *= 2.03f; y *= 2.03f; amp *= 0.5f;
    }
    return sum / norm;
}
