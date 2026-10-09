#include "render/heroes.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <raymath.h>
#include "render/character.h"
#include "render/procmodels.h"

static Color C(int r, int g, int b) { return (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, 255}; }
static RenderMat cloth(Color c, float spec) { return (RenderMat){.base = c, .spec = spec, .rim = 1.0f, .smooth = 0.0f}; }
static RenderMat glow(Color c, float e) { return (RenderMat){.base = c, .spec = 0, .rim = 0, .emissive = e, .smooth = 0, .flags = RM_NO_OUTLINE | RM_NO_SHADOW_CAST}; }

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
    bool was_air, was_staggered;
    float flash, land_t;
} MemberAnim;
static MemberAnim MA[4];
static int c_idle = -1, c_walk, c_jog, c_sprint, c_roll, c_jump_start, c_jump_loop, c_jump_land, c_hit, c_death, c_guard;
static bool g_ready;
static int g_weapon[4] = {WEAPON_SHORT_SWORD, WEAPON_LONG_SWORD, -1, -1};
static const float WEAPON_LEN[WEAPON_COUNT] = {0.55f, 0.82f, 1.0f, 1.05f, 0.95f, 0.8f, 1.5f};

void heroes_set_weapon(int member, const char *model) {
    static const char *names[WEAPON_COUNT] = {"dagger", "short_sword", "long_sword", "katana", "axe", "club", "spear"};
    if (member < 0 || member > 3 || !model) return;
    for (int i = 0; i < WEAPON_COUNT; ++i) if (!strcmp(names[i], model)) { g_weapon[member] = i; return; }
}

int heroes_clip(const char *name) { return anim_find(name); }

void heroes_init(void) {
    characters_init();
    if (!anims_load("assets/animations/ual.sanim")) return;
    c_idle = anim_find("Idle_Loop"); c_walk = anim_find("Walk_Loop"); c_jog = anim_find("Jog_Fwd_Loop");
    c_sprint = anim_find("Sprint_Loop"); c_roll = anim_find("Roll");
    c_hit = anim_find("Hit_Chest"); c_death = anim_find("Death01"); c_guard = anim_find("Sword_Idle");
    c_jump_start = anim_find("Jump_Start"); c_jump_loop = anim_find("Jump_Loop"); c_jump_land = anim_find("Jump_Land");
    for (int i = 0; i < 4; ++i) anim_player_init(&MA[i].anim, c_idle);
    g_ready = c_idle >= 0;
}

/* Fit a one-shot clip into the gameplay duration so animation and hit timing agree. */
static void play_fit(MemberAnim *k, int clip, float fade, float gameplay_seconds) {
    anim_play(&k->anim, clip, fade, true);
    float len = anim_clip_length(clip);
    k->anim.speed = (len > 0.01f && gameplay_seconds > 0.01f) ? len / gameplay_seconds : 1.0f;
}

void heroes_update(int member, float dt, const MemberMotion *m) {
    if (!g_ready || member < 0 || member > 3) return;
    MemberAnim *k = &MA[member];
    k->flash = m->flash;
    if (m->dead) {
        if (k->anim.clip[0] != c_death) { anim_play(&k->anim, c_death, 0.08f, true); k->anim.speed = 1.0f; }
        if (!k->anim.finished) anim_update(&k->anim, dt);
        return;
    }
    if (m->staggered) {
        if (!k->was_staggered) { anim_play(&k->anim, c_hit, 0.04f, true); k->anim.speed = 1.3f; }
        k->was_staggered = true; anim_update(&k->anim, dt); return;
    }
    k->was_staggered = false;
    if (m->guarding && !m->casting && !m->rolling && m->grounded && c_guard >= 0) {
        anim_play(&k->anim, c_guard, 0.1f, false); k->anim.speed = 1.0f; anim_update(&k->anim, dt); return;
    }
    if (m->ev_roll && c_roll >= 0) play_fit(k, c_roll, 0.06f, m->roll_seconds);
    else if (m->ev_cast && m->cast_clip >= 0) play_fit(k, m->cast_clip, 0.06f, m->cast_seconds);
    else if (m->rolling || m->casting) { /* keep playing the one-shot */ }
    else if (!m->grounded) {
        if (!k->was_air) { anim_play(&k->anim, c_jump_start, 0.08f, true); k->anim.speed = 1.0f; }
        else if (k->anim.clip[0] == c_jump_start && k->anim.finished) anim_play(&k->anim, c_jump_loop, 0.1f, true);
        k->was_air = true;
    } else {
        if (k->was_air) { k->was_air = false; k->land_t = m->speed > 3.0f ? 0.0f : 0.25f; if (k->land_t > 0) { anim_play(&k->anim, c_jump_land, 0.05f, true); k->anim.speed = 1.0f; } }
        if (k->land_t > 0) k->land_t -= dt;
        else {
            int c = m->speed < 0.2f ? c_idle : m->speed < 3.2f ? c_walk : m->speed < 6.2f ? c_jog : c_sprint;
            anim_play(&k->anim, c, 0.18f, false);
            k->anim.speed = 1.0f;
        }
    }
    if (!m->grounded) k->was_air = true;
    anim_update(&k->anim, dt);
}

