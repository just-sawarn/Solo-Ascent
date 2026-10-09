#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include "core/camrig.h"
#include "core/combat.h"
#include "core/data.h"
#include "core/ecs.h"
#include "core/enemy_ai.h"
#include "core/gameloop.h"
#include "core/inventory.h"
#include "core/items.h"
#include "core/log.h"
#include "core/party.h"
#include "core/physics.h"
#include "core/story.h"
#include "core/player.h"
#include "platform/platform.h"
#include "render/actors.h"
#include "render/heroes.h"
#include "render/procmodels.h"
#include "render/renderer.h"
#include "ui/debug_overlay.h"
#include "ui/dialogue.h"
#include "ui/hud.h"
#include "ui/input.h"
#include "ui/menu.h"
#include "ui/story_hud.h"
#include "world/npc_world.h"
#include "world/test_scene.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

typedef enum { STATE_TITLE, STATE_PLAYING } GameState;

typedef struct {
    World world;
    GameLoop loop;
    DebugState dbg;
    GameState state;
    PhysWorld phys;
    CamRig cam;
    Vec3 cam_focus;
    Input input;
    SkillDB skills;
    EnemyDB edb;
    PartyDB pdb;
    ItemDB idb;
    Inventory inv;
    Menu menu;
    uint32_t loot_rng;
    StoryDB story; StoryState ss; StoryHooks sh; NpcWorld npcs; DialogueUI dui;
    char region[24];
    float hour, story_timer;
    int near_npc; bool loc_inside[128]; bool story_ok;
    struct { char text[72]; float t; Color col; } loot_log[6];
    int gear_level;
    EnemyPool enemies;
    CombatWorld cw;
    PartyCtx ctx;
    Party party;
    ActorAnim actors[ENEMY_MAX];
    Hud hud;
    Entity lock_ent;
    float hitstop, respawn_t;
    int kills; float last_damage;
    bool arena, data_ok;
    bool auto_orbit, governor_on;
    double title_t, time;
    double toast_t; char toast[96];
    /* per-member one-frame animation triggers accumulated over sim steps */
    bool ev_roll[PARTY_MAX], ev_cast[PARTY_MAX];
    int cast_skill[PARTY_MAX];
} Game;

static Game G; /* static: World and PhysWorld are large, no per-frame allocation anywhere */

static void toast(Game *g, const char *fmt, int a, const char *b) {
    snprintf(g->toast, sizeof g->toast, fmt, a, b);
    g->toast_t = 2.5;
}

static void toast_text(Game *g, const char *s) { snprintf(g->toast, sizeof g->toast, "%s", s); g->toast_t = 2.5; }

static const struct { const char *id; float x, z; } SPAWNS[] = {
    {"shadow_soldier", -9.0f, -13.0f}, {"shadow_soldier", 9.0f, -13.0f}, {"shadow_archer", 0.0f, -19.0f},
    {"shadow_brute", 15.0f, -6.0f}, {"shadow_soldier", -15.0f, 9.0f},
};

static void spawn_enemy(Game *g, const char *id, float x, float z) {
    int def = enemydb_find(&g->edb, id);
    if (def >= 0) enemy_spawn(&g->enemies, &g->cw, &g->world, &g->edb, def, vec3(x, 0, z));
}

static const char *CMD_NAMES[] = {"Aggressive", "Defensive", "Support"};

static PartyMember *active_member(Game *g) { return &g->party.m[g->party.active < 0 ? 0 : g->party.active]; }
static Entity active_ent(Game *g) { return active_member(g)->ent; }

static void log_loot(Game *g, Color col, const char *fmt, const char *name, int a) {
    for (int i = 5; i > 0; --i) g->loot_log[i] = g->loot_log[i - 1];
    snprintf(g->loot_log[0].text, sizeof g->loot_log[0].text, fmt, name, a);
    g->loot_log[0].t = 5.0f; g->loot_log[0].col = col;
}

/* Re-apply equipment to every combatant and refresh the weapon each member holds. */
static void apply_all_gear(Game *g) {
    for (int i = 0; i < g->party.n; ++i) {
        Combatant *c = party_cb(&g->party, &g->ctx, i);
        if (c) inv_apply_gear(&g->inv, &g->idb, i, c);
        const ItemStack *w = &g->inv.equip[i][EQ_WEAPON];
        if (w->item >= 0) heroes_set_weapon(i, g->idb.items[w->item].model);
    }
}

static void give_equipped(Game *g, int member, const char *id) {
    int it = itemdb_find(&g->idb, id);
    if (it < 0 || !inv_add_instance(&g->inv, &g->idb, it, 0, -1)) return;
    for (int s = 0; s < INV_SLOTS; ++s) if (g->inv.slot[s].item == it) { inv_equip(&g->inv, &g->idb, member, g->pdb.m[member].role, g->gear_level, s, 0); break; }
}

