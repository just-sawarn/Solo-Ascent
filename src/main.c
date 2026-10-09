#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include "core/camrig.h"
#include "core/combat.h"
#include "core/data.h"
#include "core/ecs.h"
#include "core/enemy_ai.h"
#include "core/gameloop.h"
#include "core/log.h"
#include "core/party.h"
#include "core/physics.h"
#include "core/player.h"
#include "platform/platform.h"
#include "render/actors.h"
#include "render/heroes.h"
#include "render/procmodels.h"
#include "render/renderer.h"
#include "ui/debug_overlay.h"
#include "ui/hud.h"
#include "ui/input.h"
#include "world/test_scene.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

typedef enum { STATE_TITLE, STATE_PLAYING } GameState;

typedef struct {
    World world;
    GameLoop loop;
    DebugState dbg;
    GameState state;
    PhysWorld phys;
    CamRig cam;
    Vec3 cam_focus;
    Input input;
    SkillDB skills;
    EnemyDB edb;
    PartyDB pdb;
    EnemyPool enemies;
    CombatWorld cw;
    PartyCtx ctx;
    Party party;
    ActorAnim actors[ENEMY_MAX];
    Hud hud;
    Entity lock_ent;
    float hitstop, respawn_t;
    int kills; float last_damage;
    bool arena, data_ok;
    bool auto_orbit, governor_on;
    double title_t, time;
    double toast_t; char toast[96];
    /* per-member one-frame animation triggers accumulated over sim steps */
    bool ev_roll[PARTY_MAX], ev_cast[PARTY_MAX];
    int cast_skill[PARTY_MAX];
} Game;

static Game G; /* static: World and PhysWorld are large, no per-frame allocation anywhere */

static void toast(Game *g, const char *fmt, int a, const char *b) {
    snprintf(g->toast, sizeof g->toast, fmt, a, b);
    g->toast_t = 2.5;
}

static void toast_text(Game *g, const char *s) { snprintf(g->toast, sizeof g->toast, "%s", s); g->toast_t = 2.5; }

static const struct { const char *id; float x, z; } SPAWNS[] = {
    {"shadow_soldier", -9.0f, -13.0f}, {"shadow_soldier", 9.0f, -13.0f}, {"shadow_archer", 0.0f, -19.0f},
    {"shadow_brute", 15.0f, -6.0f}, {"shadow_soldier", -15.0f, 9.0f},
};

static void spawn_enemy(Game *g, const char *id, float x, float z) {
    int def = enemydb_find(&g->edb, id);
    if (def >= 0) enemy_spawn(&g->enemies, &g->cw, &g->world, &g->edb, def, vec3(x, 0, z));
}

static const char *CMD_NAMES[] = {"Aggressive", "Defensive", "Support"};

static PartyMember *active_member(Game *g) { return &g->party.m[g->party.active < 0 ? 0 : g->party.active]; }
static Entity active_ent(Game *g) { return active_member(g)->ent; }

static void new_game(Game *g) {
    ecs_init(&g->world);
    gameloop_init(&g->loop);
    combat_init(&g->cw, &g->skills);
    enemies_init(&g->enemies);
    hud_init(&g->hud);
    memset(g->actors, 0, sizeof g->actors);
    memset(g->ev_roll, 0, sizeof g->ev_roll); memset(g->ev_cast, 0, sizeof g->ev_cast);
    g->ctx = (PartyCtx){&g->cw, &g->world, &g->phys, &g->enemies, &g->edb, &g->skills};
    party_init(&g->party);
    g->party.cmd = CMD_DEFENSIVE;
    static const float OX[4] = {0.0f, -2.4f, 2.4f, 0.0f}, OZ[4] = {0.5f, 2.2f, 2.6f, 4.2f};
    for (int i = 0; i < g->pdb.n && i < PARTY_MAX; ++i) party_add(&g->party, &g->ctx, &g->pdb.m[i], vec3(OX[i], 0, OZ[i]), 3.14159265f);
    g->party.active = 0;
    if (g->arena) spawn_enemy(g, "shadow_brute", 0.0f, -4.5f);
    else for (unsigned i = 0; i < sizeof SPAWNS / sizeof SPAWNS[0]; ++i) spawn_enemy(g, SPAWNS[i].id, SPAWNS[i].x, SPAWNS[i].z);
    g->lock_ent = ENTITY_NULL;
    g->cam.locked = false;
    g->kills = 0; g->respawn_t = 0; g->hitstop = 0;
    Vec3 ap = active_member(g)->pc.ch.pos;
    g->cam_focus = ap;
}

