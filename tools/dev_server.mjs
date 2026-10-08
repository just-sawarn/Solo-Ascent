// Zero-dependency static dev server: correct wasm MIME, precompressed .br/.gz support,
// and live reload (SSE) when files in the served directory change.
//   node dev_server.mjs <dir> [port]
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const MIME = {
  '.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8', '.mjs': 'text/javascript',
  '.wasm': 'application/wasm', '.json': 'application/json', '.webmanifest': 'application/manifest+json',
  '.css': 'text/css', '.png': 'image/png', '.map': 'application/json', '.data': 'application/octet-stream',
  '.ogg': 'audio/ogg', '.opus': 'audio/ogg', '.ktx2': 'image/ktx2',
};
const RELOAD_SNIPPET = '<script>new EventSource("/__reload").onmessage=()=>location.reload()</script>';

export function startServer(root, port = 8080, { liveReload = false, quiet = false } = {}) {
  root = path.resolve(root);
  const clients = new Set();
  const server = http.createServer((req, res) => {
    const url = new URL(req.url, 'http://x');
    if (url.pathname === '/__reload' && liveReload) {
      res.writeHead(200, { 'Content-Type': 'text/event-stream', 'Cache-Control': 'no-cache' });
      res.write('\n'); clients.add(res); req.on('close', () => clients.delete(res)); return;
    }
    let rel = decodeURIComponent(url.pathname);
    if (rel.endsWith('/')) rel += 'index.html';
    const file = path.join(root, path.normalize(rel));
    if (!file.startsWith(root + path.sep)) { res.writeHead(403).end('forbidden'); return; }
    fs.stat(file, (err, st) => {
      if (err || !st.isFile()) { res.writeHead(404).end('not found'); return; }
      const ext = path.extname(file);
      const headers = { 'Content-Type': MIME[ext] || 'application/octet-stream', 'Cache-Control': 'no-cache' };
      const ae = String(req.headers['accept-encoding'] || '');
      for (const [enc, suffix] of [['br', '.br'], ['gzip', '.gz']]) {
        if (ae.includes(enc) && fs.existsSync(file + suffix)) {
          res.writeHead(200, { ...headers, 'Content-Encoding': enc, Vary: 'Accept-Encoding' });
          fs.createReadStream(file + suffix).pipe(res); return;
        }
      }
      if (liveReload && ext === '.html') {
        res.writeHead(200, headers);
        res.end(fs.readFileSync(file, 'utf8').replace('</body>', RELOAD_SNIPPET + '</body>')); return;
      }
      res.writeHead(200, { ...headers, 'Content-Length': st.size });
      fs.createReadStream(file).pipe(res);
    });
  });
  if (liveReload) {
    let timer;
    fs.watch(root, { recursive: true }, () => {
      clearTimeout(timer);
      timer = setTimeout(() => clients.forEach((c) => c.write('data: reload\n\n')), 250);
    });
  }
  return new Promise((resolve) => server.listen(port, () => {
    if (!quiet) console.log(`serving ${root} at http://localhost:${server.address().port}`);
    resolve(server);
  }));
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  const dir = process.argv[2] || '../build-web-debug';
  startServer(dir, Number(process.argv[3] || 8080), { liveReload: true });
}
