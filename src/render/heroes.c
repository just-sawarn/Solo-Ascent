#include "render/heroes.h"
#include <math.h>
#include <stdlib.h>
#include <raymath.h>
#include "render/character.h"
#include "render/procmodels.h"

static Color C(int r, int g, int b) { return (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, 255}; }

static RenderMat cloth(Color c, float spec) { return (RenderMat){.base = c, .spec = spec, .rim = 1.0f, .smooth = 0.0f}; }

/* all parts below are authored in the body's bind (T) pose coordinates: y up, +z forward, character's left = +x */
static void part_on(const Pose *p, int bone, ProcMesh m, Matrix bind_xf, RenderMat mat, Matrix root) {
    renderer_submit(proc_mesh(m), MatrixMultiply(pose_attach(p, bone, bind_xf), root), mat);
}
static Matrix scl(float sx, float sy, float sz, float rx, float ry, float rz, float x, float y, float z) {
    Matrix m = MatrixMultiply(MatrixScale(sx, sy, sz), MatrixRotateXYZ((Vector3){rx, ry, rz}));
    return MatrixMultiply(m, MatrixTranslate(x, y, z));
}
static void part_between(ProcMesh m, Vector3 a, Vector3 b, float rx, float rz, RenderMat mat, Matrix root) {
    renderer_submit(proc_mesh(m), MatrixMultiply(proc_limb(a, b, rx, rz), root), mat);
}
static Vector3 tr(Vector3 v, Matrix m) { return Vector3Transform(v, m); }

void heroes_init(void) { characters_init(); }

