#include "testutil.h"
#include <string.h>
#include "core/worldgen.h"

static SkillDB S; static EnemyDB E; static WorldDB W; static Zone Z, Z2; static PhysWorld P;

static bool free_of_solids(const Zone *z, float x, float zz, float r) {
    for (int i = 0; i < z->nprops; ++i) {
        const Prop *p = &z->props[i];
        float pr = p->kind == PROP_TREE ? 0.55f : p->kind == PROP_ROCK ? 1.0f : 0.0f;
        if (pr > 0 && hypotf(p->x - x, p->z - zz) < pr * p->scale + r) return false;
    }
    return true;
}

int main(void) {
    /* terrain: deterministic, bounded, plateau flat */
    Terrain a, b;
    TerrainParams tp = {.seed = 7, .base = 0, .amp = 8, .freq = 0.02f, .ridge = 0.3f, .flat_r = 20, .rim = 1};
    terrain_generate(&a, &tp); terrain_generate(&b, &tp);
    CHECK(memcmp(a.h, b.h, sizeof a.h) == 0);
    CHECK_NEAR(terrain_height(&a, 0, 0), 0.0, 1e-3); CHECK_NEAR(terrain_height(&a, 10, -10), 0.0, 1e-3);
    CHECK(a.max_h > 2.0f);
    CHECK(terrain_height(&a, TERRAIN_HALF - 1, TERRAIN_HALF - 1) > terrain_height(&a, 0, 40));   /* the rim rises */
    Vec3 n = terrain_normal(&a, 0, 0); CHECK(n.y > 0.99f);
    tp.seed = 8; terrain_generate(&b, &tp); CHECK(memcmp(a.h, b.h, sizeof a.h) != 0);

    /* terrain collision: walking along a slope follows the ground, the zone bound holds */
    PhysWorld *pw = &P;
    phys_init(pw, 0); pw->height = (float (*)(const void *, float, float))NULL;
    tp.seed = 7; terrain_generate(&a, &tp);
    Zone *zz = &Z;
    memset(zz, 0, sizeof *zz); zz->terrain = a; zz->gen = ZG_FIELD; zz->bound = 70.0f;
    zone_build_physics(zz, pw);
    CharCtl c; char_init(&c, vec3(30, 30, 30));
    for (int i = 0; i < 200; ++i) char_step(pw, &c, (CharMove){0, 0, false}, 1.0f / 60.0f);
    CHECK(c.grounded); CHECK_NEAR(c.pos.y, terrain_height(&a, c.pos.x, c.pos.z), 0.02);
    float maxdev = 0;
    for (int i = 0; i < 400; ++i) { char_step(pw, &c, (CharMove){3, 1, false}, 1.0f / 60.0f); float d = fabsf(c.pos.y - terrain_height(&a, c.pos.x, c.pos.z)); if (d > maxdev) maxdev = d; }
    CHECK(maxdev < 0.5f);
    for (int i = 0; i < 1200; ++i) char_step(pw, &c, (CharMove){8, 8, false}, 1.0f / 60.0f);
    CHECK(c.pos.x <= 70.01f && c.pos.z <= 70.01f);
    Vec3 o = vec3(0, 40, 0), down = vec3(0, -1, 0);
    float hit = phys_raycast(pw, o, down, 100.0f);
    CHECK_NEAR(40.0f - hit, terrain_height(&a, 0, 0), 0.05);

    /* the shipped data */
    CHECK(skilldb_load(&S, SA_DATA_DIR "/skills.json"));
    CHECK(enemydb_load(&E, &S, SA_DATA_DIR "/enemies.json"));
    CHECK(worlddb_load(&W, SA_DATA_DIR, &E));
    CHECK(W.nregion == 5); CHECK(W.nzone >= 40);
    int dungeons = 0, hubs = 0, fields = 0;
    for (int i = 0; i < W.nzone; ++i) { dungeons += W.zone[i].gen == ZG_DUNGEON; hubs += W.zone[i].gen == ZG_HUB; fields += W.zone[i].gen == ZG_FIELD; }
    CHECK(dungeons >= 10); CHECK(hubs >= 8); CHECK(fields >= 15);
    int aur = worlddb_zone(&W, "aurelheim");
    CHECK(aur >= 0);

    /* every zone: travel links are symmetric and everything is reachable from Aurelheim */
    bool seen[WG_MAX_ZONES] = {false}; int stack[WG_MAX_ZONES], sp = 0;
    stack[sp++] = aur; seen[aur] = true;
    while (sp) { int u = stack[--sp]; for (int k = 0; k < W.zone[u].nexits; ++k) { int v = W.zone[u].exit_to[k]; bool back = false; for (int q = 0; q < W.zone[v].nexits; ++q) back |= W.zone[v].exit_to[q] == u; CHECK(back); if (!seen[v]) { seen[v] = true; stack[sp++] = v; } } }
    int unreachable = 0; for (int i = 0; i < W.nzone; ++i) unreachable += !seen[i];
    CHECK(unreachable == 0);
    for (int i = 0; i < W.nzone; ++i) if (W.zone[i].gen == ZG_DUNGEON) CHECK(W.zone[i].nexits >= 1);

    /* generate all zones: deterministic, spawn/foes/exits sane, dungeons connected with boss far from the start */
    for (int i = 0; i < W.nzone; ++i) {
        zone_generate(&Z, &W, i); zone_generate(&Z2, &W, i);
        CHECK(Z.checksum == Z2.checksum && Z.checksum != 0);
        CHECK(Z.nexits == (W.zone[i].gen == ZG_DUNGEON ? 1 : W.zone[i].nexits));
        if (Z.gen == ZG_DUNGEON) {
            CHECK(Z.dg.nrooms >= 6);
            CHECK(dungeon_reachable(&Z.dg) == Z.dg.floor_cells);
            CHECK(Z.dg.start_room != Z.dg.boss_room);
            CHECK(dungeon_is_floor_at(&Z.dg, Z.spawn.x, Z.spawn.z)); CHECK(dungeon_is_floor_at(&Z.dg, Z.boss_pos.x, Z.boss_pos.z));
            CHECK(hypotf(Z.spawn.x - Z.boss_pos.x, Z.spawn.z - Z.boss_pos.z) > 20.0f);
            CHECK(Z.nfoes >= 8);
            for (int k = 0; k < Z.nfoes; ++k) CHECK(dungeon_is_floor_at(&Z.dg, Z.foes[k].x, Z.foes[k].z));
            for (int k = 0; k < Z.nexits; ++k) CHECK(dungeon_is_floor_at(&Z.dg, Z.exits[k].x, Z.exits[k].z));
            zone_build_physics(&Z, &P);
            CHECK(P.nboxes > 20 && P.nboxes < PH_MAX_BOXES);
        } else {
            CHECK(Z.nprops > 30);
            CHECK(free_of_solids(&Z, Z.spawn.x, Z.spawn.z, 0.5f));
            CHECK(fabsf(Z.spawn.y - terrain_height(&Z.terrain, Z.spawn.x, Z.spawn.z)) < 0.01f);
            for (int k = 0; k < Z.nfoes; ++k) CHECK(free_of_solids(&Z, Z.foes[k].x, Z.foes[k].z, 0.3f));
            for (int k = 0; k < Z.nexits; ++k) { CHECK(fabsf(Z.exits[k].x) < Z.bound && fabsf(Z.exits[k].z) < Z.bound); CHECK(terrain_slope(&Z.terrain, Z.exits[k].x, Z.exits[k].z) < 0.5f); }
            if (Z.gen == ZG_HUB) CHECK(Z.nfoes == 0 && Z.nbld >= 4); else CHECK(Z.nfoes >= 8);
            zone_build_physics(&Z, &P);
            CHECK(P.nboxes <= PH_MAX_BOXES && P.ncyls <= PH_MAX_CYLS);
            /* a character dropped at the spawn stands there and is not inside a solid */
            CharCtl ch; char_init(&ch, vec3(Z.spawn.x, Z.spawn.y + 1.0f, Z.spawn.z));
            for (int k = 0; k < 90; ++k) char_step(&P, &ch, (CharMove){0, 0, false}, 1.0f / 60.0f);
            CHECK(ch.grounded && hypotf(ch.pos.x - Z.spawn.x, ch.pos.z - Z.spawn.z) < 0.6f);
        }
        /* arriving from each neighbour lands inside the zone */
        for (int k = 0; k < Z.nexits; ++k) { Vec3 e = zone_entry(&Z, Z.exits[k].dest); CHECK(fabsf(e.x) < Z.bound + 0.1f && fabsf(e.z) < Z.bound + 0.1f); }
    }
    /* different zones differ */
    zone_generate(&Z, &W, 0); zone_generate(&Z2, &W, 1); CHECK(Z.checksum != Z2.checksum);

    /* region enemy tables resolve to real enemies */
    Rng rng; rng_seed(&rng, 5);
    for (int r = 0; r < W.nregion; ++r) { CHECK(W.region[r].nenemies >= 3); for (int i = 0; i < 20; ++i) CHECK(zone_pick_enemy(&W, r, &rng) >= 0); }
    /* every quest boss that has a marker zone is placed */
    int bosses = 0; for (int i = 0; i < W.nzone; ++i) bosses += W.zone[i].nbosses;
    CHECK(bosses >= 12);

    /* a broken data directory fails cleanly */
    CHECK(!worlddb_load(&W, SA_DATA_DIR "/nope", &E));
    TEST_DONE();
}
