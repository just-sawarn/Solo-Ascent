#include "render/procmodels.h"
#include <math.h>
#include <raymath.h>

static Mesh M_SPHERE, M_CYL, M_CONE, M_CUBE, M_TORUS, M_PLANE;
static bool g_ready;

void proc_init(void) {
    M_SPHERE = GenMeshSphere(1.0f, 14, 20);
    M_CYL = GenMeshCylinder(1.0f, 1.0f, 16);
    M_CONE = GenMeshCone(1.0f, 1.0f, 16);
    M_CUBE = GenMeshCube(1.0f, 1.0f, 1.0f);
    M_TORUS = GenMeshTorus(0.12f, 1.0f, 8, 28);
    M_PLANE = GenMeshPlane(1.0f, 1.0f, 1, 1);
    g_ready = true;
}

void proc_shutdown(void) {
    if (!g_ready) return;
    UnloadMesh(M_SPHERE); UnloadMesh(M_CYL); UnloadMesh(M_CONE);
    UnloadMesh(M_CUBE); UnloadMesh(M_TORUS); UnloadMesh(M_PLANE);
    g_ready = false;
}

static Color C(int r, int g, int b) { return (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, 255}; }

CharDesc proc_preset(Outfit o) {
    CharDesc d = {.outfit = o, .skin = C(244, 212, 188), .height = 1.75f, .hair_style = HAIR_SPIKY};
    switch (o) {
    case OUTFIT_HERO: /* Kael: black hair, dark coat with gold trim, golden eyes */
        d.hair = C(24, 24, 36); d.coat = C(34, 38, 62); d.trim = C(236, 190, 70); d.pants = C(22, 24, 38);
        d.boots = C(18, 16, 24); d.eye = C(255, 214, 90); d.hair_style = HAIR_SPIKY; d.height = 1.72f; break;
    case OUTFIT_TANK: /* Bram: steel armor, short grey-brown hair */
        d.hair = C(92, 78, 70); d.coat = C(120, 134, 160); d.trim = C(214, 170, 70); d.pants = C(58, 62, 80);
        d.boots = C(40, 36, 44); d.eye = C(130, 190, 240); d.hair_style = HAIR_SHORT; d.height = 1.92f; d.skin = C(226, 188, 160); break;
    case OUTFIT_MAGE: /* Lyra: silver hair, violet robe */
        d.hair = C(206, 212, 240); d.coat = C(72, 52, 140); d.trim = C(120, 220, 255); d.pants = C(40, 30, 80);
        d.boots = C(30, 24, 52); d.eye = C(120, 230, 255); d.hair_style = HAIR_TWINTAIL; d.height = 1.60f; break;
    case OUTFIT_HEALER: /* Mirelle: cream/white robe, rose-gold hair */
        d.hair = C(236, 178, 168); d.coat = C(242, 238, 232); d.trim = C(236, 190, 70); d.pants = C(236, 232, 224);
        d.boots = C(150, 110, 80); d.eye = C(150, 235, 190); d.hair_style = HAIR_LONG; d.height = 1.68f; break;
    case OUTFIT_SHADOW: /* shadow soldier: ink black, violet eyes */
        d.skin = C(14, 12, 24); d.hair = C(10, 8, 18); d.coat = C(16, 12, 30); d.trim = C(70, 40, 150); d.pants = C(12, 10, 22);
        d.boots = C(8, 6, 14); d.eye = C(190, 90, 255); d.hair_style = HAIR_NONE; d.height = 1.85f; break;
    }
    return d;
}

typedef struct { Matrix root; float s; } Rig;

static Matrix local(float sx, float sy, float sz, float rx, float ry, float rz, float x, float y, float z) {
    Matrix m = MatrixScale(sx, sy, sz);
    m = MatrixMultiply(m, MatrixRotateXYZ((Vector3){rx, ry, rz}));
    return MatrixMultiply(m, MatrixTranslate(x, y, z));
}

static void part(const Rig *g, const Mesh *mesh, Matrix lm, RenderMat mat) {
    renderer_submit(mesh, MatrixMultiply(lm, g->root), mat);
}

