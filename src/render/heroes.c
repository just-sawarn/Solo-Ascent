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

typedef struct {
    AnimPlayer anim;
    int idle, walk, jog, sprint, attack, roll, jump_start, jump_loop, jump_land;
    bool was_air;
    float land_t;
    bool ready;
} KaelAnim;
static KaelAnim K;

void heroes_init(void) {
    characters_init();
    if (!anims_load("assets/animations/ual.sanim")) return;
    K.idle = anim_find("Idle_Loop"); K.walk = anim_find("Walk_Loop"); K.jog = anim_find("Jog_Fwd_Loop");
    K.sprint = anim_find("Sprint_Loop"); K.attack = anim_find("Sword_Attack"); K.roll = anim_find("Roll");
    K.jump_start = anim_find("Jump_Start"); K.jump_loop = anim_find("Jump_Loop"); K.jump_land = anim_find("Jump_Land");
    anim_player_init(&K.anim, K.idle);
    K.ready = K.idle >= 0;
}

/* Fit a one-shot clip into the gameplay duration so animation and hit timing agree. */
static void play_fit(int clip, float fade, float gameplay_seconds) {
    anim_play(&K.anim, clip, fade, true);
    float len = anim_clip_length(clip);
    K.anim.speed = (len > 0.01f && gameplay_seconds > 0.01f) ? len / gameplay_seconds : 1.0f;
}

void heroes_update_kael(float dt, const KaelMotion *m) {
    if (!K.ready) return;
    if (m->ev_roll && K.roll >= 0) play_fit(K.roll, 0.06f, m->roll_seconds);
    else if (m->ev_attack && K.attack >= 0) play_fit(K.attack, 0.06f, m->attack_seconds);
    else if (m->rolling || m->attacking) { /* keep playing the one-shot */ }
    else if (!m->grounded) {
        if (!K.was_air) { anim_play(&K.anim, K.jump_start, 0.08f, true); K.anim.speed = 1.0f; }
        else if (K.anim.clip[0] == K.jump_start && K.anim.finished) anim_play(&K.anim, K.jump_loop, 0.1f, true);
        K.was_air = true;
    } else {
        if (K.was_air) { K.was_air = false; K.land_t = m->speed > 3.0f ? 0.0f : 0.25f; if (K.land_t > 0) { anim_play(&K.anim, K.jump_land, 0.05f, true); K.anim.speed = 1.0f; } }
        if (K.land_t > 0) K.land_t -= dt;
        else {
            int c = m->speed < 0.2f ? K.idle : m->speed < 3.2f ? K.walk : m->speed < 6.2f ? K.jog : K.sprint;
            anim_play(&K.anim, c, 0.18f, false);
            K.anim.speed = 1.0f;
        }
    }
    if (!m->grounded) K.was_air = true;
    anim_update(&K.anim, dt);
}

void heroes_submit_kael(Vector3 pos, float yaw, float t) {
    if (!characters_ready()) return;
    const BodySex S = SEX_MALE;
    (void)t;
    Pose pose;
    if (K.ready) character_pose_anim(S, &K.anim, NULL, 0, &pose);
    else character_pose(S, NULL, 0, &pose);
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
    (void)pw_l; (void)pw_r;
    /* Dawnbrand (Lv1): CC0 short sword held in the right fist; blade forward in the rig's bind pose */
    Matrix hb = pose_bind(S, ha_r);
    Vector3 g0 = {hb.m12 - 0.01f, hb.m13, hb.m14};
    Matrix sw_bind = proc_limb(g0, (Vector3){g0.x - 0.15f, g0.y + 0.0f, g0.z + 0.95f}, 1.0f, 1.0f);
    proc_submit_weapon(WEAPON_SHORT_SWORD, MatrixMultiply(pose_attach(&pose, ha_r, sw_bind), root), 0.82f);
    (void)sp2;
}
