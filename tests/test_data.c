#include "testutil.h"
#include <string.h>
#include "core/data.h"

static SkillDB S; static EnemyDB E;

int main(void) {
    CHECK(skilldb_load(&S, SA_DATA_DIR "/skills.json"));
    int by_role[5] = {0};
    int playable = 0;
    for (int i = 0; i < S.n; ++i) {
        const Skill *s = &S.s[i];
        by_role[s->role]++;
        if (!s->basic && s->role != ROLE_ENEMY) playable++;
        CHECK(s->hit_time <= s->total_time);
        CHECK(s->id[0] && s->name[0]);
        for (int j = i + 1; j < S.n; ++j) CHECK(strcmp(s->id, S.s[j].id) != 0);
    }
    CHECK(playable >= 40);                               /* design target: at least 40 skills across the 4 roles */
    CHECK(by_role[ROLE_TANK] >= 8 && by_role[ROLE_MAGE] >= 8 && by_role[ROLE_HEALER] >= 8 && by_role[ROLE_HERO] >= 12);
    CHECK(skilldb_find(&S, "heroic_slash") >= 0 && skilldb_find(&S, "nonexistent") < 0);
    /* story skills exist under their canonical ids */
    const char *story[] = {"heroic_slash", "flame_parry", "sunder", "gale_rush", "dawn_arc", "taunt", "iron_wall", "unbroken_vow",
                           "fireball", "frost_lance", "arcane_shield", "starfall", "heal", "cleanse", "blessing", "radiant_renewal", "resolve"};
    for (unsigned i = 0; i < sizeof story / sizeof story[0]; ++i) CHECK(skilldb_find(&S, story[i]) >= 0);

    CHECK(enemydb_load(&E, &S, SA_DATA_DIR "/enemies.json"));
    CHECK(E.n >= 3);
    CHECK(enemydb_find(&E, "shadow_soldier") >= 0);
    CHECK(E.e[enemydb_find(&E, "shadow_soldier")].resist[EL_HOLY] > 1.0f);

    /* missing file fails cleanly */
    SkillDB bad;
    CHECK(!skilldb_load(&bad, SA_DATA_DIR "/does_not_exist.json"));
    TEST_DONE();
}
