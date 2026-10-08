#include "testutil.h"
#include "core/player.h"

static PhysWorld W;
static const float DT = 1.0f / 60.0f;

int main(void) {
    phys_init(&W, 0.0f);
    PlayerCtl p; player_init(&p, vec3(0, 0, 0));
    PlayerIn in = {0};
    for (int i = 0; i < 10; ++i) player_step(&p, &W, &in, DT);
    CHECK(p.ch.grounded); CHECK_NEAR(p.speed_xz, 0.0, 1e-4);

    /* jog / sprint / walk speeds */
    in.mz = 1.0f;
    for (int i = 0; i < 60; ++i) player_step(&p, &W, &in, DT);
    CHECK_NEAR(p.speed_xz, PLAYER_JOG_SPEED, 0.05);
    in.sprint = true; for (int i = 0; i < 60; ++i) player_step(&p, &W, &in, DT);
    CHECK_NEAR(p.speed_xz, PLAYER_SPRINT_SPEED, 0.05);
    in.sprint = false; in.walk = true; for (int i = 0; i < 60; ++i) player_step(&p, &W, &in, DT);
    CHECK_NEAR(p.speed_xz, PLAYER_WALK_SPEED, 0.05);
    CHECK(fabsf(p.yaw) < 0.05f); /* facing +z */
    in.walk = false;

    /* facing turns smoothly, not instantly */
    in.mx = 1; in.mz = 0;
    player_step(&p, &W, &in, DT);
    CHECK(p.yaw > 0.0f && p.yaw < 1.0f);
    for (int i = 0; i < 40; ++i) player_step(&p, &W, &in, DT);
    CHECK_NEAR(p.yaw, 1.5708, 0.02);

    /* roll: i-frames window, fixed duration, cooldown, can't re-roll mid-roll */
    player_init(&p, vec3(0, 0, 0)); in = (PlayerIn){0};
    for (int i = 0; i < 5; ++i) player_step(&p, &W, &in, DT);
    in.roll = true; in.mz = 1;
    player_step(&p, &W, &in, DT);
    CHECK(p.ev_roll); CHECK(player_rolling(&p));
    in.roll = false;
    int inv = 0, steps = 0;
    while (player_rolling(&p) && steps < 200) { player_step(&p, &W, &in, DT); if (player_invulnerable(&p)) inv++; steps++; }
    CHECK(steps >= 34 && steps <= 38);
    CHECK(inv >= 20 && inv <= 28);
    CHECK(p.ch.pos.z > 3.0f);
    in.roll = true; player_step(&p, &W, &in, DT);
    CHECK(!p.ev_roll); /* cooldown */

    /* attack locks movement speed and fires one event */
    player_init(&p, vec3(0, 0, 0)); in = (PlayerIn){0};
    for (int i = 0; i < 5; ++i) player_step(&p, &W, &in, DT);
    in.attack = true; in.mz = 1;
    player_step(&p, &W, &in, DT);
    CHECK(p.ev_attack && player_attacking(&p));
    in.attack = false;
    for (int i = 0; i < 20; ++i) player_step(&p, &W, &in, DT);
    CHECK(p.speed_xz < PLAYER_JOG_SPEED * 0.3f);
    for (int i = 0; i < 60; ++i) player_step(&p, &W, &in, DT);
    CHECK(!player_attacking(&p)); CHECK(p.speed_xz > 4.0f);

    /* jump event only when actually leaving the ground; no roll in the air */
    player_init(&p, vec3(0, 0, 0)); in = (PlayerIn){0};
    for (int i = 0; i < 5; ++i) player_step(&p, &W, &in, DT);
    in.jump = true; player_step(&p, &W, &in, DT);
    CHECK(p.ev_jump);
    in.jump = false; in.roll = true; player_step(&p, &W, &in, DT);
    CHECK(!p.ev_roll);

    /* lock-on: faces the target while strafing */
    player_init(&p, vec3(0, 0, 0)); in = (PlayerIn){0};
    in.has_lock = true; in.lock_x = 10; in.lock_z = 0;
    for (int i = 0; i < 60; ++i) player_step(&p, &W, &in, DT);
    CHECK_NEAR(p.yaw, 1.5708, 0.02);
    in.mx = 0; in.mz = 1; /* strafing while locked keeps facing the target */
    for (int i = 0; i < 30; ++i) player_step(&p, &W, &in, DT);
    CHECK(p.yaw > 1.6f && p.yaw < 2.2f); /* target now slightly behind: facing swings with it */
    TEST_DONE();
}
