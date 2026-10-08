#include "render/character.h"
#include <string.h>
#include <raymath.h>
#include "core/log.h"
#include "render/anim.h"

typedef struct {
    Model model;
    bool ok;
    int body_mesh, brow_mesh, eye_mesh;
    Matrix bind[CHAR_MAX_BONES];
    Matrix bind_inv[CHAR_MAX_BONES];
    Texture2D skin_dark, skin_light;
} Body;

static Body g_body[SEX_COUNT];
static Model g_hair[HAIRSTYLE_COUNT];
static bool g_hair_ok[HAIRSTYLE_COUNT];
static Texture2D g_hair_tex;
static bool g_ready;

static Matrix trs(Transform t) {
    return MatrixMultiply(MatrixMultiply(MatrixScale(t.scale.x, t.scale.y, t.scale.z), QuaternionToMatrix(t.rotation)),
                          MatrixTranslate(t.translation.x, t.translation.y, t.translation.z));
}

static Texture2D load_tex(const char *path) {
    Texture2D t = LoadTexture(path);
    if (t.id) { GenTextureMipmaps(&t); SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR); }
    else LOGE("texture %s failed to load", path);
    return t;
}

static void load_body(BodySex sex, const char *gltf, const char *dark, const char *light) {
    Body *b = &g_body[sex];
    b->model = LoadModel(gltf);
    if (b->model.meshCount < 3 || b->model.boneCount <= 0 || b->model.boneCount > CHAR_MAX_BONES) {
        LOGE("character %s: unexpected rig (%d meshes, %d bones)", gltf, b->model.meshCount, b->model.boneCount);
        return;
    }
    int biggest = 0;
    for (int i = 0; i < b->model.meshCount; ++i) {
        if (b->model.meshes[i].vertexCount > b->model.meshes[biggest].vertexCount) biggest = i;
        int mat = b->model.meshMaterial[i];
        if (mat == 1) b->brow_mesh = i; /* raylib reserves material 0 for its default */
        if (mat == 2) b->eye_mesh = i;
    }
    b->body_mesh = biggest;
    for (int i = 0; i < b->model.boneCount; ++i) {
        b->bind[i] = trs(b->model.bindPose[i]);
        b->bind_inv[i] = MatrixInvert(b->bind[i]);
    }
    b->skin_dark = load_tex(dark);
    b->skin_light = load_tex(light);
    b->ok = true;
}

void characters_init(void) {
    load_body(SEX_MALE, "assets/characters/body_male.gltf", "assets/characters/skin_male_dark.png", "assets/characters/skin_male_light.png");
    load_body(SEX_FEMALE, "assets/characters/body_female.gltf", "assets/characters/skin_female_dark.png", "assets/characters/skin_female_light.png");
    static const char *HAIR_FILES[HAIRSTYLE_COUNT] = {"hair_simpleparted", "hair_buzzed", "hair_long", NULL};
    for (int i = 0; i < HAIRSTYLE_COUNT; ++i) {
        if (!HAIR_FILES[i]) continue;
        g_hair[i] = LoadModel(TextFormat("assets/characters/%s.gltf", HAIR_FILES[i]));
        g_hair_ok[i] = g_hair[i].meshCount > 0;
        if (!g_hair_ok[i]) LOGE("hair %s failed to load", HAIR_FILES[i]);
    }
    g_hair_tex = load_tex("assets/characters/hair1.png");
    g_ready = g_body[SEX_MALE].ok && g_body[SEX_FEMALE].ok;
}

void characters_shutdown(void) {
    for (int s = 0; s < SEX_COUNT; ++s) {
        if (!g_body[s].ok) continue;
        UnloadModel(g_body[s].model);
        UnloadTexture(g_body[s].skin_dark); UnloadTexture(g_body[s].skin_light);
    }
    for (int i = 0; i < HAIRSTYLE_COUNT; ++i) if (g_hair_ok[i]) UnloadModel(g_hair[i]);
    UnloadTexture(g_hair_tex);
    g_ready = false;
}

bool characters_ready(void) { return g_ready; }

int character_bone(BodySex sex, const char *name) {
    const Body *b = &g_body[sex];
    for (int i = 0; i < b->model.boneCount; ++i) if (strcmp(b->model.bones[i].name, name) == 0) return i;
    return -1;
}

/* posed_i = R_i * local_i * posed_parent (row-vector convention, as raymath) */
static void eval_bone(const Body *b, int i, const Matrix *rot, const Matrix *locals, bool *done, Matrix *posed) {
    if (done[i]) return;
    int par = b->model.bones[i].parent;
    Matrix m = MatrixMultiply(rot[i], locals[i]);
    if (par >= 0) {
        eval_bone(b, par, rot, locals, done, posed);
        m = MatrixMultiply(m, posed[par]);
    }
    posed[i] = m;
    done[i] = true;
}

void character_pose(BodySex sex, const BoneRot *rots, int n, Pose *out) {
    const Body *b = &g_body[sex];
    Matrix rot[CHAR_MAX_BONES];
    bool done[CHAR_MAX_BONES] = {0};
    int count = b->model.boneCount;
    for (int i = 0; i < count; ++i) rot[i] = MatrixIdentity();
    for (int k = 0; k < n; ++k) {
        int i = character_bone(sex, rots[k].bone);
        if (i < 0) continue;
        Vector3 o = {b->bind[i].m12, b->bind[i].m13, b->bind[i].m14};
        Matrix dw = MatrixMultiply(MatrixMultiply(MatrixTranslate(-o.x, -o.y, -o.z),
                    MatrixRotateXYZ((Vector3){rots[k].deg.x * DEG2RAD, rots[k].deg.y * DEG2RAD, rots[k].deg.z * DEG2RAD})),
                    MatrixTranslate(o.x, o.y, o.z));
        rot[i] = MatrixMultiply(MatrixMultiply(b->bind[i], dw), b->bind_inv[i]);
    }
    Matrix locals[CHAR_MAX_BONES];
    for (int i = 0; i < count; ++i) {
        int par = b->model.bones[i].parent;
        locals[i] = par >= 0 ? MatrixMultiply(b->bind[i], b->bind_inv[par]) : b->bind[i];
    }
    out->count = count;
    for (int i = 0; i < count; ++i) eval_bone(b, i, rot, locals, done, out->posed);
    for (int i = 0; i < count; ++i) out->skin[i] = MatrixMultiply(b->bind_inv[i], out->posed[i]);
}

