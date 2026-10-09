/* Thin browser glue: capability check, loading UI, start gesture (audio unlock,
 * fullscreen, orientation lock), lifecycle hooks, PWA registration.
 * All game logic stays in C. */
(function () {
  'use strict';
  var $ = function (id) { return document.getElementById(id); };
  window.__sa = window.__sa || { state: 'boot', errors: [] };

  function webgl2Supported() {
    try {
      var c = document.createElement('canvas');
      return !!(window.WebGL2RenderingContext && c.getContext('webgl2'));
    } catch (e) { return false; }
  }

  function fail(why) {
    $('overlay').hidden = true;
    $('canvas').hidden = true;
    $('unsupported').hidden = false;
    if (why) $('unsupported-why').textContent = why;
    window.__sa.state = 'unsupported';
  }

  var audioCtxs = [];
  function unlockAudio() {
    // Emscripten/miniaudio creates AudioContexts lazily; resume any that exist
    // and create a throwaway one inside the gesture so later contexts are allowed.
    try {
      var AC = window.AudioContext || window.webkitAudioContext;
      if (AC) { var c = new AC(); c.resume(); audioCtxs.push(c); }
    } catch (e) { /* audio is optional */ }
  }

  function enterFullscreenLandscape() {
    var el = document.documentElement;
    var p = el.requestFullscreen ? el.requestFullscreen({ navigationUI: 'hide' }) : null;
    Promise.resolve(p).then(function () {
      if (screen.orientation && screen.orientation.lock) return screen.orientation.lock('landscape');
    }).catch(function () { /* unsupported or denied: not fatal */ });
  }

  function preventBrowserGestures() {
    document.addEventListener('contextmenu', function (e) { e.preventDefault(); });
    ['gesturestart', 'gesturechange', 'gestureend'].forEach(function (n) {
      document.addEventListener(n, function (e) { e.preventDefault(); });
    });
    document.addEventListener('touchmove', function (e) { e.preventDefault(); }, { passive: false });
    // keys the game uses that the browser would otherwise act on (focus change, page scroll)
    document.addEventListener('keydown', function (e) {
      if (e.key === 'Tab' || (e.key === ' ' && e.target === document.body) || e.key === 'ArrowUp' || e.key === 'ArrowDown') e.preventDefault();
    });
    window.addEventListener('wheel', function (e) { if (e.ctrlKey) e.preventDefault(); }, { passive: false });
  }

  function registerSW() {
    if (!('serviceWorker' in navigator)) return;
    var local = /^(localhost|127\.0\.0\.1)$/.test(location.hostname);
    if (local && location.search.indexOf('sw=1') < 0) return; // avoid stale caches in dev
    var hadController = !!navigator.serviceWorker.controller;
    navigator.serviceWorker.register('sw.js').catch(function (e) { console.warn('SW registration failed', e); });
    navigator.serviceWorker.addEventListener('controllerchange', function () {
      // a new build took over while the old one is running: offer a reload, never force one mid-game
      if (hadController) { window.__sa.updateReady = true; var n = $('update'); if (n) n.hidden = false; }
      hadController = true;
    });
  }

  // ---- asset packs: content-addressed archives streamed into the virtual FS (see tools/pack_assets.py) ----
  var manifest = null;
  var packState = {};            // name -> 0 none, 1 loading, 2 ready, 3 failed   (mirrors window.__sa.packs)
  var packBytes = {};            // name -> bytes received (progress)
  function setPack(name, s) { packState[name] = s; (window.__sa.packs = window.__sa.packs || {})[name] = s; }

  function hex(buf) { return Array.prototype.map.call(new Uint8Array(buf), function (b) { return (b < 16 ? '0' : '') + b.toString(16); }).join(''); }

  function sleep(ms) { return new Promise(function (r) { setTimeout(r, ms); }); }

  function fetchManifest() {
    return fetch('packs.json', { cache: 'no-cache' }).then(function (r) {
      if (!r.ok) throw new Error('packs.json ' + r.status);
      return r.json();
    }).then(function (m) { manifest = m; return m; });
  }

  // download with progress; returns an ArrayBuffer
  function download(entry, onBytes) {
    return fetch(entry.file).then(function (r) {
      if (!r.ok) throw new Error(entry.file + ' ' + r.status);
      if (!r.body || !r.body.getReader) return r.arrayBuffer().then(function (b) { onBytes(b.byteLength); return b; });
      var reader = r.body.getReader(), chunks = [], got = 0;
      return (function pump() {
        return reader.read().then(function (res) {
          if (res.done) {
            var out = new Uint8Array(got), o = 0;
            chunks.forEach(function (c) { out.set(c, o); o += c.length; });
            return out.buffer;
          }
          chunks.push(res.value); got += res.value.length; onBytes(got);
          return pump();
        });
      })();
    });
  }

  function mkdirs(FS, path) {
    var parts = path.split('/').filter(Boolean), cur = '';
    for (var i = 0; i < parts.length - 1; ++i) { cur += '/' + parts[i]; try { FS.mkdir(cur); } catch (e) { /* exists */ } }
  }

  // unpack "SAPK" into the Emscripten FS
  function mount(FS, buf) {
    var dv = new DataView(buf), u8 = new Uint8Array(buf), dec = new TextDecoder();
    if (dv.getUint32(0, true) !== 0x4b504153 || dv.getUint32(4, true) !== 1) throw new Error('bad pack header');
    var n = dv.getUint32(8, true), o = 12;
    for (var i = 0; i < n; ++i) {
      var pl = dv.getUint16(o, true); o += 2;
      var path = dec.decode(u8.subarray(o, o + pl)); o += pl;
      var sz = dv.getUint32(o, true); o += 4;
      if (o + sz > buf.byteLength) throw new Error('truncated pack');
      mkdirs(FS, path);
      FS.writeFile(path, u8.slice(o, o + sz));
      o += sz;
    }
    return n;
  }

  function loadPackOnce(FS, name, onBytes) {
    var e = manifest.packs[name];
    if (!e) return Promise.reject(new Error('unknown pack ' + name));
    return download(e, onBytes).then(function (buf) {
      return crypto.subtle.digest('SHA-256', buf).then(function (d) {
        if (hex(d) !== e.sha256) {
          // a corrupt cache entry must not stick: drop it so the retry hits the network
          var drop = window.caches ? caches.open('sa-packs').then(function (c) { return c.delete(e.file); }) : Promise.resolve();
          return drop.then(function () { throw new Error('checksum mismatch for ' + name); });
        }
        mount(FS, buf);
      });
    });
  }

  // with retries and backoff; resolves when mounted
  function loadPack(FS, name, onBytes) {
    if (packState[name] === 2) return Promise.resolve();
    setPack(name, 1);
    var attempt = 0;
    return (function go() {
      return loadPackOnce(FS, name, onBytes || function () {}).then(function () { setPack(name, 2); }, function (err) {
        if (++attempt < 3) { console.warn('pack ' + name + ' failed (' + err.message + '), retrying'); return sleep(400 * attempt).then(go); }
        setPack(name, 3); window.__sa.errors.push('pack ' + name + ': ' + err.message); throw err;
      });
    })();
  }

  function formatMB(b) { return (b / 1048576).toFixed(1); }

  // preRun: block the runtime until the boot packs are mounted; drives the loading bar
  function loadBootPacks() {
    var FS = Module['FS'];
    return fetchManifest().then(function (m) {
      var names = Object.keys(m.packs).filter(function (n) { return m.packs[n].boot; });
      var total = 0; names.forEach(function (n) { total += m.packs[n].size; packBytes[n] = 0; });
      var update = function () {
        var got = 0; names.forEach(function (n) { got += packBytes[n]; });
        $('loadbar').style.width = (90 * got / total) + '%';
        $('status').textContent = 'Downloading… ' + formatMB(got) + ' / ' + formatMB(total) + ' MB';
      };
      return Promise.all(names.map(function (n) { return loadPack(FS, n, function (b) { packBytes[n] = b; update(); }); }));
    });
  }

  function showLoadError(err) {
    $('status').textContent = 'Download failed: ' + err.message;
    var b = $('start'); b.disabled = false; b.textContent = 'Retry'; b.onclick = function () { location.reload(); };
    window.__sa.state = 'load-failed';
  }

  // C calls these through platform/web/platform.c
  window.SoloAscentPacks = {
    request: function (name) {
      if (!manifest || packState[name]) return;
      loadPack(Module['FS'], name).catch(function () { /* state already 3 */ });
    },
    state: function (name) { return packState[name] || 0; }
  };
  window.SoloAscentGlue_loadPack = function (name) { return loadPack(Module['FS'], name); };

  // ask the browser to keep our caches (best effort) and report headroom
  function storageHealth() {
    if (navigator.storage && navigator.storage.persist) navigator.storage.persist().catch(function () {});
    if (navigator.storage && navigator.storage.estimate) navigator.storage.estimate().then(function (e) {
      window.__sa.storage = { usage: e.usage, quota: e.quota };
      if (e.quota && e.usage / e.quota > 0.9) console.warn('storage nearly full: offline caching may fail');
    }).catch(function () {});
  }

  window.SoloAscentGlue = {
    boot: function () {
      if (!webgl2Supported()) { fail('WebGL2 is required but not available in this browser.'); return; }
      preventBrowserGestures();
      registerSW();

      var canvas = $('canvas');
      window.canvas = canvas; // raylib's fullscreen EM_ASM references a bare `canvas`
      canvas.addEventListener('webglcontextlost', function (e) {
        e.preventDefault(); // allow restoration
        window.__sa.contextLost = (window.__sa.contextLost || 0) + 1;
        if (window.Module && Module._sa_on_context_lost) Module._sa_on_context_lost();
      });
      canvas.addEventListener('webglcontextrestored', function () {
        if (window.Module && Module._sa_on_context_restored) Module._sa_on_context_restored();
      });

      // URL query -> C argv: ?look=2&tier=high&still=1 => --look 2 --tier high --still
      var args = [];
      var q = new URLSearchParams(location.search);
      ['look', 'tier'].forEach(function (k) { if (q.has(k)) args.push('--' + k, q.get(k)); });
      if (q.has('still')) args.push('--still');
      if (q.has('hero')) args.push('--hero');
      if (q.has('arena')) args.push('--arena');
      if (q.has('packtest')) args.push('--packtest');
      if (q.has('rack')) args.push('--rack');
      if (window.matchMedia && matchMedia('(pointer: coarse)').matches) args.push('--mobile');

      window.Module = {
        arguments: args,
        canvas: canvas,
        print: function (t) { console.log(t); },
        printErr: function (t) { console.warn(t); },
        setStatus: function (text) {
          var m = /([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/.exec(text);
          if (m) $('loadbar').style.width = (100 * parseInt(m[2], 10) / parseInt(m[4], 10)) + '%';
          if (text) $('status').textContent = text;
        },
        onAbort: function (w) { window.__sa.errors.push('abort: ' + w); },
        // called from C platform_init(): wire lifecycle events that call back into C
        saHooks: function () {
          var vis = function () { Module._sa_on_visibility(document.hidden ? 1 : 0); };
          document.addEventListener('visibilitychange', vis);
          // Auto-save hooks: persistence arrives with the save milestone; the events are wired now.
          window.addEventListener('pagehide', function () { window.__sa.pagehide = (window.__sa.pagehide || 0) + 1; });
          window.__sa.hooked = true;
        },
        preRun: [function () {
          Module.addRunDependency('packs');
          loadBootPacks().then(function () { Module.removeRunDependency('packs'); }, showLoadError);
        }],
        postRun: [function () {
          $('loadbar').style.width = '100%';
          $('status').textContent = 'Ready';
          $('start').disabled = false;
          window.__sa.loaded = true;
        }]
      };

      $('start').addEventListener('click', function () {
        unlockAudio();
        storageHealth();
        enterFullscreenLandscape();
        $('overlay').hidden = true;
        canvas.focus();
        window.__sa.started = true;
      });
    }
  };
})();
