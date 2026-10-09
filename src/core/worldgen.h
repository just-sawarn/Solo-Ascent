#ifndef SA_WORLDGEN_H
#define SA_WORLDGEN_H
/*
 * World = 5 regions (biomes) x 44 zones (one per map location, minus Aurelheim's sub-places).
 * A zone is generated deterministically from its id: terrain + props + buildings (hub/field) or a dungeon.
 * No engine dependencies: rendering and physics are built from the result by the game.
 */
#include <stdbool.h>
#include <stdint.h>
#include "core/daynight.h"
#include "core/data.h"
#include "core/dungeon.h"
#include "core/physics.h"
#include "core/terrain.h"

typedef enum { PROP_TREE, PROP_PINE, PROP_BUSH, PROP_ROCK, PROP_CACTUS, PROP_DEADTREE, PROP_ICESPIKE, PROP_CRYSTAL, PROP_LAVAROCK, PROP_PILLAR, PROP_BONES, PROP_LAMP, PROP_COUNT } PropKind;
typedef enum { ZG_HUB, ZG_FIELD, ZG_DUNGEON } ZoneGen;

#define WG_MAX_REGIONS 8
#define WG_MAX_ZONES 64
#define WG_MAX_PROPS 640
#define WG_MAX_EXITS 8
#define WG_MAX_FOES 40
#define WG_MAX_BLD 16

typedef struct {
    char id[24], name[32];
    int tier;
    TerrainParams terrain;
    uint8_t ground_low[3], ground_high[3], ground_cliff[3], ground_accent[3];
    BiomeTint tint;
    float weather[WX_COUNT];
    struct { PropKind kind; float density; } props[8]; int nprops;   /* density per 1000 m^2 */
    struct { char enemy[32]; float weight; int def; } enemies[12]; int nenemies;
    uint8_t dng_wall[3], dng_floor[3], dng_torch[3];
} RegionDef;

typedef struct { char enemy[32]; char quest[16]; char at[24]; int def; } ZoneBoss;
typedef struct {
    char id[24];
    int region;
    ZoneGen gen;
    float map[2];
    ZoneBoss bosses[4]; int nbosses;
    int exit_to[WG_MAX_EXITS]; int nexits;   /* neighbouring zone indices (portals) */
} ZoneDef;

typedef struct {
    RegionDef region[WG_MAX_REGIONS]; int nregion;
    ZoneDef zone[WG_MAX_ZONES]; int nzone;
} WorldDB;

/* `dir` holds regions.json + zones.json. Unknown enemy ids / regions fail the load. */
bool worlddb_load(WorldDB *db, const char *dir, const EnemyDB *edb);
int  worlddb_zone(const WorldDB *db, const char *id);       /* -1 if unknown */
int  worlddb_region(const WorldDB *db, const char *id);

typedef struct { PropKind kind; float x, y, z, yaw, scale; } Prop;
typedef struct { float x0, z0, x1, z1, h; uint8_t col[3]; } Building;
typedef struct { float x, z; int dest; } ZoneExit;
typedef struct { float x, z; } ZonePt;

typedef struct {
    int zone, region;
    ZoneGen gen;
    uint32_t seed;
    Terrain terrain;
    Prop props[WG_MAX_PROPS]; int nprops;
    Building bld[WG_MAX_BLD]; int nbld;
    ZoneExit exits[WG_MAX_EXITS]; int nexits;
    Vec3 center;              /* hub plaza / field arena / dungeon boss room */
    Vec3 spawn;               /* default arrival point */
    Vec3 boss_pos;
    ZonePt foes[WG_MAX_FOES]; int nfoes;
    Dungeon dg;
    float bound;              /* playable half-extent */
    uint32_t checksum;        /* of everything above, for determinism tests */
} Zone;

void zone_generate(Zone *z, const WorldDB *db, int zone_index);
/* arrival point when coming from zone `from` (or the default spawn) */
Vec3 zone_entry(const Zone *z, int from);
float zone_height(const Zone *z, float x, float z_);
/* fills physics: terrain callback or flat floor, solid props, buildings, dungeon walls, bounds */
void zone_build_physics(const Zone *z, PhysWorld *w);
/* weighted random enemy def for a zone's region */
int  zone_pick_enemy(const WorldDB *db, int region, Rng *rng);

#endif
