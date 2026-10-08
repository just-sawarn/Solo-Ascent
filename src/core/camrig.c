#include "core/camrig.h"
#include <math.h>

void camrig_init(CamRig *c, float yaw, float pitch, float dist) {
    *c = (CamRig){.yaw = yaw, .pitch = pitch, .dist = dist, .cur_dist = dist};
}

void camrig_shake(CamRig *c, float amount) { c->shake = fminf(0.35f, c->shake + amount); }

void camrig_update(CamRig *c, PhysWorld *w, Vec3 focus, float dx, float dy, float dt) {
    if (c->locked) {
        float lx = c->lock_pos.x - focus.x, lz = c->lock_pos.z - focus.z;
        if (lx * lx + lz * lz > 1e-4f) {
            float want = atan2f(-lx, -lz);
            c->yaw = lerp_angle(c->yaw, want, 1.0f - expf(-8.0f * dt));
        }
        c->pitch += (0.30f - c->pitch) * (1.0f - expf(-4.0f * dt));
    } else {
        c->yaw -= dx;
        c->pitch += dy;
    }
    c->pitch = clampf(c->pitch, -0.25f, 1.30f);
    c->dist = clampf(c->dist, 2.0f, 16.0f);

    Vec3 dir = vec3(sinf(c->yaw) * cosf(c->pitch), sinf(c->pitch), cosf(c->yaw) * cosf(c->pitch));
    c->target = focus;
    float hit = phys_raycast(w, focus, dir, c->dist + 0.35f);
    float allowed = clampf(hit - 0.35f, 0.6f, c->dist);
    if (allowed < c->cur_dist) c->cur_dist = allowed;                               /* never clip: snap in */
    else c->cur_dist += (allowed - c->cur_dist) * (1.0f - expf(-5.0f * dt));         /* ease back out */
    c->eye = vec3_add(focus, vec3_scale(dir, c->cur_dist));
    if (c->shake > 0.001f) {
        c->shake_t += dt * 60.0f;
        c->eye.x += sinf(c->shake_t * 2.3f) * c->shake;
        c->eye.y += sinf(c->shake_t * 3.1f + 1.0f) * c->shake * 0.7f;
        c->eye.z += sinf(c->shake_t * 2.7f + 2.0f) * c->shake;
        c->shake *= expf(-9.0f * dt);
    } else c->shake = 0.0f;
    if (c->eye.y < w->ground_y + 0.25f) c->eye.y = w->ground_y + 0.25f;
}
