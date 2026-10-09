#ifndef SA_STORY_HUD_H
#define SA_STORY_HUD_H
/* Quest tracker, minimap with quest markers, quest markers above NPC heads, talk prompt and the world clock. */
#include <raylib.h>
#include "core/combat.h"
#include "core/enemy_ai.h"
#include "core/party.h"
#include "core/story.h"
#include "world/npc_world.h"

typedef struct {
    Camera3D cam; float cam_yaw;
    World *w; CombatWorld *cw; const EnemyPool *enemies; const Party *party;
    const StoryDB *db; const StoryState *ss; const NpcWorld *npcs;
    Vector3 focus;          /* active member position */
    float hour; const char *region_name;
    int near_npc;           /* npc instance in talk range or -1 */
    bool touch_ui;
} StoryHudCtx;

/* Returns the quest marker glyph for an NPC: '!' offer, '?' turn-in, 0 none. */
char npc_marker(const StoryDB *db, const StoryState *ss, int npc_def);
void story_hud_draw(const StoryHudCtx *c);

#endif
