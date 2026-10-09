#ifndef SA_DUNGEON_H
#define SA_DUNGEON_H
/* Seeded room-and-corridor dungeon on a square grid. Always connected: every room is joined to the
 * start room, the boss room is the farthest by walking distance. Pure data; physics boxes are derived. */
#include <stdbool.h>
#include <stdint.h>
#include "core/vec.h"

#define DG_N 36
#define DG_CELL 3.0f
#define DG_MAX_ROOMS 16
#define DG_MAX_SPAWNS 48

typedef enum { DG_WALL = 0, DG_FLOOR = 1, DG_START = 2, DG_BOSS = 3 } DgCell;
typedef struct { int x, z, w, h; } DgRoom;
typedef struct { float x, z; int room; } DgSpawn;
typedef struct { float x, z; } DgPoint;

typedef struct {
    uint8_t cell[DG_N][DG_N];
    DgRoom rooms[DG_MAX_ROOMS];
    int nrooms, start_room, boss_room;
    DgSpawn spawns[DG_MAX_SPAWNS]; int nspawns;
    DgPoint chests[8]; int nchests;
    DgPoint torches[48]; int ntorches;
    int floor_cells;
} Dungeon;

void dungeon_generate(Dungeon *d, uint32_t seed);
/* world <-> grid (grid centred on the origin) */
Vec3  dungeon_cell_center(int cx, int cz);
bool  dungeon_is_floor_at(const Dungeon *d, float x, float z);
/* number of floor cells reachable from the start room (== floor_cells when connected) */
int   dungeon_reachable(const Dungeon *d);
/* merged wall boxes for physics/rendering: min/max in world space (height `wall_h`). Returns count written. */
typedef struct { float x0, z0, x1, z1; } DgWallRect;
int   dungeon_wall_rects(const Dungeon *d, DgWallRect *out, int cap);

#endif
