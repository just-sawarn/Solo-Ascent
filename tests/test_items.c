#include "testutil.h"
#include <string.h>
#include "core/items.h"

static SkillDB S; static EnemyDB E; static ItemDB D;

int main(void) {
    CHECK(skilldb_load(&S, SA_DATA_DIR "/skills.json"));
    CHECK(enemydb_load(&E, &S, SA_DATA_DIR "/enemies.json"));
    CHECK(itemdb_load(&D, SA_DATA_DIR, &E));

    CHECK(D.nitems >= 150);                                  /* design target */
    int by_type[7] = {0}, by_rar[5] = {0};
    for (int i = 0; i < D.nitems; ++i) { by_type[D.items[i].type]++; by_rar[D.items[i].rarity]++; }
    CHECK(by_type[IT_WEAPON] >= 30 && by_type[IT_ARMOR] >= 60 && by_type[IT_ACCESSORY] >= 20);
    CHECK(by_type[IT_CONSUMABLE] >= 20 && by_type[IT_MATERIAL] >= 30 && by_type[IT_TOME] >= 6 && by_type[IT_KEY] >= 6);
    for (int r = 0; r < 5; ++r) CHECK(by_rar[r] > 0);        /* all five rarity tiers are in use */

    for (int i = 0; i < D.nitems; ++i) {
        const ItemDef *d = &D.items[i];
        CHECK(d->name[0] && d->desc[0]);
        if (item_is_equipment(d)) {
            const ItemStats *s = &d->st;
            CHECK(s->atk + s->matk + s->armor + s->hp + s->mp + s->str + s->intel + s->vit + s->wis + s->agi + s->crit > 0);
            CHECK(d->stack == 1);
            if (d->type != IT_ACCESSORY) CHECK(d->upgrade_mat >= 0);
        }
        if (d->type == IT_CONSUMABLE) CHECK(d->heal_hp > 0 || d->heal_mp > 0 || d->cleanse || d->revive > 0 || d->status != ST_NONE);
        if (d->type == IT_TOME) CHECK(skilldb_find(&S, d->teach) >= 0);
        if (d->type == IT_KEY) CHECK(d->sell == 0);
        if (d->buy > 0) CHECK(d->sell <= d->buy);            /* no buy-sell exploit */
    }

    /* progression: every role's weapon and armor strictly improve with tier, and so does the price */
    static const char *roles[] = {"hero", "tank", "mage", "healer"};
    for (int r = 0; r < 4; ++r) {
        float prev_w = 0, prev_a = 0; int prev_p = 0;
        for (int t = 1; t <= 6; ++t) {
            char id[48];
            snprintf(id, sizeof id, "w_%s_t%d", r == 3 ? "heal" : roles[r], t);
            int w = itemdb_find(&D, id);
            snprintf(id, sizeof id, "a_%s_body_t%d", roles[r], t);
            int a = itemdb_find(&D, id);
            CHECK(w >= 0 && a >= 0);
            if (w < 0 || a < 0) continue;
            float wp = D.items[w].st.atk + D.items[w].st.matk, ap = D.items[a].st.armor;
            CHECK(wp > prev_w); CHECK(ap > prev_a); CHECK(D.items[w].buy > prev_p);
            CHECK(D.items[w].level >= (t - 1) * 5 || t == 1);
            prev_w = wp; prev_a = ap; prev_p = D.items[w].buy;
        }
    }

    CHECK(D.nrecipes >= 60);
    unsigned stations = 0;
    for (int i = 0; i < D.nrecipes; ++i) {
        const RecipeDef *r = &D.recipes[i];
        stations |= r->station;
        CHECK(r->output >= 0 && r->nin > 0);
        /* crafting must not be a money printer: output value never exceeds the cost of its inputs by much */
        long in_val = r->gold; for (int k = 0; k < r->nin; ++k) in_val += (long)D.items[r->in[k].item].sell * r->in[k].qty;
        long out_val = (long)D.items[r->output].sell * r->qty;
        CHECK(out_val <= in_val * 4 + 50);
    }
    CHECK(stations == (ST_FORGE | ST_ALCHEMY | ST_COOKING));
    CHECK(D.nenchants >= 10);
    CHECK(D.nshops >= 6 && itemdb_find_shop(&D, "garrick") >= 0);
    CHECK(D.nloot >= 3);

    /* every shop item is actually priced */
    for (int s = 0; s < D.nshops; ++s) for (int k = 0; k < D.shops[s].nsells; ++k) CHECK(D.items[D.shops[s].sells[k]].buy > 0);

    /* the story's key items and legendary gear exist */
    CHECK(itemdb_find(&D, "seal_life") >= 0 && itemdb_find(&D, "sealing_crown") >= 0 && itemdb_find(&D, "w_hero_zenith") >= 0 && itemdb_find(&D, "w_tank_oaths") >= 0);
    TEST_DONE();
}
