#include "ui/input.h"
#include <math.h>

enum { B_ATTACK, B_ROLL, B_JUMP, B_LOCK, B_S1, B_S2, B_S3, B_GUARD, B_SWAP, B_CMD, B_MENU, B_COUNT };

static float ui_scale(void) {
    float s = fminf((float)GetScreenWidth(), (float)GetScreenHeight()) / 720.0f;
    return s < 0.8f ? 0.8f : s;
}

static void button_layout(int i, Vector2 *c, float *r) {
    float w = (float)GetScreenWidth(), h = (float)GetScreenHeight(), s = ui_scale();
    static const float X[B_COUNT] = {130, 250, 60, 300, 215, 185, 95, 330, 60, 150, 240}, Y[B_COUNT] = {130, 70, 250, 235, 175, 265, 340, 130, 0, 0, 0};
    static const float R[B_COUNT] = {62, 42, 42, 30, 34, 34, 34, 36, 34, 30, 30};
    *c = (Vector2){w - X[i] * s, i >= B_SWAP ? 64 * s : h - Y[i] * s};
    *r = R[i] * s;
}

void input_init(Input *in) {
    *in = (Input){.joy_id = -1, .look_id = -1, .btn_id = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, .swap_to = -1};
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
    case B_LOCK: in->lock = true; break;
    case B_S1: in->skill[0] = true; break;
    case B_S2: in->skill[1] = true; break;
    case B_S3: in->skill[2] = true; break;
    case B_SWAP: in->swap_next = true; break;
    case B_CMD: in->cmd_cycle = true; break;
    case B_MENU: in->menu = true; break;
    default: break; /* guard is a held button */
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
    for (int b = 0; b < B_COUNT; ++b) if (in->btn_id[b] >= 0 && !seen(ids, count, in->btn_id[b])) in->btn_id[b] = -1;

    float w = (float)GetScreenWidth();
    for (int i = 0; i < count; ++i) {
        int id = ids[i];
        Vector2 p = GetTouchPosition(i);
        if (id == in->joy_id) { in->joy_pos = p; continue; }
        if (id == in->look_id) { in->look_dx += (p.x - in->look_last.x) * 0.005f; in->look_dy += (p.y - in->look_last.y) * 0.004f; in->look_last = p; continue; }
        bool is_btn = false;
        for (int b = 0; b < B_COUNT; ++b) if (in->btn_id[b] == id) is_btn = true;
        if (is_btn) continue;
        /* a new touch */
        bool on_button = false;
        for (int b = 0; b < B_COUNT; ++b) {
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
    if (IsKeyPressed(KEY_I) || IsKeyPressed(KEY_M)) in->menu = true;
    in->move_x = in->move_y = in->look_dx = in->look_dy = 0.0f;
    in->sprint = in->walk = in->guard = false;
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
    if (IsKeyDown(KEY_E)) in->guard = true;
    for (int k = 0; k < 4; ++k) if (IsKeyPressed(KEY_ONE + k)) in->skill[k] = true;
    for (int k = 0; k < 4; ++k) if (IsKeyPressed(KEY_FIVE + k)) in->swap_to = k;
    if (IsKeyPressed(KEY_R)) in->swap_next = true;
    if (IsKeyPressed(KEY_Z)) in->quick[0] = true;
    if (IsKeyPressed(KEY_X)) in->quick[1] = true;
    if (IsKeyPressed(KEY_C)) in->quick[2] = true;
    if (IsKeyPressed(KEY_V)) in->quick[3] = true;
    if (IsKeyPressed(KEY_B)) in->cmd_cycle = true;

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
        bool mod = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_2); /* hold LT: face buttons become skills 1-4 */
        if (mod) {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) in->skill[0] = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP)) in->skill[1] = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) in->skill[2] = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) in->skill[3] = true;
        } else {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) in->jump = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) in->attack = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) in->roll = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_THUMB)) in->lock = true;
        }
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) in->guard = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) in->swap_next = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) in->cmd_cycle = true;
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) in->sprint = true;
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) in->walk = true;
    }

    update_touch(in);
    if (in->btn_id[B_GUARD] >= 0) in->guard = true;
    float l = sqrtf(in->move_x * in->move_x + in->move_y * in->move_y);
    if (l > 1.0f) { in->move_x /= l; in->move_y /= l; }
}

void input_consume(Input *in) { in->jump = in->attack = in->roll = in->lock = in->swap_next = in->cmd_cycle = in->menu = false; in->swap_to = -1; for (int k = 0; k < 4; ++k) in->quick[k] = false; for (int k = 0; k < 4; ++k) in->skill[k] = false; }

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
    static const char *label[B_COUNT] = {"ATK", "ROLL", "JUMP", "LOCK", "1", "2", "3", "GUARD", "SWAP", "CMD", "MENU"};
    for (int b = 0; b < B_COUNT; ++b) {
        Vector2 c; float r; button_layout(b, &c, &r);
        bool down = in->btn_id[b] >= 0;
        DrawCircleV(c, r, (Color){255, 200, 50, (unsigned char)(down ? 150 : 60)});
        DrawCircleLinesV(c, r, (Color){255, 220, 120, 190});
        int fs = (int)((b == B_GUARD ? 14 : 18) * s);
        DrawText(label[b], (int)(c.x - (float)MeasureText(label[b], fs) / 2), (int)(c.y - fs / 2), fs, WHITE);
    }
}
