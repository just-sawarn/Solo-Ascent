#ifndef SA_ECS_H
#define SA_ECS_H
/*
 * Plain-C ECS: fixed-capacity pools, struct-of-arrays component storage,
 * generational entity handles. No allocation after ecs_init (the World is
 * caller-owned, typically static).
 */
#include <stdbool.h>
#include <stdint.h>
#include "core/vec.h"

#define ECS_MAX_ENTITIES 4096

typedef uint32_t Entity; /* (generation << 16) | index; 0 is never valid */
#define ENTITY_NULL 0u

enum {
    C_TRANSFORM = 1u << 0, /* pos, prev_pos, yaw, prev_yaw, scale */
    C_VELOCITY  = 1u << 1,
    C_HEALTH    = 1u << 2,
    C_RENDER    = 1u << 3, /* placeholder shape + color until the renderer milestone */
    C_FACTION   = 1u << 4,
};

typedef enum { SHAPE_CUBE, SHAPE_SPHERE, SHAPE_CYLINDER } Shape;
typedef enum { FACTION_NONE, FACTION_PLAYER, FACTION_ENEMY, FACTION_NEUTRAL } Faction;

typedef struct World {
    /* bookkeeping */
    uint16_t generation[ECS_MAX_ENTITIES];
    uint32_t mask[ECS_MAX_ENTITIES]; /* 0 => slot is free */
    uint16_t free_list[ECS_MAX_ENTITIES];
    int free_count;
    int alive_count;
    int high_water; /* highest slot index ever used + 1, bounds iteration */

    /* transform (interpolated: prev_* written by ecs_snapshot_transforms) */
    Vec3  pos[ECS_MAX_ENTITIES];
    Vec3  prev_pos[ECS_MAX_ENTITIES];
    float yaw[ECS_MAX_ENTITIES];
    float prev_yaw[ECS_MAX_ENTITIES];
    float scale[ECS_MAX_ENTITIES];
    Vec3  vel[ECS_MAX_ENTITIES];
    float hp[ECS_MAX_ENTITIES];
    float hp_max[ECS_MAX_ENTITIES];
    uint8_t  shape[ECS_MAX_ENTITIES];
    uint32_t color[ECS_MAX_ENTITIES]; /* 0xRRGGBBAA */
    uint8_t  faction[ECS_MAX_ENTITIES];
} World;

void   ecs_init(World *w);
Entity ecs_create(World *w, uint32_t mask);
bool   ecs_destroy(World *w, Entity e);
bool   ecs_alive(const World *w, Entity e);
bool   ecs_has(const World *w, Entity e, uint32_t mask);
static inline uint32_t ecs_index(Entity e) { return e & 0xFFFFu; }

/* Call at the start of each fixed step, before gameplay systems mutate pos/yaw. */
void ecs_snapshot_transforms(World *w);
/* Interpolated transform for rendering (alpha in [0,1]). */
Vec3  ecs_render_pos(const World *w, uint32_t idx, float alpha);
float ecs_render_yaw(const World *w, uint32_t idx, float alpha);

/* Iterate live entities that have all bits in `m`. */
#define ECS_EACH(w, i, m) \
    for (uint32_t i = 0; i < (uint32_t)(w)->high_water; ++i) \
        if (((w)->mask[i] & (m)) == (m) && (m) != 0)

#endif
