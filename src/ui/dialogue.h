#ifndef SA_DIALOGUE_UI_H
#define SA_DIALOGUE_UI_H
/* Dialogue box with typewriter text, speaker names, numbered choices (keys 1-4, arrows + Enter, mouse/touch) and a
 * cinematic letterbox for cutscene scripts. Pauses the game while open. */
#include <stdbool.h>
#include "core/story.h"

#define DUI_QUEUE 8

typedef struct {
    bool open, cinematic;
    DlgRunner r;
    int npc;                       /* npc index being spoken to, -1 none */
    float reveal;                  /* characters revealed */
    int sel;
    const DlgNode *shown;
    char queue[DUI_QUEUE][48]; int nqueue;   /* cutscene tree names waiting to play */
} DialogueUI;

void dui_init(DialogueUI *d);
bool dui_open(DialogueUI *d, StoryState *s, const StoryDB *db, const StoryHooks *h, int tree, int quest, int npc, bool cinematic);
void dui_queue_cutscene(DialogueUI *d, const char *name);          /* "cs." is prepended */
/* Call every frame: starts queued cutscenes when idle, handles input and draws. Returns true while a conversation is on screen. */
bool dui_update_draw(DialogueUI *d, StoryState *s, const StoryDB *db, const StoryHooks *h, float dt);
const char *speaker_name(const StoryDB *db, const StoryState *s, const char *speaker);

#endif
