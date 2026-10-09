#include "core/enemy_ai.h"
#include <math.h>
#include <string.h>

void enemies_init(EnemyPool *p) { memset(p, 0, sizeof *p); }

Enemy *enemy_of(EnemyPool *p, Entity e) {
    for (int i = 0; i < ENEMY_MAX; ++i) if (p->e[i].used && p->e[i].ent == e) return &p->e[i];
    return NULL;
}

Enemy *enemy_spawn(EnemyPool *p, CombatWorld *cw, World *w, const EnemyDB *db, int def, Vec3 pos) {
    for (int i = 0; i < ENEMY_MAX; ++i) {
        if (p->e[i].used) continue;
        const EnemyDef *d = &db->e[def];
        Entity e = ecs_create(w, C_TRANSFORM | C_VELOCITY | C_HEALTH | C_FACTION);
        if (e == ENTITY_NULL) return NULL;
        uint32_t idx = ecs_index(e);
        w->pos[idx] = w->prev_pos[idx] = pos;
        w->faction[idx] = FACTION_ENEMY;
        Combatant *c = combat_add(cw, w, e, TEAM_ENEMY, d->level, d->stats, d->poise);
        if (!c) { ecs_destroy(w, e); return NULL; }
        memcpy(c->resist, d->resist, sizeof c->resist);
        c->radius = 0.4f * d->scale; c->height = 1.8f * d->scale;
        Enemy *en = &p->e[i];
        *en = (Enemy){.used = true, .ent = e, .def = def, .state = AI_IDLE, .home = pos, .strafe_dir = (i & 1) ? 1.0f : -1.0f};
        char_init(&en->ctl, pos);
        en->ctl.radius = 0.4f * d->scale;
        return en;
    }
    return NULL;
}

void enemy_remove(EnemyPool *p, CombatWorld *cw, World *w, Entity e) {
    Enemy *en = enemy_of(p, e);
    if (en) en->used = false;
    combat_remove(cw, e);
    ecs_destroy(w, e);
}

static float dist2d(Vec3 a, Vec3 b) { float dx = a.x - b.x, dz = a.z - b.z; return sqrtf(dx * dx + dz * dz); }

static Combatant *nearest_player(CombatWorld *cw, World *w, Vec3 from, float *out_d) {
    Combatant *best = NULL; float bd = 1e9f;
    for (int i = 0; i < CB_MAX; ++i) {
        Combatant *c = &cw->c[i];
        if (!c->used || c->dead || c->team != TEAM_PLAYER) continue;
        float d = dist2d(w->pos[ecs_index(c->ent)], from);
        if (d < bd) { bd = d; best = c; }
    }
    *out_d = bd;
    return best;
}

/* Highest-threat living party member (within double aggro range); falls back to the nearest. */
static Combatant *choose_target(CombatWorld *cw, World *w, const Combatant *self, Vec3 from, float aggro, float *out_d) {
    Combatant *best = NULL; float bt = 1.0f;
    for (int i = 0; i < CB_MAX; ++i) {
        Combatant *c = &cw->c[i];
        if (!c->used || c->dead || c->team != TEAM_PLAYER || c->party_idx < 0 || c->party_idx >= 8) continue;
        float t = self->threat[c->party_idx];
        if (t > bt && dist2d(w->pos[ecs_index(c->ent)], from) <= aggro * 2.0f) { bt = t; best = c; }
    }
    if (best) { *out_d = dist2d(w->pos[ecs_index(best->ent)], from); return best; }
    return nearest_player(cw, w, from, out_d);
}

/* Effective reach of a skill for deciding when to start it. */
static float skill_reach(const Skill *s) {
    switch (s->kind) {
    case SK_MELEE_ARC: return s->range * 0.9f;
    case SK_AOE_CIRCLE: return s->range > 0 ? s->range : s->radius * 0.8f;
    case SK_PROJECTILE: return 20.0f;
    default: return s->range;
    }
}

