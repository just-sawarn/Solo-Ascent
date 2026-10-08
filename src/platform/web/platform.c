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
#else
void platform_init(void) {}
size_t platform_heap_bytes(void) { return 0; }
void platform_report_state(const char *state) { (void)state; }
#endif

int platform_is_hidden(void) { return g_hidden; }
int platform_context_lost(void) { return g_ctx_lost; }
int platform_take_context_restored(void) { int r = g_ctx_restored; g_ctx_restored = 0; return r; }