/* grip direction (in bind space) that points straight up when the right arm hangs in a relaxed pose */
static Vector3 staff_dir(void) { return Vector3Normalize((Vector3){0.94f, 0.34f, 0.0f}); }

static void submit_staff(const Pose *pose, int hand, Color wood, Color orb, Color trim, Matrix root, float len) {
    Matrix hb = pose_bind(SEX_FEMALE, hand);
    Vector3 d = staff_dir(), h = {hb.m12, hb.m13, hb.m14};
    Vector3 a = Vector3Subtract(h, Vector3Scale(d, len * 0.30f)), b = Vector3Add(h, Vector3Scale(d, len * 0.70f));
    part_on(pose, hand, PM_CYL, proc_limb(a, b, 0.018f, 0.018f), cloth(wood, 0.5f), root);
    part_on(pose, hand, PM_SPHERE, scl(0.065f, 0.065f, 0.065f, 0, 0, 0, b.x, b.y, b.z), glow(orb, 1.0f), root);
    part_on(pose, hand, PM_TORUS, scl(0.09f, 0.09f, 0.09f, 0, 0, 1.2f, b.x, b.y, b.z), glow(trim, 0.35f), root);
}


static void submit_kael(const MemberAnim *K, Vector3 pos, float yaw) {
    const BodySex S = SEX_MALE;
    Pose pose;
    character_pose_anim(S, &K->anim, NULL, 0, &pose);
    Matrix root = MatrixMultiply(MatrixMultiply(MatrixScale(0.86f, 1.0f, 0.90f), MatrixRotateY(yaw)), MatrixTranslate(pos.x, pos.y, pos.z));

    CharStyle st = {
        .sex = S, .light_skin = true, .skin_tint = C(252, 224, 204), .hair_color = C(22, 24, 38), .hair = HAIRSTYLE_PARTED,
        .eye_color = C(28, 36, 92), .flash = K->flash,
        .top = C(236, 241, 250), .bottom = C(40, 34, 52), .shoes = C(98, 114, 152),
        .sleeve_reach = 0.0f, .hem_y = 0.085f, .shoe_h = 0.10f, .waist_y = 0.95f,
    };
    character_submit(&st, &pose, root);

    /* ---- long hooded coat (overlay parts riding on the skeleton) ---- */
    Color navy = C(52, 58, 106), navy_lt = C(74, 82, 138);
    RenderMat cm = cloth(navy, 0.4f), cl = cloth(navy_lt, 0.4f);
    int sp3 = character_bone(S, "spine_03"), pel = character_bone(S, "pelvis");
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
    proc_submit_weapon((WeaponId)g_weapon[0], MatrixMultiply(pose_attach(&pose, ha_r, sw_bind), root), WEAPON_LEN[g_weapon[0]]);
}

