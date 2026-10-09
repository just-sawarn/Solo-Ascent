// World smoke test: generated zones, dungeons, day/night, weather, portal travel.
//   node smoke_world.mjs <build-dir>
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
const WX = { clear: 0, cloudy: 1, rain: 2, storm: 3, snow: 4, sandstorm: 5, ash: 6, fog: 7 };

async function boot(query) {
  const page = await browser.newPage({ viewport: { width: 960, height: 540 } });
  const errors = [];
  page.on('console', (m) => { if (m.type() === 'error') errors.push(m.text()); });
  page.on('pageerror', (e) => errors.push('pageerror: ' + e.message));
  await page.goto(`${base}?tier=low&still=1&${query}`);
  await page.waitForFunction(() => window.__sa && window.__sa.state === 'title', null, { timeout: 90000 });
  await page.click('#start'); await page.waitForTimeout(400);
  await page.keyboard.down('Enter'); await page.waitForTimeout(1500); await page.keyboard.up('Enter');
  await page.waitForFunction(() => window.__sa.state === 'playing', null, { timeout: 15000 });
  await page.waitForTimeout(2500);
  return { page, errors };
}
const world = (page) => page.evaluate(() => window.__sa.world);
const tap = async (page, key) => { await page.keyboard.down(key); await page.waitForTimeout(900); await page.keyboard.up(key); await page.waitForTimeout(500); };

try {
  // hub in a different region, at night, in the rain
  let { page, errors } = await boot('zone=whisperwood&hour=22&weather=rain');
  let w = await world(page);
  check(w.zone === 'whisperwood' && w.gen === 0, `hub zone generated (${w.zone})`);
  check(w.props > 20, `props drawn (${w.props})`);
  check(w.weather === WX.rain && w.intensity > 0.9, 'weather forced to rain');
  check(w.particles > 40, `rain particles (${w.particles})`);
  check(w.hour > 21.5 && w.hour < 23, `time of day set (${w.hour.toFixed(1)})`);
  check(w.enemies === 0, 'towns are safe (no enemies)');
  // dev keys: T advances the clock, Y cycles weather (overlay must be on)
  await tap(page, 'F3');
  await tap(page, 't');
  const w2 = await world(page);
  check(Math.abs(((w2.hour - w.hour + 24) % 24) - 3.0) < 0.8, `T advances the clock by 3 h (${w.hour.toFixed(1)} -> ${w2.hour.toFixed(1)})`);
  await tap(page, 'y');
  check((await world(page)).weather === (WX.rain + 1) % 8, 'Y cycles the weather');
  // portal travel: P (dev) puts the party at the first portal, which moves them to the neighbouring zone
  await tap(page, 'p');
  await page.waitForTimeout(2500);
  let w3 = await world(page);
  check(w3.zone !== 'whisperwood', `stepping into a portal travels (${w3.zone})`);
  await page.waitForTimeout(1500);
  const pl = await page.evaluate(() => window.__sa.player);
  check(pl.grounded && Math.abs(pl.x) < 80 && Math.abs(pl.z) < 80, 'arrival point is inside the new zone and on the ground');
  check(errors.length === 0, `no console errors (${errors.slice(0, 2).join(' | ')})`);
  await page.close();

  // field: enemies populate, terrain props drawn
  ({ page, errors } = await boot('zone=ashbrook_outskirts&hour=15'));
  w = await world(page);
  check(w.gen === 1 && w.enemies >= 5, `field has enemy packs (${w.enemies})`);
  check(w.props > 40, `field props (${w.props})`);
  check(errors.length === 0, 'field: no console errors');
  await page.close();

  // dungeon: generated rooms full of enemies, indoor lighting, no weather
  ({ page, errors } = await boot('zone=goblin_warrens&hour=12&weather=storm'));
  w = await world(page);
  check(w.gen === 2 && w.enemies >= 8, `dungeon is populated (${w.enemies} enemies)`);
  const dpl = await page.evaluate(() => window.__sa.player);
  check(dpl.grounded, 'party stands on the dungeon floor');
  check(errors.length === 0, `dungeon: no console errors (${errors.slice(0, 2).join(' | ')})`);
  await page.close();

  // every region boots (biome data, palettes, weather tables)
  for (const z of ['qasr_amun', 'hrimvald', 'emberforge', 'front_camp']) {
    ({ page, errors } = await boot(`zone=${z}&hour=8`));
    w = await world(page);
    check(w.zone === z && errors.length === 0, `${z} loads (region ${w.region})`);
    await page.close();
  }
} finally {
  await browser.close();
  server.close();
}
if (failures.length) { console.log(`\n${failures.length} check(s) failed`); process.exit(1); }
console.log('\nworld smoke test passed');
