#ifndef SA_WORLDVIEW_H
#define SA_WORLDVIEW_H
/* Draws a generated Zone: vertex-coloured terrain mesh, biome props, hub buildings, dungeon walls, torches, portals. */
#include <raylib.h>
#include "core/worldgen.h"
#include "render/renderer.h"
#include "core/boss.h"
struct BossRtTag;

void worldview_init(void);                         /* after proc_init */
void worldview_shutdown(void);
void worldview_load(const Zone *z, const RegionDef *rd);   /* builds the terrain mesh (idempotent per zone) */
void worldview_unload(void);
/* Queue everything near `focus`. `legacy` skips terrain/props (Aurelheim's hand-built sandbox draws itself) but still draws portals. */
void worldview_submit(const Zone *z, const RegionDef *rd, Vector3 focus, float t, QualityTier tier, bool legacy);
/* Boss hazard rings/discs (telegraphs pulse, active zones glow). */
void worldview_hazards(const BossRt *rt, float t);
int  worldview_last_props(void);                   /* props queued last frame (budget diagnostics) */

#endif
