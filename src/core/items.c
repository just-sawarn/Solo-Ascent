#include "core/items.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>
#include "core/log.h"

static char *read_all(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { LOGE("cannot open %s", path); return NULL; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    char *b = malloc((size_t)n + 1);
    if (!b) { fclose(f); return NULL; }
    size_t got = fread(b, 1, (size_t)n, f); fclose(f); b[got] = 0;
    return b;
}

static float num(const cJSON *o, const char *k, float d) { const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, k); return cJSON_IsNumber(v) ? (float)v->valuedouble : d; }
static const char *str(const cJSON *o, const char *k) { const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, k); return cJSON_IsString(v) ? v->valuestring : ""; }
static void cp(char *dst, size_t cap, const char *src) { snprintf(dst, cap, "%s", src); }

static ItemStats read_stats(const cJSON *o) {
    return (ItemStats){num(o, "atk", 0), num(o, "matk", 0), num(o, "armor", 0), num(o, "hp", 0), num(o, "mp", 0), num(o, "str", 0),
                       num(o, "int", 0), num(o, "vit", 0), num(o, "wis", 0), num(o, "agi", 0), num(o, "crit", 0)};
}

static cJSON *load_json(const char *dir, const char *file) {
    char path[512];
    snprintf(path, sizeof path, "%s/%s", dir, file);
    char *t = read_all(path);
    if (!t) return NULL;
    cJSON *r = cJSON_Parse(t);
    free(t);
    if (!r) LOGE("%s: JSON parse error", path);
    return r;
}

static const char *TYPE_NAMES[] = {"weapon", "armor", "accessory", "consumable", "material", "tome", "key"};
static const char *RAR_NAMES[] = {"common", "uncommon", "rare", "epic", "legendary"};
static const char *SLOT_NAMES[] = {"weapon", "head", "body", "boots", "accessory"};
static const char *ROLE_NAMES2[] = {"hero", "tank", "mage", "healer"};

const char *rarity_name(Rarity r) { return RAR_NAMES[r]; }
bool item_is_equipment(const ItemDef *d) { return d->type == IT_WEAPON || d->type == IT_ARMOR || d->type == IT_ACCESSORY; }

static int idx(const char **names, int n, const char *s) { for (int i = 0; i < n; ++i) if (!strcmp(names[i], s)) return i; return -1; }

int itemdb_find(const ItemDB *db, const char *id) { for (int i = 0; i < db->nitems; ++i) if (!strcmp(db->items[i].id, id)) return i; return -1; }
int itemdb_find_recipe(const ItemDB *db, const char *id) { for (int i = 0; i < db->nrecipes; ++i) if (!strcmp(db->recipes[i].id, id)) return i; return -1; }
int itemdb_find_shop(const ItemDB *db, const char *id) { for (int i = 0; i < db->nshops; ++i) if (!strcmp(db->shops[i].id, id)) return i; return -1; }
int itemdb_find_enchant(const ItemDB *db, const char *id) { for (int i = 0; i < db->nenchants; ++i) if (!strcmp(db->enchants[i].id, id)) return i; return -1; }

