#include "core/combat.h"
#include <math.h>
#include <string.h>
#include "core/log.h"

float combat_rand(CombatWorld *cw) {
    uint32_t x = cw->rng ? cw->rng : 0x9E3779B9u;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    cw->rng = x;
    return (float)(x & 0xFFFFFF) / 16777216.0f;
}

void combat_init(CombatWorld *cw, const SkillDB *skills) {
    memset(cw, 0, sizeof *cw);
    cw->skills = skills;
    cw->rng = 12345u;
    for (int i = 0; i < ECS_MAX_ENTITIES; ++i) cw->slot_of[i] = -1;
}

static void emit(CombatWorld *cw, CombatEvent e) {
    if (cw->nev < EV_MAX) cw->ev[cw->nev++] = e;
}

void combatant_recalc(Combatant *c) {
    float hp_ratio = c->max_hp > 0 ? c->hp / c->max_hp : 1.0f, mp_ratio = c->max_mp > 0 ? c->mp / c->max_mp : 1.0f;
    c->max_hp = 80.0f + (float)c->st.vit * 14.0f + (float)c->level * 18.0f;
    c->max_mp = 30.0f + (float)c->st.wis * 5.0f + (float)c->st.intel * 3.0f + (float)c->level * 4.0f;
    c->atk = 8.0f + (float)c->st.str * 2.2f + (float)c->level * 2.0f + c->weapon_atk;
    c->matk = 8.0f + (float)c->st.intel * 2.6f + (float)c->level * 2.0f + c->weapon_matk;
    c->def = (float)c->st.vit * 1.3f + c->armor + (float)c->level;
    c->mdef = (float)c->st.wis * 1.3f + c->armor * 0.5f + (float)c->level;
    c->crit = 0.05f + (float)c->st.agi * 0.003f;
    c->hp = c->hp > 0 || c->dead ? c->max_hp * hp_ratio : c->max_hp;
    c->mp = c->mp > 0 ? c->max_mp * mp_ratio : c->max_mp;
}

Combatant *combat_add(CombatWorld *cw, World *w, Entity e, int team, int level, Stats st, float poise) {
    (void)w;
    for (int i = 0; i < CB_MAX; ++i) {
        if (cw->c[i].used) continue;
        Combatant *c = &cw->c[i];
        memset(c, 0, sizeof *c);
        c->used = true; c->ent = e; c->team = team; c->level = level; c->st = st;
        c->poise_max = c->poise = poise;
        c->radius = 0.4f; c->height = 1.8f; c->cast_skill = -1; c->party_idx = -1; c->threat_mult = 1.0f;
        for (int k = 0; k < EL_COUNT; ++k) c->resist[k] = 1.0f;
        combatant_recalc(c);
        c->hp = c->max_hp; c->mp = c->max_mp;
        cw->slot_of[ecs_index(e)] = (int16_t)i;
        return c;
    }
    LOGE("combat: out of combatant slots");
    return NULL;
}

void combat_remove(CombatWorld *cw, Entity e) {
    int s = cw->slot_of[ecs_index(e)];
    if (s < 0) return;
    cw->c[s].used = false;
    cw->slot_of[ecs_index(e)] = -1;
}

Combatant *combat_get(CombatWorld *cw, Entity e) {
    if (e == ENTITY_NULL) return NULL;
    int s = cw->slot_of[ecs_index(e)];
    return (s >= 0 && cw->c[s].used && cw->c[s].ent == e) ? &cw->c[s] : NULL;
}

bool combat_alive(const CombatWorld *cw, Entity e) {
    int s = e ? cw->slot_of[ecs_index(e)] : -1;
    return s >= 0 && cw->c[s].used && !cw->c[s].dead;
}

static bool has_status(const Combatant *c, StatusKind k) { return c->status[k].time > 0; }

static void add_status(CombatWorld *cw, Combatant *c, StatusKind k, float time, float power, Entity src) {
    if (k == ST_NONE || time <= 0) return;
    Status *s = &c->status[k];
    if (s->time <= 0 || power >= s->power || time > s->time) { s->time = fmaxf(s->time, time); s->power = fmaxf(s->power, power); s->src = src; }
    emit(cw, (CombatEvent){.type = CE_STATUS, .dst = c->ent, .amount = (float)k});
}

