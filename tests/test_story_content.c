#include "testutil.h"
#include <ctype.h>
#include <string.h>
#include "core/story.h"

static SkillDB S; static EnemyDB E; static ItemDB I; static StoryDB DB; static StoryState ST;

static int item_count(void *u, const char *i) { (void)u; (void)i; return 9999; }
static bool take_item(void *u, const char *i, int n) { (void)u; (void)i; (void)n; return true; }
static void give_item(void *u, const char *i, int n) { (void)u; (void)i; (void)n; }
static void give_gold(void *u, int n) { (void)u; (void)n; }
static int gold(void *u) { (void)u; return 99999; }
static StoryHooks H(void) { return (StoryHooks){NULL, give_item, take_item, item_count, give_gold, gold, NULL, NULL, NULL, NULL, NULL}; }

/* every action string in the database (dialogue nodes, choices, quest hooks) */
typedef void (*ActFn)(const char *act, void *user);
static void each_action(ActFn fn, void *user) {
    for (int i = 0; i < DB.nnode; ++i) for (int k = 0; k < DB.node[i].nacts; ++k) fn(DB.node[i].acts[k], user);
    for (int i = 0; i < DB.nchoice; ++i) for (int k = 0; k < DB.choice[i].nacts; ++k) fn(DB.choice[i].acts[k], user);
    for (int i = 0; i < DB.nquest; ++i) { for (int k = 0; k < DB.quest[i].non_complete; ++k) fn(DB.quest[i].on_complete[k], user); for (int k = 0; k < DB.quest[i].non_start; ++k) fn(DB.quest[i].on_start[k], user); }
}

static char set_flags[1024][40]; static int nset;
static int bad_refs;
static void collect_flags(const char *act, void *u) {
    (void)u;
    if (!strncmp(act, "flag.set:", 9)) {
        char name[40]; int n = 0; const char *p = act + 9;
        while (*p && *p != ':' && n < 39) name[n++] = *p++;
        name[n] = 0;
        if (nset < 1024) snprintf(set_flags[nset++], 40, "flag.%s", name);
    }
}
static bool flag_is_set_somewhere(const char *key) { for (int i = 0; i < nset; ++i) if (!strcmp(set_flags[i], key)) return true; return false; }

static void check_refs(const char *act, void *u) {
    (void)u;
    char kind[24], a[48] = "";
    const char *c = strchr(act, ':');
    size_t kl = c ? (size_t)(c - act) : strlen(act);
    snprintf(kind, sizeof kind, "%.*s", (int)kl, act);
    if (c) { const char *e = strchr(c + 1, ':'); snprintf(a, sizeof a, "%.*s", e ? (int)(e - c - 1) : (int)strlen(c + 1), c + 1); }
    if (!strcmp(kind, "cutscene")) { char t[64]; snprintf(t, sizeof t, "cs.%s", a); if (storydb_find_tree(&DB, t) < 0) { printf("missing cutscene %s\n", a); bad_refs++; } }
    else if (!strcmp(kind, "quest.start") || !strcmp(kind, "quest.complete")) { if (storydb_find_quest(&DB, a) < 0) { printf("missing quest %s\n", a); bad_refs++; } }
    else if (!strcmp(kind, "item.give") || !strcmp(kind, "item.take")) { if (itemdb_find(&I, a) < 0) { printf("missing item %s\n", a); bad_refs++; } }
    else if (!strcmp(kind, "shop.open")) { if (itemdb_find_shop(&I, a) < 0) { printf("missing shop %s\n", a); bad_refs++; } }
    else if (!strcmp(kind, "learn")) { const char *e = strchr(c + 1, ':'); if (!e || skilldb_find(&S, e + 1) < 0) { printf("missing skill in %s\n", act); bad_refs++; } }
}

static int tree_refs[1024];
static void count_tree_ref(int tree) { if (tree >= 0 && tree < 1024) tree_refs[tree]++; }

