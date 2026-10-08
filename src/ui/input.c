#include "ui/input.h"
#include <math.h>

enum { B_ATTACK, B_ROLL, B_JUMP, B_LOCK };

static float ui_scale(void) {
    float s = fminf((float)GetScreenWidth(), (float)GetScreenHeight()) / 720.0f;
    return s < 0.8f ? 0.8f : s;
}

static void button_layout(int i, Vector2 *c, float *r) {
    float w = (float)GetScreenWidth(), h = (float)GetScreenHeight(), s = ui_scale();
    switch (i) {
    case B_ATTACK: *c = (Vector2){w - 130 * s, h - 130 * s}; *r = 62 * s; break;
    case B_ROLL:   *c = (Vector2){w - 255 * s, h - 80 * s};  *r = 44 * s; break;
    case B_JUMP:   *c = (Vector2){w - 90 * s,  h - 265 * s}; *r = 44 * s; break;
    default:       *c = (Vector2){w - 250 * s, h - 200 * s}; *r = 34 * s; break;
    }
}

void input_init(Input *in) {
    *in = (Input){.joy_id = -1, .look_id = -1, .btn_id = {-1, -1, -1, -1}};
}

static bool seen(const int *ids, int n, int id) {
    for (int i = 0; i < n; ++i) if (ids[i] == id) return true;
    return false;
}

static void latch(Input *in, int b) {
    switch (b) {
    case B_ATTACK: in->attack = true; break;
    case B_ROLL: in->roll = true; break;
    case B_JUMP: in->jump = true; break;
    default: in->lock = true; break;
    }
}

static void update_touch(Input *in) {
    int n = GetTouchPointCount();
    int ids[10], count = n > 10 ? 10 : n;
    for (int i = 0; i < count; ++i) ids[i] = GetTouchPointId(i);
    if (count > 0) in->touch_ui = true;
    /* release touches that ended */
    if (in->joy_id >= 0 && !seen(ids, count, in->joy_id)) in->joy_id = -1;
    if (in->look_id >= 0 && !seen(ids, count, in->look_id)) in->look_id = -1;
    for (int b = 0; b < 4; ++b) if (in->btn_id[b] >= 0 && !seen(ids, count, in->btn_id[b])) in->btn_id[b] = -1;

    float w = (float)GetScreenWidth();
    for (int i = 0; i < count; ++i) {
        int id = ids[i];
        Vector2 p = GetTouchPosition(i);
        if (id == in->joy_id) { in->joy_pos = p; continue; }
        if (id == in->look_id) { in->look_dx += (p.x - in->look_last.x) * 0.005f; in->look_dy += (p.y - in->look_last.y) * 0.004f; in->look_last = p; continue; }
        bool is_btn = false;
        for (int b = 0; b < 4; ++b) if (in->btn_id[b] == id) is_btn = true;
        if (is_btn) continue;
        /* a new touch */
        bool on_button = false;
        for (int b = 0; b < 4; ++b) {
            Vector2 c; float r; button_layout(b, &c, &r);
            float dx = p.x - c.x, dy = p.y - c.y;
            if (dx * dx + dy * dy <= r * r * 1.3f) { in->btn_id[b] = id; latch(in, b); on_button = true; break; }
        }
        if (on_button) continue;
        if (p.x < w * 0.45f && in->joy_id < 0) { in->joy_id = id; in->joy_origin = p; in->joy_pos = p; }
        else if (in->look_id < 0) { in->look_id = id; in->look_last = p; }
    }
    if (in->joy_id >= 0) {
        float s = ui_scale(), rad = 85.0f * s;
        float dx = (in->joy_pos.x - in->joy_origin.x) / rad, dy = (in->joy_pos.y - in->joy_origin.y) / rad;
        float l = sqrtf(dx * dx + dy * dy);
        if (l > 1.0f) { dx /= l; dy /= l; l = 1.0f; }
        in->move_x += dx; in->move_y += -dy;
        if (l > 0.96f) in->sprint = true;
    }
}

void input_update(Input *in, float dt, bool active) {
    in->move_x = in->move_y = in->look_dx = in->look_dy = 0.0f;
    in->sprint = in->walk = false;
    if (!active) return;

    /* keyboard */
    in->move_x += (float)(IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) - (float)(IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT));
    in->move_y += (float)(IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) - (float)(IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN));
    if (IsKeyDown(KEY_LEFT_SHIFT)) in->sprint = true;
    if (IsKeyDown(KEY_LEFT_CONTROL)) in->walk = true;
    if (IsKeyPressed(KEY_SPACE)) in->jump = true;
    if (IsKeyPressed(KEY_Q)) in->roll = true;
    if (IsKeyPressed(KEY_J)) in->attack = true;
    if (IsKeyPressed(KEY_TAB)) in->lock = true;

    /* mouse: only while the pointer is locked (desktop) */
    if (!in->touch_ui && IsCursorHidden()) {
        Vector2 d = GetMouseDelta();
        in->look_dx += d.x * 0.0022f;
        in->look_dy += d.y * 0.0022f;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) in->attack = true;
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) in->roll = true;
        if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) in->lock = true;
    }

    /* gamepad 0 */
    if (IsGamepadAvailable(0)) {
        float lx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X), ly = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        float rx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X), ry = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
        float ll = sqrtf(lx * lx + ly * ly), rl = sqrtf(rx * rx + ry * ry);
        if (ll > 0.2f) { in->move_x += lx; in->move_y += -ly; }
        if (rl > 0.2f) { in->look_dx += rx * 2.8f * dt; in->look_dy += ry * 2.0f * dt; }
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) in->jump = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) in->attack = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) in->roll = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_THUMB)) in->lock = true;
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) in->sprint = true;
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) in->walk = true;
    }

    update_touch(in);
    float l = sqrtf(in->move_x * in->move_x + in->move_y * in->move_y);
    if (l > 1.0f) { in->move_x /= l; in->move_y /= l; }
}

void input_consume(Input *in) { in->jump = in->attack = in->roll = in->lock = false; }

void input_draw_touch(const Input *in) {
    if (!in->touch_ui) return;
    float s = ui_scale();
    if (in->joy_id >= 0) {
        DrawCircleV(in->joy_origin, 85 * s, (Color){255, 255, 255, 40});
        DrawCircleLinesV(in->joy_origin, 85 * s, (Color){255, 255, 255, 110});
        DrawCircleV(in->joy_pos, 34 * s, (Color){255, 255, 255, 120});
    } else {
        Vector2 c = {110 * s, (float)GetScreenHeight() - 130 * s};
        DrawCircleLinesV(c, 60 * s, (Color){255, 255, 255, 60});
    }
    static const char *label[4] = {"ATK", "ROLL", "JUMP", "LOCK"};
    for (int b = 0; b < 4; ++b) {
        Vector2 c; float r; button_layout(b, &c, &r);
        bool down = in->btn_id[b] >= 0;
        DrawCircleV(c, r, (Color){255, 200, 50, (unsigned char)(down ? 150 : 60)});
        DrawCircleLinesV(c, r, (Color){255, 220, 120, 190});
        int fs = (int)(18 * s);
        DrawText(label[b], (int)(c.x - (float)MeasureText(label[b], fs) / 2), (int)(c.y - fs / 2), fs, WHITE);
    }
}