float combat_base_damage(const Combatant *a, const Skill *s, const Combatant *d) {
    float stat = s->magic ? a->matk : a->atk;
    if (has_status(a, ST_BUFF_ATK)) stat *= 1.0f + a->status[ST_BUFF_ATK].power;
    float def = s->magic ? d->mdef : d->def;
    if (has_status(d, ST_DEF_DOWN)) def *= 1.0f - d->status[ST_DEF_DOWN].power;
    float mitig = 100.0f / (100.0f + fmaxf(def, 0.0f));
    float taken = d->resist[s->element];
    if (has_status(d, ST_BUFF_DEF)) taken *= 1.0f - fminf(d->status[ST_BUFF_DEF].power, 0.8f);
    return stat * s->power * mitig * taken;
}

static Vec3 pos_of(const World *w, const Combatant *c) { return w->pos[ecs_index(c->ent)]; }

static void kill_if_dead(CombatWorld *cw, World *w, Combatant *d, Entity src) {
    if (d->hp > 0 || d->dead) return;
    if (d->has_resolve && !d->resolve_used) {
        d->resolve_used = true; d->hp = 1.0f;
        emit(cw, (CombatEvent){.type = CE_RESOLVE, .dst = d->ent, .pos = pos_of(w, d)});
        return;
    }
    d->hp = 0; d->dead = true; d->dead_t = 0; d->cast_skill = -1; d->guarding = false;
    emit(cw, (CombatEvent){.type = CE_DEATH, .src = src, .dst = d->ent, .pos = pos_of(w, d)});
}

/* Raw damage after all mitigation (DoT, barrier-aware). */
static void deal_raw(CombatWorld *cw, World *w, Combatant *d, float amount, Entity src) {
    if (d->dead || amount <= 0) return;
    if (d->barrier > 0) {
        float absorbed = fminf(d->barrier, amount);
        d->barrier -= absorbed; amount -= absorbed;
    }
    d->hp -= amount;
    kill_if_dead(cw, w, d, src);
}

static void poise_hit(CombatWorld *cw, World *w, Combatant *d, float amount, Entity src) {
    if (d->dead || amount <= 0) return;
    d->poise -= amount; d->poise_regen_t = 3.0f;
    if (d->poise <= 0) {
        d->poise = d->poise_max; d->stagger_t = 0.7f; d->cast_skill = -1;
        emit(cw, (CombatEvent){.type = CE_STAGGER, .src = src, .dst = d->ent, .pos = pos_of(w, d)});
    }
}

static bool in_front(const World *w, const Combatant *d, Vec3 from) {
    uint32_t i = ecs_index(d->ent);
    float fx = sinf(w->yaw[i]), fz = cosf(w->yaw[i]);
    float dx = from.x - w->pos[i].x, dz = from.z - w->pos[i].z, l = sqrtf(dx * dx + dz * dz);
    if (l < 1e-4f) return true;
    return (fx * dx + fz * dz) / l > 0.34f; /* within ~70 degrees of facing */
}

/* One skill hit on one target. `origin` is where the blow comes from (for guard direction). */
static void hit_target(CombatWorld *cw, World *w, Combatant *a, const Skill *s, Combatant *d, Vec3 origin, float dmg_scale) {
    if (d->dead || d->team == a->team || d->invuln) return;
    float base = combat_base_damage(a, s, d) * dmg_scale;
    bool crit = combat_rand(cw) < a->crit;
    float var = 0.95f + 0.10f * combat_rand(cw);
    float dmg = fmaxf(1.0f, base * var * (crit ? 1.5f : 1.0f));
    Vec3 dp = pos_of(w, d);
    float poise_dmg = s->stagger;

    if (d->guarding && in_front(w, d, origin)) {
        if (d->guard_time < 0.20f) { /* perfect parry */
            emit(cw, (CombatEvent){.type = CE_PARRY, .src = a->ent, .dst = d->ent, .pos = dp});
            if (!a->dead) { a->stagger_t = fmaxf(a->stagger_t, 1.0f); a->cast_skill = -1; }
            return;
        }
        dmg *= 0.4f; poise_dmg *= 0.3f;
        emit(cw, (CombatEvent){.type = CE_BLOCK, .src = a->ent, .dst = d->ent, .amount = dmg, .pos = dp});
    }
    deal_raw(cw, w, d, dmg, a->ent);
    emit(cw, (CombatEvent){.type = CE_HIT, .src = a->ent, .dst = d->ent, .amount = dmg, .crit = crit, .elem = s->element, .pos = dp});
    d->hit_flash = 0.12f;
    if (a->party_idx >= 0 && a->party_idx < 8 && d->team == TEAM_ENEMY) d->threat[a->party_idx] += dmg * a->threat_mult;
    if (a->team == TEAM_PLAYER) cw->gauge = fminf(100.0f, cw->gauge + (crit ? 5.0f : 2.0f));
    if (d->dead) return;
    if (s->status != ST_NONE && s->kind != SK_BUFF && s->kind != SK_TAUNT && s->kind != SK_CLEANSE) add_status(cw, d, s->status, s->status_time, s->status_power, a->ent);
    poise_hit(cw, w, d, poise_dmg, a->ent);
}

