#include "testutil.h"
#include "core/gameloop.h"

int main(void) {
    GameLoop g; gameloop_init(&g);
    CHECK(gameloop_advance(&g, 0.0) == 0);
    CHECK(gameloop_advance(&g, SIM_DT * 0.5) == 0);
    CHECK_NEAR(gameloop_alpha(&g), 0.5, 1e-4);
    CHECK(gameloop_advance(&g, SIM_DT * 0.5 + 1e-9) == 1);

    /* 1 simulated second at an irregular 144Hz render rate => ~60 ticks */
    gameloop_init(&g);
    int steps = 0;
    for (int i = 0; i < 144; ++i) steps += gameloop_advance(&g, 1.0 / 144.0);
    CHECK(steps >= 59 && steps <= 60);
    CHECK((unsigned long long)steps == g.sim_ticks);

    /* huge stall is clamped, not replayed */
    gameloop_init(&g);
    CHECK(gameloop_advance(&g, 10.0) == SIM_MAX_STEPS_PER_FRAME);
    CHECK(g.dropped_steps > 0 && gameloop_alpha(&g) < 1.0f);

    /* pause freezes time */
    g.paused = 1;
    CHECK(gameloop_advance(&g, 1.0) == 0);
    TEST_DONE();
}