static void starting_kit(Game *g) {
    inv_init(&g->inv);
    g->inv.gold = 600;
    static const struct { const char *id; int n; } KIT[] = {{"hp_small", 6}, {"mp_small", 3}, {"antidote", 2}, {"food_bread", 4}, {"coal", 6}, {"meadow_herb", 4}, {"crystal_vial", 4},
                                                           {"iron_ore", 8}, {"wolf_pelt", 4}, {"linen_thread", 6}, {"spark_dust", 2}, {"w_hero_club_t1", 1}, {"acc_lucky_coin", 1}, {"phoenix_feather", 1}};
    for (unsigned i = 0; i < sizeof KIT / sizeof KIT[0]; ++i) { int it = itemdb_find(&g->idb, KIT[i].id); if (it >= 0) inv_add(&g->inv, &g->idb, it, KIT[i].n); }
    static const char *ROLE_KEY[] = {"hero", "tank", "mage", "healer"}, *WEAPON_KEY[] = {"w_hero_t1", "w_tank_t1", "w_mage_t1", "w_heal_t1"};
    for (int i = 0; i < g->pdb.n && i < 4; ++i) {
        char body[48]; snprintf(body, sizeof body, "a_%s_body_t1", ROLE_KEY[i]);
        give_equipped(g, i, WEAPON_KEY[i]); give_equipped(g, i, body);
    }
    give_equipped(g, 0, "acc_guild_ring");
}


/* ---------------- story hooks: the engine-agnostic story core talks to the game through these ---------------- */
static void h_give_item(void *u, const char *id, int n) {
    Game *g = u; int it = itemdb_find(&g->idb, id);
    if (it < 0) return;
    int left = inv_add(&g->inv, &g->idb, it, n);
    if (n - left > 0) log_loot(g, (Color){200, 240, 255, 255}, "%s x%d", g->idb.items[it].name, n - left);
}
static bool h_take_item(void *u, const char *id, int n) { Game *g = u; int it = itemdb_find(&g->idb, id); return it >= 0 && inv_remove(&g->inv, &g->idb, it, n); }
static int h_item_count(void *u, const char *id) { Game *g = u; int it = itemdb_find(&g->idb, id); return it >= 0 ? inv_count(&g->inv, it) : 0; }
static void h_give_gold(void *u, int n) { Game *g = u; g->inv.gold += n; char t[32]; snprintf(t, sizeof t, "+%d gold", n); log_loot(g, (Color){255, 220, 120, 255}, "%s", t, 0); }
static int h_gold(void *u) { return ((Game *)u)->inv.gold; }
static void h_give_xp(void *u, int n) { Game *g = u; char t[32]; snprintf(t, sizeof t, "+%d XP", n); log_loot(g, (Color){180, 255, 190, 255}, "%s", t, 0); }
static void h_learn(void *u, const char *member, const char *skill) {
    Game *g = u; int sk = skilldb_find(&g->skills, skill);
    for (int i = 0; i < g->pdb.n && sk >= 0; ++i) if (!strcmp(g->pdb.m[i].id, member)) {
        PartyDef *d = &g->pdb.m[i];
        for (int k = 0; k < d->nskills; ++k) if (d->skills[k] == sk) return;
        if (d->nskills < 16) { d->skills[d->nskills++] = sk; { char lt[96]; snprintf(lt, sizeof lt, "%s learned %s", d->name, g->skills.s[sk].name); log_loot(g, (Color){255, 214, 90, 255}, "%s", lt, 0); } }
    }
}
static void h_open_shop(void *u, const char *shop) { Game *g = u; int s = itemdb_find_shop(&g->idb, shop); if (s >= 0) menu_open_shop(&g->menu, s); }
static void h_cutscene(void *u, const char *name) { dui_queue_cutscene(&((Game *)u)->dui, name); }
static void h_toast(void *u, const char *text) { toast_text((Game *)u, text); }

static void story_bind(Game *g) {
    g->sh = (StoryHooks){g, h_give_item, h_take_item, h_item_count, h_give_gold, h_gold, h_give_xp, h_learn, h_open_shop, h_cutscene, h_toast};
}

static void refresh_npcs(Game *g) { npcw_refresh(&g->npcs, &g->story, &g->ss, &g->sh, g->region, g->hour); }
static void on_travel(void *u, const char *town) { Game *g = u; snprintf(g->region, sizeof g->region, "%s", town); npcw_init(&g->npcs); refresh_npcs(g); }

static void new_game(Game *g) {
    ecs_init(&g->world);
    gameloop_init(&g->loop);
    combat_init(&g->cw, &g->skills);
    enemies_init(&g->enemies);
    hud_init(&g->hud);
    memset(g->actors, 0, sizeof g->actors);
    memset(g->ev_roll, 0, sizeof g->ev_roll); memset(g->ev_cast, 0, sizeof g->ev_cast);
    g->ctx = (PartyCtx){&g->cw, &g->world, &g->phys, &g->enemies, &g->edb, &g->skills};
    party_init(&g->party);
    g->party.cmd = CMD_DEFENSIVE;
    static const float OX[4] = {0.0f, -2.4f, 2.4f, 0.0f}, OZ[4] = {0.5f, 2.2f, 2.6f, 4.2f};
    for (int i = 0; i < g->pdb.n && i < PARTY_MAX; ++i) party_add(&g->party, &g->ctx, &g->pdb.m[i], vec3(OX[i], 0, OZ[i]), 3.14159265f);
    g->party.active = 0;
    if (g->arena) spawn_enemy(g, "shadow_brute", 0.0f, -4.5f);
    else for (unsigned i = 0; i < sizeof SPAWNS / sizeof SPAWNS[0]; ++i) spawn_enemy(g, SPAWNS[i].id, SPAWNS[i].x, SPAWNS[i].z);
    g->lock_ent = ENTITY_NULL;
    g->cam.locked = false;
    g->kills = 0; g->respawn_t = 0; g->hitstop = 0;
    Vec3 ap = active_member(g)->pc.ch.pos;
    g->cam_focus = ap;
    starting_kit(g);
    apply_all_gear(g);
    menu_init(&g->menu);
    memset(g->loot_log, 0, sizeof g->loot_log);
    if (g->story_ok) {
        story_bind(g);
        story_init(&g->ss, &g->story, "Kael");
        g->ss.level = 5;
        snprintf(g->region, sizeof g->region, "aurelheim");
        g->hour = 9.0f; g->story_timer = 0;
        memset(g->loc_inside, 0, sizeof g->loc_inside);
        dui_init(&g->dui);
        npcw_init(&g->npcs);
        story_update(&g->ss, &g->story, &g->sh);
        refresh_npcs(g);
        g->near_npc = -1;
    }
}

