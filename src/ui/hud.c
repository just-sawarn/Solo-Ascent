#include "ui/hud.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

enum { N_DEALT, N_CRIT, N_TAKEN, N_HEAL, N_BLOCK, N_PARRY };

void hud_init(Hud *h) { memset(h, 0, sizeof *h); }

static void push(Hud *h, Vector3 pos, float amount, int kind) {
    HudNumber *n = &h->n[h->head++ % HUD_MAX_NUMBERS];
    *n = (HudNumber){.used = true, .pos = pos, .amount = amount, .kind = kind};
}

void hud_on_events(Hud *h, const CombatEvent *ev, int count, Entity player) {
    for (int i = 0; i < count; ++i) {
        const CombatEvent *e = &ev[i];
        Vector3 p = {e->pos.x, e->pos.y + 2.0f, e->pos.z};
        switch (e->type) {
        case CE_HIT:
            push(h, p, e->amount, e->dst == player ? N_TAKEN : (e->crit ? N_CRIT : N_DEALT));
            if (e->dst != player) h->bar_t[ecs_index(e->dst)] = 4.0f;
            break;
        case CE_HEAL: if (e->amount > 0) push(h, p, e->amount, N_HEAL); break;
        case CE_BLOCK: push(h, (Vector3){p.x, p.y + 0.4f, p.z}, -1.0f, N_BLOCK); break;
        case CE_PARRY: push(h, p, -1.0f, N_PARRY); break;
        default: break;
        }
    }
}

void hud_update(Hud *h, float dt) {
    for (int i = 0; i < HUD_MAX_NUMBERS; ++i) {
        if (!h->n[i].used) continue;
        h->n[i].t += dt;
        if (h->n[i].t > 1.0f) h->n[i].used = false;
    }
    for (int i = 0; i < ECS_MAX_ENTITIES; ++i) if (h->bar_t[i] > 0) h->bar_t[i] -= dt;
}

static void bar(float x, float y, float w, float hgt, float frac, Color fill, Color back) {
    frac = fminf(fmaxf(frac, 0.0f), 1.0f);
    DrawRectangle((int)x - 1, (int)y - 1, (int)w + 2, (int)hgt + 2, (Color){0, 0, 0, 200});
    DrawRectangle((int)x, (int)y, (int)w, (int)hgt, back);
    DrawRectangle((int)x, (int)y, (int)(w * frac), (int)hgt, fill);
}

