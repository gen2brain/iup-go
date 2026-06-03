// IUP+Go run here, so they can block on Atomics.wait for value-returning modals while
// main keeps rendering. DOM commands post to main; sync reads return over a SharedArrayBuffer.

var i32, u8, Module;
var eventQueue = [];

self.onmessage = function (e) {
  var m = e.data;
  if (m.type === 'init') {
    i32 = new Int32Array(m.sab);
    u8 = new Uint8Array(m.sab);
    boot();
  } else if (m.__iupEv) {
    dispatchEvent(m.name, m.args, m.types);
  }
};

function dispatchEvent(name, args, types) {
  if (!Module) { eventQueue.push([name, args, types]); return; }
  try {
    if (types) Module.ccall(name, null, types, args);
    else Module['_' + name].apply(null, args);
  } catch (err) { console.error('iup event ' + name, err); }
}

// post the request, block until main writes a result. result tag in i32[1]: 0 int (i32[2]),
// 1 int array (len i32[2] from i32[3]), 2 utf8 (len i32[2] from byte 16), 3 null.
globalThis.__iupReadSync = function (req) {
  Atomics.store(i32, 0, 0);
  self.postMessage({ __iupRead: 1, req: req });
  Atomics.wait(i32, 0, 0);
  var tag = i32[1];
  if (tag === 0) return i32[2];
  if (tag === 1) { var n = i32[2], a = []; for (var k = 0; k < n; k++) a.push(i32[3 + k]); return a; }
  if (tag === 3) return null;
  return new TextDecoder().decode(u8.slice(16, 16 + i32[2]));
};

// Block, then copy returned bytes (u8[16..16+len]) into dest; streams a picked file from main.
globalThis.__iupReadBytes = function (req, destArr, destOff) {
  Atomics.store(i32, 0, 0);
  self.postMessage({ __iupRead: 1, req: req });
  Atomics.wait(i32, 0, 0);
  var len = i32[2];
  destArr.set(u8.subarray(16, 16 + len), destOff);
  return len;
};

// Block until IupExitLoop pops this level; events come over the SAB since a blocked
// Worker can't receive postMessage. Nesting-aware: a modal is a deeper level.
globalThis.__iupPumpDepth = 0;
globalThis.__iupPumpDone = {};

globalThis.__iupRunPump = function () {
  var depth = ++globalThis.__iupPumpDepth;
  globalThis.__iupPumpDone[depth] = false;
  var MS = i32.length - 1;
  globalThis.__iupReadSync({ op: 'pumpenter' });
  var last = Atomics.load(i32, MS);
  while (!globalThis.__iupPumpDone[depth]) {
    // timeout so worker-thread IupPostMessage posts are drained even when idle
    var wr = Atomics.wait(i32, MS, last, 16);
    if (Module && Module._iupwasmDrainPosts) Module._iupwasmDrainPosts();
    if (wr === 'timed-out') continue;
    last = Atomics.load(i32, MS);
    while (!globalThis.__iupPumpDone[depth]) {
      var ev = globalThis.__iupReadSync({ op: 'modalnext' });
      if (ev === null) break;
      var d = JSON.parse(ev);
      dispatchEvent(d.name, d.args, d.types);
    }
  }
  delete globalThis.__iupPumpDone[depth];
  globalThis.__iupPumpDepth--;
  globalThis.__iupReadSync({ op: 'pumpleave' });
};

// End the innermost pump level; with no pump active, end the Go keep-alive.
globalThis.__iupExitLoop = function () {
  var depth = globalThis.__iupPumpDepth;
  if (depth > 0) {
    globalThis.__iupPumpDone[depth] = true;
    var MS = i32.length - 1;
    Atomics.add(i32, MS, 1); Atomics.notify(i32, MS);
    return;
  }
  if (globalThis.iupGoExitLoop) globalThis.iupGoExitLoop();
};

function boot() {
  importScripts('iup.js');
  // mainScriptUrlOrBlob: lets -pthread workers spawn from inside this Worker
  createIupModule({ locateFile: function (p) { return p; }, mainScriptUrlOrBlob: 'iup.js' }).then(function (mod) {
    Module = mod;
    globalThis.IupModule = mod;
    var q = eventQueue; eventQueue = [];
    for (var k = 0; k < q.length; k++) dispatchEvent(q[k][0], q[k][1], q[k][2]);
    return fetch('app.wasm', { method: 'HEAD' }).then(function (r) {
      if (!r.ok) { Module.callMain([]); return; }  // C app: its main() runs the blocking IupMainLoop
      importScripts('wasm_exec.js');
      var go = new Go();
      return WebAssembly.instantiateStreaming(fetch('app.wasm'), go.importObject).then(function (res) { go.run(res.instance); });
    });
  }).catch(function (err) { console.error('iup boot', err); });
}
