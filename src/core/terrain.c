#include "core/terrain.h"
#include <math.h>
#include "core/noise.h"

static float smooth01(float t) { t = clampf(t, 0.0f, 1.0f); return t * t * (3.0f - 2.0f * t); }

void terrain_generate(Terrain *t, const TerrainParams *p) {
    t->min_h = 1e9f; t->max_h = -1e9f;
    for (int j = 0; j < TERRAIN_N; ++j)
        for (int i = 0; i < TERRAIN_N; ++i) {
            float x = ((float)i - 0.5f * (TERRAIN_N - 1)) * TERRAIN_CELL, z = ((float)j - 0.5f * (TERRAIN_N - 1)) * TERRAIN_CELL;
            float n = fbm2(p->seed, x * p->freq, z * p->freq, 4);
            float rg = 1.0f - fabsf(2.0f * fbm2(p->seed + 77u, x * p->freq * 0.7f, z * p->freq * 0.7f, 3) - 1.0f);
            float h = p->base + p->amp * ((1.0f - p->ridge) * (n - 0.5f) * 2.0f + p->ridge * (rg * rg * 1.6f - 0.5f));
            /* town/camp plateau: level, blended into the hills */
            float d = hypotf(x - p->flat_x, z - p->flat_z);
            if (p->flat_r > 0) h = p->base + (h - p->base) * smooth01((d - p->flat_r) / 12.0f);
            /* rising rim keeps players inside the zone */
            float e = fmaxf(fabsf(x), fabsf(z)) - (TERRAIN_HALF - 18.0f);
            if (e > 0) h += p->rim * e * e * 0.05f;
            t->h[j * TERRAIN_N + i] = h;
            if (h < t->min_h) t->min_h = h;
            if (h > t->max_h) t->max_h = h;
        }
}

float terrain_height(const Terrain *t, float x, float z) {
    float fx = clampf(x / TERRAIN_CELL + 0.5f * (TERRAIN_N - 1), 0.0f, (float)(TERRAIN_N - 1) - 1e-4f);
    float fz = clampf(z / TERRAIN_CELL + 0.5f * (TERRAIN_N - 1), 0.0f, (float)(TERRAIN_N - 1) - 1e-4f);
    int i = (int)fx, j = (int)fz;
    float tx = fx - (float)i, tz = fz - (float)j;
    const float *r0 = &t->h[j * TERRAIN_N + i], *r1 = r0 + TERRAIN_N;
    return (r0[0] + (r0[1] - r0[0]) * tx) * (1.0f - tz) + (r1[0] + (r1[1] - r1[0]) * tx) * tz;
}

Vec3 terrain_normal(const Terrain *t, float x, float z) {
    float e = TERRAIN_CELL * 0.5f;
    float dx = terrain_height(t, x + e, z) - terrain_height(t, x - e, z);
    float dz = terrain_height(t, x, z + e) - terrain_height(t, x, z - e);
    float l = sqrtf(dx * dx + 4.0f * e * e + dz * dz);
    return vec3(-dx / l, 2.0f * e / l, -dz / l);
}

float terrain_slope(const Terrain *t, float x, float z) { return 1.0f - terrain_normal(t, x, z).y; }
