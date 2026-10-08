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

## Controls (Milestone 1 demo)
Enter / click / tap: start. WASD or arrows: move. **F3 or a three-finger tap**: debug overlay (G god mode, K quest skip).

## Hosting
Plain static files; no COOP/COEP headers or SharedArrayBuffer needed. `dist/` contains `.br`/`.gz` precompressed siblings.
- Serve `.wasm` as `application/wasm`.
- Cloudflare Pages / Netlify: they compress automatically; the precompressed files are optional.
- nginx: `brotli_static on; gzip_static on;`. GitHub Pages: deploy works via the CI workflow (compression is automatic).
- The service worker (offline cache) is only registered on non-localhost origins (use `?sw=1` to test locally).

## Layout
`src/core` engine-agnostic logic (ECS, game loop, math, log): no raylib, no browser calls, unit-tested natively.
`src/platform` platform interface; the only browser-specific C is `src/platform/web`. `web/` holds the HTML shell and JS glue. Other `src/*` dirs are reserved for later milestones.
