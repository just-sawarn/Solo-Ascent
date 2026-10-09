#ifndef SA_ENDING_H
#define SA_ENDING_H
/* The ending screen: epilogue text for the standard / true ending, scrolling credits, and the New Game+ prompt. */
#include <stdbool.h>

typedef struct { float t; bool true_ending; int ng_plus; int kills; float play_seconds; } EndingUI;
void ending_start(EndingUI *e, bool true_ending, int ng_plus, int kills, float play_seconds);
/* Returns 1 = New Game+ chosen, 2 = back to title, 0 = still showing. */
int  ending_update_draw(EndingUI *e, float dt);

#endif
