#include "ui/menu.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <raylib.h>

enum { TAB_ITEMS, TAB_GEAR, TAB_CRAFT, TAB_SMITH, TAB_SHOP, TAB_JOURNAL, TAB_MAP, TAB_COUNT };
static const char *TAB_NAMES[TAB_COUNT] = {"Items", "Gear", "Craft", "Smith", "Shop", "Journal", "Map"};
static const char *JFILTERS[3] = {"Active", "Available", "Done"};
static const char *SLOT_NAMES[EQ_COUNT] = {"Weapon", "Head", "Body", "Boots", "Accessory 1", "Accessory 2"};
static const char *STATION_NAMES[] = {"All", "Forge", "Alchemy", "Cooking"};

typedef struct { char label[72]; Color col; int id; } Row;
#define MAX_ROWS 320
static Row rows[MAX_ROWS];
static int nrows;

static float S;                    /* ui scale */
static Vector2 mouse;
static bool clicked;

void menu_init(Menu *m) { memset(m, 0, sizeof *m); m->sel = 0; }
void menu_toggle(Menu *m) { m->open = !m->open; if (m->open) { m->scroll = 0; m->sel = 0; m->msg[0] = 0; } }

static Color rarity_color(Rarity r) {
    static const Color c[5] = {{235, 235, 235, 255}, {120, 230, 130, 255}, {110, 170, 255, 255}, {200, 130, 255, 255}, {255, 170, 60, 255}};
    return c[r];
}

static void say(Menu *m, const char *fmt, const char *a) { snprintf(m->msg, sizeof m->msg, fmt, a); m->msg_t = 4.0f; }

static bool in_rect(Rectangle r) { return mouse.x >= r.x && mouse.x <= r.x + r.width && mouse.y >= r.y && mouse.y <= r.y + r.height; }

static void text(const char *t, float x, float y, float size, Color c) {
    DrawText(t, (int)x + 1, (int)y + 1, (int)size, (Color){0, 0, 0, 160});
    DrawText(t, (int)x, (int)y, (int)size, c);
}

static bool button(Rectangle r, const char *label, bool enabled, bool highlight) {
    bool hov = enabled && in_rect(r);
    Color bg = !enabled ? (Color){40, 40, 52, 220} : (highlight ? (Color){120, 96, 30, 240} : (hov ? (Color){70, 80, 130, 240} : (Color){36, 42, 78, 235}));
    DrawRectangleRec(r, bg);
    DrawRectangleLinesEx(r, 1.0f, enabled ? (Color){255, 220, 120, 200} : (Color){90, 90, 100, 200});
    int fs = (int)(15 * S);
    text(label, r.x + (r.width - (float)MeasureText(label, fs)) / 2, r.y + (r.height - (float)fs) / 2, (float)fs, enabled ? WHITE : (Color){140, 140, 150, 255});
    return hov && clicked;
}

static const char *stat_line(const ItemStats *s, float mult, char *buf, size_t cap) {
    size_t n = 0;
    #define ADD(label, v, fmt) if ((v) != 0 && n < cap) n += (size_t)snprintf(buf + n, cap - n, fmt, label, (double)(v))
    buf[0] = 0;
    ADD("ATK ", s->atk * mult, "%s+%.0f  "); ADD("MATK ", s->matk * mult, "%s+%.0f  "); ADD("DEF ", s->armor * mult, "%s+%.0f  ");
    ADD("HP ", s->hp * mult, "%s+%.0f  "); ADD("MP ", s->mp * mult, "%s+%.0f  ");
    ADD("STR ", s->str, "%s+%.0f  "); ADD("INT ", s->intel, "%s+%.0f  "); ADD("VIT ", s->vit, "%s+%.0f  "); ADD("WIS ", s->wis, "%s+%.0f  "); ADD("AGI ", s->agi, "%s+%.0f  ");
    if (s->crit != 0 && n < cap) snprintf(buf + n, cap - n, "CRIT +%.0f%%", (double)(s->crit * 100));
    #undef ADD
    return buf;
}

static void stack_label(const MenuCtx *c, const ItemStack *s, char *out, size_t cap) {
    const ItemDef *d = &c->db->items[s->item];
    size_t n = (size_t)snprintf(out, cap, "%s", d->name);
    if (s->upgrade > 0 && n < cap) n += (size_t)snprintf(out + n, cap - n, " +%d", s->upgrade);
    if (s->enchant >= 0 && n < cap) n += (size_t)snprintf(out + n, cap - n, " [%s]", c->db->enchants[s->enchant].name);
    if (s->qty > 1 && n < cap) snprintf(out + n, cap - n, "  x%d", s->qty);
}

void menu_open_shop(Menu *m, int shop_index) { m->open = true; m->tab = TAB_SHOP; m->shop = shop_index < 0 ? 0 : shop_index; m->sell_mode = 0; m->sel = 0; m->scroll = 0; m->msg[0] = 0; }

