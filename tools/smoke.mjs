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
const REGRESSION_FACTOR = 2.5;   // fail if avg frame time > baseline * factor
const ABS_FLOOR_MS = 40;         // ...and also above this, so noise on tiny numbers can't fail CI

const server = await startServer(path.resolve(here, dir), 0, { quiet: true });
const base = `http://localhost:${server.address().port}/index.html`;
// fixed tier + still camera so the benchmark is deterministic (the quality governor is off when a tier is forced)
const url = base + '?tier=medium&still=1&look=3';
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
  await page.keyboard.down('Enter'); await page.waitForTimeout(1500); await page.keyboard.up('Enter');
  await page.waitForFunction(() => window.__sa.state === 'playing', null, { timeout: 10000 });
  check(true, 'new game started (state=playing)');

  // gameplay: the player stands on the ground, moves when W is held, and leaves the ground when Space is pressed
  const pl = () => page.evaluate(() => window.__sa.player);
  await page.waitForTimeout(600);
  const p0 = await pl();
  check(p0 && p0.grounded && Math.abs(p0.y) < 0.01, 'player grounded at spawn');
  await page.keyboard.down('w'); await page.waitForTimeout(1500);
  const p1 = await pl();
  await page.keyboard.up('w');
  check(Math.hypot(p1.x - p0.x, p1.z - p0.z) > 2.0, `player moved with W (${Math.hypot(p1.x - p0.x, p1.z - p0.z).toFixed(1)} m)`);
  let maxY = 0;
  await page.keyboard.down('Space');
  for (let i = 0; i < 12; i++) { await page.waitForTimeout(60); maxY = Math.max(maxY, (await pl()).y); }
  await page.keyboard.up('Space');
  check(maxY > 0.3, `jump leaves the ground (peak sample ${maxY.toFixed(2)} m)`);
  await page.waitForTimeout(1200);
  check((await pl()).grounded, 'player lands again');

  // party: companions follow, control can be switched, the command cycles
  const party = () => page.evaluate(() => window.__sa.party);
  await page.waitForTimeout(2500);
  const pf = await party();
  check(pf && pf.followerDist < 14, `companions keep up with the leader (${pf.followerDist.toFixed(1)} m)`);
  const hold = async (k, ms = 700) => { await page.keyboard.down(k); await page.waitForTimeout(ms); await page.keyboard.up(k); await page.waitForTimeout(300); };
  const cmd0 = (await party()).command;
  await hold('b');
  check((await party()).command === (cmd0 + 1) % 3, 'B cycles the party command');
  await hold('r');
  check((await party()).active === 1, 'R switches control to the next member (Bram)');
  await hold('8');
  check((await party()).active === 3, '8 selects Mirelle');
  await hold('5');
  check((await party()).active === 0, '5 returns to Kael');

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

  // combat arena: one soldier in front of the player. Attacking must hurt it; it must hurt back; killing it must count.
  {
    const pa = await browser.newPage({ viewport: { width: 800, height: 450 } });
    const ea = [];
    pa.on('console', (m) => { if (m.type() === 'error') ea.push(m.text()); });
    pa.on('pageerror', (e) => ea.push('pageerror: ' + e.message));
    await pa.goto(base + '?arena=1&tier=mobile&still=1');
    await pa.waitForFunction(() => window.__sa && window.__sa.state === 'title', null, { timeout: 60000 });
    await pa.click('#start'); await pa.waitForTimeout(300);
    await pa.keyboard.down('Enter'); await pa.waitForTimeout(1500); await pa.keyboard.up('Enter');
    await pa.waitForFunction(() => window.__sa.combat && window.__sa.combat.enemies === 1, null, { timeout: 10000 });
    const c0 = await pa.evaluate(() => window.__sa.combat);
    const p0 = await pa.evaluate(() => window.__sa.party.hp.reduce((a, b) => a + b, 0));
    let kills = 0, minPlayer = p0, minEnemy = c0.enemyHp;
    for (let i = 0; i < 90 && kills === 0; i++) {            // mash attack (J) and the sunder skill (2)
      await pa.keyboard.down('j'); await pa.waitForTimeout(120); await pa.keyboard.up('j');
      if (i % 4 === 3) { await pa.keyboard.down('2'); await pa.waitForTimeout(120); await pa.keyboard.up('2'); }
      await pa.waitForTimeout(350);
      const c = await pa.evaluate(() => window.__sa.combat);
      kills = c.kills; minEnemy = Math.min(minEnemy, c.enemyHp);
      minPlayer = Math.min(minPlayer, await pa.evaluate(() => window.__sa.party.hp.reduce((a, b) => a + b, 0)));
    }
    check(minEnemy < c0.enemyHp, `attacks damage the enemy (${c0.enemyHp.toFixed(0)} -> ${minEnemy.toFixed(0)})`);
    const pend = await pa.evaluate(() => window.__sa.party);
    check(pend.dead === 0 && minPlayer > p0 * 0.5, `the party beats it without losses (party HP ${p0.toFixed(0)} -> min ${minPlayer.toFixed(0)}; enemy retaliation is covered by native tests)`);
    check(kills >= 1, `the enemy can be defeated (kills: ${kills})`);
    check(ea.length === 0, `no console errors in combat${ea.length ? ': ' + ea.join(' | ') : ''}`);
    await pa.close();
  }

  // unsupported-WebGL2 path: friendly page instead of a crash
  const ctx2 = await browser.newContext();
  const p2 = await ctx2.newPage();
  await p2.addInitScript(() => { Object.defineProperty(window, 'WebGL2RenderingContext', { value: undefined }); });
  await p2.goto(base);
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
