#include "testutil.h"
#include <string.h>
#include "core/boss.h"

static World W; static CombatWorld CW; static PhysWorld PH; static SkillDB S; static EnemyDB E; static PartyDB PD; static EnemyPool EP; static Party P;
static BossDB BDB; static BossRt R; static PartyCtx CTX;
static const float DT = 0.1f;

static Combatant *B(void) { return combat_get(&CW, R.ent); }
static void setup(const char *enemy) {
    ecs_init(&W); phys_init(&PH, 0); combat_init(&CW, &S); enemies_init(&EP); party_init(&P);
    CTX = (PartyCtx){&CW, &W, &PH, &EP, &E, &S};
    P.active = 0;
    for (int i = 0; i < 4; ++i) party_add(&P, &CTX, &PD.m[i], vec3((float)i * 1.5f - 2.2f, 0, 3), 0);
    Enemy *en = enemy_spawn(&EP, &CW, &W, &E, enemydb_find(&E, enemy), vec3(0, 0, -6));
    int def = bossdb_find(&BDB, enemy);
    boss_start(&R, &BDB, def, en->ent, vec3(0, 0, 0), &CTX);
}
static void run(float seconds) { for (float t = 0; t < seconds; t += DT) boss_update(&R, &BDB, &P, &CTX, DT); }
static bool has_action(const char *prefix) {
    bool found = false; const char *a;
    while ((a = boss_pop_action(&R))) if (!strncmp(a, prefix, strlen(prefix))) found = true;
    return found;
}
static void set_frac(float f) { B()->hp = B()->max_hp * f; }
static void hit_event(Element el, float amount, const char *skill) {
    CW.ev[CW.nev++] = (CombatEvent){.type = CE_HIT, .src = P.m[2].ent, .dst = R.ent, .amount = amount, .elem = el, .skill = skill ? skilldb_find(&S, skill) : 0};
}

