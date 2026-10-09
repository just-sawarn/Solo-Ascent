#ifndef SA_STORY_H
#define SA_STORY_H
/*
 * Story engine: string tables, a small condition/action language shared by quests and dialogue, the quest state machine,
 * dialogue trees and NPC definitions with day/night schedules. Content lives in the data directory (authored by tools/content),
 * all player-visible text lives in strings_en.json keyed by id. No engine or browser dependencies.
 *
 * Conditions (strings, ANDed):  "act >= 2"  "mq >= 9"  "quest.MQ05 == complete"  "bond.bram >= 3"  "flag.met_hanna"
 *                               "!flag.x"  "rep.aurelheim >= 10"  "item.hp_small >= 3"  "gold >= 100"  "level >= 5"
 * Actions:  quest.start:ID  quest.complete:ID  flag.set:name[:value]  item.give:id:n  item.take:id:n  gold.give:n
 *           bond.add:name:n  rep.add:region:n  shop.open:id  cutscene:id  toast:string_id  learn:member:skill  xp.give:n
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/data.h"
#include "core/items.h"

/* ---------- string table ---------- */
typedef struct { const char *id; const char *text; } StrEntry;
typedef struct { StrEntry *e; int n; char *pool; } StringTable;

bool        strings_load(StringTable *t, const char *path);
void        strings_free(StringTable *t);
const char *str_get(const StringTable *t, const char *id);                       /* NULL if missing */
/* Expands {player_name} (and other {vars}) into out. Returns out. */
const char *str_format(const char *text, const char *player_name, char *out, size_t cap);

/* ---------- world locations ---------- */
typedef struct {
    char id[32], name[40], region[24], kind[12];
    float map_x, map_y;          /* 0..1 on the world map */
    float lx, lz, radius;        /* centre/size in the hosting scene (explore trigger) */
    bool in_sandbox;             /* the current sandbox scene hosts this location */
} Location;

/* ---------- NPCs ---------- */
typedef struct { char place[10]; float x, z; } SchedSlot;
typedef struct {
    char id[24], name[32], region[24], role[16], archetype[16], shop[16], trainer[16];
    const char *personality;
    SchedSlot sched[3];          /* morning, afternoon, night */
    char tree[40];               /* greeting tree id */
    int sex;                     /* 0 male body, 1 female body */
    int hair;                    /* HairModel index */
    unsigned char hair_col[3], top[3], bottom[3], shoes[3], eye[3];
    unsigned char skin;          /* 0 light, 1 dark texture */
    float scale;
    bool required;               /* one of the story's named NPCs */
    const char *visible[2]; int nvisible;   /* the NPC exists in the world only while these hold */
} NpcDef;

/* ---------- quests ---------- */
typedef enum { OBJ_KILL, OBJ_COLLECT, OBJ_TALK, OBJ_EXPLORE, OBJ_ESCORT, OBJ_BOSS, OBJ_FLAG } ObjKind;
typedef enum { QT_MAIN, QT_SIDE, QT_COMPANION, QT_BOUNTY, QT_HIDDEN, QT_COLLECTION, QT_LORE } QuestType;
typedef struct { ObjKind kind; char target[40]; int count; const char *text; char marker[32]; } Objective;
typedef struct { char item[40]; int qty; } RewardItem;
typedef struct { char member[16]; char skill[32]; } RewardSkill;
typedef struct { char name[16]; int n; } RewardBond;

#define Q_MAX_OBJ 5
typedef struct {
    char id[32]; QuestType type; int act, number;   /* number: 1..25 for main quests */
    char giver[24], turn_in[24], region[24];
    bool auto_start, repeatable; int level;
    const char *title, *summary;
    const char *prereq[4]; int nprereq;
    Objective obj[Q_MAX_OBJ]; int nobj;
    int xp, gold, rep_amount; char rep_region[24];
    RewardItem items[4]; int nitems;
    RewardSkill skills[2]; int nskills;
    RewardBond bonds[2]; int nbonds;
    const char *on_complete[4]; int non_complete;
    const char *on_start[3]; int non_start;
    char offer_tree[40], progress_tree[40], complete_tree[40];
} QuestDef;

/* ---------- dialogue ---------- */
typedef struct { const char *text; const char *conds[3]; int nconds; const char *acts[4]; int nacts; int next; } DlgChoice;
typedef struct {
    char id[56], speaker[24];
    const char *text;
    const char *conds[4]; int nconds;
    const char *acts[6]; int nacts;
    int next;                    /* node index, -1 ends the conversation */
    int choice_first, nchoices;
} DlgNode;
typedef struct { const char *conds[3]; int nconds; int node; } DlgEntry;
typedef struct { char id[48]; DlgEntry entries[16]; int nentries; } DlgTree;

typedef struct StoryDB {
    StringTable strings;
    Location *loc; int nloc;
    NpcDef *npc; int nnpc;
    QuestDef *quest; int nquest;
    DlgNode *node; int nnode;
    DlgChoice *choice; int nchoice;
    DlgTree *tree; int ntree;
    char **pool; int npool;      /* owned strings (condition/action text) */
} StoryDB;

