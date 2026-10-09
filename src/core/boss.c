#include "core/boss.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>
#include "core/log.h"

/* ---------------- data ---------------- */
static char *read_all(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { LOGE("cannot open %s", path); return NULL; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)n, f); fclose(f); buf[got] = 0;
    return buf;
}
static float num(const cJSON *o, const char *k, float def) { const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, k); return cJSON_IsNumber(v) ? (float)v->valuedouble : def; }
static const char *str(const cJSON *o, const char *k) { const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, k); return cJSON_IsString(v) ? v->valuestring : ""; }

static const char *MECH_NAMES[MK_COUNT] = {"adds", "vines", "rewind", "zones", "song", "armor", "barrier", "curse", "waves", "gravity", "finisher"};

bool bossdb_load(BossDB *db, const char *dir, const EnemyDB *edb, const SkillDB *skills) {
    memset(db, 0, sizeof *db);
    char path[256]; snprintf(path, sizeof path, "%s/bosses.json", dir);
    char *text = read_all(path);
    if (!text) return false;
    cJSON *root = cJSON_Parse(text); free(text);
    if (!root) { LOGE("bosses.json: parse error"); return false; }
    bool ok = true;
    const cJSON *b;
    cJSON_ArrayForEach(b, cJSON_GetObjectItemCaseSensitive(root, "bosses")) {
        if (db->n >= BOSS_MAX) { ok = false; break; }
        BossDef *d = &db->b[db->n++];
        snprintf(d->id, sizeof d->id, "%s", str(b, "id")); snprintf(d->enemy, sizeof d->enemy, "%s", str(b, "enemy"));
        snprintf(d->title, sizeof d->title, "%s", str(b, "title")); snprintf(d->outro, sizeof d->outro, "%s", str(b, "outro"));
        d->enemy_def = enemydb_find(edb, d->enemy);
        if (d->enemy_def < 0 || !edb->e[d->enemy_def].boss) { LOGE("bosses.json: %s is not a boss enemy", d->enemy); ok = false; }
        d->arena = num(b, "arena", 20); d->enrage = num(b, "enrage", 300); d->super = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(b, "super"));
        const cJSON *ph;
        cJSON_ArrayForEach(ph, cJSON_GetObjectItemCaseSensitive(b, "phases")) {
            if (d->nphase >= BOSS_PHASES) { ok = false; break; }
            PhaseDef *p = &d->phase[d->nphase++];
            p->hp_below = num(ph, "hp_below", 1.0f); snprintf(p->name, sizeof p->name, "%s", str(ph, "name"));
            p->damage = num(ph, "damage", 1.0f); p->speed = num(ph, "speed", 1.0f);
            const cJSON *e;
            cJSON_ArrayForEach(e, cJSON_GetObjectItemCaseSensitive(ph, "enter")) if (p->nenter < 3 && cJSON_IsString(e)) snprintf(p->enter[p->nenter++], 64, "%s", e->valuestring);
            cJSON_ArrayForEach(e, cJSON_GetObjectItemCaseSensitive(ph, "skills")) {
                int si = cJSON_IsString(e) ? skilldb_find(skills, e->valuestring) : -1;
                if (si < 0 || p->nskills >= 4) { LOGE("bosses.json: bad skill in %s", d->id); ok = false; continue; }
                p->skills[p->nskills++] = si;
            }
            const cJSON *m;
            cJSON_ArrayForEach(m, cJSON_GetObjectItemCaseSensitive(ph, "mech")) {
                if (p->nmech >= BOSS_MECH) { ok = false; break; }
                MechDef *md = &p->mech[p->nmech++];
                int k = -1; for (int i = 0; i < MK_COUNT; ++i) if (!strcmp(MECH_NAMES[i], str(m, "kind"))) k = i;
                if (k < 0) { LOGE("bosses.json: unknown mechanic '%s' in %s", str(m, "kind"), d->id); ok = false; k = 0; }
                md->kind = (MechKind)k;
                md->every = num(m, "every", 20); md->power = num(m, "power", 0); md->dur = num(m, "dur", 6); md->radius = num(m, "radius", 3);
                md->reflect = num(m, "reflect", 0); md->count = (int)num(m, "count", 1); md->max = (int)num(m, "max", 4);
                const char *hz = str(m, "hazard");
                md->hazard = !strcmp(hz, "sand") ? HZ_SAND : !strcmp(hz, "rift") ? HZ_RIFT : !strcmp(hz, "fire") ? HZ_FIRE : HZ_NONE;
                if (md->kind == MK_ZONES && md->hazard == HZ_NONE) { LOGE("bosses.json: zones need a hazard in %s", d->id); ok = false; }
                snprintf(md->enemy, sizeof md->enemy, "%s", str(m, "enemy")); md->enemy_def = -1;
                if (md->enemy[0]) { md->enemy_def = enemydb_find(edb, md->enemy); if (md->enemy_def < 0) { LOGE("bosses.json: unknown add '%s' in %s", md->enemy, d->id); ok = false; } }
                if ((md->kind == MK_ADDS || md->kind == MK_VINES) && md->enemy_def < 0 && md->kind == MK_ADDS) { LOGE("bosses.json: adds need an enemy in %s", d->id); ok = false; }
            }
        }
        if (d->nphase == 0 || d->phase[0].hp_below < 0.99f) { LOGE("bosses.json: %s needs a first phase at 1.0", d->id); ok = false; }
        for (int i = 1; i < d->nphase; ++i) if (d->phase[i].hp_below >= d->phase[i - 1].hp_below) { LOGE("bosses.json: %s phases must descend", d->id); ok = false; }
    }
    cJSON_Delete(root);
    return ok && db->n > 0;
}

