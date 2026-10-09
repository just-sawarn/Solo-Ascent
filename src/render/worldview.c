#include "render/worldview.h"
#include <math.h>
#include <string.h>
#include <raymath.h>
#include "core/noise.h"
#include "render/procmodels.h"

static struct { Mesh terrain; bool have_terrain; int props_drawn; } W;

static Color C(int r, int g, int b) { return (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, 255}; }
static float smooth01(float t) { t = t < 0 ? 0 : (t > 1 ? 1 : t); return t * t * (3 - 2 * t); }
static float lerpf(float a, float b, float t) { return a + (b - a) * t; }

void worldview_init(void) { /* state is zero-initialised; zones may already be loaded (title backdrop) */ }
void worldview_unload(void) { if (W.have_terrain) { UnloadMesh(W.terrain); W.have_terrain = false; } }
void worldview_shutdown(void) { worldview_unload(); }
int worldview_last_props(void) { return W.props_drawn; }

void worldview_load(const Zone *z, const RegionDef *rd) {
    worldview_unload();
    if (z->gen == ZG_DUNGEON) return;
    const int N = TERRAIN_N;
    Mesh m = {0};
    m.vertexCount = N * N; m.triangleCount = (N - 1) * (N - 1) * 2;
    m.vertices = MemAlloc((unsigned)(m.vertexCount * 3) * sizeof(float));
    m.normals = MemAlloc((unsigned)(m.vertexCount * 3) * sizeof(float));
    m.texcoords = MemAlloc((unsigned)(m.vertexCount * 2) * sizeof(float));
    m.colors = MemAlloc((unsigned)m.vertexCount * 4);
    m.indices = MemAlloc((unsigned)(m.triangleCount * 3) * sizeof(unsigned short));
    float span = fmaxf(z->terrain.max_h - z->terrain.min_h, 1.0f);
    float plaza_r = z->gen == ZG_HUB ? 44.0f : 14.0f;
    for (int j = 0; j < N; ++j)
        for (int i = 0; i < N; ++i) {
            int k = j * N + i;
            float x = ((float)i - 0.5f * (N - 1)) * TERRAIN_CELL, zz = ((float)j - 0.5f * (N - 1)) * TERRAIN_CELL;
            m.vertices[k * 3] = x; m.vertices[k * 3 + 1] = z->terrain.h[k]; m.vertices[k * 3 + 2] = zz;
            Vec3 n = terrain_normal(&z->terrain, x, zz);
            m.normals[k * 3] = n.x; m.normals[k * 3 + 1] = n.y; m.normals[k * 3 + 2] = n.z;
            m.texcoords[k * 2] = x * 0.1f; m.texcoords[k * 2 + 1] = zz * 0.1f;
            float ht = smooth01((z->terrain.h[k] - z->terrain.min_h) / span);
            float vary = 0.90f + 0.20f * fbm2(z->seed + 5u, x * 0.09f, zz * 0.09f, 3);
            float r = lerpf(rd->ground_low[0], rd->ground_high[0], ht), g = lerpf(rd->ground_low[1], rd->ground_high[1], ht), b = lerpf(rd->ground_low[2], rd->ground_high[2], ht);
            float patch = smooth01((fbm2(z->seed + 9u, x * 0.045f, zz * 0.045f, 3) - 0.55f) * 6.0f) * 0.55f;
            r = lerpf(r, rd->ground_accent[0], patch); g = lerpf(g, rd->ground_accent[1], patch); b = lerpf(b, rd->ground_accent[2], patch);
            float cliff = smooth01((1.0f - n.y - 0.10f) * 7.0f);
            r = lerpf(r, rd->ground_cliff[0], cliff); g = lerpf(g, rd->ground_cliff[1], cliff); b = lerpf(b, rd->ground_cliff[2], cliff);
            float d = hypotf(x, zz);
            float pave = 1.0f - smooth01((d - (plaza_r - 5.0f)) / 7.0f);   /* worn stone on the plaza/arena */
            if (pave > 0) {
                float stone = 0.88f + 0.12f * hash_u32((uint32_t)((int)floorf(x / 3.0f) * 73856093 ^ (int)floorf(zz / 3.0f) * 19349663)) / 4294967295.0f * 2.0f;
                r = lerpf(r, 150.0f * stone, pave * 0.75f); g = lerpf(g, 142.0f * stone, pave * 0.75f); b = lerpf(b, 136.0f * stone, pave * 0.75f);
            }
            m.colors[k * 4] = (unsigned char)fminf(255.0f, r * vary); m.colors[k * 4 + 1] = (unsigned char)fminf(255.0f, g * vary);
            m.colors[k * 4 + 2] = (unsigned char)fminf(255.0f, b * vary); m.colors[k * 4 + 3] = 255;
        }
    int t = 0;
    for (int j = 0; j < N - 1; ++j)
        for (int i = 0; i < N - 1; ++i) {
            unsigned short a = (unsigned short)(j * N + i), b = (unsigned short)(a + 1), c = (unsigned short)(a + N), d = (unsigned short)(c + 1);
            m.indices[t++] = a; m.indices[t++] = c; m.indices[t++] = b;   /* CCW seen from above */
            m.indices[t++] = b; m.indices[t++] = c; m.indices[t++] = d;
        }
    UploadMesh(&m, false);
    W.terrain = m; W.have_terrain = true;
}

