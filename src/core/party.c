#include "core/party.h"
#include <math.h>
#include <string.h>

void party_init(Party *p) { memset(p, 0, sizeof *p); p->active = 0; p->cmd = CMD_AGGRESSIVE; }

Combatant *party_cb(const Party *p, const PartyCtx *c, int idx) {
    if (idx < 0 || idx >= p->n) return NULL;
    return combat_get(c->cw, p->m[idx].ent);
}

PartyMember *party_add(Party *p, PartyCtx *c, const PartyDef *def, Vec3 pos, float yaw) {
    if (p->n >= PARTY_MAX) return NULL;
    PartyMember *m = &p->m[p->n];
    memset(m, 0, sizeof *m);
    Entity e = ecs_create(c->w, C_TRANSFORM | C_VELOCITY | C_HEALTH | C_FACTION);
    if (e == ENTITY_NULL) return NULL;
    uint32_t i = ecs_index(e);
    c->w->pos[i] = c->w->prev_pos[i] = pos;
    c->w->yaw[i] = c->w->prev_yaw[i] = yaw;
    c->w->faction[i] = FACTION_PLAYER;
    Combatant *cb = combat_add(c->cw, c->w, e, TEAM_PLAYER, def->level, def->stats, def->poise);
    if (!cb) { ecs_destroy(c->w, e); return NULL; }
    cb->weapon_atk = def->weapon_atk; cb->weapon_matk = def->weapon_matk; cb->armor = def->armor;
    cb->party_idx = p->n;
    cb->threat_mult = def->role == ROLE_TANK ? 2.5f : 1.0f;
    cb->has_resolve = def->role == ROLE_HERO;
    combatant_recalc(cb); cb->hp = cb->max_hp; cb->mp = cb->max_mp;
    m->used = true; m->ent = e; m->def = def; m->idx = p->n;
    player_init(&m->pc, pos);
    m->pc.yaw = yaw;
    p->n++;
    return m;
}

bool party_wiped(const Party *p, const PartyCtx *c) {
    for (int i = 0; i < p->n; ++i) { Combatant *cb = party_cb(p, c, i); if (cb && !cb->dead) return false; }
    return true;
}

int party_next_alive(const Party *p, const PartyCtx *c, int from) {
    for (int k = 1; k <= p->n; ++k) {
        int i = ((from + k) % p->n + p->n) % p->n;
        Combatant *cb = party_cb(p, c, i);
        if (cb && !cb->dead) return i;
    }
    return -1;
}

bool party_switch(Party *p, PartyCtx *c, int idx) {
    Combatant *cb = party_cb(p, c, idx);
    if (!cb || cb->dead || idx == p->active) return false;
    p->active = idx;
    return true;
}

void party_teleport(Party *p, PartyCtx *c, Vec3 pos, float yaw) {
    int slot = 0;
    for (int i = 0; i < p->n; ++i) {
        PartyMember *m = &p->m[i];
        Vec3 at = pos;
        if (i != p->active) {   /* the controlled member lands exactly on `pos`; the rest fan out behind */
            float side = ((slot & 1) ? 1.0f : -1.0f) * 1.6f * (1.0f + (float)(slot / 2));
            at.x += side * cosf(yaw) + 1.2f * sinf(yaw); at.z += -side * sinf(yaw) + 1.2f * cosf(yaw);
            ++slot;
        }
        player_init(&m->pc, at); m->pc.yaw = yaw;
        uint32_t k = ecs_index(m->ent);
        c->w->pos[k] = c->w->prev_pos[k] = at;
    }
}

void party_respawn_all(Party *p, PartyCtx *c, Vec3 pos) {
    for (int i = 0; i < p->n; ++i) {
        PartyMember *m = &p->m[i];
        Combatant *cb = combat_get(c->cw, m->ent);
        if (!cb) continue;
        cb->dead = false; cb->dead_t = 0; cb->hp = cb->max_hp; cb->mp = cb->max_mp; cb->resolve_used = false; cb->barrier = 0;
        memset(cb->status, 0, sizeof cb->status); cb->cast_skill = -1; cb->stagger_t = 0; cb->guarding = false;
        float yaw = m->pc.yaw;
        Vec3 at = vec3(pos.x + (float)(i - 1) * 1.6f, pos.y, pos.z + (i == 0 ? 0.0f : 1.5f));
        player_init(&m->pc, at); m->pc.yaw = yaw;
        uint32_t k = ecs_index(m->ent);
        c->w->pos[k] = c->w->prev_pos[k] = at;
    }
}

/* ---- casting shared by human and AI ---- */
static float aim_for(const PartyMember *m, const PartyCtx *c, const PlayerIn *pin, Entity target) {
    Combatant *tc = combat_get(c->cw, target);
    if (tc && tc->ent != m->ent) {
        Vec3 tp = c->w->pos[ecs_index(tc->ent)];
        float dx = tp.x - m->pc.ch.pos.x, dz = tp.z - m->pc.ch.pos.z;
        if (dx * dx + dz * dz > 1e-4f) return atan2f(dx, dz);
    }
    if (pin->mx * pin->mx + pin->mz * pin->mz > 1e-4f) return atan2f(pin->mx, pin->mz);
    return m->pc.yaw;
}

