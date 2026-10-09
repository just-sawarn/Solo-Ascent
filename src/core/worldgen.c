#include "core/worldgen.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>
#include "core/log.h"

static char *read_all(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { LOGE("cannot open %s", path); return NULL; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)n, f); fclose(f); buf[got] = 0;
    return buf;
}
static float num(const cJSON *o, const char *k, float def) { const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, k); return cJSON_IsNumber(v) ? (float)v->valuedouble : def; }
static const char *str(const cJSON *o, const char *k) { const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, k); return cJSON_IsString(v) ? v->valuestring : ""; }
static void rgb8(const cJSON *o, const char *k, uint8_t out[3]) {
    const cJSON *a = cJSON_GetObjectItemCaseSensitive(o, k);
    for (int i = 0; i < 3; ++i) { const cJSON *v = cJSON_GetArrayItem(a, i); out[i] = (uint8_t)(cJSON_IsNumber(v) ? v->valueint : 128); }
}
static Rgb rgbf(const cJSON *o, const char *k) {
    const cJSON *a = cJSON_GetObjectItemCaseSensitive(o, k);
    float c[3] = {1, 1, 1};
    for (int i = 0; i < 3; ++i) { const cJSON *v = cJSON_GetArrayItem(a, i); if (cJSON_IsNumber(v)) c[i] = (float)v->valuedouble; }
    return (Rgb){c[0], c[1], c[2]};
}

static const char *PROP_NAMES[PROP_COUNT] = {"tree", "pine", "bush", "rock", "cactus", "deadtree", "icespike", "crystal", "lavarock", "pillar", "bones", "lamp"};
static int prop_from(const char *s) { for (int i = 0; i < PROP_COUNT; ++i) if (!strcmp(PROP_NAMES[i], s)) return i; return -1; }

int worlddb_zone(const WorldDB *db, const char *id) { for (int i = 0; i < db->nzone; ++i) if (!strcmp(db->zone[i].id, id)) return i; return -1; }
int worlddb_region(const WorldDB *db, const char *id) { for (int i = 0; i < db->nregion; ++i) if (!strcmp(db->region[i].id, id)) return i; return -1; }

static float mapdist(const ZoneDef *a, const ZoneDef *b) { return hypotf(a->map[0] - b->map[0], a->map[1] - b->map[1]); }

static bool linked(const ZoneDef *a, int j) { for (int k = 0; k < a->nexits; ++k) if (a->exit_to[k] == j) return true; return false; }
static void link_one(WorldDB *db, int a, int b) {
    if (a == b) return;
    ZoneDef *za = &db->zone[a], *zb = &db->zone[b];
    if (!linked(za, b) && za->nexits < WG_MAX_EXITS && !linked(zb, a) && zb->nexits < WG_MAX_EXITS) { za->exit_to[za->nexits++] = b; zb->exit_to[zb->nexits++] = a; }
}

/* travel graph: towns/fields link to their 2 nearest non-dungeon zones; a dungeon links to its nearest town/field;
 * remaining islands are bridged by the closest pair so everything is reachable. */
static void build_links(WorldDB *db) {
    for (int i = 0; i < db->nzone; ++i) {
        ZoneDef *z = &db->zone[i];
        int want = z->gen == ZG_DUNGEON ? 1 : 2;
        for (int n = 0; n < want; ++n) {
            int best = -1; float bd = 1e9f;
            for (int j = 0; j < db->nzone; ++j) {
                if (j == i || db->zone[j].gen == ZG_DUNGEON || linked(z, j) || db->zone[j].nexits >= WG_MAX_EXITS - 1) continue;
                float d = mapdist(z, &db->zone[j]);
                if (d < bd) { bd = d; best = j; }
            }
            if (best >= 0) link_one(db, i, best);
        }
    }
    for (int round = 0; round < db->nzone; ++round) { /* connect components */
        int comp[WG_MAX_ZONES]; for (int i = 0; i < db->nzone; ++i) comp[i] = -1;
        int stack[WG_MAX_ZONES], ncomp = 0;
        for (int s = 0; s < db->nzone; ++s) {
            if (comp[s] >= 0) continue;
            int sp = 0; stack[sp++] = s; comp[s] = ncomp;
            while (sp) { int u = stack[--sp]; for (int k = 0; k < db->zone[u].nexits; ++k) { int v = db->zone[u].exit_to[k]; if (comp[v] < 0) { comp[v] = ncomp; stack[sp++] = v; } } }
            ++ncomp;
        }
        if (ncomp <= 1) break;
        int ba = -1, bb = -1; float bd = 1e9f;
        for (int a = 0; a < db->nzone; ++a) for (int b = 0; b < db->nzone; ++b) {
            if (comp[a] == comp[b] || db->zone[a].gen == ZG_DUNGEON || db->zone[b].gen == ZG_DUNGEON) continue;
            if (db->zone[a].nexits >= WG_MAX_EXITS - 1 || db->zone[b].nexits >= WG_MAX_EXITS - 1) continue;
            float d = mapdist(&db->zone[a], &db->zone[b]);
            if (d < bd) { bd = d; ba = a; bb = b; }
        }
        if (ba < 0) break;
        link_one(db, ba, bb);
    }
}

