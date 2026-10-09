#include "core/inventory.h"
#include <math.h>
#include <string.h>

void inv_init(Inventory *inv) {
    memset(inv, 0, sizeof *inv);
    for (int i = 0; i < INV_SLOTS; ++i) inv->slot[i] = (ItemStack){-1, 0, 0, -1};
    for (int m = 0; m < INV_MEMBERS; ++m) for (int s = 0; s < EQ_COUNT; ++s) inv->equip[m][s] = (ItemStack){-1, 0, 0, -1};
}

void inv_update(Inventory *inv, float dt) { if (inv->use_cd > 0) inv->use_cd -= dt; }

int inv_free_slots(const Inventory *inv) { int n = 0; for (int i = 0; i < INV_SLOTS; ++i) if (inv->slot[i].item < 0) n++; return n; }

int inv_count(const Inventory *inv, int item) {
    int n = 0;
    for (int i = 0; i < INV_SLOTS; ++i) if (inv->slot[i].item == item) n += inv->slot[i].qty;
    return n;
}

static int free_slot(const Inventory *inv) { for (int i = 0; i < INV_SLOTS; ++i) if (inv->slot[i].item < 0) return i; return -1; }

bool inv_add_instance(Inventory *inv, const ItemDB *db, int item, int upgrade, int enchant) {
    if (item < 0 || item >= db->nitems) return false;
    int s = free_slot(inv);
    if (s < 0) return false;
    inv->slot[s] = (ItemStack){(int16_t)item, 1, (uint8_t)upgrade, (int8_t)enchant};
    return true;
}

int inv_add(Inventory *inv, const ItemDB *db, int item, int qty) {
    if (item < 0 || item >= db->nitems || qty <= 0) return qty > 0 ? qty : 0;
    const ItemDef *d = &db->items[item];
    if (item_is_equipment(d)) {
        while (qty > 0 && inv_add_instance(inv, db, item, 0, -1)) qty--;
        return qty;
    }
    for (int i = 0; i < INV_SLOTS && qty > 0; ++i) { /* top up existing stacks first */
        ItemStack *s = &inv->slot[i];
        if (s->item != item || s->qty >= d->stack) continue;
        int room = d->stack - s->qty, put = qty < room ? qty : room;
        s->qty = (int16_t)(s->qty + put); qty -= put;
    }
    while (qty > 0) {
        int f = free_slot(inv);
        if (f < 0) break;
        int put = qty < d->stack ? qty : d->stack;
        inv->slot[f] = (ItemStack){(int16_t)item, (int16_t)put, 0, -1};
        qty -= put;
    }
    return qty;
}

bool inv_remove(Inventory *inv, const ItemDB *db, int item, int qty) {
    (void)db;
    if (qty <= 0) return true;
    if (inv_count(inv, item) < qty) return false;
    for (int i = INV_SLOTS - 1; i >= 0 && qty > 0; --i) { /* drain from the back so earlier stacks stay full */
        ItemStack *s = &inv->slot[i];
        if (s->item != item) continue;
        int take = qty < s->qty ? qty : s->qty;
        s->qty = (int16_t)(s->qty - take); qty -= take;
        if (s->qty == 0) *s = (ItemStack){-1, 0, 0, -1};
    }
    return true;
}

/* ---------------- equipment ---------------- */
float item_upgrade_mult(int upgrade) { return 1.0f + 0.07f * (float)upgrade; }

EqResult inv_equip(Inventory *inv, const ItemDB *db, int member, Role role, int level, int inv_slot, int acc_slot) {
    if (inv_slot < 0 || inv_slot >= INV_SLOTS || inv->slot[inv_slot].item < 0) return EQ_ERR_EMPTY;
    ItemStack st = inv->slot[inv_slot];
    const ItemDef *d = &db->items[st.item];
    if (!item_is_equipment(d)) return EQ_ERR_TYPE;
    if (!d->any_role && d->role != role) return EQ_ERR_ROLE;
    if (d->level > level) return EQ_ERR_LEVEL;
    EquipSlot es = d->slot;
    if (d->type == IT_ACCESSORY) es = acc_slot == 1 ? EQ_ACC2 : EQ_ACC1;
    ItemStack old = inv->equip[member][es];
    inv->equip[member][es] = st;
    inv->slot[inv_slot] = old.item >= 0 ? old : (ItemStack){-1, 0, 0, -1}; /* swap into the vacated slot: never fails for space */
    return EQ_OK;
}

