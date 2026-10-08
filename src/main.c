#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include "core/camrig.h"
#include "core/combat.h"
#include "core/data.h"
#include "core/enemy_ai.h"
#include "core/ecs.h"
#include "core/gameloop.h"
#include "core/log.h"
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
    Entity player;
    PlayerCtl pc;
    PhysWorld phys;
    CamRig cam;
    Input input;
    SkillDB skills;
    EnemyDB edb;
    EnemyPool enemies;
    CombatWorld cw;
    ActorAnim actors[ENEMY_MAX];
    Hud hud;
    int hero_skill[4], basic[3];
    int combo_idx; float combo_timer; bool attack_queued;
    int awakening;                 /* sandbox: everything unlocked */
    Entity lock_ent;
    float hitstop, respawn_t;
    int kills; float last_damage;
    bool arena, data_ok;
    bool auto_orbit, governor_on;
    double title_t, time;
    double toast_t; char toast[96];
    /* events accumulated over the sim steps of one frame */
    bool ev_attack, ev_roll;
} Game;

static Game G; /* static: World and PhysWorld are large, no per-frame allocation anywhere */

static void toast(Game *g, const char *fmt, int a, const char *b) {
    snprintf(g->toast, sizeof g->toast, fmt, a, b);
    g->toast_t = 2.5;
}

static const struct { const char *id; float x, z; } SPAWNS[] = {
    {"shadow_soldier", -9.0f, -13.0f}, {"shadow_soldier", 9.0f, -13.0f}, {"shadow_archer", 0.0f, -19.0f},
    {"shadow_brute", 15.0f, -6.0f}, {"shadow_soldier", -15.0f, 9.0f},
};

static void spawn_enemy(Game *g, const char *id, float x, float z) {
    int def = enemydb_find(&g->edb, id);
    if (def >= 0) enemy_spawn(&g->enemies, &g->cw, &g->world, &g->edb, def, vec3(x, 0, z));
}

static void new_game(Game *g) {
    ecs_init(&g->world);
    gameloop_init(&g->loop);
    combat_init(&g->cw, &g->skills);
    enemies_init(&g->enemies);
    hud_init(&g->hud);
    memset(g->actors, 0, sizeof g->actors);
    g->player = ecs_create(&g->world, C_TRANSFORM | C_VELOCITY | C_HEALTH | C_FACTION);
    g->world.faction[ecs_index(g->player)] = FACTION_PLAYER;
    player_init(&g->pc, vec3(0, 0, 0.5f));
    g->pc.yaw = 3.14159265f; /* face -z, toward the arena; the camera sits behind him */
    uint32_t p = ecs_index(g->player);
    g->world.yaw[p] = g->world.prev_yaw[p] = g->pc.yaw;
    g->world.pos[p] = g->world.prev_pos[p] = g->pc.ch.pos;
    Combatant *pcb = combat_add(&g->cw, &g->world, g->player, TEAM_PLAYER, 5, (Stats){14, 8, 12, 10, 10}, 120);
    if (pcb) { pcb->has_resolve = true; pcb->weapon_atk = 6.0f; combatant_recalc(pcb); pcb->hp = pcb->max_hp; pcb->mp = pcb->max_mp; }
    if (g->arena) spawn_enemy(g, "shadow_soldier", 0.0f, -3.6f);
    else for (unsigned i = 0; i < sizeof SPAWNS / sizeof SPAWNS[0]; ++i) spawn_enemy(g, SPAWNS[i].id, SPAWNS[i].x, SPAWNS[i].z);
    g->lock_ent = ENTITY_NULL;
    g->cam.locked = false;
    g->combo_idx = 0; g->combo_timer = 0; g->attack_queued = false;
    g->kills = 0; g->respawn_t = 0; g->hitstop = 0;
}

static Vector3 lock_pos(const Game *g) {
    if (g->lock_ent == ENTITY_NULL || !ecs_alive(&g->world, g->lock_ent)) return (Vector3){0, 0, 0};
    Vec3 p = g->world.pos[ecs_index(g->lock_ent)];
    return (Vector3){p.x, p.y, p.z};
}

