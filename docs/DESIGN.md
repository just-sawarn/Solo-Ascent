# Design notes

## Architecture rules
- `src/core` has no raylib and no browser calls so it builds and tests natively. Rendering, UI and platform code depend on core, never the reverse.
- ECS (`core/ecs.h`): struct-of-arrays in a caller-owned static `World`, 4096 fixed slots, generational 32-bit handles (index 16 | generation 16; 0 is invalid). No allocation after init.
- Loop (`core/gameloop.h`): fixed 60 Hz accumulator, max 5 steps per frame (backlog dropped, counted), render interpolation via `prev_pos/prev_yaw` snapshots taken at the start of each step. Tab hidden pauses the accumulator.
- Browser glue (`web/glue.js`) does only: WebGL2 detection, loading UI, start gesture (audio unlock, fullscreen, landscape lock), gesture suppression, lifecycle events, PWA registration.
- Closure caveat: any JS property shared between C `EM_ASM` and `glue.js`/tests must use bracket access (`window['__sa']['state']`) or closure renames it in release.
- `web/externs.js` exists because raylib's own EM_ASM uses a bare `canvas` global.

## Known gaps (tracked in PROGRESS.md)
- WebGL context restore is signalled to C but GPU resources are not rebuilt yet (renderer milestone).
- "draw calls" in the overlay count draws submitted by game code, not the driver's actual count.
