#include "platform/platform.h"

static volatile int g_hidden, g_ctx_lost, g_ctx_restored;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/heap.h>

EMSCRIPTEN_KEEPALIVE void sa_on_visibility(int hidden) { g_hidden = hidden; }
EMSCRIPTEN_KEEPALIVE void sa_on_context_lost(void) { g_ctx_lost = 1; }
EMSCRIPTEN_KEEPALIVE void sa_on_context_restored(void) { g_ctx_lost = 0; g_ctx_restored = 1; }

void platform_init(void) {
    EM_ASM({
        /* quoted: closure must not rename properties set by web/glue.js */
        if (typeof Module['saHooks'] === 'function') Module['saHooks']();
    });
}
size_t platform_heap_bytes(void) { return (size_t)emscripten_get_heap_size(); }
void platform_report_state(const char *state) {
    /* bracket access: closure must not rename properties read by glue/tests */
    EM_ASM({ window['__sa'] = window['__sa'] || {}; window['__sa']['state'] = UTF8ToString($0); }, state);
}
void platform_report_player(float x, float y, float z, int grounded, float speed) {
    /* NOTE: no commas outside parentheses inside EM_ASM bodies (the preprocessor would split them) */
    EM_ASM({
        var s = (window["__sa"] = window["__sa"] || {});
        var o = {};
        o["x"] = $0; o["y"] = $1; o["z"] = $2; o["grounded"] = !!$3; o["speed"] = $4;
        s["player"] = o;
    }, x, y, z, grounded, speed);
}
void platform_report_combat(int enemies_alive, float enemy_hp_total, float player_hp, int kills, float last_damage) {
    EM_ASM({
        var s = (window["__sa"] = window["__sa"] || {});
        var o = {};
        o["enemies"] = $0; o["enemyHp"] = $1; o["playerHp"] = $2; o["kills"] = $3; o["lastDamage"] = $4;
        s["combat"] = o;
    }, enemies_alive, enemy_hp_total, player_hp, kills, last_damage);
}
void platform_report_party(int active, int command, int dead, float follower_max_dist, float hp0, float hp1, float hp2, float hp3) {
    EM_ASM({
        var s = (window["__sa"] = window["__sa"] || {});
        var o = {};
        o["active"] = $0; o["command"] = $1; o["dead"] = $2; o["followerDist"] = $3;
        var hp = new Array(); hp.push($4); hp.push($5); hp.push($6); hp.push($7); o["hp"] = hp;
        s["party"] = o;
    }, active, command, dead, follower_max_dist, hp0, hp1, hp2, hp3);
}
void platform_report_inventory(int gold, int item_count, int menu_open, float atk_kael) {
    EM_ASM({
        var s = (window["__sa"] = window["__sa"] || {});
        var o = {};
        o["gold"] = $0; o["items"] = $1; o["menu"] = !!$2; o["atkKael"] = $3;
        s["inventory"] = o;
    }, gold, item_count, menu_open, atk_kael);
}
void platform_report_story(int mq, int act, int npcs, int dialogue_open, int active_quests, int tracked, int completed) {
    EM_ASM({
        var s = (window["__sa"] = window["__sa"] || {});
        var o = {};
        o["mq"] = $0; o["act"] = $1; o["npcs"] = $2; o["dialogue"] = !!$3; o["active"] = $4; o["tracked"] = $5; o["completed"] = $6;
        s["story"] = o;
    }, mq, act, npcs, dialogue_open, active_quests, tracked, completed);
}
void platform_report_world(const char *zone, int gen, int region, int weather, float intensity, float hour, int props, int particles, int enemies) {
    EM_ASM({
        var s = (window["__sa"] = window["__sa"] || {});
        var o = {};
        o["zone"] = UTF8ToString($0); o["gen"] = $1; o["region"] = $2; o["weather"] = $3; o["intensity"] = $4; o["hour"] = $5;
        o["props"] = $6; o["particles"] = $7; o["enemies"] = $8;
        s["world"] = o;
    }, zone, gen, region, weather, intensity, hour, props, particles, enemies);
}
void platform_pack_request(const char *name) {
    EM_ASM({ var n = UTF8ToString($0); if (window["SoloAscentPacks"]) window["SoloAscentPacks"]["request"](n); }, name);
}
PackState platform_pack_state(const char *name) {
    return (PackState)EM_ASM_INT({ var n = UTF8ToString($0); return window["SoloAscentPacks"] ? window["SoloAscentPacks"]["state"](n) : 0; }, name);
}
#else
void platform_report_world(const char *z, int a, int b, int c, float d, float e, int f, int g, int h) { (void)z; (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; }
void platform_pack_request(const char *name) { (void)name; }
PackState platform_pack_state(const char *name) { (void)name; return PACK_READY; }
void platform_report_story(int a, int b, int c, int d, int e, int f, int h) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)h; }
void platform_report_inventory(int a, int b, int c, float d) { (void)a; (void)b; (void)c; (void)d; }
void platform_report_party(int a, int b, int c, float d, float e, float f, float g, float h) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; }
void platform_report_combat(int a, float b, float c, int d, float e) { (void)a; (void)b; (void)c; (void)d; (void)e; }
void platform_report_player(float x, float y, float z, int grounded, float speed) { (void)x; (void)y; (void)z; (void)grounded; (void)speed; }
void platform_init(void) {}
size_t platform_heap_bytes(void) { return 0; }
void platform_report_state(const char *state) { (void)state; }
#endif

int platform_is_hidden(void) { return g_hidden; }
int platform_context_lost(void) { return g_ctx_lost; }
int platform_take_context_restored(void) { int r = g_ctx_restored; g_ctx_restored = 0; return r; }