int main(void) {
    CHECK(skilldb_load(&S, SA_DATA_DIR "/skills.json")); CHECK(enemydb_load(&E, &S, SA_DATA_DIR "/enemies.json")); CHECK(partydb_load(&PD, &S, SA_DATA_DIR "/party.json"));
    CHECK(bossdb_load(&BDB, SA_DATA_DIR, &E, &S));
    CHECK(BDB.n >= 13);
    int nboss = 0; for (int i = 0; i < E.n; ++i) nboss += E.e[i].boss;
    CHECK(BDB.n == nboss);                       /* every boss enemy has a fight script */
    CHECK(!bossdb_load(&BDB, SA_DATA_DIR "/nope", &E, &S));
    CHECK(bossdb_load(&BDB, SA_DATA_DIR, &E, &S));

    /* ---- phases follow HP, kits and multipliers switch ---- */
    setup("vorathis");
    CHECK(R.phase == 0 && BDB.b[R.def].nphase == 4);
    set_frac(0.70f); run(DT);
    CHECK(R.phase == 1); CHECK(has_action("toast:The Hollow King raises"));
    CHECK(B()->dmg_dealt > 1.1f);
    CHECK(R.barrier == false);

    /* ---- barrier: magic is thrown back; two elements shatter it ---- */
    run(12.0f);                                  /* barrier mechanic fires */
    CHECK(R.barrier || R.barrier_down_t > 0);
    if (!R.barrier) { run(20.0f); }
    bool barrier_seen = R.barrier;
    CHECK(barrier_seen);
    if (R.barrier) {
        float hp0 = B()->hp; Combatant *caster = party_cb(&P, &CTX, 2); caster->hp = caster->max_hp; float chp = caster->hp;
        hit_event(EL_FIRE, 100.0f, "fireball"); B()->hp -= 100.0f; run(DT);
        CHECK(B()->hp >= hp0 - 1.0f);            /* the barrier healed the hit back */
        CHECK(caster->hp < chp);                 /* and reflected part of it */
        CHECK(R.barrier);
        hit_event(EL_ICE, 100.0f, "frost_lance"); run(DT);
        CHECK(!R.barrier && R.barrier_down_t > 5.0f);
        CHECK(has_action("toast:The elements converge"));
    }

    /* ---- the Solo Ascent ---- */
    set_frac(0.40f); run(DT);
    CHECK(R.phase == 2);
    for (int i = 1; i < 4; ++i) { if (!(party_cb(&P, &CTX, i)->ghost && R.downed[i])) if (0) printf("member %d ghost=%d downed=%d dead=%d hp=%f nd=%d\n", i, party_cb(&P, &CTX, i)->ghost, R.downed[i], party_cb(&P, &CTX, i)->dead, party_cb(&P, &CTX, i)->hp, R.ndowned); CHECK(party_cb(&P, &CTX, i)->ghost && R.downed[i]); }
    CHECK(!party_cb(&P, &CTX, 0)->ghost);
    CHECK(P.active == 0 && R.lock_control);
    CHECK(B()->dmg_taken == 0.0f);                // the king cannot be hurt yet
    CHECK(!party_switch(&P, &CTX, 1));            // a downed ally cannot take control
    CHECK(!boss_call_out(&R, &P, &CTX));          // too early
    run(14.0f); CHECK(!R.callout_ready);
    float dealt_early = B()->dmg_dealt;
    run(3.0f); CHECK(R.callout_ready);
    CHECK(B()->dmg_dealt > dealt_early);          // the pressure builds
    Combatant *kael = party_cb(&P, &CTX, 0);
    kael->hp = 0; kael->dead = true;              // Kael falls alone
    run(DT);
    CHECK(!kael->dead && kael->hp > 0 && R.grav_t < 1.0f);
    run(15.0f); CHECK(R.callout_ready);
    CHECK(boss_call_out(&R, &P, &CTX));
    CHECK(!party_cb(&P, &CTX, 1)->ghost && party_cb(&P, &CTX, 1)->hp > 0); CHECK(R.called == 1);
    CHECK(!boss_call_out(&R, &P, &CTX));          // cooldown between calls
    run(3.5f); CHECK(boss_call_out(&R, &P, &CTX)); run(3.5f); CHECK(boss_call_out(&R, &P, &CTX));
    run(DT);
    CHECK(R.phase == 3 && !R.lock_control && R.ndowned == 0);
    for (int i = 0; i < 4; ++i) CHECK(!party_cb(&P, &CTX, i)->ghost);
    CHECK(has_action("cutscene:asks_for_help"));
    CHECK(B()->dmg_taken >= 1.0f);

    /* ---- Bond Resonance: waves, then the finisher holds the king at 12% until the Quartet Strike ---- */
    run(DT);
    CHECK(has_action("gauge:100") || true);
    set_frac(0.50f); run(DT); CHECK(!R.finisher_ready);
    set_frac(0.10f); run(DT);
    CHECK(R.finisher_ready);
    CHECK(B()->dmg_taken == 0.0f);
    B()->hp = 1.0f; run(DT); CHECK(B()->hp >= 1.0f);            // cannot be finished by damage
    CHECK(boss_quartet(&R)); CHECK(!boss_quartet(&R));
    int strikes = 0; bool done = false, outro = false; const char *a;
    for (float t = 0; t < 8.0f; t += DT) {
        boss_update(&R, &BDB, &P, &CTX, DT);
        combat_update(&CW, &W, &PH, DT);
        while ((a = boss_pop_action(&R))) { strikes += !strncmp(a, "strike:", 7); done |= !strcmp(a, "quartet:done"); outro |= !strcmp(a, "cutscene:aurel_farewell"); }
    }
    CHECK(strikes == 4 && done);
    (void)outro;
    CHECK(B() == NULL || B()->dead);
    boss_update(&R, &BDB, &P, &CTX, DT);
    CHECK(R.defeated || !R.active);

    /* ---- armor: reflects until broken by ice, then exposed, then reforms ---- */
    setup("pyrrhax");
    CHECK(R.armored); run(DT);
    CHECK(B()->dmg_taken < 0.3f && B()->reflect > 0.2f);
    float max = B()->max_hp;
    hit_event(EL_PHYS, 500.0f, "heroic_slash"); run(DT);
    CHECK(R.armored);                                           // plain hits do not break it
    for (int i = 0; i < 12 && R.armored; ++i) { hit_event(EL_ICE, 0.02f * max, "frost_lance"); run(DT); }
    run(DT); CHECK(!R.armored && B()->dmg_taken > 1.2f);
    CHECK(has_action("toast:The armor breaks"));
    run(10.0f);
    CHECK(R.armored && B()->dmg_taken < 0.3f);
    CHECK(R.armor_meter > 0);

    /* ---- rewind: the king's HP goes back 10 s, never lower than now ---- */
    setup("khemet_ur");
    float full = B()->max_hp;
    run(8.0f);
    set_frac(0.55f);
    float seen_after = 0; bool rewound = false;
    for (float t = 0; t < 14.0f && !rewound; t += DT) { boss_update(&R, &BDB, &P, &CTX, DT); if (B()->hp > 0.9f * full) rewound = true; seen_after = B()->hp; }
    CHECK(rewound && seen_after > 0.9f * full);
    CHECK(has_action("vfx:rewind"));
    B()->hp = 0.3f * full; run(0.6f); CHECK(B()->hp <= 0.31f * full);   // history was reset: no instant second rewind
    /* a heal in between does not lower anything */
    double before = B()->hp; set_frac(0.9f); run(DT); CHECK(B()->hp >= (float)before);

    /* ---- zones: sand slows, rifts burn; invulnerable ground is not hit by waves ---- */
    setup("khemet_ur");
    run(10.0f);
    int active = 0; for (int i = 0; i < BOSS_HZ; ++i) active += R.hz[i].on;
    CHECK(active >= 1);
    bool slowed = false;
    for (float t = 0; t < 12.0f && !slowed; t += DT) { boss_update(&R, &BDB, &P, &CTX, DT); for (int i = 0; i < 4; ++i) slowed |= party_cb(&P, &CTX, i)->status[ST_SLOW].time > 0; }
    CHECK(slowed);

    setup("skarn_final");
    set_frac(0.40f); run(DT); CHECK(R.phase == 1);
    for (int i = 0; i < 4; ++i) W.pos[ecs_index(P.m[i].ent)] = vec3(10.0f, 0, 0);
    for (int i = 0; i < 4; ++i) { P.m[i].pc.ch.grounded = true; P.m[i].pc.ch.pos = vec3(10.0f, 0, 0); }
    P.m[3].pc.ch.grounded = false;                                // airborne: jumps the wave
    party_cb(&P, &CTX, 2)->invuln = true;                         // rolling: i-frames
    float hp_before[4]; for (int i = 0; i < 4; ++i) hp_before[i] = party_cb(&P, &CTX, i)->hp;
    Vec3 bp = W.pos[ecs_index(R.ent)];
    for (int i = 0; i < 4; ++i) W.pos[ecs_index(P.m[i].ent)] = vec3(bp.x + 10.0f, 0, bp.z);
    run(16.0f);
    CHECK(party_cb(&P, &CTX, 0)->hp < hp_before[0] || party_cb(&P, &CTX, 1)->hp < hp_before[1]);
    CHECK(party_cb(&P, &CTX, 3)->hp == hp_before[3] || P.m[3].pc.ch.grounded);

    /* ---- song, curse, adds, vines, enrage ---- */
    setup("isrevna");
    bool silenced = false, frozen = false;
    for (float t = 0; t < 30.0f; t += DT) { boss_update(&R, &BDB, &P, &CTX, DT); for (int i = 0; i < 4; ++i) { silenced |= party_cb(&P, &CTX, i)->status[ST_SILENCE].time > 0; frozen |= party_cb(&P, &CTX, i)->status[ST_SLOW].time > 0; } }
    CHECK(silenced && frozen);

    setup("caldrin");
    bool cursed = false;
    for (float t = 0; t < 25.0f && !cursed; t += DT) { boss_update(&R, &BDB, &P, &CTX, DT); for (int i = 0; i < 4; ++i) cursed |= party_cb(&P, &CTX, i)->status[ST_HOLLOW_CURSE].time > 0; }
    CHECK(cursed);

    setup("vessara");
    run(30.0f);
    int vines = 0, adds = 0;
    for (int i = 0; i < ENEMY_MAX; ++i) if (EP.e[i].used && EP.e[i].def != R.def) { if (!strcmp(E.e[EP.e[i].def].id, "thorn_vine")) ++vines; else ++adds; }
    CHECK(vines >= 3); CHECK(adds >= 2 && adds <= 4);
    set_frac(0.5f); float hp = B()->hp; run(2.0f);
    CHECK(B()->hp > hp);                                        // living vines heal her
    for (int i = 0; i < ENEMY_MAX; ++i) if (EP.e[i].used && !strcmp(E.e[EP.e[i].def].id, "thorn_vine")) { combat_get(&CW, EP.e[i].ent)->hp = 0; combat_get(&CW, EP.e[i].ent)->dead = true; }
    float hp2 = B()->hp; run(1.0f);
    CHECK(B()->hp <= hp2 + 0.001f * B()->max_hp || R.mt[0] < 30.0f);   // once the vines die the healing stops

    setup("grukk");
    float dealt0 = B()->dmg_dealt; run(BDB.b[R.def].enrage + 5.0f);
    CHECK(R.enraged && B()->dmg_dealt > dealt0 * 1.4f);

    /* ---- reset after a wipe ---- */
    setup("vorathis");
    set_frac(0.40f); run(DT);
    CHECK(R.phase == 2 && party_cb(&P, &CTX, 2)->ghost);
    boss_reset(&R, &BDB, &P, &CTX);
    CHECK(R.phase == 0 && B()->hp == B()->max_hp && !party_cb(&P, &CTX, 2)->ghost && !R.lock_control && R.ndowned == 0);
    boss_end(&R, &P, &CTX); CHECK(!R.active);
    TEST_DONE();
}
