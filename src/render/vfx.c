#include "render/vfx.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <raymath.h>
#include "core/noise.h"

#define MAX_QUADS 3072
#define POOL 384

typedef struct { Vector3 p, v; float life, max_life, size, grav; Color c; bool streak; } Burst;

static struct {
    Mesh mesh;
    float *verts; unsigned char *cols; float *norms; float *uvs; unsigned short *idx;
    int quads, last;
    bool ready;
    Burst pool[POOL]; int live;
    Rng rng;
    Vector3 right, up, fwd, cam;
} V;

void vfx_init(void) {
    memset(&V, 0, sizeof V);
    V.verts = MemAlloc(MAX_QUADS * 4 * 3 * sizeof(float));
    V.norms = MemAlloc(MAX_QUADS * 4 * 3 * sizeof(float));
    V.uvs = MemAlloc(MAX_QUADS * 4 * 2 * sizeof(float));
    V.cols = MemAlloc(MAX_QUADS * 4 * 4);
    V.idx = MemAlloc(MAX_QUADS * 6 * sizeof(unsigned short));
    for (int i = 0; i < MAX_QUADS; ++i) {
        unsigned short b = (unsigned short)(i * 4);
        unsigned short *ix = &V.idx[i * 6];
        ix[0] = b; ix[1] = (unsigned short)(b + 1); ix[2] = (unsigned short)(b + 2); ix[3] = b; ix[4] = (unsigned short)(b + 2); ix[5] = (unsigned short)(b + 3);
        for (int k = 0; k < 4; ++k) { V.norms[(i * 4 + k) * 3 + 1] = 1.0f; }
    }
    V.mesh.vertexCount = MAX_QUADS * 4; V.mesh.triangleCount = MAX_QUADS * 2;
    V.mesh.vertices = V.verts; V.mesh.normals = V.norms; V.mesh.texcoords = V.uvs; V.mesh.colors = V.cols; V.mesh.indices = V.idx;
    UploadMesh(&V.mesh, true);
    rng_seed(&V.rng, 4242);
    V.ready = true;
}

void vfx_shutdown(void) {
    if (!V.ready) return;
    UnloadMesh(V.mesh);    /* frees the CPU arrays too */
    memset(&V, 0, sizeof V);
}

void vfx_reset(void) { V.live = 0; for (int i = 0; i < POOL; ++i) V.pool[i].life = 0; }
int vfx_live(void) { int n = 0; for (int i = 0; i < POOL; ++i) n += V.pool[i].life > 0; return n; }
int vfx_last_count(void) { return V.last; }

static void quad(Vector3 c, Vector3 r, Vector3 u, Color col) {
    if (V.quads >= MAX_QUADS) return;
    float *v = &V.verts[V.quads * 12];
    unsigned char *k = &V.cols[V.quads * 16];
    Vector3 p[4] = {
        {c.x - r.x - u.x, c.y - r.y - u.y, c.z - r.z - u.z}, {c.x + r.x - u.x, c.y + r.y - u.y, c.z + r.z - u.z},
        {c.x + r.x + u.x, c.y + r.y + u.y, c.z + r.z + u.z}, {c.x - r.x + u.x, c.y - r.y + u.y, c.z - r.z + u.z},
    };
    for (int i = 0; i < 4; ++i) { v[i * 3] = p[i].x; v[i * 3 + 1] = p[i].y; v[i * 3 + 2] = p[i].z; k[i * 4] = col.r; k[i * 4 + 1] = col.g; k[i * 4 + 2] = col.b; k[i * 4 + 3] = 255; }
    ++V.quads;
}
static void billboard(Vector3 c, float size, Color col) { quad(c, Vector3Scale(V.right, size), Vector3Scale(V.up, size), col); }
/* a streak from a to b, width w, facing the camera around its own axis */
static void streak(Vector3 a, Vector3 b, float w, Color col) {
    Vector3 axis = Vector3Subtract(b, a);
    Vector3 view = Vector3Subtract(Vector3Scale(Vector3Add(a, b), 0.5f), V.cam);
    Vector3 side = Vector3CrossProduct(view, axis);   /* (view x axis) x axis points back at the camera: front-facing winding */
    float l = Vector3Length(side);
    if (l < 1e-6f) return;
    side = Vector3Scale(side, w / l);
    Vector3 mid = Vector3Scale(Vector3Add(a, b), 0.5f), half = Vector3Scale(axis, 0.5f);
    quad(mid, side, half, col);
}