static Vector3 lock_pos(const Game *g) {
    if (g->lock_ent == ENTITY_NULL || !ecs_alive(&g->world, g->lock_ent)) return (Vector3){0, 0, 0};
    Vec3 p = g->world.pos[ecs_index(g->lock_ent)];
    return (Vector3){p.x, p.y, p.z};
}

static PartyInput make_party_input(const Game *g) {
    const Input *in = &g->input;
    float cs = cosf(g->cam.yaw), sn = sinf(g->cam.yaw); /* camera-relative movement */
    PartyInput pin = {0};
    pin.move.mx = in->move_x * cs - in->move_y * sn;
    pin.move.mz = -in->move_x * sn - in->move_y * cs;
    pin.move.sprint = in->sprint; pin.move.walk = in->walk;
    pin.attack = in->attack; pin.jump = in->jump; pin.roll = in->roll; pin.guard = in->guard;
    for (int k = 0; k < 4; ++k) pin.skill[k] = in->skill[k];
    pin.lock = (g->lock_ent != ENTITY_NULL && combat_alive(&g->cw, g->lock_ent)) ? g->lock_ent : ENTITY_NULL;
    return pin;
}

/* Fixed 60 Hz simulation step. */
static void sim_step(Game *g, bool first_step) {
    Party *pt = &g->party;
    PartyInput pin = make_party_input(g);
    pt->focus = pin.lock;
    party_step(pt, &g->ctx, &pin, first_step, SIM_DT);
    for (int i = 0; i < pt->n; ++i) { g->ev_roll[i] |= pt->m[i].pc.ev_roll; }
    enemies_update(&g->enemies, &g->cw, &g->world, &g->phys, &g->edb, SIM_DT);
    combat_update(&g->cw, &g->world, &g->phys, SIM_DT);
    if (g->dbg.god_mode) for (int i = 0; i < pt->n; ++i) { Combatant *c = party_cb(pt, &g->ctx, i); if (c && !c->dead) c->hp = c->max_hp; }

    /* the controlled member fell: hand control to someone alive; a full wipe respawns the party */
    Combatant *ac = party_cb(pt, &g->ctx, pt->active);
    if (ac && ac->dead) { int nx = party_next_alive(pt, &g->ctx, pt->active); if (nx >= 0) { party_switch(pt, &g->ctx, nx); g->lock_ent = ENTITY_NULL; } }
    if (party_wiped(pt, &g->ctx)) {
        g->respawn_t += SIM_DT;
        if (g->respawn_t > 3.5f) { party_respawn_all(pt, &g->ctx, vec3(0, 0, 0.5f)); pt->active = 0; g->lock_ent = ENTITY_NULL; g->respawn_t = 0; }
    }
}

/* Presentation reactions to this frame's combat events. */
static void process_events(Game *g) {
    Entity act = active_ent(g);
    for (int i = 0; i < g->cw.nev; ++i) {
        const CombatEvent *e = &g->cw.ev[i];
        bool by_active = e->src == act, on_party = false;
        for (int k = 0; k < g->party.n; ++k) {
            if (e->dst == g->party.m[k].ent) on_party = true;
            if (e->type == CE_CAST && e->amount < 0.5f && e->src == g->party.m[k].ent) { g->ev_cast[k] = true; g->cast_skill[k] = e->skill; }
        }
        bool on_active = e->dst == act;
        switch (e->type) {
        case CE_HIT:
            if (by_active) { g->hitstop = fmaxf(g->hitstop, e->crit ? 0.085f : 0.05f); camrig_shake(&g->cam, e->crit ? 0.05f : 0.02f); g->last_damage = e->amount; }
            else if (!on_party) camrig_shake(&g->cam, 0.008f);
            if (on_active) { g->hitstop = fmaxf(g->hitstop, 0.045f); camrig_shake(&g->cam, 0.06f); }
            break;
        case CE_PARRY: g->hitstop = fmaxf(g->hitstop, 0.14f); camrig_shake(&g->cam, 0.09f); break;
        case CE_BLOCK: camrig_shake(&g->cam, 0.03f); break;
        case CE_DEATH:
            if (!on_party) {
                g->kills++; g->hitstop = fmaxf(g->hitstop, 0.10f); camrig_shake(&g->cam, 0.08f);
                Enemy *en = enemy_of(&g->enemies, e->dst);
                if (en && g->story_ok) {
                    const EnemyDef *ed = &g->edb.e[en->def];
                    if (ed->boss) story_on_boss(&g->ss, &g->story, &g->sh, ed->id);
                    story_on_kill(&g->ss, &g->story, &g->sh, ed->id);
                }
                if (en) {
                    LootDrop drops[12]; int gold = 0, n = loot_roll(&g->idb, en->def, &g->loot_rng, drops, &gold);
                    if (gold > 0) { char gl[32]; snprintf(gl, sizeof gl, "+%d gold", gold); g->inv.gold += gold; log_loot(g, (Color){255, 220, 120, 255}, "%s", gl, 0); }
                    for (int k = 0; k < n; ++k) {
                        int left = inv_add(&g->inv, &g->idb, drops[k].item, drops[k].qty);
                        if (drops[k].qty - left > 0) log_loot(g, (Color){200, 240, 255, 255}, "%s x%d", g->idb.items[drops[k].item].name, drops[k].qty - left);
                    }
                }
            }
            else camrig_shake(&g->cam, 0.15f);
            break;
        case CE_CAST:
            if (e->amount > 0.5f && by_active) camrig_shake(&g->cam, 0.05f); /* ground impact */
            break;
        default: break;
        }
    }
    hud_on_events(&g->hud, g->cw.ev, g->cw.nev, &g->party);
    g->cw.nev = 0;
}

