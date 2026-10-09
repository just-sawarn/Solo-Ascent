#ifndef SA_HUD_H
#define SA_HUD_H
/* Combat HUD: floating damage numbers, health bars, player vitals, skill bar, resonance gauge. */
#include <raylib.h>
#include "core/combat.h"
#include "core/enemy_ai.h"
#include "core/party.h"

#define HUD_MAX_NUMBERS 64

typedef struct { bool used; Vector3 pos; float t, amount; int kind; } HudNumber;
typedef struct {
    HudNumber n[HUD_MAX_NUMBERS];
    int head;
    float bar_t[ECS_MAX_ENTITIES];     /* seconds an enemy health bar stays visible after a hit */
} Hud;

void hud_init(Hud *h);
void hud_on_events(Hud *h, const CombatEvent *ev, int n, const Party *party);
void hud_update(Hud *h, float dt);
void hud_draw(const Hud *h, Camera3D cam, World *w, CombatWorld *cw, const EnemyPool *pool, const EnemyDB *db,
              const Party *party, Entity locked, bool touch_ui);

#endif