static bool member_cast(PartyMember *m, PartyCtx *c, PlayerIn *pin, int skill, Entity target) {
    if (skill < 0) return false;
    Combatant *cb = combat_get(c->cw, m->ent);
    if (!cb || cb->dead || cb->stagger_t > 0) return false;
    if (!m->pc.ch.grounded || player_rolling(&m->pc) || player_attacking(&m->pc)) return false;
    const Skill *s = &c->skills->s[skill];
    float aim = aim_for(m, c, pin, target);
    if (!combat_begin_cast(c->cw, c->w, m->ent, skill, target, aim)) return false;
    pin->attack = true; pin->attack_time = s->total_time; pin->has_aim = true; pin->aim_yaw = aim;
    return true;
}

static void member_basic(PartyMember *m, PartyCtx *c, PlayerIn *pin, Entity target) {
    int n = m->def->nbasic;
    if (n <= 0) return;
    if (member_cast(m, c, pin, m->def->basic[m->combo_idx % n], target)) { m->combo_idx = (m->combo_idx + 1) % n; m->combo_timer = 0.9f; m->attack_queued = false; }
}

void party_step(Party *p, PartyCtx *c, const PartyInput *in, bool first_step, float dt) {
    ecs_snapshot_transforms(c->w);
    for (int i = 0; i < p->n; ++i) {
        PartyMember *m = &p->m[i];
        Combatant *cb = combat_get(c->cw, m->ent);
        if (!cb) continue;
        uint32_t ei = ecs_index(m->ent);
        if (cb->dash_pending) { m->pc.ch.pos.x = cb->dash_to.x; m->pc.ch.pos.z = cb->dash_to.z; m->pc.ch.vel = vec3(0, 0, 0); cb->dash_pending = false; }
        PlayerIn pin = {0};
        bool human = i == p->active;
        if (cb->dead) {
            player_step(&m->pc, c->phys, &pin, dt);
            c->w->pos[ei] = m->pc.ch.pos; c->w->vel[ei] = vec3(0, 0, 0);
            continue;
        }
        cb->invuln = player_invulnerable(&m->pc);
        bool can_guard = m->pc.ch.grounded && !player_attacking(&m->pc) && !player_rolling(&m->pc) && cb->stagger_t <= 0;
        bool want_guard = false, do_roll = false, do_jump = false, do_attack = false;
        bool skill_press[4] = {false, false, false, false};
        Entity target = ENTITY_NULL;
        int ai_cast = -1; bool ai_basic = false;

        if (human) {
            pin = in->move;
            want_guard = in->guard; do_roll = in->roll && first_step; do_jump = in->jump && first_step;
            do_attack = in->attack && first_step;
            for (int k = 0; k < 4; ++k) skill_press[k] = in->skill[k] && first_step;
            target = in->lock;
            if (combat_alive(c->cw, in->lock)) { Vec3 lp = c->w->pos[ecs_index(in->lock)]; pin.has_lock = true; pin.lock_x = lp.x; pin.lock_z = lp.z; }
        } else {
            CompanionOut o;
            companion_decide(p, c, m, dt, &o);
            pin.mx = o.mx; pin.mz = o.mz; pin.sprint = o.sprint; pin.walk = o.walk;
            want_guard = o.guard; do_roll = o.roll; ai_cast = o.cast; ai_basic = o.basic; target = o.target;
        }
        if (cb->guarding) pin.walk = true;
        combat_set_guard(c->cw, m->ent, want_guard && can_guard);
        if (m->combo_timer > 0 && !player_attacking(&m->pc)) { m->combo_timer -= dt; if (m->combo_timer <= 0) m->combo_idx = 0; }

        if (!cb->guarding) {
            bool cast = false;
            if (human) {
                for (int k = 0; k < 4 && !cast; ++k) if (skill_press[k]) cast = member_cast(m, c, &pin, m->def->slots[k], target);
                if (!cast && do_attack) { if (player_attacking(&m->pc)) m->attack_queued = true; else member_basic(m, c, &pin, target); }
            } else if (ai_cast >= 0) cast = member_cast(m, c, &pin, ai_cast, target);
            else if (ai_basic && !player_attacking(&m->pc)) member_basic(m, c, &pin, target);
            if (m->attack_queued && !player_attacking(&m->pc) && !pin.attack) member_basic(m, c, &pin, target);
            pin.roll = do_roll; pin.jump = do_jump;
        }
        player_step(&m->pc, c->phys, &pin, dt);
        c->w->pos[ei] = m->pc.ch.pos; c->w->yaw[ei] = m->pc.yaw; c->w->vel[ei] = m->pc.ch.vel;
    }
}