bool itemdb_load(ItemDB *db, const char *dir, const EnemyDB *enemies) {
    memset(db, 0, sizeof *db);
    bool ok = true;
    cJSON *root = load_json(dir, "items.json");
    if (!root) return false;
    const cJSON *it;
    cJSON_ArrayForEach(it, cJSON_GetObjectItemCaseSensitive(root, "items")) {
        if (db->nitems >= DB_MAX_ITEMS) { LOGE("items: too many"); ok = false; break; }
        ItemDef *d = &db->items[db->nitems];
        cp(d->id, sizeof d->id, str(it, "id")); cp(d->name, sizeof d->name, str(it, "name")); cp(d->desc, sizeof d->desc, str(it, "desc")); cp(d->model, sizeof d->model, str(it, "model"));
        int type = idx(TYPE_NAMES, 7, str(it, "type")), rar = idx(RAR_NAMES, 5, str(it, "rarity"));
        if (!d->id[0] || type < 0 || rar < 0 || itemdb_find(db, d->id) >= 0) { LOGE("items: bad or duplicate entry '%s'", d->id); ok = false; continue; }
        d->type = (ItemType)type; d->rarity = (Rarity)rar;
        const char *slot = str(it, "slot");
        int sl = slot[0] ? idx(SLOT_NAMES, 5, slot) : 0;
        if (item_is_equipment(d) && sl < 0) { LOGE("items: '%s' has a bad slot", d->id); ok = false; }
        d->slot = sl == 4 ? EQ_ACC1 : (EquipSlot)(sl < 0 ? 0 : sl);
        const char *role = str(it, "role");
        d->any_role = !role[0] || !strcmp(role, "any");
        int ro = d->any_role ? 0 : idx(ROLE_NAMES2, 4, role);
        if (ro < 0) { LOGE("items: '%s' has a bad role", d->id); ok = false; ro = 0; }
        d->role = (Role)ro;
        const char *tier = str(it, "tier");
        d->tier = tier[0] == 't' ? atoi(tier + 1) : 0;
        d->level = (int)num(it, "level", 1); d->stack = (int)num(it, "stack", 1); d->buy = (int)num(it, "buy", 0); d->sell = (int)num(it, "sell", 0);
        if (d->stack < 1) d->stack = 1;
        if (item_is_equipment(d)) d->stack = 1;
        d->st = read_stats(cJSON_GetObjectItemCaseSensitive(it, "stats"));
        const cJSON *ef = cJSON_GetObjectItemCaseSensitive(it, "effect");
        d->heal_hp = num(ef, "hp", 0); d->heal_mp = num(ef, "mp", 0); d->status_power = num(ef, "status_power", 0); d->status_time = num(ef, "status_time", 0);
        d->revive = num(ef, "revive", 0); d->cooldown = num(ef, "cooldown", 1.5f); d->cleanse = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(ef, "cleanse"));
        cp(d->status_name, sizeof d->status_name, str(ef, "status")); cp(d->teach, sizeof d->teach, str(ef, "teach"));
        d->status = d->status_name[0] ? status_from_name(d->status_name) : ST_NONE;
        if (d->status_name[0] && d->status == ST_NONE) { LOGE("items: '%s' has an unknown status", d->id); ok = false; }
        cp(d->upgrade_mat_id, sizeof d->upgrade_mat_id, str(it, "upgrade_mat"));
        d->upgrade_mat = -1;
        db->nitems++;
    }
    cJSON_Delete(root);
    for (int i = 0; i < db->nitems; ++i) { /* second pass: references */
        ItemDef *d = &db->items[i];
        if (d->upgrade_mat_id[0]) { d->upgrade_mat = itemdb_find(db, d->upgrade_mat_id); if (d->upgrade_mat < 0) { LOGE("items: '%s' upgrade material '%s' unknown", d->id, d->upgrade_mat_id); ok = false; } }
    }

    root = load_json(dir, "enchants.json");
    if (!root) return false;
    cJSON_ArrayForEach(it, cJSON_GetObjectItemCaseSensitive(root, "enchants")) {
        if (db->nenchants >= DB_MAX_ENCHANTS) { ok = false; break; }
        EnchantDef *e = &db->enchants[db->nenchants];
        cp(e->id, sizeof e->id, str(it, "id")); cp(e->name, sizeof e->name, str(it, "name"));
        const cJSON *ap; cJSON_ArrayForEach(ap, cJSON_GetObjectItemCaseSensitive(it, "applies")) {
            int t = cJSON_IsString(ap) ? idx(TYPE_NAMES, 7, ap->valuestring) : -1;
            if (t >= 0) e->applies |= 1u << t;
        }
        const char *el = str(it, "element");
        if (el[0]) { bool eok; e->element = element_from_name(el, &eok); if (!eok) { LOGE("enchants: '%s' unknown element", e->id); ok = false; } e->element_boost = num(it, "element_boost", 0); }
        e->st = read_stats(cJSON_GetObjectItemCaseSensitive(it, "stats"));
        e->essence = itemdb_find(db, str(it, "essence")); e->min_tier = (int)num(it, "min_tier", 1);
        if (!e->id[0] || e->essence < 0 || !e->applies) { LOGE("enchants: '%s' invalid", e->id); ok = false; continue; }
        db->nenchants++;
    }
    cJSON_Delete(root);

    root = load_json(dir, "recipes.json");
    if (!root) return false;
    cJSON_ArrayForEach(it, cJSON_GetObjectItemCaseSensitive(root, "recipes")) {
        if (db->nrecipes >= DB_MAX_RECIPES) { LOGE("recipes: too many"); ok = false; break; }
        RecipeDef *r = &db->recipes[db->nrecipes];
        cp(r->id, sizeof r->id, str(it, "id"));
        r->output = itemdb_find(db, str(it, "output")); r->qty = (int)num(it, "qty", 1); r->gold = (int)num(it, "gold", 0); r->level = (int)num(it, "level", 1);
        const char *stn = str(it, "station");
        r->station = !strcmp(stn, "forge") ? ST_FORGE : !strcmp(stn, "alchemy") ? ST_ALCHEMY : !strcmp(stn, "cooking") ? ST_COOKING : 0;
        const cJSON *in; cJSON_ArrayForEach(in, cJSON_GetObjectItemCaseSensitive(it, "inputs")) {
            int item = itemdb_find(db, str(in, "item"));
            if (item < 0 || r->nin >= 6) { LOGE("recipes: '%s' has a bad input", r->id); ok = false; continue; }
            r->in[r->nin++] = (ItemQty){item, (int)num(in, "qty", 1)};
        }
        if (r->output < 0 || !r->station || r->nin == 0 || r->qty < 1) { LOGE("recipes: '%s' invalid", r->id); ok = false; continue; }
        db->nrecipes++;
    }
    cJSON_Delete(root);

    root = load_json(dir, "shops.json");
    if (!root) return false;
    cJSON_ArrayForEach(it, cJSON_GetObjectItemCaseSensitive(root, "shops")) {
        if (db->nshops >= DB_MAX_SHOPS) { ok = false; break; }
        ShopDef *s = &db->shops[db->nshops];
        cp(s->id, sizeof s->id, str(it, "id")); cp(s->name, sizeof s->name, str(it, "name")); cp(s->keeper, sizeof s->keeper, str(it, "keeper")); cp(s->region, sizeof s->region, str(it, "region"));
        const cJSON *sl; cJSON_ArrayForEach(sl, cJSON_GetObjectItemCaseSensitive(it, "sells")) {
            int item = cJSON_IsString(sl) ? itemdb_find(db, sl->valuestring) : -1;
            if (item < 0 || s->nsells >= 48) { LOGE("shops: '%s' sells unknown item '%s'", s->id, cJSON_IsString(sl) ? sl->valuestring : "?"); ok = false; continue; }
            s->sells[s->nsells++] = item;
        }
        cJSON_ArrayForEach(sl, cJSON_GetObjectItemCaseSensitive(it, "buys")) { int t = cJSON_IsString(sl) ? idx(TYPE_NAMES, 7, sl->valuestring) : -1; if (t >= 0) s->buys_types |= 1u << t; }
        db->nshops++;
    }
    cJSON_Delete(root);

    root = load_json(dir, "loot.json");
    if (!root) return false;
    cJSON_ArrayForEach(it, cJSON_GetObjectItemCaseSensitive(root, "loot")) {
        if (db->nloot >= DB_MAX_LOOT) { ok = false; break; }
        LootTable *l = &db->loot[db->nloot];
        l->enemy = enemies ? enemydb_find(enemies, str(it, "enemy")) : -1;
        const cJSON *g = cJSON_GetObjectItemCaseSensitive(it, "gold");
        l->gold_min = (int)(cJSON_IsArray(g) ? cJSON_GetArrayItem(g, 0)->valuedouble : 0); l->gold_max = (int)(cJSON_IsArray(g) ? cJSON_GetArrayItem(g, 1)->valuedouble : 0);
        const cJSON *dr; cJSON_ArrayForEach(dr, cJSON_GetObjectItemCaseSensitive(it, "drops")) {
            int item = itemdb_find(db, str(dr, "item"));
            const cJSON *q = cJSON_GetObjectItemCaseSensitive(dr, "qty");
            if (item < 0 || l->ndrops >= 12) { LOGE("loot: '%s' drops unknown item '%s'", str(it, "enemy"), str(dr, "item")); ok = false; continue; }
            l->drops[l->ndrops++] = (DropDef){item, num(dr, "chance", 0), cJSON_IsArray(q) ? (int)cJSON_GetArrayItem(q, 0)->valuedouble : 1, cJSON_IsArray(q) ? (int)cJSON_GetArrayItem(q, 1)->valuedouble : 1};
        }
        if (l->enemy < 0) { LOGE("loot: unknown enemy '%s'", str(it, "enemy")); ok = false; continue; }
        db->nloot++;
    }
    cJSON_Delete(root);
    return ok && db->nitems > 0;
}
