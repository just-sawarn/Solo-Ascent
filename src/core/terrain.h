#ifndef SA_TERRAIN_H
#define SA_TERRAIN_H
/* Square heightfield (one zone). Pure data + sampling: rendering and physics both read it. */
#include <stdbool.h>
#include <stdint.h>
#include "core/vec.h"

#define TERRAIN_N 81            /* vertices per side */
#define TERRAIN_CELL 2.0f       /* metres between vertices */
#define TERRAIN_HALF (0.5f * (TERRAIN_N - 1) * TERRAIN_CELL)   /* 80 m */

typedef struct {
    uint32_t seed;
    float base;                 /* mean height */
    float amp;                  /* hill height (m) */
    float freq;                 /* hills per metre */
    float ridge;                /* 0..1: how ridged/jagged (mesas, ice spires) */
    float flat_x, flat_z, flat_r; /* plateau kept level for towns/camps; blends over 12 m */
    float rim;                  /* edge wall steepness (the zone is bounded by rising hills) */
} TerrainParams;

typedef struct { float h[TERRAIN_N * TERRAIN_N]; float min_h, max_h; } Terrain;

void  terrain_generate(Terrain *t, const TerrainParams *p);
float terrain_height(const Terrain *t, float x, float z);       /* bilinear; clamps outside */
Vec3  terrain_normal(const Terrain *t, float x, float z);
float terrain_slope(const Terrain *t, float x, float z);        /* 0 flat .. 1 vertical (1 - normal.y) */

#endif