bool worlddb_load(WorldDB *db, const char *dir, const EnemyDB *edb) {
    memset(db, 0, sizeof *db);
    char path[256];
    snprintf(path, sizeof path, "%s/regions.json", dir);
    char *text = read_all(path);
    if (!text) return false;
    cJSON *root = cJSON_Parse(text); free(text);
    if (!root) { LOGE("regions.json: parse error"); return false; }
    bool ok = true;
    const cJSON *r;
    cJSON_ArrayForEach(r, cJSON_GetObjectItemCaseSensitive(root, "regions")) {
        if (db->nregion >= WG_MAX_REGIONS) { ok = false; break; }
        RegionDef *d = &db->region[db->nregion++];
        snprintf(d->id, sizeof d->id, "%s", str(r, "id")); snprintf(d->name, sizeof d->name, "%s", str(r, "name"));
        d->tier = (int)num(r, "tier", 1);
        const cJSON *t = cJSON_GetObjectItemCaseSensitive(r, "terrain");
        d->terrain = (TerrainParams){.base = num(t, "base", 0), .amp = num(t, "amp", 4), .freq = num(t, "freq", 0.02f), .ridge = num(t, "ridge", 0), .rim = num(t, "rim", 1)};
        const cJSON *g = cJSON_GetObjectItemCaseSensitive(r, "ground");
        rgb8(g, "low", d->ground_low); rgb8(g, "high", d->ground_high); rgb8(g, "cliff", d->ground_cliff); rgb8(g, "accent", d->ground_accent);
        d->tint = (BiomeTint){rgbf(r, "sky_tint"), rgbf(r, "fog_tint"), num(r, "fog_add", 0)};
        const cJSON *wx = cJSON_GetObjectItemCaseSensitive(r, "weather");
        for (int k = 0; k < WX_COUNT; ++k) d->weather[k] = num(wx, weather_name((WeatherKind)k), 0);
        const cJSON *p;
        cJSON_ArrayForEach(p, cJSON_GetObjectItemCaseSensitive(r, "props")) {
            int k = prop_from(cJSON_GetStringValue(cJSON_GetArrayItem(p, 0)) ? cJSON_GetStringValue(cJSON_GetArrayItem(p, 0)) : "");
            if (k < 0 || d->nprops >= 8) { LOGE("regions.json: bad prop in %s", d->id); ok = false; continue; }
            d->props[d->nprops].kind = (PropKind)k; d->props[d->nprops++].density = (float)cJSON_GetArrayItem(p, 1)->valuedouble;
        }
        cJSON_ArrayForEach(p, cJSON_GetObjectItemCaseSensitive(r, "enemies")) {
            if (d->nenemies >= 12) { ok = false; break; }
            const char *eid = cJSON_GetStringValue(cJSON_GetArrayItem(p, 0));
            int def = eid ? enemydb_find(edb, eid) : -1;
            if (def < 0) { LOGE("regions.json: unknown enemy '%s' in %s", eid ? eid : "?", d->id); ok = false; continue; }
            snprintf(d->enemies[d->nenemies].enemy, sizeof d->enemies[0].enemy, "%s", eid);
            d->enemies[d->nenemies].weight = (float)cJSON_GetArrayItem(p, 1)->valuedouble; d->enemies[d->nenemies++].def = def;
        }
        const cJSON *dg = cJSON_GetObjectItemCaseSensitive(r, "dungeon");
        rgb8(dg, "wall", d->dng_wall); rgb8(dg, "floor", d->dng_floor); rgb8(dg, "torch", d->dng_torch);
    }
    cJSON_Delete(root);

    snprintf(path, sizeof path, "%s/zones.json", dir);
    text = read_all(path);
    if (!text) return false;
    root = cJSON_Parse(text); free(text);
    if (!root) { LOGE("zones.json: parse error"); return false; }
    cJSON_ArrayForEach(r, cJSON_GetObjectItemCaseSensitive(root, "zones")) {
        if (db->nzone >= WG_MAX_ZONES) { ok = false; break; }
        ZoneDef *z = &db->zone[db->nzone++];
        snprintf(z->id, sizeof z->id, "%s", str(r, "id"));
        z->region = worlddb_region(db, str(r, "region"));
        if (z->region < 0) { LOGE("zones.json: zone %s has unknown region", z->id); ok = false; z->region = 0; }
        const char *gen = str(r, "gen");
        z->gen = !strcmp(gen, "hub") ? ZG_HUB : !strcmp(gen, "dungeon") ? ZG_DUNGEON : ZG_FIELD;
        const cJSON *m = cJSON_GetObjectItemCaseSensitive(r, "map");
        z->map[0] = (float)cJSON_GetArrayItem(m, 0)->valuedouble; z->map[1] = (float)cJSON_GetArrayItem(m, 1)->valuedouble;
        const cJSON *b;
        cJSON_ArrayForEach(b, cJSON_GetObjectItemCaseSensitive(r, "bosses")) {
            if (z->nbosses >= 4) { ok = false; break; }
            ZoneBoss *zb = &z->bosses[z->nbosses++];
            snprintf(zb->enemy, sizeof zb->enemy, "%s", str(b, "enemy")); snprintf(zb->quest, sizeof zb->quest, "%s", str(b, "quest"));
            snprintf(zb->at, sizeof zb->at, "%s", str(b, "at")); zb->ng_plus = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(b, "ng"));
            zb->def = enemydb_find(edb, zb->enemy);
            if (zb->def < 0) { LOGE("zones.json: unknown boss '%s' in %s", zb->enemy, z->id); ok = false; }
        }
    }
    cJSON_Delete(root);
    if (db->nregion == 0 || db->nzone == 0) ok = false;
    if (ok) build_links(db);
    return ok;
}

