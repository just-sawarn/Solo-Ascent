#ifndef SA_ANIM_H
#define SA_ANIM_H
/* Clip playback for the shared 65-bone humanoid rig (Quaternius UAL clips converted by tools/prep_animations.py).
 * Clips are sampled at 30 fps with linear quaternion interpolation; two clips can crossfade. */
#include <stdbool.h>
#include <raylib.h>

#define ANIM_MAX_BONES 72

bool  anims_load(const char *path);
void  anims_unload(void);
int   anim_find(const char *name);        /* -1 if missing */
float anim_clip_length(int clip);         /* seconds */
bool  anim_clip_loops(int clip);
int   anim_clip_bone(const char *name);   /* bone index in the clip skeleton, -1 if unknown */
const char *anim_bone_name(int bone);
int   anim_bone_count(void);

typedef struct {
    int   clip[2];   /* [0] current, [1] previous (fading out) */
    float time[2];
    float fade;      /* weight of the current clip: 0..1 */
    float fade_rate; /* per second */
    float speed;
    bool  finished;  /* one-shot reached its last frame */
} AnimPlayer;

void anim_player_init(AnimPlayer *p, int clip);
/* Switch clip with a crossfade. No-op if already playing it (unless restart). */
void anim_play(AnimPlayer *p, int clip, float fade_s, bool restart);
void anim_update(AnimPlayer *p, float dt);
/* Blended local rotation (and translation, if the clip animates it) of one bone. */
Quaternion anim_sample_rot(const AnimPlayer *p, int bone);
bool  anim_sample_trans(const AnimPlayer *p, int bone, Vector3 *out);

#endif