/* ---- detail panel for one stack ---- */
static float draw_item_details(const MenuCtx *c, const ItemStack *s, float x, float y, float w) {
    if (!s || s->item < 0) { text("Nothing selected.", x, y, 15 * S, GRAY); return y + 24 * S; }
    const ItemDef *d = &c->db->items[s->item];
    char lbl[96], buf[200];
    stack_label(c, s, lbl, sizeof lbl);
    text(lbl, x, y, 20 * S, rarity_color(d->rarity)); y += 26 * S;
    static const char *tn[] = {"Weapon", "Armor", "Accessory", "Consumable", "Material", "Tome", "Key item"};
    static const char *rn[] = {"Hero", "Tank", "Mage", "Healer"};
    snprintf(buf, sizeof buf, "%s  |  %s%s%s  |  Lv %d", tn[d->type], rarity_name(d->rarity), item_is_equipment(d) ? "  |  " : "", item_is_equipment(d) ? (d->any_role ? "Any role" : rn[d->role]) : "", d->level);
    text(buf, x, y, 13 * S, (Color){190, 200, 230, 255}); y += 20 * S;
    if (item_is_equipment(d)) {
        text(stat_line(&d->st, item_upgrade_mult(s->upgrade), buf, sizeof buf), x, y, 14 * S, (Color){150, 255, 170, 255}); y += 20 * S;
        if (s->enchant >= 0) {
            const EnchantDef *e = &c->db->enchants[s->enchant];
            char eb[160]; stat_line(&e->st, 1.0f, eb, sizeof eb);
            snprintf(buf, sizeof buf, "Enchant: %s  %s%s", e->name, eb, e->element_boost > 0 ? "  (+elemental damage)" : "");
            text(buf, x, y, 13 * S, (Color){220, 170, 255, 255}); y += 20 * S;
        }
    }
    if (d->type == IT_CONSUMABLE) {
        buf[0] = 0; size_t n = 0;
        if (d->heal_hp > 0) n += (size_t)snprintf(buf + n, sizeof buf - n, "HP +%.0f  ", (double)d->heal_hp);
        if (d->heal_mp > 0) n += (size_t)snprintf(buf + n, sizeof buf - n, "MP +%.0f  ", (double)d->heal_mp);
        if (d->revive > 0) n += (size_t)snprintf(buf + n, sizeof buf - n, "Revive %.0f%%  ", (double)(d->revive * 100));
        if (d->cleanse) n += (size_t)snprintf(buf + n, sizeof buf - n, "Cleanse  ");
        if (d->status != ST_NONE) snprintf(buf + n, sizeof buf - n, "%s %.0fs", status_name(d->status), (double)d->status_time);
        text(buf, x, y, 14 * S, (Color){150, 255, 170, 255}); y += 20 * S;
    }
    /* word-wrapped description */
    char line[128]; const char *p = d->desc; int fs = (int)(14 * S);
    while (*p) {
        int n = 0, last_space = -1;
        while (p[n] && n < 127) { line[n] = p[n]; if (p[n] == ' ') last_space = n; line[n + 1] = 0; if ((float)MeasureText(line, fs) > w) { if (last_space > 0) { n = last_space; } break; } n++; }
        if (!p[n] ) n = (int)strlen(p);
        memcpy(line, p, (size_t)n); line[n] = 0;
        text(line, x, y, (float)fs, (Color){210, 210, 220, 255}); y += 18 * S;
        p += n; while (*p == ' ') p++;
    }
    snprintf(buf, sizeof buf, "Buy %d g  |  Sell %d g", d->buy, d->sell);
    text(buf, x, y + 4 * S, 13 * S, (Color){255, 220, 120, 255});
    return y + 26 * S;
}

static void clamp_sel(Menu *m) {
    if (m->sel >= nrows) m->sel = nrows - 1;
    if (m->sel < 0) m->sel = 0;
}

static void member_buttons(Menu *m, MenuCtx *c, float x, float y, float w) {
    float bw = (w - 3 * 6 * S) / 4;
    for (int i = 0; i < c->party->n; ++i) {
        Rectangle r = {x + (float)i * (bw + 6 * S), y, bw, 26 * S};
        if (button(r, c->party->m[i].def->name, true, i == m->member)) m->member = i;
    }
}


/* word-wrapped text; returns the y after the last line */
static float wrap_text(const char *t, float x, float y, float w, float fs, Color col) {
    char line[160];
    const char *p = t; int f = (int)fs;
    while (*p) {
        int n = 0, last_space = -1;
        while (p[n] && n < 158) { line[n] = p[n]; line[n + 1] = 0; if (p[n] == ' ') last_space = n; if ((float)MeasureText(line, f) > w) { if (last_space > 0) n = last_space; break; } n++; }
        if (!p[n] || n >= 158) n = (int)strlen(p) < 158 ? (int)strlen(p) : n;
        memcpy(line, p, (size_t)n); line[n] = 0;
        text(line, x, y, fs, col); y += fs * 1.3f;
        p += n; while (*p == ' ') p++;
    }
    return y;
}

static const char *npc_name(const StoryDB *db, const char *id) { int n = storydb_find_npc(db, id); return n >= 0 ? db->npc[n].name : id; }
static Color quest_color(const QuestDef *q) {
    switch (q->type) { case QT_MAIN: return (Color){255, 214, 90, 255}; case QT_COMPANION: return (Color){120, 230, 255, 255}; case QT_BOUNTY: return (Color){255, 160, 90, 255};
                       case QT_LORE: return (Color){200, 160, 255, 255}; default: return WHITE; }
}
static const char *quest_tag(const QuestDef *q) { static const char *t[] = {"MAIN", "SIDE", "BOND", "BOUNTY", "HIDDEN", "COLLECT", "LORE"}; return t[q->type]; }
static int quest_progress_pct(const StoryState *ss, const QuestDef *q, int qi) {
    int have = 0, need = 0;
    for (int k = 0; k < q->nobj; ++k) { need += q->obj[k].count; have += ss->q[qi].prog[k]; }
    return need ? have * 100 / need : 0;
}
static bool journal_match(const MenuCtx *c, const Menu *m, int qi) {
    QState st = (QState)c->ss->q[qi].state;
    if (m->jfilter == 0) return st == QS_ACTIVE || st == QS_READY;
    if (m->jfilter == 1) return st == QS_AVAILABLE;
    return st == QS_COMPLETE || (c->story->quest[qi].repeatable && c->ss->q[qi].times_done > 0);
}