void hud_draw(const Hud *h, Camera3D cam, World *w, CombatWorld *cw, const EnemyPool *pool, const EnemyDB *db,
              Entity player, Entity locked, const int slots[4], bool touch_ui) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float s = fminf((float)sw, (float)sh) / 720.0f;
    if (s < 0.8f) s = 0.8f;

    /* enemy bars */
    for (int i = 0; i < ENEMY_MAX; ++i) {
        const Enemy *en = &pool->e[i];
        if (!en->used) continue;
        Combatant *c = combat_get(cw, en->ent);
        if (!c || c->dead) continue;
        bool is_locked = en->ent == locked;
        if (h->bar_t[ecs_index(en->ent)] <= 0 && !is_locked) continue;
        Vector3 wp = {w->pos[ecs_index(en->ent)].x, w->pos[ecs_index(en->ent)].y + c->height + 0.35f, w->pos[ecs_index(en->ent)].z};
        Vector2 sp = GetWorldToScreen(wp, cam);
        float bw = (is_locked ? 90.0f : 64.0f) * s;
        bar(sp.x - bw / 2, sp.y, bw, 6 * s, c->hp / c->max_hp, (Color){220, 60, 70, 255}, (Color){40, 14, 18, 255});
        if (is_locked) DrawText(db->e[en->def].name, (int)(sp.x - (float)MeasureText(db->e[en->def].name, (int)(14 * s)) / 2), (int)(sp.y - 18 * s), (int)(14 * s), WHITE);
    }

    /* damage numbers */
    static const Color col[] = {{255, 255, 255, 255}, {255, 190, 60, 255}, {255, 80, 80, 255}, {110, 255, 150, 255}, {170, 190, 220, 255}, {255, 226, 120, 255}};
    for (int i = 0; i < HUD_MAX_NUMBERS; ++i) {
        const HudNumber *n = &h->n[i];
        if (!n->used) continue;
        Vector3 wp = {n->pos.x, n->pos.y + n->t * 1.1f, n->pos.z};
        Vector2 sp = GetWorldToScreen(wp, cam);
        char buf[24];
        if (n->kind == N_BLOCK) snprintf(buf, sizeof buf, "BLOCK");
        else if (n->kind == N_PARRY) snprintf(buf, sizeof buf, "PARRY!");
        else snprintf(buf, sizeof buf, "%d", (int)(n->amount + 0.5f));
        int fs = (int)((n->kind == N_CRIT ? 34 : n->kind == N_PARRY ? 32 : 24) * s * (n->t < 0.12f ? 1.0f + (0.12f - n->t) * 4.0f : 1.0f));
        Color c = col[n->kind];
        c.a = (unsigned char)(255.0f * (1.0f - fmaxf(0.0f, n->t - 0.55f) / 0.45f));
        DrawText(buf, (int)(sp.x - (float)MeasureText(buf, fs) / 2) + 2, (int)sp.y + 2, fs, (Color){0, 0, 0, (unsigned char)(c.a * 0.7f)});
        DrawText(buf, (int)(sp.x - (float)MeasureText(buf, fs) / 2), (int)sp.y, fs, c);
    }

    /* player vitals */
    Combatant *pc = combat_get(cw, player);
    if (!pc) return;
    float x = 20 * s, y = 54 * s, bw = 280 * s;
    bar(x, y, bw, 16 * s, pc->hp / pc->max_hp, (Color){90, 220, 120, 255}, (Color){16, 40, 22, 255});
    if (pc->barrier > 0) bar(x, y + 17 * s, bw * fminf(1.0f, pc->barrier / pc->max_hp), 4 * s, 1.0f, (Color){240, 220, 120, 255}, (Color){0, 0, 0, 0});
    bar(x, y + 22 * s, bw * 0.8f, 10 * s, pc->mp / pc->max_mp, (Color){90, 160, 255, 255}, (Color){14, 22, 44, 255});
    char buf[48];
    snprintf(buf, sizeof buf, "HP %d / %d", (int)pc->hp, (int)pc->max_hp);
    DrawText(buf, (int)(x + 6), (int)(y + 1), (int)(14 * s), WHITE);
    /* resonance gauge */
    bar(x, y + 36 * s, bw * 0.8f, 6 * s, cw->gauge / 100.0f, (Color){255, 200, 70, 255}, (Color){40, 32, 10, 255});
    if (cw->gauge >= 100.0f) DrawText("RESONANCE READY", (int)x, (int)(y + 44 * s), (int)(13 * s), (Color){255, 220, 120, 255});

    /* skill bar (desktop; touch has on-screen buttons) */
    if (!touch_ui) {
        float cell = 56 * s, gap = 8 * s, total = 4 * cell + 3 * gap, bx = ((float)sw - total) / 2, by = (float)sh - cell - 40 * s;
        for (int i = 0; i < 4; ++i) {
            float cx = bx + (float)i * (cell + gap);
            DrawRectangle((int)cx, (int)by, (int)cell, (int)cell, (Color){10, 10, 24, 190});
            DrawRectangleLines((int)cx, (int)by, (int)cell, (int)cell, (Color){255, 220, 120, 200});
            if (slots[i] >= 0) {
                const Skill *sk = &cw->skills->s[slots[i]];
                char nm[8]; snprintf(nm, sizeof nm, "%.4s", sk->name);
                DrawText(nm, (int)(cx + 5 * s), (int)(by + cell / 2 - 7 * s), (int)(14 * s), WHITE);
                float cd = pc->cd[slots[i]];
                if (cd > 0 && sk->cooldown > 0) {
                    float f = cd / sk->cooldown;
                    DrawRectangle((int)cx, (int)(by + cell * (1 - f)), (int)cell, (int)(cell * f), (Color){0, 0, 0, 170});
                    char t[8]; snprintf(t, sizeof t, "%.1f", cd);
                    DrawText(t, (int)(cx + 8 * s), (int)(by + 6 * s), (int)(16 * s), (Color){255, 255, 255, 220});
                } else if (pc->mp < sk->mp) DrawRectangle((int)cx, (int)by, (int)cell, (int)cell, (Color){20, 40, 120, 120});
            }
            snprintf(buf, sizeof buf, "%d", i + 1);
            DrawText(buf, (int)(cx + cell - 14 * s), (int)(by + cell - 18 * s), (int)(14 * s), (Color){255, 220, 120, 255});
        }
    }
    if (pc->dead) {
        DrawRectangle(0, 0, sw, sh, (Color){40, 0, 0, 110});
        const char *m = "DEFEATED";
        DrawText(m, (sw - MeasureText(m, (int)(60 * s))) / 2, sh / 2 - (int)(30 * s), (int)(60 * s), (Color){255, 120, 120, 255});
    }
}