static RenderMat mat(Color c, float spec, float rim, float emi, uint8_t flags) {
    return (RenderMat){.base = c, .spec = spec, .rim = rim, .emissive = emi, .smooth = 0.0f, .flags = flags};
}
static RenderMat matb(Color c) { return mat(c, 1.0f, 1.0f, 0.0f, 0); }
static RenderMat matbox(Color c) { RenderMat m = matb(c); m.smooth = 0.85f; return m; }

static void build_hair(const Rig *g, const CharDesc *c, float hy) {
    RenderMat hm = mat(c->hair, 1.4f, 1.0f, 0.0f, RM_HAIR);
    if (c->hair_style == HAIR_NONE) return;
    /* skull cap */
    part(g, &M_SPHERE, local(0.142f, 0.125f, 0.150f, 0, 0, 0, 0, hy + 0.025f, -0.012f), hm);
    int spikes = 0; float len = 0.17f;
    switch (c->hair_style) {
    case HAIR_SPIKY: spikes = 9; len = 0.20f; break;
    case HAIR_SHORT: spikes = 7; len = 0.09f; break;
    case HAIR_LONG: spikes = 5; len = 0.10f; break;
    case HAIR_TWINTAIL: spikes = 6; len = 0.10f; break;
    default: break;
    }
    for (int i = 0; i < spikes; ++i) {
        float a = ((float)i / (float)spikes) * 6.2831853f + 0.4f;
        float tilt = 0.9f + 0.25f * sinf((float)i * 2.1f);
        /* spikes sweep back and outward from the crown */
        float px = sinf(a) * 0.075f, pz = cosf(a) * 0.075f - 0.02f;
        float rx = (cosf(a) * -1.0f) * tilt, rz = sinf(a) * tilt;
        float l = len * (0.8f + 0.4f * fabsf(sinf((float)i * 1.7f)));
        part(g, &M_CONE, local(0.045f, l, 0.045f, rx, 0, rz, px, hy + 0.085f, pz), hm);
    }
    /* fringe: short pointed locks across the forehead */
    for (int i = 0; i < 4; ++i) {
        float x = -0.075f + 0.05f * (float)i;
        part(g, &M_CONE, local(0.034f, 0.075f, 0.03f, 3.14159f - 0.25f, 0, x * 1.5f, x, hy + 0.098f, 0.100f), hm);
    }
    if (c->hair_style == HAIR_LONG) {
        part(g, &M_CONE, local(0.15f, 0.50f, 0.075f, 3.14159f, 0, 0, 0, hy + 0.05f - 0.0f, -0.075f), hm);
    }
    if (c->hair_style == HAIR_TWINTAIL) {
        for (int s = -1; s <= 1; s += 2) {
            part(g, &M_SPHERE, local(0.04f, 0.04f, 0.04f, 0, 0, 0, 0.115f * (float)s, hy + 0.06f, -0.02f), mat(c->trim, 1, 1, 0.2f, RM_NO_OUTLINE));
            part(g, &M_CONE, local(0.06f, 0.44f, 0.06f, 3.14159f, 0, -0.12f * (float)s, 0.125f * (float)s, hy + 0.04f, -0.03f), hm);
        }
    }
}

static void build_face(const Rig *g, const CharDesc *c, float hy) {
    RenderMat eye = mat(c->eye, 0, 0, 1.0f, RM_NO_OUTLINE | RM_NO_SHADOW_CAST);
    for (int s = -1; s <= 1; s += 2) {
        float x = 0.052f * (float)s;
        part(g, &M_SPHERE, local(0.028f, 0.040f, 0.014f, 0, 0, 0, x, hy - 0.005f, 0.118f), eye);
        /* brow */
        part(g, &M_CUBE, local(0.05f, 0.008f, 0.01f, 0, 0, 0.18f * (float)s, x, hy + 0.045f, 0.122f),
             mat(c->hair, 0, 0, 0, RM_NO_OUTLINE | RM_NO_SHADOW_CAST));
    }
}