static void build_rows(Menu *m, MenuCtx *c) {
    nrows = 0;
    char buf[96];
    switch (m->tab) {
    case TAB_ITEMS:
        for (int i = 0; i < INV_SLOTS && nrows < MAX_ROWS; ++i) {
            if (c->inv->slot[i].item < 0) continue;
            stack_label(c, &c->inv->slot[i], rows[nrows].label, sizeof rows[nrows].label);
            rows[nrows].col = rarity_color(c->db->items[c->inv->slot[i].item].rarity); rows[nrows++].id = i;
        }
        break;
    case TAB_GEAR:
        for (int s = 0; s < EQ_COUNT; ++s) {
            const ItemStack *st = &c->inv->equip[m->member][s];
            if (st->item >= 0) { char l[64]; stack_label(c, st, l, sizeof l); snprintf(rows[nrows].label, sizeof rows[nrows].label, "%s: %s", SLOT_NAMES[s], l); rows[nrows].col = rarity_color(c->db->items[st->item].rarity); }
            else { snprintf(rows[nrows].label, sizeof rows[nrows].label, "%s: (empty)", SLOT_NAMES[s]); rows[nrows].col = (Color){130, 130, 150, 255}; }
            rows[nrows++].id = s;
        }
        break;
    case TAB_CRAFT:
        for (int r = 0; r < c->db->nrecipes && nrows < MAX_ROWS; ++r) {
            const RecipeDef *rc = &c->db->recipes[r];
            if (m->station > 0 && rc->station != (1u << (m->station - 1))) continue;
            bool ok = craft_check(c->inv, c->db, r, ST_FORGE | ST_ALCHEMY | ST_COOKING, c->gear_level) == CR_OK;
            snprintf(buf, sizeof buf, "%s%s", c->db->items[rc->output].name, rc->qty > 1 ? " x" : "");
            if (rc->qty > 1) snprintf(rows[nrows].label, sizeof rows[nrows].label, "%s%d", buf, rc->qty); else snprintf(rows[nrows].label, sizeof rows[nrows].label, "%s", buf);
            rows[nrows].col = ok ? (Color){150, 255, 170, 255} : (Color){160, 160, 175, 255}; rows[nrows++].id = r;
        }
        break;
    case TAB_SMITH:
        for (int s = 0; s < EQ_COUNT; ++s) {
            const ItemStack *st = &c->inv->equip[m->member][s];
            if (st->item < 0) continue;
            char l[64]; stack_label(c, st, l, sizeof l);
            snprintf(rows[nrows].label, sizeof rows[nrows].label, "[worn] %s", l); rows[nrows].col = rarity_color(c->db->items[st->item].rarity); rows[nrows++].id = 1000 + s;
        }
        for (int i = 0; i < INV_SLOTS && nrows < MAX_ROWS; ++i) {
            const ItemStack *st = &c->inv->slot[i];
            if (st->item < 0 || !item_is_equipment(&c->db->items[st->item])) continue;
            stack_label(c, st, rows[nrows].label, sizeof rows[nrows].label); rows[nrows].col = rarity_color(c->db->items[st->item].rarity); rows[nrows++].id = i;
        }
        break;
    case TAB_JOURNAL: {
        static const QuestType order[] = {QT_MAIN, QT_COMPANION, QT_SIDE, QT_LORE, QT_HIDDEN, QT_COLLECTION, QT_BOUNTY};
        for (unsigned o = 0; o < sizeof order / sizeof order[0]; ++o)
            for (int qi = 0; qi < c->story->nquest && nrows < MAX_ROWS; ++qi) {
                const QuestDef *q = &c->story->quest[qi];
                if (q->type != order[o] || !journal_match(c, m, qi)) continue;
                int pct = quest_progress_pct(c->ss, q, qi);
                if (m->jfilter == 0) snprintf(rows[nrows].label, sizeof rows[nrows].label, "%s%s  (%d%%)", c->ss->tracked == qi ? "> " : "", q->title, pct);
                else snprintf(rows[nrows].label, sizeof rows[nrows].label, "[%s] %s", quest_tag(q), q->title);
                rows[nrows].col = quest_color(q); rows[nrows++].id = qi;
            }
    } break;
    case TAB_MAP: break;
    case TAB_SHOP: {
        const ShopDef *sh = &c->db->shops[m->shop];
        if (!m->sell_mode) {
            for (int k = 0; k < sh->nsells; ++k) {
                snprintf(rows[nrows].label, sizeof rows[nrows].label, "%s   %d g", c->db->items[sh->sells[k]].name, shop_buy_price(c->db, sh->sells[k], 1.0f));
                rows[nrows].col = rarity_color(c->db->items[sh->sells[k]].rarity); rows[nrows++].id = sh->sells[k];
            }
        } else {
            for (int i = 0; i < INV_SLOTS && nrows < MAX_ROWS; ++i) {
                const ItemStack *st = &c->inv->slot[i];
                if (st->item < 0) continue;
                char l[64]; stack_label(c, st, l, sizeof l);
                snprintf(rows[nrows].label, sizeof rows[nrows].label, "%s   %d g", l, shop_sell_price(c->db, st, 1.0f));
                rows[nrows].col = rarity_color(c->db->items[st->item].rarity); rows[nrows++].id = i;
            }
        }
    } break;
    }
    clamp_sel(m);
}

