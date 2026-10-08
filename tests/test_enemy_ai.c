#include "testutil.h"
#include <math.h>
#include "core/enemy_ai.h"

static World W;
static CombatWorld CW;
static PhysWorld PH;
static SkillDB S;
static EnemyDB E;
static EnemyPool P;
static const float DT = 1.0f / 60.0f;

static void tick(float seconds) {
    for (int i = 0; i < (int)(seconds * 60.0f + 0.5f); ++i) {
        for (int k = 0; k < CB_MAX; ++k) if (CW.c[k].used && CW.c[k].team == TEAM_PLAYER) { /* player stands still */ }
        enemies_update(&P, &CW, &W, &PH, &E, DT);
        combat_update(&CW, &W, &PH, DT);
    }
}
static Entity add_player(Vec3 pos) {
    Entity e = ecs_create(&W, C_TRANSFORM | C_HEALTH);
    W.pos[ecs_index(e)] = pos;
    combat_add(&CW, &W, e, TEAM_PLAYER, 3, (Stats){10, 4, 8, 6, 6}, 100);
    return e;
}
static void reset(void) { ecs_init(&W); phys_init(&PH, 0); combat_init(&CW, &S); enemies_init(&P); }
static float dist(Vec3 a, Vec3 b) { return sqrtf((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z)); }

int main(void) {
    CHECK(skilldb_load(&S, SA_DATA_DIR "/skills.json"));
    CHECK(enemydb_load(&E, &S, SA_DATA_DIR "/enemies.json"));
    int soldier = enemydb_find(&E, "shadow_soldier"), archer = enemydb_find(&E, "shadow_archer"), brute = enemydb_find(&E, "shadow_brute");

    /* idle until the player is inside aggro range, then chase and attack */
    reset();
    Entity pl = add_player(vec3(0, 0, 0));
    Enemy *en = enemy_spawn(&P, &CW, &W, &E, soldier, vec3(0, 0, 25));
    CHECK(en != NULL);
    tick(1.0f); CHECK(en->state == AI_IDLE);
    en->ctl.pos = vec3(0, 0, 10); W.pos[ecs_index(en->ent)] = en->ctl.pos;
    tick(3.0f);
    CHECK(en->state != AI_IDLE);
    CHECK(dist(W.pos[ecs_index(en->ent)], W.pos[ecs_index(pl)]) < 2.6f);
    tick(6.0f);
    Combatant *cp = combat_get(&CW, pl);
    CHECK(cp->hp < cp->max_hp);                                 /* it attacked */

    /* the attack is telegraphed: damage never lands before the windup finishes */
    reset();
    pl = add_player(vec3(0, 0, 0)); cp = combat_get(&CW, pl);
    en = enemy_spawn(&P, &CW, &W, &E, soldier, vec3(0, 0, 2.0f));
    float first_hit_t = -1; float t = 0;
    for (int i = 0; i < 600 && first_hit_t < 0; ++i) { tick(DT); t += DT; if (cp->hp < cp->max_hp) first_hit_t = t; }
    CHECK(first_hit_t >= S.s[skilldb_find(&S, "shadow_slash")].hit_time - 0.02f);

    /* staggered enemies do nothing */
    reset();
    pl = add_player(vec3(0, 0, 0)); cp = combat_get(&CW, pl);
    en = enemy_spawn(&P, &CW, &W, &E, soldier, vec3(0, 0, 1.5f));
    combat_get(&CW, en->ent)->stagger_t = 3.0f;
    tick(2.5f); CHECK(cp->hp == cp->max_hp);

    /* archers keep their distance and shoot */
    reset();
    pl = add_player(vec3(0, 0, 0)); cp = combat_get(&CW, pl);
    en = enemy_spawn(&P, &CW, &W, &E, archer, vec3(0, 0, 3.0f));
    tick(6.0f);
    float d = dist(W.pos[ecs_index(en->ent)], W.pos[ecs_index(pl)]);
    CHECK(d > 4.5f && d < 12.0f); /* backs off to ~60% of preferred range (8 m) and holds */
    CHECK(cp->hp < cp->max_hp);

    /* walls block line of fire */
    reset();
    pl = add_player(vec3(0, 0, 0)); cp = combat_get(&CW, pl);
    phys_add_box(&PH, vec3(-30, 0, 4), vec3(30, 6, 5));
    en = enemy_spawn(&P, &CW, &W, &E, archer, vec3(0, 0, 9.0f));
    tick(5.0f); CHECK(cp->hp == cp->max_hp);

    /* brute: slow, big poise, area slam */
    reset();
    pl = add_player(vec3(0, 0, 0)); cp = combat_get(&CW, pl);
    en = enemy_spawn(&P, &CW, &W, &E, brute, vec3(0, 0, 4.0f));
    CHECK(combat_get(&CW, en->ent)->poise_max >= 200);
    tick(8.0f); CHECK(cp->hp < cp->max_hp);

    /* taunt redirects the target */
    reset();
    Entity a = add_player(vec3(0, 0, 0)), b = add_player(vec3(6, 0, 0));
    en = enemy_spawn(&P, &CW, &W, &E, soldier, vec3(1, 0, 4));
    tick(0.2f); CHECK(en->target == a);
    combat_get(&CW, en->ent)->status[ST_TAUNT] = (Status){5.0f, 1.0f, b};
    tick(0.2f); CHECK(en->target == b);

    /* leash: loses a distant target and returns home */
    reset();
    pl = add_player(vec3(0, 0, 0));
    en = enemy_spawn(&P, &CW, &W, &E, soldier, vec3(0, 0, 8));
    tick(1.0f); CHECK(en->state == AI_CHASE || en->state == AI_ATTACK || en->state == AI_RECOVER);
    W.pos[ecs_index(pl)] = vec3(0, 0, -60);
    tick(10.0f);
    CHECK(en->state == AI_RETURN || en->state == AI_IDLE);
    CHECK(dist(en->ctl.pos, en->home) < 12.0f);

    /* dead enemies stay put, and the slot is reusable */
    reset();
    pl = add_player(vec3(0, 0, 0));
    en = enemy_spawn(&P, &CW, &W, &E, soldier, vec3(0, 0, 3));
    Entity dead = en->ent; Combatant *cd = combat_get(&CW, dead); cd->hp = 0; cd->dead = true;
    Vec3 where = en->ctl.pos; tick(1.0f);
    CHECK(dist(en->ctl.pos, where) < 0.1f);
    enemy_remove(&P, &CW, &W, dead);
    CHECK(!ecs_alive(&W, dead)); CHECK(enemy_of(&P, dead) == NULL);
    CHECK(enemy_spawn(&P, &CW, &W, &E, soldier, vec3(0, 0, 3)) != NULL);
    TEST_DONE();
}
