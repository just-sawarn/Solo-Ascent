#ifndef SA_ACTORS_H
#define SA_ACTORS_H
/* Enemy presentation on the shared CC0 rig: animation selection from combat/AI state, outfit paint from data. */
#include <raylib.h>
#include "core/combat.h"
#include "core/enemy_ai.h"
#include "render/anim.h"

typedef struct {
    AnimPlayer anim;
    bool init, was_staggered, was_casting;
    int cur_cast;
} ActorAnim;

void actors_init(void);
void actor_enemy_update(ActorAnim *a, const Combatant *c, const EnemyDef *d, const SkillDB *skills, float speed, float dt);
void actor_enemy_submit(const ActorAnim *a, const Combatant *c, const EnemyDef *d, Vector3 pos, float yaw);

#endif
