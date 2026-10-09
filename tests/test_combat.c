#include "testutil.h"
#include <string.h>
#include "core/combat.h"

static World W;
static CombatWorld CW;
static PhysWorld PH;
static SkillDB S;
static const float DT = 1.0f / 60.0f;

static int sk(const char *id) { int i = skilldb_find(&S, id); if (i < 0) { printf("missing skill %s\n", id); exit(2); } return i; }

static Entity spawn(int team, Vec3 pos, float yaw, Stats st, int level, float poise) {
    Entity e = ecs_create(&W, C_TRANSFORM | C_HEALTH);
    W.pos[ecs_index(e)] = pos; W.yaw[ecs_index(e)] = yaw;
    combat_add(&CW, &W, e, team, level, st, poise);
    return e;
}
static void run(float seconds) { for (int i = 0; i < (int)(seconds * 60.0f + 0.5f); ++i) combat_update(&CW, &W, &PH, DT); }
static void reset(void) {
    ecs_init(&W); phys_init(&PH, 0.0f);
    uint32_t seed = CW.rng; combat_init(&CW, &S); CW.rng = seed ? seed : 777u; CW.awakening = 5;
}
static const Stats HERO = {10, 4, 8, 6, 6}, GRUNT = {9, 4, 8, 4, 6};
static int count_events(CombatEventType t) { int n = 0; for (int i = 0; i < CW.nev; ++i) if (CW.ev[i].type == t) n++; return n; }

