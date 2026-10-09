#include "ui/boss_hud.h"
#include <math.h>
#include <stdio.h>

static void centered(const char *s, int y, int fs, Color c) {
    int w = MeasureText(s, fs);
    DrawText(s, (GetScreenWidth() - w) / 2 + 2, y + 2, fs, (Color){0, 0, 0, 180});
    DrawText(s, (GetScreenWidth() - w) / 2, y, fs, c);
}

void boss_hud_draw(const BossRt *rt, const BossDB *db, PartyCtx *pc, float time, bool touch) {
    if (!rt->active) return;
    const BossDef *d = &db->b[rt->def];
    Combatant *bc = combat_get(pc->cw, rt->ent);
    if (!bc) return;
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float s = sh / 720.0f;
    int bw = (int)(sw * 0.46f), bx = (sw - bw) / 2, by = (int)(24 * s), bh = (int)(16 * s);
    float frac = bc->hp / bc->max_hp; if (frac < 0) frac = 0;
    DrawRectangle(bx - 3, by - 3, bw + 6, bh + 6, (Color){0, 0, 0, 190});
    bool immune = bc->dmg_taken <= 0.0f;
    DrawRectangle(bx, by, (int)(bw * frac), bh, immune ? (Color){150, 150, 190, 255} : (rt->armored ? (Color){210, 130, 60, 255} : (Color){200, 50, 60, 255}));
    for (int i = 1; i < d->nphase; ++i) {   /* phase thresholds */
        int px = bx + (int)(bw * d->phase[i].hp_below);
        if (d->phase[i].hp_below > 0) DrawRectangle(px - 1, by - 3, 2, bh + 6, (Color){255, 230, 150, 220});
    }
    char title[96]; snprintf(title, sizeof title, "%s  -  %s", d->title, d->phase[rt->phase].name);
    DrawText(title, bx, by - (int)(20 * s), (int)(18 * s), (Color){255, 240, 210, 255});
    int y = by + bh + (int)(10 * s);
    if (rt->armored) {   /* armor meter */
        float a = rt->armor_max > 0 ? rt->armor_meter / rt->armor_max : 0; if (a < 0) a = 0;
        DrawRectangle(bx, y, (int)(bw * 0.5f), (int)(7 * s), (Color){30, 20, 10, 220});
        DrawRectangle(bx, y, (int)(bw * 0.5f * a), (int)(7 * s), (Color){255, 150, 60, 255});
        DrawText("ARMOR: ice and block-breaks wear it down", bx + (int)(bw * 0.5f) + 8, y - 3, (int)(13 * s), (Color){255, 200, 150, 230});
        y += (int)(14 * s);
    } else if (rt->exposed_t > 0) { DrawText("EXPOSED!", bx, y - 2, (int)(16 * s), (Color){255, 240, 120, 255}); y += (int)(18 * s); }
    if (rt->barrier) { DrawText("MAGIC BARRIER - use two different elements", bx, y - 2, (int)(15 * s), (Color){170, 140, 255, 255}); y += (int)(18 * s); }
    if (rt->tele_t > 0) {
        const char *t = rt->tele_kind == MK_REWIND ? "REWIND!" : rt->tele_kind == MK_SONG ? "THE CHOIR SINGS!" : "VOID WAVE!";
        if (((int)(time * 8)) % 2 == 0) centered(t, sh / 4, (int)(36 * s), (Color){255, 120, 120, 255});
    }
    if (rt->lock_control) {   /* Solo Ascent */
        char b[96]; snprintf(b, sizeof b, "THE SOLO ASCENT   %d:%02d", (int)rt->grav_t / 60, (int)rt->grav_t % 60);
        centered(b, y + (int)(4 * s), (int)(22 * s), (Color){255, 220, 140, 255});
        if (rt->callout_ready) centered(rt->call_cd > 0 ? "Call Out..." : (touch ? "Tap CALL to ask for help" : "Press H: CALL OUT"), sh - (int)(150 * s), (int)(30 * s), ((int)(time * 4)) % 2 ? (Color){255, 240, 160, 255} : (Color){255, 200, 90, 255});
        else centered("Survive. Alone.", sh - (int)(150 * s), (int)(22 * s), (Color){200, 200, 230, 220});
    }
    if (rt->finisher_ready && !rt->quartet) centered(touch ? "Tap QUARTET to finish!" : "Press F: QUARTET STRIKE", sh - (int)(150 * s), (int)(32 * s), ((int)(time * 4)) % 2 ? (Color){255, 240, 160, 255} : (Color){255, 190, 70, 255});
    if (rt->enraged) centered("ENRAGED", by + bh + (int)(46 * s), (int)(20 * s), (Color){255, 80, 80, 255});
}

void impact_frame_draw(const ImpactFrame *f) {
    if (f->t <= 0) return;
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float k = f->t / 1.1f;
    if (k > 0.86f) DrawRectangle(0, 0, sw, sh, (Color){255, 255, 255, 235});                         /* the impact frame itself */
    else {
        DrawRectangle(0, 0, sw, sh, (Color){f->col.r, f->col.g, f->col.b, (unsigned char)(70 * k)});
        for (int i = 0; i < 18; ++i) {                                                               /* speed lines from the centre */
            float a = (float)i / 18.0f * 6.2831853f + 0.2f;
            Vector2 c = {sw * 0.5f, sh * 0.5f};
            DrawLineEx((Vector2){c.x + cosf(a) * sw * 0.18f, c.y + sinf(a) * sh * 0.18f}, (Vector2){c.x + cosf(a) * sw * 0.8f, c.y + sinf(a) * sh * 0.8f}, 2.0f + 3.0f * k, (Color){255, 255, 255, (unsigned char)(160 * k)});
        }
    }
    int fs = sh / 9; int w = MeasureText(f->text, fs);
    DrawText(f->text, (sw - w) / 2 + 4, sh / 2 - fs / 2 + 4, fs, (Color){0, 0, 0, (unsigned char)(220 * k)});
    DrawText(f->text, (sw - w) / 2, sh / 2 - fs / 2, fs, (Color){f->col.r, f->col.g, f->col.b, (unsigned char)(255 * (k > 0.86f ? 0.0f : 1.0f))});
}
