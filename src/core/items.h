#ifndef SA_ITEMS_H
#define SA_ITEMS_H
/* Item content tables (items, enchants, recipes, shops, loot) loaded from the data directory and cross-validated. */
#include <stdbool.h>
#include "core/data.h"

typedef enum { IT_WEAPON, IT_ARMOR, IT_ACCESSORY, IT_CONSUMABLE, IT_MATERIAL, IT_TOME, IT_KEY } ItemType;
typedef enum { RAR_COMMON, RAR_UNCOMMON, RAR_RARE, RAR_EPIC, RAR_LEGENDARY } Rarity;
typedef enum { EQ_WEAPON, EQ_HEAD, EQ_BODY, EQ_BOOTS, EQ_ACC1, EQ_ACC2, EQ_COUNT } EquipSlot;

typedef struct { float atk, matk, armor, hp, mp, str, intel, vit, wis, agi, crit; } ItemStats;

typedef struct {
    char id[40], name[48], desc[128], model[16], upgrade_mat_id[40], teach[32], status_name[16];
    ItemType type; Rarity rarity;
    EquipSlot slot;            /* for equipment (accessories fit EQ_ACC1 or EQ_ACC2) */
    bool any_role; Role role;
    int tier, level, stack, buy, sell;
    ItemStats st;
    /* consumable effect */
    float heal_hp, heal_mp, status_power, status_time, revive, cooldown; bool cleanse; StatusKind status;
    int upgrade_mat;           /* item index, -1 if none */
} ItemDef;

typedef struct {
    char id[24], name[24];
    unsigned applies;          /* bit mask over ItemType */
    Element element; float element_boost;
    ItemStats st;
    int essence, min_tier;     /* essence: item index */
} EnchantDef;

typedef struct { int item, qty; } ItemQty;
typedef enum { ST_FORGE = 1, ST_ALCHEMY = 2, ST_COOKING = 4 } Station;
typedef struct { char id[40]; int output, qty; ItemQty in[6]; int nin; unsigned station; int gold, level; } RecipeDef;

typedef struct { char id[24], name[40], keeper[32], region[24]; int sells[48]; int nsells; unsigned buys_types; } ShopDef;
typedef struct { int item; float chance; int qmin, qmax; } DropDef;
typedef struct { int enemy; int gold_min, gold_max; DropDef drops[12]; int ndrops; } LootTable;

#define DB_MAX_ITEMS 320
#define DB_MAX_ENCHANTS 24
#define DB_MAX_RECIPES 160
#define DB_MAX_SHOPS 16
#define DB_MAX_LOOT 32

typedef struct {
    ItemDef items[DB_MAX_ITEMS]; int nitems;
    EnchantDef enchants[DB_MAX_ENCHANTS]; int nenchants;
    RecipeDef recipes[DB_MAX_RECIPES]; int nrecipes;
    ShopDef shops[DB_MAX_SHOPS]; int nshops;
    LootTable loot[DB_MAX_LOOT]; int nloot;
} ItemDB;

/* `dir` holds items.json, enchants.json, recipes.json, shops.json, loot.json. Enemies are needed to resolve loot tables. */
bool itemdb_load(ItemDB *db, const char *dir, const EnemyDB *enemies);
int  itemdb_find(const ItemDB *db, const char *id);
int  itemdb_find_recipe(const ItemDB *db, const char *id);
int  itemdb_find_shop(const ItemDB *db, const char *id);
int  itemdb_find_enchant(const ItemDB *db, const char *id);
const char *rarity_name(Rarity r);
bool item_is_equipment(const ItemDef *d);

#endif
