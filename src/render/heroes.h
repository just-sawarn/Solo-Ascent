#ifndef SA_HEROES_H
#define SA_HEROES_H
#include <stdbool.h>
#include <raylib.h>
/* Styled characters built on the CC0 base body: outfit paint + overlay parts + pose. */
void heroes_init(void);
/* Drive Kael's animation from the player controller state. */
typedef struct {
    float speed;                           /* horizontal m/s */
    bool grounded, attacking, rolling;     /* state from the player controller */
    bool ev_attack, ev_roll;               /* started this frame */
    float attack_seconds, roll_seconds;    /* gameplay durations the clips are fitted into */
    bool dead, staggered, guarding;        /* combat state */
    float flash;                           /* white hit flash 0..1 */
} KaelMotion;
void heroes_update_kael(float dt, const KaelMotion *m);
/* Kael, the Ascendant: `pos`/`yaw` place him in the world, `t` drives the idle sway. */
void heroes_submit_kael(Vector3 pos, float yaw, float t);
#endif
