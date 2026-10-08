#include <math.h>
#include <raylib.h>
#include "core/ecs.h"
#include "core/gameloop.h"
#include "core/log.h"
#include "platform/platform.h"
#include "ui/debug_overlay.h"

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
    Camera3D cam;
    double title_t;
} Game;

static Game G; /* static: World is large, no per-frame allocation anywhere */

#define ORBITERS 400

static void new_game(Game *g) {
    ecs_init(&g->world);
    gameloop_init(&g->loop);
    g->player = ecs_create(&g->world, C_TRANSFORM | C_VELOCITY | C_RENDER | C_HEALTH | C_FACTION);
    g->world.color[ecs_index(g->player)] = 0xFFC832FFu; /* gold: the Ascendant glow */
    g->world.shape[ecs_index(g->player)] = SHAPE_CUBE;
    g->world.faction[ecs_index(g->player)] = FACTION_PLAYER;
    g->world.pos[ecs_index(g->player)] = vec3(0, 0.5f, 0);
    for (int i = 0; i < ORBITERS; ++i) {
        Entity e = ecs_create(&g->world, C_TRANSFORM | C_VELOCITY | C_RENDER);
        uint32_t k = ecs_index(e);
        float a = (float)i / ORBITERS * 6.2831853f, r = 6.0f + (float)(i % 10) * 1.5f;
        g->world.pos[k] = vec3(cosf(a) * r, 0.3f, sinf(a) * r);
        g->world.scale[k] = 0.4f;
        g->world.shape[k] = (uint8_t)(i % 3);
        g->world.color[k] = 0x3C78C8FFu;
        g->world.vel[k].x = 0.15f + (float)(i % 10) * 0.02f; /* angular speed (rad/s), reused field */
    }
    g->state = STATE_PLAYING;
    platform_report_state("playing");
}

/* Fixed 60 Hz simulation step. */
static void sim_step(Game *g) {
    World *w = &g->world;
    ecs_snapshot_transforms(w);
    /* player input */
    float ix = (float)(IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) - (float)(IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT));
    float iz = (float)(IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) - (float)(IsKeyDown(KEY_W) || IsKeyDown(KEY_UP));
    uint32_t p = ecs_index(g->player);
    float len = sqrtf(ix * ix + iz * iz);
    if (len > 0) {
        ix /= len; iz /= len;
        w->pos[p].x += ix * 6.0f * SIM_DT;
        w->pos[p].z += iz * 6.0f * SIM_DT;
        w->yaw[p] = atan2f(ix, iz);
    }
    /* orbiters rotate around the origin */
    ECS_EACH(w, i, C_TRANSFORM | C_VELOCITY | C_RENDER) {
        if (i == p) continue;
        float s = sinf(w->vel[i].x * SIM_DT), c = cosf(w->vel[i].x * SIM_DT);
        float x = w->pos[i].x, z = w->pos[i].z;
        w->pos[i].x = x * c - z * s;
        w->pos[i].z = x * s + z * c;
        w->yaw[i] += w->vel[i].x * SIM_DT;
    }
}

static Color unpack(uint32_t c) {
    return (Color){(unsigned char)(c >> 24), (unsigned char)(c >> 16), (unsigned char)(c >> 8), (unsigned char)c};
}

static void draw_world(Game *g, float alpha) {
    World *w = &g->world;
    BeginMode3D(g->cam);
    DrawPlane((Vector3){0, 0, 0}, (Vector2){60, 60}, (Color){40, 52, 44, 255});
    DrawGrid(40, 1.5f);
    ECS_EACH(w, i, C_TRANSFORM | C_RENDER) {
        Vec3 p = ecs_render_pos(w, i, alpha);
        Color col = unpack(w->color[i]);
        float s = w->scale[i];
        Vector3 vp = {p.x, p.y, p.z};
        if (w->shape[i] == SHAPE_SPHERE) DrawSphereEx(vp, s * 0.6f, 6, 6, col);
        else if (w->shape[i] == SHAPE_CYLINDER) DrawCylinder(vp, s * 0.4f, s * 0.4f, s, 8, col);
        else DrawCube(vp, s, s, s, col);
        dbg_count_draw(&g->dbg);
    }
    EndMode3D();
}

static void frame(void) {
    Game *g = &G;
    float dt = GetFrameTime();
    int hidden = platform_is_hidden();
    g->loop.paused = hidden;

    if (platform_take_context_restored()) LOGI("WebGL context restored (GPU resource rebuild lands with the renderer milestone)");

    int steps = 0;
    if (g->state == STATE_PLAYING) {
        steps = gameloop_advance(&g->loop, (double)dt);
        for (int s = 0; s < steps; ++s) sim_step(g);
        g->dbg.quest_skip_request = false; /* no quest system yet */
    } else {
        g->title_t += (double)dt;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
            GetTouchPointCount() > 0)
            new_game(g);
    }

    dbg_update(&g->dbg, dt, steps);
    g->dbg.draw_calls = 0;

    /* camera follows the player's interpolated position */
    float alpha = gameloop_alpha(&g->loop);
    Vec3 pp = vec3(0, 0, 0);
    if (g->state == STATE_PLAYING) pp = ecs_render_pos(&g->world, ecs_index(g->player), alpha);
    g->cam.target = (Vector3){pp.x, 0.5f, pp.z};
    double t = g->state == STATE_TITLE ? g->title_t * 0.2 : 0.0;
    g->cam.position = (Vector3){pp.x + (float)sin(t) * 16.0f, 12.0f, pp.z + (float)cos(t) * 16.0f};

    BeginDrawing();
    ClearBackground((Color){20, 24, 36, 255});
    draw_world(g, alpha);
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    if (g->state == STATE_TITLE) {
        const char *title = "SOLO ASCENT";
        int fs = sh / 8;
        DrawText(title, (sw - MeasureText(title, fs)) / 2, sh / 4, fs, GOLD);
        const char *sub = "The S-Rank Hero";
        DrawText(sub, (sw - MeasureText(sub, fs / 2)) / 2, sh / 4 + fs, fs / 2, RAYWHITE);
        if (((int)(g->title_t * 2)) % 2 == 0) {
            const char *p = "Press ENTER / Tap to begin";
            DrawText(p, (sw - MeasureText(p, 24)) / 2, sh * 3 / 4, 24, RAYWHITE);
        }
    } else {
        DrawText("WASD / arrows: move   F3 or 3-finger tap: debug", 10, sh - 28, 18, (Color){255, 255, 255, 160});
    }
    if (hidden) DrawText("PAUSED", sw / 2 - 40, sh / 2, 30, WHITE);
    dbg_draw(&g->dbg, g->world.alive_count, g->loop.sim_ticks, (unsigned long)platform_heap_bytes());
    EndDrawing();
}

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Solo Ascent");
    SetExitKey(KEY_NULL);
    platform_init();

    ecs_init(&G.world);
    gameloop_init(&G.loop);
    dbg_init(&G.dbg);
    G.state = STATE_TITLE;
    G.cam = (Camera3D){.position = {0, 12, 16}, .target = {0, 0.5f, 0}, .up = {0, 1, 0}, .fovy = 50, .projection = CAMERA_PERSPECTIVE};
    // Title backdrop: spawn the orbiters without starting the game.
    new_game(&G);
    G.state = STATE_TITLE;
    platform_report_state("title");
    LOGI("Solo Ascent boot OK");

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) frame();
    CloseWindow();
#endif
    return 0;
}
