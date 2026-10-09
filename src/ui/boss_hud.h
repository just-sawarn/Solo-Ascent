#ifndef SA_BOSS_HUD_H
#define SA_BOSS_HUD_H
/* Boss health bar with phase pips, mechanic callouts, Solo Ascent and Quartet Strike prompts, impact-frame overlay. */
#include <raylib.h>
#include "core/boss.h"

typedef struct { float t; char text[48]; Color col; } ImpactFrame;

void boss_hud_draw(const BossRt *rt, const BossDB *db, PartyCtx *pc, float time, bool touch);
void impact_frame_draw(const ImpactFrame *f);   /* white flash + speed lines + the skill's name */

#endif