int zone_pick_enemy(const WorldDB *db, int region, Rng *rng) {
    const RegionDef *r = &db->region[region];
    float total = 0; for (int i = 0; i < r->nenemies; ++i) total += r->enemies[i].weight;
    if (total <= 0) return -1;
    float x = rng_float(rng) * total;
    for (int i = 0; i < r->nenemies; ++i) { if (x < r->enemies[i].weight) return r->enemies[i].def; x -= r->enemies[i].weight; }
    return r->enemies[r->nenemies - 1].def;
}

/* ---------------- zone generation ---------------- */
static float terrain_h(const void *u, float x, float z) { return terrain_height((const Terrain *)u, x, z); }

float zone_height(const Zone *z, float x, float zz) { return z->gen == ZG_DUNGEON ? 0.0f : terrain_height(&z->terrain, x, zz); }

typedef struct { float r_solid, h; } PropShape;
static PropShape prop_shape(PropKind k, float s) {
    switch (k) {
        case PROP_TREE: return (PropShape){0.55f * s, 4.5f * s};
        case PROP_PINE: return (PropShape){0.5f * s, 5.0f * s};
        case PROP_ROCK: return (PropShape){1.0f * s, 1.6f * s};
        case PROP_CACTUS: return (PropShape){0.5f * s, 2.8f * s};
        case PROP_DEADTREE: return (PropShape){0.4f * s, 3.6f * s};
        case PROP_ICESPIKE: return (PropShape){0.8f * s, 3.0f * s};
        case PROP_CRYSTAL: return (PropShape){0.6f * s, 2.4f * s};
        case PROP_LAVAROCK: return (PropShape){1.1f * s, 1.5f * s};
        case PROP_PILLAR: return (PropShape){0.7f * s, 4.0f * s};
        default: return (PropShape){0, 0};   /* bush, bones, lamp: decoration only */
    }
}
static bool prop_is_solid(PropKind k) { return prop_shape(k, 1.0f).r_solid > 0; }

