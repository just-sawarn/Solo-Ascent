#include "world/npc_world.h"
#include <math.h>
#include <string.h>
#include <raymath.h>
#include "render/character.h"
#include "render/procmodels.h"

static int c_idle = -2, c_talk, c_walk;

void npcw_init(NpcWorld *w) {
    memset(w, 0, sizeof *w);
    c_idle = anim_find("Idle_Loop"); c_talk = anim_find("Idle_Talking_Loop"); c_walk = anim_find("Walk_Loop");
}

const char *npcw_slot_name(int slot) { static const char *n[3] = {"morning", "afternoon", "night"}; return n[slot < 0 ? 0 : slot > 2 ? 2 : slot]; }

static int find_inst(const NpcWorld *w, int def) { for (int i = 0; i < w->count; ++i) if (w->n[i].def == def) return i; return -1; }

static Vector3 slot_pos(const NpcDef *d, int slot) { return (Vector3){d->sched[slot].x, 0.0f, d->sched[slot].z}; }

void npcw_refresh(NpcWorld *w, const StoryDB *db, const StoryState *s, const StoryHooks *h, const char *region, float hour) {
    /* drop NPCs that left or are no longer visible */
    for (int i = 0; i < w->count;) {
        const NpcDef *d = &db->npc[w->n[i].def];
        bool here = !strcmp(d->region, region) || !strcmp(d->region, "roaming");
        if (here && conds_eval(s, db, h, d->visible, d->nvisible)) { ++i; continue; }
        w->n[i] = w->n[--w->count];
    }
    int slot = sched_slot(hour);
    for (int k = 0; k < db->nnpc && w->count < NPCW_MAX; ++k) {
        const NpcDef *d = &db->npc[k];
        if (strcmp(d->region, region) && strcmp(d->region, "roaming")) continue;
        if (!conds_eval(s, db, h, d->visible, d->nvisible)) continue;
        if (find_inst(w, k) >= 0) continue;
        NpcInst *n = &w->n[w->count++];
        memset(n, 0, sizeof *n);
        n->def = k; n->slot = slot; n->pos = n->goal = slot_pos(d, slot); n->yaw = 3.14159265f;
        anim_player_init(&n->anim, c_idle);
    }
}

static float turn_to(float yaw, float target, float step) {
    const float pi = 3.14159265f;
    float d = fmodf(target - yaw, 2 * pi);
    if (d > pi) d -= 2 * pi;
    if (d < -pi) d += 2 * pi;
    return yaw + fminf(fmaxf(d, -step), step);
}

void npcw_update(NpcWorld *w, const StoryDB *db, float hour, float dt) {
    int slot = sched_slot(hour);
    for (int i = 0; i < w->count; ++i) {
        NpcInst *n = &w->n[i];
        const NpcDef *d = &db->npc[n->def];
        if (n->slot != slot) { n->slot = slot; n->goal = slot_pos(d, slot); }
        float dx = n->goal.x - n->pos.x, dz = n->goal.z - n->pos.z, dist = sqrtf(dx * dx + dz * dz);
        n->moving = !n->talking && dist > 0.15f;
        if (n->moving) {
            float step = fminf(dist, 1.7f * dt);
            n->pos.x += dx / dist * step; n->pos.z += dz / dist * step;
            n->yaw = turn_to(n->yaw, atan2f(dx, dz), 8.0f * dt);
        }
        int clip = n->talking ? c_talk : (n->moving ? c_walk : c_idle);
        if (clip < 0) clip = c_idle;
        anim_play(&n->anim, clip, 0.2f, false);
        anim_update(&n->anim, dt);
    }
}

void npcw_face(NpcWorld *w, int i, Vector3 toward) {
    if (i < 0 || i >= w->count) return;
    w->n[i].yaw = atan2f(toward.x - w->n[i].pos.x, toward.z - w->n[i].pos.z);
}