static PlayerIn make_player_input(const Game *g) {
    const Input *in = &g->input;
    /* camera-relative movement */
    float cs = cosf(g->cam.yaw), sn = sinf(g->cam.yaw);
    PlayerIn pin = {0};
    pin.mx = in->move_x * cs - in->move_y * sn;
    pin.mz = -in->move_x * sn - in->move_y * cs;
    pin.sprint = in->sprint; pin.walk = in->walk;
    if (g->lock_ent != ENTITY_NULL && combat_alive(&g->cw, g->lock_ent)) { Vector3 lp = lock_pos(g); pin.has_lock = true; pin.lock_x = lp.x; pin.lock_z = lp.z; }
    return pin;
}

static float aim_yaw_for(const Game *g, const PlayerIn *pin) {
    if (pin->has_lock) return atan2f(pin->lock_x - g->pc.ch.pos.x, pin->lock_z - g->pc.ch.pos.z);
    if (pin->mx * pin->mx + pin->mz * pin->mz > 1e-4f) return atan2f(pin->mx, pin->mz);
    return g->pc.yaw;
}

static bool try_cast(Game *g, PlayerIn *pin, int skill) {
    if (skill < 0) return false;
    const Skill *s = &g->skills.s[skill];
    Combatant *pcb = combat_get(&g->cw, g->player);
    if (!pcb || pcb->dead || pcb->stagger_t > 0 || s->awakening > g->awakening) return false;
    if (!g->pc.ch.grounded || player_rolling(&g->pc) || player_attacking(&g->pc)) return false;
    float aim = aim_yaw_for(g, pin);
    if (!combat_begin_cast(&g->cw, &g->world, g->player, skill, g->lock_ent, aim)) return false;
    pin->attack = true; pin->attack_time = s->total_time; pin->has_aim = true; pin->aim_yaw = aim;
    return true;
}

static void try_basic(Game *g, PlayerIn *pin) {
    if (try_cast(g, pin, g->basic[g->combo_idx])) { g->combo_idx = (g->combo_idx + 1) % 3; g->combo_timer = 0.9f; g->attack_queued = false; }
}

static void respawn(Game *g) {
    Combatant *pcb = combat_get(&g->cw, g->player);
    if (!pcb) return;
    pcb->dead = false; pcb->dead_t = 0; pcb->hp = pcb->max_hp; pcb->mp = pcb->max_mp; pcb->resolve_used = false;
    memset(pcb->status, 0, sizeof pcb->status); pcb->barrier = 0; pcb->cast_skill = -1; pcb->stagger_t = 0;
    player_init(&g->pc, vec3(0, 0, 0.5f));
    g->pc.yaw = 3.14159265f;
    g->lock_ent = ENTITY_NULL; g->respawn_t = 0;
}

