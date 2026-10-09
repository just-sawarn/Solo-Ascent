#ifndef SA_VFX_H
#define SA_VFX_H
/*
 * Billboard particle batch. Weather and ambient motes are stateless (position = f(index, time) wrapped around the camera),
 * bursts (hit sparks, skill effects, pickups) use a small CPU pool. Everything is one dynamic mesh with vertex colours,
 * drawn emissive and unlit-ish so it blooms; cost scales with the quality tier.
 */
#include <raylib.h>
#include "core/daynight.h"
#include "render/renderer.h"

void vfx_init(void);
void vfx_shutdown(void);
void vfx_reset(void);                                    /* drop live bursts (zone change) */
/* Call once per frame after renderer_begin_frame: builds this frame's particles and queues the batch. */
void vfx_frame(Camera3D cam, float t, float dt, QualityTier tier, WeatherKind wx, float wx_intensity, int biome, float hour, bool indoor);

typedef enum { VFX_SPARK, VFX_HIT, VFX_SLASH, VFX_HEAL, VFX_DUST, VFX_PICKUP, VFX_AURA, VFX_EMBER, VFX_COUNT } VfxKind;
void vfx_burst(VfxKind k, Vector3 pos, Vector3 dir);   /* dir: preferred direction (zero = omni) */
int  vfx_live(void);                                    /* burst particles alive (tests/HUD) */
int  vfx_last_count(void);                              /* quads submitted last frame */

#endif