static struct { Vector3 a, b; Color c; } BEAM[16]; static int nbeam;
void vfx_beam(Vector3 a, Vector3 b, Color c) { if (nbeam < 16) BEAM[nbeam++] = (typeof(BEAM[0])){a, b, c}; }

static float wrap(float v, float w) { return v - w * floorf(v / w + 0.5f); }
static float h01(uint32_t i, uint32_t k) { return (float)(hash_combine(i, k) >> 8) * (1.0f / 16777216.0f); }

static int tier_scale(QualityTier t) {   /* particle budget in per-mille of the ultra count */
    switch (t) { case TIER_ULTRA: return 1000; case TIER_HIGH: return 800; case TIER_MEDIUM: return 520; case TIER_LOW: return 300; default: return 160; }
}

static void weather_particles(WeatherKind k, float inten, float t, QualityTier tier) {
    if (inten < 0.02f) return;
    const float W = 40.0f, H = 22.0f;
    int base = 0; float vy = 0, vx = 0, len = 0, wid = 0; Color col = WHITE; bool snow = false;
    switch (k) {
        case WX_RAIN:  base = 900;  vy = -22.0f; vx = 3.0f;  len = 0.5f; wid = 0.005f; col = (Color){170, 195, 235, 255}; break;
        case WX_STORM: base = 1400; vy = -30.0f; vx = 9.0f;  len = 0.7f; wid = 0.006f; col = (Color){170, 190, 235, 255}; break;
        case WX_SNOW:  base = 800;  vy = -1.6f;  vx = 0.8f;  snow = true; wid = 0.03f; col = (Color){246, 248, 255, 255}; break;
        case WX_SANDSTORM: base = 700; vy = -0.3f; vx = 22.0f; len = 1.6f; wid = 0.012f; col = (Color){226, 184, 120, 255}; break;
        case WX_ASH:   base = 500;  vy = -0.9f;  vx = 1.2f;  snow = true; wid = 0.05f; col = (Color){90, 80, 80, 255}; break;
        default: return;
    }
    int n = base * tier_scale(tier) / 1000;
    n = (int)((float)n * inten);
    for (int i = 0; i < n; ++i) {
        float sx = h01((uint32_t)i, 1) * W, sy = h01((uint32_t)i, 2) * H, sz = h01((uint32_t)i, 3) * W;
        float wob = snow ? sinf(t * 1.3f + (float)i) * 0.5f : 0.0f;
        Vector3 p = {sx + vx * t + wob, sy + vy * t, sz + (snow ? cosf(t * 0.9f + (float)i * 1.7f) * 0.4f : 0.0f)};
        p.x = V.cam.x + wrap(p.x - V.cam.x, W); p.z = V.cam.z + wrap(p.z - V.cam.z, W);
        p.y = V.cam.y - 4.0f + fmodf(fmodf(p.y, H) + H, H);
        Color c = col;
        if (k == WX_ASH && (i % 3) == 0) c = (Color){255, 130, 60, 255};
        if (snow) billboard(p, wid * (0.7f + 0.6f * h01((uint32_t)i, 4)), c);
        else {
            Vector3 d = Vector3Normalize((Vector3){vx, vy, 0});
            streak(p, Vector3Subtract(p, Vector3Scale(d, len)), wid, c);
        }
    }
}