int bossdb_find(const BossDB *db, const char *enemy_id) { for (int i = 0; i < db->n; ++i) if (!strcmp(db->b[i].enemy, enemy_id)) return i; return -1; }

/* ---------------- helpers ---------------- */
static void push(BossRt *rt, const char *s) { if (rt->nout < 10) snprintf(rt->out[rt->nout++], 64, "%s", s); }
static void pushf(BossRt *rt, const char *fmt, const char *a) { char b[64]; snprintf(b, sizeof b, fmt, a); push(rt, b); }

const char *boss_pop_action(BossRt *rt) {
    static char cur[64];
    if (rt->nout <= 0) return NULL;
    snprintf(cur, sizeof cur, "%s", rt->out[0]);
    memmove(rt->out[0], rt->out[1], (size_t)(rt->nout - 1) * 64); --rt->nout;
    return cur;
}

static Combatant *boss_cb(BossRt *rt, PartyCtx *pc) { return combat_get(pc->cw, rt->ent); }
float boss_hp_frac(const BossRt *rt, PartyCtx *pc) { Combatant *c = combat_get(pc->cw, rt->ent); return c && c->max_hp > 0 ? c->hp / c->max_hp : 0.0f; }
const char *boss_phase_name(const BossRt *rt, const BossDB *db) { return db->b[rt->def].phase[rt->phase].name; }

static Vec3 pos_of(PartyCtx *pc, Entity e) { return pc->w->pos[ecs_index(e)]; }
static float d2(Vec3 a, float x, float z) { return hypotf(a.x - x, a.z - z); }

static int alive_members(Party *pt, PartyCtx *pc, int *idx, int cap) {
    int n = 0;
    for (int i = 0; i < pt->n && n < cap; ++i) { Combatant *c = party_cb(pt, pc, i); if (c && !c->dead && !c->ghost) idx[n++] = i; }
    return n;
}

static int count_alive(PartyCtx *pc, Entity *list, int n) {
    int a = 0;
    for (int i = 0; i < n; ++i) { if (list[i] != ENTITY_NULL && combat_alive(pc->cw, list[i]) && ecs_alive(pc->w, list[i])) ++a; else list[i] = ENTITY_NULL; }
    return a;
}