/* Fixed 60 Hz simulation step. */
static void sim_step(Game *g, bool first_step) {
    World *w = &g->world;
    ecs_snapshot_transforms(w);
    Combatant *pcb = combat_get(&g->cw, g->player);
    bool alive = pcb && !pcb->dead;
    PlayerIn pin = make_player_input(g);
    if (!alive) { pin.mx = pin.mz = 0; pin.sprint = false; }

    if (alive) {
        pcb->invuln = player_invulnerable(&g->pc);
        bool can_guard = g->pc.ch.grounded && !player_attacking(&g->pc) && !player_rolling(&g->pc) && pcb->stagger_t <= 0;
        combat_set_guard(&g->cw, g->player, g->input.guard && can_guard);
        if (pcb->guarding) pin.walk = true;
        if (g->combo_timer > 0 && !player_attacking(&g->pc)) { g->combo_timer -= SIM_DT; if (g->combo_timer <= 0) g->combo_idx = 0; }
        if (first_step && !pcb->guarding) {
            bool cast = false;
            for (int i = 0; i < 4 && !cast; ++i) if (g->input.skill[i]) cast = try_cast(g, &pin, g->hero_skill[i]);
            if (!cast && g->input.attack) { if (player_attacking(&g->pc)) g->attack_queued = true; else try_basic(g, &pin); }
            if (g->input.roll) pin.roll = true;
            if (g->input.jump) pin.jump = true;
        }
        if (g->attack_queued && !player_attacking(&g->pc) && !pin.attack) try_basic(g, &pin);
        if (pcb->guarding) { pin.roll = false; pin.jump = false; }
    }
    player_step(&g->pc, &g->phys, &pin, SIM_DT);
    g->ev_attack |= g->pc.ev_attack;
    g->ev_roll |= g->pc.ev_roll;
    uint32_t p = ecs_index(g->player);
    w->pos[p] = g->pc.ch.pos;
    w->yaw[p] = g->pc.yaw;
    w->vel[p] = g->pc.ch.vel;

    enemies_update(&g->enemies, &g->cw, w, &g->phys, &g->edb, SIM_DT);
    combat_update(&g->cw, w, &g->phys, SIM_DT);
    if (pcb && pcb->dash_pending) { g->pc.ch.pos.x = pcb->dash_to.x; g->pc.ch.pos.z = pcb->dash_to.z; g->pc.ch.vel = vec3(0, 0, 0); pcb->dash_pending = false; w->pos[p] = g->pc.ch.pos; }
    if (g->dbg.god_mode && pcb && !pcb->dead) pcb->hp = pcb->max_hp;
    if (pcb && pcb->dead) { g->respawn_t += SIM_DT; if (g->respawn_t > 3.5f) respawn(g); }
}