static void submit_bram(const MemberAnim *K, Vector3 pos, float yaw) {
    const BodySex S = SEX_MALE;
    Pose pose;
    character_pose_anim(S, &K->anim, NULL, 0, &pose);
    Matrix root = MatrixMultiply(MatrixMultiply(MatrixScale(0.98f, 1.06f, 1.0f), MatrixRotateY(yaw)), MatrixTranslate(pos.x, pos.y, pos.z));
    CharStyle st = {
        .sex = S, .light_skin = true, .skin_tint = C(236, 200, 172), .hair_color = C(92, 78, 70), .hair = HAIRSTYLE_BUZZED,
        .eye_color = C(130, 190, 240), .flash = K->flash,
        .top = C(120, 134, 160), .bottom = C(58, 62, 80), .shoes = C(40, 36, 44),
        .sleeve_reach = 0.44f, .hem_y = 0.085f, .shoe_h = 0.12f, .waist_y = 0.95f,
    };
    character_submit(&st, &pose, root);
    Color steel = C(150, 164, 190), gold = C(214, 170, 70), dark = C(70, 78, 100);
    RenderMat sm = cloth(steel, 1.4f), gm = cloth(gold, 1.6f), dm = cloth(dark, 0.8f);
    int sp2 = character_bone(S, "spine_02"), pel = character_bone(S, "pelvis");
    int ua_l = character_bone(S, "upperarm_l"), la_l = character_bone(S, "lowerarm_l"), ha_l = character_bone(S, "hand_l");
    int ua_r = character_bone(S, "upperarm_r"), la_r = character_bone(S, "lowerarm_r"), ha_r = character_bone(S, "hand_r");
    /* cuirass, belt, faulds */
    part_on(&pose, sp2, PM_CYL, scl(0.205f, 0.40f, 0.145f, 0, 0, 0, 0, 1.06f, 0.01f), sm, root);
    part_on(&pose, sp2, PM_CYL, scl(0.215f, 0.06f, 0.155f, 0, 0, 0, 0, 0.98f, 0.01f), gm, root);
    part_on(&pose, pel, PM_CONE, scl(0.26f, 0.34f, 0.19f, 3.14159f, 0, 0, 0, 0.98f, 0.0f), dm, root);
    /* pauldrons + vambraces follow the posed arms */
    int ups[2] = {ua_l, ua_r}, los[2] = {la_l, la_r}, hands[2] = {ha_l, ha_r};
    for (int k = 0; k < 2; ++k) {
        Vector3 s = tr(pose_bone_pos(&pose, ups[k]), root), e = tr(pose_bone_pos(&pose, los[k]), root), h = tr(pose_bone_pos(&pose, hands[k]), root);
        renderer_submit(proc_mesh(PM_SPHERE), MatrixMultiply(MatrixScale(0.125f, 0.10f, 0.125f), MatrixTranslate(s.x, s.y + 0.03f, s.z)), sm);
        part_between(PM_CYL, s, e, 0.058f, 0.058f, dm, MatrixIdentity());
        part_between(PM_CYL, e, h, 0.052f, 0.052f, sm, MatrixIdentity());
    }
    /* Bulwark of Oaths: a round shield strapped to the left forearm */
    Matrix lb = pose_bind(S, la_l);
    Vector3 c = {lb.m12 + 0.02f, lb.m13, lb.m14 + 0.16f};
    part_on(&pose, la_l, PM_CYL, scl(0.30f, 0.05f, 0.30f, 1.5708f, 0, 0, c.x, c.y, c.z), sm, root);
    part_on(&pose, la_l, PM_CYL, scl(0.14f, 0.065f, 0.14f, 1.5708f, 0, 0, c.x, c.y, c.z + 0.02f), gm, root);
    part_on(&pose, la_l, PM_TORUS, scl(0.30f, 0.30f, 0.30f, 1.5708f, 0, 0, c.x, c.y, c.z + 0.03f), gm, root);
    /* boots */
    for (int sd = -1; sd <= 1; sd += 2) {
        int foot = character_bone(S, sd > 0 ? "foot_l" : "foot_r");
        float x = 0.105f * (float)sd;
        part_on(&pose, foot, PM_SPHERE, scl(0.066f, 0.056f, 0.14f, 0, 0, 0, x, 0.065f, 0.045f), dm, root);
        part_on(&pose, foot, PM_CYL, scl(0.075f, 0.14f, 0.08f, 0, 0, 0, x, 0.07f, -0.04f), dm, root);
    }
    /* sword in the right hand */
    Matrix hb = pose_bind(S, ha_r);
    Vector3 g0 = {hb.m12 - 0.01f, hb.m13, hb.m14};
    Matrix sw_bind = proc_limb(g0, (Vector3){g0.x - 0.15f, g0.y, g0.z + 0.95f}, 1.0f, 1.0f);
    proc_submit_weapon((WeaponId)g_weapon[1], MatrixMultiply(pose_attach(&pose, ha_r, sw_bind), root), WEAPON_LEN[g_weapon[1]]);
}

