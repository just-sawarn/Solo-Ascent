#include "core/player.h"
#include <math.h>

void player_init(PlayerCtl *p, Vec3 pos) {
    *p = (PlayerCtl){0};
    char_init(&p->ch, pos);
}

bool player_rolling(const PlayerCtl *p) { return p->roll_t > 0.0f; }
bool player_attacking(const PlayerCtl *p) { return p->attack_t > 0.0f; }
bool player_invulnerable(const PlayerCtl *p) {
    float elapsed = PLAYER_ROLL_TIME - p->roll_t;
    return p->roll_t > 0.0f && elapsed >= 0.05f && elapsed <= 0.45f;
}

static float turn_toward(float yaw, float target, float max_step) {
    const float pi = 3.14159265f;
    float d = fmodf(target - yaw, 2.0f * pi);
    if (d > pi) d -= 2.0f * pi;
    if (d < -pi) d += 2.0f * pi;
    if (d > max_step) d = max_step;
    if (d < -max_step) d = -max_step;
    return yaw + d;
}

void player_step(PlayerCtl *p, PhysWorld *w, const PlayerIn *in, float dt) {
    p->ev_attack = p->ev_roll = p->ev_jump = false;
    if (p->attack_t > 0) p->attack_t = fmaxf(0.0f, p->attack_t - dt);
    if (p->roll_t > 0) p->roll_t = fmaxf(0.0f, p->roll_t - dt);
    if (p->roll_cd > 0) p->roll_cd = fmaxf(0.0f, p->roll_cd - dt);

    float mag = sqrtf(in->mx * in->mx + in->mz * in->mz);
    if (mag > 1.0f) mag = 1.0f;
    float dx = 0, dz = 0;
    if (mag > 1e-3f) { dx = in->mx / sqrtf(in->mx * in->mx + in->mz * in->mz); dz = in->mz / sqrtf(in->mx * in->mx + in->mz * in->mz); }

    bool grounded = p->ch.grounded;
    bool busy = p->roll_t > 0;

    /* start a roll (grounded, not attacking) */
    if (in->roll && !busy && p->roll_cd <= 0 && grounded && p->attack_t <= 0) {
        p->roll_t = PLAYER_ROLL_TIME;
        p->roll_cd = PLAYER_ROLL_TIME + PLAYER_ROLL_COOLDOWN;
        p->roll_dir = mag > 1e-3f ? vec3(dx, 0, dz) : vec3(sinf(p->yaw), 0, cosf(p->yaw));
        p->ev_roll = true;
        busy = true;
    }
    /* start an attack (grounded, not rolling); turn toward the input direction at the start */
    if (in->attack && !busy && p->attack_t <= 0 && grounded) {
        p->attack_t = PLAYER_ATTACK_TIME;
        p->ev_attack = true;
        if (mag > 1e-3f) p->yaw = atan2f(dx, dz);
        else if (in->has_lock) p->yaw = atan2f(in->lock_x - p->ch.pos.x, in->lock_z - p->ch.pos.z);
    }

    CharMove mv = {0, 0, false};
    if (busy) {
        mv.wish_x = p->roll_dir.x * PLAYER_ROLL_SPEED;
        mv.wish_z = p->roll_dir.z * PLAYER_ROLL_SPEED;
        p->yaw = atan2f(p->roll_dir.x, p->roll_dir.z);
    } else {
        float speed = in->walk ? PLAYER_WALK_SPEED : (in->sprint ? PLAYER_SPRINT_SPEED : PLAYER_JOG_SPEED);
        if (p->attack_t > 0) speed *= 0.25f;
        mv.wish_x = dx * speed * mag;
        mv.wish_z = dz * speed * mag;
        mv.jump = in->jump && p->attack_t <= 0;
        if (p->attack_t <= 0) {
            if (in->has_lock) p->yaw = turn_toward(p->yaw, atan2f(in->lock_x - p->ch.pos.x, in->lock_z - p->ch.pos.z), 12.0f * dt);
            else if (mag > 1e-3f) p->yaw = turn_toward(p->yaw, atan2f(dx, dz), 14.0f * dt);
        }
    }
    bool was_grounded = p->ch.grounded;
    char_step(w, &p->ch, mv, dt);
    if (mv.jump && p->ch.vel.y > 0 && was_grounded) p->ev_jump = true;
    p->speed_xz = sqrtf(p->ch.vel.x * p->ch.vel.x + p->ch.vel.z * p->ch.vel.z);
}
