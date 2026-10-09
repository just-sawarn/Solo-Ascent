#ifndef SA_COMBAT_H
#define SA_COMBAT_H
/*
 * Combat rules: stats -> derived values, damage/heal formulas, elements, guard/parry, poise/stagger, statuses,
 * barriers, cast timing (windup -> effect -> recovery), projectiles, and an event stream for presentation.
 * Pure logic over the ECS World (positions) and PhysWorld (walls); no engine or browser dependencies.
 */
#include <stdbool.h>
#include "core/data.h"
#include "core/ecs.h"
#include "core/physics.h"

#define CB_MAX 96
#define MAX_PROJ 64
#define EV_MAX 256

enum { TEAM_NONE = 0, TEAM_PLAYER = 1, TEAM_ENEMY = 2 };

typedef struct { float time, power; Entity src; } Status;

typedef struct {
    bool used;
    Entity ent;
    int team;
    int level;
    Stats st;
    float weapon_atk, weapon_matk, armor;
    struct { float str, intel, vit, wis, agi; } gear; /* stat bonuses from equipment */
    float gear_hp, gear_mp, gear_crit;
    float elem_boost[EL_COUNT];                         /* enchant damage multipliers (1 = none) */
    float max_hp, max_mp, atk, matk, def, mdef, crit;       /* derived (see combatant_recalc) */
    float hp, mp;
    float resist[EL_COUNT];                                  /* damage taken multiplier per element */
    Status status[ST_COUNT];
    float cd[DB_MAX_SKILLS];
    float poise, poise_max, poise_regen_t, stagger_t;
    float barrier;
    bool guarding; float guard_time;                          /* guard_time: seconds since the guard was raised */
    bool invuln;                                              /* set by the game (dodge-roll i-frames) */
    float radius, height;
    int cast_skill; float cast_t; bool cast_hit; float cast_yaw; Entity cast_target;
    bool has_resolve, resolve_used;
    bool dead; float dead_t;
    bool dash_pending; Vec3 dash_to;                         /* the owner of the movement controller applies this */
    float dot_t, hit_flash;
    int party_idx;             /* index in the party (threat slot), -1 otherwise */
    float threat_mult;         /* how much threat this combatant's damage generates (tanks > 1) */
    float threat[8];           /* on enemies: accumulated threat per party member */
} Combatant;

typedef struct { bool active; Vec3 pos, vel; Entity owner; int skill, team; float life; } Projectile;

typedef enum { CE_HIT, CE_HEAL, CE_DEATH, CE_PARRY, CE_BLOCK, CE_STAGGER, CE_CAST, CE_STATUS, CE_REVIVE, CE_RESOLVE, CE_DASH, CE_PROJECTILE } CombatEventType;
typedef struct { CombatEventType type; Entity src, dst; float amount; bool crit; Element elem; Vec3 pos; int skill; } CombatEvent;

typedef struct {
    const SkillDB *skills;
    Combatant c[CB_MAX];
    int16_t slot_of[ECS_MAX_ENTITIES];
    Projectile proj[MAX_PROJ];
    CombatEvent ev[EV_MAX];
    int nev;
    uint32_t rng;
    float gauge;               /* Resonance Gauge 0..100 (party ultimate) */
} CombatWorld;

void  combat_init(CombatWorld *cw, const SkillDB *skills);
Combatant *combat_add(CombatWorld *cw, World *w, Entity e, int team, int level, Stats st, float poise);
void  combat_remove(CombatWorld *cw, Entity e);
Combatant *combat_get(CombatWorld *cw, Entity e);
void  combatant_recalc(Combatant *c);
/* Apply a status from outside combat (consumables, scripted events). */
void  combat_apply_status(CombatWorld *cw, Combatant *c, StatusKind k, float time, float power, Entity src);          /* recompute derived stats; keeps hp/mp ratios */

bool  combat_can_cast(const CombatWorld *cw, const Combatant *c, int skill);
bool  combat_begin_cast(CombatWorld *cw, World *w, Entity caster, int skill, Entity target, float aim_yaw);
void  combat_set_guard(CombatWorld *cw, Entity e, bool held);
void  combat_update(CombatWorld *cw, World *w, PhysWorld *phys, float dt);
float combat_rand(CombatWorld *cw);            /* [0,1) */

/* Deterministic part of the damage formula (no variance, no crit). */
float combat_base_damage(const Combatant *a, const Skill *s, const Combatant *d);
bool  combat_alive(const CombatWorld *cw, Entity e);

#endif
