#ifndef SA_NPC_WORLD_H
#define SA_NPC_WORLD_H
/* NPCs in the world: they exist while their visibility conditions hold, follow their day/night schedule, and are drawn on the shared CC0 rig. */
#include <raylib.h>
#include "core/story.h"
#include "render/anim.h"

#define NPCW_MAX 96

typedef struct {
    int def;                     /* index into StoryDB.npc */
    Vector3 pos, goal;
    float yaw;
    AnimPlayer anim;
    int slot;                    /* schedule slot currently pursued */
    bool moving, talking;
} NpcInst;

typedef struct { NpcInst n[NPCW_MAX]; int count; float (*ground)(void *, float, float); void *ground_user; } NpcWorld;

void npcw_init(NpcWorld *w);
/* NPCs stand on the zone's terrain (call after npcw_init). */
void npcw_set_ground(NpcWorld *w, float (*fn)(void *, float, float), void *user);
/* Re-evaluates who is present in `region` (and "roaming" NPCs everywhere). New NPCs appear at their scheduled spot. */
void npcw_refresh(NpcWorld *w, const StoryDB *db, const StoryState *s, const StoryHooks *h, const char *region, float hour);
void npcw_update(NpcWorld *w, const StoryDB *db, float hour, float dt);
/* Draws the `max_count` nearest NPCs within `radius` of `focus` (crowds are capped for mobile GPUs). */
void npcw_submit(const NpcWorld *w, const StoryDB *db, Vector3 focus, float radius, int max_count);
/* Nearest NPC within `range` of pos, or -1. */
int  npcw_nearest(const NpcWorld *w, Vector3 pos, float range);
void npcw_face(NpcWorld *w, int i, Vector3 toward);
const char *npcw_slot_name(int slot);

#endif
