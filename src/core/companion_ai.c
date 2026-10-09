#include "core/party.h"
#include <math.h>
#include <string.h>

/* Companion AI: role priorities + a party command, producing the same inputs a human would. */

typedef struct { Entity e; Enemy *en; Combatant *c; Vec3 p; float d; } EView;

#define MAX_VIEW 24

static float dist2(Vec3 a, Vec3 b) { float dx = a.x - b.x, dz = a.z - b.z; return sqrtf(dx * dx + dz * dz); }

static bool knows(const PartyMember *m, int skill) {
    if (skill < 0) return false;
    for (int i = 0; i < m->def->nskills; ++i) if (m->def->skills[i] == skill) return true;
    return false;
}

/* Skill index by id, only when this member actually knows it and it is ready. */
static int ready(PartyCtx *c, PartyMember *m, const char *id) {
    int s = skilldb_find(c->skills, id);
    if (!knows(m, s)) return -1;
    return combat_can_cast(c->cw, combat_get(c->cw, m->ent), s) ? s : -1;
}

static Vec3 steer(Vec3 from, Vec3 goal, float arrive, float *mx, float *mz, float *dist) {
    float dx = goal.x - from.x, dz = goal.z - from.z, d = sqrtf(dx * dx + dz * dz);
    *dist = d;
    if (d > arrive && d > 1e-4f) { *mx = dx / d; *mz = dz / d; } else { *mx = *mz = 0; }
    return goal;
}

static void gather(PartyCtx *c, Vec3 around, float radius, EView *out, int *n) {
    *n = 0;
    for (int i = 0; i < ENEMY_MAX && *n < MAX_VIEW; ++i) {
        Enemy *en = &c->enemies->e[i];
        if (!en->used) continue;
        Combatant *cb = combat_get(c->cw, en->ent);
        if (!cb || cb->dead) continue;
        Vec3 p = c->w->pos[ecs_index(en->ent)];
        float d = dist2(p, around);
        if (d > radius) continue;
        out[(*n)++] = (EView){en->ent, en, cb, p, d};
    }
}

static int cluster_count(const EView *v, int n, Vec3 at, float r) {
    int k = 0;
    for (int i = 0; i < n; ++i) if (dist2(v[i].p, at) <= r) k++;
    return k;
}

