# Progress

## Milestone 1: Project skeleton: DONE
Delivered: CMake + Emscripten + raylib 5.5 (ES3/WebGL2), HTML shell with loading bar and "Click / Tap to Start" (audio unlock, fullscreen, landscape lock), WebGL2 detection with friendly error page, ECS, fixed-step loop with interpolation, debug overlay (F3 / 3-finger tap; FPS, frame graph, draws, ticks, heap, god mode, quest skip flags), native tests (ecs, gameloop; ASan/UBSan), dev server (MIME, br/gz, live reload), Playwright smoke test + frame-time regression check, debug / debug-san / release flavors (release uses closure + LTO + wasm-opt), PWA manifest + service worker, GitHub Actions workflow, `scripts/package.sh` (dist + brotli).

Verified here: native tests pass; web debug, sanitizer and release builds pass the smoke test in headless Chromium (software GL) and `dist/` passes too. Release wasm ≈118 KB (45 KB brotli) before any game assets.

NOT verified: real GPU / phone performance (headless numbers are software-GL and relative only); touch input (3-finger tap, fullscreen/orientation lock need a real device); service worker offline behavior; CI workflow has not run on GitHub. Test on a phone: `scripts/package.sh`, serve `dist/` over HTTPS (e.g. Cloudflare Pages), open in Android Chrome, tap Start, three-finger tap for the overlay.

Limits: the ASan build instruments only our code, not raylib. Cross-origin isolation is deliberately not used.

## Next: Milestone 2: Renderer (toon shader, shadows, outlines, post, quality tiers, warm-up).