/* ambient motes: pollen/fireflies, dust, ice glints, embers, void motes */
static void ambient_motes(int biome, float t, QualityTier tier, float hour) {
    int n = 90 * tier_scale(tier) / 1000;
    const float W = 34.0f, H = 9.0f;
    bool night = hour < 5.5f || hour > 19.0f;
    for (int i = 0; i < n; ++i) {
        float sx = h01((uint32_t)i, 11) * W, sy = h01((uint32_t)i, 12) * H, sz = h01((uint32_t)i, 13) * W;
        float ph = (float)i * 0.77f;
        Vector3 p; Color c; float sz_q = 0.05f;
        switch (biome) {
            case 0: /* vale: fireflies at night, drifting pollen by day */
                p = (Vector3){sx + sinf(t * 0.4f + ph) * 1.5f, sy * 0.45f + 0.4f + sinf(t * 0.6f + ph) * 0.4f, sz + cosf(t * 0.35f + ph) * 1.5f};
                c = night ? (Color){210, 255, 120, 255} : (Color){255, 248, 200, 255}; sz_q = night ? 0.06f : 0.035f;
                if (!night && (i & 1)) continue;
                break;
            case 1: p = (Vector3){sx + 3.0f * t, sy * 0.3f + 0.2f, sz}; c = (Color){232, 200, 140, 255}; sz_q = 0.04f; break;   /* dust */
            case 2: p = (Vector3){sx + sinf(t * 0.5f + ph) * 0.8f, sy + 0.5f + sinf(t * 1.9f + ph) * 0.1f, sz}; c = (Color){200, 235, 255, 255}; sz_q = 0.05f * (0.5f + 0.5f * sinf(t * 3.0f + ph) + 0.2f); break;
            case 3: p = (Vector3){sx + sinf(t * 0.7f + ph), fmodf(sy + t * 1.4f, H), sz}; c = (Color){255, 120 + (i * 7) % 90, 40, 255}; sz_q = 0.045f; break;   /* embers */
            default: p = (Vector3){sx + sinf(t * 0.3f + ph) * 2.0f, fmodf(sy + t * 0.7f, H), sz}; c = (Color){180, 120, 255, 255}; sz_q = 0.05f; break;
        }
        p.x = V.cam.x + wrap(p.x - V.cam.x, W); p.z = V.cam.z + wrap(p.z - V.cam.z, W);
        p.y += V.cam.y - 3.0f;
        billboard(p, sz_q, c);
    }
}

void vfx_burst(VfxKind k, Vector3 pos, Vector3 dir) {
    if (!V.ready) return;
    int n = 8; float speed = 4.0f, life = 0.4f, size = 0.06f, grav = 9.0f; Color c = (Color){255, 230, 150, 255}; bool streaky = true;
    switch (k) {
        case VFX_SPARK: n = 10; speed = 6.0f; life = 0.35f; size = 0.05f; c = (Color){255, 220, 120, 255}; break;
        case VFX_HIT:   n = 14; speed = 7.0f; life = 0.30f; size = 0.07f; c = (Color){255, 245, 210, 255}; break;
        case VFX_SLASH: n = 12; speed = 9.0f; life = 0.25f; size = 0.08f; grav = 0.0f; c = (Color){160, 215, 255, 255}; break;
        case VFX_HEAL:  n = 14; speed = 1.5f; life = 0.9f; size = 0.07f; grav = -2.5f; c = (Color){150, 255, 190, 255}; streaky = false; break;
        case VFX_DUST:  n = 6; speed = 1.4f; life = 0.5f; size = 0.12f; grav = -0.6f; c = (Color){190, 180, 160, 255}; streaky = false; break;
        case VFX_PICKUP: n = 10; speed = 2.5f; life = 0.7f; size = 0.06f; grav = -1.5f; c = (Color){255, 215, 90, 255}; streaky = false; break;
        case VFX_AURA:  n = 6; speed = 0.8f; life = 1.1f; size = 0.08f; grav = -3.0f; c = (Color){150, 140, 255, 255}; streaky = false; break;
        case VFX_EMBER: n = 6; speed = 1.2f; life = 1.2f; size = 0.05f; grav = -2.0f; c = (Color){255, 120, 50, 255}; streaky = false; break;
        default: break;
    }
    float dl = Vector3Length(dir);
    for (int i = 0; i < n; ++i) {
        Burst *b = NULL;
        for (int j = 0; j < POOL; ++j) if (V.pool[j].life <= 0) { b = &V.pool[j]; break; }
        if (!b) return;
        float a = rng_range(&V.rng, 0.0f, 6.2831853f), e = rng_range(&V.rng, -0.2f, 1.0f), s = speed * rng_range(&V.rng, 0.5f, 1.0f);
        Vector3 v = {cosf(a) * (1.0f - e * 0.4f), e, sinf(a) * (1.0f - e * 0.4f)};
        if (dl > 1e-3f) v = Vector3Add(Vector3Scale(v, 0.45f), Vector3Scale(dir, 1.0f / dl));
        *b = (Burst){.p = pos, .v = Vector3Scale(v, s), .life = life * rng_range(&V.rng, 0.7f, 1.0f), .size = size, .grav = grav, .c = c, .streak = streaky};
        b->max_life = b->life;
    }
}

