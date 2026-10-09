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

    PartyDB P;
    CHECK(partydb_load(&P, &S, SA_DATA_DIR "/party.json"));
    CHECK(P.n == 4);
    CHECK(P.m[0].role == ROLE_HERO && P.m[1].role == ROLE_TANK && P.m[2].role == ROLE_MAGE && P.m[3].role == ROLE_HEALER);
    for (int i = 0; i < P.n; ++i) { CHECK(P.m[i].nskills >= 8); CHECK(P.m[i].nbasic >= 1); for (int k = 0; k < 4; ++k) CHECK(P.m[i].slots[k] >= 0); }
    /* every member only knows skills of their own role (hero skills may be used by Kael only) */
    for (int i = 0; i < P.n; ++i) for (int k = 0; k < P.m[i].nskills; ++k) CHECK(S.s[P.m[i].skills[k]].role == P.m[i].role);

    /* missing file fails cleanly */
    SkillDB bad;
    CHECK(!skilldb_load(&bad, SA_DATA_DIR "/does_not_exist.json"));
    TEST_DONE();
}
