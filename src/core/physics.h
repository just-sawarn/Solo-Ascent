#ifndef SA_PHYSICS_H
#define SA_PHYSICS_H
/*
 * Lightweight collision world + capsule character controller (no engine dependencies).
 * Static colliders: axis-aligned boxes and vertical cylinders, indexed by a fixed spatial hash.
 * The controller slides along walls, climbs stairs up to `step_h`, jumps with coyote time and input buffering.
 * Slopes arrive with the terrain heightfield (world milestone); stairs/ledges/crates work now.
 */
#include <stdbool.h>
#include <stdint.h>
#include "core/vec.h"

#define PH_MAX_BOXES 768
#define PH_MAX_CYLS 256
#define PH_GRID 64              /* cells per side, centred on the origin */
#define PH_CELL 4.0f            /* metres per cell: world covers +-128 m, beyond that clamps to the border cell */
#define PH_CELL_CAP 24

typedef struct { Vec3 min, max; } PhBox;
typedef struct { float x, z, r, y0, y1; } PhCyl;

typedef struct {
    PhBox boxes[PH_MAX_BOXES];
    PhCyl cyls[PH_MAX_CYLS];
    int nboxes, ncyls;
    float ground_y;
    /* optional terrain: when set, the ground height is height(user,x,z) instead of the flat ground_y */
    float (*height)(const void *user, float x, float z);
    const void *height_user;
    float bound;                                        /* >0: characters are kept inside +-bound on x/z */
    uint16_t cell_item[PH_GRID * PH_GRID][PH_CELL_CAP]; /* item ids: box i, or PH_MAX_BOXES + cyl i */
    uint8_t  cell_n[PH_GRID * PH_GRID];
    uint16_t overflow[64];                              /* items whose cells were full: always tested */
    int noverflow;
    uint32_t stamp;                                     /* query dedupe */
    uint32_t seen[PH_MAX_BOXES + PH_MAX_CYLS];
} PhysWorld;

void phys_init(PhysWorld *w, float ground_y);
float phys_ground(const PhysWorld *w, float x, float z);   /* terrain or flat ground height */
bool phys_add_box(PhysWorld *w, Vec3 min, Vec3 max);
bool phys_add_cyl(PhysWorld *w, float x, float z, float r, float y0, float y1);

/* Nearest hit of a ray against statics and the ground plane. Returns distance, or maxd if nothing is hit. */
float phys_raycast(PhysWorld *w, Vec3 origin, Vec3 dir_unit, float maxd);

typedef struct {
    Vec3 pos;              /* feet */
    Vec3 vel;
    float radius, height, step_h;
    bool grounded;
    float coyote, jump_buf;
    float ground_accel, air_accel;
} CharCtl;

typedef struct {
    float wish_x, wish_z;  /* desired horizontal velocity (m/s) */
    bool jump;             /* pressed this step */
} CharMove;

void char_init(CharCtl *c, Vec3 pos);
void char_step(PhysWorld *w, CharCtl *c, CharMove in, float dt);

#define PH_GRAVITY 25.0f
#define PH_JUMP_SPEED 8.6f

#endif
