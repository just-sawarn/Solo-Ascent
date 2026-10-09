#include "testutil.h"
#include <string.h>
#include "core/story.h"

static SkillDB S; static EnemyDB E; static ItemDB I;
static StoryDB DB; static StoryState ST;

typedef struct { int gold, xp, hp_small; int learned; char last_skill[32]; char toast[128]; int shop_opens; } Mock;
static Mock M;
static void give_item(void *u, const char *item, int n) { Mock *m = u; if (!strcmp(item, "hp_small")) m->hp_small += n; }
static bool take_item(void *u, const char *item, int n) { Mock *m = u; if (!strcmp(item, "hp_small") && m->hp_small >= n) { m->hp_small -= n; return true; } return false; }
static int item_count(void *u, const char *item) { Mock *m = u; return !strcmp(item, "hp_small") ? m->hp_small : 0; }
static void give_gold(void *u, int n) { ((Mock *)u)->gold += n; }
static int gold(void *u) { return ((Mock *)u)->gold; }
static void give_xp(void *u, int n) { ((Mock *)u)->xp += n; }
static void learn(void *u, const char *mem, const char *sk) { Mock *m = u; m->learned++; snprintf(m->last_skill, sizeof m->last_skill, "%s:%s", mem, sk); }
static void shop(void *u, const char *s) { (void)s; ((Mock *)u)->shop_opens++; }
static void toast(void *u, const char *t) { snprintf(((Mock *)u)->toast, sizeof ((Mock *)u)->toast, "%s", t); }
static StoryHooks H(void) { return (StoryHooks){&M, give_item, take_item, item_count, give_gold, gold, give_xp, learn, shop, NULL, toast}; }