static void handle_render_keys(Game *g) {
    if (IsKeyPressed(KEY_L)) {
        int l = (renderer_look() + 1) % LOOK_COUNT;
        renderer_set_look(l);
        toast(g, "Look %d: %s", l + 1, renderer_look_name(l));
    }
    int t = (int)renderer_tier();
    if (IsKeyPressed(KEY_LEFT_BRACKET) && t < TIER_MOBILE) { g->governor_on = false; renderer_set_tier((QualityTier)(t + 1)); toast(g, "Quality %d: %s", t + 1, renderer_tier_settings(renderer_tier())->name); }
    if (IsKeyPressed(KEY_RIGHT_BRACKET) && t > TIER_ULTRA) { g->governor_on = false; renderer_set_tier((QualityTier)(t - 1)); toast(g, "Quality %d: %s", t - 1, renderer_tier_settings(renderer_tier())->name); }
}

static void toggle_lock(Game *g) {
    if (g->lock_ent != ENTITY_NULL) { g->lock_ent = ENTITY_NULL; return; }
    Vec3 pp = active_member(g)->pc.ch.pos;
    float best = 25.0f * 25.0f; Entity bi = ENTITY_NULL;
    float fx = -sinf(g->cam.yaw), fz = -cosf(g->cam.yaw);
    for (int i = 0; i < ENEMY_MAX; ++i) {
        const Enemy *en = &g->enemies.e[i];
        if (!en->used || !combat_alive(&g->cw, en->ent)) continue;
        Vec3 tp = g->world.pos[ecs_index(en->ent)];
        float dx = tp.x - pp.x, dz = tp.z - pp.z, d2 = dx * dx + dz * dz;
        float facing = (dx * fx + dz * fz) / (sqrtf(d2) + 1e-4f); /* prefer what the camera looks at */
        if (facing < -0.2f) continue;
        if (d2 < best) { best = d2; bi = en->ent; }
    }
    g->lock_ent = bi;
}

static void quick_use(Game *g, int kind) {
    int slot = inv_quick_slot(&g->inv, &g->idb, kind);
    if (slot < 0) { toast_text(g, "None left."); return; }
    int target = -1; float best = 2.0f;
    for (int i = 0; i < g->party.n; ++i) {
        Combatant *c = party_cb(&g->party, &g->ctx, i);
        if (!c) continue;
        float score = 9;
        if (kind == 0 && !c->dead && c->hp < c->max_hp) score = c->hp / c->max_hp;
        if (kind == 1 && !c->dead && c->mp < c->max_mp) score = c->mp / c->max_mp;
        if (kind == 2 && c->dead) score = 0;
        if (kind == 3 && !c->dead && (c->status[ST_BURN].time > 0 || c->status[ST_POISON].time > 0 || c->status[ST_SLOW].time > 0 || c->status[ST_STUN].time > 0 || c->status[ST_SILENCE].time > 0 || c->status[ST_HOLLOW_CURSE].time > 0)) score = 0;
        if (score < best) { best = score; target = i; }
    }
    if (target < 0) { toast_text(g, "Nobody needs that."); return; }
    char name[48]; snprintf(name, sizeof name, "%s", g->idb.items[g->inv.slot[slot].item].name);
    UseResult r = inv_use(&g->inv, &g->idb, &g->cw, party_cb(&g->party, &g->ctx, target), slot);
    char b[96];
    if (r == USE_OK) snprintf(b, sizeof b, "%s on %s", name, g->party.m[target].def->name);
    else snprintf(b, sizeof b, "%s", r == USE_COOLDOWN ? "Potions need a moment between uses." : "No effect.");
    toast_text(g, b);
}

static void do_party_controls(Game *g) {
    Input *in = &g->input;
    Party *pt = &g->party;
    int to = -1;
    if (in->swap_to >= 0 && in->swap_to < pt->n) to = in->swap_to;
    else if (in->swap_next) to = party_next_alive(pt, &g->ctx, pt->active);
    if (to >= 0 && party_switch(pt, &g->ctx, to)) {
        g->lock_ent = ENTITY_NULL;
        char b[64]; snprintf(b, sizeof b, "Controlling %s", pt->m[to].def->name);
        toast_text(g, b);
    }
    for (int k = 0; k < 4; ++k) if (in->quick[k]) quick_use(g, k);
    if (in->cmd_cycle) {
        pt->cmd = (PartyCommand)(((int)pt->cmd + 1) % 3);
        char b[64]; snprintf(b, sizeof b, "Party command: %s", CMD_NAMES[pt->cmd]);
        toast_text(g, b);
    }
}


