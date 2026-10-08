#ifndef SA_TEST_SCENE_H
#define SA_TEST_SCENE_H
#include <stdbool.h>
#include <raylib.h>
#include "core/physics.h"
/* Renderer showcase: the party, shadow soldiers, ruins and a crystal. Queues draws only. */
/* Only the hero on the ground plane (character design review). */
void test_scene_set_showcase(bool on);
void test_scene_set_rack(bool on); /* weapon model review row (with showcase) */
/* Colliders matching the visible props. */
void test_scene_build_physics(PhysWorld *w);
void test_scene_submit(float t, Vector3 hero_pos, float hero_yaw);
#endif
