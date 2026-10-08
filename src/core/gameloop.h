#ifndef SA_GAMELOOP_H
#define SA_GAMELOOP_H
/* Fixed-timestep accumulator. Pure logic, no clocks: the platform passes real dt. */

#define SIM_HZ 60
#define SIM_DT (1.0f / SIM_HZ)
#define SIM_MAX_STEPS_PER_FRAME 5 /* spiral-of-death guard */

typedef struct {
    double accumulator;
    unsigned long long sim_ticks;
    double sim_time;
    int dropped_steps; /* total steps discarded by the guard */
    int paused;
} GameLoop;

void gameloop_init(GameLoop *g);
/* Advance by real elapsed seconds; returns number of fixed steps to run now. */
int gameloop_advance(GameLoop *g, double real_dt);
/* Render interpolation factor in [0,1) after stepping. */
float gameloop_alpha(const GameLoop *g);

#endif