static Vector3 lock_pos(const Game *g) {
    if (g->lock_ent == ENTITY_NULL || !ecs_alive(&g->world, g->lock_ent)) return (Vector3){0, 0, 0};
    Vec3 p = g->world.pos[ecs_index(g->lock_ent)];
    return (Vector3){p.x, p.y, p.z};
}

static PartyInput make_party_input(const Game *g) {
    const Input *in = &g->input;
    float cs = cosf(g->cam.yaw), sn = sinf(g->cam.yaw); /* camera-relative movement */
    PartyInput pin = {0};
    pin.move.mx = in->move_x * cs - in->move_y * sn;
    pin.move.mz = -in->move_x * sn - in->move_y * cs;
    pin.move.sprint = in->sprint; pin.move.walk = in->walk;
    pin.attack = in->attack; pin.jump = in->jump; pin.roll = in->roll; pin.guard = in->guard;
    for (int k = 0; k < 4; ++k) pin.skill[k] = in->skill[k];
    pin.lock = (g->lock_ent != ENTITY_NULL && combat_alive(&g->cw, g->lock_ent)) ? g->lock_ent : ENTITY_NULL;
    return pin;
}

/* Fixed 60 Hz simulation step. */
static void sim_step(Game *g, bool first_step) {
    Party *pt = &g->party;
    PartyInput pin = make_party_input(g);
    pt->focus = pin.lock;
    party_step(pt, &g->ctx, &pin, first_step, SIM_DT);
    for (int i = 0; i < pt->n; ++i) { g->ev_roll[i] |= pt->m[i].pc.ev_roll; }
    enemies_update(&g->enemies, &g->cw, &g->world, &g->phys, &g->edb, SIM_DT);
    combat_update(&g->cw, &g->world, &g->phys, SIM_DT);
    if (g->dbg.god_mode) for (int i = 0; i < pt->n; ++i) { Combatant *c = party_cb(pt, &g->ctx, i); if (c && !c->dead) c->hp = c->max_hp; }

    /* the controlled member fell: hand control to someone alive; a full wipe respawns the party */
    Combatant *ac = party_cb(pt, &g->ctx, pt->active);
    if (ac && ac->dead) { int nx = party_next_alive(pt, &g->ctx, pt->active); if (nx >= 0) { party_switch(pt, &g->ctx, nx); g->lock_ent = ENTITY_NULL; } }
    if (party_wiped(pt, &g->ctx)) {
        g->respawn_t += SIM_DT;
        if (g->respawn_t > 3.5f) { party_respawn_all(pt, &g->ctx, vec3(0, 0, 0.5f)); pt->active = 0; g->lock_ent = ENTITY_NULL; g->respawn_t = 0; }
    }
}

/* Presentation reactions to this frame's combat events. */
static void process_events(Game *g) {
    Entity act = active_ent(g);
    for (int i = 0; i < g->cw.nev; ++i) {
        const CombatEvent *e = &g->cw.ev[i];
        bool by_active = e->src == act, on_party = false;
        for (int k = 0; k < g->party.n; ++k) {
            if (e->dst == g->party.m[k].ent) on_party = true;
            if (e->type == CE_CAST && e->amount < 0.5f && e->src == g->party.m[k].ent) { g->ev_cast[k] = true; g->cast_skill[k] = e->skill; }
        }
        bool on_active = e->dst == act;
        switch (e->type) {
        case CE_HIT:
            if (by_active) { g->hitstop = fmaxf(g->hitstop, e->crit ? 0.085f : 0.05f); camrig_shake(&g->cam, e->crit ? 0.05f : 0.02f); g->last_damage = e->amount; }
            else if (!on_party) camrig_shake(&g->cam, 0.008f);
            if (on_active) { g->hitstop = fmaxf(g->hitstop, 0.045f); camrig_shake(&g->cam, 0.06f); }
            break;
        case CE_PARRY: g->hitstop = fmaxf(g->hitstop, 0.14f); camrig_shake(&g->cam, 0.09f); break;
        case CE_BLOCK: camrig_shake(&g->cam, 0.03f); break;
        case CE_DEATH:
            if (!on_party) { g->kills++; g->hitstop = fmaxf(g->hitstop, 0.10f); camrig_shake(&g->cam, 0.08f); }
            else camrig_shake(&g->cam, 0.15f);
            break;
        case CE_CAST:
            if (e->amount > 0.5f && by_active) camrig_shake(&g->cam, 0.05f); /* ground impact */
            break;
        default: break;
        }
    }
    hud_on_events(&g->hud, g->cw.ev, g->cw.nev, &g->party);
    g->cw.nev = 0;
}

