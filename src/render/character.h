#ifndef SA_CHARACTER_H
#define SA_CHARACTER_H
/* CC0 rigged base bodies (Quaternius Universal Base Characters) + procedural palette-swap styling.
 * Pose = list of world-axis bone rotations (degrees, about the bone's bind origin); evaluated on the CPU
 * into skinning matrices uploaded as uniforms (65 bones; WebGL2 vertex-uniform budget is the limit). */
#include <stdbool.h>
#include <raylib.h>
#include "render/renderer.h"
#include "render/anim.h"

#define CHAR_MAX_BONES 72

typedef enum { SEX_MALE, SEX_FEMALE, SEX_COUNT } BodySex;
typedef enum { HAIRSTYLE_PARTED, HAIRSTYLE_BUZZED, HAIRSTYLE_LONG, HAIRSTYLE_NONE, HAIRSTYLE_COUNT } HairModel;

typedef struct { const char *bone; Vector3 deg; } BoneRot;

typedef struct {
    int count;
    Matrix posed[CHAR_MAX_BONES]; /* model-space bone transforms */
    Matrix skin[CHAR_MAX_BONES];  /* inverse(bind) * posed: what the skinning shader needs */
} Pose;

typedef struct {
    BodySex sex;
    bool light_skin;
    Color skin_tint;     /* multiplies the skin texture */
    Color hair_color;
    HairModel hair;
    Color eye_color;
    /* outfit paint, see RM_PAINT */
    Color top, bottom, shoes;
    float sleeve_reach, hem_y, shoe_h, waist_y;
} CharStyle;

void  characters_init(void);
void  characters_shutdown(void);
bool  characters_ready(void);
int   character_bone(BodySex sex, const char *name); /* -1 if unknown */
void  character_pose(BodySex sex, const BoneRot *rots, int n, Pose *out);
/* Pose from a playing animation; `extra` bone rotations (world axes, degrees) are layered on top. */
void  character_pose_anim(BodySex sex, const AnimPlayer *player, const BoneRot *extra, int n, Pose *out);
Matrix pose_bind(BodySex sex, int bone);   /* bind-pose (T) world matrix of a bone */
Vector3 pose_bone_pos(const Pose *p, int bone);       /* posed origin, model space */
Matrix  pose_attach(const Pose *p, int bone, Matrix bind_space_xf); /* place a part authored in the bind (T) pose on a bone */
/* Queue body + brows + eyes + hair. `root` maps model space to world. */
void  character_submit(const CharStyle *style, const Pose *pose, Matrix root);

#endif
