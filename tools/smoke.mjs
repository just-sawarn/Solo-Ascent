// Playwright smoke test + frame-time benchmark.
//   node smoke.mjs <build-dir> [--update-baseline]
// Checks: page loads, WebGL2 initialises, title screen reached, new game starts,
// no console errors, and average frame time has not regressed vs bench_baseline.json.
import { chromium } from 'playwright';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startServer } from './dev_server.mjs';

const dir = process.argv[2] || '../build-web-debug';
const updateBaseline = process.argv.includes('--update-baseline');
const here = path.dirname(fileURLToPath(import.meta.url));
const baselineFile = path.join(here, 'bench_baseline.json');
const REGRESSION_FACTOR = 2.0;   // fail if avg frame time > baseline * factor
const ABS_FLOOR_MS = 40;         // ...and also above this, so noise on tiny numbers can't fail CI

const server = await startServer(path.resolve(here, dir), 0, { quiet: true });
const url = `http://localhost:${server.address().port}/index.html`;
const browser = await chromium.launch({
  args: ['--use-angle=swiftshader', '--use-gl=angle', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--enable-webgl'],
});
const failures = [];
const check = (ok, msg) => { console.log(`${ok ? 'PASS' : 'FAIL'}  ${msg}`); if (!ok) failures.push(msg); };

try {
  const page = await browser.newPage({ viewport: { width: 1280, height: 720 } });
  const errors = [];
  page.on('console', (m) => { if (m.type() === 'error') errors.push(m.text()); });
  page.on('pageerror', (e) => errors.push('pageerror: ' + e.message));

  await page.goto(url);
  const gl2 = await page.evaluate(() => !!document.createElement('canvas').getContext('webgl2'));
  check(gl2, 'WebGL2 context available');

  await page.waitForFunction(() => window.__sa && window.__sa.loaded, null, { timeout: 60000 });
  check(true, 'wasm loaded (Module.postRun)');
  await page.waitForFunction(() => window.__sa.state === 'title', null, { timeout: 30000 });
  check(true, 'reached title screen');

  await page.click('#start');
  await page.waitForTimeout(500);
  // hold the key across several frames: raylib polls key state once per frame
  await page.keyboard.down('Enter'); await page.waitForTimeout(300); await page.keyboard.up('Enter');
  await page.waitForFunction(() => window.__sa.state === 'playing', null, { timeout: 10000 });
  check(true, 'new game started (state=playing)');

  // hold a movement key, toggle the debug overlay, then measure frame times
  await page.keyboard.down('F3'); await page.waitForTimeout(200); await page.keyboard.up('F3');
  await page.keyboard.down('d');
  const avgMs = await page.evaluate(() => new Promise((resolve) => {
    let last = performance.now(), n = 0, total = 0;
    const end = last + 4000;
    (function tick(now) {
      total += now - last; last = now; n++;
      if (now < end) requestAnimationFrame(tick); else resolve(total / n);
    })(last);
  }));
  await page.keyboard.up('d');
  console.log(`      avg frame time: ${avgMs.toFixed(2)} ms (software GL; relative only)`);

  // visibility hook reaches C without errors
  const hooked = await page.evaluate(() => window.__sa.hooked === true);
  check(hooked, 'lifecycle hooks registered from C');

  check(errors.length === 0, `no console errors${errors.length ? ': ' + errors.join(' | ') : ''}`);

  if (updateBaseline || !fs.existsSync(baselineFile)) {
    fs.writeFileSync(baselineFile, JSON.stringify({ avgFrameMs: avgMs, note: 'software GL on dev machine' }, null, 2));
    console.log('      baseline written');
  } else {
    const base = JSON.parse(fs.readFileSync(baselineFile, 'utf8')).avgFrameMs;
    check(!(avgMs > base * REGRESSION_FACTOR && avgMs > ABS_FLOOR_MS),
      `frame time within ${REGRESSION_FACTOR}x of baseline (${base.toFixed(2)} ms)`);
  }

  // unsupported-WebGL2 path: friendly page instead of a crash
  const ctx2 = await browser.newContext();
  const p2 = await ctx2.newPage();
  await p2.addInitScript(() => { Object.defineProperty(window, 'WebGL2RenderingContext', { value: undefined }); });
  await p2.goto(url);
  await p2.waitForFunction(() => window.__sa && window.__sa.state === 'unsupported', null, { timeout: 5000 })
    .then(() => check(true, 'friendly error page when WebGL2 is missing'))
    .catch(() => check(false, 'friendly error page when WebGL2 is missing'));
} catch (e) {
  check(false, 'smoke test threw: ' + e.message);
} finally {
  await browser.close();
  server.close();
}
if (failures.length) { console.error(`\n${failures.length} check(s) failed`); process.exit(1); }
console.log('\nsmoke test passed');
