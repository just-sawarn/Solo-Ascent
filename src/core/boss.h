#ifndef SA_BOSS_H
#define SA_BOSS_H
/*
 * Boss fights: HP-threshold phases, each with a kit override, damage/speed multipliers and mechanics (adds, healing vines,
 * rewind, hazard zones, silencing song, reflecting armor, magic barrier, hollow curse, void waves, the Solo Ascent,
 * the Quartet Strike finisher), plus an enrage timer. Data comes from data/bosses.json (tools/gen_bosses.py).
 * Pure logic over the combat/party/enemy systems; presentation reads BossRt and drains `boss_pop_action`.
 */
#include <stdbool.h>
#include "core/data.h"
#include "core/noise.h"
#include "core/party.h"

#define BOSS_MAX 16
#define BOSS_PHASES 5
#define BOSS_MECH 6
#define BOSS_HZ 24
#define BOSS_ADDS 12

typedef enum { MK_ADDS, MK_VINES, MK_REWIND, MK_ZONES, MK_SONG, MK_ARMOR, MK_BARRIER, MK_CURSE, MK_WAVES, MK_GRAVITY, MK_FINISHER, MK_COUNT } MechKind;
typedef enum { HZ_NONE, HZ_SAND, HZ_RIFT, HZ_FIRE, HZ_WAVE } HazardKind;

typedef struct {
    MechKind kind;
    float every, power, dur, radius, reflect;
    int count, max;
    HazardKind hazard;
    char enemy[32]; int enemy_def;
} MechDef;

typedef struct {
    float hp_below;                    /* enter when HP fraction <= this */
    char name[32];
    float damage, speed;               /* multipliers (1 = unchanged) */
    int nmech; MechDef mech[BOSS_MECH];
    int nskills; int skills[4];        /* kit override (empty = the enemy's own) */
    int nenter; char enter[3][64];     /* actions queued on entering: toast:..  cutscene:..  gauge:N */
} PhaseDef;

typedef struct {
    char id[24], enemy[32], title[48], outro[24];
    int enemy_def;
    float arena, enrage;
    bool super;
    int nphase; PhaseDef phase[BOSS_PHASES];
} BossDef;

typedef struct { BossDef b[BOSS_MAX]; int n; } BossDB;

bool bossdb_load(BossDB *db, const char *dir, const EnemyDB *edb, const SkillDB *skills);
int  bossdb_find(const BossDB *db, const char *enemy_id);       /* -1 if none */

typedef struct { bool on; HazardKind kind; float x, z, r, tele, t, power, grow; unsigned hit_mask; } Hazard;

typedef struct {
    bool active, defeated;
    int def;
    Entity ent;
    Vec3 center;
    int phase;
    float time, phase_time;
    bool enraged;
    float mt[BOSS_MECH];
    float hist[21]; float hist_t; int nhist;
    float tele_t; MechKind tele_kind;                  /* a telegraphed mechanic is about to fire */
    Hazard hz[BOSS_HZ];
    Entity adds[BOSS_ADDS], vines[6];
    float armor_meter, armor_max, exposed_t; bool armored;
    bool barrier; float barrier_t, barrier_down_t, elem_t[EL_COUNT];
    bool downed[PARTY_MAX]; int ndowned, called; float call_cd, grav_t; bool callout_ready, lock_control, first_call;
    bool finisher_ready, quartet; float quartet_t; int quartet_step;
    int ev_seen;
    float dmg_scale;                                   /* New Game+ pressure on top of the phase multiplier */
    char out[10][64]; int nout;
    Rng rng;
} BossRt;

void boss_start(BossRt *rt, const BossDB *db, int def, Entity ent, Vec3 center, PartyCtx *pc);
void boss_update(BossRt *rt, const BossDB *db, Party *pt, PartyCtx *pc, float dt);
/* party wiped / left the arena: the boss resets to its first phase at full health */
void boss_reset(BossRt *rt, const BossDB *db, Party *pt, PartyCtx *pc);
void boss_end(BossRt *rt, Party *pt, PartyCtx *pc);              /* despawns adds, frees downed allies */
bool boss_call_out(BossRt *rt, Party *pt, PartyCtx *pc);         /* Solo Ascent: restore the next fallen ally */
bool boss_quartet(BossRt *rt);                                    /* start the finisher if it is ready */
const char *boss_pop_action(BossRt *rt);                          /* next queued presentation action, or NULL */
float boss_hp_frac(const BossRt *rt, PartyCtx *pc);
const char *boss_phase_name(const BossRt *rt, const BossDB *db);

#endif