static void handle_render_keys(Game *g) {
    if (IsKeyPressed(KEY_L)) {
        int l = (renderer_look() + 1) % LOOK_COUNT;
        renderer_set_look(l);
        toast(g, "Look %d: %s", l + 1, renderer_look_name(l));
    }
    int t = (int)renderer_tier();
    if (IsKeyPressed(KEY_LEFT_BRACKET) && t < TIER_MOBILE) { g->governor_on = false; renderer_set_tier((QualityTier)(t + 1)); toast(g, "Quality %d: %s", t + 1, renderer_tier_settings(renderer_tier())->name); }
    if (IsKeyPressed(KEY_RIGHT_BRACKET) && t > TIER_ULTRA) { g->governor_on = false; renderer_set_tier((QualityTier)(t - 1)); toast(g, "Quality %d: %s", t - 1, renderer_tier_settings(renderer_tier())->name); }
}

static void toggle_lock(Game *g) {
    if (g->lock_ent != ENTITY_NULL) { g->lock_ent = ENTITY_NULL; return; }
    Vec3 pp = active_member(g)->pc.ch.pos;
    float best = 25.0f * 25.0f; Entity bi = ENTITY_NULL;
    float fx = -sinf(g->cam.yaw), fz = -cosf(g->cam.yaw);
    for (int i = 0; i < ENEMY_MAX; ++i) {
        const Enemy *en = &g->enemies.e[i];
        if (!en->used || !combat_alive(&g->cw, en->ent)) continue;
        Vec3 tp = g->world.pos[ecs_index(en->ent)];
        float dx = tp.x - pp.x, dz = tp.z - pp.z, d2 = dx * dx + dz * dz;
        float facing = (dx * fx + dz * fz) / (sqrtf(d2) + 1e-4f); /* prefer what the camera looks at */
        if (facing < -0.2f) continue;
        if (d2 < best) { best = d2; bi = en->ent; }
    }
    g->lock_ent = bi;
}

static void do_party_controls(Game *g) {
    Input *in = &g->input;
    Party *pt = &g->party;
    int to = -1;
    if (in->swap_to >= 0 && in->swap_to < pt->n) to = in->swap_to;
    else if (in->swap_next) to = party_next_alive(pt, &g->ctx, pt->active);
    if (to >= 0 && party_switch(pt, &g->ctx, to)) {
        g->lock_ent = ENTITY_NULL;
        char b[64]; snprintf(b, sizeof b, "Controlling %s", pt->m[to].def->name);
        toast_text(g, b);
    }
    if (in->cmd_cycle) {
        pt->cmd = (PartyCommand)(((int)pt->cmd + 1) % 3);
        char b[64]; snprintf(b, sizeof b, "Party command: %s", CMD_NAMES[pt->cmd]);
        toast_text(g, b);
    }
}