static float dist2d(Vec3 a, Vec3 b) { float dx = a.x - b.x, dz = a.z - b.z; return sqrtf(dx * dx + dz * dz); }

static float seg_dist2d(Vec3 p, Vec3 a, Vec3 b) {
    float abx = b.x - a.x, abz = b.z - a.z, l2 = abx * abx + abz * abz;
    float t = l2 > 1e-6f ? ((p.x - a.x) * abx + (p.z - a.z) * abz) / l2 : 0.0f;
    t = fminf(fmaxf(t, 0.0f), 1.0f);
    return dist2d(p, vec3(a.x + abx * t, 0, a.z + abz * t));
}

static void heal_target(CombatWorld *cw, World *w, Combatant *src, const Skill *s, Combatant *t, float amount) {
    if (t->dead || amount <= 0) return;
    if (has_status(t, ST_HOLLOW_CURSE)) amount = 0; /* the Hollow Curse blocks healing */
    float real = fminf(amount, t->max_hp - t->hp);
    t->hp += real;
    if (src->party_idx >= 0 && src->party_idx < 8 && real > 0) { /* healing draws attention from engaged enemies */
        for (int i = 0; i < CB_MAX; ++i) {
            Combatant *e = &cw->c[i];
            if (!e->used || e->dead || e->team == src->team) continue;
            float engaged = 0; for (int k = 0; k < 8; ++k) engaged += e->threat[k];
            if (engaged > 0) e->threat[src->party_idx] += real * 0.35f * src->threat_mult;
        }
    }
    emit(cw, (CombatEvent){.type = CE_HEAL, .src = src->ent, .dst = t->ent, .amount = amount, .elem = s->element, .pos = pos_of(w, t)});
}

static Combatant *pick_ally(CombatWorld *cw, World *w, Combatant *caster, const Skill *s, bool want_dead) {
    Combatant *t = combat_get(cw, caster->cast_target);
    if (t && t->team == caster->team && t->dead == want_dead && dist2d(pos_of(w, t), pos_of(w, caster)) <= s->range + 1.0f) return t;
    Combatant *best = NULL; float bd = 1e9f;
    for (int i = 0; i < CB_MAX; ++i) {
        Combatant *o = &cw->c[i];
        if (!o->used || o->team != caster->team || o->dead != want_dead) continue;
        float d = dist2d(pos_of(w, o), pos_of(w, caster));
        if (d > s->range + 1.0f) continue;
        float score = want_dead ? d : o->hp / o->max_hp; /* heal the weakest; revive the nearest */
        if (score < bd) { bd = score; best = o; }
    }
    return want_dead || best ? best : caster;
}

static void fire_projectile(CombatWorld *cw, World *w, Combatant *c, int skill_idx, const Skill *s) {
    for (int i = 0; i < MAX_PROJ; ++i) {
        Projectile *p = &cw->proj[i];
        if (p->active) continue;
        Vec3 o = pos_of(w, c);
        Vec3 dir = vec3(sinf(c->cast_yaw), 0, cosf(c->cast_yaw));
        Combatant *tg = combat_get(cw, c->cast_target);
        Vec3 start = vec3(o.x + dir.x * 0.6f, o.y + 1.3f, o.z + dir.z * 0.6f);
        if (tg && !tg->dead) { /* soft aim at the locked target's chest */
            Vec3 tp = vec3_add(pos_of(w, tg), vec3(0, 1.1f, 0));
            Vec3 d = vec3_sub(tp, start); float l = vec3_len(d);
            if (l > 0.1f) dir = vec3_scale(d, 1.0f / l);
        }
        float speed = s->proj_speed > 0 ? s->proj_speed : 15.0f;
        *p = (Projectile){.active = true, .pos = start, .vel = vec3_scale(dir, speed), .owner = c->ent, .skill = skill_idx, .team = c->team, .life = 2.5f};
        emit(cw, (CombatEvent){.type = CE_PROJECTILE, .src = c->ent, .pos = start, .skill = skill_idx});
        return;
    }
}

