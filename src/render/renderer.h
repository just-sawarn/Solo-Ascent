#ifndef SA_RENDERER_H
#define SA_RENDERER_H
/*
 * Toon renderer. Frame flow:
 *   renderer_begin_frame(cam, t)  -> renderer_submit(...)*  -> [BeginDrawing()] renderer_end_frame() [UI] [EndDrawing()]
 * Passes: shadow map (packed RGBA8 depth, PCF) -> sky + toon lit pass -> inverted-hull outlines
 *         -> bloom (bright/blur) -> composite (FXAA-lite, ACES, grade, vignette) to the screen.
 */
#include <stdbool.h>
#include <stdint.h>
#include <raylib.h>

typedef enum { TIER_ULTRA, TIER_HIGH, TIER_MEDIUM, TIER_LOW, TIER_MOBILE, TIER_COUNT } QualityTier;

typedef struct {
    const char *name;
    int   shadow_res;     /* square shadow map */
    int   pcf_radius;     /* 0 = hard, 1 = 3x3, 2 = 5x5 */
    float render_scale;   /* internal resolution multiplier */
    bool  outlines;
    bool  fxaa;
    int   bloom_passes;   /* blur iterations; 0 disables bloom */
    int   bloom_div;      /* bloom buffer = scene / div */
    bool  shadows;
} TierSettings;

enum { RM_NO_SHADOW_CAST = 1, RM_NO_OUTLINE = 2, RM_GROUND = 4, RM_HAIR = 8, RM_NO_RECEIVE = 16, RM_PAINT = 32 };

typedef struct {
    Color base;
    float spec;      /* multiplier on the look's specular */
    float rim;       /* multiplier on the look's rim light */
    float emissive;  /* 0..1: unlit glow, feeds bloom */
    float smooth;    /* outline direction blend toward radial (0 for smooth meshes, ~0.8 for cubes) */
    uint8_t flags;
    Texture2D tex;   /* optional albedo (palette) texture; id 0 = none */
    /* RM_PAINT: outfit colours painted by bind-space body region (top, bottom, shoes) */
    Color paint[3];
    float paint_cfg[4]; /* sleeve reach |x|, pants hem y, shoe height y, waist y */
} RenderMat;

#define LOOK_COUNT 3

void  renderer_init(int width, int height, QualityTier tier, int look);
void  renderer_shutdown(void);
void  renderer_set_tier(QualityTier t);
QualityTier renderer_tier(void);
const TierSettings *renderer_tier_settings(QualityTier t);
void  renderer_set_look(int look);
int   renderer_look(void);
const char *renderer_look_name(int look);
/* internal render size after scale */
void  renderer_target_size(int *w, int *h);

void  renderer_begin_frame(Camera3D cam, float time_s);
void  renderer_submit(const Mesh *mesh, Matrix xf, RenderMat mat);
/* Skinned mesh: `bones` must stay valid until end_frame (use renderer_alloc_bones). */
Matrix *renderer_alloc_bones(int count);
void  renderer_submit_skinned(const Mesh *mesh, Matrix xf, RenderMat mat, const Matrix *bones, int bone_count);
void  renderer_end_frame(void); /* call inside BeginDrawing(); draws the final image to the screen */
int   renderer_queue_count(void);

/* Quality governor: first call each frame. Benchmarks the first frames then stays adaptive. */
void  renderer_governor_update(float frame_dt, bool enabled);
bool  renderer_governor_settled(void);

#endif
