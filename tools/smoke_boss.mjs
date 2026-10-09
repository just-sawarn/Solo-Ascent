// Boss smoke test: Vorathis from first phase to Quartet Strike, then the ending screen.
//   node smoke_boss.mjs <build-dir>
import { chromium } from 'playwright';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startServer } from './dev_server.mjs';

const dir = process.argv[2] || '../build-web-debug';
const here = path.dirname(fileURLToPath(import.meta.url));
const server = await startServer(path.resolve(here, dir), 0, { quiet: true });
const base = `http://localhost:${server.address().port}/index.html`;
const browser = await chromium.launch({
  args: ['--use-angle=swiftshader', '--use-gl=angle', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--enable-webgl'],
});
const failures = [];
const check = (ok, msg) => { console.log(`${ok ? 'PASS' : 'FAIL'}  ${msg}`); if (!ok) failures.push(msg); };
const tap = async (page, key, ms = 900) => { await page.keyboard.down(key); await page.waitForTimeout(ms); await page.keyboard.up(key); await page.waitForTimeout(400); };
const boss = (page) => page.evaluate(() => window.__sa.boss);
async function until(page, pred, ms, step = 400) {
  for (let t = 0; t < ms; t += step) { if (await page.evaluate(pred)) return true; await page.waitForTimeout(step); }
  return false;
}

try {
  const page = await browser.newPage({ viewport: { width: 960, height: 540 } });
  const errors = [];
  page.on('console', (m) => { if (m.type() === 'error') errors.push(m.text()); });
  page.on('pageerror', (e) => errors.push('pageerror: ' + e.message));
  await page.goto(`${base}?tier=low&still=1&zone=voidspire_throne&boss=vorathis&hour=1`);
  await page.waitForFunction(() => window.__sa && window.__sa.state === 'title', null, { timeout: 90000 });
  await page.click('#start'); await page.waitForTimeout(400);
  await page.keyboard.down('Enter'); await page.waitForTimeout(1500); await page.keyboard.up('Enter');
  await page.waitForFunction(() => window.__sa.state === 'playing', null, { timeout: 15000 });
  await page.waitForTimeout(2500);

  let b = await boss(page);
  check(b.active && b.phase <= 1, `Vorathis fight starts (phase ${b.phase + 1})`);
  await tap(page, 'F3'); await tap(page, 'g');                      // overlay on, god mode on (we test mechanics, not survival)
  for (let i = 0; i < 6 && (await boss(page)).phase < 2; ++i) await tap(page, 'u');   // -18% per press (dev)
  b = await boss(page);
  check(b.phase === 2 && b.solo, 'phase 3 is the Solo Ascent (Kael alone)');
  check(await page.evaluate(() => window.__sa.party.active) === 0, 'control is locked to Kael');
  await tap(page, 'r');
  check(await page.evaluate(() => window.__sa.party.active) === 0, 'R cannot switch away during the Solo Ascent');
  check(await until(page, () => window.__sa.boss.callout, 60000), 'Call Out becomes available after the king bears down');
  let called = 0;
  for (let i = 0; i < 3; ++i) {
    for (let k = 0; k < 4; ++k) { await tap(page, 'h', 300); b = await boss(page); if (b.called > called) break; await page.waitForTimeout(1200); }
    called = b.called;
    // the first call plays "Kael asks for help": skip any cutscene
    for (let k = 0; k < 4 && (await page.evaluate(() => window.__sa.story.dialogue)); ++k) { await tap(page, 'Escape', 600); }
  }
  check(called === 3, `all three allies answered (${called})`);
  await page.waitForTimeout(1500);
  b = await boss(page);
  check(b.phase === 3 && !b.solo, 'Bond Resonance phase begins');
  for (let i = 0; i < 6 && !(await boss(page)).finisher; ++i) await tap(page, 'u');
  check((await boss(page)).finisher, 'the Quartet Strike becomes available');
  await tap(page, 'f', 700);
  for (let k = 0; k < 6 && (await page.evaluate(() => window.__sa.story.dialogue)); ++k) await tap(page, 'Escape', 600);   // the Quartet Strike cutscene
  check(await until(page, () => window.__sa.boss.defeated || !window.__sa.boss.active, 45000), 'the Quartet Strike defeats Vorathis');
  for (let k = 0; k < 5 && (await page.evaluate(() => window.__sa.story.dialogue)); ++k) await tap(page, 'Escape', 600);

  // ending screen
  await tap(page, ';');
  check(await until(page, () => window.__sa.state === 'ending', 20000), 'game_cleared opens the ending screen');
  await page.screenshot({ path: '/tmp/ending.png' }).catch(() => {});
  await page.waitForTimeout(4000);
  await tap(page, 'Enter', 800);
  check(await until(page, () => window.__sa.state === 'playing' && window.__sa.ngplus === 1, 15000), 'Enter starts New Game+');
  check(errors.length === 0, `no console errors (${errors.slice(0, 2).join(' | ')})`);
  await page.close();
} finally {
  await browser.close();
  server.close();
}
if (failures.length) { console.log(`\n${failures.length} check(s) failed`); process.exit(1); }
console.log('\nboss smoke test passed');