static void explode_or_hit(CombatWorld *cw, World *w, Combatant *owner, const Skill *s, Vec3 at, Combatant *direct) {
    if (s->radius > 0) {
        for (int i = 0; i < CB_MAX; ++i) {
            Combatant *d = &cw->c[i];
            if (!d->used || d->dead || d->team == owner->team) continue;
            if (dist2d(pos_of(w, d), at) <= s->radius + d->radius) hit_target(cw, w, owner, s, d, at, d == direct ? 1.0f : 0.8f);
        }
    } else if (direct) hit_target(cw, w, owner, s, direct, at, 1.0f);
}

static void execute_skill(CombatWorld *cw, World *w, PhysWorld *phys, Combatant *c) {
    int idx = c->cast_skill;
    const Skill *s = &cw->skills->s[idx];
    Vec3 o = pos_of(w, c);
    Vec3 fwd = vec3(sinf(c->cast_yaw), 0, cosf(c->cast_yaw));
    switch (s->kind) {
    case SK_MELEE_ARC: {
        float cosh = cosf(s->arc_deg * 0.5f * 3.14159265f / 180.0f);
        for (int i = 0; i < CB_MAX; ++i) {
            Combatant *d = &cw->c[i];
            if (!d->used || d->dead || d->team == c->team) continue;
            Vec3 dp = pos_of(w, d);
            float dist = dist2d(dp, o);
            if (dist > s->range + d->radius) continue;
            if (dist > 0.5f) {
                float dot = ((dp.x - o.x) * fwd.x + (dp.z - o.z) * fwd.z) / dist;
                if (dot < cosh) continue;
            }
            hit_target(cw, w, c, s, d, o, 1.0f);
        }
    } break;
    case SK_DASH_STRIKE: {
        float dist = s->dash > 0 ? s->dash : 5.0f;
        if (phys) { float hit = phys_raycast(phys, vec3(o.x, o.y + 0.9f, o.z), fwd, dist + 0.6f); if (hit < dist + 0.6f) dist = fmaxf(0.0f, hit - 0.6f); }
        Vec3 dest = vec3(o.x + fwd.x * dist, o.y, o.z + fwd.z * dist);
        for (int i = 0; i < CB_MAX; ++i) {
            Combatant *d = &cw->c[i];
            if (!d->used || d->dead || d->team == c->team) continue;
            if (seg_dist2d(pos_of(w, d), o, dest) <= 1.2f) hit_target(cw, w, c, s, d, o, 1.0f);
        }
        c->dash_pending = true; c->dash_to = dest;
        emit(cw, (CombatEvent){.type = CE_DASH, .src = c->ent, .pos = dest});
    } break;
    case SK_AOE_CIRCLE: {
        Vec3 center = s->range > 0 ? vec3(o.x + fwd.x * s->range, o.y, o.z + fwd.z * s->range) : o;
        Combatant *tg = combat_get(cw, c->cast_target);
        if (s->range > 0 && tg && !tg->dead && dist2d(pos_of(w, tg), o) <= s->range + 3.0f) center = pos_of(w, tg); /* ground spells land on the lock target */
        for (int i = 0; i < CB_MAX; ++i) {
            Combatant *d = &cw->c[i];
            if (!d->used || d->dead || d->team == c->team) continue;
            if (dist2d(pos_of(w, d), center) <= s->radius + d->radius) hit_target(cw, w, c, s, d, center, 1.0f);
        }
        emit(cw, (CombatEvent){.type = CE_CAST, .src = c->ent, .pos = center, .skill = idx, .amount = 1.0f}); /* amount=1: impact marker for VFX */
    } break;
    case SK_PROJECTILE: fire_projectile(cw, w, c, idx, s); break;
    case SK_HEAL: {
        Combatant *t = pick_ally(cw, w, c, s, false);
        float amount = s->power > 0 ? c->matk * s->power : t->max_hp * s->status_power;
        heal_target(cw, w, c, s, t, amount);
    } break;
    case SK_HEAL_AOE: {
        for (int i = 0; i < CB_MAX; ++i) {
            Combatant *t = &cw->c[i];
            if (!t->used || t->dead || t->team != c->team) continue;
            if (dist2d(pos_of(w, t), o) > s->radius) continue;
            heal_target(cw, w, c, s, t, c->matk * s->power);
            if (s->status != ST_NONE) add_status(cw, t, s->status, s->status_time, s->status_power, c->ent);
        }
    } break;
    case SK_BUFF: {
        for (int i = 0; i < CB_MAX; ++i) {
            Combatant *t = &cw->c[i];
            if (!t->used || t->dead || t->team != c->team) continue;
            if (t != c && (s->radius <= 0 || dist2d(pos_of(w, t), o) > s->radius)) continue;
            add_status(cw, t, s->status, s->status_time, s->status_power, c->ent);
        }
    } break;
    case SK_BARRIER: {
        for (int i = 0; i < CB_MAX; ++i) {
            Combatant *t = &cw->c[i];
            if (!t->used || t->dead || t->team != c->team) continue;
            if (t != c && (s->radius <= 0 || dist2d(pos_of(w, t), o) > s->radius)) continue;
            float amount = s->magic || c->st.intel > c->st.str ? c->matk * s->power : t->max_hp * s->power;
            t->barrier = fmaxf(t->barrier, amount);
            emit(cw, (CombatEvent){.type = CE_STATUS, .dst = t->ent, .amount = -1.0f});
        }
    } break;
    case SK_TAUNT: {
        for (int i = 0; i < CB_MAX; ++i) {
            Combatant *d = &cw->c[i];
            if (!d->used || d->dead || d->team == c->team) continue;
            if (dist2d(pos_of(w, d), o) > s->radius) continue;
            add_status(cw, d, ST_TAUNT, s->status_time, 1.0f, c->ent);
            if (c->party_idx >= 0) { float top = 0; for (int k = 0; k < 8; ++k) top = fmaxf(top, d->threat[k]); d->threat[c->party_idx] = fmaxf(d->threat[c->party_idx], top * 1.1f + 60.0f); }
            poise_hit(cw, w, d, s->stagger, c->ent);
        }
    } break;
    case SK_CLEANSE: {
        Combatant *t = pick_ally(cw, w, c, s, false);
        static const StatusKind bad[] = {ST_BURN, ST_POISON, ST_SLOW, ST_STUN, ST_SILENCE, ST_DEF_DOWN};
        for (unsigned k = 0; k < sizeof bad / sizeof bad[0]; ++k) t->status[bad[k]].time = 0;
        if (s->status == ST_HOLLOW_CURSE) t->status[ST_HOLLOW_CURSE].time = 0;
        emit(cw, (CombatEvent){.type = CE_STATUS, .dst = t->ent, .amount = -2.0f});
    } break;
    case SK_REVIVE: {
        Combatant *t = pick_ally(cw, w, c, s, true);
        if (t && t->dead) {
            t->dead = false; t->dead_t = 0; t->hp = t->max_hp * s->power; t->stagger_t = 0;
            emit(cw, (CombatEvent){.type = CE_REVIVE, .src = c->ent, .dst = t->ent, .pos = pos_of(w, t)});
        }
    } break;
    case SK_PASSIVE: break;
    }
}

