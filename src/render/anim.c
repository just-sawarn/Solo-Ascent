#include "render/anim.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <raymath.h>
#include "core/log.h"

typedef struct {
    char name[32];
    float fps;
    int frames;
    bool loop;
    uint8_t has_t[ANIM_MAX_BONES];
    int t_index[ANIM_MAX_BONES]; /* bone -> slot in trans, or -1 */
    int16_t *rot;                /* frames * nbones * 4 */
    float *trans;                /* nslots * frames * 3 */
} Clip;

static Clip *g_clips;
static int g_nclips, g_nbones;
static char g_bone_names[ANIM_MAX_BONES][32];

int anim_bone_count(void) { return g_nbones; }
const char *anim_bone_name(int b) { return g_bone_names[b]; }

bool anims_load(const char *path) {
    int size = 0;
    unsigned char *d = LoadFileData(path, &size);
    if (!d || size < 16 || memcmp(d, "SANM", 4) != 0) { LOGE("animation file %s missing or invalid", path); if (d) UnloadFileData(d); return false; }
    const unsigned char *p = d, *end = d + size;
    uint32_t ver, nb, nc;
    memcpy(&ver, p + 4, 4); memcpy(&nb, p + 8, 4); p += 12;
    if (ver != 1 || nb == 0 || nb > ANIM_MAX_BONES) { LOGE("animation file %s: bad header", path); UnloadFileData(d); return false; }
    g_nbones = (int)nb;
    for (uint32_t i = 0; i < nb; ++i) { memcpy(g_bone_names[i], p, 32); g_bone_names[i][31] = 0; p += 32; }
    memcpy(&nc, p, 4); p += 4;
    g_clips = calloc(nc, sizeof(Clip));
    g_nclips = 0;
    for (uint32_t c = 0; c < nc; ++c) {
        Clip *cl = &g_clips[c];
        if (p + 32 + 4 + 4 + 1 + nb > end) { LOGE("animation file truncated"); break; }
        memcpy(cl->name, p, 32); cl->name[31] = 0; p += 32;
        uint32_t frames; memcpy(&cl->fps, p, 4); memcpy(&frames, p + 4, 4); cl->loop = p[8]; p += 9;
        cl->frames = (int)frames;
        int slots = 0;
        for (uint32_t b = 0; b < nb; ++b) { cl->has_t[b] = *p++; cl->t_index[b] = cl->has_t[b] ? slots++ : -1; }
        size_t rot_bytes = (size_t)frames * nb * 4 * sizeof(int16_t), tr_bytes = (size_t)slots * frames * 3 * sizeof(float);
        if (p + rot_bytes + tr_bytes > end) { LOGE("animation file truncated in clip %s", cl->name); break; }
        cl->rot = malloc(rot_bytes); memcpy(cl->rot, p, rot_bytes); p += rot_bytes;
        cl->trans = malloc(tr_bytes ? tr_bytes : 4); if (tr_bytes) memcpy(cl->trans, p, tr_bytes); p += tr_bytes;
        g_nclips++;
    }
    UnloadFileData(d);
    LOGI("animations: %d clips, %d bones", g_nclips, g_nbones);
    return g_nclips > 0;
}

void anims_unload(void) {
    for (int i = 0; i < g_nclips; ++i) { free(g_clips[i].rot); free(g_clips[i].trans); }
    free(g_clips); g_clips = NULL; g_nclips = 0;
}

int anim_find(const char *name) {
    for (int i = 0; i < g_nclips; ++i) if (strcmp(g_clips[i].name, name) == 0) return i;
    return -1;
}
float anim_clip_length(int c) { return c < 0 ? 0.0f : (float)(g_clips[c].frames - 1) / g_clips[c].fps; }
bool anim_clip_loops(int c) { return c >= 0 && g_clips[c].loop; }
int anim_clip_bone(const char *name) {
    for (int i = 0; i < g_nbones; ++i) if (strcmp(g_bone_names[i], name) == 0) return i;
    return -1;
}

void anim_player_init(AnimPlayer *p, int clip) {
    *p = (AnimPlayer){.clip = {clip, -1}, .fade = 1.0f, .fade_rate = 1.0f, .speed = 1.0f};
}

