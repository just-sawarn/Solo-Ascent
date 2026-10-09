#ifndef SA_PLATFORM_H
#define SA_PLATFORM_H
/* Thin platform interface. Game code calls only this; browser specifics live in platform/web. */
#include <stddef.h>

void   platform_init(void);
size_t platform_heap_bytes(void);                 /* WASM heap size; 0 on native */
void   platform_report_state(const char *state);  /* exposes state to JS test hooks */
/* test hook: exposes the player's position/state as window.__sa.player */
void   platform_report_player(float x, float y, float z, int grounded, float speed);
/* test hook: combat summary as window.__sa.combat */
void   platform_report_combat(int enemies_alive, float enemy_hp_total, float player_hp, int kills, float last_damage);
/* test hook: party summary as window.__sa.party */
void   platform_report_party(int active, int command, int dead, float follower_max_dist, float hp0, float hp1, float hp2, float hp3);
/* test hook: inventory summary as window.__sa.inventory */
void   platform_report_inventory(int gold, int item_count, int menu_open, float atk_kael);
/* test hook: story summary as window.__sa.story */
void   platform_report_story(int mq, int act, int npcs, int dialogue_open, int active_quests, int tracked, int completed);
/* On-demand asset packs (streamed archives). Native builds read assets straight from disk: always ready. */
typedef enum { PACK_NONE = 0, PACK_LOADING = 1, PACK_READY = 2, PACK_FAILED = 3 } PackState;
void      platform_pack_request(const char *name);  /* start streaming; idempotent */
PackState platform_pack_state(const char *name);
/* test hook: world summary as window.__sa.world */
void   platform_report_world(const char *zone, int gen, int region, int weather, float intensity, float hour, int props, int particles, int enemies);
int    platform_is_hidden(void);                  /* tab hidden => sim paused */
int    platform_context_lost(void);
int    platform_take_context_restored(void);      /* true once after a restore */

#endif
