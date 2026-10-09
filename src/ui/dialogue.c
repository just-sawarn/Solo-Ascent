#include "ui/dialogue.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <raylib.h>

void dui_init(DialogueUI *d) { memset(d, 0, sizeof *d); d->npc = -1; }

const char *speaker_name(const StoryDB *db, const StoryState *s, const char *sp) {
    static char buf[48];
    if (!sp[0] || !strcmp(sp, "narrator")) return "";
    if (!strcmp(sp, "kael") || !strcmp(sp, "player")) return s->player_name;
    if (!strcmp(sp, "voice")) return "???";
    int n = storydb_find_npc(db, sp);
    if (n >= 0) return db->npc[n].name;
    snprintf(buf, sizeof buf, "%s", sp);
    if (buf[0] >= 'a' && buf[0] <= 'z') buf[0] = (char)(buf[0] - 32);
    for (char *p = buf; *p; ++p) if (*p == '_') *p = ' ';
    if (!strcmp(sp, "khemet_ur")) return "Khemet-Ur";
    if (!strcmp(sp, "skarn")) return "Skarn";
    return buf;
}

bool dui_open(DialogueUI *d, StoryState *s, const StoryDB *db, const StoryHooks *h, int tree, int quest, int npc, bool cinematic) {
    if (!dlg_start(&d->r, s, db, h, tree, quest)) return false;
    d->open = true; d->cinematic = cinematic; d->npc = npc; d->reveal = 0; d->sel = 0; d->shown = NULL;
    return true;
}

void dui_queue_cutscene(DialogueUI *d, const char *name) {
    if (d->nqueue < DUI_QUEUE) snprintf(d->queue[d->nqueue++], sizeof d->queue[0], "%s", name);
}

static void text_wrapped(const char *t, float x, float y, float w, int fs, Color c, int max_chars) {
    char line[160];
    int shown = 0;
    const char *p = t;
    while (*p && shown < max_chars) {
        int n = 0, last_space = -1;
        while (p[n] && n < 158) { line[n] = p[n]; line[n + 1] = 0; if (p[n] == ' ') last_space = n; if ((float)MeasureText(line, fs) > w) { if (last_space > 0) n = last_space; break; } n++; }
        if (!p[n] || n >= 158) n = (int)strlen(p) < 158 ? (int)strlen(p) : n;
        char out[160]; int take = n; if (shown + take > max_chars) take = max_chars - shown;
        memcpy(out, p, (size_t)take); out[take] = 0;
        DrawText(out, (int)x + 1, (int)y + 1, fs, (Color){0, 0, 0, 180}); DrawText(out, (int)x, (int)y, fs, c);
        shown += n; p += n; while (*p == ' ') { p++; shown++; }
        y += (float)fs * 1.35f;
    }
}

