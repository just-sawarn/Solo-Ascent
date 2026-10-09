#ifndef SA_INVENTORY_H
#define SA_INVENTORY_H
/*
 * Inventory, equipment, smithing (upgrades + enchants), crafting, shops, consumables, loot and persistence.
 * Equipment pieces are instances (own slot, upgrade level, optional enchant); everything else stacks.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/combat.h"
#include "core/items.h"

#define INV_SLOTS 120
#define INV_MEMBERS 4
#define MAX_UPGRADE 8

typedef struct { int16_t item; int16_t qty; uint8_t upgrade; int8_t enchant; } ItemStack; /* item < 0: empty */

typedef struct {
    ItemStack slot[INV_SLOTS];
    ItemStack equip[INV_MEMBERS][EQ_COUNT];
    int gold;
    float use_cd;                /* shared consumable cooldown */
} Inventory;

void inv_init(Inventory *inv);
int  inv_add(Inventory *inv, const ItemDB *db, int item, int qty);                  /* returns the quantity that did NOT fit */
bool inv_add_instance(Inventory *inv, const ItemDB *db, int item, int upgrade, int enchant);
int  inv_count(const Inventory *inv, int item);
bool inv_remove(Inventory *inv, const ItemDB *db, int item, int qty);               /* all or nothing */
int  inv_free_slots(const Inventory *inv);
void inv_update(Inventory *inv, float dt);

typedef enum { EQ_OK, EQ_ERR_EMPTY, EQ_ERR_TYPE, EQ_ERR_ROLE, EQ_ERR_LEVEL, EQ_ERR_NOSPACE } EqResult;
EqResult inv_equip(Inventory *inv, const ItemDB *db, int member, Role role, int level, int inv_slot, int acc_slot);
EqResult inv_unequip(Inventory *inv, const ItemDB *db, int member, EquipSlot slot);
ItemStats inv_gear_stats(const Inventory *inv, const ItemDB *db, int member);       /* totals incl. upgrades and enchants */
void inv_apply_gear(const Inventory *inv, const ItemDB *db, int member, Combatant *c);
float item_upgrade_mult(int upgrade);

/* smithing */
typedef struct { bool ok; int gold; int mat, mat_qty; } UpgradeCost;
UpgradeCost smith_upgrade_cost(const ItemDB *db, const ItemStack *s);
bool smith_upgrade(Inventory *inv, const ItemDB *db, ItemStack *target);
typedef enum { EN_OK, EN_ERR_TARGET, EN_ERR_TYPE, EN_ERR_TIER, EN_ERR_MATERIALS, EN_ERR_GOLD, EN_ERR_SAME } EnchantResult;
int  smith_enchant_gold(const ItemDB *db, const ItemStack *s);
EnchantResult smith_enchant(Inventory *inv, const ItemDB *db, ItemStack *target, int enchant);

/* crafting: `stations` is a bit mask of the stations the player can use right now */
typedef enum { CR_OK, CR_NO_STATION, CR_LEVEL, CR_MATERIALS, CR_GOLD, CR_SPACE } CraftResult;
CraftResult craft_check(const Inventory *inv, const ItemDB *db, int recipe, unsigned stations, int level);
CraftResult craft_do(Inventory *inv, const ItemDB *db, int recipe, unsigned stations, int level);

/* shops: rep_mult < 1 is a discount (reputation), sell_mult scales what shopkeepers pay */
typedef enum { SH_OK, SH_GOLD, SH_SPACE, SH_NOT_SOLD, SH_NOT_BOUGHT, SH_UNSELLABLE, SH_NOITEM } ShopResult;
int  shop_buy_price(const ItemDB *db, int item, float rep_mult);
int  shop_sell_price(const ItemDB *db, const ItemStack *s, float sell_mult);
ShopResult shop_buy(Inventory *inv, const ItemDB *db, int shop, int item, int qty, float rep_mult);
ShopResult shop_sell(Inventory *inv, const ItemDB *db, int shop, int inv_slot, int qty, float sell_mult);

/* consumables and tomes */
typedef enum { USE_OK, USE_NO_EFFECT, USE_COOLDOWN, USE_BAD_TARGET, USE_NOT_USABLE } UseResult;
UseResult inv_use(Inventory *inv, const ItemDB *db, CombatWorld *cw, Combatant *target, int inv_slot);
/* Teaches the tome's skill to a party member when the role matches. Returns false if already known / wrong role. */
bool tome_learn(const ItemDB *db, const SkillDB *skills, PartyDef *member, int item);
/* kind: 0 HP potion, 1 MP potion, 2 revive, 3 cleanse. Returns the inventory slot to use, -1 if none owned. */
int  inv_quick_slot(const Inventory *inv, const ItemDB *db, int kind);

/* loot */
typedef struct { int item, qty; } LootDrop;
int loot_roll(const ItemDB *db, int enemy_def, uint32_t *rng, LootDrop out[12], int *gold);

/* persistence: item ids are stored as strings so saves survive content re-ordering */
size_t inv_serialize(const Inventory *inv, const ItemDB *db, uint8_t *buf, size_t cap);
/* returns 0 on success, nonzero on a corrupt/unsupported blob; `dropped` counts entries with unknown ids */
int  inv_deserialize(Inventory *inv, const ItemDB *db, const uint8_t *buf, size_t len, int *dropped);

#endif
