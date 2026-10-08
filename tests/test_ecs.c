#include "testutil.h"
#include "core/ecs.h"

static World w; /* large: keep off the stack */

int main(void) {
    ecs_init(&w);
    CHECK(w.alive_count == 0);

    Entity a = ecs_create(&w, C_TRANSFORM | C_HEALTH);
    Entity b = ecs_create(&w, C_TRANSFORM);
    CHECK(a != ENTITY_NULL && b != ENTITY_NULL && a != b);
    CHECK(ecs_has(&w, a, C_HEALTH) && !ecs_has(&w, b, C_HEALTH));
    CHECK(ecs_create(&w, 0) == ENTITY_NULL);

    /* stale handle detection after slot reuse */
    CHECK(ecs_destroy(&w, a));
    CHECK(!ecs_alive(&w, a) && !ecs_destroy(&w, a));
    Entity c = ecs_create(&w, C_TRANSFORM);
    CHECK(ecs_index(c) == ecs_index(a) && c != a && ecs_alive(&w, c) && !ecs_alive(&w, a));

    /* iteration */
    int n = 0;
    ECS_EACH(&w, i, C_TRANSFORM) { (void)i; n++; }
    CHECK(n == 2);

    /* interpolation */
    w.pos[ecs_index(b)] = vec3(0, 0, 0);
    ecs_snapshot_transforms(&w);
    w.pos[ecs_index(b)] = vec3(10, 0, 0);
    CHECK_NEAR(ecs_render_pos(&w, ecs_index(b), 0.25f).x, 2.5, 1e-5);
    w.yaw[ecs_index(b)] = 3.0f; w.prev_yaw[ecs_index(b)] = -3.0f; /* shortest path crosses pi */
    float y = ecs_render_yaw(&w, ecs_index(b), 0.5f);
    CHECK(fabsf(fabsf(y) - 3.14159265f) < 0.01f);

    /* exhaustion and recovery */
    ecs_init(&w);
    int made = 0;
    while (ecs_create(&w, C_TRANSFORM) != ENTITY_NULL) made++;
    CHECK(made == ECS_MAX_ENTITIES);
    CHECK(ecs_create(&w, C_TRANSFORM) == ENTITY_NULL);
    Entity last = ((Entity)w.generation[5] << 16) | 5;
    CHECK(ecs_destroy(&w, last));
    CHECK(ecs_create(&w, C_TRANSFORM) != ENTITY_NULL);
    TEST_DONE();
}