int main(void) {
    CHECK(skilldb_load(&S, SA_DATA_DIR "/skills.json"));
    reset();

    /* --- formulas --- */
    Entity h = spawn(TEAM_PLAYER, vec3(0, 0, 0), 0, HERO, 1, 100);
    Entity g = spawn(TEAM_ENEMY, vec3(0, 0, 2), 3.14159f, GRUNT, 1, 100);
    Combatant *ch = combat_get(&CW, h), *cg = combat_get(&CW, g);
    const Skill *slash = &S.s[sk("heroic_slash")], *fire = &S.s[sk("fireball")];
    float base = combat_base_damage(ch, slash, cg);
    CHECK(base > 5.0f);
    cg->def += 50; CHECK(combat_base_damage(ch, slash, cg) < base); cg->def -= 50;
    cg->resist[EL_FIRE] = 1.5f; float weak = combat_base_damage(ch, fire, cg); cg->resist[EL_FIRE] = 0.5f; float res = combat_base_damage(ch, fire, cg);
    CHECK(weak > res * 2.5f); cg->resist[EL_FIRE] = 1.0f;
    float before = combat_base_damage(ch, slash, cg); ch->level = 10; combatant_recalc(ch); CHECK(combat_base_damage(ch, slash, cg) > before * 1.5f); ch->level = 1; combatant_recalc(ch);
    CHECK(ch->max_hp > 150 && ch->max_mp > 50);

    /* --- cast gating --- */
    int hs = sk("heroic_slash"), sunder = sk("sunder");
    CHECK(combat_begin_cast(&CW, &W, h, hs, g, 0));
    CHECK(!combat_begin_cast(&CW, &W, h, sunder, g, 0));          /* already casting */
    run(S.s[hs].total_time + 0.05f);
    CHECK(!combat_begin_cast(&CW, &W, h, hs, g, 0));              /* cooldown */
    run(S.s[hs].cooldown);
    ch->mp = 0; CHECK(!combat_begin_cast(&CW, &W, h, sunder, g, 0)); ch->mp = ch->max_mp;
    ch->status[ST_STUN].time = 1; CHECK(!combat_can_cast(&CW, ch, sunder)); ch->status[ST_STUN].time = 0;
    int fb = sk("fireball"); ch->status[ST_SILENCE].time = 1; CHECK(!combat_can_cast(&CW, ch, fb)); CHECK(combat_can_cast(&CW, ch, sunder)); ch->status[ST_SILENCE].time = 0;

    /* --- melee arc: timing, front vs back --- */
    reset();
    h = spawn(TEAM_PLAYER, vec3(0, 0, 0), 0, HERO, 1, 100); g = spawn(TEAM_ENEMY, vec3(0, 0, 2), 0, GRUNT, 1, 1000);
    Entity behind = spawn(TEAM_ENEMY, vec3(0, 0, -2), 0, GRUNT, 1, 1000);
    ch = combat_get(&CW, h); cg = combat_get(&CW, g); Combatant *cb = combat_get(&CW, behind);
    hs = sk("heroic_slash");
    combat_begin_cast(&CW, &W, h, hs, ENTITY_NULL, 0);
    run(S.s[hs].hit_time * 0.5f); CHECK(cg->hp == cg->max_hp);    /* wind-up: nothing yet */
    run(S.s[hs].hit_time * 0.5f + 0.05f);
    CHECK(cg->hp < cg->max_hp); CHECK(cb->hp == cb->max_hp);      /* hits the front only */
    CHECK(count_events(CE_HIT) == 1);
    run(1.0f); CHECK(count_events(CE_HIT) == 1);                  /* effect fires exactly once */

    /* --- aoe radius --- */
    reset();
    h = spawn(TEAM_PLAYER, vec3(0, 0, 0), 0, HERO, 1, 100);
    Entity near_ = spawn(TEAM_ENEMY, vec3(2, 0, 0), 0, GRUNT, 1, 1000), far_ = spawn(TEAM_ENEMY, vec3(9, 0, 0), 0, GRUNT, 1, 1000);
    ch = combat_get(&CW, h); ch->mp = ch->max_mp = 200;
    combat_begin_cast(&CW, &W, h, sk("rising_slash"), ENTITY_NULL, 0); run(1.0f);
    CHECK(combat_get(&CW, near_)->hp < combat_get(&CW, near_)->max_hp); CHECK(combat_get(&CW, far_)->hp == combat_get(&CW, far_)->max_hp);

    /* --- guard, parry, barrier --- */
    reset();
    h = spawn(TEAM_PLAYER, vec3(0, 0, 0), 0, HERO, 1, 100); g = spawn(TEAM_ENEMY, vec3(0, 0, 1.5f), 3.14159f, GRUNT, 1, 1000);
    ch = combat_get(&CW, h); cg = combat_get(&CW, g);
    int ss = sk("shadow_slash");
    combat_begin_cast(&CW, &W, g, ss, h, 3.14159f); run(S.s[ss].hit_time + 0.02f);
    float unguarded = ch->max_hp - ch->hp; CHECK(unguarded > 0);
    ch->hp = ch->max_hp; run(3.0f);                                 /* let the cooldown pass */
    CW.nev = 0;
    combat_set_guard(&CW, h, true); run(0.5f);                      /* held guard past the parry window */
    combat_begin_cast(&CW, &W, g, ss, h, 3.14159f); run(S.s[ss].hit_time + 0.02f);
    float guarded = ch->max_hp - ch->hp; CHECK(guarded > 0 && guarded < unguarded * 0.6f); CHECK(count_events(CE_BLOCK) == 1);
    ch->hp = ch->max_hp; run(3.0f); CW.nev = 0;
    combat_set_guard(&CW, h, false); combat_set_guard(&CW, h, true);
    combat_begin_cast(&CW, &W, g, ss, h, 3.14159f); run(S.s[ss].hit_time - 0.1f); /* attack lands ~0.1 s after raising guard: perfect parry */
    ch->guard_time = 0.05f; run(0.12f);
    CHECK(ch->hp == ch->max_hp); CHECK(count_events(CE_PARRY) == 1); CHECK(cg->stagger_t > 0.5f);
    /* guard does not help against attacks from behind */
    ch->hp = ch->max_hp; cg->stagger_t = 0; W.pos[ecs_index(g)] = vec3(0, 0, -1.5f); run(3.0f);
    combat_set_guard(&CW, h, false); combat_set_guard(&CW, h, true); run(0.5f);
    combat_begin_cast(&CW, &W, g, ss, h, 0); run(S.s[ss].hit_time + 0.02f);
    CHECK(ch->max_hp - ch->hp > unguarded * 0.7f);
    /* barrier absorbs before HP */
    ch->hp = ch->max_hp; ch->guarding = false; ch->barrier = 1000; run(3.0f);
    combat_begin_cast(&CW, &W, g, ss, h, 0); run(S.s[ss].hit_time + 0.02f);
    CHECK(ch->hp == ch->max_hp); CHECK(ch->barrier < 1000);

    /* --- poise / stagger interrupts a cast --- */
    reset();
    h = spawn(TEAM_PLAYER, vec3(0, 0, 0), 0, HERO, 1, 100); g = spawn(TEAM_ENEMY, vec3(0, 0, 1.5f), 3.14159f, GRUNT, 1, 40);
    cg = combat_get(&CW, g);
    combat_begin_cast(&CW, &W, g, ss, h, 3.14159f); run(0.1f); CHECK(cg->cast_skill >= 0);
    combat_begin_cast(&CW, &W, h, sk("sunder"), ENTITY_NULL, 0); run(S.s[sk("sunder")].hit_time + 0.05f);
    CHECK(cg->stagger_t > 0); CHECK(cg->cast_skill < 0); CHECK(count_events(CE_STAGGER) >= 1);
    CHECK(cg->status[ST_DEF_DOWN].time > 0);                       /* sunder breaks armor */

    /* --- DoT, healing, hollow curse, cleanse --- */
    reset();
    h = spawn(TEAM_PLAYER, vec3(0, 0, 0), 0, HERO, 1, 100);
    Entity healer = spawn(TEAM_PLAYER, vec3(1, 0, 0), 0, (Stats){4, 12, 6, 14, 6}, 1, 100);
    ch = combat_get(&CW, h); Combatant *chl = combat_get(&CW, healer);
    ch->status[ST_BURN] = (Status){4.0f, 6.0f, ENTITY_NULL};
    run(2.1f); CHECK(ch->hp < ch->max_hp - 11.0f && ch->hp > ch->max_hp - 13.0f);
    float lowered = ch->hp;
    combat_begin_cast(&CW, &W, healer, sk("heal"), h, 0); run(1.0f); CHECK(ch->hp > lowered);
    ch->status[ST_HOLLOW_CURSE].time = 5; lowered = ch->hp; run(3.0f);
    combat_begin_cast(&CW, &W, healer, sk("heal"), h, 0); run(1.0f); CHECK(ch->hp <= lowered + 0.01f);  /* curse blocks healing */
    chl->mp = chl->max_mp;
    combat_begin_cast(&CW, &W, healer, sk("purify_curse"), h, 0); run(1.0f); CHECK(ch->status[ST_HOLLOW_CURSE].time <= 0);
    ch->status[ST_BURN].time = 5; run(3.0f);
    combat_begin_cast(&CW, &W, healer, sk("cleanse"), h, 0); run(1.0f); CHECK(ch->status[ST_BURN].time <= 0);

    /* --- projectile: hits, stops at walls, fireball splash --- */
    reset();
    h = spawn(TEAM_PLAYER, vec3(0, 0, 0), 0, (Stats){6, 14, 6, 10, 6}, 1, 100);
    Entity t1 = spawn(TEAM_ENEMY, vec3(0, 0, 8), 0, GRUNT, 1, 1000), t2 = spawn(TEAM_ENEMY, vec3(1.5f, 0, 8), 0, GRUNT, 1, 1000);
    Combatant *c1 = combat_get(&CW, t1), *c2 = combat_get(&CW, t2);
    combat_begin_cast(&CW, &W, h, sk("fireball"), ENTITY_NULL, 0); run(0.2f); CHECK(c1->hp == c1->max_hp);
    run(1.2f); CHECK(c1->hp < c1->max_hp); CHECK(c2->hp < c2->max_hp);          /* splash */
    CHECK(c1->status[ST_BURN].time > 0);
    reset();
    h = spawn(TEAM_PLAYER, vec3(0, 0, 0), 0, (Stats){6, 14, 6, 10, 6}, 1, 100); t1 = spawn(TEAM_ENEMY, vec3(0, 0, 8), 0, GRUNT, 1, 1000);
    phys_add_box(&PH, vec3(-3, 0, 4), vec3(3, 4, 5)); c1 = combat_get(&CW, t1);
    combat_begin_cast(&CW, &W, h, sk("frost_lance"), ENTITY_NULL, 0); run(2.0f);
    CHECK(c1->hp == c1->max_hp);                                                  /* wall blocks the bolt */

    /* --- revive, taunt, resolve, i-frames, dash, gauge --- */
    reset();
    h = spawn(TEAM_PLAYER, vec3(0, 0, 0), 0, HERO, 1, 100);
    healer = spawn(TEAM_PLAYER, vec3(1, 0, 0), 0, (Stats){4, 12, 6, 14, 6}, 1, 100); chl = combat_get(&CW, healer);
    g = spawn(TEAM_ENEMY, vec3(0, 0, 3), 0, GRUNT, 1, 100);
    ch = combat_get(&CW, h); ch->hp = 0; ch->dead = true;
    combat_begin_cast(&CW, &W, healer, sk("revive"), h, 0); run(2.0f); CHECK(!ch->dead && ch->hp > 0);
    int tn = sk("taunt"); Entity tank = spawn(TEAM_PLAYER, vec3(0, 0, 1), 0, (Stats){8, 3, 16, 8, 4}, 1, 200);
    combat_begin_cast(&CW, &W, tank, tn, ENTITY_NULL, 0); run(1.0f);
    CHECK(combat_get(&CW, g)->status[ST_TAUNT].time > 0 && combat_get(&CW, g)->status[ST_TAUNT].src == tank);
    ch->has_resolve = true; ch->hp = 5;
    { Combatant *a = combat_get(&CW, g); a->atk = 5000; combat_begin_cast(&CW, &W, g, ss, h, 3.14159f); W.pos[ecs_index(g)] = vec3(0, 0, 1.2f); W.pos[ecs_index(h)] = vec3(0, 0, 0); run(1.2f);
      CHECK(!ch->dead && ch->hp == 1.0f && ch->resolve_used); }
    ch->hp = ch->max_hp; ch->invuln = true; combat_get(&CW, g)->cd[ss] = 0; combat_begin_cast(&CW, &W, g, ss, h, 3.14159f); run(1.2f);
    CHECK(ch->hp == ch->max_hp);                                                  /* dodge i-frames */
    reset();
    h = spawn(TEAM_PLAYER, vec3(0, 0, 0), 0, HERO, 3, 100); g = spawn(TEAM_ENEMY, vec3(0, 0, 4), 0, GRUNT, 1, 1000);
    ch = combat_get(&CW, h); ch->mp = ch->max_mp = 200;
    phys_add_box(&PH, vec3(-3, 0, 8), vec3(3, 3, 9));
    combat_begin_cast(&CW, &W, h, sk("gale_rush"), ENTITY_NULL, 0); run(0.5f);
    CHECK(ch->dash_pending); CHECK(ch->dash_to.z > 4.0f && ch->dash_to.z < 6.1f); CHECK(combat_get(&CW, g)->hp < combat_get(&CW, g)->max_hp);
    CW.gauge = 0; CHECK(!combat_can_cast(&CW, ch, sk("bond_resonance")));
    for (int i = 0; i < 80; ++i) { combat_get(&CW, g)->hp = combat_get(&CW, g)->max_hp; combat_get(&CW, g)->cd[hs] = 0; ch->cd[hs] = 0; combat_begin_cast(&CW, &W, h, hs, ENTITY_NULL, 0); W.pos[ecs_index(h)] = vec3(0, 0, 3.0f); run(S.s[hs].total_time + 0.02f); }
    CHECK(CW.gauge >= 100.0f); ch->mp = ch->max_mp;
    CHECK(combat_can_cast(&CW, ch, sk("bond_resonance")));

    /* --- crit rate matches the derived chance (statistics) --- */
    reset();
    h = spawn(TEAM_PLAYER, vec3(0, 0, 0), 0, (Stats){10, 4, 8, 6, 30}, 1, 100);
    g = spawn(TEAM_ENEMY, vec3(0, 0, 1.5f), 0, (Stats){1, 1, 500, 1, 1}, 1, 1e9f);
    ch = combat_get(&CW, h); cg = combat_get(&CW, g); cg->max_hp = cg->hp = 1e9f;
    int crits = 0, hits = 0;
    for (int i = 0; i < 3000; ++i) {
        ch->cd[hs] = 0; CW.nev = 0;
        combat_begin_cast(&CW, &W, h, hs, ENTITY_NULL, 0); run(S.s[hs].hit_time + 0.02f); ch->cast_skill = -1;
        for (int k = 0; k < CW.nev; ++k) if (CW.ev[k].type == CE_HIT) { hits++; crits += CW.ev[k].crit; }
    }
    CHECK(hits == 3000);
    float rate = (float)crits / (float)hits;
    CHECK(rate > 0.11f && rate < 0.17f);   /* expected 0.05 + 30 * 0.003 = 0.14 */

    /* --- determinism: same seed, same outcome --- */
    float out[2];
    for (int run_i = 0; run_i < 2; ++run_i) {
        reset(); CW.rng = 4242;
        h = spawn(TEAM_PLAYER, vec3(0, 0, 0), 0, HERO, 1, 100); g = spawn(TEAM_ENEMY, vec3(0, 0, 1.5f), 0, GRUNT, 1, 1e9f);
        combat_get(&CW, g)->max_hp = combat_get(&CW, g)->hp = 1e9f;
        for (int i = 0; i < 20; ++i) { combat_get(&CW, h)->cd[hs] = 0; combat_begin_cast(&CW, &W, h, hs, ENTITY_NULL, 0); run(0.5f); }
        out[run_i] = combat_get(&CW, g)->hp;
    }
    CHECK(out[0] == out[1]);
    TEST_DONE();
}
