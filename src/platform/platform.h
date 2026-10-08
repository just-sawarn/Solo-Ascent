#ifndef SA_PLATFORM_H
#define SA_PLATFORM_H
/* Thin platform interface. Game code calls only this; browser specifics live in platform/web. */
#include <stddef.h>

void   platform_init(void);
size_t platform_heap_bytes(void);                 /* WASM heap size; 0 on native */
void   platform_report_state(const char *state);  /* exposes state to JS test hooks */
int    platform_is_hidden(void);                  /* tab hidden => sim paused */
int    platform_context_lost(void);
int    platform_take_context_restored(void);      /* true once after a restore */

#endif
