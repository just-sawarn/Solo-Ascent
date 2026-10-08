#ifndef SA_DEBUG_OVERLAY_H
#define SA_DEBUG_OVERLAY_H
#include <stdbool.h>

typedef struct {
    bool visible;
    bool god_mode;           /* consumed by combat in later milestones */
    bool quest_skip_request; /* consumed (and cleared) by the quest system */
    int  draw_calls;         /* submitted by game code this frame (see dbg_count_draw) */
    int  sim_steps_last;
    float frame_ms[120];
    int   frame_head;
    float avg_ms, worst_ms;
} DebugState;

void dbg_init(DebugState *d);
void dbg_count_draw(DebugState *d);
void dbg_update(DebugState *d, float frame_dt_s, int sim_steps);
/* extra: renderer status line (may be NULL) */
void dbg_draw(const DebugState *d, int alive_entities, unsigned long long ticks, unsigned long heap_bytes, const char *extra);

#endif
