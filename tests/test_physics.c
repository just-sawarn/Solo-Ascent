#include "testutil.h"
#include "core/physics.h"

static PhysWorld W;
static const float DT = 1.0f / 60.0f;

static void run(CharCtl *c, float wx, float wz, int steps, bool jump_first) {
    for (int i = 0; i < steps; ++i) char_step(&W, c, (CharMove){wx, wz, jump_first && i == 0}, DT);
}

int main(void) {
    /* falls and lands on the ground */
    phys_init(&W, 0.0f);
    CharCtl c; char_init(&c, vec3(0, 2.0f, 0));
    run(&c, 0, 0, 120, false);
    CHECK(c.grounded); CHECK_NEAR(c.pos.y, 0.0, 1e-4);

    /* jump rises ~ v^2/2g and returns */
    float peak = 0; bool left = false;
    char_step(&W, &c, (CharMove){0, 0, true}, DT);
    for (int i = 0; i < 120; ++i) { char_step(&W, &c, (CharMove){0, 0, false}, DT); if (c.pos.y > peak) peak = c.pos.y; if (!c.grounded) left = true; }
    CHECK(left); CHECK(peak > 1.2f && peak < 1.6f); CHECK(c.grounded);

    /* wall blocks, and sliding keeps tangential motion */
    phys_init(&W, 0.0f);
    phys_add_box(&W, vec3(2, 0, -5), vec3(3, 3, 5));
    char_init(&c, vec3(0, 0, 0));
    run(&c, 6, 0, 120, false);
    CHECK(c.pos.x <= 2.0f - c.radius + 1e-3f);
    float z0 = c.pos.z; run(&c, 6, 3, 60, false);
    CHECK(c.pos.z > z0 + 1.0f); CHECK(c.pos.x <= 2.0f - c.radius + 1e-3f);

    /* stairs: 0.3 m step is climbed, 0.8 m crate blocks */
    phys_init(&W, 0.0f);
    phys_add_box(&W, vec3(2, 0, -2), vec3(4, 0.3f, 2));
    phys_add_box(&W, vec3(6, 0, -2), vec3(8, 0.8f, 2));
    char_init(&c, vec3(0, 0, 0));
    run(&c, 5, 0, 36, false); /* 3 m: standing on the step */
    CHECK(c.grounded); CHECK_NEAR(c.pos.y, 0.3, 1e-3);
    run(&c, 5, 0, 60, false);   /* walks off the far side, returns to ground */
    CHECK(c.grounded);
    CHECK(c.pos.x < 6.0f - c.radius + 1e-3f);

    /* jump onto the 0.8 m crate and stand on it */
    char_init(&c, vec3(5.0f, 0, 0));
    for (int i = 0; i < 45; ++i) char_step(&W, &c, (CharMove){3, 0, i == 0}, DT);
    CHECK(c.grounded); CHECK_NEAR(c.pos.y, 0.8, 1e-3); CHECK(c.pos.x > 6.0f);

    /* walk off a ledge: falls to the ground */
    run(&c, 3, 0, 200, false);
    CHECK(c.grounded); CHECK_NEAR(c.pos.y, 0.0, 1e-3);

    /* low ceiling stops a jump */
    phys_init(&W, 0.0f);
    phys_add_box(&W, vec3(-5, 2.0f, -5), vec3(5, 3.0f, 5));
    char_init(&c, vec3(0, 0, 0));
    char_step(&W, &c, (CharMove){0, 0, true}, DT);
    for (int i = 0; i < 60; ++i) { char_step(&W, &c, (CharMove){0, 0, false}, DT); CHECK(c.pos.y + c.height <= 2.0f + 1e-3f); }

    /* cylinder pillar: no penetration from any side */
    phys_init(&W, 0.0f);
    phys_add_cyl(&W, 0, 0, 0.5f, 0, 5);
    char_init(&c, vec3(-3, 0, 0.1f));
    run(&c, 4, 0, 180, false);
    float d = sqrtf(c.pos.x * c.pos.x + c.pos.z * c.pos.z);
    CHECK(d >= 0.5f + c.radius - 1e-3f);

    /* coyote time: can still jump a few frames after leaving a ledge */
    phys_init(&W, 0.0f);
    phys_add_box(&W, vec3(-1, 0, -5), vec3(1, 1.0f, 5));
    char_init(&c, vec3(0, 1.0f, 0));
    run(&c, 0, 0, 5, false);
    for (int i = 0; i < 100 && c.grounded; ++i) char_step(&W, &c, (CharMove){4, 0, false}, DT);
    CHECK(!c.grounded);
    char_step(&W, &c, (CharMove){4, 0, false}, DT); /* airborne one more frame */
    char_step(&W, &c, (CharMove){4, 0, true}, DT);
    CHECK(c.vel.y > 0);

    /* spatial hash: many boxes, queries still find the right one */
    phys_init(&W, 0.0f);
    for (int i = 0; i < 200; ++i) phys_add_box(&W, vec3((float)(i % 20) * 6 - 60, 0, (float)(i / 20) * 6 - 30), vec3((float)(i % 20) * 6 - 59, 2, (float)(i / 20) * 6 - 29));
    char_init(&c, vec3(-59.5f, 0, -32));
    run(&c, 0, 3, 120, false);
    CHECK(c.pos.z < -30.0f + 0.01f - c.radius + 0.01f);

    /* ray casts: boxes, cylinders, ground */
    phys_init(&W, 0.0f);
    phys_add_box(&W, vec3(5, 0, -1), vec3(6, 4, 1));
    phys_add_cyl(&W, 0, 10, 1.0f, 0, 5);
    CHECK_NEAR(phys_raycast(&W, vec3(0, 1, 0), vec3(1, 0, 0), 20), 5.0, 1e-4);
    CHECK_NEAR(phys_raycast(&W, vec3(0, 1, 0), vec3(0, 0, 1), 20), 9.0, 1e-4);
    CHECK_NEAR(phys_raycast(&W, vec3(0, 1, 0), vec3(0, 0, -1), 20), 20.0, 1e-4);
    CHECK_NEAR(phys_raycast(&W, vec3(0, 3, 0), vec3(0, -1, 0), 20), 3.0, 1e-4);
    TEST_DONE();
}
