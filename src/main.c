#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include "core/camrig.h"
#include "core/ecs.h"
#include "core/gameloop.h"
#include "core/log.h"
#include "core/physics.h"
#include "core/player.h"
#include "platform/platform.h"
#include "render/heroes.h"
#include "render/procmodels.h"
#include "render/renderer.h"
#include "ui/debug_overlay.h"
#include "ui/input.h"
#include "world/test_scene.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

typedef enum { STATE_TITLE, STATE_PLAYING } GameState;

#define MAX_TARGETS 8

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
    Vector3 targets[MAX_TARGETS];
    int ntargets, lock_idx;
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

static void new_game(Game *g) {
    ecs_init(&g->world);
    gameloop_init(&g->loop);
    g->player = ecs_create(&g->world, C_TRANSFORM | C_VELOCITY | C_HEALTH | C_FACTION);
    g->world.faction[ecs_index(g->player)] = FACTION_PLAYER;
    player_init(&g->pc, vec3(0, 0, 0.5f));
    uint32_t p = ecs_index(g->player);
    g->world.pos[p] = g->world.prev_pos[p] = g->pc.ch.pos;
    g->lock_idx = -1;
    g->cam.locked = false;
}

static PlayerIn make_player_input(const Game *g, bool first_step) {
    const Input *in = &g->input;
    /* camera-relative movement */
    float cs = cosf(g->cam.yaw), sn = sinf(g->cam.yaw);
    PlayerIn pin = {0};
    pin.mx = in->move_x * cs - in->move_y * sn;
    pin.mz = -in->move_x * sn - in->move_y * cs;
    pin.sprint = in->sprint; pin.walk = in->walk;
    if (first_step) { pin.jump = in->jump; pin.attack = in->attack; pin.roll = in->roll; }
    if (g->lock_idx >= 0) { pin.has_lock = true; pin.lock_x = g->targets[g->lock_idx].x; pin.lock_z = g->targets[g->lock_idx].z; }
    return pin;
}

/* Fixed 60 Hz simulation step. */
static void sim_step(Game *g, bool first_step) {
    World *w = &g->world;
    ecs_snapshot_transforms(w);
    PlayerIn pin = make_player_input(g, first_step);
    player_step(&g->pc, &g->phys, &pin, SIM_DT);
    g->ev_attack |= g->pc.ev_attack;
    g->ev_roll |= g->pc.ev_roll;
    uint32_t p = ecs_index(g->player);
    w->pos[p] = g->pc.ch.pos;
    w->yaw[p] = g->pc.yaw;
    w->vel[p] = g->pc.ch.vel;
}

static void handle_render_keys(Game *g) {
    for (int i = 0; i < LOOK_COUNT; ++i) {
        if (IsKeyPressed(KEY_ONE + i)) {
            renderer_set_look(i);
            toast(g, "Look %d: %s", i + 1, renderer_look_name(i));
        }
    }
    int t = (int)renderer_tier();
    if (IsKeyPressed(KEY_LEFT_BRACKET) && t < TIER_MOBILE) { g->governor_on = false; renderer_set_tier((QualityTier)(t + 1)); toast(g, "Quality %d: %s", t + 1, renderer_tier_settings(renderer_tier())->name); }
    if (IsKeyPressed(KEY_RIGHT_BRACKET) && t > TIER_ULTRA) { g->governor_on = false; renderer_set_tier((QualityTier)(t - 1)); toast(g, "Quality %d: %s", t - 1, renderer_tier_settings(renderer_tier())->name); }
}

