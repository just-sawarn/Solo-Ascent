# Solo Ascent: The S-Rank Hero

3D anime action RPG in C11 → WebAssembly (raylib, WebGL2). Story bible: [docs/STORY.md](docs/STORY.md).

## Prerequisites
cmake ≥ 3.20, Emscripten (`~/emsdk`, `emsdk install latest && emsdk activate latest`), node ≥ 20, brotli (for `package.sh`).

## Commands
| Task | Command |
|---|---|
| Native unit tests (ASan/UBSan) | `scripts/build.sh native-test` |
| Web debug build (assertions, source maps) | `scripts/build.sh debug` |
| Web debug + sanitizers | `scripts/build.sh debug-san` |
| Web release (-O3, LTO, closure, wasm-opt) | `scripts/build.sh release` |
| Dev server with live reload | `scripts/dev.sh` → http://localhost:8080 |
| Full test (native + browser smoke) | `scripts/test.sh` |
| Static-host bundle in `dist/` | `scripts/package.sh` |
| Native desktop build for quick iteration | `scripts/build.sh native-game` |

First time: `cd tools && npm install && npx playwright install chromium`.

## Controls
| Action | Keyboard / mouse | Gamepad | Touch |
|---|---|---|---|
| Move | WASD / arrows | left stick | floating joystick (left side); push to the edge to sprint |
| Look | mouse (click once to capture; Esc releases) | right stick | swipe the right side |
| Sprint / walk | Shift / Ctrl | RB / LB | joystick edge |
| Jump | Space | A | JUMP |
| Attack (3-hit combo) | J or left-click (press again during a swing to chain) | X | ATK |
| Skills 1-4 (Heroic Slash, Sunder, Gale Rush, Dawn Arc) | 1 2 3 4 | hold LT + X / Y / B / A | 1 2 3 |
| Guard (press just before a hit to parry) | E (hold) | RT (hold) | GUARD |
| Dodge roll (i-frames) | Q or right-click | B | ROLL |
| Lock on | Tab or middle-click | Y / R3 | LOCK |
| Debug overlay | F3 (or a three-finger tap); G god mode, K quest skip | | |
| Menu (items, gear, craft, smith, shop, journal, map) | I (items) / M (map) | | MENU |
| Talk to a nearby NPC / advance dialogue | F, Enter, click; 1-4 pick a reply; Esc skips cutscenes | dpad down | TALK (appears near an NPC) |
| Dev: skip quest / next time slot / next zone | K (overlay on) / N / F6 | | |
| Dev (overlay on): weather / +3 h / step into the first portal | Y / T / P | | |
| Travel between zones | walk into a glowing portal at the zone edge | | |
| Quick use: HP potion / MP potion / revive / cleanse | Z / X / C / V | | |
| Look / quality | L cycles the look, `[` `]` change quality tier | | |

## Test URLs
`index.html?zone=<location id>&hour=<0-24>&weather=<clear|cloudy|rain|storm|snow|sandstorm|ash|fog>` starts directly in a zone (e.g. `zone=goblin_warrens&hour=22`).

## Hosting
Plain static files; no COOP/COEP headers or SharedArrayBuffer needed. `dist/` contains `.br`/`.gz` precompressed siblings.
- Serve `.wasm` as `application/wasm`.
- Cloudflare Pages / Netlify: they compress automatically; the precompressed files are optional.
- nginx: `brotli_static on; gzip_static on;`. GitHub Pages: deploy works via the CI workflow (compression is automatic).
- The service worker (offline cache) is only registered on non-localhost origins (use `?sw=1` to test locally).
- Assets ship as content-addressed packs in `packs/*.pak` (hash in the file name) plus `packs.json`. Serve `packs/` with long-lived `Cache-Control: immutable`; keep `packs.json`, `index.html` and `sw.js` revalidating (`no-cache`).
- Download budget (brotli): ~0.45 MB wasm+js+data core, ~2.8 MB boot packs (characters 1.9, animations 0.8, weapons 0.03).

## Layout
`src/core` engine-agnostic logic (ECS, game loop, math, log): no raylib, no browser calls, unit-tested natively.
`src/platform` platform interface; the only browser-specific C is `src/platform/web`. `web/` holds the HTML shell and JS glue. Other `src/*` dirs are reserved for later milestones.