static Entity spawn_add(BossRt *rt, PartyCtx *pc, int def, float rmin, float rmax, Entity *list, int cap) {
    int slot = -1; for (int i = 0; i < cap; ++i) if (list[i] == ENTITY_NULL) { slot = i; break; }
    if (slot < 0) return ENTITY_NULL;
    float a = rng_range(&rt->rng, 0.0f, 6.2831853f), r = rng_range(&rt->rng, rmin, rmax);
    float x = rt->center.x + cosf(a) * r, z = rt->center.z + sinf(a) * r;
    Enemy *e = enemy_spawn(pc->enemies, pc->cw, pc->w, pc->edb, def, vec3(x, phys_ground(pc->phys, x, z), z));
    if (!e) return ENTITY_NULL;
    e->state = AI_CHASE; e->home = vec3(x, 0, z);
    list[slot] = e->ent;
    return e->ent;
}

static void clear_list(PartyCtx *pc, Entity *list, int n) {
    for (int i = 0; i < n; ++i) if (list[i] != ENTITY_NULL) { if (ecs_alive(pc->w, list[i])) enemy_remove(pc->enemies, pc->cw, pc->w, list[i]); list[i] = ENTITY_NULL; }
}

static const MechDef *find_mech(const BossDef *d, int phase, MechKind k) {
    for (int i = 0; i < d->phase[phase].nmech; ++i) if (d->phase[phase].mech[i].kind == k) return &d->phase[phase].mech[i];
    return NULL;
}

static void free_downed(BossRt *rt, Party *pt, PartyCtx *pc) {
    for (int i = 0; i < pt->n; ++i) if (rt->downed[i]) { Combatant *c = party_cb(pt, pc, i); if (c) { c->ghost = false; c->status[ST_STUN].time = 0; } rt->downed[i] = false; }
    rt->ndowned = 0; rt->lock_control = false; rt->callout_ready = false;
}

/* ---------------- phases ---------------- */
static void enter_phase(BossRt *rt, const BossDB *db, Party *pt, PartyCtx *pc, int p) {
    const BossDef *d = &db->b[rt->def];
    const PhaseDef *ph = &d->phase[p];
    Enemy *en = enemy_of(pc->enemies, rt->ent);
    rt->phase = p; rt->phase_time = 0; rt->tele_t = 0;
    for (int i = 0; i < BOSS_MECH; ++i) rt->mt[i] = i < ph->nmech ? ph->mech[i].every * 0.6f : 0;
    if (en) {
        en->nsk = ph->nskills; for (int i = 0; i < ph->nskills; ++i) en->sk[i] = ph->skills[i];
        en->speed_mult = ph->speed > 0 ? ph->speed : 1.0f;
    }
    for (int i = 0; i < ph->nenter; ++i) push(rt, ph->enter[i]);
    const MechDef *arm = find_mech(d, p, MK_ARMOR);
    Combatant *bc = boss_cb(rt, pc);
    if (arm && bc) { rt->armored = true; rt->armor_max = rt->armor_meter = arm->power * bc->max_hp; rt->exposed_t = 0; } else { rt->armored = false; rt->exposed_t = 0; }
    rt->barrier = false; rt->barrier_t = rt->barrier_down_t = 0;
    memset(rt->hz, 0, sizeof rt->hz);
    const MechDef *gv = find_mech(d, p, MK_GRAVITY);
    if (gv) {   /* the Solo Ascent: every ally but the hero goes down */
        rt->grav_t = 0; rt->called = 0; rt->call_cd = 0; rt->callout_ready = false; rt->first_call = true; rt->lock_control = true; rt->ndowned = 0;
        for (int i = 1; i < pt->n; ++i) {
            Combatant *c = party_cb(pt, pc, i);
            if (c) { rt->downed[i] = true; ++rt->ndowned; c->ghost = true; c->dead = false; c->hp = fmaxf(c->hp, 1.0f); c->guarding = false; c->cast_skill = -1; }
        }
        if (pt->active != 0) party_switch(pt, pc, 0);
    }
}

