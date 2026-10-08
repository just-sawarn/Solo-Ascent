#ifndef SA_PLAYER_H
#define SA_PLAYER_H
/* Third-person player locomotion: speeds, facing, jump, dodge roll (with invulnerability window), attack lock-in.
 * Pure logic on top of core/physics; the game feeds it world-space input. */
#include <stdbool.h>
#include "core/physics.h"

#define PLAYER_WALK_SPEED 2.0f
#define PLAYER_JOG_SPEED 5.0f
#define PLAYER_SPRINT_SPEED 7.5f
#define PLAYER_ATTACK_TIME 0.55f
#define PLAYER_ROLL_TIME 0.60f
#define PLAYER_ROLL_SPEED 8.5f
#define PLAYER_ROLL_COOLDOWN 0.25f

typedef struct {
    CharCtl ch;
    float yaw;                 /* facing; forward = (sin yaw, cos yaw) in (x, z) */
    float attack_t, roll_t, roll_cd;
    Vec3 roll_dir;
    float speed_xz;
    /* one-step events (cleared at the start of every step) */
    bool ev_attack, ev_roll, ev_jump;
} PlayerCtl;

typedef struct {
    float mx, mz;              /* desired world-space direction, magnitude 0..1 */
    bool sprint, walk;
    bool jump, attack, roll;   /* pressed */
    bool has_lock; float lock_x, lock_z; /* lock-on target position (world xz) */
} PlayerIn;

void player_init(PlayerCtl *p, Vec3 pos);
void player_step(PlayerCtl *p, PhysWorld *w, const PlayerIn *in, float dt);
/* True while dodge-roll i-frames are active (0.05 s .. 0.45 s into the roll). */
bool player_invulnerable(const PlayerCtl *p);
bool player_rolling(const PlayerCtl *p);
bool player_attacking(const PlayerCtl *p);

#endif