static void skip_tracked_quest(Game *g) {
    int q = g->ss.tracked;
    if (q < 0) for (int i = 0; i < g->story.nquest; ++i) if (g->ss.q[i].state == QS_ACTIVE || g->ss.q[i].state == QS_READY) { q = i; break; }
    if (q < 0) { toast_text(g, "No active quest to skip."); return; }
    const QuestDef *d = &g->story.quest[q];
    for (int k = 0; k < d->nobj; ++k) {
        if (d->obj[k].kind == OBJ_FLAG) { char key[64]; snprintf(key, sizeof key, "flag.%s", d->obj[k].target); flag_set(&g->ss, key, 1); }
        g->ss.q[q].prog[k] = (int16_t)d->obj[k].count;
    }
    story_update(&g->ss, &g->story, &g->sh);
    if (g->ss.q[q].state == QS_READY) story_complete_quest(&g->ss, &g->story, &g->sh, q);
    refresh_npcs(g);
}

static const char *DEV_REGIONS[] = {"aurelheim", "ashbrook", "whisperwood", "spire_academy", "fort_greywall", "forbidden_library", "sanctum_of_dawn", "qasr_amun", "khemet_ruins", "hrimvald", "emberforge", "front_camp"};

static void story_frame(Game *g, float dt, bool hidden) {
    bool paused = hidden || g->menu.open || g->dui.open;
    Combatant *acb = combat_get(&g->cw, active_ent(g));
    Vec3 ap = active_member(g)->pc.ch.pos;
    Vector3 apos = {ap.x, ap.y, ap.z};
    if (!paused) g->hour += dt / 60.0f;            /* one game hour per real minute */
    if (g->hour >= 24.0f) g->hour -= 24.0f;
    if (g->dbg.quest_skip_request) { skip_tracked_quest(g); g->dbg.quest_skip_request = false; }
    if (IsKeyPressed(KEY_N)) { float next[3] = {6.0f, 12.0f, 19.0f}; int s = sched_slot(g->hour); g->hour = next[(s + 1) % 3]; toast_text(g, s == 2 ? "Morning." : s == 0 ? "Afternoon." : "Evening."); }
    if (IsKeyPressed(KEY_F6)) {
        int cur = 0; for (unsigned i = 0; i < sizeof DEV_REGIONS / sizeof DEV_REGIONS[0]; ++i) if (!strcmp(DEV_REGIONS[i], g->region)) cur = (int)i;
        on_travel(g, DEV_REGIONS[(cur + 1) % (int)(sizeof DEV_REGIONS / sizeof DEV_REGIONS[0])]);
        char b[64]; snprintf(b, sizeof b, "Dev travel: %s", g->region); toast_text(g, b);
    }
    g->story_timer += dt;
    if (g->story_timer > 0.4f) {
        g->story_timer = 0;
        story_sync_collect(&g->ss, &g->story, &g->sh);
        story_update(&g->ss, &g->story, &g->sh);
        refresh_npcs(g);
    }
    /* explore objectives: entering a location hosted by this scene */
    if (!paused && !strcmp(g->region, "aurelheim")) {
        for (int i = 0; i < g->story.nloc && i < 128; ++i) {
            const Location *l = &g->story.loc[i];
            if (!l->in_sandbox) continue;
            float dx = ap.x - l->lx, dz = ap.z - l->lz;
            bool inside = dx * dx + dz * dz < l->radius * l->radius;
            if (inside && !g->loc_inside[i]) { story_on_location(&g->ss, &g->story, &g->sh, l->id); log_loot(g, (Color){150, 210, 255, 255}, "Discovered: %s", l->name, 0); }
            g->loc_inside[i] = inside;
        }
    }
    npcw_update(&g->npcs, &g->story, g->hour, hidden ? 0.0f : dt);
    /* talking */
    if (!g->dui.open) for (int i = 0; i < g->npcs.count; ++i) g->npcs.n[i].talking = false;
    g->near_npc = (!paused && acb && !acb->dead) ? npcw_nearest(&g->npcs, apos, 2.8f) : -1;
    g->input.talk_visible = g->near_npc >= 0;
    if (g->input.talk) {
        g->input.talk = false;
        if (g->near_npc >= 0 && !g->dui.open) {
            NpcInst *ni = &g->npcs.n[g->near_npc];
            DlgKind kind; int q;
            int tree = story_pick_dialogue(&g->ss, &g->story, &g->sh, ni->def, &kind, &q);
            if (tree >= 0 && dui_open(&g->dui, &g->ss, &g->story, &g->sh, tree, q, g->near_npc, false)) {
                ni->talking = true;
                npcw_face(&g->npcs, g->near_npc, apos);
                story_on_talk(&g->ss, &g->story, &g->sh, g->story.npc[ni->def].id);
            }
        }
    }
    if (g->dui.open && g->dui.npc >= 0 && g->dui.npc < g->npcs.count) g->npcs.n[g->dui.npc].talking = true;
}

