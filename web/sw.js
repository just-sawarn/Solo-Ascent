/* Service worker. The build id is stamped in by tools/pack_assets.py, so each build installs a new worker.
 *  - core files: cached per build (cache name carries the build id), cache-first
 *  - packs/*.pak: content-addressed (hash in the file name) so cached forever in one shared cache;
 *    packs no longer listed in packs.json are pruned
 *  - page navigation and packs.json: network-first, cache fallback (offline play)
 * Installing precaches the core and every `boot` pack so the game starts offline after one visit. */
const BUILD = '__BUILD_ID__';
const CORE_CACHE = 'sa-core-' + BUILD;
const PACK_CACHE = 'sa-packs';
const CORE = ['./', 'index.html', 'index.js', 'index.wasm', 'index.data', 'glue.js', 'game.css', 'manifest.webmanifest', 'icon-192.png', 'icon-512.png', 'packs.json'];

async function precache() {
  const core = await caches.open(CORE_CACHE);
  await core.addAll(CORE.map((u) => new Request(u, { cache: 'reload' })));
  const man = await (await core.match('packs.json')).json();
  const packs = await caches.open(PACK_CACHE);
  for (const p of Object.values(man.packs)) {
    if (p.boot && !(await packs.match(p.file))) await packs.add(new Request(p.file, { cache: 'reload' }));
  }
}

async function prune() {
  for (const k of await caches.keys()) if (k.startsWith('sa-core-') && k !== CORE_CACHE) await caches.delete(k);
  try {
    const man = await (await (await caches.open(CORE_CACHE)).match('packs.json')).json();
    const keep = new Set(Object.values(man.packs).map((p) => new URL(p.file, self.registration.scope).href));
    const packs = await caches.open(PACK_CACHE);
    for (const r of await packs.keys()) if (!keep.has(r.url)) await packs.delete(r);
  } catch (e) { /* no manifest cached yet: keep everything */ }
}

self.addEventListener('install', (e) => e.waitUntil(precache().then(() => self.skipWaiting())));
self.addEventListener('activate', (e) => e.waitUntil(prune().then(() => self.clients.claim())));

self.addEventListener('message', (e) => {
  if (e.data === 'build') e.source.postMessage({ build: BUILD });
});

async function networkFirst(req, cacheName) {
  const cache = await caches.open(cacheName);
  try {
    const r = await fetch(req);
    if (r.ok) cache.put(req, r.clone());
    return r;
  } catch (err) {
    return (await cache.match(req)) || (req.mode === 'navigate' ? await cache.match('index.html') : Response.error());
  }
}

self.addEventListener('fetch', (e) => {
  const req = e.request;
  const url = new URL(req.url);
  if (req.method !== 'GET' || url.origin !== location.origin) return;
  if (url.pathname.includes('/packs/')) {
    e.respondWith(caches.open(PACK_CACHE).then(async (c) => (await c.match(req)) || fetch(req).then((r) => { if (r.ok) c.put(req, r.clone()); return r; })));
  } else if (req.mode === 'navigate' || url.pathname.endsWith('/packs.json')) {
    e.respondWith(networkFirst(req, CORE_CACHE));
  } else {
    e.respondWith(caches.open(CORE_CACHE).then(async (c) => (await c.match(req)) || fetch(req).then((r) => { if (r.ok) c.put(req, r.clone()); return r; })));
  }
});