/* Loads and cross-validates locations, npcs, quests, dialogue and strings from `dir`. Returns false (with logged errors) on any bad reference. */
bool storydb_load(StoryDB *db, const char *dir, const EnemyDB *enemies, const ItemDB *items);
void storydb_free(StoryDB *db);
int  storydb_find_quest(const StoryDB *db, const char *id);
int  storydb_find_npc(const StoryDB *db, const char *id);
int  storydb_find_tree(const StoryDB *db, const char *id);
int  storydb_find_loc(const StoryDB *db, const char *id);

/* ---------- runtime state ---------- */
typedef enum { QS_LOCKED, QS_AVAILABLE, QS_ACTIVE, QS_READY, QS_COMPLETE } QState;
typedef struct { uint8_t state; int16_t prog[Q_MAX_OBJ]; uint16_t times_done; } QuestRt;
typedef struct { char key[40]; int value; } Flag;

#define STORY_MAX_QUESTS 256
#define STORY_MAX_FLAGS 512
typedef struct {
    QuestRt q[STORY_MAX_QUESTS];
    Flag flags[STORY_MAX_FLAGS]; int nflags;
    int tracked;                 /* quest index shown in the HUD tracker, -1 none */
    int awakening;
    int level;                   /* party level for conditions */
    char player_name[24];
} StoryState;

typedef struct {
    void *user;
    void (*give_item)(void *u, const char *item, int n);
    bool (*take_item)(void *u, const char *item, int n);
    int  (*item_count)(void *u, const char *item);
    void (*give_gold)(void *u, int n);
    int  (*gold)(void *u);
    void (*give_xp)(void *u, int n);
    void (*learn_skill)(void *u, const char *member, const char *skill);
    void (*open_shop)(void *u, const char *shop);
    void (*cutscene)(void *u, const char *id);
    void (*toast)(void *u, const char *text);
} StoryHooks;

void story_init(StoryState *s, const StoryDB *db, const char *player_name);
int  flag_get(const StoryState *s, const char *key);
void flag_set(StoryState *s, const char *key, int v);
void flag_add(StoryState *s, const char *key, int d);

int  story_mq(const StoryState *s, const StoryDB *db);            /* highest completed main quest number */
int  story_act(const StoryState *s, const StoryDB *db);
bool cond_eval(const StoryState *s, const StoryDB *db, const StoryHooks *h, const char *cond);
bool conds_eval(const StoryState *s, const StoryDB *db, const StoryHooks *h, const char *const *conds, int n);
bool cond_valid(const char *cond);                                  /* syntax check, used at load */
bool act_valid(const char *act);
void act_run(StoryState *s, const StoryDB *db, const StoryHooks *h, const char *act);

/* quests */
void story_update(StoryState *s, const StoryDB *db, const StoryHooks *h);   /* unlock + auto-start, call after any state change */
bool story_start_quest(StoryState *s, const StoryDB *db, const StoryHooks *h, int q);
bool story_complete_quest(StoryState *s, const StoryDB *db, const StoryHooks *h, int q);
void story_on_kill(StoryState *s, const StoryDB *db, const StoryHooks *h, const char *enemy_id);
void story_on_boss(StoryState *s, const StoryDB *db, const StoryHooks *h, const char *boss_id);
void story_on_talk(StoryState *s, const StoryDB *db, const StoryHooks *h, const char *npc_id);
void story_on_location(StoryState *s, const StoryDB *db, const StoryHooks *h, const char *loc_id);
void story_on_escort(StoryState *s, const StoryDB *db, const StoryHooks *h, const char *npc_id, const char *loc_id);
void story_sync_collect(StoryState *s, const StoryDB *db, const StoryHooks *h);   /* uses h->item_count */
bool quest_objectives_done(const StoryState *s, const StoryDB *db, int q);

/* NPC schedules: slot 0 morning 06-12, 1 afternoon 12-19, 2 night 19-06 */
int  sched_slot(float hour);
const SchedSlot *npc_slot_at(const NpcDef *n, float hour);

/* choosing what an NPC says: ready turn-in > quest offer > active hint > greeting. Returns tree index or -1. */
typedef enum { DLG_GREETING, DLG_TURN_IN, DLG_OFFER, DLG_PROGRESS } DlgKind;
int  story_pick_dialogue(const StoryState *s, const StoryDB *db, const StoryHooks *h, int npc, DlgKind *kind, int *quest);

/* dialogue runner */
typedef struct { int tree, node; bool active; int quest; int visible[6]; int nvisible; } DlgRunner;
bool dlg_start(DlgRunner *r, StoryState *s, const StoryDB *db, const StoryHooks *h, int tree, int quest);
const DlgNode *dlg_node(const DlgRunner *r, const StoryDB *db);
void dlg_advance(DlgRunner *r, StoryState *s, const StoryDB *db, const StoryHooks *h);            /* continue a linear node */
void dlg_choose(DlgRunner *r, StoryState *s, const StoryDB *db, const StoryHooks *h, int visible_index);

#endif
