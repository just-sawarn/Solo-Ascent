/* Service worker: cache-first for the versioned core build, network-first for the HTML page. */
const VERSION = 'sa-core-v1';
const CORE = ['./', 'index.html', 'index.js', 'index.wasm', 'index.data', 'glue.js', 'game.css', 'manifest.webmanifest', 'icon-192.png', 'icon-512.png'];

self.addEventListener('install', (e) => {
  e.waitUntil(caches.open(VERSION).then((c) => c.addAll(CORE)).then(() => self.skipWaiting()));
});
self.addEventListener('activate', (e) => {
  e.waitUntil(
    caches.keys().then((ks) => Promise.all(ks.filter((k) => k !== VERSION).map((k) => caches.delete(k))))
      .then(() => self.clients.claim()));
});
self.addEventListener('fetch', (e) => {
  const req = e.request;
  if (req.method !== 'GET' || new URL(req.url).origin !== location.origin) return;
  const isPage = req.mode === 'navigate';
  e.respondWith(
    isPage
      ? fetch(req).then((r) => { const cp = r.clone(); caches.open(VERSION).then((c) => c.put(req, cp)); return r; })
          .catch(() => caches.match(req).then((r) => r || caches.match('index.html')))
      : caches.match(req).then((hit) => hit || fetch(req).then((r) => {
          if (r.ok) { const cp = r.clone(); caches.open(VERSION).then((c) => c.put(req, cp)); }
          return r;
        })));
});