static void submit_lyra(const MemberAnim *K, Vector3 pos, float yaw) {
    const BodySex S = SEX_FEMALE;
    Pose pose;
    character_pose_anim(S, &K->anim, NULL, 0, &pose);
    Matrix root = MatrixMultiply(MatrixMultiply(MatrixScale(0.88f, 0.915f, 0.90f), MatrixRotateY(yaw)), MatrixTranslate(pos.x, pos.y, pos.z));
    Color robe = C(72, 52, 140), robe_lt = C(104, 84, 176), cyan = C(120, 230, 255);
    CharStyle st = {
        .sex = S, .light_skin = true, .skin_tint = C(250, 226, 214), .hair_color = C(206, 212, 240), .hair = HAIRSTYLE_PARTED,
        .eye_color = cyan, .flash = K->flash,
        .top = robe, .bottom = C(40, 30, 80), .shoes = C(30, 24, 52),
        .sleeve_reach = 0.0f, .hem_y = 0.06f, .shoe_h = 0.1f, .waist_y = 0.95f,
    };
    character_submit(&st, &pose, root);
    RenderMat rm = cloth(robe, 0.4f), rl = cloth(robe_lt, 0.5f), tm = glow(cyan, 0.5f);
    int pel = character_bone(S, "pelvis"), head = character_bone(S, "Head");
    int ua_l = character_bone(S, "upperarm_l"), la_l = character_bone(S, "lowerarm_l"), ua_r = character_bone(S, "upperarm_r"), la_r = character_bone(S, "lowerarm_r");
    /* flared robe skirt with a glowing hem */
    part_on(&pose, pel, PM_CONE, scl(0.37f, 1.0f, 0.34f, 0, 0, 0, 0, 0.08f, 0), rm, root);
    part_on(&pose, pel, PM_TORUS, scl(0.34f, 0.34f, 0.34f, 0, 0, 0, 0, 0.10f, 0), tm, root);
    /* sleeves */
    int ups[2] = {ua_l, ua_r}, los[2] = {la_l, la_r};
    for (int k = 0; k < 2; ++k) {
        Vector3 s = tr(pose_bone_pos(&pose, ups[k]), root), e = tr(pose_bone_pos(&pose, los[k]), root);
        part_between(PM_CYL, s, e, 0.062f, 0.062f, rm, MatrixIdentity());
        renderer_submit(proc_mesh(PM_SPHERE), MatrixMultiply(MatrixScale(0.078f, 0.078f, 0.078f), MatrixTranslate(s.x, s.y, s.z)), rl);
    }
    /* witch hat and twin tails on the head bone */
    part_on(&pose, head, PM_CYL, scl(0.27f, 0.025f, 0.27f, 0, 0, 0, 0, 1.80f, 0), rm, root);
    part_on(&pose, head, PM_CONE, scl(0.15f, 0.44f, 0.15f, -0.22f, 0, 0, 0, 1.81f, 0), rm, root);
    part_on(&pose, head, PM_TORUS, scl(0.15f, 0.15f, 0.15f, 0, 0, 0, 0, 1.84f, 0), tm, root);
    for (int sd = -1; sd <= 1; sd += 2) {
        float f = (float)sd;
        part_on(&pose, head, PM_CONE, proc_limb((Vector3){0.10f * f, 1.74f, -0.03f}, (Vector3){0.20f * f, 1.30f, -0.12f}, 0.06f, 0.05f), cloth(C(206, 212, 240), 1.0f), root);
        part_on(&pose, head, PM_SPHERE, scl(0.035f, 0.035f, 0.035f, 0, 0, 0, 0.105f * f, 1.75f, -0.03f), tm, root);
    }
    submit_staff(&pose, character_bone(S, "hand_r"), C(120, 86, 60), cyan, C(214, 170, 70), root, 1.5f);
}