int main(void) {
    CHECK(skilldb_load(&S, SA_DATA_DIR "/skills.json")); CHECK(enemydb_load(&E, &S, SA_DATA_DIR "/enemies.json")); CHECK(itemdb_load(&I, SA_DATA_DIR, &E));

    /* ---- the validator rejects broken content ---- */
    StoryDB bad;
    CHECK(!storydb_load(&bad, SA_DATA_DIR "/../tests/fixtures/story_bad", &E, &I));
    storydb_free(&bad);

    CHECK(storydb_load(&DB, SA_DATA_DIR "/../tests/fixtures/story_mini", &E, &I));
    CHECK(DB.nnpc == 2 && DB.nquest == 4 && DB.nloc == 1);

    /* ---- condition language ---- */
    CHECK(cond_valid("act >= 2") && cond_valid("quest.MQ05 == complete") && cond_valid("!flag.x") && cond_valid("bond.bram >= 3") && cond_valid("item.hp_small >= 3"));
    CHECK(!cond_valid("") && !cond_valid("act") && !cond_valid("quest.X == soon") && !cond_valid("nonsense >= 3") && !cond_valid("act >>= 3"));
    CHECK(act_valid("quest.start:MQ02") && act_valid("flag.set:x:2") && act_valid("bond.add:bram:1") && !act_valid("explode:now") && !act_valid("quest.start"));

    /* ---- strings and formatting ---- */
    char buf[128];
    CHECK(!strcmp(str_get(&DB.strings, "pip.hi"), "Hi!") && str_get(&DB.strings, "nope") == NULL);
    CHECK(!strcmp(str_format("Welcome, {player_name}. {unknown}", "Kael", buf, sizeof buf), "Welcome, Kael. {unknown}"));
    CHECK(!strcmp(str_format("{player_name}{player_name}", "Hanna", buf, 8), "HannaHa"));   /* never overflows */

    /* ---- initial state: main quest 1 auto-starts, everything else waits ---- */
    memset(&M, 0, sizeof M); StoryHooks h = H();
    story_init(&ST, &DB, "Kaela");
    story_update(&ST, &DB, &h);
    int mq1 = storydb_find_quest(&DB, "MQ01"), mq2 = storydb_find_quest(&DB, "MQ02"), sq1 = storydb_find_quest(&DB, "SQ01"), bq = storydb_find_quest(&DB, "BQ01");
    CHECK(ST.q[mq1].state == QS_ACTIVE && ST.q[mq2].state == QS_LOCKED && ST.q[sq1].state == QS_LOCKED && ST.q[bq].state == QS_LOCKED);
    CHECK(story_act(&ST, &DB) == 1 && story_mq(&ST, &DB) == 0 && ST.tracked == mq1);

    /* flag objectives complete themselves; auto turn-in pays out once */
    flag_set(&ST, "flag.awakened", 1);
    story_update(&ST, &DB, &h);
    CHECK(ST.q[mq1].state == QS_COMPLETE && M.xp == 50 && M.gold == 10 && M.hp_small == 2);
    CHECK(flag_get(&ST, "bond.bram") == 1 && flag_get(&ST, "rep.aurelheim") == 3 && flag_get(&ST, "flag.mq1_seen") == 1);
    story_update(&ST, &DB, &h); CHECK(M.xp == 50);                     /* no double reward */
    CHECK(!story_complete_quest(&ST, &DB, &h, mq1));
    CHECK(ST.q[mq2].state == QS_AVAILABLE && ST.q[sq1].state == QS_AVAILABLE && story_mq(&ST, &DB) == 1);
    CHECK(strstr(M.toast, "Day of the Crystal") != NULL);

    /* ---- dialogue picks: offer > greeting; greeting depends on story progress ---- */
    int hanna = storydb_find_npc(&DB, "hanna"), pip = storydb_find_npc(&DB, "pip");
    DlgKind kind; int q; DlgRunner r;
    int tree = story_pick_dialogue(&ST, &DB, &h, hanna, &kind, &q);
    CHECK(kind == DLG_OFFER && q == mq2 && tree == storydb_find_tree(&DB, "offer.mq2"));
    CHECK(dlg_start(&r, &ST, &DB, &h, tree, q) && r.nvisible == 2);
    dlg_choose(&r, &ST, &DB, &h, 1);                                     /* "Not now": quest stays offered */
    CHECK(!r.active && ST.q[mq2].state == QS_AVAILABLE);
    dlg_start(&r, &ST, &DB, &h, tree, q); dlg_choose(&r, &ST, &DB, &h, 0);   /* accept */
    CHECK(ST.q[mq2].state == QS_ACTIVE && r.active && !strcmp(dlg_node(&r, &DB)->id, "o2.2"));
    dlg_advance(&r, &ST, &DB, &h); CHECK(!r.active);
    tree = story_pick_dialogue(&ST, &DB, &h, hanna, &kind, &q);
    CHECK(kind == DLG_PROGRESS && q == mq2);                              /* active quest: she reminds you */

    /* ---- objectives: kills, talk, markers ---- */
    story_on_kill(&ST, &DB, &h, "shadow_archer"); CHECK(ST.q[mq2].prog[0] == 0);      /* wrong enemy */
    story_on_kill(&ST, &DB, &h, "shadow_soldier"); CHECK(ST.q[mq2].prog[0] == 1);
    story_on_kill(&ST, &DB, &h, "shadow_soldier"); story_on_kill(&ST, &DB, &h, "shadow_soldier"); CHECK(ST.q[mq2].prog[0] == 2);  /* capped */
    CHECK(ST.q[mq2].state == QS_ACTIVE);
    story_on_talk(&ST, &DB, &h, "pip"); CHECK(ST.q[mq2].prog[1] == 0);
    story_on_talk(&ST, &DB, &h, "hanna");
    CHECK(ST.q[mq2].state == QS_READY && M.xp == 50);                    /* needs her to turn it in */
    tree = story_pick_dialogue(&ST, &DB, &h, hanna, &kind, &q);
    CHECK(kind == DLG_TURN_IN && q == mq2);
    dlg_start(&r, &ST, &DB, &h, tree, q);
    CHECK(ST.q[mq2].state == QS_COMPLETE && M.xp == 150 && M.gold == 60 && flag_get(&ST, "flag.hanna_thanked") == 3 && flag_get(&ST, "bond.lyra") == 2);
    char out[160]; str_format(dlg_node(&r, &DB)->text, ST.player_name, out, sizeof out);
    CHECK(!strcmp(out, "Well done, Kaela."));                            /* variables expand */
    CHECK(story_act(&ST, &DB) == 5 && story_mq(&ST, &DB) == 2);        /* no more main quests in the fixture */
    tree = story_pick_dialogue(&ST, &DB, &h, hanna, &kind, &q);
    dlg_start(&r, &ST, &DB, &h, tree, q);
    CHECK(kind == DLG_GREETING && !strcmp(dlg_node(&r, &DB)->id, "hanna.late"));

    /* ---- collect objectives follow the inventory, and are consumed on turn-in ---- */
    CHECK(ST.q[sq1].state == QS_AVAILABLE);
    tree = story_pick_dialogue(&ST, &DB, &h, pip, &kind, &q);
    CHECK(kind == DLG_OFFER && q == sq1);
    dlg_start(&r, &ST, &DB, &h, tree, q); dlg_choose(&r, &ST, &DB, &h, 0);
    CHECK(ST.q[sq1].state == QS_ACTIVE && ST.q[sq1].prog[0] == 2);        /* quest start counts what you already hold (2 from MQ01) */
    M.hp_small = 2; story_sync_collect(&ST, &DB, &h); CHECK(ST.q[sq1].prog[0] == 2 && ST.q[sq1].state == QS_ACTIVE);
    M.hp_small = 5; story_sync_collect(&ST, &DB, &h); CHECK(ST.q[sq1].prog[0] == 3 && ST.q[sq1].state == QS_READY);
    M.hp_small = 1; story_sync_collect(&ST, &DB, &h); CHECK(ST.q[sq1].state == QS_ACTIVE);   /* gave them away: no longer ready */
    M.hp_small = 4; story_sync_collect(&ST, &DB, &h);
    tree = story_pick_dialogue(&ST, &DB, &h, pip, &kind, &q); CHECK(kind == DLG_TURN_IN);
    int gold_before = M.gold;
    dlg_start(&r, &ST, &DB, &h, tree, q);
    CHECK(ST.q[sq1].state == QS_COMPLETE && M.hp_small == 1 && M.gold == gold_before + 30);   /* 3 consumed, reward paid */
    CHECK(M.learned == 1 && !strcmp(M.last_skill, "kael:focus_strike"));

    /* ---- dialogue conditions: choices and lines appear only when their conditions hold ---- */
    tree = story_pick_dialogue(&ST, &DB, &h, pip, &kind, &q);
    CHECK(kind == DLG_GREETING);
    dlg_start(&r, &ST, &DB, &h, tree, q); dlg_advance(&r, &ST, &DB, &h);
    CHECK(!strcmp(dlg_node(&r, &DB)->id, "pip.menu") && r.nvisible == 2);   /* bond.bram is 1 */
    flag_set(&ST, "bond.bram", 0);
    dlg_start(&r, &ST, &DB, &h, tree, q); dlg_advance(&r, &ST, &DB, &h);
    CHECK(r.nvisible == 1);                                                 /* the secret option is hidden below bond 1 */
    dlg_choose(&r, &ST, &DB, &h, 0); CHECK(!r.active);
    flag_set(&ST, "bond.bram", 2);
    dlg_start(&r, &ST, &DB, &h, tree, q); dlg_advance(&r, &ST, &DB, &h); dlg_choose(&r, &ST, &DB, &h, 0);
    CHECK(r.active && !strcmp(dlg_node(&r, &DB)->id, "pip.sec"));
    dlg_advance(&r, &ST, &DB, &h);                                          /* the next line's condition fails: skipped, conversation ends */
    CHECK(!r.active);

    /* ---- repeatable bounties reset after completion ---- */
    CHECK(ST.q[bq].state == QS_LOCKED);
    flag_set(&ST, "flag.bounties_open", 1);
    story_update(&ST, &DB, &h);
    CHECK(ST.q[bq].state == QS_ACTIVE);
    int g0 = M.gold;
    story_on_kill(&ST, &DB, &h, "shadow_soldier"); story_on_kill(&ST, &DB, &h, "shadow_brute");
    CHECK(M.gold == g0 + 5 && ST.q[bq].times_done == 1 && ST.q[bq].state == QS_ACTIVE && ST.q[bq].prog[0] == 0);   /* available again, auto-restarted */
    story_on_kill(&ST, &DB, &h, "shadow_archer"); story_on_kill(&ST, &DB, &h, "shadow_archer");
    CHECK(ST.q[bq].times_done == 2);

    /* ---- NPC schedules ---- */
    CHECK(sched_slot(6.0f) == 0 && sched_slot(11.99f) == 0 && sched_slot(12.0f) == 1 && sched_slot(18.99f) == 1 && sched_slot(19.0f) == 2 && sched_slot(3.0f) == 2 && sched_slot(30.0f) == 0 && sched_slot(-2.0f) == 2);
    CHECK(!strcmp(npc_slot_at(&DB.npc[hanna], 14.0f)->place, "work") && !strcmp(npc_slot_at(&DB.npc[hanna], 22.0f)->place, "tavern"));

    /* ---- explore / escort / boss hooks do nothing without matching quests ---- */
    story_on_location(&ST, &DB, &h, "plaza"); story_on_escort(&ST, &DB, &h, "pip", "plaza"); story_on_boss(&ST, &DB, &h, "nobody");
    storydb_free(&DB);
    TEST_DONE();
}