bool combat_can_cast(const CombatWorld *cw, const Combatant *c, int skill) {
    if (!c || skill < 0 || skill >= cw->skills->n) return false;
    const Skill *s = &cw->skills->s[skill];
    if (c->dead || c->cast_skill >= 0 || c->stagger_t > 0 || s->passive) return false;
    if (has_status(c, ST_STUN)) return false;
    if (s->magic && has_status(c, ST_SILENCE)) return false;
    if (c->cd[skill] > 0 || c->mp < s->mp || cw->gauge < s->gauge) return false;
    return true;
}

bool combat_begin_cast(CombatWorld *cw, World *w, Entity caster, int skill, Entity target, float aim_yaw) {
    Combatant *c = combat_get(cw, caster);
    if (!combat_can_cast(cw, c, skill)) return false;
    const Skill *s = &cw->skills->s[skill];
    c->mp -= s->mp;
    if (s->gauge > 0) cw->gauge -= s->gauge;
    c->cd[skill] = s->cooldown;
    c->cast_skill = skill; c->cast_t = 0; c->cast_hit = false; c->cast_yaw = aim_yaw; c->cast_target = target;
    c->guarding = false;
    emit(cw, (CombatEvent){.type = CE_CAST, .src = caster, .pos = w->pos[ecs_index(caster)], .skill = skill});
    return true;
}