int main(void) {
    CHECK(skilldb_load(&S, SA_DATA_DIR "/skills.json")); CHECK(enemydb_load(&E, &S, SA_DATA_DIR "/enemies.json")); CHECK(itemdb_load(&I, SA_DATA_DIR, &E));
    CHECK(storydb_load(&DB, SA_DATA_DIR, &E, &I));                         /* all references, strings, conditions, actions parse */

    /* ---- size targets from the design ---- */
    CHECK(DB.nnpc >= 60);
    int main_q = 0, comp = 0, side = 0, bounty = 0, nmq[26] = {0};
    for (int i = 0; i < DB.nquest; ++i) {
        const QuestDef *q = &DB.quest[i];
        if (q->type == QT_MAIN) { main_q++; if (q->number >= 1 && q->number <= 25) nmq[q->number]++; }
        else if (q->type == QT_COMPANION) comp++;
        else if (q->type == QT_BOUNTY) { bounty++; CHECK(q->repeatable); }
        else side++;
    }
    CHECK(main_q == 25); for (int n = 1; n <= 25; ++n) CHECK(nmq[n] == 1);
    CHECK(comp >= 15);                                                      /* 4 chains x 3 companions + 3 for Darius */
    CHECK(side >= 50);
    CHECK(bounty >= 8);
    CHECK(DB.nloc >= 40);

    /* the 24 required NPCs from the story bible, with a schedule, a personality and a greeting each */
    static const char *required[] = {"orwyn", "tilly", "haldor", "hanna", "pip", "garrick", "selwyn", "darius", "caldrin", "voss", "dunmore", "fenn", "kharim", "zaira", "anwen",
                                     "torvik", "elowen", "brandt", "ysolde", "nox", "edda", "marek", "mira", "lio"};
    for (unsigned i = 0; i < sizeof required / sizeof required[0]; ++i) {
        int n = storydb_find_npc(&DB, required[i]);
        CHECK(n >= 0);
        if (n >= 0) { CHECK(DB.npc[n].personality[0]); CHECK(DB.npc[n].required); CHECK(storydb_find_tree(&DB, DB.npc[n].tree) >= 0); }
    }
    for (int i = 0; i < DB.nnpc; ++i) {
        const NpcDef *n = &DB.npc[i];
        CHECK(n->sched[0].place[0] && n->sched[1].place[0] && n->sched[2].place[0]);
        int t = storydb_find_tree(&DB, n->tree);
        CHECK(t >= 0 && DB.tree[t].nentries >= 4);                         /* stage greetings: early, mid, late, post */
        CHECK(!strcmp(n->sched[1].place, "work"));
    }
    /* NPCs react to the major milestones: some greeting entries are conditioned on MQ09 / MQ20 / MQ24 */
    bool has09 = false, has20 = false, has24 = false;
    for (int i = 0; i < DB.ntree; ++i) for (int e = 0; e < DB.tree[i].nentries; ++e) for (int c = 0; c < DB.tree[i].entries[e].nconds; ++c) {
        if (strstr(DB.tree[i].entries[e].conds[c], "MQ09")) has09 = true;
        if (strstr(DB.tree[i].entries[e].conds[c], "MQ20")) has20 = true;
        if (strstr(DB.tree[i].entries[e].conds[c], "MQ24")) has24 = true;
    }
    CHECK(has09 && has20 && has24);

    /* ---- static analysis: references and flags ---- */
    each_action(collect_flags, NULL);
    each_action(check_refs, NULL);
    CHECK(bad_refs == 0);
    for (int i = 0; i < DB.nquest; ++i) for (int k = 0; k < DB.quest[i].nobj; ++k) {
        const Objective *o = &DB.quest[i].obj[k];
        if (o->kind == OBJ_FLAG) { char key[64]; snprintf(key, sizeof key, "flag.%s", o->target); if (!flag_is_set_somewhere(key)) { printf("objective flag never set: %s (%s)\n", key, DB.quest[i].id); CHECK(0); } }
    }
    /* every flag used in a condition is set by something */
    int unset = 0;
    for (int i = 0; i < DB.nquest; ++i) for (int k = 0; k < DB.quest[i].nprereq; ++k) {
        const char *p = strstr(DB.quest[i].prereq[k], "flag."); if (!p) continue;
        char key[64]; int n = 0; while (*p && !isspace((unsigned char)*p) && n < 63) key[n++] = *p++; key[n] = 0;
        if (!flag_is_set_somewhere(key)) { printf("prereq flag never set: %s (%s)\n", key, DB.quest[i].id); unset++; }
    }
    for (int i = 0; i < DB.nnode; ++i) for (int k = 0; k < DB.node[i].nconds; ++k) {
        const char *p = strstr(DB.node[i].conds[k], "flag."); if (!p || p != DB.node[i].conds[k] + (DB.node[i].conds[k][0] == '!')) continue;
        char key[64]; int n = 0; while (*p && !isspace((unsigned char)*p) && n < 63) key[n++] = *p++; key[n] = 0;
        if (strncmp(key, "flag.done_", 10) && !flag_is_set_somewhere(key)) { printf("node flag never set: %s (%s)\n", key, DB.node[i].id); unset++; }
    }
    for (int i = 0; i < DB.nnpc; ++i) for (int k = 0; k < DB.npc[i].nvisible; ++k) {
        const char *p = strstr(DB.npc[i].visible[k], "flag."); if (!p) continue;
        char key[64]; int n = 0; while (*p && !isspace((unsigned char)*p) && n < 63) key[n++] = *p++; key[n] = 0;
        if (!flag_is_set_somewhere(key)) { printf("npc visibility flag never set: %s (%s)\n", key, DB.npc[i].id); unset++; }
    }
    CHECK(unset == 0);

    /* ---- no orphan dialogue: every tree is reachable from an NPC, a quest or a cutscene action ---- */
    for (int i = 0; i < DB.nnpc; ++i) count_tree_ref(storydb_find_tree(&DB, DB.npc[i].tree));
    for (int i = 0; i < DB.nquest; ++i) { count_tree_ref(storydb_find_tree(&DB, DB.quest[i].offer_tree)); count_tree_ref(storydb_find_tree(&DB, DB.quest[i].progress_tree)); count_tree_ref(storydb_find_tree(&DB, DB.quest[i].complete_tree)); }
    for (int i = 0; i < DB.nnode; ++i) for (int k = 0; k < DB.node[i].nacts; ++k) if (!strncmp(DB.node[i].acts[k], "cutscene:", 9)) { char t[64]; snprintf(t, sizeof t, "cs.%s", DB.node[i].acts[k] + 9); count_tree_ref(storydb_find_tree(&DB, t)); }
    for (int i = 0; i < DB.nquest; ++i) { for (int k = 0; k < DB.quest[i].non_complete; ++k) if (!strncmp(DB.quest[i].on_complete[k], "cutscene:", 9)) { char t[64]; snprintf(t, sizeof t, "cs.%s", DB.quest[i].on_complete[k] + 9); count_tree_ref(storydb_find_tree(&DB, t)); }
                                            for (int k = 0; k < DB.quest[i].non_start; ++k) if (!strncmp(DB.quest[i].on_start[k], "cutscene:", 9)) { char t[64]; snprintf(t, sizeof t, "cs.%s", DB.quest[i].on_start[k] + 9); count_tree_ref(storydb_find_tree(&DB, t)); } }
    int orphans = 0;
    for (int i = 0; i < DB.ntree; ++i) if (tree_refs[i] == 0 && strncmp(DB.tree[i].id, "cs.", 3)) { printf("orphan tree %s\n", DB.tree[i].id); orphans++; }
    CHECK(orphans == 0);
    int unused_cs = 0;
    for (int i = 0; i < DB.ntree; ++i) if (tree_refs[i] == 0 && !strncmp(DB.tree[i].id, "cs.", 3)) { printf("unused cutscene %s\n", DB.tree[i].id); unused_cs++; }
    CHECK(unused_cs <= 3);                                                  /* post-credits / quartet strike are triggered by the boss script (Milestone 10) */

    /* ---- the offer and turn-in dialogues actually drive the quest state machine ---- */
    StoryHooks h = H();
    for (int i = 0; i < DB.nquest; ++i) {
        const QuestDef *q = &DB.quest[i];
        if (q->offer_tree[0]) {
            story_init(&ST, &DB, "Kael");
            ST.q[i].state = QS_AVAILABLE;
            DlgRunner r;
            CHECK(dlg_start(&r, &ST, &DB, &h, storydb_find_tree(&DB, q->offer_tree), i));
            bool accepted = false;
            for (int adv = 0; adv < 12 && r.active && r.nvisible == 0; ++adv) dlg_advance(&r, &ST, &DB, &h);   /* read the briefing until the choice appears */
            for (int v = 0; v < r.nvisible && !accepted; ++v) {
                const DlgChoice *ch = &DB.choice[DB.node[r.node].choice_first + r.visible[v]];
                for (int a = 0; a < ch->nacts; ++a) { char want[64]; snprintf(want, sizeof want, "quest.start:%s", q->id); if (!strcmp(ch->acts[a], want)) { dlg_choose(&r, &ST, &DB, &h, v); accepted = true; break; } }
            }
            CHECK(accepted);
            if (!accepted) printf("offer for %s has no accept choice\n", q->id);
            CHECK(ST.q[i].state == QS_ACTIVE || ST.q[i].state == QS_READY || ST.q[i].state == QS_COMPLETE);
        }
        if (q->complete_tree[0]) {
            story_init(&ST, &DB, "Kael");
            ST.q[i].state = QS_ACTIVE;
            for (int k = 0; k < q->nobj; ++k) ST.q[i].prog[k] = (int16_t)q->obj[k].count;
            ST.q[i].state = QS_READY;
            DlgRunner r;
            CHECK(dlg_start(&r, &ST, &DB, &h, storydb_find_tree(&DB, q->complete_tree), i));
            int guard = 0;
            while (r.active && guard++ < 40) { if (r.nvisible > 0) dlg_choose(&r, &ST, &DB, &h, 0); else dlg_advance(&r, &ST, &DB, &h); }
            if (ST.q[i].state != QS_COMPLETE && !q->repeatable) printf("turn-in for %s does not complete it\n", q->id);
            CHECK(ST.q[i].state == QS_COMPLETE || q->repeatable);
        }
    }

    /* ---- a headless playthrough: complete everything reachable, in whatever order the prerequisites allow ---- */
    story_init(&ST, &DB, "Kael");
    story_update(&ST, &DB, &h);
    int rounds = 0;
    for (; rounds < 600; ++rounds) {
        bool moved = false;
        for (int i = 0; i < DB.nquest; ++i) {
            const QuestDef *q = &DB.quest[i];
            if (q->type == QT_BOUNTY) continue;
            if (ST.q[i].state == QS_AVAILABLE) { story_start_quest(&ST, &DB, &h, i); moved = true; }
            if (ST.q[i].state == QS_ACTIVE || ST.q[i].state == QS_READY) {
                for (int k = 0; k < q->nobj; ++k) {
                    if (q->obj[k].kind == OBJ_FLAG) { char key[64]; snprintf(key, sizeof key, "flag.%s", q->obj[k].target); flag_set(&ST, key, 1); }
                    ST.q[i].prog[k] = (int16_t)q->obj[k].count;
                }
                story_update(&ST, &DB, &h);
                if (ST.q[i].state == QS_READY) story_complete_quest(&ST, &DB, &h, i);
                moved = true;
            }
        }
        if (!moved) break;
    }
    CHECK(rounds < 600);
    int locked = 0;
    for (int i = 0; i < DB.nquest; ++i) if (DB.quest[i].type != QT_BOUNTY && ST.q[i].state != QS_COMPLETE) { printf("not completed: %s (state %d)\n", DB.quest[i].id, ST.q[i].state); locked++; }
    CHECK(locked == 0);
    CHECK(story_mq(&ST, &DB) == 25);
    CHECK(flag_get(&ST, "flag.game_cleared") == 1);
    CHECK(ST.awakening == 5);
    for (int i = 0; i < DB.nquest; ++i) if (DB.quest[i].type == QT_BOUNTY) CHECK(ST.q[i].state != QS_LOCKED);   /* the board opens */

    /* main quests chain strictly: quest N needs quest N-1 */
    for (int n = 2; n <= 25; ++n) {
        int qi = -1, prev = -1;
        for (int i = 0; i < DB.nquest; ++i) if (DB.quest[i].type == QT_MAIN) { if (DB.quest[i].number == n) qi = i; if (DB.quest[i].number == n - 1) prev = i; }
        char want[64]; snprintf(want, sizeof want, "quest.%s == complete", DB.quest[prev].id);
        bool ok = false; for (int k = 0; k < DB.quest[qi].nprereq; ++k) if (!strcmp(DB.quest[qi].prereq[k], want)) ok = true;
        CHECK(ok);
    }
    storydb_free(&DB);
    TEST_DONE();
}
