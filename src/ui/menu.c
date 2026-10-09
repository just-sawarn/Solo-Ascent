#include "ui/menu.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <raylib.h>

enum { TAB_ITEMS, TAB_GEAR, TAB_CRAFT, TAB_SMITH, TAB_SHOP, TAB_COUNT };
static const char *TAB_NAMES[TAB_COUNT] = {"Items", "Gear", "Craft", "Smith", "Shop"};
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
    if (m->tab == TAB_SHOP) { /* shop selector / buy-sell toggle on top of the list */ }

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
    } else if (m->tab != TAB_ITEMS || true) {
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
