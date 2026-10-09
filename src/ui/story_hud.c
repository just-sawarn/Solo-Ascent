#include "ui/story_hud.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

char npc_marker(const StoryDB *db, const StoryState *ss, int npc) {
    const char *id = db->npc[npc].id;
    for (int i = 0; i < db->nquest; ++i) if (ss->q[i].state == QS_READY && !strcmp(db->quest[i].turn_in, id)) return '?';
    for (int i = 0; i < db->nquest; ++i) if (ss->q[i].state == QS_AVAILABLE && !db->quest[i].auto_start && !strcmp(db->quest[i].giver, id) && db->quest[i].offer_tree[0]) return '!';
    return 0;
}

static float ui_s(void) { float s = fminf((float)GetScreenWidth(), (float)GetScreenHeight()) / 720.0f; return s < 0.8f ? 0.8f : s; }

static void shadow_text(const char *t, int x, int y, int fs, Color c) { DrawText(t, x + 1, y + 1, fs, (Color){0, 0, 0, 190}); DrawText(t, x, y, fs, c); }

void story_hud_draw(const StoryHudCtx *c) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float s = ui_s();

    /* ---- quest markers floating above NPCs ---- */
    for (int i = 0; i < c->npcs->count; ++i) {
        const NpcInst *n = &c->npcs->n[i];
        char m = npc_marker(c->db, c->ss, n->def);
        if (!m) continue;
        Vector2 sp = GetWorldToScreen((Vector3){n->pos.x, n->pos.y + 2.25f, n->pos.z}, c->cam);
        char b[2] = {m, 0};
        Color col = m == '?' ? (Color){255, 214, 90, 255} : (Color){255, 240, 160, 255};
        int fs = (int)((26 + sinf((float)GetTime() * 4) * 2) * s);
        shadow_text(b, (int)sp.x - MeasureText(b, fs) / 2, (int)sp.y - fs, fs, col);
    }
    if (c->near_npc >= 0) { /* talk prompt */
        const NpcDef *d = &c->db->npc[c->npcs->n[c->near_npc].def];
        char b[96]; snprintf(b, sizeof b, c->touch_ui ? "Tap TALK: %s" : "F: Talk to %s", d->name);
        int fs = (int)(20 * s);
        DrawRectangle(sw / 2 - MeasureText(b, fs) / 2 - 12, sh - (int)(150 * s), MeasureText(b, fs) + 24, (int)(34 * s), (Color){8, 10, 28, 215});
        shadow_text(b, sw / 2 - MeasureText(b, fs) / 2, sh - (int)(146 * s), fs, WHITE);
    }

    /* ---- minimap ---- */
    float R = 78 * s, cx = (float)sw - R - 14 * s, cy = 112 * s + R;
    DrawCircle((int)cx, (int)cy, R + 3, (Color){0, 0, 0, 200});
    DrawCircle((int)cx, (int)cy, R, (Color){14, 18, 40, 225});
    float f_x = -sinf(c->cam_yaw), f_z = -cosf(c->cam_yaw), r_x = cosf(c->cam_yaw), r_z = -sinf(c->cam_yaw);
    float scale = R / 38.0f;
    #define TO_MM(wx, wz, outx, outy) do { float dx_ = (wx) - c->focus.x, dz_ = (wz) - c->focus.z; outx = cx + (dx_ * r_x + dz_ * r_z) * scale; outy = cy - (dx_ * f_x + dz_ * f_z) * scale; } while (0)
    float px, py;
    /* locations hosted by this scene */
    for (int i = 0; i < c->db->nloc; ++i) {
        const Location *l = &c->db->loc[i];
        if (!l->in_sandbox) continue;
        TO_MM(l->lx, l->lz, px, py);
        float dx = px - cx, dy = py - cy;
        if (dx * dx + dy * dy < (R - 4) * (R - 4)) DrawCircleLines((int)px, (int)py, l->radius * scale, (Color){120, 160, 220, 120});
    }
    for (int i = 0; i < c->npcs->count; ++i) {
        const NpcInst *n = &c->npcs->n[i];
        TO_MM(n->pos.x, n->pos.z, px, py);
        float dx = px - cx, dy = py - cy;
        if (dx * dx + dy * dy > (R - 3) * (R - 3)) continue;
        char m = npc_marker(c->db, c->ss, n->def);
        DrawCircle((int)px, (int)py, 3 * s, m ? (Color){255, 220, 90, 255} : (Color){150, 150, 160, 255});
    }
    for (int i = 0; i < ENEMY_MAX; ++i) {
        const Enemy *en = &c->enemies->e[i];
        if (!en->used || !combat_alive(c->cw, en->ent)) continue;
        Vec3 p = c->w->pos[ecs_index(en->ent)];
        TO_MM(p.x, p.z, px, py);
        float dx = px - cx, dy = py - cy;
        if (dx * dx + dy * dy < (R - 3) * (R - 3)) DrawCircle((int)px, (int)py, 3 * s, (Color){240, 70, 80, 255});
    }
    for (int i = 0; i < c->party->n; ++i) {
        Vec3 p = c->w->pos[ecs_index(c->party->m[i].ent)];
        TO_MM(p.x, p.z, px, py);
        float dx = px - cx, dy = py - cy;
        if (dx * dx + dy * dy < (R - 3) * (R - 3)) DrawCircle((int)px, (int)py, (i == c->party->active ? 4.5f : 3.0f) * s, i == c->party->active ? WHITE : (Color){110, 220, 255, 255});
    }
    /* tracked quest target(s): gold diamonds, clamped to the rim when off-map */
    if (c->ss->tracked >= 0) {
        const QuestDef *q = &c->db->quest[c->ss->tracked];
        for (int k = 0; k < q->nobj; ++k) {
            if (c->ss->q[c->ss->tracked].prog[k] >= q->obj[k].count) continue;
            int li = q->obj[k].marker[0] ? storydb_find_loc(c->db, q->obj[k].marker) : -1;
            if (li < 0 || !c->db->loc[li].in_sandbox) continue;
            TO_MM(c->db->loc[li].lx, c->db->loc[li].lz, px, py);
            float dx = px - cx, dy = py - cy, d = sqrtf(dx * dx + dy * dy);
            if (d > R - 8) { px = cx + dx / d * (R - 8); py = cy + dy / d * (R - 8); }
            DrawPoly((Vector2){px, py}, 4, 6 * s, 45.0f, (Color){255, 214, 90, 255});
            break;
        }
    }
    DrawCircleLines((int)cx, (int)cy, R, (Color){255, 214, 90, 200});
    #undef TO_MM

    /* clock + place */
    char tb[64]; int hh = (int)c->hour % 24, mm = (int)((c->hour - floorf(c->hour)) * 60.0f);
    snprintf(tb, sizeof tb, "%02d:%02d  %s", hh, mm, c->region_name);
    shadow_text(tb, (int)(cx - R), (int)(cy + R + 8 * s), (int)(14 * s), (Color){220, 225, 245, 255});

    /* ---- quest tracker ---- */
    if (c->ss->tracked >= 0) {
        const QuestDef *q = &c->db->quest[c->ss->tracked];
        const QuestRt *rt = &c->ss->q[c->ss->tracked];
        float tx = (float)sw - 330 * s, ty = cy + R + 34 * s;
        shadow_text(q->title, (int)tx, (int)ty, (int)(17 * s), q->type == QT_MAIN ? (Color){255, 214, 90, 255} : WHITE);
        ty += 22 * s;
        for (int k = 0; k < q->nobj; ++k) {
            bool dn = rt->prog[k] >= q->obj[k].count;
            char b[160];
            if (q->obj[k].count > 1) snprintf(b, sizeof b, "%s %s (%d/%d)", dn ? "[x]" : "[ ]", q->obj[k].text, rt->prog[k] < q->obj[k].count ? rt->prog[k] : q->obj[k].count, q->obj[k].count);
            else snprintf(b, sizeof b, "%s %s", dn ? "[x]" : "[ ]", q->obj[k].text);
            shadow_text(b, (int)tx, (int)ty, (int)(14 * s), dn ? (Color){130, 220, 150, 255} : WHITE);
            ty += 18 * s;
        }
        if (rt->state == QS_READY && strcmp(q->turn_in, "auto")) {
            int n = storydb_find_npc(c->db, q->turn_in);
            char b[96]; snprintf(b, sizeof b, "Return to %s", n >= 0 ? c->db->npc[n].name : q->turn_in);
            shadow_text(b, (int)tx, (int)ty, (int)(14 * s), (Color){255, 214, 90, 255});
        }
    }
}
