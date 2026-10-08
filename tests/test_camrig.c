#include "testutil.h"
#include <math.h>
#include "core/camrig.h"

static PhysWorld W;
static const float DT = 1.0f / 60.0f;

int main(void) {
    phys_init(&W, 0.0f);
    CamRig c; camrig_init(&c, 0.0f, 0.2f, 6.0f);
    Vec3 focus = vec3(0, 1.2f, 0);
    for (int i = 0; i < 10; ++i) camrig_update(&c, &W, focus, 0, 0, DT);
    CHECK_NEAR(vec3_len(vec3_sub(c.eye, focus)), 6.0, 0.01);
    CHECK(c.eye.z > 5.0f); /* yaw 0 puts the camera on +z */

    /* wall behind the camera: eye never goes through it, snaps in instantly */
    phys_add_box(&W, vec3(-5, 0, 3.0f), vec3(5, 6, 4.0f));
    camrig_update(&c, &W, focus, 0, 0, DT);
    CHECK(c.eye.z < 3.0f); CHECK(c.cur_dist < 3.5f);
    /* removing the wall: eases back out, not instantly */
    phys_init(&W, 0.0f);
    camrig_update(&c, &W, focus, 0, 0, DT);
    CHECK(c.cur_dist < 6.0f);
    for (int i = 0; i < 120; ++i) camrig_update(&c, &W, focus, 0, 0, DT);
    CHECK(c.cur_dist > 5.9f);

    /* pitch clamps and ground clearance */
    camrig_update(&c, &W, focus, 0, -5.0f, DT);
    CHECK(c.pitch >= -0.25f - 1e-4f); CHECK(c.eye.y >= 0.25f - 1e-4f);
    camrig_update(&c, &W, focus, 0, 10.0f, DT);
    CHECK(c.pitch <= 1.30f + 1e-4f);

    /* lock-on swings the camera behind the player, looking at the target */
    camrig_init(&c, 0.0f, 0.2f, 6.0f);
    c.locked = true; c.lock_pos = vec3(10, 0, 0);
    for (int i = 0; i < 180; ++i) camrig_update(&c, &W, focus, 0, 0, DT);
    CHECK(c.eye.x < -4.0f); CHECK(fabsf(c.eye.z) < 0.5f);
    /* shake decays to nothing and is capped */
    camrig_init(&c, 0.0f, 0.2f, 6.0f);
    camrig_shake(&c, 1.0f); CHECK(c.shake <= 0.35f + 1e-6f);
    for (int i = 0; i < 120; ++i) camrig_update(&c, &W, focus, 0, 0, DT);
    CHECK(c.shake == 0.0f);
    TEST_DONE();
}