/* Presentation reactions to this frame's combat events. */
static void process_events(Game *g) {
    for (int i = 0; i < g->cw.nev; ++i) {
        const CombatEvent *e = &g->cw.ev[i];
        bool by_player = e->src == g->player, on_player = e->dst == g->player;
        switch (e->type) {
        case CE_HIT: {
            if (by_player) { g->hitstop = fmaxf(g->hitstop, e->crit ? 0.085f : 0.05f); camrig_shake(&g->cam, e->crit ? 0.05f : 0.02f); g->last_damage = e->amount; }
            if (on_player) { g->hitstop = fmaxf(g->hitstop, 0.045f); camrig_shake(&g->cam, 0.06f); }
        } break;
        case CE_PARRY: g->hitstop = fmaxf(g->hitstop, 0.14f); camrig_shake(&g->cam, 0.09f); break;
        case CE_BLOCK: camrig_shake(&g->cam, 0.03f); break;
        case CE_DEATH:
            if (e->dst != g->player) { g->kills++; g->hitstop = fmaxf(g->hitstop, 0.10f); camrig_shake(&g->cam, 0.08f); }
            else camrig_shake(&g->cam, 0.15f);
            break;
        case CE_CAST:
            if (e->amount > 0.5f && by_player) camrig_shake(&g->cam, 0.05f); /* ground impact */
            break;
        default: break;
        }
    }
    hud_on_events(&g->hud, g->cw.ev, g->cw.nev, g->player);
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
    Vec3 pp = g->pc.ch.pos;
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
        if (g->input.lock) { toggle_lock(g); }
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

    Combatant *pcb = combat_get(&g->cw, g->player);
    if (playing) {
        int alive = 0; float ehp = 0;
        for (int i = 0; i < ENEMY_MAX; ++i) { const Enemy *en = &g->enemies.e[i]; if (!en->used) continue; Combatant *c = combat_get(&g->cw, en->ent); if (c && !c->dead) { alive++; ehp += c->hp; } }
        platform_report_player(g->pc.ch.pos.x, g->pc.ch.pos.y, g->pc.ch.pos.z, g->pc.ch.grounded, g->pc.speed_xz);
        platform_report_combat(alive, ehp, pcb ? pcb->hp : 0.0f, g->kills, g->last_damage);
    }
    dbg_update(&g->dbg, dt, steps);
    g->dbg.draw_calls = 0;
    handle_render_keys(g);
    renderer_governor_update(dt, g->governor_on && !hidden);

    float alpha = gameloop_alpha(&g->loop);
    Vec3 pp = ecs_render_pos(&g->world, ecs_index(g->player), alpha);
    float yaw = ecs_render_yaw(&g->world, ecs_index(g->player), alpha);

    /* camera */
    float ldx = g->input.look_dx, ldy = g->input.look_dy;
    if (g->auto_orbit) g->cam.yaw += dt * 0.12f;
    g->cam.locked = g->lock_ent != ENTITY_NULL;
    if (g->cam.locked) { Vector3 lp = lock_pos(g); g->cam.lock_pos = vec3(lp.x, 0, lp.z); }
    camrig_update(&g->cam, &g->phys, vec3(pp.x, pp.y + 1.3f, pp.z), ldx, ldy, dt);
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) g->cam.dist -= wheel * 0.6f;
    Camera3D cam = {.position = {g->cam.eye.x, g->cam.eye.y, g->cam.eye.z}, .target = {g->cam.target.x, g->cam.target.y, g->cam.target.z},
                    .up = {0, 1, 0}, .fovy = 38.0f, .projection = CAMERA_PERSPECTIVE};

    float anim_dt = !hidden && g->hitstop <= 0 ? dt : 0.0f;
    {
        KaelMotion km = {.speed = g->pc.speed_xz, .grounded = g->pc.ch.grounded, .attacking = player_attacking(&g->pc),
                         .rolling = player_rolling(&g->pc), .ev_attack = g->ev_attack, .ev_roll = g->ev_roll,
                         .attack_seconds = PLAYER_ATTACK_TIME, .roll_seconds = PLAYER_ROLL_TIME};
        if (pcb) {
            km.dead = pcb->dead; km.staggered = pcb->stagger_t > 0; km.guarding = pcb->guarding;
            km.flash = pcb->hit_flash > 0 ? fminf(1.0f, pcb->hit_flash / 0.12f) : 0.0f;
            if (pcb->cast_skill >= 0) { km.attack_seconds = g->skills.s[pcb->cast_skill].total_time; }
        }
        g->ev_attack = g->ev_roll = false;
        heroes_update_kael(playing ? anim_dt : 0.0f, &km);
    }
    renderer_begin_frame(cam, (float)g->time);
    test_scene_submit((float)g->time, (Vector3){pp.x, pp.y, pp.z}, yaw);
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
        hud_draw(&g->hud, cam, &g->world, &g->cw, &g->enemies, &g->edb, g->player, g->lock_ent, g->hero_skill, g->input.touch_ui);
        if (!g->input.touch_ui)
            DrawText(IsCursorHidden() ? "WASD move | Shift sprint | Space jump | J/LMB attack | 1-4 skills | E guard (parry on press) | Q/RMB roll | Tab lock | Esc mouse"
                                      : "Click to capture the mouse | WASD | Shift | Space | J attack | 1-4 skills | E guard | Q roll | Tab lock | L look | F3 debug",
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
             renderer_look_name(renderer_look()), renderer_queue_count(), g->pc.ch.grounded ? "grounded" : "airborne",
             player_invulnerable(&g->pc) ? " | I-FRAMES" : "");
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
    G.data_ok = skilldb_load(&G.skills, "data/skills.json") && enemydb_load(&G.edb, &G.skills, "data/enemies.json");
    if (!G.data_ok) LOGE("content data failed to load");
    G.awakening = 5;
    const char *hs[4] = {"heroic_slash", "sunder", "gale_rush", "dawn_arc"}, *bs[3] = {"hero_light_1", "hero_light_2", "hero_light_3"};
    for (int i = 0; i < 4; ++i) G.hero_skill[i] = skilldb_find(&G.skills, hs[i]);
    for (int i = 0; i < 3; ++i) G.basic[i] = skilldb_find(&G.skills, bs[i]);
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