void proc_submit_character(const CharDesc *c, Vector3 pos, float yaw, float t) {
    float s = c->height / 1.75f;
    float bob = sinf(t * 1.9f) * 0.006f;
    Rig g;
    g.s = s;
    g.root = MatrixMultiply(MatrixMultiply(MatrixScale(s, s, s), MatrixRotateY(yaw)), MatrixTranslate(pos.x, pos.y + bob, pos.z));
    float sway = sinf(t * 1.9f + 1.0f) * 0.03f;

    Color coat = c->coat, trim = c->trim;
    bool robe = c->outfit == OUTFIT_MAGE || c->outfit == OUTFIT_HEALER;
    /* legs and boots */
    for (int sd = -1; sd <= 1; sd += 2) {
        float x = 0.095f * (float)sd;
        if (!robe) part(&g, &M_CYL, local(0.072f, 0.80f, 0.075f, 0, 0, 0, x, 0.10f, 0), matb(c->pants));
        part(&g, &M_CYL, local(0.082f, 0.17f, 0.105f, 0, 0, 0, x, 0.0f, 0.012f), matb(c->boots));
    }
    if (robe) {
        part(&g, &M_CONE, local(c->outfit == OUTFIT_MAGE ? 0.34f : 0.38f, 0.98f, c->outfit == OUTFIT_MAGE ? 0.34f : 0.38f, 0, 0, 0, 0, 0.10f, 0), matb(c->pants));
        part(&g, &M_TORUS, local(0.30f, 0.30f, 0.30f, 0, 0, 0, 0, 0.12f, 0), mat(trim, 0.5f, 1, 0.0f, 0));
    } else {
        /* hip belt + coat skirt */
        part(&g, &M_CYL, local(0.185f, 0.10f, 0.135f, 0, 0, 0, 0, 0.86f, 0), matb(c->pants));
        if (c->outfit == OUTFIT_HERO || c->outfit == OUTFIT_SHADOW) {
            part(&g, &M_CONE, local(0.25f, 0.62f, 0.17f, 3.14159f + 0.06f, 0, 0, 0, 0.98f + 0.0f, -0.05f), matb(coat)); /* coat tails */
            part(&g, &M_CUBE, local(0.28f, 0.03f, 0.20f, 0, 0, 0, 0, 0.93f, 0), mat(trim, 1, 1, 0, 0));
        }
    }
    /* torso */
    part(&g, &M_CYL, local(0.175f, 0.56f, 0.125f, 0, 0, 0, 0, 0.84f, 0), matb(coat));
    part(&g, &M_CYL, local(0.205f, 0.045f, 0.145f, 0, 0, 0, 0, 1.20f, 0), mat(trim, 1.2f, 1.0f, 0.0f, 0)); /* collar / chest trim */
    /* neck */
    part(&g, &M_CYL, local(0.045f, 0.10f, 0.045f, 0, 0, 0, 0, 1.38f, 0), matb(c->skin));
    /* arms */
    for (int sd = -1; sd <= 1; sd += 2) {
        float fs = (float)sd;
        part(&g, &M_SPHERE, local(0.085f, 0.085f, 0.085f, 0, 0, 0, 0.215f * fs, 1.33f, 0), c->outfit == OUTFIT_TANK ? matb(trim) : matb(coat));
        bool front = (c->outfit == OUTFIT_HERO && sd == 1) || (c->outfit == OUTFIT_MAGE && sd == 1) || (c->outfit == OUTFIT_HEALER && sd == 1);
        float rx = front ? -0.9f : sway * fs, rz = 0.13f * fs;
        part(&g, &M_CYL, local(0.052f, 0.44f, 0.052f, rx + 3.14159f, 0, -rz, 0.225f * fs, 1.31f, 0), matb(coat));
        /* hand */
        float hx = 0.225f * fs + sinf(rz) * 0.44f;
        float hyy = 1.31f - cosf(rx) * 0.44f;
        float hz = front ? sinf(-rx) * 0.44f * -1.0f : 0.0f;
        part(&g, &M_SPHERE, local(0.055f, 0.055f, 0.055f, 0, 0, 0, hx, hyy, hz), matb(c->skin));
        if (front) {
            if (c->outfit == OUTFIT_HERO) { /* Dawnbrand */
                part(&g, &M_CUBE, local(0.035f, 0.95f, 0.012f, 0.35f, 0, 0, hx, hyy + 0.42f, hz + 0.18f), mat(C(255, 226, 140), 1.5f, 1.2f, 0.85f, RM_NO_OUTLINE));
                part(&g, &M_CUBE, local(0.16f, 0.025f, 0.03f, 0.35f, 0, 0, hx, hyy + 0.05f, hz + 0.02f), mat(trim, 1, 1, 0.1f, 0));
                part(&g, &M_CYL, local(0.018f, 0.16f, 0.018f, 0.35f, 0, 0, hx, hyy - 0.06f, hz - 0.01f), matb(c->boots));
            } else { /* staff */
                part(&g, &M_CYL, local(0.016f, 1.7f, 0.016f, 0.0f, 0, 0, hx, hyy - 0.55f, hz), matb(C(120, 86, 60)));
                part(&g, &M_SPHERE, local(0.07f, 0.07f, 0.07f, 0, 0, 0, hx, hyy + 1.18f, hz),
                     mat(c->outfit == OUTFIT_MAGE ? C(110, 225, 255) : C(255, 240, 170), 1, 1, 1.0f, RM_NO_OUTLINE));
                part(&g, &M_TORUS, local(0.10f, 0.10f, 0.10f, 1.57f, 0, 0, hx, hyy + 1.18f, hz), mat(trim, 1, 1, 0.3f, RM_NO_OUTLINE));
            }
        }
    }
    float hy = 1.60f;
    /* head */
    part(&g, &M_SPHERE, local(0.118f, 0.135f, 0.125f, 0, 0, 0, 0, hy, 0), matb(c->skin));
    build_face(&g, c, hy);
    build_hair(&g, c, hy);

    switch (c->outfit) {
    case OUTFIT_HERO:
        /* scarf + gold pauldron accent */
        part(&g, &M_TORUS, local(0.17f, 0.17f, 0.17f, 0, 0, 0, 0, 1.37f, 0), mat(trim, 1, 1, 0.1f, 0));
        break;
    case OUTFIT_TANK:
        /* shield: Bulwark of Oaths */
        part(&g, &M_CYL, local(0.34f, 0.05f, 0.34f, 1.5708f, 0, 0, -0.34f, 1.0f, 0.25f), mat(C(132, 148, 178), 1.4f, 1.0f, 0, 0));
        part(&g, &M_CYL, local(0.17f, 0.065f, 0.17f, 1.5708f, 0, 0, -0.34f, 1.0f, 0.255f), mat(trim, 1.6f, 1.0f, 0.35f, 0));
        part(&g, &M_TORUS, local(0.345f, 0.345f, 0.345f, 1.5708f, 0, 0, -0.34f, 1.0f, 0.275f), mat(trim, 1.2f, 1.0f, 0.1f, 0));
        /* breastplate + helm crest */
        part(&g, &M_CYL, local(0.20f, 0.30f, 0.15f, 0, 0, 0, 0, 0.98f, 0.01f), mat(C(150, 164, 190), 1.4f, 1.1f, 0, 0));
        part(&g, &M_CONE, local(0.04f, 0.14f, 0.12f, 0, 0, 0, 0, hy + 0.14f, -0.02f), mat(C(180, 40, 50), 0.8f, 1, 0, 0));
        break;
    case OUTFIT_MAGE:
        /* witch hat */
        part(&g, &M_CYL, local(0.27f, 0.025f, 0.27f, 0, 0, 0, 0, hy + 0.09f, 0), matb(coat));
        part(&g, &M_CONE, local(0.15f, 0.42f, 0.15f, -0.18f, 0, 0, 0, hy + 0.10f, 0), matb(coat));
        part(&g, &M_TORUS, local(0.15f, 0.15f, 0.15f, 0, 0, 0, 0, hy + 0.13f, 0), mat(trim, 1, 1, 0.5f, 0));
        break;
    case OUTFIT_HEALER:
        /* halo + stole */
        part(&g, &M_TORUS, local(0.17f, 0.17f, 0.17f, 0.12f, 0, 0, 0, hy + 0.33f, -0.03f), mat(C(255, 236, 160), 0, 0, 1.0f, RM_NO_OUTLINE | RM_NO_SHADOW_CAST));
        part(&g, &M_CUBE, local(0.09f, 0.78f, 0.03f, 0, 0, 0, 0, 0.60f, 0.12f), mat(trim, 1.2f, 1.0f, 0.1f, 0));
        break;
    case OUTFIT_SHADOW:
        /* horns/crest */
        part(&g, &M_CONE, local(0.03f, 0.18f, 0.03f, 0, 0, -0.5f, 0.09f, hy + 0.10f, 0), mat(c->trim, 1, 1, 0.5f, 0));
        part(&g, &M_CONE, local(0.03f, 0.18f, 0.03f, 0, 0, 0.5f, -0.09f, hy + 0.10f, 0), mat(c->trim, 1, 1, 0.5f, 0));
        break;
    }
}