static void frame(void) {
    Game *g = &G;
    float dt = GetFrameTime();
    int hidden = platform_is_hidden();
    g->loop.paused = hidden;
    g->time += (double)dt;

    if (platform_take_context_restored()) LOGI("WebGL context restored (GPU resource rebuild lands with the lifecycle milestone)");

    bool playing = g->state == STATE_PLAYING;
    input_update(&g->input, dt, playing && !hidden && !g->menu.open && !g->dui.open);
    if (playing && g->input.menu && !g->dui.open) { menu_toggle(&g->menu); if (g->menu.open && IsCursorHidden()) EnableCursor(); g->input.menu = false; }
    else g->input.menu = false;
    inv_update(&g->inv, g->menu.open ? 0.0f : dt);
    /* desktop: the first click in game grabs the mouse (pointer lock); Esc releases it */
    if (g->dui.open && IsCursorHidden()) EnableCursor();
    if (playing && !g->menu.open && !g->dui.open && !g->input.touch_ui && !IsCursorHidden() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) DisableCursor();
    if (IsKeyPressed(KEY_ESCAPE) && IsCursorHidden() && !g->menu.open) EnableCursor();

    /* hit-stop freezes the simulation and animation for a few frames, rendering continues */
    float sim_dt = hidden || g->menu.open || g->dui.open ? 0.0f : dt;
    if (g->hitstop > 0) { g->hitstop -= dt; sim_dt = 0.0f; }

    int steps = 0;
    if (playing) {
        if (!g->menu.open && !g->dui.open) do_party_controls(g);
        if (g->input.lock) toggle_lock(g);
        if (g->lock_ent != ENTITY_NULL && !combat_alive(&g->cw, g->lock_ent)) g->lock_ent = ENTITY_NULL;
        steps = gameloop_advance(&g->loop, (double)sim_dt);
        for (int s = 0; s < steps; ++s) sim_step(g, s == 0);
        if (steps > 0) input_consume(&g->input);
        for (int i = 0; i < ENEMY_MAX; ++i) { /* corpses vanish after their death animation */
            Enemy *en = &g->enemies.e[i];
            if (!en->used) continue;
            Combatant *c = combat_get(&g->cw, en->ent);
            if (c && c->dead && c->dead_t > 4.2f) enemy_remove(&g->enemies, &g->cw, &g->world, en->ent);
        }
    } else {
        g->title_t += (double)dt;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
            GetTouchPointCount() > 0) {
            new_game(g);
            g->state = STATE_PLAYING;
            g->auto_orbit = false;
            platform_report_state("playing");
        }
        /* the title backdrop keeps the world alive (idle enemies) */
        steps = gameloop_advance(&g->loop, (double)sim_dt);
        for (int s = 0; s < steps; ++s) { ecs_snapshot_transforms(&g->world); enemies_update(&g->enemies, &g->cw, &g->world, &g->phys, &g->edb, SIM_DT); combat_update(&g->cw, &g->world, &g->phys, SIM_DT); }
    }
    process_events(g);
    hud_update(&g->hud, dt);
    if (playing && g->story_ok) story_frame(g, dt, hidden);

    Combatant *acb = combat_get(&g->cw, active_ent(g));
    if (playing) {
        int alive = 0; float ehp = 0;
        for (int i = 0; i < ENEMY_MAX; ++i) { const Enemy *en = &g->enemies.e[i]; if (!en->used) continue; Combatant *c = combat_get(&g->cw, en->ent); if (c && !c->dead) { alive++; ehp += c->hp; } }
        PlayerCtl *apc = &active_member(g)->pc;
        platform_report_player(apc->ch.pos.x, apc->ch.pos.y, apc->ch.pos.z, apc->ch.grounded, apc->speed_xz);
        platform_report_combat(alive, ehp, acb ? acb->hp : 0.0f, g->kills, g->last_damage);
        float hp[4] = {0, 0, 0, 0}; int dead = 0;
        for (int i = 0; i < g->party.n && i < 4; ++i) { Combatant *c = party_cb(&g->party, &g->ctx, i); if (c) { hp[i] = c->hp; dead += c->dead; } }
        float fol = 0; /* furthest follower distance from the leader */
        for (int i = 0; i < g->party.n; ++i) if (i != g->party.active) { Vec3 a = g->party.m[i].pc.ch.pos, b = apc->ch.pos; fol = fmaxf(fol, sqrtf((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z))); }
        platform_report_inventory(g->inv.gold, INV_SLOTS - inv_free_slots(&g->inv), g->menu.open, acb ? acb->atk : 0.0f);
        if (g->story_ok) { int act_q = 0, done_q = 0; for (int i = 0; i < g->story.nquest; ++i) done_q += g->ss.q[i].state == QS_COMPLETE, act_q += g->ss.q[i].state == QS_ACTIVE || g->ss.q[i].state == QS_READY; platform_report_story(story_mq(&g->ss, &g->story), story_act(&g->ss, &g->story), g->npcs.count, g->dui.open, act_q, g->ss.tracked, done_q); }
        platform_report_party(g->party.active, (int)g->party.cmd, dead, fol, hp[0], hp[1], hp[2], hp[3]);
    }
    dbg_update(&g->dbg, dt, steps);
    g->dbg.draw_calls = 0;
    handle_render_keys(g);
    renderer_governor_update(dt, g->governor_on && !hidden);

    float alpha = gameloop_alpha(&g->loop);
    Entity act = active_ent(g);
    Vec3 pp = ecs_render_pos(&g->world, ecs_index(act), alpha);

    /* camera follows the controlled member, smoothing the hand-over when control switches */
    Vec3 want = vec3(pp.x, pp.y, pp.z);
    g->cam_focus = vec3_lerp(g->cam_focus, want, 1.0f - expf(-10.0f * dt));
    float ldx = g->input.look_dx, ldy = g->input.look_dy;
    if (g->auto_orbit) g->cam.yaw += dt * 0.12f;
    g->cam.locked = g->lock_ent != ENTITY_NULL;
    if (g->cam.locked) { Vector3 lp = lock_pos(g); g->cam.lock_pos = vec3(lp.x, 0, lp.z); }
    camrig_update(&g->cam, &g->phys, vec3(g->cam_focus.x, g->cam_focus.y + 1.3f, g->cam_focus.z), ldx, ldy, dt);
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) g->cam.dist -= wheel * 0.6f;
    Camera3D cam = {.position = {g->cam.eye.x, g->cam.eye.y, g->cam.eye.z}, .target = {g->cam.target.x, g->cam.target.y, g->cam.target.z},
                    .up = {0, 1, 0}, .fovy = 38.0f, .projection = CAMERA_PERSPECTIVE};

    float anim_dt = !hidden && !g->menu.open && !g->dui.open && g->hitstop <= 0 ? dt : 0.0f;
    for (int i = 0; i < g->party.n; ++i) {
        PartyMember *m = &g->party.m[i];
        Combatant *c = combat_get(&g->cw, m->ent);
        MemberMotion mm = {.speed = m->pc.speed_xz, .grounded = m->pc.ch.grounded, .casting = player_attacking(&m->pc), .rolling = player_rolling(&m->pc),
                           .ev_roll = g->ev_roll[i], .ev_cast = g->ev_cast[i], .cast_clip = -1, .roll_seconds = PLAYER_ROLL_TIME, .cast_seconds = PLAYER_ATTACK_TIME};
        if (g->ev_cast[i] && g->cast_skill[i] >= 0) {
            const Skill *s = &g->skills.s[g->cast_skill[i]];
            mm.cast_clip = heroes_clip(s->anim[0] ? s->anim : "Sword_Attack"); mm.cast_seconds = s->total_time;
        }
        if (c) {
            mm.dead = c->dead; mm.staggered = c->stagger_t > 0; mm.guarding = c->guarding;
            mm.flash = c->hit_flash > 0 ? fminf(1.0f, c->hit_flash / 0.12f) : 0.0f;
        }
        g->ev_roll[i] = g->ev_cast[i] = false;
        heroes_update(i, playing ? anim_dt : 0.0f, &mm);
    }

    renderer_begin_frame(cam, (float)g->time);
    test_scene_submit((float)g->time);
    for (int i = 0; i < g->party.n; ++i) {
        Vec3 mp = ecs_render_pos(&g->world, ecs_index(g->party.m[i].ent), alpha);
        heroes_submit(i, (Vector3){mp.x, mp.y, mp.z}, ecs_render_yaw(&g->world, ecs_index(g->party.m[i].ent), alpha), (float)g->time);
    }
    if (g->story_ok) { Vec3 fp = active_member(g)->pc.ch.pos; npcw_submit(&g->npcs, &g->story, (Vector3){fp.x, fp.y, fp.z}, 30.0f, 10); }
    for (int i = 0; i < ENEMY_MAX; ++i) {
        const Enemy *en = &g->enemies.e[i];
        if (!en->used) continue;
        Combatant *c = combat_get(&g->cw, en->ent);
        if (!c) continue;
        uint32_t ei = ecs_index(en->ent);
        const EnemyDef *def = &g->edb.e[en->def];
        float spd = sqrtf(g->world.vel[ei].x * g->world.vel[ei].x + g->world.vel[ei].z * g->world.vel[ei].z);
        actor_enemy_update(&g->actors[i], c, def, &g->skills, spd, anim_dt);
        Vec3 ep = ecs_render_pos(&g->world, ei, alpha);
        actor_enemy_submit(&g->actors[i], c, def, (Vector3){ep.x, ep.y, ep.z}, ecs_render_yaw(&g->world, ei, alpha));
    }
    if (g->lock_ent != ENTITY_NULL && ecs_alive(&g->world, g->lock_ent)) {
        Vector3 t = lock_pos(g);
        Combatant *lc = combat_get(&g->cw, g->lock_ent);
        proc_submit_marker((Vector3){t.x, t.y + (lc ? lc->height : 1.8f) + 0.9f, t.z}, (float)g->time);
    }
    g->dbg.draw_calls = renderer_queue_count();

    BeginDrawing();
    renderer_end_frame();
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    if (g->state == STATE_TITLE) {
        const char *title = "SOLO ASCENT";
        int fs = sh / 8;
        DrawText(title, (sw - MeasureText(title, fs)) / 2 + 3, sh / 7 + 3, fs, (Color){0, 0, 0, 160});
        DrawText(title, (sw - MeasureText(title, fs)) / 2, sh / 7, fs, GOLD);
        const char *sub = "The S-Rank Hero";
        DrawText(sub, (sw - MeasureText(sub, fs / 2)) / 2, sh / 7 + fs, fs / 2, RAYWHITE);
        if (((int)(g->title_t * 2)) % 2 == 0) {
            const char *p = "Press ENTER / Tap to begin";
            DrawText(p, (sw - MeasureText(p, 24)) / 2, sh * 6 / 7, 24, RAYWHITE);
        }
    } else {
        hud_draw(&g->hud, cam, &g->world, &g->cw, &g->enemies, &g->edb, &g->party, g->lock_ent, g->input.touch_ui);
        if (!g->input.touch_ui)
            DrawText(IsCursorHidden() ? "WASD | Shift | Space | J attack | 1-4 skills | E guard | Q roll | Tab lock | R/5-8 switch | B command | I menu | Z/X/C/V potions | Esc mouse"
                                      : "Click to capture the mouse | WASD | J attack | 1-4 skills | E guard | Q roll | Tab lock | R/5-8 switch | B command | I menu | Z/X/C/V potions",
                     10, sh - 24, 16, (Color){255, 255, 255, 170});
        if (g->story_ok) {
            StoryHudCtx sc = {.cam = cam, .cam_yaw = g->cam.yaw, .w = &g->world, .cw = &g->cw, .enemies = &g->enemies, .party = &g->party, .db = &g->story, .ss = &g->ss,
                              .npcs = &g->npcs, .focus = {active_member(g)->pc.ch.pos.x, active_member(g)->pc.ch.pos.y, active_member(g)->pc.ch.pos.z},
                              .hour = g->hour, .region_name = g->region, .near_npc = g->near_npc, .touch_ui = g->input.touch_ui};
            story_hud_draw(&sc);
        }
        input_draw_touch(&g->input);
        for (int i = 0; i < 6; ++i) {
            if (g->loot_log[i].t <= 0) continue;
            g->loot_log[i].t -= dt;
            unsigned char a = (unsigned char)(255.0f * fminf(1.0f, g->loot_log[i].t));
            Color cc = g->loot_log[i].col; cc.a = a;
            DrawText(g->loot_log[i].text, sw - 20 - MeasureText(g->loot_log[i].text, 18), sh / 2 + i * 22, 18, cc);
        }
        if (g->story_ok && !g->menu.open) dui_update_draw(&g->dui, &g->ss, &g->story, &g->sh, dt);
        if (g->menu.open) {
            MenuCtx mc = {&g->inv, &g->idb, &g->skills, &g->pdb, &g->party, &g->cw, g->gear_level, &g->story, &g->ss, &g->sh, g->region, on_travel, g};
            menu_update_draw(&g->menu, &mc);
            if (g->menu.gear_dirty) { apply_all_gear(g); g->menu.gear_dirty = false; }
        }
    }
    if (g->toast_t > 0) {
        g->toast_t -= (double)dt;
        DrawText(g->toast, 14, 14, 24, (Color){255, 255, 255, 230});
    }
    if (hidden) DrawText("PAUSED", sw / 2 - 40, sh / 2, 30, WHITE);
    char extra[192];
    int rw, rh;
    renderer_target_size(&rw, &rh);
    snprintf(extra, sizeof extra, "%s%s %dx%d | look: %s | %d meshes | %s%s", renderer_tier_settings(renderer_tier())->name,
             g->governor_on ? (renderer_governor_settled() ? " (auto)" : " (benchmarking)") : " (manual)", rw, rh,
             renderer_look_name(renderer_look()), renderer_queue_count(), active_member(g)->pc.ch.grounded ? "grounded" : "airborne",
             player_invulnerable(&active_member(g)->pc) ? " | I-FRAMES" : "");
    dbg_draw(&g->dbg, g->world.alive_count, g->loop.sim_ticks, (unsigned long)platform_heap_bytes(), extra);
    EndDrawing();
}

