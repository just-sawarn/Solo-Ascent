#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include "core/ecs.h"
#include "core/gameloop.h"
#include "core/log.h"
#include "platform/platform.h"
#include "render/procmodels.h"
#include "render/renderer.h"
#include "ui/debug_overlay.h"
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
    /* orbit camera */
    float cam_yaw, cam_pitch, cam_dist;
    bool auto_orbit;
    bool governor_on;
    double title_t, time;
    double toast_t; char toast[96];
} Game;

static Game G; /* static: World is large, no per-frame allocation anywhere */

static void toast(Game *g, const char *fmt, int a, const char *b) {
    snprintf(g->toast, sizeof g->toast, fmt, a, b);
    g->toast_t = 2.5;
}

static void new_game(Game *g) {
    ecs_init(&g->world);
    gameloop_init(&g->loop);
    g->player = ecs_create(&g->world, C_TRANSFORM | C_VELOCITY | C_HEALTH | C_FACTION);
    g->world.faction[ecs_index(g->player)] = FACTION_PLAYER;
    g->world.pos[ecs_index(g->player)] = vec3(0, 0, 0.5f);
    g->world.prev_pos[ecs_index(g->player)] = g->world.pos[ecs_index(g->player)];
}

/* Fixed 60 Hz simulation step. */
static void sim_step(Game *g) {
    World *w = &g->world;
    ecs_snapshot_transforms(w);
    float ix = (float)(IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) - (float)(IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT));
    float iz = (float)(IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) - (float)(IsKeyDown(KEY_W) || IsKeyDown(KEY_UP));
    uint32_t p = ecs_index(g->player);
    float len = sqrtf(ix * ix + iz * iz);
    if (len > 0) {
        ix /= len; iz /= len;
        /* camera-relative movement */
        float cs = cosf(g->cam_yaw), sn = sinf(g->cam_yaw);
        float wx = ix * cs + iz * sn, wz = -ix * sn + iz * cs;
        w->pos[p].x += wx * 4.5f * SIM_DT;
        w->pos[p].z += wz * 4.5f * SIM_DT;
        w->yaw[p] = atan2f(wx, wz);
    }
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

static void update_camera(Game *g, float dt, Camera3D *cam, Vector3 focus) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && g->state == STATE_PLAYING) {
        Vector2 d = GetMouseDelta();
        g->cam_yaw -= d.x * 0.006f;
        g->cam_pitch += d.y * 0.004f;
        g->auto_orbit = false;
    }
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) g->cam_dist -= wheel * 0.6f;
    if (g->auto_orbit) g->cam_yaw += dt * 0.12f;
    if (g->cam_pitch < 0.02f) g->cam_pitch = 0.02f;
    if (g->cam_pitch > 1.3f) g->cam_pitch = 1.3f;
    if (g->cam_dist < 3.0f) g->cam_dist = 3.0f;
    if (g->cam_dist > 22.0f) g->cam_dist = 22.0f;
    cam->target = (Vector3){focus.x, focus.y + 1.15f, focus.z};
    cam->position = (Vector3){
        cam->target.x + sinf(g->cam_yaw) * cosf(g->cam_pitch) * g->cam_dist,
        cam->target.y + sinf(g->cam_pitch) * g->cam_dist,
        cam->target.z + cosf(g->cam_yaw) * cosf(g->cam_pitch) * g->cam_dist};
}

static void frame(void) {
    Game *g = &G;
    float dt = GetFrameTime();
    int hidden = platform_is_hidden();
    g->loop.paused = hidden;
    g->time += (double)dt;

    if (platform_take_context_restored()) LOGI("WebGL context restored (GPU resource rebuild lands with the lifecycle milestone)");

    int steps = 0;
    if (g->state == STATE_PLAYING) {
        steps = gameloop_advance(&g->loop, (double)dt);
        for (int s = 0; s < steps; ++s) sim_step(g);
        g->dbg.quest_skip_request = false; /* no quest system yet */
    } else {
        g->title_t += (double)dt;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
            GetTouchPointCount() > 0) {
            new_game(g);
            g->state = STATE_PLAYING;
            platform_report_state("playing");
        }
    }

    dbg_update(&g->dbg, dt, steps);
    g->dbg.draw_calls = 0;
    handle_render_keys(g);
    renderer_governor_update(dt, g->governor_on && !hidden);

    float alpha = gameloop_alpha(&g->loop);
    Vec3 pp = ecs_render_pos(&g->world, ecs_index(g->player), alpha);
    float yaw = ecs_render_yaw(&g->world, ecs_index(g->player), alpha);
    Camera3D cam = {.up = {0, 1, 0}, .fovy = 38.0f, .projection = CAMERA_PERSPECTIVE};
    update_camera(g, dt, &cam, (Vector3){pp.x, pp.y, pp.z});

    renderer_begin_frame(cam, (float)g->time);
    test_scene_submit((float)g->time, (Vector3){pp.x, pp.y, pp.z}, yaw);
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
        DrawText("WASD move  |  drag: orbit  |  wheel: zoom  |  1-3: look  |  [ ]: quality  |  F3: debug", 10, sh - 26, 18, (Color){255, 255, 255, 170});
    }
    if (g->toast_t > 0) {
        g->toast_t -= (double)dt;
        DrawText(g->toast, 14, 14, 24, (Color){255, 255, 255, 230});
    }
    if (hidden) DrawText("PAUSED", sw / 2 - 40, sh / 2, 30, WHITE);
    char extra[128];
    int rw, rh;
    renderer_target_size(&rw, &rh);
    snprintf(extra, sizeof extra, "%s%s %dx%d | look: %s | %d meshes", renderer_tier_settings(renderer_tier())->name,
             g->governor_on ? (renderer_governor_settled() ? " (auto)" : " (benchmarking)") : " (manual)", rw, rh,
             renderer_look_name(renderer_look()), renderer_queue_count());
    dbg_draw(&g->dbg, g->world.alive_count, g->loop.sim_ticks, (unsigned long)platform_heap_bytes(), extra);
    EndDrawing();
}

static QualityTier parse_tier(const char *s) {
    static const char *names[] = {"ultra", "high", "medium", "low", "mobile"};
    for (int i = 0; i < TIER_COUNT; ++i) if (strcmp(s, names[i]) == 0) return (QualityTier)i;
    return TIER_HIGH;
}

int main(int argc, char **argv) {
    /* args (from URL query on web): --look N  --tier name  --still  --mobile */
    int look = 2;
    QualityTier tier = TIER_HIGH;
    bool tier_forced = false, still = false;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--look") && i + 1 < argc) look = atoi(argv[++i]) - 1 < 0 ? 0 : atoi(argv[i]) - 1;
        else if (!strcmp(argv[i], "--tier") && i + 1 < argc) { tier = parse_tier(argv[++i]); tier_forced = true; }
        else if (!strcmp(argv[i], "--still")) still = true;
        else if (!strcmp(argv[i], "--mobile") && !tier_forced) tier = TIER_LOW;
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Solo Ascent");
    SetExitKey(KEY_NULL);
    platform_init();

    ecs_init(&G.world);
    gameloop_init(&G.loop);
    dbg_init(&G.dbg);
    new_game(&G);
    G.state = STATE_TITLE;
    G.cam_yaw = -0.45f; G.cam_pitch = 0.14f; G.cam_dist = 6.0f;
    G.auto_orbit = !still;
    G.governor_on = !tier_forced;

    proc_init();
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
