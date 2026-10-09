#include "core/dungeon.h"
#include <string.h>
#include "core/noise.h"

Vec3 dungeon_cell_center(int cx, int cz) {
    return vec3(((float)cx - 0.5f * (DG_N - 1)) * DG_CELL, 0.0f, ((float)cz - 0.5f * (DG_N - 1)) * DG_CELL);
}

static bool in_grid(int x, int z) { return x >= 1 && z >= 1 && x < DG_N - 1 && z < DG_N - 1; }

static bool room_overlaps(const Dungeon *d, int x, int z, int w, int h) {
    for (int i = 0; i < d->nrooms; ++i) {
        const DgRoom *r = &d->rooms[i];
        if (x < r->x + r->w + 2 && x + w + 2 > r->x && z < r->z + r->h + 2 && z + h + 2 > r->z) return true;
    }
    return false;
}

static void carve_h(Dungeon *d, int x0, int x1, int z) { for (int x = x0 < x1 ? x0 : x1; x <= (x0 < x1 ? x1 : x0); ++x) if (in_grid(x, z) && !d->cell[z][x]) d->cell[z][x] = DG_FLOOR; }
static void carve_v(Dungeon *d, int z0, int z1, int x) { for (int z = z0 < z1 ? z0 : z1; z <= (z0 < z1 ? z1 : z0); ++z) if (in_grid(x, z) && !d->cell[z][x]) d->cell[z][x] = DG_FLOOR; }

static void room_center(const DgRoom *r, int *cx, int *cz) { *cx = r->x + r->w / 2; *cz = r->z + r->h / 2; }

/* BFS over floor cells from (sx,sz); dist[] filled with steps or -1 */
static void bfs(const Dungeon *d, int sx, int sz, int dist[DG_N][DG_N]) {
    for (int z = 0; z < DG_N; ++z) for (int x = 0; x < DG_N; ++x) dist[z][x] = -1;
    static int qx[DG_N * DG_N], qz[DG_N * DG_N];
    int head = 0, tail = 0;
    dist[sz][sx] = 0; qx[tail] = sx; qz[tail++] = sz;
    static const int DX[4] = {1, -1, 0, 0}, DZ[4] = {0, 0, 1, -1};
    while (head < tail) {
        int x = qx[head], z = qz[head++];
        for (int k = 0; k < 4; ++k) {
            int nx = x + DX[k], nz = z + DZ[k];
            if (nx < 0 || nz < 0 || nx >= DG_N || nz >= DG_N || !d->cell[nz][nx] || dist[nz][nx] >= 0) continue;
            dist[nz][nx] = dist[z][x] + 1; qx[tail] = nx; qz[tail++] = nz;
        }
    }
}

