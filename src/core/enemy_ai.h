#ifndef SA_ENEMY_AI_H
#define SA_ENEMY_AI_H
/* Enemy brains: aggro, chase / kite, telegraphed attacks through the combat system, recovery, leash. */
#include "core/combat.h"

#define ENEMY_MAX 48

typedef enum { AI_IDLE, AI_CHASE, AI_ATTACK, AI_RECOVER, AI_RETURN } AiState;

typedef struct {
    bool used;
    Entity ent;
    int def;               /* index into the EnemyDB */
    AiState state;
    float timer, strafe_dir, yaw;
    Entity target;
    Vec3 home;
    CharCtl ctl;
} Enemy;

typedef struct { Enemy e[ENEMY_MAX]; } EnemyPool;

void   enemies_init(EnemyPool *p);
/* Creates the ECS entity, combatant and brain. Returns NULL when out of slots. */
Enemy *enemy_spawn(EnemyPool *p, CombatWorld *cw, World *w, const EnemyDB *db, int def, Vec3 pos);
void   enemies_update(EnemyPool *p, CombatWorld *cw, World *w, PhysWorld *phys, const EnemyDB *db, float dt);
Enemy *enemy_of(EnemyPool *p, Entity e);
void   enemy_remove(EnemyPool *p, CombatWorld *cw, World *w, Entity e);

#endif
