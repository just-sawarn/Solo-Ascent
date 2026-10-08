#ifndef SA_CAMRIG_H
#define SA_CAMRIG_H
/* Third-person orbit camera with collision (ray cast against the physics statics) and lock-on framing. */
#include <stdbool.h>
#include "core/physics.h"

typedef struct {
    float yaw, pitch;          /* eye = target + (sin yaw cos p, sin p, cos yaw cos p) * dist */
    float dist, cur_dist;
    Vec3 eye, target;
    bool locked; Vec3 lock_pos;
    float shake, shake_t;      /* impact shake amplitude (metres) and phase */
} CamRig;

void camrig_init(CamRig *c, float yaw, float pitch, float dist);
/* look_dx/dy in radians this frame (ignored while locked). */
/* Add camera shake; amplitudes accumulate (max 0.35 m) and decay quickly. */
void camrig_shake(CamRig *c, float amount);
void camrig_update(CamRig *c, PhysWorld *w, Vec3 focus, float look_dx, float look_dy, float dt);

#endif