void boss_start(BossRt *rt, const BossDB *db, int def, Entity ent, Vec3 center, PartyCtx *pc) {
    memset(rt, 0, sizeof *rt);
    rt->active = true; rt->def = def; rt->ent = ent; rt->center = center; rt->dmg_scale = 1.0f;
    rng_seed(&rt->rng, 777u + (uint32_t)def);
    Combatant *bc = combat_get(pc->cw, ent);
    for (int i = 0; i < 21; ++i) rt->hist[i] = bc ? bc->hp : 0;
    rt->nhist = 21;
    Party dummy = {0};
    enter_phase(rt, db, &dummy, pc, 0);
    /* enter_phase may touch party for gravity; phase 0 never has it */
}

void boss_end(BossRt *rt, Party *pt, PartyCtx *pc) {
    clear_list(pc, rt->adds, BOSS_ADDS); clear_list(pc, rt->vines, 6);
    free_downed(rt, pt, pc);
    memset(rt->hz, 0, sizeof rt->hz);
    Combatant *bc = boss_cb(rt, pc);
    if (bc) { bc->dmg_taken = 1; bc->dmg_dealt = 1; bc->reflect = 0; }
    rt->active = false;
}

void boss_reset(BossRt *rt, const BossDB *db, Party *pt, PartyCtx *pc) {
    Combatant *bc = boss_cb(rt, pc);
    Enemy *en = enemy_of(pc->enemies, rt->ent);
    clear_list(pc, rt->adds, BOSS_ADDS); clear_list(pc, rt->vines, 6);
    free_downed(rt, pt, pc);
    if (bc) { bc->hp = bc->max_hp; bc->dmg_taken = 1; bc->dmg_dealt = 1; bc->reflect = 0; memset(bc->status, 0, sizeof bc->status); }
    if (en) { en->ctl.pos = en->home; pc->w->pos[ecs_index(rt->ent)] = en->home; en->state = AI_RETURN; }
    rt->time = 0; rt->enraged = false; rt->dmg_scale = rt->dmg_scale > 0 ? rt->dmg_scale : 1.0f; rt->finisher_ready = rt->quartet = false; rt->quartet_t = 0; rt->quartet_step = 0;
    rt->nout = 0; memset(rt->hz, 0, sizeof rt->hz);
    for (int i = 0; i < 21; ++i) rt->hist[i] = bc ? bc->hp : 0;
    Party dummy = {0};
    enter_phase(rt, db, &dummy, pc, 0);
}

/* ---------------- mechanics ---------------- */
static void telegraph(BossRt *rt, MechKind k, float lead, const char *msg) { if (rt->tele_t <= 0) { rt->tele_t = lead; rt->tele_kind = k; if (msg) push(rt, msg); } }

static void add_hazard(BossRt *rt, HazardKind kind, float x, float z, float r, float tele, float dur, float power, float grow) {
    for (int i = 0; i < BOSS_HZ; ++i) if (!rt->hz[i].on) { rt->hz[i] = (Hazard){.on = true, .kind = kind, .x = x, .z = z, .r = r, .tele = tele, .t = dur, .power = power, .grow = grow}; return; }
}

static void update_hazards(BossRt *rt, const BossDef *d, Party *pt, PartyCtx *pc, float dt) {
    for (int i = 0; i < BOSS_HZ; ++i) {
        Hazard *h = &rt->hz[i];
        if (!h->on) continue;
        if (h->tele > 0) { h->tele -= dt; continue; }
        if (h->kind == HZ_WAVE) { h->r += h->grow * dt; if (h->r > d->arena + 6.0f) { h->on = false; continue; } }
        else { h->t -= dt; if (h->t <= 0) { h->on = false; continue; } }
        for (int m = 0; m < pt->n; ++m) {
            Combatant *c = party_cb(pt, pc, m);
            if (!c || c->dead || c->ghost) continue;
            Vec3 p = pc->w->pos[ecs_index(c->ent)];
            float dist = d2(p, h->x, h->z);
            if (h->kind == HZ_WAVE) {
                bool in_ring = dist > h->r - 1.2f && dist < h->r + 0.6f;
                if (in_ring && !(h->hit_mask & (1u << m)) && !c->invuln && pt->m[m].pc.ch.grounded) {
                    h->hit_mask |= 1u << m;
                    combat_damage_raw(pc->cw, pc->w, c, h->power * c->max_hp, rt->ent);
                    c->stagger_t = fmaxf(c->stagger_t, 0.5f);
                }
            } else if (dist < h->r) {
                if (h->kind == HZ_SAND) combat_apply_status(pc->cw, c, ST_SLOW, 0.4f, 0.55f, rt->ent);
                else if (!c->invuln) combat_damage_raw(pc->cw, pc->w, c, h->power * c->max_hp * dt, rt->ent);
            }
        }
    }
}