EqResult inv_unequip(Inventory *inv, const ItemDB *db, int member, EquipSlot slot) {
    (void)db;
    ItemStack st = inv->equip[member][slot];
    if (st.item < 0) return EQ_ERR_EMPTY;
    int f = free_slot(inv);
    if (f < 0) return EQ_ERR_NOSPACE;
    inv->slot[f] = st;
    inv->equip[member][slot] = (ItemStack){-1, 0, 0, -1};
    return EQ_OK;
}

static void add_stats(ItemStats *t, const ItemStats *s, float mult, float flat_mult) {
    t->atk += s->atk * mult; t->matk += s->matk * mult; t->armor += s->armor * mult; t->hp += s->hp * mult; t->mp += s->mp * mult;
    t->str += s->str * flat_mult; t->intel += s->intel * flat_mult; t->vit += s->vit * flat_mult; t->wis += s->wis * flat_mult; t->agi += s->agi * flat_mult; t->crit += s->crit * flat_mult;
}

ItemStats inv_gear_stats(const Inventory *inv, const ItemDB *db, int member) {
    ItemStats t = {0};
    for (int s = 0; s < EQ_COUNT; ++s) {
        const ItemStack *st = &inv->equip[member][s];
        if (st->item < 0) continue;
        add_stats(&t, &db->items[st->item].st, item_upgrade_mult(st->upgrade), 1.0f);
        if (st->enchant >= 0) add_stats(&t, &db->enchants[st->enchant].st, 1.0f, 1.0f);
    }
    return t;
}

void inv_apply_gear(const Inventory *inv, const ItemDB *db, int member, Combatant *c) {
    ItemStats t = inv_gear_stats(inv, db, member);
    c->weapon_atk = t.atk; c->weapon_matk = t.matk; c->armor = t.armor;
    c->gear.str = t.str; c->gear.intel = t.intel; c->gear.vit = t.vit; c->gear.wis = t.wis; c->gear.agi = t.agi;
    c->gear_hp = t.hp; c->gear_mp = t.mp; c->gear_crit = t.crit;
    for (int k = 0; k < EL_COUNT; ++k) c->elem_boost[k] = 1.0f;
    for (int s = 0; s < EQ_COUNT; ++s) {
        const ItemStack *st = &inv->equip[member][s];
        if (st->item < 0 || st->enchant < 0) continue;
        const EnchantDef *e = &db->enchants[st->enchant];
        if (e->element_boost > 0 && 1.0f + e->element_boost > c->elem_boost[e->element]) c->elem_boost[e->element] = 1.0f + e->element_boost;
    }
    combatant_recalc(c);
}

/* ---------------- smithing ---------------- */
UpgradeCost smith_upgrade_cost(const ItemDB *db, const ItemStack *s) {
    UpgradeCost c = {0};
    if (!s || s->item < 0) return c;
    const ItemDef *d = &db->items[s->item];
    if (d->type != IT_WEAPON && d->type != IT_ARMOR) return c;
    if (s->upgrade >= MAX_UPGRADE || d->upgrade_mat < 0) return c;
    int lv = s->upgrade;
    float base = (float)(d->buy > 0 ? d->buy : 400 + 300 * d->tier);
    c.gold = (int)fmaxf(25.0f * (float)(lv + 1), base * 0.12f * powf((float)(lv + 1), 1.4f));
    c.mat = d->upgrade_mat; c.mat_qty = 2 * (lv + 1);
    c.ok = true;
    return c;
}

bool smith_upgrade(Inventory *inv, const ItemDB *db, ItemStack *target) {
    UpgradeCost c = smith_upgrade_cost(db, target);
    if (!c.ok || inv->gold < c.gold || inv_count(inv, c.mat) < c.mat_qty) return false;
    inv->gold -= c.gold;
    inv_remove(inv, db, c.mat, c.mat_qty);
    target->upgrade++;
    return true;
}

int smith_enchant_gold(const ItemDB *db, const ItemStack *s) {
    if (!s || s->item < 0) return 0;
    return 120 + 90 * db->items[s->item].tier * db->items[s->item].tier;
}

EnchantResult smith_enchant(Inventory *inv, const ItemDB *db, ItemStack *target, int enchant) {
    if (!target || target->item < 0 || enchant < 0 || enchant >= db->nenchants) return EN_ERR_TARGET;
    const ItemDef *d = &db->items[target->item];
    const EnchantDef *e = &db->enchants[enchant];
    if (!(e->applies & (1u << d->type))) return EN_ERR_TYPE;
    if (d->tier < e->min_tier && d->type != IT_ACCESSORY) return EN_ERR_TIER;
    if (target->enchant == enchant) return EN_ERR_SAME;
    int gold = smith_enchant_gold(db, target);
    if (inv_count(inv, e->essence) < 1) return EN_ERR_MATERIALS;
    if (inv->gold < gold) return EN_ERR_GOLD;
    inv->gold -= gold;
    inv_remove(inv, db, e->essence, 1);
    target->enchant = (int8_t)enchant; /* replaces any previous enchant */
    return EN_OK;
}

