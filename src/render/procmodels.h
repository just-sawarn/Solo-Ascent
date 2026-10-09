#ifndef SA_PROCMODELS_H
#define SA_PROCMODELS_H
/* Procedural placeholder art: primitive meshes and an anime-proportioned character builder.
 * Real CC0 glTF characters replace these in the asset-pipeline milestone; the renderer API stays the same. */
#include <raylib.h>
#include "render/renderer.h"

typedef enum { OUTFIT_HERO, OUTFIT_TANK, OUTFIT_MAGE, OUTFIT_HEALER, OUTFIT_SHADOW } Outfit;
typedef enum { HAIR_SPIKY, HAIR_SHORT, HAIR_LONG, HAIR_TWINTAIL, HAIR_NONE } HairStyle;

typedef struct {
    Outfit outfit;
    HairStyle hair_style;
    Color skin, hair, coat, trim, pants, boots, eye;
    float height; /* world units, ~1.8 for adults */
} CharDesc;

typedef enum { WEAPON_DAGGER, WEAPON_SHORT_SWORD, WEAPON_LONG_SWORD, WEAPON_KATANA, WEAPON_AXE, WEAPON_CLUB, WEAPON_SPEAR, WEAPON_COUNT } WeaponId;

void proc_init(void);
void proc_shutdown(void);
CharDesc proc_preset(Outfit o);

/* Queue a character for rendering. t = seconds for idle animation. */
void proc_submit_character(const CharDesc *c, Vector3 pos, float yaw, float t);
/* CC0 weapon models (assets/models/weapons, see ATTRIBUTIONS.md). `grip_to_tip` maps weapon space
 * (grip at origin, +Y toward the tip, metres) into the world; length_m is the visual blade+handle length. */
void proc_submit_weapon(WeaponId id, Matrix grip_to_tip, float length_m);
typedef enum { PM_SPHERE, PM_CYL, PM_CONE, PM_CUBE, PM_TORUS, PM_PLANE } ProcMesh;
const Mesh *proc_mesh(ProcMesh m);
/* Matrix mapping a unit cylinder/cone (base at origin, +Y axis, radius 1) onto the segment a->b with radii rx, rz. */
Matrix proc_limb(Vector3 a, Vector3 b, float rx, float rz);
/* Scene props */
void proc_submit_pillar(Vector3 pos, float h, float r, Color c);
void proc_submit_crystal(Vector3 pos, float h, Color c, float t);
void proc_submit_ground(float size, Color c);
/* Lock-on marker: a spinning gold diamond. */
void proc_submit_marker(Vector3 pos, float t);
void proc_submit_box(Vector3 mn, Vector3 mx, Color c);
void proc_submit_arch(Vector3 pos, float yaw, Color c);

#endif