int npcw_nearest(const NpcWorld *w, Vector3 pos, float range) {
    int best = -1; float bd = range * range;
    for (int i = 0; i < w->count; ++i) {
        float dx = w->n[i].pos.x - pos.x, dz = w->n[i].pos.z - pos.z, d2 = dx * dx + dz * dz;
        if (d2 < bd) { bd = d2; best = i; }
    }
    return best;
}

static bool robed(const char *arch) {
    static const char *r[] = {"priest", "scholar", "sage", "alchemist", "monk", "mage", "healer", "elder"};
    for (unsigned i = 0; i < sizeof r / sizeof r[0]; ++i) if (!strcmp(arch, r[i])) return true;
    return false;
}

void npcw_submit(const NpcWorld *w, const StoryDB *db, Vector3 focus, float radius, int max_count) {
    if (!characters_ready()) return;
    float dist2[NPCW_MAX]; bool taken[NPCW_MAX] = {false};
    int order[NPCW_MAX], n_ord = 0;
    for (int i = 0; i < w->count; ++i) { float dx = w->n[i].pos.x - focus.x, dz = w->n[i].pos.z - focus.z; dist2[i] = dx * dx + dz * dz; }
    for (int pick = 0; pick < max_count; ++pick) { /* partial selection sort: the nearest few */
        int best = -1;
        for (int i = 0; i < w->count; ++i) if (!taken[i] && dist2[i] < radius * radius && (best < 0 || dist2[i] < dist2[best])) best = i;
        if (best < 0) break;
        taken[best] = true; order[n_ord++] = best;
    }
    for (int oi = 0; oi < n_ord; ++oi) {
        const NpcInst *n = &w->n[order[oi]];
        const NpcDef *d = &db->npc[n->def];
        BodySex sex = d->sex ? SEX_FEMALE : SEX_MALE;
        Pose pose;
        character_pose_anim(sex, &n->anim, NULL, 0, &pose);
        float s = d->scale;
        Matrix root = sex == SEX_MALE ? MatrixScale(0.9f * s, s, 0.92f * s) : MatrixScale(0.88f * s, 0.95f * s, 0.9f * s);
        root = MatrixMultiply(MatrixMultiply(root, MatrixRotateY(n->yaw)), MatrixTranslate(n->pos.x, n->pos.y, n->pos.z));
        bool dark = d->skin == 1;
        Color top = {d->top[0], d->top[1], d->top[2], 255};
        CharStyle st = {
            .sex = sex, .light_skin = !dark, .skin_tint = dark ? (Color){190, 140, 100, 255} : (Color){250, 224, 206, 255},
            .hair = (HairModel)(d->hair < 0 || d->hair >= HAIRSTYLE_COUNT ? 0 : d->hair),
            .hair_color = {d->hair_col[0], d->hair_col[1], d->hair_col[2], 255}, .eye_color = {d->eye[0], d->eye[1], d->eye[2], 255},
            .top = top, .bottom = {d->bottom[0], d->bottom[1], d->bottom[2], 255}, .shoes = {d->shoes[0], d->shoes[1], d->shoes[2], 255},
            .sleeve_reach = robed(d->archetype) ? 0.5f : 0.3f, .hem_y = 0.08f, .shoe_h = 0.1f, .waist_y = 0.95f,
        };
        character_submit(&st, &pose, root);
        if (robed(d->archetype)) { /* long robe over the legs */
            int pel = character_bone(sex, "pelvis");
            Matrix bind = MatrixMultiply(MatrixMultiply(MatrixScale(0.36f, 0.95f, 0.33f), MatrixRotateXYZ((Vector3){0, 0, 0})), MatrixTranslate(0, 0.10f, 0));
            RenderMat rm = {.base = top, .spec = 0.3f, .rim = 1.0f};
            renderer_submit(proc_mesh(PM_CONE), MatrixMultiply(pose_attach(&pose, pel, bind), root), rm);
        }
    }
}