static float dist2d(float ax, float az, float bx, float bz) { return hypotf(ax - bx, az - bz); }

static void add_prop(Zone *z, PropKind k, float x, float zz, float yaw, float scale) {
    if (z->nprops >= WG_MAX_PROPS) return;
    z->props[z->nprops++] = (Prop){k, x, zone_height(z, x, zz), zz, yaw, scale};
}

static bool too_close(const Zone *z, float x, float zz, float r) {
    for (int i = 0; i < z->nprops; ++i) {
        const Prop *p = &z->props[i];
        if (!prop_is_solid(p->kind)) continue;
        float rr = prop_shape(p->kind, p->scale).r_solid + r;
        if (fabsf(p->x - x) < rr && fabsf(p->z - zz) < rr && dist2d(p->x, p->z, x, zz) < rr) return true;
    }
    return false;
}

/* exits: one portal per neighbour at 66 m in the direction of the neighbour on the world map */
static void place_exits(Zone *z, const WorldDB *db) {
    const ZoneDef *zd = &db->zone[z->zone];
    float used[WG_MAX_EXITS]; int nused = 0;
    for (int k = 0; k < zd->nexits; ++k) {
        const ZoneDef *nb = &db->zone[zd->exit_to[k]];
        float ang = atan2f(nb->map[1] - zd->map[1], nb->map[0] - zd->map[0]);
        for (int tries = 0; tries < 12; ++tries) {   /* keep portals apart */
            bool clash = false;
            for (int u = 0; u < nused; ++u) { float d = fabsf(remainderf(ang - used[u], 6.2831853f)); if (d < 0.5f) { clash = true; break; } }
            if (!clash) break;
            ang += 0.55f;
        }
        used[nused++] = ang;
        z->exits[z->nexits++] = (ZoneExit){cosf(ang) * 66.0f, sinf(ang) * 66.0f, zd->exit_to[k]};
    }
}

static void gen_hub(Zone *z, const WorldDB *db, Rng *rng) {
    (void)db;
    z->center = vec3(0, 0, 0);
    /* buildings in a ring, doors facing the plaza */
    int n = rng_int(rng, 7, 10);
    for (int i = 0; i < n && z->nbld < WG_MAX_BLD; ++i) {
        float a = (float)i / (float)n * 6.2831853f + rng_range(rng, -0.12f, 0.12f);
        float rad = rng_range(rng, 48.0f, 55.0f);
        float cx = cosf(a) * rad, cz = sinf(a) * rad, hw = rng_range(rng, 2.6f, 4.0f), hd = rng_range(rng, 2.4f, 3.4f);
        bool clash = false;
        for (int k = 0; k < z->nexits; ++k) if (dist2d(cx, cz, z->exits[k].x * 0.78f, z->exits[k].z * 0.78f) < 8.0f) clash = true;   /* keep the roads clear */
        if (clash) continue;
        uint8_t shade = (uint8_t)rng_int(rng, 0, 40);
        z->bld[z->nbld++] = (Building){cx - hw, cz - hd, cx + hw, cz + hd, rng_range(rng, 3.4f, 5.5f), {(uint8_t)(150 + shade), (uint8_t)(130 + shade / 2), (uint8_t)(110 + shade / 3)}};
    }
    /* centre piece and lamps */
    z->props[z->nprops++] = (Prop){PROP_CRYSTAL, 0, 0, 0, 0, 1.4f};
    for (int i = 0; i < 8; ++i) { float a = (float)i / 8.0f * 6.2831853f + 0.2f; add_prop(z, PROP_LAMP, cosf(a) * 12.0f, sinf(a) * 12.0f, 0, 1.0f); }
}

static void gen_field(Zone *z, const WorldDB *db, Rng *rng, bool has_boss) {
    (void)db; (void)has_boss;
    z->center = vec3(0, 0, 0);
    /* a ruined ring around the arena at the centre */
    int n = rng_int(rng, 6, 9);
    for (int i = 0; i < n; ++i) {
        float a = (float)i / (float)n * 6.2831853f;
        if (rng_float(rng) < 0.25f) continue;   /* broken gaps */
        add_prop(z, PROP_PILLAR, cosf(a) * 10.5f, sinf(a) * 10.5f, a, rng_range(rng, 0.7f, 1.1f));
    }
}