void dungeon_generate(Dungeon *d, uint32_t seed) {
    memset(d, 0, sizeof *d);
    Rng rng; rng_seed(&rng, seed ^ 0xD06E0u);
    int target = rng_int(&rng, 9, 13);
    for (int tries = 0; tries < 300 && d->nrooms < target; ++tries) {
        int w = rng_int(&rng, 3, 7), h = rng_int(&rng, 3, 7);
        int x = rng_int(&rng, 2, DG_N - 3 - w), z = rng_int(&rng, 2, DG_N - 3 - h);
        if (room_overlaps(d, x, z, w, h)) continue;
        d->rooms[d->nrooms++] = (DgRoom){x, z, w, h};
        for (int zz = z; zz < z + h; ++zz) for (int xx = x; xx < x + w; ++xx) d->cell[zz][xx] = DG_FLOOR;
    }
    if (d->nrooms < 2) { /* degenerate fallback: two fixed rooms */
        d->nrooms = 2; d->rooms[0] = (DgRoom){3, 3, 5, 5}; d->rooms[1] = (DgRoom){DG_N - 9, DG_N - 9, 5, 5};
        for (int i = 0; i < 2; ++i) for (int zz = d->rooms[i].z; zz < d->rooms[i].z + d->rooms[i].h; ++zz) for (int xx = d->rooms[i].x; xx < d->rooms[i].x + d->rooms[i].w; ++xx) d->cell[zz][xx] = DG_FLOOR;
    }
    /* connect: each room to the nearest already-connected room (a spanning tree), then a few loops */
    bool joined[DG_MAX_ROOMS] = {false}; joined[0] = true;
    for (int n = 1; n < d->nrooms; ++n) {
        int bi = -1, bj = -1, bd = 1 << 30;
        for (int i = 0; i < d->nrooms; ++i) if (joined[i])
            for (int j = 0; j < d->nrooms; ++j) if (!joined[j]) {
                int ax, az, bx, bz; room_center(&d->rooms[i], &ax, &az); room_center(&d->rooms[j], &bx, &bz);
                int dist = (ax > bx ? ax - bx : bx - ax) + (az > bz ? az - bz : bz - az);
                if (dist < bd) { bd = dist; bi = i; bj = j; }
            }
        int ax, az, bx, bz; room_center(&d->rooms[bi], &ax, &az); room_center(&d->rooms[bj], &bx, &bz);
        if (rng_next(&rng) & 1) { carve_h(d, ax, bx, az); carve_v(d, az, bz, bx); } else { carve_v(d, az, bz, ax); carve_h(d, ax, bx, bz); }
        joined[bj] = true;
    }
    for (int k = 0; k < 2 && d->nrooms > 3; ++k) {
        int i = rng_int(&rng, 0, d->nrooms - 1), j = rng_int(&rng, 0, d->nrooms - 1);
        if (i == j) continue;
        int ax, az, bx, bz; room_center(&d->rooms[i], &ax, &az); room_center(&d->rooms[j], &bx, &bz);
        carve_h(d, ax, bx, az); carve_v(d, az, bz, bx);
    }
    /* start = room 0 (leftmost-ish via generation order is arbitrary); boss = farthest room by walking distance */
    d->start_room = 0;
    int sx, sz; room_center(&d->rooms[0], &sx, &sz);
    static int dist[DG_N][DG_N];
    bfs(d, sx, sz, dist);
    int best = 0, bestd = -1;
    for (int i = 1; i < d->nrooms; ++i) { int cx, cz; room_center(&d->rooms[i], &cx, &cz); if (dist[cz][cx] > bestd) { bestd = dist[cz][cx]; best = i; } }
    d->boss_room = best;
    for (int i = 0; i < 2; ++i) {
        const DgRoom *r = &d->rooms[i == 0 ? d->start_room : d->boss_room];
        for (int zz = r->z; zz < r->z + r->h; ++zz) for (int xx = r->x; xx < r->x + r->w; ++xx) d->cell[zz][xx] = i == 0 ? DG_START : DG_BOSS;
    }
    for (int z = 0; z < DG_N; ++z) for (int x = 0; x < DG_N; ++x) d->floor_cells += d->cell[z][x] != DG_WALL;

    /* encounters: 2-5 enemies per ordinary room, none in the start room, boss spot in the boss room */
    for (int i = 0; i < d->nrooms; ++i) {
        const DgRoom *r = &d->rooms[i];
        if (i == d->start_room) continue;
        int n = i == d->boss_room ? 0 : rng_int(&rng, 2, 5);
        for (int k = 0; k < n && d->nspawns < DG_MAX_SPAWNS; ++k) {
            int cx = rng_int(&rng, r->x, r->x + r->w - 1), cz = rng_int(&rng, r->z, r->z + r->h - 1);
            Vec3 c = dungeon_cell_center(cx, cz);
            d->spawns[d->nspawns++] = (DgSpawn){c.x, c.z, i};
        }
        if (i != d->boss_room && d->nchests < 8 && rng_float(&rng) < 0.55f) {
            Vec3 c = dungeon_cell_center(r->x + r->w - 1, r->z + r->h - 1);
            d->chests[d->nchests++] = (DgPoint){c.x - 0.8f, c.z - 0.8f};
        }
        /* wall torches at room corners */
        if (d->ntorches + 2 <= 48) {
            Vec3 a = dungeon_cell_center(r->x, r->z), b = dungeon_cell_center(r->x + r->w - 1, r->z + r->h - 1);
            d->torches[d->ntorches++] = (DgPoint){a.x - 1.1f, a.z - 1.1f};
            d->torches[d->ntorches++] = (DgPoint){b.x + 1.1f, b.z + 1.1f};
        }
    }
}

bool dungeon_is_floor_at(const Dungeon *d, float x, float z) {
    int cx = (int)((x / DG_CELL) + 0.5f * (DG_N - 1) + 0.5f), cz = (int)((z / DG_CELL) + 0.5f * (DG_N - 1) + 0.5f);
    if (x < -0.5f * DG_N * DG_CELL || z < -0.5f * DG_N * DG_CELL) return false;
    return cx >= 0 && cz >= 0 && cx < DG_N && cz < DG_N && d->cell[cz][cx] != DG_WALL;
}

int dungeon_reachable(const Dungeon *d) {
    int sx, sz; room_center(&d->rooms[d->start_room], &sx, &sz);
    static int dist[DG_N][DG_N];
    bfs(d, sx, sz, dist);
    int n = 0;
    for (int z = 0; z < DG_N; ++z) for (int x = 0; x < DG_N; ++x) n += dist[z][x] >= 0;
    return n;
}

int dungeon_wall_rects(const Dungeon *d, DgWallRect *out, int cap) {
    int n = 0;
    float half = 0.5f * DG_CELL;
    for (int z = 0; z < DG_N; ++z) {
        int x = 0;
        while (x < DG_N) {
            /* only wall cells touching floor matter (others are never seen or reached) */
            bool edge = false;
            if (d->cell[z][x] == DG_WALL) {
                for (int dz = -1; dz <= 1 && !edge; ++dz) for (int dx = -1; dx <= 1; ++dx) {
                    int nx = x + dx, nz = z + dz;
                    if (nx >= 0 && nz >= 0 && nx < DG_N && nz < DG_N && d->cell[nz][nx] != DG_WALL) { edge = true; break; }
                }
            }
            if (!edge) { ++x; continue; }
            int x1 = x;
            while (x1 + 1 < DG_N && d->cell[z][x1 + 1] == DG_WALL) {
                bool e2 = false;
                for (int dz = -1; dz <= 1 && !e2; ++dz) for (int dx = -1; dx <= 1; ++dx) {
                    int nx = x1 + 1 + dx, nz = z + dz;
                    if (nx >= 0 && nz >= 0 && nx < DG_N && nz < DG_N && d->cell[nz][nx] != DG_WALL) { e2 = true; break; }
                }
                if (!e2) break;
                ++x1;
            }
            if (n < cap) {
                Vec3 a = dungeon_cell_center(x, z), b = dungeon_cell_center(x1, z);
                out[n] = (DgWallRect){a.x - half, a.z - half, b.x + half, b.z + half};
            }
            ++n;
            x = x1 + 1;
        }
    }
    return n < cap ? n : cap;
}
