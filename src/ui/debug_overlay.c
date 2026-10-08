#include "ui/debug_overlay.h"
#include <raylib.h>

void dbg_init(DebugState *d) { *d = (DebugState){0}; }
void dbg_count_draw(DebugState *d) { d->draw_calls++; }

void dbg_update(DebugState *d, float dt, int steps) {
    /* Toggle: F3, or a three-finger tap (touch count rising to >= 3). */
    static int prev_touches;
    int touches = GetTouchPointCount();
    if (IsKeyPressed(KEY_F3) || (touches >= 3 && prev_touches < 3)) d->visible = !d->visible;
    prev_touches = touches;
    if (d->visible) {
        if (IsKeyPressed(KEY_G)) d->god_mode = !d->god_mode;
        if (IsKeyPressed(KEY_K)) d->quest_skip_request = true;
    }
    d->sim_steps_last = steps;
    d->frame_ms[d->frame_head] = dt * 1000.0f;
    d->frame_head = (d->frame_head + 1) % 120;
    float sum = 0, worst = 0;
    for (int i = 0; i < 120; ++i) {
        sum += d->frame_ms[i];
        if (d->frame_ms[i] > worst) worst = d->frame_ms[i];
    }
    d->avg_ms = sum / 120.0f;
    d->worst_ms = worst;
}

void dbg_draw(const DebugState *d, int alive, unsigned long long ticks, unsigned long heap) {
    if (!d->visible) return;
    const int x = 10, w = 330, h = 190;
    DrawRectangle(x - 5, 5, w, h, (Color){0, 0, 0, 170});
    int y = 10;
    DrawText(TextFormat("FPS %d  (avg %.2f ms, worst %.1f ms)", GetFPS(), d->avg_ms, d->worst_ms), x, y, 16, LIME); y += 20;
    DrawText(TextFormat("draw calls (submitted): %d", d->draw_calls), x, y, 16, WHITE); y += 20;
    DrawText(TextFormat("sim ticks %llu  steps/frame %d", ticks, d->sim_steps_last), x, y, 16, WHITE); y += 20;
    DrawText(TextFormat("entities alive: %d", alive), x, y, 16, WHITE); y += 20;
    if (heap) DrawText(TextFormat("WASM heap: %.1f MB", (double)heap / (1024.0 * 1024.0)), x, y, 16, WHITE);
    else DrawText("WASM heap: n/a (native)", x, y, 16, GRAY);
    y += 20;
    DrawText(TextFormat("[G] god mode: %s", d->god_mode ? "ON" : "off"), x, y, 16, d->god_mode ? GOLD : WHITE); y += 20;
    DrawText("[K] quest skip   [F3 / 3-finger tap] hide", x, y, 16, WHITE); y += 20;
    /* frame-time graph */
    for (int i = 0; i < 120; ++i) {
        float ms = d->frame_ms[(d->frame_head + i) % 120];
        int bh = (int)(ms * 1.5f); if (bh > 30) bh = 30;
        DrawRectangle(x + i * 2, 5 + h - 4 - bh, 2, bh, ms > 20.0f ? RED : GREEN);
    }
}