static void toggle_lock(Game *g) {
    if (g->lock_idx >= 0) { g->lock_idx = -1; return; }
    Vector3 pp = {g->pc.ch.pos.x, 0, g->pc.ch.pos.z};
    float best = 25.0f * 25.0f; int bi = -1;
    for (int i = 0; i < g->ntargets; ++i) {
        float dx = g->targets[i].x - pp.x, dz = g->targets[i].z - pp.z, d2 = dx * dx + dz * dz;
        /* prefer targets in front of the camera */
        float fx = -sinf(g->cam.yaw), fz = -cosf(g->cam.yaw);
        float facing = (dx * fx + dz * fz) / (sqrtf(d2) + 1e-4f);
        if (facing < -0.2f) continue;
        if (d2 < best) { best = d2; bi = i; }
    }
    g->lock_idx = bi;
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

    int steps = 0;
    if (playing) {
        if (g->input.lock) toggle_lock(g);
        steps = gameloop_advance(&g->loop, (double)dt);
        for (int s = 0; s < steps; ++s) sim_step(g, s == 0);
        if (steps > 0) input_consume(&g->input);
        g->dbg.quest_skip_request = false; /* no quest system yet */
    } else {
        g->title_t += (double)dt;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
            GetTouchPointCount() > 0) {
            new_game(g);
            g->state = STATE_PLAYING;
            g->auto_orbit = false;
            platform_report_state("playing");
        }
    }

    if (playing) platform_report_player(g->pc.ch.pos.x, g->pc.ch.pos.y, g->pc.ch.pos.z, g->pc.ch.grounded, g->pc.speed_xz);
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
    g->cam.locked = g->lock_idx >= 0;
    if (g->cam.locked) { g->cam.lock_pos = vec3(g->targets[g->lock_idx].x, 0, g->targets[g->lock_idx].z); }
    camrig_update(&g->cam, &g->phys, vec3(pp.x, pp.y + 1.3f, pp.z), ldx, ldy, dt);
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) g->cam.dist -= wheel * 0.6f;
    Camera3D cam = {.position = {g->cam.eye.x, g->cam.eye.y, g->cam.eye.z}, .target = {g->cam.target.x, g->cam.target.y, g->cam.target.z},
                    .up = {0, 1, 0}, .fovy = 38.0f, .projection = CAMERA_PERSPECTIVE};

    {
        KaelMotion km = {.speed = g->pc.speed_xz, .grounded = g->pc.ch.grounded, .attacking = player_attacking(&g->pc),
                         .rolling = player_rolling(&g->pc), .ev_attack = g->ev_attack, .ev_roll = g->ev_roll,
                         .attack_seconds = PLAYER_ATTACK_TIME, .roll_seconds = PLAYER_ROLL_TIME};
        g->ev_attack = g->ev_roll = false;
        heroes_update_kael(playing && !hidden ? dt : 0.0f, &km);
    }
    renderer_begin_frame(cam, (float)g->time);
    test_scene_submit((float)g->time, (Vector3){pp.x, pp.y, pp.z}, yaw);
    if (g->lock_idx >= 0) {
        Vector3 t = g->targets[g->lock_idx];
        proc_submit_marker((Vector3){t.x, t.y + 2.3f, t.z}, (float)g->time);
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
        if (!g->input.touch_ui)
            DrawText(IsCursorHidden() ? "WASD move | Shift sprint | Ctrl walk | Space jump | LMB/J attack | RMB/Q roll | Tab lock | Esc free mouse"
                                      : "Click to capture the mouse | WASD move | Shift sprint | Space jump | J attack | Q roll | Tab lock | F3 debug",
                     10, sh - 26, 18, (Color){255, 255, 255, 170});
        input_draw_touch(&g->input);
    }
    if (g->toast_t > 0) {
        g->toast_t -= (double)dt;
        DrawText(g->toast, 14, 14, 24, (Color){255, 255, 255, 230});
    }
    if (hidden) DrawText("PAUSED", sw / 2 - 40, sh / 2, 30, WHITE);
    char extra[160];
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
    /* args (from URL query on web): --look N  --tier name  --still  --mobile  --hero  --rack */
    int look = 2;
    QualityTier tier = TIER_HIGH;
    bool tier_forced = false, still = false, hero_only = false, rack = false;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--look") && i + 1 < argc) look = atoi(argv[++i]) - 1 < 0 ? 0 : atoi(argv[i]) - 1;
        else if (!strcmp(argv[i], "--tier") && i + 1 < argc) { tier = parse_tier(argv[++i]); tier_forced = true; }
        else if (!strcmp(argv[i], "--still")) still = true;
        else if (!strcmp(argv[i], "--hero")) hero_only = true;
        else if (!strcmp(argv[i], "--rack")) rack = true;
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
    test_scene_build_physics(&G.phys);
    G.ntargets = test_scene_targets(G.targets, MAX_TARGETS);
    new_game(&G);
    G.state = STATE_TITLE;
    camrig_init(&G.cam, -0.45f, 0.14f, 6.0f);
    G.auto_orbit = !still;
    G.governor_on = !tier_forced;

    if (hero_only) { test_scene_set_showcase(true); test_scene_set_rack(rack); G.cam.dist = rack ? 7.5f : 4.4f; G.cam.cur_dist = G.cam.dist; G.cam.pitch = rack ? 0.25f : 0.04f; G.cam.yaw = rack ? 0.0f : 0.35f; }
    proc_init();
    heroes_init();
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
