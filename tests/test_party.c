#include "testutil.h"
#include <math.h>
#include <string.h>
#include "core/party.h"

static World W; static CombatWorld CW; static PhysWorld PH; static SkillDB S; static EnemyDB E; static PartyDB PD; static EnemyPool EP; static Party P;
static const float DT = 1.0f / 60.0f;
static PartyCtx CTX;

typedef struct {
    float dmg_taken[4]; int heals_by[4]; int casts_by[4]; int deaths, kills; bool taunted;
    float mage_near_s, total_s; float healed_total;
} Stats_;
static Stats_ ST;

static void collect(void) {
    for (int i = 0; i < CW.nev; ++i) {
        const CombatEvent *e = &CW.ev[i];
        for (int k = 0; k < P.n; ++k) {
            if (e->type == CE_HIT && e->dst == P.m[k].ent) ST.dmg_taken[k] += e->amount;
            if (e->type == CE_HEAL && e->src == P.m[k].ent) { ST.heals_by[k]++; ST.healed_total += e->amount; }
            if (e->type == CE_CAST && e->amount < 0.5f && e->src == P.m[k].ent) ST.casts_by[k]++;
        }
        if (e->type == CE_DEATH) { bool member = false; for (int k = 0; k < P.n; ++k) if (e->dst == P.m[k].ent) member = true; if (member) ST.deaths++; else ST.kills++; }
        if (e->type == CE_STATUS && e->amount == (float)ST_TAUNT) ST.taunted = true;
    }
    CW.nev = 0;
}

static void sim(float seconds) {
    PartyInput in = {0}; in.lock = ENTITY_NULL;
    for (int i = 0; i < (int)(seconds * 60.0f + 0.5f); ++i) {
        party_step(&P, &CTX, &in, true, DT);
        enemies_update(&EP, &CW, &W, &PH, &E, DT);
        combat_update(&CW, &W, &PH, DT);
        collect();
        ST.total_s += DT;
        /* how often is the mage within melee range of a living enemy? */
        Vec3 mp = P.m[2].pc.ch.pos; Combatant *mc = party_cb(&P, &CTX, 2);
        if (mc && !mc->dead) for (int k = 0; k < ENEMY_MAX; ++k) { Enemy *en = &EP.e[k]; if (!en->used) continue; Combatant *ec = combat_get(&CW, en->ent); if (ec && !ec->dead && sqrtf((en->ctl.pos.x - mp.x) * (en->ctl.pos.x - mp.x) + (en->ctl.pos.z - mp.z) * (en->ctl.pos.z - mp.z)) < 2.5f) { ST.mage_near_s += DT; break; } }
    }
}

static void setup(PartyCommand cmd) {
    ecs_init(&W); phys_init(&PH, 0); combat_init(&CW, &S); enemies_init(&EP); party_init(&P); memset(&ST, 0, sizeof ST);
    CTX = (PartyCtx){&CW, &W, &PH, &EP, &E, &S};
    P.active = -1; P.cmd = cmd;
    for (int i = 0; i < 4; ++i) party_add(&P, &CTX, &PD.m[i], vec3((float)i * 1.5f - 2.2f, 0, 0), 0);
}
static void spawn(const char *id, float x, float z) { enemy_spawn(&EP, &CW, &W, &E, enemydb_find(&E, id), vec3(x, 0, z)); }
static int alive_enemies(void) { int n = 0; for (int i = 0; i < ENEMY_MAX; ++i) if (EP.e[i].used) { Combatant *c = combat_get(&CW, EP.e[i].ent); if (c && !c->dead) n++; } return n; }
static float dist_to(int member, float x, float z) { Vec3 p = P.m[member].pc.ch.pos; return sqrtf((p.x - x) * (p.x - x) + (p.z - z) * (p.z - z)); }

