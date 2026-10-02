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
    runEvent(m);
  }
};

// An event can arrive twice, by postMessage and replayed over the SAB when a modal pump starts;
// the sequence drops the duplicate and tells main how far this Worker has got.
var lastEvSeq = 0;

function runEvent(ev) {
  if (ev.seq) {
    if (ev.seq <= lastEvSeq) return;
    lastEvSeq = ev.seq;
    if (i32) Atomics.store(i32, i32.length - 2, ev.seq);
  }
  dispatchEvent(ev.name, ev.args, ev.types);
}

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
      runEvent(JSON.parse(ev));
    }
  }
  delete globalThis.__iupPumpDone[depth];
  globalThis.__iupPumpDepth--;
  globalThis.__iupReadSync({ op: 'pumpleave' });
};

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

// CACHEDIR/DATADIR/CONFIGDIR live under the home directory; /tmp stays in memory. syncfs is
// asynchronous, so the mount has to happen before the app runs.
var IUP_HOME = '/home/web_user';

function mountPersistentHome() {
  return new Promise(function (done) {
    try {
      Module.FS.mkdirTree(IUP_HOME);
      Module.FS.mount(Module.IDBFS, {}, IUP_HOME);
    } catch (e) { done(); return; }
    Module.FS.syncfs(true, function () {
      var timer = 0, busy = 0;
      var flush = function () {
        timer = 0;
        if (busy) { timer = setTimeout(flush, 250); return; }
        busy = 1;
        Module.FS.syncfs(false, function () { busy = 0; });
      };
      var touch = function (path) {
        if (typeof path !== 'string' || path.indexOf(IUP_HOME) !== 0) return;
        if (!timer) timer = setTimeout(flush, 250);
      };
      // FS.trackingDelegate only exists in an FS_DEBUG build, so the writers are wrapped instead
      var wrap = function (name, pathOf) {
        var orig = Module.FS[name];
        if (typeof orig !== 'function') return;
        Module.FS[name] = function () {
          var r = orig.apply(Module.FS, arguments);
          touch(pathOf.apply(null, arguments));
          return r;
        };
      };
      wrap('close', function (stream) { return stream && stream.path; });
      wrap('unlink', function (path) { return path; });
      wrap('rmdir', function (path) { return path; });
      done();
    });
  });
}

// iupdrvImageLoad is synchronous and the browser decoder is not, so images are decoded here
function preloadResources() {
  var FS = Module.FS;
  globalThis.__iupResourceImages = {};
  try { FS.mkdirTree('/resources'); } catch (e) { return Promise.resolve(); }
  return fetch('resources.json').then(function (r) { return r.ok ? r.json() : []; }, function () { return []; }).then(function (names) {
    return Promise.all(names.map(function (name) {
      var path = '/resources/' + name;
      return fetch('resources/' + name.split('/').map(encodeURIComponent).join('/')).then(function (r) {
        if (!r.ok) throw new Error(name + ': ' + r.status);
        return r.arrayBuffer();
      }).then(function (buf) {
        var data = new Uint8Array(buf);
        FS.mkdirTree(path.substring(0, path.lastIndexOf('/')));
        FS.writeFile(path, data);
        if (!/\.(png|jpe?g|gif|bmp|webp|ico|avif)$/i.test(name)) return;
        return createImageBitmap(new Blob([data]), { colorSpaceConversion: 'none', premultiplyAlpha: 'none' }).then(function (bmp) {
          var cv = new OffscreenCanvas(bmp.width, bmp.height);
          var ctx = cv.getContext('2d');
          ctx.drawImage(bmp, 0, 0);
          globalThis.__iupResourceImages[path] = { w: bmp.width, h: bmp.height, rgba: ctx.getImageData(0, 0, bmp.width, bmp.height).data };
          bmp.close();
        }, function () {});
      });
    }));
  }).catch(function (err) { console.error('iup resources', err); });
}