void companion_decide(Party *p, PartyCtx *c, PartyMember *m, float dt, CompanionOut *out) {
    *out = (CompanionOut){.cast = -1};
    Combatant *me = combat_get(c->cw, m->ent);
    if (!me || me->dead) return;
    Vec3 mp = m->pc.ch.pos;
    Role role = m->def->role;

    /* leader: the active member, or the first living one when everyone is AI */
    PartyMember *lead = (p->active >= 0 && p->active < p->n) ? &p->m[p->active] : &p->m[0];
    Combatant *lead_cb = combat_get(c->cw, lead->ent);
    if (lead == m || !lead_cb || lead_cb->dead) { lead = m; lead_cb = me; }
    Vec3 lp = lead->pc.ch.pos;

    /* ---- engaged enemies, depending on the command ---- */
    EView view[MAX_VIEW]; int nv;
    gather(c, mp, 30.0f, view, &nv);
    EView eng[MAX_VIEW]; int ne = 0;
    for (int i = 0; i < nv; ++i) {
        bool attacking_party = false;
        if (view[i].en->target != ENTITY_NULL) { Combatant *t = combat_get(c->cw, view[i].en->target); attacking_party = t && !t->dead && view[i].en->state != AI_IDLE; }
        bool near_leader = dist2(view[i].p, lp) <= 12.0f, near_me = view[i].d <= 7.0f;
        bool take = false;
        switch (p->cmd) {
        case CMD_AGGRESSIVE: take = true; break;
        case CMD_DEFENSIVE: take = attacking_party || near_leader || near_me; break;
        case CMD_SUPPORT: take = (attacking_party && view[i].d <= 12.0f) || dist2(view[i].p, lp) <= 7.0f; break;
        }
        if (take) eng[ne++] = view[i];
    }
    /* focus: the leader's lock target if engaged, else the nearest engaged enemy */
    int fi = -1;
    for (int i = 0; i < ne; ++i) if (p->focus != ENTITY_NULL && eng[i].e == p->focus) fi = i;
    if (fi < 0) { float bd = 1e9f; for (int i = 0; i < ne; ++i) if (eng[i].d < bd) { bd = eng[i].d; fi = i; } }

    /* ---- party state ---- */
    int worst = -1; float worst_f = 2.0f; int hurt65 = 0, hurt55 = 0, dead = -1; float avg = 0; int alive = 0;
    for (int i = 0; i < p->n; ++i) {
        Combatant *a = party_cb(p, c, i);
        if (!a) continue;
        if (a->dead) { if (dead < 0) dead = i; continue; }
        float f = a->hp / a->max_hp; avg += f; alive++;
        if (f < worst_f) { worst_f = f; worst = i; }
        if (f < 0.65f) hurt65++;
        if (f < 0.55f) hurt55++;
    }
    if (alive) avg /= (float)alive;

    /* ---- formation slot (used when nothing to fight, and as a leash) ---- */
    float yaw = lead->pc.yaw, fx = sinf(yaw), fz = cosf(yaw), rx = cosf(yaw), rz = -sinf(yaw);
    float fwd = 0, side = 0;
    float tight = p->cmd == CMD_SUPPORT ? 0.7f : 1.0f;
    switch (role) {
    case ROLE_TANK: fwd = 2.6f; side = -1.8f; break;
    case ROLE_MAGE: fwd = -3.4f; side = 2.6f; break;
    case ROLE_HEALER: fwd = -3.0f; side = -2.6f; break;
    default: fwd = 1.5f; side = 2.0f; break;
    }
    Vec3 slot = vec3(lp.x + (fx * fwd + rx * side) * tight, 0, lp.z + (fz * fwd + rz * side) * tight);
    float to_leader = dist2(mp, lp);
    if (to_leader > 35.0f) { /* lost: warp to the slot */
        m->pc.ch.pos.x = slot.x; m->pc.ch.pos.z = slot.z; m->pc.ch.vel = vec3(0, 0, 0); mp = m->pc.ch.pos;
    }

    /* ---- reactive defense: raise guard / dodge an incoming telegraphed attack ---- */
    bool threatened = false; float time_to_hit = 9.0f; EView *attacker = NULL;
    for (int i = 0; i < nv; ++i) {
        Combatant *ec = view[i].c;
        if (ec->cast_skill < 0 || ec->cast_hit) continue;
        const Skill *es = &c->skills->s[ec->cast_skill];
        if (es->kind == SK_PROJECTILE) continue;
        float reach = es->kind == SK_AOE_CIRCLE ? (es->range + es->radius) : es->range + 0.5f;
        if (view[i].d > reach + 0.6f) continue;
        float t = es->hit_time - ec->cast_t;
        if (t < time_to_hit) { time_to_hit = t; threatened = true; attacker = &view[i]; }
    }
    if (m->guard_hold > 0) m->guard_hold -= dt;
    if (role == ROLE_TANK && threatened && time_to_hit <= 0.20f && time_to_hit > -0.05f) m->guard_hold = 0.45f;
    out->guard = role == ROLE_TANK && m->guard_hold > 0;
    if (role != ROLE_TANK && threatened && time_to_hit <= 0.30f && time_to_hit > 0.02f && !player_attacking(&m->pc) && m->pc.roll_cd <= 0) {
        Vec3 away = vec3(mp.x - attacker->p.x, 0, mp.z - attacker->p.z); float l = sqrtf(away.x * away.x + away.z * away.z) + 1e-4f;
        out->roll = true; out->mx = away.x / l; out->mz = away.z / l;
        return;
    }

    /* ---- decisions (throttled): which skill to cast ---- */
    m->think_t -= dt;
    bool think = m->think_t <= 0;
    if (think) m->think_t = 0.12f;
    int cast = -1; Entity target = ENTITY_NULL; bool basic = false;
    Vec3 focus_p = fi >= 0 ? eng[fi].p : mp;
    float focus_d = fi >= 0 ? eng[fi].d : 99.0f;
    Entity focus_e = fi >= 0 ? eng[fi].e : ENTITY_NULL;
    bool melee_busy = player_attacking(&m->pc) || player_rolling(&m->pc);
    bool offensive_ok = p->cmd != CMD_SUPPORT || (fi >= 0 && dist2(focus_p, lp) <= 6.0f);

    if (think && !melee_busy && me->cast_skill < 0 && me->stagger_t <= 0) {
        switch (role) {
        case ROLE_TANK: {
            int s;
            /* protect allies: taunt when a non-tank is being hunted */
            bool ally_hunted = false;
            for (int i = 0; i < ne && !ally_hunted; ++i) {
                Combatant *tc = combat_get(c->cw, eng[i].en->target);
                if (tc && tc->ent != m->ent && !tc->dead && eng[i].d < 12.0f) ally_hunted = true;
            }
            if (ally_hunted && (s = ready(c, m, "taunt")) >= 0) { cast = s; break; }
            if (me->hp / me->max_hp < 0.55f && (s = ready(c, m, "iron_wall")) >= 0) { cast = s; break; }
            if (avg < 0.40f && ne >= 2 && (s = ready(c, m, "unbroken_vow")) >= 0) { cast = s; break; }
            if (avg < 0.55f && (s = ready(c, m, "bulwark_barrier")) >= 0) { cast = s; break; }
            if (cluster_count(eng, ne, mp, 3.5f) >= 3 && (s = ready(c, m, "ground_slam")) >= 0) { cast = s; break; }
            for (int i = 0; i < ne && cast < 0; ++i) { /* interrupt a wind-up with a bash */
                if (eng[i].c->cast_skill >= 0 && !eng[i].c->cast_hit && eng[i].d < 2.4f && (s = ready(c, m, "shield_bash")) >= 0) { cast = s; target = eng[i].e; }
            }
            if (cast >= 0) break;
            if (fi >= 0 && focus_d < 2.2f && offensive_ok && !threatened) { basic = true; target = focus_e; }
        } break;
        case ROLE_MAGE: {
            int s; float mpf = me->mp / me->max_mp;
            if (me->hp / me->max_hp < 0.45f && (s = ready(c, m, "arcane_shield")) >= 0) { cast = s; break; }
            if (fi < 0 || !offensive_ok) break;
            target = focus_e;
            if (focus_d < 4.5f && (s = ready(c, m, "ice_nova")) >= 0) { cast = s; break; }  /* panic button: slows the pack */
            int cl = cluster_count(eng, ne, focus_p, 4.0f);
            if (cl >= 3 && mpf > 0.55f) {
                if ((s = ready(c, m, "meteor")) >= 0) { cast = s; break; }
                if ((s = ready(c, m, "starfall")) >= 0) { cast = s; break; }
                if ((s = ready(c, m, "thunderstorm")) >= 0) { cast = s; break; }
            }
            if (mpf > 0.30f) {
                bool slowed = eng[fi].c->status[ST_SLOW].time > 0;
                if (!slowed && (s = ready(c, m, "frost_lance")) >= 0) { cast = s; break; }
                if ((s = ready(c, m, "fireball")) >= 0 && (cl >= 2 || mpf > 0.5f)) { cast = s; break; }
                if ((s = ready(c, m, "lightning_chain")) >= 0 && cl >= 2) { cast = s; break; }
            }
            if (focus_d <= 20.0f) basic = true;
        } break;
        case ROLE_HEALER: {
            int s;
            if (dead >= 0 && ne == 0 && (s = ready(c, m, "revive")) >= 0) { cast = s; target = p->m[dead].ent; break; }
            if (dead >= 0 && fi >= 0 && eng[fi].d > 12.0f && (s = ready(c, m, "revive")) >= 0) { cast = s; target = p->m[dead].ent; break; }
            for (int i = 0; i < p->n && cast < 0; ++i) { /* curses and debuffs first */
                Combatant *a = party_cb(p, c, i);
                if (!a || a->dead || dist2(c->w->pos[ecs_index(a->ent)], mp) > 12.0f) continue;
                if (a->status[ST_HOLLOW_CURSE].time > 0 && (s = ready(c, m, "purify_curse")) >= 0) { cast = s; target = a->ent; }
                else if ((a->status[ST_BURN].time > 0 || a->status[ST_POISON].time > 0 || a->status[ST_SLOW].time > 0 || a->status[ST_STUN].time > 0 || a->status[ST_SILENCE].time > 0)
                         && a->hp / a->max_hp < 0.85f && (s = ready(c, m, "cleanse")) >= 0) { cast = s; target = a->ent; }
            }
            if (cast >= 0) break;
            if (hurt55 >= 3 && (s = ready(c, m, "radiant_renewal")) >= 0) { cast = s; break; }
            if (hurt65 >= 2 && (s = ready(c, m, "sanctuary")) >= 0) { cast = s; break; }
            if (hurt65 >= 2 && (s = ready(c, m, "prayer_of_mending")) >= 0) { cast = s; break; }
            if (worst >= 0 && worst_f < 0.38f && (s = ready(c, m, "dawns_embrace")) >= 0) { cast = s; target = p->m[worst].ent; break; }
            if (worst >= 0 && worst_f < 0.72f && (s = ready(c, m, "heal")) >= 0) { cast = s; target = p->m[worst].ent; break; }
            if (ne > 0 && worst_f > 0.8f && (s = ready(c, m, "blessing")) >= 0 && lead_cb->status[ST_BUFF_ATK].time <= 0) { cast = s; break; }
            if (fi >= 0 && worst_f > 0.85f && me->mp / me->max_mp > 0.5f && offensive_ok && p->cmd != CMD_SUPPORT && focus_d < 18.0f) { basic = true; target = focus_e; }
        } break;
        default: { /* hero when not human-controlled: simple melee rotation */
            int s;
            if (fi < 0 || !offensive_ok) break;
            target = focus_e;
            if (focus_d > 8.0f && (s = ready(c, m, "gale_rush")) >= 0 && focus_d < 12.0f) { cast = s; break; }
            if (focus_d < 3.0f) {
                if (cluster_count(eng, ne, mp, 2.8f) >= 2 && (s = ready(c, m, "rising_slash")) >= 0) { cast = s; break; }
                if ((s = ready(c, m, "sunder")) >= 0) { cast = s; break; }
                if ((s = ready(c, m, "heroic_slash")) >= 0) { cast = s; break; }
                basic = true;
            }
        } break;
        }
        m->want_cast = cast; m->want_target = target; m->want_basic = basic;
    }
    if (m->want_cast >= 0 || m->want_basic) { out->cast = m->want_cast; out->basic = m->want_basic; out->target = m->want_target; m->want_cast = -1; m->want_basic = false; }

    /* ---- movement ---- */
    Vec3 goal = slot; float arrive = 1.0f; float dist;
    bool fight = fi >= 0;
    float standoff = 0;
    switch (role) {
    case ROLE_TANK:
        if (fight) {
            float ex = lp.x - focus_p.x, ez = lp.z - focus_p.z, l = sqrtf(ex * ex + ez * ez) + 1e-4f;
            float hold = p->cmd == CMD_AGGRESSIVE ? 1.7f : 2.6f;
            goal = vec3(focus_p.x + ex / l * hold, 0, focus_p.z + ez / l * hold);
            if (p->cmd == CMD_SUPPORT) goal = vec3(lp.x - ex / l * -2.0f, 0, lp.z - ez / l * -2.0f);
            arrive = 0.6f;
        }
        break;
    case ROLE_MAGE:
    case ROLE_HEALER:
        standoff = role == ROLE_MAGE ? 8.5f : 9.0f;
        if (fight) {
            float ex = mp.x - focus_p.x, ez = mp.z - focus_p.z, l = sqrtf(ex * ex + ez * ez) + 1e-4f;
            /* nearest threat decides: back off if anything is close, otherwise hold the standoff ring near the party */
            float nearest = 99; for (int i = 0; i < ne; ++i) nearest = fminf(nearest, eng[i].d);
            if (nearest < 5.0f) goal = vec3(mp.x + ex / l * 4.0f, 0, mp.z + ez / l * 4.0f);                 /* kite away */
            else if (focus_d > standoff + 3.0f && role == ROLE_MAGE) goal = vec3(focus_p.x + ex / l * standoff, 0, focus_p.z + ez / l * standoff);
            else goal = mp;                                                                                    /* hold */
            if (role == ROLE_HEALER && dist2(mp, lp) > 8.0f) goal = slot;                                      /* stay with the party */
            arrive = 0.8f;
        }
        break;
    default:
        if (fight) { float ex = mp.x - focus_p.x, ez = mp.z - focus_p.z, l = sqrtf(ex * ex + ez * ez) + 1e-4f; goal = vec3(focus_p.x + ex / l * 1.8f, 0, focus_p.z + ez / l * 1.8f); arrive = 0.5f; }
        break;
    }
    /* never wander off from the leader under Defensive/Support */
    if (p->cmd != CMD_AGGRESSIVE && dist2(goal, lp) > (p->cmd == CMD_SUPPORT ? 7.0f : 11.0f)) goal = slot;
    float mx, mz; steer(mp, goal, arrive, &mx, &mz, &dist);
    if (player_attacking(&m->pc)) { mx = mz = 0; } /* casters plant their feet */
    out->mx = mx; out->mz = mz;
    out->sprint = dist > 9.0f;
    out->walk = dist < 2.5f && !fight;
}