/* ---------- props ---------- */
static RenderMat pm(Color c, float emi, uint8_t flags) { return (RenderMat){.base = c, .spec = 0.4f, .rim = 0.8f, .emissive = emi, .smooth = 0.0f, .flags = flags}; }
static Matrix at(Vector3 p, float yaw) { return MatrixMultiply(MatrixRotateY(yaw), MatrixTranslate(p.x, p.y, p.z)); }
static Matrix part(float sx, float sy, float sz, float x, float y, float zz, Matrix root) {
    return MatrixMultiply(MatrixMultiply(MatrixScale(sx, sy, sz), MatrixTranslate(x, y, zz)), root);
}

static void draw_prop(const Prop *p, float dist, float t, const RegionDef *rd) {
    Vector3 pos = {p->x, p->y, p->z};
    Matrix root = at(pos, p->yaw);
    float s = p->scale;
    bool far = dist > 38.0f;
    uint8_t nsc = far ? RM_NO_SHADOW_CAST | RM_NO_OUTLINE : 0;
    const Mesh *cyl = proc_mesh(PM_CYL), *cone = proc_mesh(PM_CONE), *sph = proc_mesh(PM_SPHERE), *cube = proc_mesh(PM_CUBE);
    switch (p->kind) {
        case PROP_TREE: {
            Color leaf = C(rd->ground_low[0] * 0.55f, rd->ground_low[1] * 0.95f, rd->ground_low[2] * 0.6f);
            renderer_submit(cyl, part(0.32f * s, 2.8f * s, 0.32f * s, 0, 0, 0, root), pm(C(98, 70, 50), 0, nsc));
            renderer_submit(sph, part(2.0f * s, 1.7f * s, 2.0f * s, 0, 3.4f * s, 0, root), pm(leaf, 0, nsc));
            if (!far) renderer_submit(sph, part(1.4f * s, 1.2f * s, 1.4f * s, 0.7f * s, 4.4f * s, 0.3f * s, root), pm(C(leaf.r + 14, leaf.g + 18, leaf.b + 6), 0, nsc));
        } break;
        case PROP_PINE: {
            bool snowy = rd->ground_high[0] > 200;
            Color leaf = snowy ? C(70, 110, 100) : C(36, 86, 66);
            renderer_submit(cyl, part(0.26f * s, 1.6f * s, 0.26f * s, 0, 0, 0, root), pm(C(84, 62, 46), 0, nsc));
            for (int k = 0; k < (far ? 2 : 3); ++k)
                renderer_submit(cone, part((1.7f - 0.45f * (float)k) * s, 2.2f * s, (1.7f - 0.45f * (float)k) * s, 0, (1.2f + 1.3f * (float)k) * s, 0, root), pm(snowy && k == 2 ? C(236, 242, 250) : leaf, 0, nsc));
        } break;
        case PROP_BUSH:
            renderer_submit(sph, part(0.9f * s, 0.6f * s, 0.9f * s, 0, 0.35f * s, 0, root), pm(C(rd->ground_low[0] * 0.8f, rd->ground_low[1] * 0.95f, rd->ground_low[2] * 0.7f), 0, RM_NO_SHADOW_CAST | RM_NO_OUTLINE));
            break;
        case PROP_ROCK:
            renderer_submit(sph, MatrixMultiply(MatrixMultiply(MatrixScale(1.1f * s, 0.8f * s, 0.95f * s), MatrixRotateXYZ((Vector3){0.2f, p->yaw, 0.15f})), MatrixTranslate(pos.x, pos.y + 0.45f * s, pos.z)), pm(C(rd->ground_cliff[0], rd->ground_cliff[1], rd->ground_cliff[2]), 0, nsc));
            break;
        case PROP_LAVAROCK: {
            renderer_submit(sph, MatrixMultiply(MatrixMultiply(MatrixScale(1.2f * s, 0.9f * s, 1.0f * s), MatrixRotateXYZ((Vector3){0.1f, p->yaw, 0.2f})), MatrixTranslate(pos.x, pos.y + 0.5f * s, pos.z)), pm(C(36, 26, 28), 0, nsc));
            float glow = 0.55f + 0.25f * sinf(t * 2.0f + p->x);
            renderer_submit(sph, part(0.45f * s, 0.18f * s, 0.45f * s, 0.2f * s, 1.0f * s, 0, root), pm(C(255, 110, 40), glow, RM_NO_SHADOW_CAST | RM_NO_OUTLINE));
        } break;
        case PROP_CACTUS:
            renderer_submit(cyl, part(0.42f * s, 2.6f * s, 0.42f * s, 0, 0, 0, root), pm(C(86, 140, 84), 0, nsc));
            if (!far) { renderer_submit(cyl, part(0.26f * s, 0.9f * s, 0.26f * s, 0.55f * s, 1.2f * s, 0, root), pm(C(86, 140, 84), 0, nsc)); }
            break;
        case PROP_DEADTREE:
            renderer_submit(cyl, part(0.22f * s, 3.2f * s, 0.22f * s, 0, 0, 0, root), pm(C(70, 58, 52), 0, nsc));
            if (!far) renderer_submit(cyl, MatrixMultiply(part(0.12f * s, 1.5f * s, 0.12f * s, 0, 0, 0, MatrixRotateZ(-0.9f)), MatrixMultiply(MatrixTranslate(0.1f * s, 2.0f * s, 0), root)), pm(C(70, 58, 52), 0, nsc));
            break;
        case PROP_ICESPIKE:
            renderer_submit(cone, part(0.9f * s, 3.4f * s, 0.9f * s, 0, 0, 0, root), pm(C(160, 210, 245), 0.2f, nsc));
            if (!far) renderer_submit(cone, part(0.5f * s, 2.0f * s, 0.5f * s, 0.8f * s, 0, 0.3f * s, root), pm(C(190, 228, 250), 0.2f, nsc));
            break;
        case PROP_CRYSTAL: {
            Color c = rd->ground_accent[2] > rd->ground_accent[0] ? C(170, 120, 255) : C(255, 150, 90);
            proc_submit_crystal((Vector3){p->x, p->y + 0.1f, p->z}, 2.2f * s, c, t + p->x);
        } break;
        case PROP_PILLAR:
            proc_submit_pillar(pos, 3.6f * s, 0.6f * s, C(130, 126, 138));
            break;
        case PROP_BONES:
            renderer_submit(cube, MatrixMultiply(MatrixMultiply(MatrixScale(0.08f, 0.08f, 0.9f * s), MatrixRotateY(p->yaw)), MatrixTranslate(pos.x, pos.y + 0.06f, pos.z)), pm(C(226, 220, 204), 0, RM_NO_SHADOW_CAST | RM_NO_OUTLINE));
            renderer_submit(cube, MatrixMultiply(MatrixMultiply(MatrixScale(0.08f, 0.08f, 0.6f * s), MatrixRotateY(p->yaw + 1.1f)), MatrixTranslate(pos.x, pos.y + 0.06f, pos.z)), pm(C(226, 220, 204), 0, RM_NO_SHADOW_CAST | RM_NO_OUTLINE));
            break;
        case PROP_LAMP:
            renderer_submit(cyl, part(0.07f, 2.6f, 0.07f, 0, 0, 0, root), pm(C(60, 56, 70), 0, nsc));
            renderer_submit(sph, part(0.22f, 0.26f, 0.22f, 0, 2.75f, 0, root), pm(C(255, 214, 140), 0.9f, RM_NO_SHADOW_CAST | RM_NO_OUTLINE));
            break;
        default: break;
    }
}

