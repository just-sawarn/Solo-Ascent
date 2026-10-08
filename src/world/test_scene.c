#include "world/test_scene.h"
#include <math.h>
#include "render/procmodels.h"

static Color C(int r, int g, int b) { return (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, 255}; }

static bool g_showcase;
void test_scene_set_showcase(bool on) { g_showcase = on; }

void test_scene_submit(float t, Vector3 hero, float hero_yaw) {
    static CharDesc kael, bram, lyra, mirelle, shadow;
    static bool init;
    if (!init) {
        kael = proc_preset(OUTFIT_HERO); bram = proc_preset(OUTFIT_TANK); lyra = proc_preset(OUTFIT_MAGE);
        mirelle = proc_preset(OUTFIT_HEALER); shadow = proc_preset(OUTFIT_SHADOW);
        init = true;
    }
    proc_submit_ground(120.0f, C(54, 56, 76));

    /* the party, in a loose arc around the hero's start; the hero is driven by the ECS player */
    proc_submit_character(&kael, hero, hero_yaw, t);
    if (g_showcase) return;
    proc_submit_character(&bram, (Vector3){-1.7f, 0, -0.5f}, 0.35f, t + 0.7f);
    proc_submit_character(&lyra, (Vector3){1.6f, 0, -0.35f}, -0.40f, t + 1.4f);
    proc_submit_character(&mirelle, (Vector3){0.0f, 0, -1.8f}, 0.0f, t + 2.1f);

    /* shadow soldiers in the distance */
    for (int i = 0; i < 5; ++i) {
        float a = 3.14159f + ((float)i - 2.0f) * 0.42f;
        proc_submit_character(&shadow, (Vector3){sinf(a) * 8.5f, 0, cosf(a) * 8.5f - 1.0f}, a + 3.14159f, t + (float)i);
    }

    /* ruins + crystal */
    proc_submit_arch((Vector3){0, 0, -9.5f}, 0.0f, C(70, 72, 96));
    proc_submit_crystal((Vector3){0, 0.2f, -9.5f}, 2.4f, C(150, 110, 255), t);
    proc_submit_pillar((Vector3){-6.0f, 0, -5.0f}, 4.5f, 0.55f, C(78, 80, 104));
    proc_submit_pillar((Vector3){6.5f, 0, -6.0f}, 3.2f, 0.55f, C(78, 80, 104));
    proc_submit_pillar((Vector3){-9.0f, 0, 2.0f}, 5.5f, 0.6f, C(78, 80, 104));
    proc_submit_pillar((Vector3){9.5f, 0, 3.0f}, 2.4f, 0.6f, C(78, 80, 104));
}