bool dui_update_draw(DialogueUI *d, StoryState *s, const StoryDB *db, const StoryHooks *h, float dt) {
    if (!d->open && d->nqueue > 0) { /* start the next queued cutscene */
        char tree[64]; snprintf(tree, sizeof tree, "cs.%s", d->queue[0]);
        memmove(d->queue[0], d->queue[1], sizeof d->queue[0] * (size_t)(d->nqueue - 1)); d->nqueue--;
        int t = storydb_find_tree(db, tree);
        if (t >= 0) dui_open(d, s, db, h, t, -1, -1, true);
    }
    if (!d->open) return false;
    const DlgNode *n = dlg_node(&d->r, db);
    if (!n) { d->open = false; return false; }
    if (n != d->shown) { d->shown = n; d->reveal = 0; d->sel = 0; }
    char text[640];
    str_format(n->text, s->player_name, text, sizeof text);
    int len = (int)strlen(text);
    d->reveal += dt * 70.0f;

    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float sc = fminf((float)sw / 1100.0f, (float)sh / 640.0f); if (sc < 0.7f) sc = 0.7f; if (sc > 1.4f) sc = 1.4f;
    Vector2 mouse = GetMousePosition();
    bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_J) || IsKeyPressed(KEY_F);

    if (d->cinematic) { /* letterbox */
        float bar = (float)sh * 0.11f;
        DrawRectangle(0, 0, sw, (int)bar, BLACK); DrawRectangle(0, sh - (int)bar, sw, (int)bar, BLACK);
    }
    float bh = 190 * sc, bx = 40 * sc, bw = (float)sw - 80 * sc, by = (float)sh - bh - (d->cinematic ? (float)sh * 0.11f + 10 : 30 * sc);
    DrawRectangle((int)bx, (int)by, (int)bw, (int)bh, (Color){8, 10, 28, 235});
    DrawRectangleLines((int)bx, (int)by, (int)bw, (int)bh, (Color){255, 214, 90, 230});
    const char *who = speaker_name(db, s, n->speaker);
    if (who[0]) {
        float tw = (float)MeasureText(who, (int)(20 * sc)) + 28 * sc;
        DrawRectangle((int)(bx + 18 * sc), (int)(by - 18 * sc), (int)tw, (int)(30 * sc), (Color){60, 46, 10, 245});
        DrawRectangleLines((int)(bx + 18 * sc), (int)(by - 18 * sc), (int)tw, (int)(30 * sc), (Color){255, 214, 90, 230});
        DrawText(who, (int)(bx + 32 * sc), (int)(by - 14 * sc), (int)(20 * sc), (Color){255, 232, 150, 255});
    }
    bool fully = d->reveal >= (float)len;
    text_wrapped(text, bx + 24 * sc, by + 20 * sc, bw - 48 * sc, (int)(20 * sc), n->speaker[0] && strcmp(n->speaker, "narrator") ? WHITE : (Color){200, 210, 235, 255}, fully ? 100000 : (int)d->reveal);

    if (fully && d->r.nvisible > 0) {
        for (int i = 0; i < d->r.nvisible; ++i) {
            const DlgChoice *ch = &db->choice[n->choice_first + d->r.visible[i]];
            char ct[300]; snprintf(ct, sizeof ct, "%d. ", i + 1);
            str_format(ch->text, s->player_name, ct + strlen(ct), sizeof ct - strlen(ct));
            Rectangle r = {bx + 40 * sc, by + bh - (float)(d->r.nvisible - i) * 34 * sc - 8 * sc, bw - 80 * sc, 30 * sc};
            bool hov = mouse.x >= r.x && mouse.x <= r.x + r.width && mouse.y >= r.y && mouse.y <= r.y + r.height;
            if (hov) d->sel = i;
            DrawRectangleRec(r, i == d->sel ? (Color){70, 78, 150, 235} : (Color){24, 28, 70, 220});
            DrawRectangleLinesEx(r, 1.0f, (Color){255, 214, 90, i == d->sel ? 255 : 120});
            DrawText(ct, (int)(r.x + 10 * sc), (int)(r.y + 6 * sc), (int)(18 * sc), i == d->sel ? (Color){255, 240, 190, 255} : WHITE);
            if (hov && click) { dlg_choose(&d->r, s, db, h, i); if (!d->r.active) d->open = false; return d->open; }
        }
        if (IsKeyPressed(KEY_DOWN)) d->sel = (d->sel + 1) % d->r.nvisible;
        if (IsKeyPressed(KEY_UP)) d->sel = (d->sel + d->r.nvisible - 1) % d->r.nvisible;
        for (int i = 0; i < d->r.nvisible && i < 4; ++i) if (IsKeyPressed(KEY_ONE + i)) d->sel = i, confirm = true;
        if (confirm) { dlg_choose(&d->r, s, db, h, d->sel); if (!d->r.active) d->open = false; }
    } else {
        if (fully && d->r.nvisible == 0) {
            DrawText(((int)(GetTime() * 2)) % 2 ? "v" : " ", (int)(bx + bw - 36 * sc), (int)(by + bh - 34 * sc), (int)(22 * sc), (Color){255, 214, 90, 255});
        }
        if (confirm || click) {
            if (!fully) d->reveal = (float)len + 1;
            else { dlg_advance(&d->r, s, db, h); if (!d->r.active) d->open = false; }
        }
    }
    if (IsKeyPressed(KEY_ESCAPE) && d->cinematic) { /* skip the cutscene (skippable per the design) */
        int guard = 0;
        while (d->r.active && guard++ < 200) { if (d->r.nvisible > 0) dlg_choose(&d->r, s, db, h, 0); else dlg_advance(&d->r, s, db, h); }
        d->open = false;
    }
    if (d->cinematic) DrawText("Esc: skip", sw - MeasureText("Esc: skip", (int)(14 * sc)) - 20, 12, (int)(14 * sc), (Color){255, 255, 255, 150});
    return d->open;
}