static void move(Enemy *en, PhysWorld *phys, float dx, float dz, float speed, float dt) {
    float l = sqrtf(dx * dx + dz * dz);
    CharMove m = {0, 0, false};
    if (l > 1e-4f && speed > 0) { m.wish_x = dx / l * speed; m.wish_z = dz / l * speed; }
    char_step(phys, &en->ctl, m, dt);
}

static float turn_to(float yaw, float target, float step) {
    const float pi = 3.14159265f;
    float d = fmodf(target - yaw, 2 * pi);
    if (d > pi) d -= 2 * pi;
    if (d < -pi) d += 2 * pi;
    return yaw + fminf(fmaxf(d, -step), step);
}

void enemies_update(EnemyPool *p, CombatWorld *cw, World *w, PhysWorld *phys, const EnemyDB *db, float dt) {
    for (int i = 0; i < ENEMY_MAX; ++i) {
        Enemy *en = &p->e[i];
        if (!en->used) continue;
        Combatant *c = combat_get(cw, en->ent);
        if (!c) { en->used = false; continue; }
        const EnemyDef *d = &db->e[en->def];
        uint32_t idx = ecs_index(en->ent);
        if (c->dead) { move(en, phys, 0, 0, 0, dt); w->pos[idx] = en->ctl.pos; w->vel[idx] = vec3(0, 0, 0); continue; }

        float speed = d->speed;
        if (c->status[ST_SLOW].time > 0) speed *= 1.0f - c->status[ST_SLOW].power;
        bool disabled = c->stagger_t > 0 || c->status[ST_STUN].time > 0;

        /* target: taunt wins, else the nearest player-side combatant */
        float td;
        Combatant *tg = choose_target(cw, w, c, en->ctl.pos, d->aggro_range, &td);
        if (c->status[ST_TAUNT].time > 0) {
            Combatant *tc = combat_get(cw, c->status[ST_TAUNT].src);
            if (tc && !tc->dead) { tg = tc; td = dist2d(w->pos[ecs_index(tc->ent)], en->ctl.pos); }
        }
        Vec3 tp = tg ? w->pos[ecs_index(tg->ent)] : en->ctl.pos;
        en->target = tg ? tg->ent : ENTITY_NULL;
        float home_d = dist2d(en->ctl.pos, en->home);

        if (en->state == AI_IDLE && tg && td <= d->aggro_range) en->state = AI_CHASE;
        if ((en->state == AI_CHASE || en->state == AI_RECOVER) && (!tg || home_d > d->leash || td > d->aggro_range * 2.0f)) en->state = AI_RETURN;
        if (en->state == AI_RETURN) {
            if (home_d < 1.0f) { en->state = AI_IDLE; }
            else if (tg && td <= d->aggro_range * 0.6f && home_d < d->leash * 0.5f) en->state = AI_CHASE;
            else { c->hp = fminf(c->max_hp, c->hp + c->max_hp * 0.1f * dt); }
        }

        float dx = tp.x - en->ctl.pos.x, dz = tp.z - en->ctl.pos.z;
        float want_yaw = atan2f(dx, dz);
        float mvx = 0, mvz = 0, mv_speed = 0;

        if (disabled) {
            en->state = en->state == AI_IDLE ? AI_IDLE : AI_CHASE;
        } else switch (en->state) {
        case AI_IDLE: break;
        case AI_CHASE: {
            en->yaw = turn_to(en->yaw, want_yaw, 9.0f * dt);
            /* choose a skill that is ready and in reach */
            int pick = -1;
            for (int k = 0; k < d->nskills && pick < 0; ++k) {
                const Skill *s = &cw->skills->s[d->skills[k]];
                if (combat_can_cast(cw, c, d->skills[k]) && td <= skill_reach(s)) {
                    bool ranged_ok = s->kind != SK_PROJECTILE || phys_raycast(phys, vec3(en->ctl.pos.x, 1.2f, en->ctl.pos.z), vec3(dx / (td + 1e-4f), 0, dz / (td + 1e-4f)), td) >= td - 0.2f;
                    if (ranged_ok) pick = d->skills[k];
                }
            }
            if (pick >= 0 && fabsf(fmodf(want_yaw - en->yaw + 9.4247f, 6.2832f) - 3.1416f) < 0.35f) {
                combat_begin_cast(cw, w, en->ent, pick, tg->ent, en->yaw);
                en->state = AI_ATTACK;
                break;
            }
            float pref = d->preferred_range;
            if (d->behavior == AI_RANGED) {
                if (td < pref * 0.6f) { mvx = -dx; mvz = -dz; mv_speed = speed; }
                else if (td > pref * 1.2f) { mvx = dx; mvz = dz; mv_speed = speed; }
                else { mvx = -dz * en->strafe_dir; mvz = dx * en->strafe_dir; mv_speed = speed * 0.5f; }
            } else if (td > pref * 0.8f) { mvx = dx; mvz = dz; mv_speed = speed; }
        } break;
        case AI_ATTACK:
            en->yaw = turn_to(en->yaw, want_yaw, 2.0f * dt); /* telegraph: barely tracks while winding up */
            if (c->cast_skill < 0) { en->state = AI_RECOVER; en->timer = d->behavior == AI_BRUTE ? 1.2f : 0.6f; }
            break;
        case AI_RECOVER:
            en->yaw = turn_to(en->yaw, want_yaw, 6.0f * dt);
            en->timer -= dt;
            if (en->timer <= 0) en->state = AI_CHASE;
            break;
        case AI_RETURN:
            mvx = en->home.x - en->ctl.pos.x; mvz = en->home.z - en->ctl.pos.z; mv_speed = speed;
            en->yaw = turn_to(en->yaw, atan2f(mvx, mvz), 9.0f * dt);
            break;
        }

        if (c->dash_pending) { en->ctl.pos.x = c->dash_to.x; en->ctl.pos.z = c->dash_to.z; c->dash_pending = false; }
        move(en, phys, mvx, mvz, mv_speed, dt);
        w->pos[idx] = en->ctl.pos;
        w->yaw[idx] = en->yaw;
        w->vel[idx] = en->ctl.vel;
    }

    /* soft separation between enemies and from player-side combatants (enemies yield) */
    for (int i = 0; i < ENEMY_MAX; ++i) {
        Enemy *a = &p->e[i];
        if (!a->used) continue;
        for (int j = i + 1; j < ENEMY_MAX; ++j) {
            Enemy *b = &p->e[j];
            if (!b->used) continue;
            float dx = b->ctl.pos.x - a->ctl.pos.x, dz = b->ctl.pos.z - a->ctl.pos.z, l = sqrtf(dx * dx + dz * dz);
            if (l < 0.9f && l > 1e-4f) { float push = (0.9f - l) * 0.5f; a->ctl.pos.x -= dx / l * push; a->ctl.pos.z -= dz / l * push; b->ctl.pos.x += dx / l * push; b->ctl.pos.z += dz / l * push; }
        }
        for (int k = 0; k < CB_MAX; ++k) {
            Combatant *pc = &cw->c[k];
            if (!pc->used || pc->team != TEAM_PLAYER || pc->dead) continue;
            Vec3 pp = w->pos[ecs_index(pc->ent)];
            float dx = a->ctl.pos.x - pp.x, dz = a->ctl.pos.z - pp.z, l = sqrtf(dx * dx + dz * dz);
            if (l < 0.8f && l > 1e-4f) { a->ctl.pos.x = pp.x + dx / l * 0.8f; a->ctl.pos.z = pp.z + dz / l * 0.8f; }
        }
        w->pos[ecs_index(a->ent)] = a->ctl.pos;
    }
}