static void draw_portal(const ZoneExit *e, const Zone *z, float t, const RegionDef *rd) {
    float gy = zone_height(z, e->x, e->z);
    float yaw = atan2f(-e->x, -e->z);   /* faces the zone centre */
    Matrix root = at((Vector3){e->x, gy, e->z}, yaw);
    Color stone = C(120, 118, 140);
    const Mesh *cyl = proc_mesh(PM_CYL), *tor = proc_mesh(PM_TORUS), *cone = proc_mesh(PM_CONE);
    Color glow = z->gen == ZG_DUNGEON ? C(rd->dng_torch[0], rd->dng_torch[1], rd->dng_torch[2]) : C(150, 190, 255);
    float pulse = 0.65f + 0.25f * sinf(t * 2.2f);
    renderer_submit(cyl, part(0.45f, 4.2f, 0.45f, -2.0f, 0, 0, root), pm(stone, 0, 0));
    renderer_submit(cyl, part(0.45f, 4.2f, 0.45f, 2.0f, 0, 0, root), pm(stone, 0, 0));
    renderer_submit(cyl, MatrixMultiply(MatrixMultiply(MatrixScale(0.4f, 4.4f, 0.4f), MatrixRotateZ(1.5708f)), MatrixMultiply(MatrixTranslate(2.2f, 4.3f, 0), root)), pm(stone, 0, 0));
    renderer_submit(tor, MatrixMultiply(MatrixMultiply(MatrixScale(1.6f, 1.6f, 1.6f), MatrixRotateX(1.5708f)), MatrixMultiply(MatrixTranslate(0, 2.1f, 0), root)), pm(glow, pulse, RM_NO_SHADOW_CAST | RM_NO_OUTLINE));
    renderer_submit(cone, part(0.35f, 1.6f, 0.35f, 0, 0.1f + 0.15f * sinf(t * 1.5f), 0, root), pm(glow, pulse, RM_NO_SHADOW_CAST | RM_NO_OUTLINE));
}

