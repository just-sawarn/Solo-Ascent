#ifndef SA_HEROES_H
#define SA_HEROES_H
#include <stdbool.h>
#include <raylib.h>
/* Styled characters built on the CC0 base body: outfit paint + overlay parts + pose. */
void heroes_init(void);
/* Drive Kael's animation: speed in m/s, one-frame triggers for attack and dodge roll. */
void heroes_update_kael(float dt, float speed, bool attack, bool roll);
/* Kael, the Ascendant: `pos`/`yaw` place him in the world, `t` drives the idle sway. */
void heroes_submit_kael(Vector3 pos, float yaw, float t);
#endif