static QualityTier parse_tier(const char *s) {
    static const char *names[] = {"ultra", "high", "medium", "low", "mobile"};
    for (int i = 0; i < TIER_COUNT; ++i) if (strcmp(s, names[i]) == 0) return (QualityTier)i;
    return TIER_HIGH;
}

int main(int argc, char **argv) {
    /* args (from URL query on web): --look N  --tier name  --still  --mobile  --hero  --rack  --arena */
    int look = 2;
    QualityTier tier = TIER_HIGH;
    bool tier_forced = false, still = false, hero_only = false, rack = false;
    G.arena = false;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--look") && i + 1 < argc) look = atoi(argv[++i]) - 1 < 0 ? 0 : atoi(argv[i]) - 1;
        else if (!strcmp(argv[i], "--tier") && i + 1 < argc) { tier = parse_tier(argv[++i]); tier_forced = true; }
        else if (!strcmp(argv[i], "--still")) still = true;
        else if (!strcmp(argv[i], "--hero")) hero_only = true;
        else if (!strcmp(argv[i], "--rack")) rack = true;
        else if (!strcmp(argv[i], "--arena")) G.arena = true;
        else if (!strcmp(argv[i], "--mobile") && !tier_forced) tier = TIER_LOW;
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Solo Ascent");
    SetExitKey(KEY_NULL);
    platform_init();

    ecs_init(&G.world);
    gameloop_init(&G.loop);
    dbg_init(&G.dbg);
    input_init(&G.input);
    G.data_ok = skilldb_load(&G.skills, "data/skills.json") && enemydb_load(&G.edb, &G.skills, "data/enemies.json") &&
                partydb_load(&G.pdb, &G.skills, "data/party.json") && itemdb_load(&G.idb, "data", &G.edb);
    G.story_ok = storydb_load(&G.story, "data", &G.edb, &G.idb);
    if (!G.story_ok) LOGE("story data failed to load");
    G.gear_level = 25; /* sandbox: all gear usable until the XP system lands */
    G.loot_rng = 20261009u;
    if (!G.data_ok) LOGE("content data failed to load");
    test_scene_build_physics(&G.phys);
    hud_init(&G.hud);
    new_game(&G);
    G.state = STATE_TITLE;
    camrig_init(&G.cam, 0.0f, 0.16f, 6.0f);
    G.auto_orbit = !still;
    G.governor_on = !tier_forced;

    if (hero_only) { test_scene_set_showcase(true); test_scene_set_rack(rack); G.cam.dist = rack ? 7.5f : 4.4f; G.cam.cur_dist = G.cam.dist; G.cam.pitch = rack ? 0.25f : 0.04f; G.cam.yaw = rack ? 0.0f : 0.35f; }
    proc_init();
    heroes_init();
    if (G.story_ok) { npcw_init(&G.npcs); refresh_npcs(&G); }
    actors_init();
    renderer_init(GetScreenWidth(), GetScreenHeight(), tier, look);
    platform_report_state("title");
    LOGI("Solo Ascent boot OK");

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) frame();
    renderer_shutdown();
    proc_shutdown();
    CloseWindow();
#endif
    return 0;
}
