#ifndef SA_PARTY_H
#define SA_PARTY_H
/*
 * The party of four: Kael (hero), Bram (tank), Lyra (mage), Mirelle (healer).
 * Every member has a PlayerCtl. The ACTIVE member is driven by human input; the others by companion AI that
 * produces the very same kind of input (move vector, guard, roll, skill to cast), so cast rules, cooldowns,
 * casting locks, i-frames and dashes are identical for human and AI.
 */
#include "core/combat.h"
#include "core/data.h"
#include "core/enemy_ai.h"
#include "core/player.h"

#define PARTY_MAX 4

typedef enum { CMD_AGGRESSIVE, CMD_DEFENSIVE, CMD_SUPPORT } PartyCommand;

typedef struct {
    bool used;
    Entity ent;
    const PartyDef *def;
    int idx;
    PlayerCtl pc;
    int combo_idx; float combo_timer; bool attack_queued;
    float think_t, guard_hold, strafe;
    int want_cast; Entity want_target; bool want_basic;   /* AI decision, executed on the next step */
    bool ai_guard, ai_roll; float ai_mx, ai_mz; bool ai_sprint, ai_walk;
} PartyMember;

typedef struct {
    PartyMember m[PARTY_MAX];
    int n;
    int active;                 /* human-controlled member, -1 = everyone is AI (tests/cutscenes) */
    PartyCommand cmd;
    Entity focus;               /* the leader's lock target; AI prefers it */
} Party;

typedef struct {
    CombatWorld *cw; World *w; PhysWorld *phys;
    EnemyPool *enemies; const EnemyDB *edb; const SkillDB *skills;
} PartyCtx;

typedef struct {
    PlayerIn move;              /* mx, mz (world space), sprint, walk */
    bool attack, jump, roll, guard;
    bool skill[4];
    Entity lock;                /* human lock-on target (aim + soft target for casts) */
} PartyInput;

void party_init(Party *p);
PartyMember *party_add(Party *p, PartyCtx *c, const PartyDef *def, Vec3 pos, float yaw);
/* One 60 Hz step for every member. `first_step`: one-shot presses in `in` count only on the first step of a frame. */
void party_step(Party *p, PartyCtx *c, const PartyInput *in, bool first_step, float dt);
/* Switch control to another living member (returns false if impossible). */
bool party_switch(Party *p, PartyCtx *c, int idx);
int  party_next_alive(const Party *p, const PartyCtx *c, int from);
bool party_wiped(const Party *p, const PartyCtx *c);
/* Moves everyone to `pos` (fanned out) without healing: zone travel. */
void party_teleport(Party *p, PartyCtx *c, Vec3 pos, float yaw);
void party_respawn_all(Party *p, PartyCtx *c, Vec3 pos);
Combatant *party_cb(const Party *p, const PartyCtx *c, int idx);

/* companion AI (core/companion_ai.c) */
typedef struct { float mx, mz; bool sprint, walk, guard, roll; int cast; Entity target; bool basic; } CompanionOut;
void companion_decide(Party *p, PartyCtx *c, PartyMember *m, float dt, CompanionOut *out);

#endif
