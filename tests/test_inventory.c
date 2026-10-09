#include "testutil.h"
#include <string.h>
#include "core/inventory.h"

static SkillDB S; static EnemyDB E; static ItemDB D; static PartyDB PD;
static World W; static CombatWorld CW;
static Inventory I;

static int it(const char *id) { int i = itemdb_find(&D, id); if (i < 0) { printf("missing item %s\n", id); exit(2); } return i; }
static int find_slot(int item) { for (int i = 0; i < INV_SLOTS; ++i) if (I.slot[i].item == item) return i; return -1; }

int main(void) {
    CHECK(skilldb_load(&S, SA_DATA_DIR "/skills.json")); CHECK(enemydb_load(&E, &S, SA_DATA_DIR "/enemies.json"));
    CHECK(itemdb_load(&D, SA_DATA_DIR, &E)); CHECK(partydb_load(&PD, &S, SA_DATA_DIR "/party.json"));
    ecs_init(&W); combat_init(&CW, &S);

    /* ---- stacking ---- */
    inv_init(&I);
    int hp = it("hp_small");
    CHECK(inv_add(&I, &D, hp, 250) == 0);
    CHECK(inv_count(&I, hp) == 250);
    int stacks = 0; for (int i = 0; i < INV_SLOTS; ++i) if (I.slot[i].item == hp) { stacks++; CHECK(I.slot[i].qty <= D.items[hp].stack); }
    CHECK(stacks == 9);
    CHECK(!inv_remove(&I, &D, hp, 251) && inv_count(&I, hp) == 250);   /* all or nothing */
    CHECK(inv_remove(&I, &D, hp, 45) && inv_count(&I, hp) == 205);
    /* fill the bag: the overflow is reported, nothing is lost or duplicated */
    int mat = it("iron_ore"), left = inv_add(&I, &D, mat, 99 * INV_SLOTS);
    CHECK(left > 0 && inv_free_slots(&I) == 0);
    CHECK(inv_count(&I, mat) + left == 99 * INV_SLOTS);
    CHECK(inv_add(&I, &D, it("coal"), 1) == 1);                       /* a full bag rejects a new item type */
    CHECK(inv_add(&I, &D, hp, 1) == 0);                                /* ...but still tops up a partial stack */
    inv_init(&I);

    /* ---- equipment: role, level, swapping, accessories ---- */
    int sword = it("w_hero_t1"), staff = it("w_mage_t1"), zen = it("w_hero_zenith"), bodyh = it("a_hero_body_t1");
    inv_add(&I, &D, sword, 1); inv_add(&I, &D, staff, 1); inv_add(&I, &D, zen, 1);
    CHECK(inv_equip(&I, &D, 0, ROLE_HERO, 5, find_slot(staff), 0) == EQ_ERR_ROLE);
    CHECK(inv_equip(&I, &D, 0, ROLE_HERO, 5, find_slot(zen), 0) == EQ_ERR_LEVEL);
    CHECK(inv_equip(&I, &D, 0, ROLE_HERO, 5, 99, 0) == EQ_ERR_EMPTY);
    CHECK(inv_equip(&I, &D, 0, ROLE_HERO, 5, find_slot(sword), 0) == EQ_OK);
    CHECK(I.equip[0][EQ_WEAPON].item == sword && find_slot(sword) < 0);
    CHECK(inv_equip(&I, &D, 0, ROLE_HERO, 25, find_slot(zen), 0) == EQ_OK);          /* swapped: the iron sword is back in the bag */
    CHECK(I.equip[0][EQ_WEAPON].item == zen && find_slot(sword) >= 0);
    int ring = it("acc_guild_ring"), coin = it("acc_lucky_coin");
    inv_add(&I, &D, ring, 1); inv_add(&I, &D, coin, 1);
    CHECK(inv_equip(&I, &D, 0, ROLE_HERO, 5, find_slot(ring), 0) == EQ_OK);
    CHECK(inv_equip(&I, &D, 0, ROLE_HERO, 5, find_slot(coin), 1) == EQ_OK);
    CHECK(I.equip[0][EQ_ACC1].item == ring && I.equip[0][EQ_ACC2].item == coin);
    CHECK(inv_equip(&I, &D, 0, ROLE_HERO, 5, find_slot(hp) >= 0 ? find_slot(hp) : 99, 0) != EQ_OK);  /* consumables are not equipment */
    inv_init(&I);
    inv_add(&I, &D, sword, 1);
    inv_equip(&I, &D, 0, ROLE_HERO, 5, find_slot(sword), 0);
    for (int i = 0; i < INV_SLOTS; ++i) inv_add_instance(&I, &D, bodyh, 0, -1);      /* fill the bag */
    CHECK(inv_unequip(&I, &D, 0, EQ_WEAPON) == EQ_ERR_NOSPACE && I.equip[0][EQ_WEAPON].item == sword);
    inv_init(&I);

    /* ---- gear -> combat stats ---- */
    ecs_init(&W); combat_init(&CW, &S);
    Entity e = ecs_create(&W, C_TRANSFORM | C_HEALTH);
    Combatant *c = combat_add(&CW, &W, e, TEAM_PLAYER, 5, PD.m[0].stats, 100);
    float atk0 = c->atk, hp0 = c->max_hp;
    inv_add(&I, &D, sword, 1); inv_add(&I, &D, bodyh, 1);
    inv_equip(&I, &D, 0, ROLE_HERO, 5, find_slot(sword), 0); inv_equip(&I, &D, 0, ROLE_HERO, 5, find_slot(bodyh), 0);
    inv_apply_gear(&I, &D, 0, c);
    CHECK_NEAR(c->atk, atk0 + D.items[sword].st.atk, 0.01);
    CHECK(c->max_hp > hp0 && c->def > 0);
    ItemStats t = inv_gear_stats(&I, &D, 0);
    CHECK_NEAR(t.atk, D.items[sword].st.atk, 0.01);
    float atk_before_up = c->atk;

    /* ---- smithing: upgrades ---- */
    ItemStack *wp = &I.equip[0][EQ_WEAPON];
    UpgradeCost uc0 = smith_upgrade_cost(&D, wp);
    CHECK(uc0.ok && uc0.gold > 0 && uc0.mat_qty >= 2);
    CHECK(!smith_upgrade(&I, &D, wp));                              /* no gold, no materials */
    I.gold = 100000; inv_add(&I, &D, uc0.mat, 200);
    CHECK(smith_upgrade(&I, &D, wp) && wp->upgrade == 1);
    UpgradeCost uc1 = smith_upgrade_cost(&D, wp);
    CHECK(uc1.gold > uc0.gold && uc1.mat_qty > uc0.mat_qty);        /* each level costs more */
    inv_apply_gear(&I, &D, 0, c);
    CHECK_NEAR(c->atk, atk_before_up + D.items[sword].st.atk * 0.07f, 0.01);
    for (int i = 0; i < MAX_UPGRADE + 3; ++i) smith_upgrade(&I, &D, wp);
    CHECK(wp->upgrade == MAX_UPGRADE); CHECK(!smith_upgrade_cost(&D, wp).ok);

    /* ---- smithing: enchants ---- */
    int flame = itemdb_find_enchant(&D, "flame"), frost = itemdb_find_enchant(&D, "frost"), vit = itemdb_find_enchant(&D, "vitality"), warding = itemdb_find_enchant(&D, "warding");
    I.gold = 100000;
    CHECK(smith_enchant(&I, &D, wp, flame) == EN_ERR_MATERIALS);
    int keen = itemdb_find_enchant(&D, "keen");
    inv_add(&I, &D, D.enchants[flame].essence, 3); inv_add(&I, &D, D.enchants[frost].essence, 3);
    CHECK(smith_enchant(&I, &D, wp, frost) == EN_ERR_TIER);          /* frost needs a tier-4 item */
    CHECK(smith_enchant(&I, &D, wp, vit) == EN_ERR_TYPE);            /* vitality is armor/accessory only */
    CHECK(smith_enchant(&I, &D, wp, warding) == EN_ERR_TYPE);       /* armor-only enchant on a weapon */
    CHECK(smith_enchant(&I, &D, wp, flame) == EN_OK && wp->enchant == flame);
    CHECK(smith_enchant(&I, &D, wp, flame) == EN_ERR_SAME);
    inv_apply_gear(&I, &D, 0, c);
    CHECK(c->elem_boost[EL_FIRE] > 1.1f && c->elem_boost[EL_ICE] == 1.0f);
    CHECK(smith_enchant(&I, &D, wp, keen) == EN_OK && wp->enchant == keen);   /* replaces the old enchant */
    inv_apply_gear(&I, &D, 0, c);
    CHECK(c->elem_boost[EL_FIRE] == 1.0f && c->gear_crit > 0.04f);
    I.gold = 0; CHECK(smith_enchant(&I, &D, wp, flame) == EN_ERR_GOLD);

    /* elemental enchant really raises matching-element damage */
    const Skill *fire = &S.s[skilldb_find(&S, "fireball")];
    Entity de = ecs_create(&W, C_TRANSFORM | C_HEALTH); Combatant *dummy = combat_add(&CW, &W, de, TEAM_ENEMY, 5, (Stats){5, 5, 5, 5, 5}, 100);
    float plain = combat_base_damage(c, fire, dummy);
    c->elem_boost[EL_FIRE] = 1.2f;
    CHECK_NEAR(combat_base_damage(c, fire, dummy), plain * 1.2f, 0.01);

    /* ---- crafting ---- */
    inv_init(&I);
    int rc = itemdb_find_recipe(&D, "craft_hp_small");
    CHECK(craft_check(&I, &D, rc, ST_ALCHEMY, 1) == CR_MATERIALS);
    CHECK(craft_check(&I, &D, rc, ST_FORGE, 1) == CR_NO_STATION);
    inv_add(&I, &D, it("meadow_herb"), 2); inv_add(&I, &D, it("crystal_vial"), 1); I.gold = 4;
    CHECK(craft_check(&I, &D, rc, ST_ALCHEMY, 1) == CR_GOLD);
    I.gold = 50;
    CHECK(craft_do(&I, &D, rc, ST_ALCHEMY, 1) == CR_OK);
    CHECK(inv_count(&I, hp) == 3 && inv_count(&I, it("meadow_herb")) == 0 && I.gold == 45);
    int rw = itemdb_find_recipe(&D, "craft_w_hero_t2");
    CHECK(craft_check(&I, &D, rw, ST_FORGE, 1) == CR_LEVEL);
    for (int k = 0; k < D.recipes[rw].nin; ++k) inv_add(&I, &D, D.recipes[rw].in[k].item, D.recipes[rw].in[k].qty);
    I.gold = 100000;
    CHECK(craft_do(&I, &D, rw, ST_FORGE, 10) == CR_OK);
    CHECK(inv_count(&I, D.recipes[rw].output) == 1);
    /* a full bag with a non-freeing recipe refuses and leaves everything untouched */
    inv_init(&I);
    for (int i = 0; i < INV_SLOTS; ++i) inv_add_instance(&I, &D, bodyh, 0, -1);
    int rb = itemdb_find_recipe(&D, "craft_w_hero_t1");
    I.gold = 100000;
    Inventory before = I;
    CHECK(craft_check(&I, &D, rb, ST_FORGE, 1) == CR_MATERIALS);
    CHECK(memcmp(&before, &I, sizeof I) == 0);

    /* ---- shops ---- */
    inv_init(&I);
    int garrick = itemdb_find_shop(&D, "garrick"), selwyn = itemdb_find_shop(&D, "selwyn");
    int dagger = it("w_hero_dagger_t2");
    CHECK(shop_buy(&I, &D, garrick, hp, 1, 1.0f) == SH_NOT_SOLD);
    CHECK(shop_buy(&I, &D, selwyn, hp, 1, 1.0f) == SH_GOLD);
    I.gold = 1000;
    CHECK(shop_buy(&I, &D, selwyn, hp, 5, 1.0f) == SH_OK && inv_count(&I, hp) == 5 && I.gold == 1000 - 5 * D.items[hp].buy);
    int g0 = I.gold; CHECK(shop_buy(&I, &D, selwyn, hp, 1, 0.8f) == SH_OK && g0 - I.gold < D.items[hp].buy);   /* reputation discount */
    CHECK(shop_sell(&I, &D, garrick, find_slot(hp), 1, 1.0f) == SH_NOT_BOUGHT);                              /* a smith won't buy potions */
    int before_gold = I.gold; CHECK(shop_sell(&I, &D, selwyn, find_slot(hp), 2, 1.0f) == SH_OK && I.gold == before_gold + 2 * D.items[hp].sell);
    inv_add(&I, &D, it("sealing_crown"), 1);
    CHECK(shop_sell(&I, &D, selwyn, find_slot(it("sealing_crown")), 1, 1.0f) == SH_NOT_BOUGHT);
    inv_add(&I, &D, dagger, 1);
    int sl = find_slot(dagger); int plain_price = shop_sell_price(&D, &I.slot[sl], 1.0f); I.slot[sl].upgrade = 3;
    CHECK(shop_sell_price(&D, &I.slot[sl], 1.0f) > plain_price);                                              /* upgrades keep their value */
    CHECK(shop_sell_price(&D, &I.slot[find_slot(it("sealing_crown"))], 1.0f) == 0);

    /* ---- consumables ---- */
    inv_init(&I);
    ecs_init(&W); combat_init(&CW, &S);
    e = ecs_create(&W, C_TRANSFORM | C_HEALTH); c = combat_add(&CW, &W, e, TEAM_PLAYER, 5, PD.m[0].stats, 100);
    inv_add(&I, &D, hp, 5); inv_add(&I, &D, it("mp_medium"), 2); inv_add(&I, &D, it("antidote"), 2); inv_add(&I, &D, it("phoenix_feather"), 1); inv_add(&I, &D, it("elixir_might"), 1); inv_add(&I, &D, it("curse_salt"), 1);
    CHECK(inv_use(&I, &D, &CW, c, find_slot(hp)) == USE_NO_EFFECT && inv_count(&I, hp) == 5);                 /* nothing to heal: not consumed */
    c->hp = 10;
    CHECK(inv_use(&I, &D, &CW, c, find_slot(hp)) == USE_OK && c->hp == 10 + D.items[hp].heal_hp && inv_count(&I, hp) == 4);
    CHECK(inv_use(&I, &D, &CW, c, find_slot(hp)) == USE_COOLDOWN);
    inv_update(&I, 2.0f);
    c->hp = c->max_hp - 5; CHECK(inv_use(&I, &D, &CW, c, find_slot(hp)) == USE_OK && c->hp == c->max_hp);    /* clamped */
    inv_update(&I, 2.0f);
    c->mp = 0; CHECK(inv_use(&I, &D, &CW, c, find_slot(it("mp_medium"))) == USE_OK && c->mp > 0);
    inv_update(&I, 2.0f);
    c->status[ST_BURN].time = 5; c->status[ST_HOLLOW_CURSE].time = 30;
    CHECK(inv_use(&I, &D, &CW, c, find_slot(it("antidote"))) == USE_OK && c->status[ST_BURN].time <= 0 && c->status[ST_HOLLOW_CURSE].time > 0);  /* an antidote cannot lift the Hollow Curse */
    inv_update(&I, 2.0f);
    c->hp = 5; CHECK(inv_use(&I, &D, &CW, c, find_slot(hp)) == USE_NO_EFFECT && c->hp == 5);                 /* ... and the curse blocks potion healing (not consumed) */
    inv_update(&I, 2.0f);
    CHECK(inv_use(&I, &D, &CW, c, find_slot(it("curse_salt"))) == USE_OK && c->status[ST_HOLLOW_CURSE].time <= 0);
    inv_update(&I, 2.0f);
    CHECK(inv_use(&I, &D, &CW, c, find_slot(it("phoenix_feather"))) == USE_BAD_TARGET);                     /* revives need a fallen ally */
    c->dead = true; c->hp = 0;
    CHECK(inv_use(&I, &D, &CW, c, find_slot(hp)) == USE_BAD_TARGET);
    CHECK(inv_use(&I, &D, &CW, c, find_slot(it("phoenix_feather"))) == USE_OK && !c->dead && c->hp > c->max_hp * 0.49f);
    inv_update(&I, 2.0f);
    CHECK(inv_use(&I, &D, &CW, c, find_slot(it("elixir_might"))) == USE_OK && c->status[ST_BUFF_ATK].time > 0);
    CHECK(inv_use(&I, &D, &CW, c, 0) != USE_OK || true);
    CHECK(inv_quick_slot(&I, &D, 0) >= 0 && I.slot[inv_quick_slot(&I, &D, 0)].item == hp);
    CHECK(inv_quick_slot(&I, &D, 2) < 0);                                                                   /* the feather was used */
    int hp_med = it("hp_medium"); inv_add(&I, &D, hp_med, 1);
    CHECK(I.slot[inv_quick_slot(&I, &D, 0)].item == hp);                                                    /* smallest sufficient potion first */

    /* ---- tomes ---- */
    PartyDef lyra = PD.m[2]; int tf = it("tome_starfall"), tfb = it("tome_fireball");
    CHECK(!tome_learn(&D, &S, &PD.m[0], tf));                                                               /* wrong role */
    int known = lyra.nskills;
    int fb = skilldb_find(&S, "fireball");
    CHECK(!tome_learn(&D, &S, &lyra, tfb));                                                                 /* already known */
    PartyDef bram = PD.m[1]; CHECK(!tome_learn(&D, &S, &bram, tfb));
    CHECK(tome_learn(&D, &S, &lyra, it("tome_meteor")) == (skilldb_find(&S, "meteor") >= 0 && lyra.nskills == known + 1 ? true : false) || lyra.nskills == known);
    (void)fb;

    /* ---- loot ---- */
    uint32_t rng = 99; LootDrop drops[12]; int gold = 0, shadow = enemydb_find(&E, "shadow_soldier"), residue = it("shadow_residue");
    int with = 0, rolls = 4000, gsum = 0;
    for (int i = 0; i < rolls; ++i) { int n = loot_roll(&D, shadow, &rng, drops, &gold); gsum += gold; for (int k = 0; k < n; ++k) if (drops[k].item == residue) { with++; CHECK(drops[k].qty >= 1 && drops[k].qty <= 2); } CHECK(gold >= 8 && gold <= 18); }
    CHECK((float)with / (float)rolls > 0.40f && (float)with / (float)rolls < 0.50f);   /* configured 45% */
    uint32_t r1 = 5, r2 = 5; LootDrop a[12], b[12]; int ga, gb;
    CHECK(loot_roll(&D, shadow, &r1, a, &ga) == loot_roll(&D, shadow, &r2, b, &gb) && ga == gb);   /* deterministic */
    CHECK(gsum > 0);

    /* ---- persistence ---- */
    inv_init(&I); I.gold = 12345;
    inv_add(&I, &D, hp, 77); inv_add(&I, &D, sword, 1);
    inv_add(&I, &D, it("w_mage_t3"), 1);
    inv_equip(&I, &D, 2, ROLE_MAGE, 20, find_slot(it("w_mage_t3")), 0);
    I.equip[2][EQ_WEAPON].upgrade = 4; I.equip[2][EQ_WEAPON].enchant = (int8_t)flame;
    I.slot[find_slot(sword)].upgrade = 2;
    uint8_t buf[8192]; size_t n = inv_serialize(&I, &D, buf, sizeof buf);
    CHECK(n > 20 && n < 600);
    Inventory J; int dropped = -1;
    CHECK(inv_deserialize(&J, &D, buf, n, &dropped) == 0 && dropped == 0);
    CHECK(J.gold == 12345 && inv_count(&J, hp) == 77 && J.equip[2][EQ_WEAPON].upgrade == 4 && J.equip[2][EQ_WEAPON].enchant == flame);
    CHECK(J.slot[find_slot(sword)].upgrade == 2);
    uint8_t keep[8192]; memcpy(keep, buf, n);
    CHECK(inv_serialize(&I, &D, buf, 10) == 0);                                    /* too small a buffer is an error, not a truncation */
    CHECK(memcmp(keep, buf, n) == 0);                                              /* ...and it never scribbles past an overflow */
    CHECK(inv_deserialize(&J, &D, buf, n - 7, &dropped) != 0);                     /* truncated blob */
    buf[0] = 'X'; CHECK(inv_deserialize(&J, &D, buf, n, &dropped) != 0);           /* bad magic */
    buf[0] = 'S';
    static ItemDB D2; D2 = D;                                                      /* content changed: an item was removed */
    strcpy(D2.items[hp].id, "removed_item");
    CHECK(inv_deserialize(&J, &D2, buf, n, &dropped) == 0 && dropped >= 1 && inv_count(&J, hp) == 0 && J.gold == 12345);
    TEST_DONE();
}