static float budget_radius(QualityTier t) {
    switch (t) { case TIER_ULTRA: return 95.0f; case TIER_HIGH: return 80.0f; case TIER_MEDIUM: return 62.0f; case TIER_LOW: return 48.0f; default: return 38.0f; }
}

static void submit_dungeon(const Zone *z, const RegionDef *rd, Vector3 focus, float t, QualityTier tier) {
    float size = DG_N * DG_CELL;
    Color floor = C(rd->dng_floor[0], rd->dng_floor[1], rd->dng_floor[2]), wall = C(rd->dng_wall[0], rd->dng_wall[1], rd->dng_wall[2]);
    renderer_submit(proc_mesh(PM_PLANE), MatrixScale(size * 1.2f, 1.0f, size * 1.2f), (RenderMat){.base = floor, .spec = 0.3f, .rim = 0.3f, .flags = RM_NO_SHADOW_CAST | RM_GROUND});
    renderer_submit(proc_mesh(PM_CUBE), MatrixMultiply(MatrixScale(size * 1.2f, 0.6f, size * 1.2f), MatrixTranslate(0, 6.3f, 0)), (RenderMat){.base = C(wall.r / 2, wall.g / 2, wall.b / 2), .spec = 0, .rim = 0, .flags = RM_NO_OUTLINE | RM_NO_SHADOW_CAST});
    DgWallRect rects[320];
    int n = dungeon_wall_rects(&z->dg, rects, 320);
    float R = budget_radius(tier) * 0.8f;
    for (int i = 0; i < n; ++i) {
        const DgWallRect *w = &rects[i];
        float cx = 0.5f * (w->x0 + w->x1), cz = 0.5f * (w->z0 + w->z1);
        if (hypotf(cx - focus.x, cz - focus.z) - 0.5f * hypotf(w->x1 - w->x0, w->z1 - w->z0) > R) continue;
        RenderMat m = {.base = wall, .spec = 0.2f, .rim = 0.6f, .smooth = 0.85f, .flags = 0};
        renderer_submit(proc_mesh(PM_CUBE), MatrixMultiply(MatrixScale(w->x1 - w->x0, 6.0f, w->z1 - w->z0), MatrixTranslate(cx, 3.0f, cz)), m);
    }
    Color tc = C(rd->dng_torch[0], rd->dng_torch[1], rd->dng_torch[2]);
    for (int i = 0; i < z->dg.ntorches; ++i) {
        const DgPoint *p = &z->dg.torches[i];
        if (hypotf(p->x - focus.x, p->z - focus.z) > R) continue;
        float fl = 0.75f + 0.2f * sinf(t * 9.0f + (float)i * 1.7f);
        renderer_submit(proc_mesh(PM_CYL), MatrixMultiply(MatrixScale(0.08f, 1.5f, 0.08f), MatrixTranslate(p->x, 0.7f, p->z)), pm(C(70, 56, 46), 0, RM_NO_OUTLINE));
        renderer_submit(proc_mesh(PM_CONE), MatrixMultiply(MatrixScale(0.2f, 0.55f * fl, 0.2f), MatrixTranslate(p->x, 2.2f, p->z)), pm(tc, 0.95f, RM_NO_SHADOW_CAST | RM_NO_OUTLINE));
    }
    for (int i = 0; i < z->dg.nchests; ++i) {
        const DgPoint *p = &z->dg.chests[i];
        renderer_submit(proc_mesh(PM_CUBE), MatrixMultiply(MatrixScale(1.0f, 0.6f, 0.65f), MatrixTranslate(p->x, 0.3f, p->z)), (RenderMat){.base = C(130, 88, 52), .spec = 0.3f, .rim = 0.6f, .smooth = 0.85f});
        renderer_submit(proc_mesh(PM_CUBE), MatrixMultiply(MatrixScale(1.04f, 0.14f, 0.69f), MatrixTranslate(p->x, 0.66f, p->z)), pm(C(236, 190, 70), 0.3f, 0));
    }
    /* the boss chamber is marked by a slow ring on the floor */
    renderer_submit(proc_mesh(PM_TORUS), MatrixMultiply(MatrixScale(5.0f, 5.0f, 5.0f), MatrixTranslate(z->boss_pos.x, 0.12f, z->boss_pos.z)), pm(tc, 0.7f, RM_NO_SHADOW_CAST | RM_NO_OUTLINE));
}