static void submit_mirelle(const MemberAnim *K, Vector3 pos, float yaw) {
    const BodySex S = SEX_FEMALE;
    Pose pose;
    character_pose_anim(S, &K->anim, NULL, 0, &pose);
    Matrix root = MatrixMultiply(MatrixMultiply(MatrixScale(0.90f, 0.96f, 0.92f), MatrixRotateY(yaw)), MatrixTranslate(pos.x, pos.y, pos.z));
    Color cream = C(244, 240, 232), gold = C(236, 190, 70);
    CharStyle st = {
        .sex = S, .light_skin = true, .skin_tint = C(252, 228, 214), .hair_color = C(236, 178, 168), .hair = HAIRSTYLE_LONG,
        .eye_color = C(150, 235, 190), .flash = K->flash,
        .top = cream, .bottom = cream, .shoes = C(150, 110, 80),
        .sleeve_reach = 0.50f, .hem_y = 0.06f, .shoe_h = 0.1f, .waist_y = 0.95f,
    };
    character_submit(&st, &pose, root);
    RenderMat dm = cloth(cream, 0.4f), gm = cloth(gold, 1.5f);
    int pel = character_bone(S, "pelvis"), sp3 = character_bone(S, "spine_03"), head = character_bone(S, "Head");
    int ua_l = character_bone(S, "upperarm_l"), la_l = character_bone(S, "lowerarm_l"), ua_r = character_bone(S, "upperarm_r"), la_r = character_bone(S, "lowerarm_r");
    part_on(&pose, pel, PM_CONE, scl(0.40f, 1.0f, 0.36f, 0, 0, 0, 0, 0.06f, 0), dm, root);
    part_on(&pose, pel, PM_TORUS, scl(0.37f, 0.37f, 0.37f, 0, 0, 0, 0, 0.09f, 0), gm, root);
    part_on(&pose, pel, PM_CUBE, scl(0.09f, 0.95f, 0.03f, 0, 0, 0, 0, 0.55f, 0.20f), gm, root);   /* stole */
    part_on(&pose, sp3, PM_TORUS, scl(0.16f, 0.16f, 0.16f, 0, 0, 0, 0, 1.48f, 0), gm, root);
    int ups[2] = {ua_l, ua_r}, los[2] = {la_l, la_r};
    for (int k = 0; k < 2; ++k) {
        Vector3 s = tr(pose_bone_pos(&pose, ups[k]), root), e = tr(pose_bone_pos(&pose, los[k]), root);
        part_between(PM_CYL, s, e, 0.060f, 0.060f, dm, MatrixIdentity());
        renderer_submit(proc_mesh(PM_SPHERE), MatrixMultiply(MatrixScale(0.074f, 0.074f, 0.074f), MatrixTranslate(s.x, s.y, s.z)), dm);
    }
    /* halo */
    part_on(&pose, head, PM_TORUS, scl(0.17f, 0.17f, 0.17f, 0.12f, 0, 0, 0, 1.97f, -0.03f), glow(C(255, 236, 160), 1.0f), root);
    submit_staff(&pose, character_bone(S, "hand_r"), C(236, 224, 200), C(255, 240, 170), gold, root, 1.55f);
}

void heroes_submit(int member, Vector3 pos, float yaw, float t) {
    (void)t;
    if (!characters_ready() || !g_ready || member < 0 || member > 3) return;
    switch (member) {
    case 0: submit_kael(&MA[0], pos, yaw); break;
    case 1: submit_bram(&MA[1], pos, yaw); break;
    case 2: submit_lyra(&MA[2], pos, yaw); break;
    default: submit_mirelle(&MA[3], pos, yaw); break;
    }
}