static uint32_t checksum_zone(const Zone *z) {
    uint32_t h = 2166136261u;
#define MIXF(f) do { int32_t q = (int32_t)lroundf((f) * 64.0f); h = (h ^ (uint32_t)q) * 16777619u; } while (0)
    for (int i = 0; i < z->nprops; ++i) { MIXF(z->props[i].x); MIXF(z->props[i].z); MIXF(z->props[i].y); h = (h ^ (uint32_t)z->props[i].kind) * 16777619u; }
    for (int i = 0; i < z->nbld; ++i) { MIXF(z->bld[i].x0); MIXF(z->bld[i].z0); MIXF(z->bld[i].h); }
    for (int i = 0; i < z->nfoes; ++i) { MIXF(z->foes[i].x); MIXF(z->foes[i].z); }
    for (int i = 0; i < z->nexits; ++i) { MIXF(z->exits[i].x); MIXF(z->exits[i].z); }
    for (int i = 0; i < TERRAIN_N * TERRAIN_N; i += 7) MIXF(z->terrain.h[i]);
    for (int zz = 0; zz < DG_N; ++zz) for (int x = 0; x < DG_N; ++x) h = (h ^ z->dg.cell[zz][x]) * 16777619u;
    MIXF(z->spawn.x); MIXF(z->spawn.z); MIXF(z->boss_pos.x); MIXF(z->boss_pos.z);
#undef MIXF
    return h;
}

void zone_generate(Zone *z, const WorldDB *db, int zi) {
    memset(z, 0, sizeof *z);
    const ZoneDef *zd = &db->zone[zi];
    const RegionDef *rd = &db->region[zd->region];
    z->zone = zi; z->region = zd->region; z->gen = zd->gen;
    z->seed = hash_str(zd->id);
    Rng rng; rng_seed(&rng, z->seed);
    place_exits(z, db);

    if (zd->gen == ZG_DUNGEON) {
        dungeon_generate(&z->dg, z->seed);
        z->bound = 0.5f * DG_N * DG_CELL - 1.0f;
        const DgRoom *s = &z->dg.rooms[z->dg.start_room], *b = &z->dg.rooms[z->dg.boss_room];
        z->spawn = dungeon_cell_center(s->x + s->w / 2, s->z + s->h / 2);
        z->center = z->boss_pos = dungeon_cell_center(b->x + b->w / 2, b->z + b->h / 2);
        for (int i = 0; i < z->dg.nspawns && z->nfoes < 26; ++i) z->foes[z->nfoes++] = (ZonePt){z->dg.spawns[i].x, z->dg.spawns[i].z};   /* a crowd the AI and GPU can afford */
        /* a single portal back out, in the start room (pulled off the spawn so arrival doesn't trigger it) */
        z->nexits = 0;
        if (zd->nexits > 0) z->exits[z->nexits++] = (ZoneExit){z->spawn.x + 3.0f * DG_CELL * 0.0f + 2.5f, z->spawn.z - 2.5f, zd->exit_to[0]};
        z->spawn.x -= 1.5f; z->spawn.z += 1.5f;
        z->checksum = checksum_zone(z);
        return;
    }

    TerrainParams tp = rd->terrain;
    tp.seed = z->seed;
    tp.flat_x = tp.flat_z = 0;
    tp.flat_r = zd->gen == ZG_HUB ? 44.0f : 14.0f;
    terrain_generate(&z->terrain, &tp);
    z->bound = TERRAIN_HALF - 4.0f;
    /* level the plateau exactly: the arena/plaza is flat ground at base height */
    if (zd->gen == ZG_HUB) gen_hub(z, db, &rng); else gen_field(z, db, &rng, zd->nbosses > 0);
    z->center.y = zone_height(z, 0, 0);
    z->boss_pos = z->center;

    /* scatter biome props away from the centre feature, portals and the arrival road */
    float keep_clear = zd->gen == ZG_HUB ? 46.0f : 16.0f;
    for (int e = 0; e < rd->nprops; ++e) {
        int count = (int)(rd->props[e].density * 25.6f);
        for (int i = 0; i < count && z->nprops < WG_MAX_PROPS - 4; ++i) {
            float x = rng_range(&rng, -(TERRAIN_HALF - 3.0f), TERRAIN_HALF - 3.0f), zz = rng_range(&rng, -(TERRAIN_HALF - 3.0f), TERRAIN_HALF - 3.0f);
            if (dist2d(x, zz, 0, 0) < keep_clear) continue;
            bool near_exit = false;
            for (int k = 0; k < z->nexits; ++k) if (dist2d(x, zz, z->exits[k].x, z->exits[k].z) < 9.0f) near_exit = true;
            if (near_exit) continue;
            PropKind pk = rd->props[e].kind;
            if (terrain_slope(&z->terrain, x, zz) > (pk == PROP_ROCK || pk == PROP_LAVAROCK || pk == PROP_ICESPIKE ? 0.45f : 0.12f)) continue;
            float sc = rng_range(&rng, 0.8f, 1.35f);
            if (prop_is_solid(pk) && too_close(z, x, zz, 1.2f)) continue;
            add_prop(z, pk, x, zz, rng_range(&rng, 0.0f, 6.28f), sc);
        }
    }

    /* arrival: a point near the first portal, pulled inward */
    z->spawn = vec3(0, 0, 8.0f);
    if (zd->gen == ZG_HUB) z->spawn = vec3(0, 0, 6.0f);
    z->spawn.y = zone_height(z, z->spawn.x, z->spawn.z);

    /* enemy packs (fields only; hubs are safe) */
    if (zd->gen == ZG_FIELD) {
        int packs = rng_int(&rng, 5, 8);
        for (int p = 0; p < packs && z->nfoes < WG_MAX_FOES - 3; ++p) {
            float a = rng_range(&rng, 0.0f, 6.2831853f), rad = rng_range(&rng, 26.0f, 62.0f);
            float cx = cosf(a) * rad, cz = sinf(a) * rad;
            int size = rng_int(&rng, 2, 3);
            for (int k = 0; k < size; ++k) {
                float x = cx + rng_range(&rng, -3.0f, 3.0f), zz = cz + rng_range(&rng, -3.0f, 3.0f);
                if (too_close(z, x, zz, 0.8f) || terrain_slope(&z->terrain, x, zz) > 0.3f) continue;
                z->foes[z->nfoes++] = (ZonePt){x, zz};
            }
        }
    }
    z->checksum = checksum_zone(z);
}