static void handle_events(BossRt *rt, const BossDef *d, Party *pt, PartyCtx *pc, Combatant *bc) {
    CombatWorld *cw = pc->cw;
    if (cw->nev < rt->ev_seen) rt->ev_seen = 0;   /* the game cleared the stream */
    for (int i = rt->ev_seen; i < cw->nev; ++i) {
        const CombatEvent *e = &cw->ev[i];
        if (e->type != CE_HIT || e->dst != rt->ent) continue;
        if (rt->armored && bc->dmg_taken > 0) {
            float raw = e->amount / fmaxf(bc->dmg_taken, 0.05f);
            const char *sid = e->skill >= 0 && e->skill < cw->skills->n ? cw->skills->s[e->skill].id : "";
            float drain = 0;
            if (e->elem == EL_ICE) drain = raw * 1.5f;
            if (!strcmp(sid, "shield_bash") || !strcmp(sid, "ground_slam") || !strcmp(sid, "guardians_roar")) drain = fmaxf(drain, raw * 2.0f);
            if (drain > 0) rt->armor_meter -= drain;
        }
        if (rt->barrier && e->elem != EL_PHYS) {
            Combatant *src = combat_get(cw, e->src);
            bc->hp = fminf(bc->max_hp, bc->hp + e->amount);                       /* the barrier throws the spell back */
            if (src && !src->dead) combat_damage_raw(cw, pc->w, src, e->amount * 0.5f, rt->ent);
            bool converge = false;
            for (int k = 0; k < EL_COUNT; ++k) if (k != (int)e->elem && k != (int)EL_PHYS && rt->time - rt->elem_t[k] < 3.0f && rt->elem_t[k] > 0) converge = true;
            rt->elem_t[e->elem] = rt->time;
            if (converge) {
                rt->barrier = false; rt->barrier_down_t = 8.0f; bc->stagger_t = fmaxf(bc->stagger_t, 2.0f);
                push(rt, "toast:The elements converge - the barrier shatters!");
            }
        }
    }
    rt->ev_seen = cw->nev;
    (void)d; (void)pt;
}

bool boss_call_out(BossRt *rt, Party *pt, PartyCtx *pc) {
    if (!rt->active || !rt->callout_ready || rt->call_cd > 0) return false;
    const BossDef *none = NULL; (void)none;
    for (int i = 1; i < pt->n; ++i) {
        if (!rt->downed[i]) continue;
        Combatant *c = party_cb(pt, pc, i);
        rt->downed[i] = false; --rt->ndowned; ++rt->called; rt->call_cd = 3.0f;
        if (c) {
            c->ghost = false; c->dead = false; memset(c->status, 0, sizeof c->status);
            c->hp = c->max_hp * 0.6f; c->mp = c->max_mp; c->stagger_t = 0;
            combat_apply_status(pc->cw, c, ST_BUFF_ATK, 15.0f, 0.3f, c->ent);
            Combatant *k = party_cb(pt, pc, 0);
            Vec3 kp = k ? pos_of(pc, k->ent) : rt->center;
            Vec3 at = vec3(kp.x + (i == 1 ? -2.0f : i == 2 ? 2.0f : 0.0f), kp.y, kp.z - 1.5f - (float)(i / 3));
            player_init(&pt->m[i].pc, at);
            pc->w->pos[ecs_index(c->ent)] = pc->w->prev_pos[ecs_index(c->ent)] = at;
        }
        Combatant *bc = boss_cb(rt, pc);
        if (bc) bc->stagger_t = fmaxf(bc->stagger_t, 2.0f);        /* the return skill staggers the king */
        char b[64]; snprintf(b, sizeof b, "return:%d", i); push(rt, b);
        if (rt->first_call) { rt->first_call = false; push(rt, "cutscene:asks_for_help"); }
        return true;
    }
    return false;
}