static void frame(void) {
    Game *g = &G;
    float dt = GetFrameTime();
    int hidden = platform_is_hidden();
    g->loop.paused = hidden;
    g->time += (double)dt;

    if (platform_take_context_restored()) LOGI("WebGL context restored (GPU resource rebuild lands with the lifecycle milestone)");

    bool playing = g->state == STATE_PLAYING;
    input_update(&g->input, dt, playing && !hidden);
    /* desktop: the first click in game grabs the mouse (pointer lock); Esc releases it */
    if (playing && !g->input.touch_ui && !IsCursorHidden() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) DisableCursor();
    if (IsKeyPressed(KEY_ESCAPE) && IsCursorHidden()) EnableCursor();

    /* hit-stop freezes the simulation and animation for a few frames, rendering continues */
    float sim_dt = hidden ? 0.0f : dt;
    if (g->hitstop > 0) { g->hitstop -= dt; sim_dt = 0.0f; }

    int steps = 0;
    if (playing) {
        do_party_controls(g);
        if (g->input.lock) toggle_lock(g);
        if (g->lock_ent != ENTITY_NULL && !combat_alive(&g->cw, g->lock_ent)) g->lock_ent = ENTITY_NULL;
        steps = gameloop_advance(&g->loop, (double)sim_dt);
        for (int s = 0; s < steps; ++s) sim_step(g, s == 0);
        if (steps > 0) input_consume(&g->input);
        g->dbg.quest_skip_request = false; /* no quest system yet */
        for (int i = 0; i < ENEMY_MAX; ++i) { /* corpses vanish after their death animation */
            Enemy *en = &g->enemies.e[i];
            if (!en->used) continue;
            Combatant *c = combat_get(&g->cw, en->ent);
            if (c && c->dead && c->dead_t > 4.2f) enemy_remove(&g->enemies, &g->cw, &g->world, en->ent);
        }
    } else {
        g->title_t += (double)dt;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
            GetTouchPointCount() > 0) {
            new_game(g);
            g->state = STATE_PLAYING;
            g->auto_orbit = false;
            platform_report_state("playing");
        }
        /* the title backdrop keeps the world alive (idle enemies) */
        steps = gameloop_advance(&g->loop, (double)sim_dt);
        for (int s = 0; s < steps; ++s) { ecs_snapshot_transforms(&g->world); enemies_update(&g->enemies, &g->cw, &g->world, &g->phys, &g->edb, SIM_DT); combat_update(&g->cw, &g->world, &g->phys, SIM_DT); }
    }
    process_events(g);
    hud_update(&g->hud, dt);

    Combatant *acb = combat_get(&g->cw, active_ent(g));
    if (playing) {
        int alive = 0; float ehp = 0;
        for (int i = 0; i < ENEMY_MAX; ++i) { const Enemy *en = &g->enemies.e[i]; if (!en->used) continue; Combatant *c = combat_get(&g->cw, en->ent); if (c && !c->dead) { alive++; ehp += c->hp; } }
        PlayerCtl *apc = &active_member(g)->pc;
        platform_report_player(apc->ch.pos.x, apc->ch.pos.y, apc->ch.pos.z, apc->ch.grounded, apc->speed_xz);
        platform_report_combat(alive, ehp, acb ? acb->hp : 0.0f, g->kills, g->last_damage);
        float hp[4] = {0, 0, 0, 0}; int dead = 0;
        for (int i = 0; i < g->party.n && i < 4; ++i) { Combatant *c = party_cb(&g->party, &g->ctx, i); if (c) { hp[i] = c->hp; dead += c->dead; } }
        float fol = 0; /* furthest follower distance from the leader */
        for (int i = 0; i < g->party.n; ++i) if (i != g->party.active) { Vec3 a = g->party.m[i].pc.ch.pos, b = apc->ch.pos; fol = fmaxf(fol, sqrtf((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z))); }
        platform_report_party(g->party.active, (int)g->party.cmd, dead, fol, hp[0], hp[1], hp[2], hp[3]);
    }
    dbg_update(&g->dbg, dt, steps);
    g->dbg.draw_calls = 0;
    handle_render_keys(g);
    renderer_governor_update(dt, g->governor_on && !hidden);

    float alpha = gameloop_alpha(&g->loop);
    Entity act = active_ent(g);
    Vec3 pp = ecs_render_pos(&g->world, ecs_index(act), alpha);

    /* camera follows the controlled member, smoothing the hand-over when control switches */
    Vec3 want = vec3(pp.x, pp.y, pp.z);
    g->cam_focus = vec3_lerp(g->cam_focus, want, 1.0f - expf(-10.0f * dt));
    float ldx = g->input.look_dx, ldy = g->input.look_dy;
    if (g->auto_orbit) g->cam.yaw += dt * 0.12f;
    g->cam.locked = g->lock_ent != ENTITY_NULL;
    if (g->cam.locked) { Vector3 lp = lock_pos(g); g->cam.lock_pos = vec3(lp.x, 0, lp.z); }
    camrig_update(&g->cam, &g->phys, vec3(g->cam_focus.x, g->cam_focus.y + 1.3f, g->cam_focus.z), ldx, ldy, dt);
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) g->cam.dist -= wheel * 0.6f;
    Camera3D cam = {.position = {g->cam.eye.x, g->cam.eye.y, g->cam.eye.z}, .target = {g->cam.target.x, g->cam.target.y, g->cam.target.z},
                    .up = {0, 1, 0}, .fovy = 38.0f, .projection = CAMERA_PERSPECTIVE};

    float anim_dt = !hidden && g->hitstop <= 0 ? dt : 0.0f;
    for (int i = 0; i < g->party.n; ++i) {
        PartyMember *m = &g->party.m[i];
        Combatant *c = combat_get(&g->cw, m->ent);
        MemberMotion mm = {.speed = m->pc.speed_xz, .grounded = m->pc.ch.grounded, .casting = player_attacking(&m->pc), .rolling = player_rolling(&m->pc),
                           .ev_roll = g->ev_roll[i], .ev_cast = g->ev_cast[i], .cast_clip = -1, .roll_seconds = PLAYER_ROLL_TIME, .cast_seconds = PLAYER_ATTACK_TIME};
        if (g->ev_cast[i] && g->cast_skill[i] >= 0) {
            const Skill *s = &g->skills.s[g->cast_skill[i]];
            mm.cast_clip = heroes_clip(s->anim[0] ? s->anim : "Sword_Attack"); mm.cast_seconds = s->total_time;
        }
        if (c) {
            mm.dead = c->dead; mm.staggered = c->stagger_t > 0; mm.guarding = c->guarding;
            mm.flash = c->hit_flash > 0 ? fminf(1.0f, c->hit_flash / 0.12f) : 0.0f;
        }
        g->ev_roll[i] = g->ev_cast[i] = false;
        heroes_update(i, playing ? anim_dt : 0.0f, &mm);
    }

    renderer_begin_frame(cam, (float)g->time);
    test_scene_submit((float)g->time);
    for (int i = 0; i < g->party.n; ++i) {
        Vec3 mp = ecs_render_pos(&g->world, ecs_index(g->party.m[i].ent), alpha);
        heroes_submit(i, (Vector3){mp.x, mp.y, mp.z}, ecs_render_yaw(&g->world, ecs_index(g->party.m[i].ent), alpha), (float)g->time);
    }
    for (int i = 0; i < ENEMY_MAX; ++i) {
        const Enemy *en = &g->enemies.e[i];
        if (!en->used) continue;
        Combatant *c = combat_get(&g->cw, en->ent);
        if (!c) continue;
        uint32_t ei = ecs_index(en->ent);
        const EnemyDef *def = &g->edb.e[en->def];
        float spd = sqrtf(g->world.vel[ei].x * g->world.vel[ei].x + g->world.vel[ei].z * g->world.vel[ei].z);
        actor_enemy_update(&g->actors[i], c, def, &g->skills, spd, anim_dt);
        Vec3 ep = ecs_render_pos(&g->world, ei, alpha);
        actor_enemy_submit(&g->actors[i], c, def, (Vector3){ep.x, ep.y, ep.z}, ecs_render_yaw(&g->world, ei, alpha));
    }
    if (g->lock_ent != ENTITY_NULL && ecs_alive(&g->world, g->lock_ent)) {
        Vector3 t = lock_pos(g);
        Combatant *lc = combat_get(&g->cw, g->lock_ent);
        proc_submit_marker((Vector3){t.x, t.y + (lc ? lc->height : 1.8f) + 0.9f, t.z}, (float)g->time);
    }
    g->dbg.draw_calls = renderer_queue_count();

    BeginDrawing();
    renderer_end_frame();
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    if (g->state == STATE_TITLE) {
        const char *title = "SOLO ASCENT";
        int fs = sh / 8;
        DrawText(title, (sw - MeasureText(title, fs)) / 2 + 3, sh / 7 + 3, fs, (Color){0, 0, 0, 160});
        DrawText(title, (sw - MeasureText(title, fs)) / 2, sh / 7, fs, GOLD);
        const char *sub = "The S-Rank Hero";
        DrawText(sub, (sw - MeasureText(sub, fs / 2)) / 2, sh / 7 + fs, fs / 2, RAYWHITE);
        if (((int)(g->title_t * 2)) % 2 == 0) {
            const char *p = "Press ENTER / Tap to begin";
            DrawText(p, (sw - MeasureText(p, 24)) / 2, sh * 6 / 7, 24, RAYWHITE);
        }
    } else {
        hud_draw(&g->hud, cam, &g->world, &g->cw, &g->enemies, &g->edb, &g->party, g->lock_ent, g->input.touch_ui);
        if (!g->input.touch_ui)
            DrawText(IsCursorHidden() ? "WASD move | Shift sprint | Space jump | J attack | 1-4 skills | E guard | Q roll | Tab lock | R / 5-8 switch | B command | Esc mouse"
                                      : "Click to capture the mouse | WASD | J attack | 1-4 skills | E guard | Q roll | Tab lock | R / 5-8 switch character | B party command | F3 debug",
                     10, sh - 24, 16, (Color){255, 255, 255, 170});
        input_draw_touch(&g->input);
    }
    if (g->toast_t > 0) {
        g->toast_t -= (double)dt;
        DrawText(g->toast, 14, 14, 24, (Color){255, 255, 255, 230});
    }
    if (hidden) DrawText("PAUSED", sw / 2 - 40, sh / 2, 30, WHITE);
    char extra[192];
    int rw, rh;
    renderer_target_size(&rw, &rh);
    snprintf(extra, sizeof extra, "%s%s %dx%d | look: %s | %d meshes | %s%s", renderer_tier_settings(renderer_tier())->name,
             g->governor_on ? (renderer_governor_settled() ? " (auto)" : " (benchmarking)") : " (manual)", rw, rh,
             renderer_look_name(renderer_look()), renderer_queue_count(), active_member(g)->pc.ch.grounded ? "grounded" : "airborne",
             player_invulnerable(&active_member(g)->pc) ? " | I-FRAMES" : "");
    dbg_draw(&g->dbg, g->world.alive_count, g->loop.sim_ticks, (unsigned long)platform_heap_bytes(), extra);
    EndDrawing();
}