/* ---------------- crafting ---------------- */
CraftResult craft_check(const Inventory *inv, const ItemDB *db, int recipe, unsigned stations, int level) {
    const RecipeDef *r = &db->recipes[recipe];
    if (!(stations & r->station)) return CR_NO_STATION;
    if (level < r->level) return CR_LEVEL;
    for (int i = 0; i < r->nin; ++i) if (inv_count(inv, r->in[i].item) < r->in[i].qty) return CR_MATERIALS;
    if (inv->gold < r->gold) return CR_GOLD;
    return CR_OK;
}

CraftResult craft_do(Inventory *inv, const ItemDB *db, int recipe, unsigned stations, int level) {
    CraftResult c = craft_check(inv, db, recipe, stations, level);
    if (c != CR_OK) return c;
    const RecipeDef *r = &db->recipes[recipe];
    /* make room first: removing inputs may free slots, so test on a copy */
    Inventory trial = *inv;
    for (int i = 0; i < r->nin; ++i) inv_remove(&trial, db, r->in[i].item, r->in[i].qty);
    if (inv_add(&trial, db, r->output, r->qty) > 0) return CR_SPACE;
    trial.gold -= r->gold;
    *inv = trial;
    return CR_OK;
}

/* ---------------- shops ---------------- */
int shop_buy_price(const ItemDB *db, int item, float rep_mult) { int p = (int)ceilf((float)db->items[item].buy * rep_mult); return p < 1 && db->items[item].buy > 0 ? 1 : p; }

int shop_sell_price(const ItemDB *db, const ItemStack *s, float sell_mult) {
    const ItemDef *d = &db->items[s->item];
    if (d->type == IT_KEY) return 0;
    float v = (float)d->sell * (d->type == IT_WEAPON || d->type == IT_ARMOR ? item_upgrade_mult(s->upgrade) : 1.0f);
    if (s->enchant >= 0) v += 60.0f;
    return (int)(v * sell_mult);
}

ShopResult shop_buy(Inventory *inv, const ItemDB *db, int shop, int item, int qty, float rep_mult) {
    const ShopDef *sh = &db->shops[shop];
    bool sold = false;
    for (int i = 0; i < sh->nsells; ++i) if (sh->sells[i] == item) sold = true;
    if (!sold) return SH_NOT_SOLD;
    long cost = (long)shop_buy_price(db, item, rep_mult) * qty;
    if (inv->gold < cost) return SH_GOLD;
    Inventory trial = *inv;
    if (inv_add(&trial, db, item, qty) > 0) return SH_SPACE;
    trial.gold -= (int)cost;
    *inv = trial;
    return SH_OK;
}

ShopResult shop_sell(Inventory *inv, const ItemDB *db, int shop, int inv_slot, int qty, float sell_mult) {
    if (inv_slot < 0 || inv_slot >= INV_SLOTS || inv->slot[inv_slot].item < 0) return SH_NOITEM;
    ItemStack *s = &inv->slot[inv_slot];
    const ItemDef *d = &db->items[s->item];
    if (!(db->shops[shop].buys_types & (1u << d->type))) return SH_NOT_BOUGHT;
    int price = shop_sell_price(db, s, sell_mult);
    if (price <= 0) return SH_UNSELLABLE;
    if (qty > s->qty) qty = s->qty;
    inv->gold += price * qty;
    s->qty = (int16_t)(s->qty - qty);
    if (s->qty <= 0) *s = (ItemStack){-1, 0, 0, -1};
    return SH_OK;
}