void proc_submit_pillar(Vector3 p, float h, float r, Color c) {
    Matrix base = MatrixTranslate(p.x, p.y, p.z);
    renderer_submit(&M_CYL, MatrixMultiply(MatrixScale(r * 1.25f, 0.25f, r * 1.25f), base), matb(c));
    renderer_submit(&M_CYL, MatrixMultiply(MatrixMultiply(MatrixScale(r, h, r), MatrixTranslate(0, 0.25f, 0)), base), matb(c));
    renderer_submit(&M_CYL, MatrixMultiply(MatrixMultiply(MatrixScale(r * 1.25f, 0.22f, r * 1.25f), MatrixTranslate(0, 0.25f + h, 0)), base), matb(c));
}

void proc_submit_crystal(Vector3 p, float h, Color c, float t) {
    float bob = sinf(t * 1.1f) * 0.12f;
    Matrix top = MatrixMultiply(MatrixMultiply(MatrixScale(h * 0.28f, h * 0.55f, h * 0.28f), MatrixRotateY(t * 0.4f)), MatrixTranslate(p.x, p.y + h * 0.45f + bob, p.z));
    Matrix bot = MatrixMultiply(MatrixMultiply(MatrixScale(h * 0.28f, h * 0.45f, h * 0.28f), MatrixRotateXYZ((Vector3){3.14159f, t * 0.4f, 0})), MatrixTranslate(p.x, p.y + h * 0.45f + bob, p.z));
    RenderMat m = mat(c, 1.5f, 1.4f, 0.75f, RM_NO_OUTLINE);
    renderer_submit(&M_CONE, top, m);
    renderer_submit(&M_CONE, bot, m);
}

void proc_submit_ground(float size, Color c) {
    renderer_submit(&M_PLANE, MatrixScale(size, 1.0f, size), mat(c, 0.2f, 0.4f, 0, RM_NO_SHADOW_CAST | RM_GROUND));
}

void proc_submit_arch(Vector3 p, float yaw, Color c) {
    Matrix rt = MatrixMultiply(MatrixRotateY(yaw), MatrixTranslate(p.x, p.y, p.z));
    renderer_submit(&M_CUBE, MatrixMultiply(MatrixMultiply(MatrixScale(0.9f, 5.0f, 0.9f), MatrixTranslate(-2.2f, 2.5f, 0)), rt), matbox(c));
    renderer_submit(&M_CUBE, MatrixMultiply(MatrixMultiply(MatrixScale(0.9f, 3.4f, 0.9f), MatrixTranslate(2.2f, 1.7f, 0)), rt), matbox(c));
    renderer_submit(&M_CUBE, MatrixMultiply(MatrixMultiply(MatrixScale(5.8f, 0.8f, 1.1f), MatrixTranslate(0, 5.3f, 0)), rt), matbox(c));
}