Vec3 zone_entry(const Zone *z, int from) {
    for (int i = 0; i < z->nexits; ++i) {
        if (z->exits[i].dest != from) continue;
        float l = hypotf(z->exits[i].x, z->exits[i].z);
        if (z->gen == ZG_DUNGEON) return z->spawn;
        float x = z->exits[i].x * (l - 7.0f) / l, zz = z->exits[i].z * (l - 7.0f) / l;
        return vec3(x, zone_height(z, x, zz), zz);
    }
    return z->spawn;
}

void zone_build_physics(const Zone *z, PhysWorld *w) {
    phys_init(w, 0.0f);
    w->bound = z->bound;
    if (z->gen == ZG_DUNGEON) {
        DgWallRect rects[320];
        int n = dungeon_wall_rects(&z->dg, rects, 320);
        for (int i = 0; i < n; ++i) phys_add_box(w, vec3(rects[i].x0, -1.0f, rects[i].z0), vec3(rects[i].x1, 6.0f, rects[i].z1));
        return;
    }
    w->height = terrain_h; w->height_user = &z->terrain;
    for (int i = 0; i < z->nprops; ++i) {
        const Prop *p = &z->props[i];
        PropShape s = prop_shape(p->kind, p->scale);
        if (s.r_solid > 0) phys_add_cyl(w, p->x, p->z, s.r_solid, p->y - 1.0f, p->y + s.h);
    }
    for (int i = 0; i < z->nbld; ++i) {
        const Building *b = &z->bld[i];
        float gy = zone_height(z, 0.5f * (b->x0 + b->x1), 0.5f * (b->z0 + b->z1));
        phys_add_box(w, vec3(b->x0, gy - 1.0f, b->z0), vec3(b->x1, gy + b->h, b->z1));
    }
}
