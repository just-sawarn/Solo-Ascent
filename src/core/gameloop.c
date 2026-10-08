#include "core/gameloop.h"

void gameloop_init(GameLoop *g) { *g = (GameLoop){0}; }

int gameloop_advance(GameLoop *g, double real_dt) {
    if (g->paused || real_dt <= 0.0) return 0;
    g->accumulator += real_dt;
    int steps = 0;
    while (g->accumulator >= SIM_DT) {
        if (steps == SIM_MAX_STEPS_PER_FRAME) { /* too far behind: drop the backlog */
            g->dropped_steps += (int)(g->accumulator / SIM_DT);
            g->accumulator = 0.0;
            break;
        }
        g->accumulator -= SIM_DT;
        g->sim_ticks++;
        g->sim_time += SIM_DT;
        steps++;
    }
    return steps;
}

float gameloop_alpha(const GameLoop *g) { return (float)(g->accumulator / SIM_DT); }