// wasm_exec.js installs its ENOSYS fs stub only when globalThis.fs is unset
function installGoFs() {
  var FS = Module.FS;
  var codes = { 2: 'EACCES', 8: 'EBADF', 10: 'EBUSY', 20: 'EEXIST', 28: 'EINVAL', 29: 'EIO', 31: 'EISDIR',
    32: 'ELOOP', 33: 'EMFILE', 37: 'ENAMETOOLONG', 44: 'ENOENT', 51: 'ENOSPC', 52: 'ENOSYS', 54: 'ENOTDIR',
    55: 'ENOTEMPTY', 63: 'EPERM', 69: 'EROFS', 70: 'ESPIPE', 75: 'EXDEV' };
  var fail = function (e) {
    var err = new Error((e && e.message) || 'fs error');
    err.code = (e && codes[e.errno]) || 'EIO';
    return err;
  };
  var call = function (cb, fn) {
    var r;
    try { r = fn(); } catch (e) { cb(fail(e)); return; }
    cb(null, r);
  };
  var stream = function (fd) {
    var st = FS.getStream(fd);
    if (!st) throw new FS.ErrnoError(8);
    return st;
  };
  var at = function (p) { return p === null ? undefined : p; };
  var stat = function (s) {
    return { dev: s.dev, ino: s.ino, mode: s.mode, nlink: s.nlink, uid: s.uid, gid: s.gid, rdev: s.rdev,
      size: s.size, blksize: s.blksize, blocks: s.blocks, atimeMs: +s.atime, mtimeMs: +s.mtime, ctimeMs: +s.ctime,
      isDirectory: function () { return (s.mode & 61440) === 16384; } };
  };
  var out = '', dec = new TextDecoder();
  var print = function (buf) {
    out += dec.decode(buf);
    var nl = out.lastIndexOf('\n');
    if (nl !== -1) { console.log(out.substring(0, nl)); out = out.substring(nl + 1); }
    return buf.length;
  };
  globalThis.fs = {
    constants: { O_WRONLY: 1, O_RDWR: 2, O_CREAT: 64, O_EXCL: 128, O_TRUNC: 512, O_APPEND: 1024, O_DIRECTORY: 65536 },
    writeSync: function (fd, buf) {
      if (fd === 1 || fd === 2) return print(buf);
      return FS.write(stream(fd), buf, 0, buf.length);
    },
    write: function (fd, buf, offset, length, position, cb) {
      if ((fd === 1 || fd === 2) && offset === 0 && length === buf.length && position === null) { cb(null, print(buf)); return; }
      call(cb, function () { return FS.write(stream(fd), buf, offset, length, at(position)); });
    },
    read: function (fd, buf, offset, length, position, cb) {
      call(cb, function () { return FS.read(stream(fd), buf, offset, length, at(position)); });
    },
    open: function (path, flags, mode, cb) { call(cb, function () { return FS.open(path, flags, mode).fd; }); },
    close: function (fd, cb) { call(cb, function () { FS.close(stream(fd)); }); },
    fstat: function (fd, cb) { call(cb, function () { return stat(FS.fstat(fd)); }); },
    stat: function (path, cb) { call(cb, function () { return stat(FS.stat(path)); }); },
    lstat: function (path, cb) { call(cb, function () { return stat(FS.lstat(path)); }); },
    readdir: function (path, cb) {
      call(cb, function () { return FS.readdir(path).filter(function (n) { return n !== '.' && n !== '..'; }); });
    },
    mkdir: function (path, perm, cb) { call(cb, function () { FS.mkdir(path, perm); }); },
    unlink: function (path, cb) { call(cb, function () { FS.unlink(path); }); },
    rmdir: function (path, cb) { call(cb, function () { FS.rmdir(path); }); },
    rename: function (from, to, cb) { call(cb, function () { FS.rename(from, to); }); },
    chmod: function (path, mode, cb) { call(cb, function () { FS.chmod(path, mode); }); },
    fchmod: function (fd, mode, cb) { call(cb, function () { FS.fchmod(fd, mode); }); },
    chown: function (path, uid, gid, cb) { cb(null); },
    fchown: function (fd, uid, gid, cb) { cb(null); },
    lchown: function (path, uid, gid, cb) { cb(null); },
    utimes: function (path, atime, mtime, cb) { call(cb, function () { FS.utime(path, atime * 1000, mtime * 1000); }); },
    truncate: function (path, length, cb) { call(cb, function () { FS.truncate(path, length); }); },
    ftruncate: function (fd, length, cb) { call(cb, function () { FS.ftruncate(fd, length); }); },
    readlink: function (path, cb) { call(cb, function () { return FS.readlink(path); }); },
    symlink: function (path, link, cb) { call(cb, function () { FS.symlink(path, link); }); },
    link: function (path, link, cb) { cb(fail({ errno: 52 })); },
    fsync: function (fd, cb) { cb(null); }
  };
}

function boot() {
  importScripts('iup.js');
  // mainScriptUrlOrBlob: lets -pthread workers spawn from inside this Worker
  createIupModule({ locateFile: function (p) { return p; }, mainScriptUrlOrBlob: 'iup.js' }).then(function (mod) {
    Module = mod;
    globalThis.IupModule = mod;
    var q = eventQueue; eventQueue = [];
    for (var k = 0; k < q.length; k++) dispatchEvent(q[k][0], q[k][1], q[k][2]);
    return mountPersistentHome();
  }).then(function () {
    return preloadResources();
  }).then(function () {
    return fetch('app.wasm', { method: 'HEAD' }).then(function (r) {
      if (!r.ok) { Module.callMain([]); return; }  // C app: its main() runs the blocking IupMainLoop
      installGoFs();
      importScripts('wasm_exec.js');
      var go = new Go();
      return WebAssembly.instantiateStreaming(fetch('app.wasm'), go.importObject).then(function (res) { go.run(res.instance); });
    });
  }).catch(function (err) { console.error('iup boot', err); });
}