/* ---------------- consumables ---------------- */
UseResult inv_use(Inventory *inv, const ItemDB *db, CombatWorld *cw, Combatant *t, int slot) {
    if (slot < 0 || slot >= INV_SLOTS || inv->slot[slot].item < 0) return USE_NOT_USABLE;
    const ItemDef *d = &db->items[inv->slot[slot].item];
    if (d->type != IT_CONSUMABLE) return USE_NOT_USABLE;
    if (inv->use_cd > 0) return USE_COOLDOWN;
    if (!t) return USE_BAD_TARGET;
    bool revive = d->revive > 0;
    if (revive != t->dead) return USE_BAD_TARGET;      /* revives need a corpse, everything else a living target */
    bool changed = false;
    if (revive) {
        t->dead = false; t->dead_t = 0; t->hp = t->max_hp * d->revive; t->stagger_t = 0; changed = true;
    } else {
        bool cursed = t->status[ST_HOLLOW_CURSE].time > 0;
        if (d->heal_hp > 0 && t->hp < t->max_hp && !cursed) { t->hp = fminf(t->max_hp, t->hp + d->heal_hp); changed = true; }
        if (d->heal_mp > 0 && t->mp < t->max_mp) { t->mp = fminf(t->max_mp, t->mp + d->heal_mp); changed = true; }
        if (d->cleanse) {
            static const StatusKind bad[] = {ST_BURN, ST_POISON, ST_SLOW, ST_STUN, ST_SILENCE, ST_DEF_DOWN};
            for (unsigned k = 0; k < sizeof bad / sizeof bad[0]; ++k) if (t->status[bad[k]].time > 0) { t->status[bad[k]].time = 0; changed = true; }
            if (d->rarity >= RAR_RARE && cursed) { t->status[ST_HOLLOW_CURSE].time = 0; changed = true; }
        }
        if (d->status != ST_NONE) { combat_apply_status(cw, t, d->status, d->status_time, d->status_power, t->ent); changed = true; }
    }
    if (!changed) return USE_NO_EFFECT;
    inv->use_cd = d->cooldown;
    inv_remove(inv, db, inv->slot[slot].item, 1);
    return USE_OK;
}

bool tome_learn(const ItemDB *db, const SkillDB *skills, PartyDef *m, int item) {
    const ItemDef *d = &db->items[item];
    if (d->type != IT_TOME || (!d->any_role && d->role != m->role)) return false;
    int sk = skilldb_find(skills, d->teach);
    if (sk < 0 || m->nskills >= 16) return false;
    for (int i = 0; i < m->nskills; ++i) if (m->skills[i] == sk) return false;
    m->skills[m->nskills++] = sk;
    return true;
}

int inv_quick_slot(const Inventory *inv, const ItemDB *db, int kind) {
    int best = -1; float bv = 1e9f;
    for (int i = 0; i < INV_SLOTS; ++i) {
        if (inv->slot[i].item < 0) continue;
        const ItemDef *d = &db->items[inv->slot[i].item];
        if (d->type != IT_CONSUMABLE) continue;
        float v = -1;
        if (kind == 0 && d->heal_hp > 0 && d->revive <= 0 && d->heal_mp <= 0 && d->status == ST_NONE) v = d->heal_hp;
        if (kind == 1 && d->heal_mp > 0 && d->revive <= 0 && d->heal_hp <= 0) v = d->heal_mp;
        if (kind == 2 && d->revive > 0) v = d->revive;
        if (kind == 3 && d->cleanse && d->heal_hp <= 0) v = (float)d->rarity;
        if (v >= 0 && v < bv) { bv = v; best = i; }
    }
    return best;
}

/* ---------------- loot ---------------- */
static float frand(uint32_t *s) { uint32_t x = *s ? *s : 2463534242u; x ^= x << 13; x ^= x >> 17; x ^= x << 5; *s = x; return (float)(x & 0xFFFFFF) / 16777216.0f; }

int loot_roll(const ItemDB *db, int enemy_def, uint32_t *rng, LootDrop out[12], int *gold) {
    int n = 0;
    *gold = 0;
    for (int i = 0; i < db->nloot; ++i) {
        const LootTable *l = &db->loot[i];
        if (l->enemy != enemy_def) continue;
        if (l->gold_max >= l->gold_min && l->gold_max > 0) *gold = l->gold_min + (int)(frand(rng) * (float)(l->gold_max - l->gold_min + 1));
        for (int k = 0; k < l->ndrops && n < 12; ++k) {
            const DropDef *dr = &l->drops[k];
            if (frand(rng) >= dr->chance) continue;
            int q = dr->qmin + (int)(frand(rng) * (float)(dr->qmax - dr->qmin + 1));
            if (q < 1) q = 1;
            out[n++] = (LootDrop){dr->item, q};
        }
        break;
    }
    return n;
}

