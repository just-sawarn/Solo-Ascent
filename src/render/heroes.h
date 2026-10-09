#ifndef SA_HEROES_H
#define SA_HEROES_H
#include <stdbool.h>
#include <raylib.h>
/* The four party members on the shared CC0 rig: animation state machines, outfit paint and overlay parts.
 * member: 0 Kael, 1 Bram, 2 Lyra, 3 Mirelle. */
void heroes_init(void);
int  heroes_clip(const char *name);   /* animation clip index by name, -1 if unknown */

typedef struct {
    float speed;                           /* horizontal m/s */
    bool grounded, casting, rolling;       /* state from the controller / combat */
    bool ev_cast, ev_roll;                 /* started this frame */
    int cast_clip;                         /* clip for the cast that just started */
    float cast_seconds, roll_seconds;      /* gameplay durations the clips are fitted into */
    bool dead, staggered, guarding;
    float flash;                           /* white hit flash 0..1 */
} MemberMotion;

/* Which CC0 weapon model a member holds: "dagger", "short_sword", "long_sword", "katana", "axe", "club", "spear" (staves are built in). */
void heroes_set_weapon(int member, const char *model);
void heroes_update(int member, float dt, const MemberMotion *m);
void heroes_submit(int member, Vector3 pos, float yaw, float t);
#endif