static Matrix extra_rot(const Body *b, int i, Vector3 deg) {
    Vector3 o = {b->bind[i].m12, b->bind[i].m13, b->bind[i].m14};
    Matrix dw = MatrixMultiply(MatrixMultiply(MatrixTranslate(-o.x, -o.y, -o.z),
                MatrixRotateXYZ((Vector3){deg.x * DEG2RAD, deg.y * DEG2RAD, deg.z * DEG2RAD})), MatrixTranslate(o.x, o.y, o.z));
    return MatrixMultiply(MatrixMultiply(b->bind[i], dw), b->bind_inv[i]);
}

void character_pose_anim(BodySex sex, const AnimPlayer *player, const BoneRot *extra, int n, Pose *out) {
    const Body *b = &g_body[sex];
    int count = b->model.boneCount;
    Matrix rot[CHAR_MAX_BONES], locals[CHAR_MAX_BONES];
    bool done[CHAR_MAX_BONES] = {0};
    for (int i = 0; i < count; ++i) rot[i] = MatrixIdentity();
    for (int i = 0; i < count; ++i) {
        int par = b->model.bones[i].parent;
        Matrix bind_local = par >= 0 ? MatrixMultiply(b->bind[i], b->bind_inv[par]) : b->bind[i];
        int ab = anim_clip_bone(b->model.bones[i].name);
        if (ab < 0 || player->clip[0] < 0) { locals[i] = bind_local; continue; }
        Quaternion q = anim_sample_rot(player, ab);
        Vector3 t = {bind_local.m12, bind_local.m13, bind_local.m14};
        anim_sample_trans(player, ab, &t);
        locals[i] = MatrixMultiply(QuaternionToMatrix(q), MatrixTranslate(t.x, t.y, t.z));
    }
    for (int k = 0; k < n; ++k) {
        int i = character_bone(sex, extra[k].bone);
        if (i >= 0) rot[i] = extra_rot(b, i, extra[k].deg);
    }
    out->count = count;
    for (int i = 0; i < count; ++i) eval_bone(b, i, rot, locals, done, out->posed);
    for (int i = 0; i < count; ++i) out->skin[i] = MatrixMultiply(b->bind_inv[i], out->posed[i]);
}

Matrix pose_bind(BodySex sex, int bone) { return g_body[sex].bind[bone]; }

Vector3 pose_bone_pos(const Pose *p, int bone) { return (Vector3){p->posed[bone].m12, p->posed[bone].m13, p->posed[bone].m14}; }

Matrix pose_attach(const Pose *p, int bone, Matrix bind_space_xf) { return MatrixMultiply(bind_space_xf, p->skin[bone]); }

void character_submit(const CharStyle *st, const Pose *pose, Matrix root) {
    if (!g_ready) return;
    Body *b = &g_body[st->sex];
    Matrix *bones = renderer_alloc_bones(pose->count);
    if (!bones) return;
    memcpy(bones, pose->skin, sizeof(Matrix) * (size_t)pose->count);
#ifdef __EMSCRIPTEN__
    /* raylib bug: rlSetUniformMatrices() cannot transpose on WebGL (it passes `false`), but desktop GL transposes.
     * Compensate so the same pose math works on both. */
    for (int i = 0; i < pose->count; ++i) bones[i] = MatrixTranspose(bones[i]);
#endif

    RenderMat body = {.base = st->skin_tint, .spec = 0.35f, .rim = 1.0f, .smooth = 0.0f, .flags = RM_PAINT,
                      .tex = st->light_skin ? b->skin_light : b->skin_dark,
                      .paint = {st->top, st->bottom, st->shoes},
                      .paint_cfg = {st->sleeve_reach, st->hem_y, st->shoe_h, st->waist_y}};
    renderer_submit_skinned(&b->model.meshes[b->body_mesh], root, body, bones, pose->count);

    RenderMat brow = {.base = st->hair_color, .spec = 0.0f, .rim = 0.6f, .flags = RM_NO_OUTLINE | RM_NO_SHADOW_CAST, .tex = g_hair_tex};
    renderer_submit_skinned(&b->model.meshes[b->brow_mesh], root, brow, bones, pose->count);
    RenderMat eye = {.base = st->eye_color, .spec = 0.0f, .rim = 0.0f, .emissive = 0.15f, .flags = RM_NO_OUTLINE | RM_NO_SHADOW_CAST, .tex = {0}};
    renderer_submit_skinned(&b->model.meshes[b->eye_mesh], root, eye, bones, pose->count);

    if (st->hair != HAIRSTYLE_NONE && g_hair_ok[st->hair]) {
        int head = character_bone(st->sex, "Head");
        Matrix hx = MatrixMultiply(pose->skin[head], root); /* rigid on the head bone */
        RenderMat hm = {.base = st->hair_color, .spec = 0.5f, .rim = 1.0f, .flags = RM_HAIR, .tex = g_hair_tex};
        for (int i = 0; i < g_hair[st->hair].meshCount; ++i) renderer_submit(&g_hair[st->hair].meshes[i], hx, hm);
    }
}