void anim_play(AnimPlayer *p, int clip, float fade_s, bool restart) {
    if (clip < 0) return;
    if (p->clip[0] == clip && !restart) return;
    p->clip[1] = p->clip[0]; p->time[1] = p->time[0];
    p->clip[0] = clip; p->time[0] = 0.0f;
    p->finished = false;
    if (fade_s <= 0.0f || p->clip[1] < 0) { p->fade = 1.0f; p->clip[1] = -1; }
    else { p->fade = 0.0f; p->fade_rate = 1.0f / fade_s; }
}

static void advance(const Clip *c, float *t, float dt, bool *finished) {
    *t += dt;
    float len = (float)(c->frames - 1) / c->fps;
    if (c->loop) { if (len > 0) *t = fmodf(*t, len); }
    else if (*t >= len) { *t = len; if (finished) *finished = true; }
}

void anim_update(AnimPlayer *p, float dt) {
    if (p->clip[0] < 0) return;
    float d = dt * p->speed;
    advance(&g_clips[p->clip[0]], &p->time[0], d, &p->finished);
    if (p->clip[1] >= 0) {
        advance(&g_clips[p->clip[1]], &p->time[1], d, NULL);
        p->fade += p->fade_rate * dt;
        if (p->fade >= 1.0f) { p->fade = 1.0f; p->clip[1] = -1; }
    }
}

static Quaternion q_at(const Clip *c, int frame, int bone) {
    const int16_t *q = &c->rot[((size_t)frame * g_nbones + bone) * 4];
    return (Quaternion){q[0] / 32767.0f, q[1] / 32767.0f, q[2] / 32767.0f, q[3] / 32767.0f};
}

static void frame_pos(const Clip *c, float t, int *f0, int *f1, float *u) {
    float fp = t * c->fps;
    int last = c->frames - 1;
    int i0 = (int)floorf(fp);
    if (i0 < 0) i0 = 0;
    if (i0 > last) i0 = last;
    *u = fp - (float)i0; if (*u < 0) *u = 0; if (*u > 1) *u = 1;
    *f0 = i0;
    *f1 = i0 + 1 > last ? (c->loop ? 0 : last) : i0 + 1;
}

static Quaternion clip_rot(const Clip *c, float t, int bone) {
    int f0, f1; float u;
    frame_pos(c, t, &f0, &f1, &u);
    Quaternion a = q_at(c, f0, bone), b = q_at(c, f1, bone);
    if (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w < 0) b = (Quaternion){-b.x, -b.y, -b.z, -b.w};
    return QuaternionNormalize(QuaternionLerp(a, b, u));
}

Quaternion anim_sample_rot(const AnimPlayer *p, int bone) {
    Quaternion cur = clip_rot(&g_clips[p->clip[0]], p->time[0], bone);
    if (p->clip[1] < 0 || p->fade >= 1.0f) return cur;
    Quaternion prev = clip_rot(&g_clips[p->clip[1]], p->time[1], bone);
    if (prev.x * cur.x + prev.y * cur.y + prev.z * cur.z + prev.w * cur.w < 0) cur = (Quaternion){-cur.x, -cur.y, -cur.z, -cur.w};
    return QuaternionNormalize(QuaternionLerp(prev, cur, p->fade));
}

static bool clip_trans(const Clip *c, float t, int bone, Vector3 *out) {
    int slot = c->t_index[bone];
    if (slot < 0) return false;
    int f0, f1; float u;
    frame_pos(c, t, &f0, &f1, &u);
    const float *a = &c->trans[((size_t)slot * c->frames + f0) * 3], *b = &c->trans[((size_t)slot * c->frames + f1) * 3];
    *out = (Vector3){a[0] + (b[0] - a[0]) * u, a[1] + (b[1] - a[1]) * u, a[2] + (b[2] - a[2]) * u};
    return true;
}

bool anim_sample_trans(const AnimPlayer *p, int bone, Vector3 *out) {
    Vector3 cur;
    if (!clip_trans(&g_clips[p->clip[0]], p->time[0], bone, &cur)) return false;
    Vector3 prev;
    if (p->clip[1] >= 0 && p->fade < 1.0f && clip_trans(&g_clips[p->clip[1]], p->time[1], bone, &prev)) cur = Vector3Lerp(prev, cur, p->fade);
    *out = cur;
    return true;
}
