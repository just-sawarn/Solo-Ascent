#include "core/ecs.h"
#include <string.h>

void ecs_init(World *w) {
    memset(w, 0, sizeof *w);
    for (int i = 0; i < ECS_MAX_ENTITIES; ++i) {
        w->generation[i] = 1; /* gen 0 never used so handle 0 stays invalid */
        w->free_list[i] = (uint16_t)(ECS_MAX_ENTITIES - 1 - i); /* pop yields index 0 first */
    }
    w->free_count = ECS_MAX_ENTITIES;
}

Entity ecs_create(World *w, uint32_t mask) {
    if (w->free_count == 0 || mask == 0) return ENTITY_NULL;
    uint16_t idx = w->free_list[--w->free_count];
    w->mask[idx] = mask;
    w->pos[idx] = w->prev_pos[idx] = (Vec3){0, 0, 0};
    w->yaw[idx] = w->prev_yaw[idx] = 0;
    w->scale[idx] = 1.0f;
    w->vel[idx] = (Vec3){0, 0, 0};
    w->hp[idx] = w->hp_max[idx] = 1.0f;
    w->shape[idx] = SHAPE_CUBE;
    w->color[idx] = 0xFFFFFFFFu;
    w->faction[idx] = FACTION_NONE;
    w->alive_count++;
    if (idx + 1 > w->high_water) w->high_water = idx + 1;
    return ((Entity)w->generation[idx] << 16) | idx;
}

bool ecs_alive(const World *w, Entity e) {
    if (e == ENTITY_NULL) return false;
    uint32_t idx = ecs_index(e);
    return idx < ECS_MAX_ENTITIES && w->mask[idx] != 0 && w->generation[idx] == (e >> 16);
}

bool ecs_destroy(World *w, Entity e) {
    if (!ecs_alive(w, e)) return false;
    uint32_t idx = ecs_index(e);
    w->mask[idx] = 0;
    if (++w->generation[idx] == 0) w->generation[idx] = 1; /* skip 0 on wrap */
    w->free_list[w->free_count++] = (uint16_t)idx;
    w->alive_count--;
    return true;
}

bool ecs_has(const World *w, Entity e, uint32_t mask) {
    return ecs_alive(w, e) && (w->mask[ecs_index(e)] & mask) == mask;
}

void ecs_snapshot_transforms(World *w) {
    ECS_EACH(w, i, C_TRANSFORM) {
        w->prev_pos[i] = w->pos[i];
        w->prev_yaw[i] = w->yaw[i];
    }
}

Vec3 ecs_render_pos(const World *w, uint32_t i, float alpha) {
    return vec3_lerp(w->prev_pos[i], w->pos[i], alpha);
}
float ecs_render_yaw(const World *w, uint32_t i, float alpha) {
    return lerp_angle(w->prev_yaw[i], w->yaw[i], alpha);
}