bool boss_quartet(BossRt *rt) {
    if (!rt->active || !rt->finisher_ready || rt->quartet) return false;
    rt->quartet = true; rt->quartet_t = 0; rt->quartet_step = 0;
    push(rt, "cutscene:quartet_strike");
    return true;
}

static void finish_boss(BossRt *rt, const BossDef *d, Party *pt, PartyCtx *pc) {
    rt->defeated = true; rt->active = false;
    clear_list(pc, rt->adds, BOSS_ADDS); clear_list(pc, rt->vines, 6);
    free_downed(rt, pt, pc);
    memset(rt->hz, 0, sizeof rt->hz);
    if (d->outro[0]) pushf(rt, "cutscene:%s", d->outro);
}

void boss_update(BossRt *rt, const BossDB *db, Party *pt, PartyCtx *pc, float dt) {
    if (!rt->active) return;
    const BossDef *d = &db->b[rt->def];
    Combatant *bc = boss_cb(rt, pc);
    Enemy *en = enemy_of(pc->enemies, rt->ent);
    if (!bc || !en) { rt->active = false; return; }
    if (bc->dead) { finish_boss(rt, d, pt, pc); return; }
    rt->time += dt; rt->phase_time += dt;
    if (rt->call_cd > 0) rt->call_cd -= dt;
    if (rt->tele_t > 0) rt->tele_t -= dt;

    handle_events(rt, d, pt, pc, bc);

    /* HP history for the rewind (21 samples, 0.5 s apart: [20] is 10 s ago) */
    rt->hist_t += dt;
    if (rt->hist_t >= 0.5f) { rt->hist_t -= 0.5f; memmove(&rt->hist[1], &rt->hist[0], 20 * sizeof(float)); rt->hist[0] = bc->hp; }

    /* phase by HP */
    float frac = bc->hp / bc->max_hp;
    while (rt->phase + 1 < d->nphase && d->phase[rt->phase + 1].hp_below > 0.0f && frac <= d->phase[rt->phase + 1].hp_below) {
        enter_phase(rt, db, pt, pc, rt->phase + 1);
    }
    const PhaseDef *ph = &d->phase[rt->phase];

    float taken = 1.0f, reflect = 0.0f, dealt = ph->damage > 0 ? ph->damage : 1.0f, speed = ph->speed > 0 ? ph->speed : 1.0f;
    int idx[PARTY_MAX]; int nalive = alive_members(pt, pc, idx, PARTY_MAX);

    for (int mi = 0; mi < ph->nmech; ++mi) {
        const MechDef *m = &ph->mech[mi];
        switch (m->kind) {
        case MK_ADDS:
            rt->mt[mi] -= dt;
            if (rt->mt[mi] <= 0) {
                rt->mt[mi] = m->every;
                int alive = count_alive(pc, rt->adds, BOSS_ADDS);
                for (int k = 0; k < m->count && alive < m->max; ++k) if (spawn_add(rt, pc, m->enemy_def, 4.0f, 8.0f, rt->adds, BOSS_ADDS) != ENTITY_NULL) ++alive;
            }
            break;
        case MK_VINES: {
            int alive = count_alive(pc, rt->vines, 6);
            if (alive == 0) {
                rt->mt[mi] -= dt;
                if (rt->mt[mi] <= 0) {
                    rt->mt[mi] = m->every;
                    int vd = enemydb_find(pc->edb, "thorn_vine");
                    for (int k = 0; k < m->count && vd >= 0; ++k) spawn_add(rt, pc, vd, 7.0f, 12.0f, rt->vines, 6);
                    push(rt, "toast:Living vines feed the Thornqueen - burn them!");
                }
            } else bc->hp = fminf(bc->max_hp, bc->hp + m->power * bc->max_hp * (float)alive * dt);
        } break;
        case MK_REWIND:
            rt->mt[mi] -= dt;
            if (rt->mt[mi] <= 1.5f && rt->tele_t <= 0 && rt->mt[mi] > 0) telegraph(rt, MK_REWIND, 1.5f, "toast:The sands begin to turn back...");
            if (rt->mt[mi] <= 0) {
                rt->mt[mi] = m->every;
                float ref = rt->hist[20];
                if (ref > bc->hp) { bc->hp = ref; push(rt, "toast:TIME REWINDS!"); push(rt, "vfx:rewind"); }
                for (int k = 0; k < 21; ++k) rt->hist[k] = bc->hp;
            }
            break;
        case MK_ZONES:
            rt->mt[mi] -= dt;
            if (rt->mt[mi] <= 0 && nalive > 0) {
                rt->mt[mi] = m->every;
                for (int k = 0; k < m->count; ++k) {
                    Combatant *c = party_cb(pt, pc, idx[rng_int(&rt->rng, 0, nalive - 1)]);
                    Vec3 p = pos_of(pc, c->ent);
                    add_hazard(rt, m->hazard, p.x + rng_range(&rt->rng, -2.5f, 2.5f), p.z + rng_range(&rt->rng, -2.5f, 2.5f), m->radius, 1.2f, m->dur, m->power, 0);
                }
            }
            break;
        case MK_SONG:
            rt->mt[mi] -= dt;
            if (rt->mt[mi] <= 1.5f && rt->mt[mi] > 0 && rt->tele_t <= 0) telegraph(rt, MK_SONG, 1.5f, "toast:The choir draws breath...");
            if (rt->mt[mi] <= 0 && nalive > 0) {
                rt->mt[mi] = m->every;
                for (int k = 0; k < nalive; ++k) combat_apply_status(pc->cw, party_cb(pt, pc, idx[k]), ST_SILENCE, m->dur, 1.0f, rt->ent);
                int vic = idx[rng_int(&rt->rng, 0, nalive - 1)];
                combat_apply_status(pc->cw, party_cb(pt, pc, vic), ST_SLOW, m->dur, 0.85f, rt->ent);
                char b[64]; snprintf(b, sizeof b, "freeze:%d", vic); push(rt, b);
                push(rt, "toast:Spells silenced - an ally is frozen (cleanse thaws)!");
            }
            break;
        case MK_ARMOR:
            if (rt->armored) {
                taken = 0.25f; reflect = m->reflect;
                if (rt->armor_meter <= 0) { rt->armored = false; rt->exposed_t = m->dur; bc->stagger_t = fmaxf(bc->stagger_t, 2.5f); push(rt, "toast:The armor breaks - strike now!"); }
            } else {
                taken = 1.4f; rt->exposed_t -= dt;
                if (rt->exposed_t <= 0) { rt->armored = true; rt->armor_meter = rt->armor_max; push(rt, "toast:The armor reforms."); }
            }
            break;
        case MK_BARRIER:
            if (rt->barrier) { rt->barrier_t -= dt; if (rt->barrier_t <= 0) rt->barrier = false; }
            else {
                if (rt->barrier_down_t > 0) rt->barrier_down_t -= dt;
                rt->mt[mi] -= dt;
                if (rt->mt[mi] <= 0 && rt->barrier_down_t <= 0) {
                    rt->mt[mi] = m->every; rt->barrier = true; rt->barrier_t = m->dur;
                    for (int k = 0; k < EL_COUNT; ++k) rt->elem_t[k] = 0;
                    push(rt, "toast:A barrier of despair - combine two elements to break it!");
                }
            }
            break;
        case MK_CURSE:
            rt->mt[mi] -= dt;
            if (rt->mt[mi] <= 0 && nalive > 0) {
                rt->mt[mi] = m->every;
                Combatant *c = party_cb(pt, pc, idx[rng_int(&rt->rng, 0, nalive - 1)]);
                combat_apply_status(pc->cw, c, ST_HOLLOW_CURSE, m->dur, 1.0f, rt->ent);
                push(rt, "toast:Hollow Curse - healing is blocked until cleansed!");
            }
            break;
        case MK_WAVES:
            rt->mt[mi] -= dt;
            if (rt->mt[mi] <= 0.8f && rt->mt[mi] > 0 && rt->tele_t <= 0) telegraph(rt, MK_WAVES, 0.8f, NULL);
            if (rt->mt[mi] <= 0) { rt->mt[mi] = m->every; Vec3 bp = pos_of(pc, rt->ent); add_hazard(rt, HZ_WAVE, bp.x, bp.z, 1.5f, 0.0f, 0, m->power, 9.0f); push(rt, "toast:Void wave - jump!"); }
            break;
        case MK_GRAVITY: {
            taken = 0.0f;
            rt->grav_t += dt;
            dealt *= 1.0f + 0.03f * rt->grav_t;
            if (!rt->callout_ready && rt->grav_t >= m->every && rt->ndowned > 0) { rt->callout_ready = true; push(rt, "toast:Call Out! (H)"); }
            Combatant *k = party_cb(pt, pc, 0);
            if (k && k->dead) {   /* alone and fallen: the ascent starts over */
                k->dead = false; k->dead_t = 0; k->hp = k->max_hp * m->power; k->stagger_t = 0; memset(k->status, 0, sizeof k->status);
                rt->grav_t = 0; dealt = 1.0f;
                push(rt, "toast:Kael gets back up.");
            }
            if (rt->ndowned == 0 && rt->called > 0) {   /* everyone is back: the Hollow King shows his true form */
                rt->lock_control = false;
                if (rt->phase + 1 < d->nphase) enter_phase(rt, db, pt, pc, rt->phase + 1);
                bc->dmg_taken = 1.0f; bc->dmg_dealt = d->phase[rt->phase].damage > 0 ? d->phase[rt->phase].damage : 1.0f;
                return;
            }
        } break;
        case MK_FINISHER:
            if (frac <= m->power || rt->finisher_ready) {
                if (!rt->finisher_ready) { rt->finisher_ready = true; push(rt, "toast:QUARTET STRIKE ready (F)!"); }
                bc->hp = fmaxf(1.0f, fminf(bc->hp, m->power * bc->max_hp * 0.999f)); taken = 0.0f;
                if (rt->quartet) {
                    static const float T[4] = {0.3f, 1.5f, 2.7f, 3.9f};
                    static const char *WHO[4] = {"strike:bram", "strike:lyra", "strike:mirelle", "strike:kael"};
                    rt->quartet_t += dt;
                    while (rt->quartet_step < 4 && rt->quartet_t >= T[rt->quartet_step]) push(rt, WHO[rt->quartet_step++]);
                    if (rt->quartet_step >= 4 && rt->quartet_t >= 5.4f) {
                        Combatant *hero = party_cb(pt, pc, 0);
                        bc->dmg_taken = 1.0f;
                        combat_damage_raw(pc->cw, pc->w, bc, bc->hp + 1.0f, hero ? hero->ent : ENTITY_NULL);
                        push(rt, "quartet:done");
                    }
                }
            }
            break;
        default: break;
        }
    }

    /* enrage */
    if (!rt->enraged && d->enrage > 0 && rt->time > d->enrage) { rt->enraged = true; push(rt, "toast:ENRAGED!"); }
    if (rt->enraged) { dealt *= fminf(2.5f, 1.5f + (rt->time - d->enrage) / 60.0f * 0.5f); speed *= 1.2f; }

    dealt *= rt->dmg_scale;
    bc->dmg_taken = taken; bc->reflect = reflect; bc->dmg_dealt = dealt; en->speed_mult = speed;
    update_hazards(rt, d, pt, pc, dt);
    if (bc->dead) finish_boss(rt, d, pt, pc);
}