void heroes_submit_kael(Vector3 pos, float yaw, float t) {
    if (!characters_ready()) return;
    const BodySex S = SEX_MALE;
    float sway = sinf(t * 1.3f) * 2.0f;
    BoneRot rots[] = {
        /* relaxed stride, weight on the left leg */
        {"thigh_l", {10, 0, 0}}, {"calf_l", {-6, 0, 0}}, {"thigh_r", {-8, 0, 0}}, {"calf_r", {10, 0, 0}},
        {"spine_02", {0, 3, 0}}, {"Head", {2 + sway * 0.3f, -4, 0}},
        /* his right arm (-x): lowered and slightly forward, holding the blade; left arm (+x) down, hand in pocket */
        {"upperarm_r", {0, 0, 68}}, {"lowerarm_r", {-30, 0, 0}},
        {"upperarm_l", {0, 0, -70}}, {"lowerarm_l", {-8, 0, 0}},
        {"clavicle_r", {0, 0, 4}}, {"clavicle_l", {0, 0, -4}},
    };
    Pose pose;
    character_pose(S, rots, (int)(sizeof rots / sizeof rots[0]), &pose);
    Matrix root = MatrixMultiply(MatrixMultiply(MatrixScale(0.86f, 1.0f, 0.90f), MatrixRotateY(yaw)), MatrixTranslate(pos.x, pos.y, pos.z));

    CharStyle st = {
        .sex = S, .light_skin = true, .skin_tint = C(252, 224, 204), .hair_color = C(22, 24, 38), .hair = HAIRSTYLE_PARTED,
        .eye_color = C(28, 36, 92),
        .top = C(236, 241, 250), .bottom = C(40, 34, 52), .shoes = C(98, 114, 152),
        .sleeve_reach = 0.0f, .hem_y = 0.085f, .shoe_h = 0.10f, .waist_y = 0.95f,
    };
    character_submit(&st, &pose, root);

    /* ---- long hooded coat (overlay parts riding on the skeleton) ---- */
    Color navy = C(52, 58, 106), navy_lt = C(74, 82, 138);
    RenderMat cm = cloth(navy, 0.4f), cl = cloth(navy_lt, 0.4f);
    int sp2 = character_bone(S, "spine_02"), sp3 = character_bone(S, "spine_03"), pel = character_bone(S, "pelvis");
    int neck = character_bone(S, "neck_01");
    int ua_l = character_bone(S, "upperarm_l"), la_l = character_bone(S, "lowerarm_l");
    int ua_r = character_bone(S, "upperarm_r"), la_r = character_bone(S, "lowerarm_r"), ha_r = character_bone(S, "hand_r");

    /* back panel, front lapels and side panels hang from the pelvis so the hem follows the hips */
    part_on(&pose, pel, PM_CONE, scl(0.25f, 1.12f, 0.030f, 0, 0, 0, 0, 0.36f, -0.11f), cm, root);
    for (int sd = -1; sd <= 1; sd += 2) {
        float f = (float)sd;
        part_on(&pose, pel, PM_CONE, scl(0.100f, 1.06f, 0.026f, 0, 0, 0.05f * f, 0.185f * f, 0.38f, 0.115f), cm, root);
        part_on(&pose, pel, PM_CONE, scl(0.030f, 1.00f, 0.15f, 0, 0, -0.05f * f, 0.215f * f, 0.40f, -0.01f), cm, root);
    }
    /* collar + hood bunch behind the neck, hood peak drooping to his left shoulder */
    part_on(&pose, sp3, PM_TORUS, scl(0.17f, 0.17f, 0.17f, 0, 0, 0, 0, 1.50f, -0.015f), cl, root);
    part_on(&pose, neck, PM_SPHERE, scl(0.125f, 0.095f, 0.095f, 0.5f, 0, 0, 0, 1.56f, -0.10f), cl, root);
    part_on(&pose, neck, PM_CONE, scl(0.085f, 0.20f, 0.05f, 0.2f, 0, -1.2f, 0.03f, 1.58f, -0.12f), cl, root);
    /* sneakers: upper + white sole on each foot bone (authored in the bind pose) */
    for (int sd = -1; sd <= 1; sd += 2) {
        int foot = character_bone(S, sd > 0 ? "foot_l" : "foot_r");
        float x = 0.105f * (float)sd;
        part_on(&pose, foot, PM_SPHERE, scl(0.062f, 0.052f, 0.135f, 0, 0, 0, x, 0.065f, 0.045f), cloth(C(98, 114, 152), 0.6f), root);
        part_on(&pose, foot, PM_CUBE, scl(0.118f, 0.026f, 0.27f, 0, 0, 0, x, 0.016f, 0.045f), cloth(C(236, 238, 244), 0.5f), root);
    }
    /* sleeves: cylinders between posed shoulder / elbow, rounded shoulder caps, rolled cuff at the elbow */
    Vector3 ps_l = tr(pose_bone_pos(&pose, ua_l), root), pe_l = tr(pose_bone_pos(&pose, la_l), root);
    Vector3 ps_r = tr(pose_bone_pos(&pose, ua_r), root), pe_r = tr(pose_bone_pos(&pose, la_r), root);
    Vector3 pw_l = tr(pose_bone_pos(&pose, character_bone(S, "hand_l")), root);
    Vector3 pw_r = tr(pose_bone_pos(&pose, ha_r), root);
    struct { Vector3 s, e; } arms[2] = {{ps_l, pe_l}, {ps_r, pe_r}};
    for (int k = 0; k < 2; ++k) {
        part_between(PM_CYL, arms[k].s, arms[k].e, 0.066f, 0.066f, cm, MatrixIdentity());
        renderer_submit(proc_mesh(PM_SPHERE), MatrixMultiply(MatrixScale(0.082f, 0.082f, 0.082f), MatrixTranslate(arms[k].s.x, arms[k].s.y, arms[k].s.z)), cm);
        Vector3 d = Vector3Normalize(Vector3Subtract(arms[k].e, arms[k].s));
        part_between(PM_CYL, Vector3Subtract(arms[k].e, Vector3Scale(d, 0.035f)), Vector3Add(arms[k].e, Vector3Scale(d, 0.035f)), 0.074f, 0.074f, cl, MatrixIdentity());
    }
    (void)pw_l;
    /* Dawnbrand (Lv1): CC0 short sword, reverse grip, blade pointing down along the forearm direction */
    Vector3 fd = Vector3Normalize(Vector3Subtract(pw_r, pe_r));
    Vector3 blade_dir = Vector3Normalize(Vector3Add(fd, (Vector3){-0.0f, -0.5f, 0.25f}));
    Vector3 g0 = Vector3Add(pw_r, Vector3Scale(fd, 0.015f));
    Matrix wm = proc_limb(g0, Vector3Add(g0, blade_dir), 1.0f, 1.0f);
    proc_submit_weapon(WEAPON_SHORT_SWORD, wm, 0.82f);
    (void)sp2;
}