static ItemStack *smith_target(Menu *m, MenuCtx *c, int id) {
    if (id >= 1000) return &c->inv->equip[m->member][id - 1000];
    return (id >= 0 && id < INV_SLOTS) ? &c->inv->slot[id] : NULL;
}

static int equip_acc_slot(const Inventory *inv, int member) { return inv->equip[member][EQ_ACC1].item >= 0 && inv->equip[member][EQ_ACC2].item < 0 ? 1 : 0; }

static const char *eq_error(EqResult r) {
    switch (r) { case EQ_ERR_ROLE: return "That belongs to a different role."; case EQ_ERR_LEVEL: return "Level too low."; case EQ_ERR_NOSPACE: return "No room in the bag.";
                 case EQ_ERR_TYPE: return "That can't be equipped."; default: return "Nothing there."; }
}


static void draw_map(Menu *m, MenuCtx *c, float px, float py, float pw, float ph) {
    static const struct { const char *id, *name; float x, y; Color col; } REG[5] = {
        {"vale", "Ashbrook Vale and the Whisperwood", 0.27f, 0.50f, {60, 120, 70, 255}}, {"sunscar", "Sunscar Sands", 0.48f, 0.82f, {190, 150, 70, 255}},
        {"frostspire", "Frostspire Peaks", 0.56f, 0.17f, {110, 150, 200, 255}}, {"cinderhold", "Cinderhold", 0.79f, 0.52f, {190, 80, 50, 255}},
        {"hollow", "The Hollow Reach", 0.89f, 0.22f, {110, 60, 150, 255}}};
    Rectangle area = {px + 14 * S, py + 56 * S, pw - 28 * S, ph - 120 * S};
    DrawRectangleRec(area, (Color){28, 30, 52, 255});
    DrawRectangleLinesEx(area, 1.0f, (Color){255, 214, 90, 200});
    for (int i = 0; i < 5; ++i) {
        float cx = area.x + REG[i].x * area.width, cy = area.y + REG[i].y * area.height, r = (i == 0 ? 0.20f : 0.13f) * area.height * 1.5f;
        Color col = REG[i].col; col.a = 90;
        DrawCircle((int)cx, (int)cy, r, col);
        Color ring = REG[i].col; ring.a = 200;
        DrawCircleLines((int)cx, (int)cy, r, ring);
        text(REG[i].name, cx - (float)MeasureText(REG[i].name, (int)(13 * S)) / 2, cy - r - 16 * S, 13 * S, (Color){220, 225, 245, 255});
    }
    Vector2 mouse = GetMousePosition();
    bool clicked_now = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    const char *hover = NULL; int hover_i = -1;
    /* tracked quest targets */
    char tracked_markers[Q_MAX_OBJ][32]; int nmark = 0;
    if (c->ss->tracked >= 0) {
        const QuestDef *q = &c->story->quest[c->ss->tracked];
        for (int k = 0; k < q->nobj; ++k) if (q->obj[k].marker[0] && c->ss->q[c->ss->tracked].prog[k] < q->obj[k].count && nmark < Q_MAX_OBJ) snprintf(tracked_markers[nmark++], 32, "%s", q->obj[k].marker);
    }
    for (int i = 0; i < c->story->nloc; ++i) {
        const Location *l = &c->story->loc[i];
        float x = area.x + l->map_x * area.width, y = area.y + l->map_y * area.height;
        Color col = !strcmp(l->kind, "town") ? (Color){255, 220, 110, 255} : !strcmp(l->kind, "dungeon") ? (Color){255, 100, 100, 255} : !strcmp(l->kind, "shrine") ? WHITE : (Color){120, 220, 255, 255};
        float r = (!strcmp(l->kind, "town") ? 6.0f : 4.0f) * S;
        bool here = !strcmp(l->id, c->town);
        DrawCircle((int)x, (int)y, r, col);
        if (here) DrawCircleLines((int)x, (int)y, r + 5 * S, WHITE);
        for (int k = 0; k < nmark; ++k) if (!strcmp(tracked_markers[k], l->id)) {
            DrawCircleLines((int)x, (int)y, r + 8 * S + sinf((float)GetTime() * 4) * 2 * S, (Color){255, 214, 90, 255});
            text("!", x - 3 * S, y - r - 22 * S, 18 * S, (Color){255, 214, 90, 255});
        }
        float dx = mouse.x - x, dy = mouse.y - y;
        if (dx * dx + dy * dy < (r + 6 * S) * (r + 6 * S)) { hover = l->name; hover_i = i; }
    }
    if (hover) {
        const Location *l = &c->story->loc[hover_i];
        char b[96]; snprintf(b, sizeof b, "%s  (%s)", hover, l->kind);
        float tw = (float)MeasureText(b, (int)(15 * S)) + 16 * S;
        float tx = fminf(mouse.x + 14 * S, area.x + area.width - tw), ty = mouse.y + 14 * S;
        DrawRectangle((int)tx, (int)ty, (int)tw, (int)(26 * S), (Color){6, 8, 24, 240});
        DrawRectangleLines((int)tx, (int)ty, (int)tw, (int)(26 * S), (Color){255, 214, 90, 220});
        text(b, tx + 8 * S, ty + 5 * S, 15 * S, WHITE);
        if (clicked_now && !strcmp(l->kind, "town") && c->on_travel) {
            c->on_travel(c->user, l->id);
            snprintf(m->msg, sizeof m->msg, "Dev travel: the NPCs of %s now stand in the sandbox plaza.", l->name); m->msg_t = 5.0f;
        }
    }
    text("Gold ring: tracked quest target.  White ring: where the sandbox NPCs are from.  Click a town to dev-travel (real regions arrive in Milestone 9).", area.x, area.y + area.height + 8 * S, 12 * S, (Color){190, 200, 230, 255});
    text("Yellow: town    Red: dungeon    Blue: point of interest    White: shrine", area.x, area.y + area.height + 26 * S, 12 * S, (Color){190, 200, 230, 255});
}

