#include "world/test_scene.h"
#include <math.h>
#include <raymath.h>
#include "render/heroes.h"
#include "render/procmodels.h"

typedef struct { Vector3 mn, mx; Color col; } Box;
static const Box BOXES[] = {
    /* stairs up to a platform (0.22 m risers), and a 1.2 m crate for jump tests */
    {{4.0f, 0, 2.0f}, {5.2f, 0.22f, 4.0f}, {96, 100, 128, 255}},
    {{5.2f, 0, 2.0f}, {6.4f, 0.44f, 4.0f}, {96, 100, 128, 255}},
    {{6.4f, 0, 2.0f}, {7.6f, 0.66f, 4.0f}, {96, 100, 128, 255}},
    {{7.6f, 0, 1.5f}, {10.6f, 0.66f, 5.0f}, {84, 88, 116, 255}},
    {{-4.6f, 0, 2.0f}, {-3.4f, 1.2f, 3.2f}, {110, 84, 70, 255}},
    /* arch posts */
    {{-2.65f, 0, -9.95f}, {-1.75f, 5.0f, -9.05f}, {70, 72, 96, 255}},
    {{1.75f, 0, -9.95f}, {2.65f, 3.4f, -9.05f}, {70, 72, 96, 255}},
};
static const struct { float x, z, h, r; } PILLARS[] = {
    {-6.0f, -5.0f, 4.5f, 0.55f}, {6.5f, -6.0f, 3.2f, 0.55f}, {-9.0f, 2.0f, 5.5f, 0.6f}, {9.5f, 3.0f, 2.4f, 0.6f},
};

static Color C(int r, int g, int b) { return (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, 255}; }

static bool g_showcase, g_rack;
void test_scene_set_rack(bool on) { g_rack = on; }
void test_scene_set_showcase(bool on) { g_showcase = on; }

void test_scene_submit(float t, Vector3 hero, float hero_yaw) {
    static CharDesc bram, lyra, mirelle, shadow;
    static bool init;
    if (!init) {
        bram = proc_preset(OUTFIT_TANK); lyra = proc_preset(OUTFIT_MAGE);
        mirelle = proc_preset(OUTFIT_HEALER); shadow = proc_preset(OUTFIT_SHADOW);
        init = true;
    }
    proc_submit_ground(120.0f, C(54, 56, 76));

    /* the party, in a loose arc around the hero's start; the hero is driven by the ECS player */
    heroes_submit_kael(hero, hero_yaw, t);
    if (g_showcase) {
        if (g_rack) { /* weapon review: one of each, tips up, 0.9 m long */
            for (int i = 0; i < WEAPON_COUNT; ++i)
                proc_submit_weapon((WeaponId)i, MatrixTranslate(-3.0f + 1.0f * (float)i, 0.1f, 1.5f), 1.3f);
        }
        return;
    }
    proc_submit_character(&bram, (Vector3){-1.7f, 0, -0.5f}, 0.35f, t + 0.7f);
    proc_submit_character(&lyra, (Vector3){1.6f, 0, -0.35f}, -0.40f, t + 1.4f);
    proc_submit_character(&mirelle, (Vector3){0.0f, 0, -1.8f}, 0.0f, t + 2.1f);

    /* shadow soldiers in the distance */
    for (int i = 0; i < 5; ++i) {
        float a = 3.14159f + ((float)i - 2.0f) * 0.42f;
        proc_submit_character(&shadow, (Vector3){sinf(a) * 8.5f, 0, cosf(a) * 8.5f - 1.0f}, a + 3.14159f, t + (float)i);
    }

    /* ruins + crystal */
    proc_submit_box((Vector3){-2.65f, 4.9f, -10.05f}, (Vector3){2.65f, 5.7f, -8.95f}, C(70, 72, 96)); /* lintel */
    proc_submit_crystal((Vector3){0, 0.2f, -9.5f}, 2.4f, C(150, 110, 255), t);
    for (unsigned i = 0; i < sizeof PILLARS / sizeof PILLARS[0]; ++i)
        proc_submit_pillar((Vector3){PILLARS[i].x, 0, PILLARS[i].z}, PILLARS[i].h, PILLARS[i].r, C(78, 80, 104));
    for (unsigned i = 0; i < sizeof BOXES / sizeof BOXES[0]; ++i) proc_submit_box(BOXES[i].mn, BOXES[i].mx, BOXES[i].col);
}

void test_scene_build_physics(PhysWorld *w) {
    phys_init(w, 0.0f);
    for (unsigned i = 0; i < sizeof BOXES / sizeof BOXES[0]; ++i)
        phys_add_box(w, vec3(BOXES[i].mn.x, BOXES[i].mn.y, BOXES[i].mn.z), vec3(BOXES[i].mx.x, BOXES[i].mx.y, BOXES[i].mx.z));
    phys_add_box(w, vec3(-2.65f, 4.9f, -10.05f), vec3(2.65f, 5.7f, -8.95f)); /* lintel */
    for (unsigned i = 0; i < sizeof PILLARS / sizeof PILLARS[0]; ++i)
        phys_add_cyl(w, PILLARS[i].x, PILLARS[i].z, PILLARS[i].r * 1.25f, 0.0f, PILLARS[i].h + 0.5f);
}

int test_scene_targets(Vector3 *out, int max) {
    int n = 0;
    for (int i = 0; i < 5 && n < max; ++i) {
        float a = 3.14159f + ((float)i - 2.0f) * 0.42f;
        out[n++] = (Vector3){sinf(a) * 8.5f, 0, cosf(a) * 8.5f - 1.0f};
    }
    return n;
}