void worldview_submit(const Zone *z, const RegionDef *rd, Vector3 focus, float t, QualityTier tier, bool legacy) {
    W.props_drawn = 0;
    for (int i = 0; i < z->nexits; ++i) {
        if (hypotf(z->exits[i].x - focus.x, z->exits[i].z - focus.z) < 90.0f) draw_portal(&z->exits[i], z, t, rd);
    }
    if (z->gen == ZG_DUNGEON) { submit_dungeon(z, rd, focus, t, tier); return; }
    if (legacy) return;
    if (W.have_terrain) renderer_submit(&W.terrain, MatrixIdentity(), (RenderMat){.base = WHITE, .spec = 0.1f, .rim = 0.3f, .flags = RM_NO_OUTLINE});
    float R = budget_radius(tier);
    for (int i = 0; i < z->nprops; ++i) {
        const Prop *p = &z->props[i];
        float d = hypotf(p->x - focus.x, p->z - focus.z);
        if (d > R) continue;
        draw_prop(p, d, t, rd);
        ++W.props_drawn;
    }
    for (int i = 0; i < z->nbld; ++i) {
        const Building *b = &z->bld[i];
        float cx = 0.5f * (b->x0 + b->x1), cz = 0.5f * (b->z0 + b->z1);
        if (hypotf(cx - focus.x, cz - focus.z) > R + 8.0f) continue;
        float gy = zone_height(z, cx, cz);
        Color wallc = C(b->col[0], b->col[1], b->col[2]);
        renderer_submit(proc_mesh(PM_CUBE), MatrixMultiply(MatrixScale(b->x1 - b->x0, b->h, b->z1 - b->z0), MatrixTranslate(cx, gy + b->h * 0.5f, cz)), (RenderMat){.base = wallc, .spec = 0.2f, .rim = 0.6f, .smooth = 0.85f});
        float rw = fmaxf(b->x1 - b->x0, b->z1 - b->z0) * 0.78f;
        renderer_submit(proc_mesh(PM_CONE), MatrixMultiply(MatrixMultiply(MatrixScale(rw, 2.4f, rw), MatrixRotateY(0.785398f)), MatrixTranslate(cx, gy + b->h, cz)), (RenderMat){.base = C(150 - b->col[0] / 5, 70, 66), .spec = 0.3f, .rim = 0.8f, .smooth = 0.5f});
        /* warm window glow facing the plaza */
        float a = atan2f(-cz, -cx);
        renderer_submit(proc_mesh(PM_CUBE), MatrixMultiply(MatrixScale(0.9f, 1.2f, 0.9f), MatrixTranslate(cx + cosf(a) * ((b->x1 - b->x0) * 0.5f + 0.02f), gy + 1.8f, cz + sinf(a) * ((b->z1 - b->z0) * 0.5f + 0.02f))), pm(C(255, 214, 140), 0.85f, RM_NO_SHADOW_CAST | RM_NO_OUTLINE));
    }
}