void menu_update_draw(Menu *m, MenuCtx *c) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    S = fminf((float)sw / 1060.0f, (float)sh / 640.0f); if (S < 0.55f) S = 0.55f; if (S > 1.4f) S = 1.4f;
    mouse = GetMousePosition();
    clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    float wheel = GetMouseWheelMove();
    if (m->msg_t > 0) m->msg_t -= GetFrameTime();

    /* keyboard */
    if (IsKeyPressed(KEY_ESCAPE)) { m->open = false; return; }
    if (IsKeyPressed(KEY_TAB)) { m->tab = (m->tab + 1) % TAB_COUNT; m->sel = 0; m->scroll = 0; }
    if (IsKeyPressed(KEY_LEFT) && m->member > 0) m->member--;
    if (IsKeyPressed(KEY_RIGHT) && m->member < c->party->n - 1) m->member++;
    if (IsKeyPressed(KEY_DOWN)) m->sel++;
    if (IsKeyPressed(KEY_UP)) m->sel--;
    bool enter = IsKeyPressed(KEY_ENTER);

    float pw = 1040 * S, ph = 600 * S, px = ((float)sw - pw) / 2, py = ((float)sh - ph) / 2;
    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 12, 170});
    DrawRectangle((int)px, (int)py, (int)pw, (int)ph, (Color){14, 16, 36, 245});
    DrawRectangleLines((int)px, (int)py, (int)pw, (int)ph, (Color){255, 214, 90, 230});

    /* tabs */
    float tx = px + 14 * S;
    for (int t = 0; t < TAB_COUNT; ++t) {
        Rectangle r = {tx, py + 12 * S, 96 * S, 32 * S};
        if (button(r, TAB_NAMES[t], true, t == m->tab)) { m->tab = t; m->sel = 0; m->scroll = 0; }
        tx += 102 * S;
    }
    char gold[48]; snprintf(gold, sizeof gold, "%d g", c->inv->gold);
    text(gold, px + pw - 16 * S - (float)MeasureText(gold, (int)(20 * S)), py + 16 * S, 20 * S, (Color){255, 220, 120, 255});
    if (button((Rectangle){px + pw - 110 * S - (float)MeasureText(gold, (int)(20 * S)), py + 12 * S, 90 * S, 32 * S}, "Close", true, false)) { m->open = false; return; }

    build_rows(m, c);
    if (m->tab == TAB_MAP) {
        draw_map(m, c, px, py, pw, ph);
        if (m->msg_t > 0) text(m->msg, px + 16 * S, py + ph - 24 * S, 14 * S, (Color){255, 240, 170, 255});
        return;
    }

    /* list column */
    float lx = px + 14 * S, ly = py + 96 * S, lw = 420 * S, lh = ph - 146 * S, rowh = 28 * S;
    if (m->tab == TAB_CRAFT) {
        float bx = lx;
        for (int i = 0; i < 4; ++i) { if (button((Rectangle){bx, py + 56 * S, 100 * S, 28 * S}, STATION_NAMES[i], true, i == m->station)) { m->station = i; m->sel = 0; m->scroll = 0; } bx += 104 * S; }
    } else if (m->tab == TAB_SHOP) {
        if (button((Rectangle){lx, py + 56 * S, 30 * S, 28 * S}, "<", true, false)) { m->shop = (m->shop + c->db->nshops - 1) % c->db->nshops; m->sel = 0; }
        text(c->db->shops[m->shop].name, lx + 38 * S, py + 62 * S, 15 * S, WHITE);
        if (button((Rectangle){lx + lw - 150 * S, py + 56 * S, 30 * S, 28 * S}, ">", true, false)) { m->shop = (m->shop + 1) % c->db->nshops; m->sel = 0; }
        if (button((Rectangle){lx + lw - 112 * S, py + 56 * S, 112 * S, 28 * S}, m->sell_mode ? "Selling" : "Buying", true, m->sell_mode)) { m->sell_mode = !m->sell_mode; m->sel = 0; m->scroll = 0; }
    } else if (m->tab == TAB_JOURNAL) {
        for (int i = 0; i < 3; ++i) if (button((Rectangle){lx + (float)i * 112 * S, py + 56 * S, 108 * S, 28 * S}, JFILTERS[i], true, i == m->jfilter)) { m->jfilter = i; m->sel = 0; m->scroll = 0; }
    } else {
        member_buttons(m, c, lx, py + 56 * S, lw);
    }
    DrawRectangle((int)lx, (int)ly, (int)lw, (int)lh, (Color){6, 8, 22, 230});
    int visible = (int)(lh / rowh);
    if (in_rect((Rectangle){lx, ly, lw, lh})) {
        if (wheel != 0) m->scroll -= (int)(wheel * 3);
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && GetMouseDelta().y != 0 && !clicked) { static float acc; acc -= GetMouseDelta().y; if (fabsf(acc) > rowh) { m->scroll += (int)(acc / rowh); acc = 0; } }
    }
    if (m->sel < m->scroll) m->scroll = m->sel;
    if (m->sel >= m->scroll + visible) m->scroll = m->sel - visible + 1;
    if (m->scroll > nrows - visible) m->scroll = nrows - visible;
    if (m->scroll < 0) m->scroll = 0;
    for (int i = 0; i < visible && m->scroll + i < nrows; ++i) {
        int ri = m->scroll + i;
        Rectangle r = {lx, ly + (float)i * rowh, lw, rowh};
        if (ri == m->sel) DrawRectangleRec(r, (Color){60, 66, 130, 220});
        else if (in_rect(r)) DrawRectangleRec(r, (Color){34, 40, 84, 200});
        if (in_rect(r) && clicked) m->sel = ri;
        text(rows[ri].label, r.x + 8 * S, r.y + 6 * S, 15 * S, rows[ri].col);
    }
    if (nrows == 0) text(m->tab == TAB_ITEMS ? "The bag is empty." : "Nothing here.", lx + 10 * S, ly + 10 * S, 15 * S, GRAY);
    if (nrows > visible) { /* scrollbar */
        float sbh = lh * (float)visible / (float)nrows, sby = ly + (lh - sbh) * (float)m->scroll / (float)(nrows - visible);
        DrawRectangle((int)(lx + lw - 5 * S), (int)sby, (int)(4 * S), (int)sbh, (Color){255, 214, 90, 160});
    }
    clamp_sel(m);

    /* detail / actions column */
    float dx = lx + lw + 20 * S, dw = pw - lw - 54 * S, dy = py + 60 * S;
    int selid = nrows > 0 ? rows[m->sel].id : -1;
    Combatant *mc = party_cb(c->party, &(PartyCtx){.cw = c->cw}, m->member);
    PartyMember *pm = &c->party->m[m->member];
    float by = py + ph - 76 * S, bw = 150 * S, bh = 34 * S;

    switch (m->tab) {
    case TAB_ITEMS: {
        ItemStack *st = selid >= 0 ? &c->inv->slot[selid] : NULL;
        draw_item_details(c, st, dx, dy, dw);
        text("Target:", dx, py + ph - 122 * S, 13 * S, (Color){190, 200, 230, 255});
        member_buttons(m, c, dx + 56 * S, py + ph - 128 * S, dw - 56 * S);
        if (st && st->item >= 0) {
            const ItemDef *d = &c->db->items[st->item];
            bool eq = item_is_equipment(d), use = d->type == IT_CONSUMABLE || d->type == IT_TOME;
            if (eq && (button((Rectangle){dx, by, bw, bh}, "Equip", true, false) || enter)) {
                EqResult r = inv_equip(c->inv, c->db, m->member, pm->def->role, c->gear_level, selid, equip_acc_slot(c->inv, m->member));
                if (r == EQ_OK) { m->gear_dirty = true; say(m, "Equipped on %s.", pm->def->name); } else say(m, "%s", eq_error(r));
            } else if (use && (button((Rectangle){dx, by, bw, bh}, d->type == IT_TOME ? "Learn" : "Use", true, false) || enter)) {
                if (d->type == IT_TOME) {
                    if (tome_learn(c->db, c->skills, pm->def == NULL ? NULL : &c->pdb->m[m->member], st->item)) { inv_remove(c->inv, c->db, st->item, 1); say(m, "%s learned a new skill.", pm->def->name); }
                    else say(m, "%s", "They can't learn that (wrong role or already known).");
                } else {
                    UseResult r = inv_use(c->inv, c->db, c->cw, mc, selid);
                    if (r == USE_OK) say(m, "Used on %s.", pm->def->name);
                    else say(m, "%s", r == USE_COOLDOWN ? "Wait a moment." : r == USE_NO_EFFECT ? "It would have no effect." : "Not a valid target.");
                }
            }
            if (d->type != IT_KEY && button((Rectangle){dx + bw + 10 * S, by, 110 * S, bh}, "Drop", true, false)) { *st = (ItemStack){-1, 0, 0, -1}; say(m, "%s", "Dropped."); }
        }
    } break;
    case TAB_GEAR: {
        const ItemStack *st = selid >= 0 ? &c->inv->equip[m->member][selid] : NULL;
        float y = draw_item_details(c, st, dx, dy, dw);
        if (mc) {
            char b[160];
            snprintf(b, sizeof b, "%s  Lv %d   HP %.0f   MP %.0f", pm->def->name, mc->level, (double)mc->max_hp, (double)mc->max_mp);
            text(b, dx, y + 10 * S, 16 * S, WHITE);
            snprintf(b, sizeof b, "ATK %.0f   MATK %.0f   DEF %.0f   MDEF %.0f   CRIT %.0f%%", (double)mc->atk, (double)mc->matk, (double)mc->def, (double)mc->mdef, (double)(mc->crit * 100));
            text(b, dx, y + 34 * S, 14 * S, (Color){150, 255, 170, 255});
        }
        if (st && st->item >= 0 && (button((Rectangle){dx, by, bw, bh}, "Unequip", true, false) || enter)) {
            EqResult r = inv_unequip(c->inv, c->db, m->member, (EquipSlot)selid);
            if (r == EQ_OK) { m->gear_dirty = true; say(m, "%s", "Unequipped."); } else say(m, "%s", eq_error(r));
        }
    } break;
    case TAB_CRAFT: {
        if (selid >= 0) {
            const RecipeDef *rc = &c->db->recipes[selid];
            ItemStack out = {(int16_t)rc->output, (int16_t)rc->qty, 0, -1};
            float y = draw_item_details(c, &out, dx, dy, dw);
            text("Requires:", dx, y + 4 * S, 14 * S, (Color){190, 200, 230, 255}); y += 24 * S;
            for (int k = 0; k < rc->nin; ++k) {
                int have = inv_count(c->inv, rc->in[k].item);
                char b[96]; snprintf(b, sizeof b, "%s   %d / %d", c->db->items[rc->in[k].item].name, have, rc->in[k].qty);
                text(b, dx + 10 * S, y, 14 * S, have >= rc->in[k].qty ? (Color){150, 255, 170, 255} : (Color){255, 130, 120, 255}); y += 19 * S;
            }
            char b[96]; snprintf(b, sizeof b, "Gold %d   |   Level %d   |   %s", rc->gold, rc->level, rc->station == ST_FORGE ? "Forge" : rc->station == ST_ALCHEMY ? "Alchemy" : "Cooking");
            text(b, dx + 10 * S, y + 4 * S, 14 * S, (Color){255, 220, 120, 255});
            CraftResult cr = craft_check(c->inv, c->db, selid, ST_FORGE | ST_ALCHEMY | ST_COOKING, c->gear_level);
            if (button((Rectangle){dx, by, bw, bh}, "Craft", cr == CR_OK, false) || (enter && cr == CR_OK)) {
                CraftResult r = craft_do(c->inv, c->db, selid, ST_FORGE | ST_ALCHEMY | ST_COOKING, c->gear_level);
                if (r == CR_OK) say(m, "Crafted %s.", c->db->items[rc->output].name); else say(m, "%s", r == CR_SPACE ? "No room in the bag." : "Can't craft that yet.");
            }
        }
    } break;
    case TAB_SMITH: {
        ItemStack *st = smith_target(m, c, selid);
        float y = draw_item_details(c, st, dx, dy, dw);
        if (st && st->item >= 0) {
            const ItemDef *d = &c->db->items[st->item];
            UpgradeCost uc = smith_upgrade_cost(c->db, st);
            char b[128];
            if (uc.ok) snprintf(b, sizeof b, "Upgrade to +%d: %d g + %d x %s (have %d)", st->upgrade + 1, uc.gold, uc.mat_qty, c->db->items[uc.mat].name, inv_count(c->inv, uc.mat));
            else snprintf(b, sizeof b, "%s", d->type == IT_ACCESSORY ? "Accessories can't be upgraded (enchant them)." : "Fully upgraded.");
            text(b, dx, y, 14 * S, uc.ok ? (Color){255, 220, 120, 255} : GRAY); y += 24 * S;
            bool can_up = uc.ok && c->inv->gold >= uc.gold && inv_count(c->inv, uc.mat) >= uc.mat_qty;
            if (button((Rectangle){dx, by, bw, bh}, "Upgrade", can_up, false) || (enter && can_up)) {
                if (smith_upgrade(c->inv, c->db, st)) { m->gear_dirty = true; say(m, "%s is stronger.", d->name); }
            }
            /* enchant picker */
            if (c->db->nenchants > 0) {
                if (m->enchant_sel >= c->db->nenchants) m->enchant_sel = 0;
                const EnchantDef *e = &c->db->enchants[m->enchant_sel];
                if (button((Rectangle){dx, y, 30 * S, 28 * S}, "<", true, false)) m->enchant_sel = (m->enchant_sel + c->db->nenchants - 1) % c->db->nenchants;
                if (button((Rectangle){dx + dw - 30 * S, y, 30 * S, 28 * S}, ">", true, false)) m->enchant_sel = (m->enchant_sel + 1) % c->db->nenchants;
                char eb[96], sb[96]; stat_line(&e->st, 1.0f, sb, sizeof sb);
                snprintf(eb, sizeof eb, "%s  %s%s", e->name, sb, e->element_boost > 0 ? "+15% elemental damage" : "");
                text(eb, dx + 38 * S, y + 6 * S, 14 * S, (Color){220, 170, 255, 255});
                snprintf(b, sizeof b, "Needs 1 x %s (have %d), %d g, tier %d+", c->db->items[e->essence].name, inv_count(c->inv, e->essence), smith_enchant_gold(c->db, st), e->min_tier);
                text(b, dx, y + 36 * S, 13 * S, (Color){190, 200, 230, 255});
                if (button((Rectangle){dx + bw + 10 * S, by, 150 * S, bh}, "Enchant", d->type != IT_CONSUMABLE, false)) {
                    EnchantResult r = smith_enchant(c->inv, c->db, st, m->enchant_sel);
                    static const char *msg[] = {"Enchanted.", "Pick an item.", "That enchant doesn't fit this item.", "The item's tier is too low.", "Missing essence.", "Not enough gold.", "Already has that enchant."};
                    if (r == EN_OK) m->gear_dirty = true;
                    say(m, "%s", msg[r]);
                }
            }
        }
    } break;
    case TAB_JOURNAL: {
        if (selid < 0) { text(m->jfilter == 1 ? "Nothing on the board right now." : "Nothing here yet.", dx, dy, 15 * S, GRAY); break; }
        const QuestDef *q = &c->story->quest[selid];
        const QuestRt *rt = &c->ss->q[selid];
        text(q->title, dx, dy, 20 * S, quest_color(q)); float y = dy + 28 * S;
        char b[200];
        snprintf(b, sizeof b, "%s quest   |   %s   |   Act %d", quest_tag(q), q->region[0] ? q->region : "-", q->act);
        text(b, dx, y, 13 * S, (Color){190, 200, 230, 255}); y += 20 * S;
        y = wrap_text(q->summary, dx, y + 2 * S, dw, 14 * S, (Color){210, 212, 225, 255}) + 6 * S;
        for (int k = 0; k < q->nobj; ++k) {
            bool dn = rt->prog[k] >= q->obj[k].count || rt->state == QS_COMPLETE;
            if (q->obj[k].count > 1) snprintf(b, sizeof b, "[%c] %s  (%d/%d)", dn ? 'x' : ' ', q->obj[k].text, rt->prog[k] < q->obj[k].count ? rt->prog[k] : q->obj[k].count, q->obj[k].count);
            else snprintf(b, sizeof b, "[%c] %s", dn ? 'x' : ' ', q->obj[k].text);
            y = wrap_text(b, dx + 6 * S, y, dw - 6 * S, 14 * S, dn ? (Color){130, 220, 150, 255} : WHITE);
        }
        y += 6 * S;
        if (q->giver[0] && strcmp(q->giver, "auto") && strcmp(q->giver, "none")) { snprintf(b, sizeof b, "From: %s", npc_name(c->story, q->giver)); text(b, dx, y, 13 * S, (Color){190, 200, 230, 255}); y += 18 * S; }
        if (q->turn_in[0] && strcmp(q->turn_in, "auto") && strcmp(q->turn_in, "none")) { snprintf(b, sizeof b, "Report to: %s", npc_name(c->story, q->turn_in)); text(b, dx, y, 13 * S, (Color){190, 200, 230, 255}); y += 18 * S; }
        snprintf(b, sizeof b, "Reward: %d XP, %d g", q->xp, q->gold);
        for (int k = 0; k < q->nitems && strlen(b) < 150; ++k) { int it = itemdb_find(c->db, q->items[k].item); snprintf(b + strlen(b), sizeof b - strlen(b), ", %s%s", it >= 0 ? c->db->items[it].name : q->items[k].item, q->items[k].qty > 1 ? " x" : ""); }
        for (int k = 0; k < q->nskills; ++k) snprintf(b + strlen(b), sizeof b - strlen(b), ", skill: %s", q->skills[k].skill);
        y = wrap_text(b, dx, y, dw, 13 * S, (Color){255, 220, 120, 255});
        if ((rt->state == QS_ACTIVE || rt->state == QS_READY) && (button((Rectangle){dx, by, bw, bh}, c->ss->tracked == selid ? "Tracked" : "Track", c->ss->tracked != selid, false) || (enter && c->ss->tracked != selid))) { c->ss->tracked = selid; say(m, "%s", "Tracking this quest."); }
        if (rt->state == QS_AVAILABLE && (!strcmp(q->giver, "none") || !q->giver[0]) && button((Rectangle){dx, by, bw, bh}, "Accept", true, false)) {
            if (story_start_quest(c->ss, c->story, c->sh, selid)) say(m, "Accepted: %s", q->title);
        }
        if (rt->state == QS_AVAILABLE && strcmp(q->giver, "none") && q->giver[0]) text("Talk to the giver to accept this quest.", dx, by + 8 * S, 13 * S, (Color){255, 220, 120, 255});
    } break;
    case TAB_MAP: break;
    case TAB_SHOP: {
        const ShopDef *shp = &c->db->shops[m->shop];
        text(shp->keeper, dx, dy - 4 * S, 14 * S, (Color){255, 220, 120, 255});
        if (!m->sell_mode) {
            ItemStack tmp = {(int16_t)(selid >= 0 ? selid : 0), 1, 0, -1};
            if (selid >= 0) {
                draw_item_details(c, &tmp, dx, dy + 18 * S, dw);
                bool can = c->inv->gold >= shop_buy_price(c->db, selid, 1.0f);
                if (button((Rectangle){dx, by, bw, bh}, "Buy 1", can, false) || (enter && can)) {
                    ShopResult r = shop_buy(c->inv, c->db, m->shop, selid, 1, 1.0f);
                    say(m, "%s", r == SH_OK ? "Bought." : r == SH_SPACE ? "No room in the bag." : "Not enough gold.");
                }
                if (button((Rectangle){dx + bw + 10 * S, by, 110 * S, bh}, "Buy 5", c->inv->gold >= 5 * shop_buy_price(c->db, selid, 1.0f) && c->db->items[selid].stack > 1, false)) {
                    ShopResult r = shop_buy(c->inv, c->db, m->shop, selid, 5, 1.0f);
                    say(m, "%s", r == SH_OK ? "Bought 5." : r == SH_SPACE ? "No room in the bag." : "Not enough gold.");
                }
            }
        } else if (selid >= 0) {
            ItemStack *st = &c->inv->slot[selid];
            draw_item_details(c, st, dx, dy + 18 * S, dw);
            bool buys = (shp->buys_types & (1u << c->db->items[st->item].type)) != 0 && shop_sell_price(c->db, st, 1.0f) > 0;
            if (button((Rectangle){dx, by, bw, bh}, "Sell 1", buys, false) || (enter && buys)) { shop_sell(c->inv, c->db, m->shop, selid, 1, 1.0f); say(m, "%s", "Sold."); }
            if (button((Rectangle){dx + bw + 10 * S, by, 130 * S, bh}, "Sell all", buys && st->qty > 1, false)) { shop_sell(c->inv, c->db, m->shop, selid, st->qty, 1.0f); say(m, "%s", "Sold."); }
            if (!buys) text("This shopkeeper won't buy that.", dx, by - 22 * S, 13 * S, (Color){255, 150, 130, 255});
        }
    } break;
    }
    if (m->msg_t > 0) text(m->msg, px + 16 * S, py + ph - 56 * S, 16 * S, (Color){255, 240, 170, 255});
    text("Tab: next tab   Arrows: select / member   Enter: action   Esc or I: close", px + 16 * S, py + ph - 24 * S, 12 * S, (Color){150, 160, 190, 255});
    clamp_sel(m);
}
