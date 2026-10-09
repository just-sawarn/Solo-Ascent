// PWA / streaming smoke test.
//   node smoke_pwa.mjs <build-dir>
// Checks: boot packs mounted, on-demand pack streaming from C, service worker precache,
// offline reload reaches the title screen, and a corrupt pack fails loudly (no silent half-boot).
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

try {
  // 1. online: boot packs, streaming from C, SW precache
  const ctx = await browser.newContext({ viewport: { width: 960, height: 540 } });
  const page = await ctx.newPage();
  const logs = [];
  page.on('console', (m) => logs.push(m.text()));
  await page.goto(base + '?sw=1&packtest=1&tier=low&still=1');
  await page.waitForFunction(() => window.__sa && window.__sa.state === 'title', null, { timeout: 90000 });
  const packs = await page.evaluate(() => window.__sa.packs);
  check(packs && packs.characters === 2 && packs.animations === 2 && packs.weapons === 2, 'boot packs mounted before the game started');
  await page.waitForFunction(() => window.__sa.packs.selftest === 2, null, { timeout: 15000 }).catch(() => {});
  check(logs.some((l) => l.includes('packtest ok')), 'C-requested pack streamed into the virtual FS');
  await page.waitForFunction(() => navigator.serviceWorker.controller, null, { timeout: 30000 }).catch(() => {});
  const cached = await page.evaluate(async () => {
    const names = await caches.keys();
    const packCache = await caches.open('sa-packs');
    return { names, packs: (await packCache.keys()).map((r) => r.url.split('/').pop()) };
  });
  check(cached.names.some((n) => n.startsWith('sa-core-')), 'service worker installed a versioned core cache');
  check(['characters', 'animations', 'weapons'].every((n) => cached.packs.some((f) => f.startsWith(n + '.'))), 'boot packs precached for offline play');

  // 2. offline reload still boots (served from the service worker caches)
  await ctx.setOffline(true);
  const off = await ctx.newPage();
  await off.goto(base + '?sw=1&tier=low&still=1');
  await off.waitForFunction(() => window.__sa && window.__sa.state === 'title', null, { timeout: 90000 }).then(
    () => check(true, 'offline reload reaches the title screen'), () => check(false, 'offline reload reaches the title screen'));
  await ctx.setOffline(false);
  await ctx.close();

  // 3. corrupt pack: the game must refuse to start and say why
  const ctx2 = await browser.newContext({ viewport: { width: 960, height: 540 } });
  const bad = await ctx2.newPage();
  await bad.route('**/packs/weapons.*.pak', (r) => r.fulfill({ status: 200, contentType: 'application/octet-stream', body: Buffer.from('garbage') }));
  await bad.goto(base + '?tier=low');
  await bad.waitForFunction(() => window.__sa && window.__sa.state === 'load-failed', null, { timeout: 30000 }).then(
    () => check(true, 'corrupt pack fails loudly after retries'), () => check(false, 'corrupt pack fails loudly after retries'));
  const status = await bad.evaluate(() => document.getElementById('status').textContent);
  check(/failed/i.test(status), `status tells the player (“${status}”)`);
  await ctx2.close();
} finally {
  await browser.close();
  server.close();
}
if (failures.length) { console.log(`\n${failures.length} check(s) failed`); process.exit(1); }
console.log('\npwa smoke test passed');