int main(void) {
    CHECK(skilldb_load(&S, SA_DATA_DIR "/skills.json"));
    CHECK(enemydb_load(&E, &S, SA_DATA_DIR "/enemies.json"));
    CHECK(partydb_load(&PD, &S, SA_DATA_DIR "/party.json"));

    /* ---- full party AI beats a mixed encounter, with roles behaving like roles ---- */
    setup(CMD_AGGRESSIVE);
    for (int i = 0; i < E.n; ++i) E.e[i].level += 7;               /* a real fight: the party is level 5 */
    spawn("shadow_soldier", -4, 14); spawn("shadow_soldier", 4, 14); spawn("shadow_soldier", 0, 17); spawn("shadow_soldier", -8, 19);
    spawn("shadow_soldier", 8, 20); spawn("shadow_archer", 0, 22); spawn("shadow_archer", -6, 24); spawn("shadow_brute", 8, 16);
    spawn("shadow_brute", -9, 15);
    sim(150.0f);
    CHECK(alive_enemies() == 0);
    CHECK(ST.kills == 9);
    CHECK(ST.deaths <= 1);
    CHECK(ST.heals_by[3] >= 2);                                   /* Mirelle healed */
    CHECK(ST.dmg_taken[1] > ST.dmg_taken[2] && ST.dmg_taken[1] > ST.dmg_taken[3]);  /* Bram absorbed more than the squishies */
    CHECK(ST.casts_by[2] >= 5);                                   /* Lyra cast spells */
    CHECK(ST.mage_near_s < ST.total_s * 0.25f);                   /* ...from a distance */
    CHECK(ST.taunted);                                            /* Bram used a taunt */

    CHECK(enemydb_load(&E, &S, SA_DATA_DIR "/enemies.json")); /* back to normal levels */

    /* ---- commands ---- */
    setup(CMD_AGGRESSIVE);
    spawn("shadow_soldier", 0, 24);
    float before = dist_to(1, 0, 24);
    sim(6.0f);
    CHECK(dist_to(1, 0, 24) < before - 6.0f);                     /* Aggressive: the tank advances on a distant enemy */

    setup(CMD_DEFENSIVE);
    spawn("shadow_soldier", 0, 24);
    sim(6.0f);
    CHECK(dist_to(1, 0, 0) < 9.0f && dist_to(2, 0, 0) < 9.0f);    /* Defensive: nobody leaves the leader to chase it */

    setup(CMD_SUPPORT);
    spawn("shadow_soldier", 0, 18);
    party_cb(&P, &CTX, 0)->hp *= 0.3f;                            /* hurt the leader */
    sim(8.0f);
    CHECK(ST.heals_by[3] >= 1);
    CHECK(ST.casts_by[2] == 0);                                   /* Support: the mage does not start fights */
    CHECK(party_cb(&P, &CTX, 0)->hp > party_cb(&P, &CTX, 0)->max_hp * 0.3f);

    /* ---- the healer raises the dead once it is safe ---- */
    setup(CMD_DEFENSIVE);
    Combatant *k = party_cb(&P, &CTX, 0); k->hp = 0; k->dead = true;
    sim(12.0f);
    CHECK(!party_cb(&P, &CTX, 0)->dead);

    /* ---- Hollow Curse is purified, burns are cleansed ---- */
    setup(CMD_DEFENSIVE);
    party_cb(&P, &CTX, 1)->status[ST_HOLLOW_CURSE].time = 20; party_cb(&P, &CTX, 1)->hp *= 0.5f;
    sim(4.0f);
    CHECK(party_cb(&P, &CTX, 1)->status[ST_HOLLOW_CURSE].time <= 0);

    /* ---- control switching: the human drives the active member, the former leader becomes AI ---- */
    setup(CMD_DEFENSIVE);
    P.active = 0;
    PartyInput in = {0}; in.move.mx = 1; in.move.mz = 0;
    for (int i = 0; i < 120; ++i) { party_step(&P, &CTX, &in, true, DT); enemies_update(&EP, &CW, &W, &PH, &E, DT); combat_update(&CW, &W, &PH, DT); CW.nev = 0; }
    CHECK(P.m[0].pc.ch.pos.x > 6.0f);                             /* Kael ran right under human control */
    CHECK(P.m[1].pc.ch.pos.x > 0.0f);                             /* companions followed */
    CHECK(party_switch(&P, &CTX, 2)); CHECK(P.active == 2);
    Vec3 before_lyra = P.m[2].pc.ch.pos;
    in.move.mx = -1;
    for (int i = 0; i < 60; ++i) { party_step(&P, &CTX, &in, true, DT); combat_update(&CW, &W, &PH, DT); CW.nev = 0; }
    CHECK(P.m[2].pc.ch.pos.x < before_lyra.x - 3.0f);             /* Lyra now obeys the human */
    party_cb(&P, &CTX, 3)->dead = true;
    CHECK(!party_switch(&P, &CTX, 3));                            /* cannot switch to the fallen */
    CHECK(party_next_alive(&P, &CTX, 2) == 0);

    /* ---- wipe and respawn ---- */
    for (int i = 0; i < 4; ++i) { Combatant *c = party_cb(&P, &CTX, i); c->dead = true; c->hp = 0; }
    CHECK(party_wiped(&P, &CTX));
    party_respawn_all(&P, &CTX, vec3(0, 0, 0));
    CHECK(!party_wiped(&P, &CTX)); CHECK(party_cb(&P, &CTX, 1)->hp == party_cb(&P, &CTX, 1)->max_hp);
    TEST_DONE();
}
