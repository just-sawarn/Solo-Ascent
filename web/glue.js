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
    window.addEventListener('wheel', function (e) { if (e.ctrlKey) e.preventDefault(); }, { passive: false });
  }

  function registerSW() {
    if (!('serviceWorker' in navigator)) return;
    var local = /^(localhost|127\.0\.0\.1)$/.test(location.hostname);
    if (local && location.search.indexOf('sw=1') < 0) return; // avoid stale caches in dev
    navigator.serviceWorker.register('sw.js').catch(function (e) { console.warn('SW registration failed', e); });
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

      window.Module = {
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
        postRun: [function () {
          $('loadbar').style.width = '100%';
          $('status').textContent = 'Ready';
          $('start').disabled = false;
          window.__sa.loaded = true;
        }]
      };

      $('start').addEventListener('click', function () {
        unlockAudio();
        enterFullscreenLandscape();
        $('overlay').hidden = true;
        canvas.focus();
        window.__sa.started = true;
      });
    }
  };
})();