void worldview_hazards(const BossRt *rt, float t) {
    for (int i = 0; i < BOSS_HZ; ++i) {
        const Hazard *h = &rt->hz[i];
        if (!h->on) continue;
        Color col = h->kind == HZ_SAND ? C(226, 190, 120) : h->kind == HZ_FIRE ? C(255, 110, 40) : C(170, 90, 255);
        bool tele = h->tele > 0;
        float pulse = 0.5f + 0.4f * sinf(t * 10.0f);
        if (h->kind == HZ_WAVE) {   /* an expanding ring of glowing blocks */
            int n = (int)(h->r * 5.0f) + 12; if (n > 96) n = 96;
            for (int k = 0; k < n; ++k) {
                float a = (float)k / (float)n * 6.2831853f;
                float x = h->x + cosf(a) * h->r, z = h->z + sinf(a) * h->r;
                renderer_submit(proc_mesh(PM_CUBE), MatrixMultiply(MatrixMultiply(MatrixScale(0.5f, tele ? 0.05f : 0.35f, 1.2f), MatrixRotateY(-a)), MatrixTranslate(x, 0.15f, z)),
                                (RenderMat){.base = col, .emissive = tele ? pulse : 0.9f, .flags = RM_NO_SHADOW_CAST | RM_NO_OUTLINE | RM_NO_RECEIVE});
            }
            continue;
        }
        renderer_submit(proc_mesh(PM_CYL), MatrixMultiply(MatrixScale(h->r, tele ? 0.03f : 0.06f, h->r), MatrixTranslate(h->x, 0.04f + 0.0f, h->z)),
                        (RenderMat){.base = tele ? C(col.r / 2, col.g / 2, col.b / 2) : col, .emissive = tele ? 0.35f * pulse : 0.55f, .flags = RM_NO_SHADOW_CAST | RM_NO_OUTLINE});
        int n = (int)(h->r * 5.0f) + 8;
        for (int k = 0; k < n; ++k) {
            float a = (float)k / (float)n * 6.2831853f;
            renderer_submit(proc_mesh(PM_CUBE), MatrixMultiply(MatrixMultiply(MatrixScale(0.35f, 0.5f, 0.12f), MatrixRotateY(-a)), MatrixTranslate(h->x + cosf(a) * h->r, 0.25f, h->z + sinf(a) * h->r)),
                            (RenderMat){.base = col, .emissive = tele ? pulse : 0.95f, .flags = RM_NO_SHADOW_CAST | RM_NO_OUTLINE | RM_NO_RECEIVE});
        }
    }
}
