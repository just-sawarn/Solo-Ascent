#ifndef SA_TEST_SCENE_H
#define SA_TEST_SCENE_H
#include <stdbool.h>
#include <raylib.h>
#include "core/physics.h"
/* The sandbox scene: ground, ruins, stairs, crate, crystal. Queues draws only (characters are submitted by the game). */
/* Only the hero on the ground plane (character design review). */
void test_scene_set_showcase(bool on);
bool test_scene_showcase(void);
void test_scene_set_rack(bool on); /* weapon model review row (with showcase) */
/* Colliders matching the visible props. */
void test_scene_build_physics(PhysWorld *w);
void test_scene_submit(float t);
#endif