static QualityTier parse_tier(const char *s) {
    static const char *names[] = {"ultra", "high", "medium", "low", "mobile"};
    for (int i = 0; i < TIER_COUNT; ++i) if (strcmp(s, names[i]) == 0) return (QualityTier)i;
    return TIER_HIGH;
}

int main(int argc, char **argv) {
    /* args (from URL query on web): --look N  --tier name  --still  --mobile  --hero  --rack  --arena */
    int look = 2;
    QualityTier tier = TIER_HIGH;
    bool tier_forced = false, still = false, hero_only = false, rack = false;
    G.arena = false;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--look") && i + 1 < argc) look = atoi(argv[++i]) - 1 < 0 ? 0 : atoi(argv[i]) - 1;
        else if (!strcmp(argv[i], "--tier") && i + 1 < argc) { tier = parse_tier(argv[++i]); tier_forced = true; }
        else if (!strcmp(argv[i], "--still")) still = true;
        else if (!strcmp(argv[i], "--hero")) hero_only = true;
        else if (!strcmp(argv[i], "--rack")) rack = true;
        else if (!strcmp(argv[i], "--arena")) G.arena = true;
        else if (!strcmp(argv[i], "--mobile") && !tier_forced) tier = TIER_LOW;
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Solo Ascent");
    SetExitKey(KEY_NULL);
    platform_init();

    ecs_init(&G.world);
    gameloop_init(&G.loop);
    dbg_init(&G.dbg);
    input_init(&G.input);
    G.data_ok = skilldb_load(&G.skills, "data/skills.json") && enemydb_load(&G.edb, &G.skills, "data/enemies.json") &&
                partydb_load(&G.pdb, &G.skills, "data/party.json");
    if (!G.data_ok) LOGE("content data failed to load");
    test_scene_build_physics(&G.phys);
    hud_init(&G.hud);
    new_game(&G);
    G.state = STATE_TITLE;
    camrig_init(&G.cam, 0.0f, 0.16f, 6.0f);
    G.auto_orbit = !still;
    G.governor_on = !tier_forced;

    if (hero_only) { test_scene_set_showcase(true); test_scene_set_rack(rack); G.cam.dist = rack ? 7.5f : 4.4f; G.cam.cur_dist = G.cam.dist; G.cam.pitch = rack ? 0.25f : 0.04f; G.cam.yaw = rack ? 0.0f : 0.35f; }
    proc_init();
    heroes_init();
    actors_init();
    renderer_init(GetScreenWidth(), GetScreenHeight(), tier, look);
    platform_report_state("title");
    LOGI("Solo Ascent boot OK");

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) frame();
    renderer_shutdown();
    proc_shutdown();
    CloseWindow();
#endif
    return 0;
}