static void burst_update_draw(float dt) {
    for (int i = 0; i < POOL; ++i) {
        Burst *b = &V.pool[i];
        if (b->life <= 0) continue;
        b->life -= dt;
        b->v.y -= b->grav * dt;
        b->p = Vector3Add(b->p, Vector3Scale(b->v, dt));
        if (b->life <= 0) continue;
        float f = b->life / b->max_life;
        Color c = b->c;
        if (b->streak) streak(b->p, Vector3Subtract(b->p, Vector3Scale(b->v, 0.045f)), b->size * (0.4f + f * 0.6f), c);
        else billboard(b->p, b->size * (0.3f + f * 0.9f), c);
    }
}

void vfx_frame(Camera3D cam, float t, float dt, QualityTier tier, WeatherKind wx, float wxi, int biome, float hour, bool indoor) {
    if (!V.ready) return;
    V.quads = 0;
    V.cam = cam.position;
    V.fwd = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    V.right = Vector3Normalize(Vector3CrossProduct(V.fwd, cam.up));
    V.up = Vector3CrossProduct(V.right, V.fwd);
    if (!indoor) { weather_particles(wx, wxi, t, tier); ambient_motes(biome, t, tier, hour); }
    else ambient_motes(4, t, tier == TIER_MOBILE ? TIER_MOBILE : TIER_LOW, 12.0f);
    burst_update_draw(dt);
    for (int i = 0; i < nbeam; ++i) {   /* beams are drawn as short overlapping streaks so they shimmer */
        Vector3 d = Vector3Subtract(BEAM[i].b, BEAM[i].a); float l = Vector3Length(d);
        int seg = (int)(l / 0.6f) + 1;
        for (int k = 0; k < seg; ++k) {
            float t0 = ((float)k + 0.1f * sinf(t * 6.0f + (float)(k + i))) / (float)seg, t1 = (float)(k + 1) / (float)seg;
            streak(Vector3Add(BEAM[i].a, Vector3Scale(d, t0)), Vector3Add(BEAM[i].a, Vector3Scale(d, t1)), 0.035f, BEAM[i].c);
        }
    }
    nbeam = 0;
    V.last = V.quads;
    if (V.quads == 0) return;
    V.mesh.triangleCount = V.quads * 2;
    UpdateMeshBuffer(V.mesh, 0, V.verts, (int)(V.quads * 12 * sizeof(float)), 0);
    UpdateMeshBuffer(V.mesh, 3, V.cols, V.quads * 16, 0);
    RenderMat m = {.base = WHITE, .spec = 0, .rim = 0, .emissive = 0.55f, .flags = RM_NO_SHADOW_CAST | RM_NO_OUTLINE | RM_NO_RECEIVE};
    renderer_submit(&V.mesh, MatrixIdentity(), m);
}
