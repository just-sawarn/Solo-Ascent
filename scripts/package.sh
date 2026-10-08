#!/usr/bin/env bash
# Build release and assemble /dist (static-host ready, with brotli + gzip precompressed copies).
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh release
rm -rf dist && mkdir dist
cp build-web-release/{index.html,index.js,index.wasm,glue.js,game.css,sw.js,manifest.webmanifest,icon-192.png,icon-512.png} dist/
cat > dist/config.json <<'JSON'
{ "supabaseUrl": "", "supabaseAnonKey": "" }
JSON
for f in dist/index.js dist/index.wasm dist/glue.js dist/index.html dist/game.css; do
  brotli -q 11 -k -f "$f"; gzip -9 -k -f "$f"
done
du -sh dist; ls -la dist