void combat_set_guard(CombatWorld *cw, Entity e, bool held) {
    Combatant *c = combat_get(cw, e);
    if (!c || c->dead) return;
    if (held && !c->guarding) {
        if (c->cast_skill >= 0 || c->stagger_t > 0) return;
        c->guarding = true; c->guard_time = 0;
    } else if (!held) c->guarding = false;
}

static void update_projectiles(CombatWorld *cw, World *w, PhysWorld *phys, float dt) {
    for (int i = 0; i < MAX_PROJ; ++i) {
        Projectile *p = &cw->proj[i];
        if (!p->active) continue;
        Combatant *owner = combat_get(cw, p->owner);
        const Skill *s = &cw->skills->s[p->skill];
        p->life -= dt;
        Vec3 step = vec3_scale(p->vel, dt);
        float sl = vec3_len(step);
        Vec3 next = vec3_add(p->pos, step);
        bool done = p->life <= 0;
        Combatant *hit = NULL;
        Vec3 impact = next;
        if (phys && sl > 1e-5f) {
            Vec3 dir = vec3_scale(step, 1.0f / sl);
            float t = phys_raycast(phys, p->pos, dir, sl);
            if (t < sl) { impact = vec3_add(p->pos, vec3_scale(dir, t)); done = true; }
        }
        for (int k = 0; k < CB_MAX && !done; ++k) {
            Combatant *d = &cw->c[k];
            if (!d->used || d->dead || d->team == p->team) continue;
            Vec3 dp = pos_of(w, d);
            if (next.y < dp.y - 0.2f || next.y > dp.y + d->height + 0.2f) continue;
            if (seg_dist2d(dp, p->pos, next) <= d->radius + 0.25f) { hit = d; impact = vec3(dp.x, next.y, dp.z); done = true; }
        }
        if (done) {
            p->active = false;
            if (owner && !owner->dead && (hit || s->radius > 0)) explode_or_hit(cw, w, owner, s, impact, hit);
            continue;
        }
        p->pos = next;
    }
}

void combat_update(CombatWorld *cw, World *w, PhysWorld *phys, float dt) {
    for (int i = 0; i < CB_MAX; ++i) {
        Combatant *c = &cw->c[i];
        if (!c->used) continue;
        if (c->hit_flash > 0) c->hit_flash -= dt;
        if (c->team == TEAM_ENEMY) for (int k = 0; k < 8; ++k) c->threat[k] *= 1.0f - fminf(1.0f, 0.06f * dt);
        if (c->dead) { c->dead_t += dt; continue; }
        for (int k = 0; k < cw->skills->n; ++k) if (c->cd[k] > 0) c->cd[k] -= dt;
        if (c->stagger_t > 0) c->stagger_t -= dt;
        if (c->guarding) c->guard_time += dt;
        if (c->poise_regen_t > 0) c->poise_regen_t -= dt; else if (c->poise < c->poise_max) c->poise = fminf(c->poise_max, c->poise + c->poise_max * 0.25f * dt);
        for (int k = 0; k < ST_COUNT; ++k) if (c->status[k].time > 0) c->status[k].time -= dt;
        c->mp = fminf(c->max_mp, c->mp + (1.5f + (has_status(c, ST_MANA_REGEN) ? c->status[ST_MANA_REGEN].power : 0.0f)) * dt);
        /* damage / healing over time, once per second */
        c->dot_t += dt;
        if (c->dot_t >= 1.0f) {
            c->dot_t -= 1.0f;
            if (has_status(c, ST_BURN)) deal_raw(cw, w, c, c->status[ST_BURN].power, c->status[ST_BURN].src);
            if (has_status(c, ST_POISON)) deal_raw(cw, w, c, c->status[ST_POISON].power, c->status[ST_POISON].src);
            if (has_status(c, ST_REGEN) && !has_status(c, ST_HOLLOW_CURSE)) c->hp = fminf(c->max_hp, c->hp + c->status[ST_REGEN].power);
            if (c->dead) continue;
        }
        if (c->cast_skill >= 0) {
            const Skill *s = &cw->skills->s[c->cast_skill];
            if (c->stagger_t > 0 || has_status(c, ST_STUN)) { c->cast_skill = -1; continue; }
            c->cast_t += dt;
            if (!c->cast_hit && c->cast_t >= s->hit_time) { c->cast_hit = true; execute_skill(cw, w, phys, c); }
            if (c->cast_t >= s->total_time) c->cast_skill = -1;
        }
    }
    update_projectiles(cw, w, phys, dt);
}