/* ---------------- persistence ---------------- */
typedef struct { uint8_t *p; size_t cap, n; bool over; } W;
static void wb(W *w, const void *d, size_t len) { if (w->over || w->n + len > w->cap) { w->over = true; return; } memcpy(w->p + w->n, d, len); w->n += len; }
static void wu8(W *w, unsigned v) { uint8_t b = (uint8_t)v; wb(w, &b, 1); }
static void wu16(W *w, unsigned v) { uint8_t b[2] = {(uint8_t)(v & 255), (uint8_t)(v >> 8)}; wb(w, b, 2); }
static void wu32(W *w, uint32_t v) { uint8_t b[4] = {(uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24)}; wb(w, b, 4); }
static void wstr(W *w, const char *s) { size_t l = strlen(s); wu8(w, (unsigned)l); wb(w, s, l); }
static void wstack(W *w, const ItemDB *db, const ItemStack *s) {
    wstr(w, db->items[s->item].id); wu16(w, (unsigned)s->qty); wu8(w, s->upgrade);
    wstr(w, s->enchant >= 0 ? db->enchants[s->enchant].id : "");
}

size_t inv_serialize(const Inventory *inv, const ItemDB *db, uint8_t *buf, size_t cap) {
    W w = {buf, cap, 0, false};
    wb(&w, "SINV", 4); wu32(&w, 1); wu32(&w, (uint32_t)inv->gold);
    unsigned used = 0; for (int i = 0; i < INV_SLOTS; ++i) if (inv->slot[i].item >= 0) used++;
    wu16(&w, used);
    for (int i = 0; i < INV_SLOTS; ++i) if (inv->slot[i].item >= 0) { wu16(&w, (unsigned)i); wstack(&w, db, &inv->slot[i]); }
    unsigned eq = 0; for (int m = 0; m < INV_MEMBERS; ++m) for (int s = 0; s < EQ_COUNT; ++s) if (inv->equip[m][s].item >= 0) eq++;
    wu16(&w, eq);
    for (int m = 0; m < INV_MEMBERS; ++m) for (int s = 0; s < EQ_COUNT; ++s) if (inv->equip[m][s].item >= 0) { wu8(&w, (unsigned)m); wu8(&w, (unsigned)s); wstack(&w, db, &inv->equip[m][s]); }
    return w.over ? 0 : w.n;
}

typedef struct { const uint8_t *p; size_t n, i; bool bad; } R;
static unsigned ru8(R *r) { if (r->i + 1 > r->n) { r->bad = true; return 0; } return r->p[r->i++]; }
static unsigned ru16(R *r) { unsigned a = ru8(r); unsigned b = ru8(r); return a | (b << 8); }
static uint32_t ru32(R *r) { uint32_t a = ru16(r); uint32_t b = ru16(r); return a | (b << 16); }
static void rstr(R *r, char *out, size_t cap) {
    unsigned l = ru8(r);
    if (r->bad || r->i + l > r->n || l >= cap) { r->bad = true; out[0] = 0; return; }
    memcpy(out, r->p + r->i, l); out[l] = 0; r->i += l;
}
static bool rstack(R *r, const ItemDB *db, ItemStack *out, bool *known) {
    char id[64], en[32];
    rstr(r, id, sizeof id); unsigned q = ru16(r); unsigned up = ru8(r); rstr(r, en, sizeof en);
    if (r->bad) return false;
    int item = itemdb_find(db, id);
    *known = item >= 0;
    int enchant = en[0] ? itemdb_find_enchant(db, en) : -1;
    if (item < 0) return true;
    if (up > MAX_UPGRADE) up = MAX_UPGRADE;
    *out = (ItemStack){(int16_t)item, (int16_t)q, (uint8_t)up, (int8_t)enchant};
    return true;
}

int inv_deserialize(Inventory *inv, const ItemDB *db, const uint8_t *buf, size_t len, int *dropped) {
    R r = {buf, len, 0, false};
    int drop = 0;
    if (len < 12 || memcmp(buf, "SINV", 4) != 0) return 1;
    r.i = 4;
    if (ru32(&r) != 1) return 2;
    Inventory tmp; inv_init(&tmp);
    tmp.gold = (int)ru32(&r);
    unsigned used = ru16(&r);
    for (unsigned k = 0; k < used && !r.bad; ++k) {
        unsigned idx = ru16(&r); ItemStack s; bool known = false;
        if (!rstack(&r, db, &s, &known)) break;
        if (idx >= INV_SLOTS) { r.bad = true; break; }
        if (known) tmp.slot[idx] = s; else drop++;
    }
    unsigned eq = ru16(&r);
    for (unsigned k = 0; k < eq && !r.bad; ++k) {
        unsigned m = ru8(&r), sl = ru8(&r); ItemStack s; bool known = false;
        if (!rstack(&r, db, &s, &known)) break;
        if (m >= INV_MEMBERS || sl >= EQ_COUNT) { r.bad = true; break; }
        if (known) tmp.equip[m][sl] = s; else drop++;
    }
    if (r.bad) return 3;
    *inv = tmp;
    if (dropped) *dropped = drop;
    return 0;
}
