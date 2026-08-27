// Loads build/index.html in a headless Chrome/Chromium (via playwright-core, reusing the
// system browser), captures console output and a screenshot, and optionally clicks a
// selector to drive a callback. Set IUP_BROWSER to force a specific browser binary.
//
//   node run-browser.js <buildDir> <screenshot.png> <console.txt> [clickSelector]

const http = require('http');
const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');
const { chromium } = require('playwright-core');

// Prefer an explicit binary, then the first Chrome/Chromium found on PATH, else the chrome channel.
function browserLaunchOptions() {
  const opts = { headless: false, args: ['--headless=new'] };
  if (process.env.IUP_BROWSER) { opts.executablePath = process.env.IUP_BROWSER; return opts; }
  for (const bin of ['google-chrome', 'google-chrome-stable', 'chromium', 'chromium-browser']) {
    try { opts.executablePath = execSync('command -v ' + bin, { stdio: ['ignore', 'pipe', 'ignore'] }).toString().trim(); return opts; }
    catch (e) { /* not on PATH */ }
  }
  opts.channel = 'chrome';
  return opts;
}

const buildDir = process.argv[2];
const shot = process.argv[3] || '/tmp/screenshot.png';
const logFile = process.argv[4] || '/tmp/wasm_console.txt';
const clickSel = process.argv[5];
const typeText = process.argv[6];
const keySeq = process.argv[7];

const MIME = {
  '.html': 'text/html',
  '.js': 'text/javascript',
  '.wasm': 'application/wasm',
  '.png': 'image/png',
};

function serve(dir) {
  return http.createServer((req, res) => {
    let rel = decodeURIComponent(req.url.split('?')[0]);
    if (rel === '/favicon.ico') { res.writeHead(204); res.end(); return; }
    if (rel === '/') rel = '/index.html';
    const file = path.join(dir, rel);
    fs.readFile(file, (err, data) => {
      if (err) { res.writeHead(404); res.end(); return; }
      res.writeHead(200, {
        'Content-Type': MIME[path.extname(file)] || 'application/octet-stream',
        // Cross-origin isolation: required for SharedArrayBuffer in worker mode.
        'Cross-Origin-Opener-Policy': 'same-origin',
        'Cross-Origin-Embedder-Policy': 'require-corp',
      });
      res.end(data);
    });
  });
}

(async () => {
  const server = serve(buildDir);
  await new Promise((r) => server.listen(0, r));
  const port = server.address().port;

  // new headless renders like a real browser (classic scrollbars, etc.)
  const browser = await chromium.launch(browserLaunchOptions());
  // touch is opt-in: reporting it changes hover and UA styling for every other test
  const wantTouch = /(^|;)\s*(tap|swipe|pinch|rotate):/.test(keySeq || '');
  const page = await browser.newPage({
    viewport: { width: 1280, height: 960 },
    colorScheme: process.env.IUP_DARK ? 'dark' : 'light',
    hasTouch: wantTouch,
  });
  await page.context().grantPermissions(['clipboard-read', 'clipboard-write'], { origin: 'http://localhost:' + port }).catch(() => {});

  const logs = [];
  page.on('console', (m) => logs.push(m.text()));
  page.on('pageerror', (e) => logs.push('PAGEERROR: ' + e.message));
  page.on('dialog', (d) => { logs.push('DIALOG[' + d.type() + ']: ' + d.message()); d.accept().catch(() => {}); });

  // feed an IupFileDlg OPEN a tiny valid PNG headlessly, written only when a chooser actually opens
  page.on('filechooser', (fc) => {
    logs.push('FILECHOOSER');
    const testFile = path.join(require('os').tmpdir(), 'iup_filechooser_test.png');
    fs.writeFileSync(testFile, Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYPhfDwAChwGA60e6kgAAAABJRU5ErkJggg==', 'base64'));
    fc.setFiles(testFile).catch((e) => logs.push('setFiles failed: ' + e.message));
  });

  // Measure the rendered dialog(s) and size the viewport to them (the dialog's pixel size is
  // intrinsic from IUP layout). MARGIN is viewport headroom only; capture() clips to the dialog.
  const MARGIN = 24;
  let clip = null;
  async function measure() {
    return page.evaluate(() => {
      const els = (globalThis.__iup && globalThis.__iup.els) || {};
      let minL = Infinity, minT = Infinity, maxR = -Infinity, maxB = -Infinity;
      const dialogs = [];
      let fill = false;
      for (const k in els) {
        const el = els[k];
        if (!el || el.nodeType !== 1 || el.parentElement !== document.body) continue;
        const cs = getComputedStyle(el);
        if (cs.display === 'none' || cs.visibility === 'hidden') continue;
        const r = el.getBoundingClientRect();
        if (r.width < 1 || r.height < 1) continue;
        if (el.__iupViewportFill) fill = true;
        dialogs.push({ k, left: r.left, top: r.top });
        minL = Math.min(minL, r.left); minT = Math.min(minT, r.top);
        maxR = Math.max(maxR, r.right); maxB = Math.max(maxB, r.bottom);
      }
      return dialogs.length ? { minL, minT, maxR, maxB, dialogs, fill } : null;
    });
  }
  async function fitViewport() {
    // a heavy dialog can still be laying out when the first element attaches: settle for two
    // equal measurements before clipping to it
    let box = await measure(), stable = 0;
    for (let i = 0; i < 12 && stable < 1; i++) {
      await page.waitForTimeout(100);
      const next = await measure();
      const same = box && next && box.minL === next.minL && box.minT === next.minT &&
                   box.maxR === next.maxR && box.maxB === next.maxB;
      if (same) stable++;
      box = next;
    }
    if (!box) { clip = null; return; }
    // a maximized dialog re-fills the viewport on every resize, so fitting it would chase itself
    if (box.fill) { clip = null; return; }
    const w = Math.ceil(box.maxR - box.minL), h = Math.ceil(box.maxB - box.minT);
    await page.setViewportSize({ width: Math.min(2400, w + 2 * MARGIN), height: Math.min(2400, h + 2 * MARGIN) });
    await page.evaluate(({ box, MARGIN }) => {
      const els = globalThis.__iup.els;
      for (const d of box.dialogs) {
        els[d.k].style.left = (d.left - box.minL + MARGIN) + 'px';
        els[d.k].style.top = (d.top - box.minT + MARGIN) + 'px';
      }
    }, { box, MARGIN });
    await page.waitForTimeout(50);
    clip = { x: MARGIN, y: MARGIN, width: w, height: h };
  }

  async function capture(file) {
    await page.screenshot(clip ? { path: file, clip } : { path: file });
  }

  await page.goto(`http://localhost:${port}/index.html`, { waitUntil: 'load' });
  await page.waitForSelector('[data-iup-id]', { state: 'attached', timeout: parseInt(process.env.IUP_WAIT) || 20000 })
    .catch(() => logs.push('(timeout waiting for an IUP element)'));
  await page.waitForTimeout(150);

  await fitViewport();
  await capture(shot);
  logs.push('screenshot: ' + shot);

  if (clickSel || typeText) {
    if (clickSel) await page.click(clickSel).catch((e) => logs.push('click failed: ' + e.message));
    if (typeText) await page.fill(clickSel || 'input', typeText).catch((e) => logs.push('type failed: ' + e.message));
    await page.waitForTimeout(typeText ? 1200 : 200);
    await fitViewport();
    const after = shot.replace(/\.png$/, '_after.png');
    await capture(after);
    logs.push('screenshot after ' + (typeText ? 'type' : 'click') + ': ' + after);
  }

  // Scripted sequence: steps split by ';', each "cmd:arg"; see build-wasm.sh -h for the vocabulary.
  if (keySeq) {
    let n = 0;
    for (const st of keySeq.split(';')) {
      const ci = st.indexOf(':');
      const cmd = ci < 0 ? st : st.slice(0, ci);
      const arg = ci < 0 ? '' : st.slice(ci + 1);
      if (cmd === 'click' || cmd === 'dblclick') {
        // x,y clicks inside the element, the only way to reach a canvas-drawn control
        var cp = arg.split('##'), copt = {};
        if (cp[1]) { var cxy = cp[1].split(','); copt.position = { x: +cxy[0], y: +cxy[1] }; }
        await page[cmd](cp[0], copt).catch((e) => logs.push(cmd + ' failed: ' + e.message));
      }
      else if (cmd === 'type') await page.keyboard.type(arg).catch((e) => logs.push('type failed: ' + e.message));
      else if (cmd === 'press') await page.keyboard.press(arg).catch((e) => logs.push('press failed: ' + e.message));
      else if (cmd === 'rawkey') {
        // CDP: dispatch a trusted keydown carrying arbitrary text (playwright press() only knows named keys)
        const cdp = await page.context().newCDPSession(page).catch(() => null);
        if (cdp) {
          await cdp.send('Input.dispatchKeyEvent', { type: 'keyDown', text: arg, key: arg })
            .catch((e) => logs.push('rawkey failed: ' + e.message));
          await cdp.send('Input.dispatchKeyEvent', { type: 'keyUp', key: arg }).catch(() => {});
          await cdp.detach().catch(() => {});
        } else logs.push('rawkey: no CDP session');
      }
      else if (cmd === 'tap' || cmd === 'swipe' || cmd === 'pinch' || cmd === 'rotate') {
        // CDP touch: playwright's touchscreen has no multi-finger or move support
        const p = arg.split('##');
        const box = await page.locator(p[0]).boundingBox().catch(() => null);
        if (!box) { logs.push(cmd + ': element not found: ' + p[0]); continue; }
        let cx = box.x + box.width / 2, cy = box.y + box.height / 2;
        if (cmd !== 'pinch' && p[1] && cmd === 'tap') { const xy = p[1].split(','); cx = box.x + (+xy[0]); cy = box.y + (+xy[1]); }
        const cdp = await page.context().newCDPSession(page).catch(() => null);
        if (!cdp) { logs.push(cmd + ': no CDP session'); continue; }
        const send = (type, points) => cdp.send('Input.dispatchTouchEvent', { type, touchPoints: points })
          .catch((e) => logs.push(cmd + ' failed: ' + e.message));
        if (cmd === 'tap') {
          await send('touchStart', [{ x: cx, y: cy, id: 1 }]);
          await page.waitForTimeout(parseInt(p[2]) || 60);
          await send('touchEnd', []);
        } else if (cmd === 'swipe') {
          // one move per frame: Chrome coalesces touch moves sent inside the same frame
          const d = (p[1] || '0,0').split(','), dx = +d[0], dy = +d[1];
          await send('touchStart', [{ x: cx, y: cy, id: 1 }]);
          for (let i = 1; i <= 5; i++) {
            await page.waitForTimeout(30);
            await send('touchMove', [{ x: cx + dx * i / 5, y: cy + dy * i / 5, id: 1 }]);
          }
          await send('touchEnd', []);
        } else {
          const half = Math.min(box.width, box.height) / 4;
          const scale = cmd === 'pinch' ? (parseFloat(p[1]) || 2) : 1;
          const turn = cmd === 'rotate' ? (parseFloat(p[1]) || 45) * Math.PI / 180 : 0;
          const pair = (h, a) => [{ x: cx - h * Math.cos(a), y: cy - h * Math.sin(a), id: 1 },
                                  { x: cx + h * Math.cos(a), y: cy + h * Math.sin(a), id: 2 }];
          await send('touchStart', pair(half, 0));
          for (let i = 1; i <= 5; i++) {
            await page.waitForTimeout(20);
            await send('touchMove', pair(half * (1 + (scale - 1) * i / 5), turn * i / 5));
          }
          await send('touchEnd', []);
        }
        await cdp.detach().catch(() => {});
      }
      else if (cmd === 'reload') {
        await page.reload({ waitUntil: 'load' }).catch((e) => logs.push('reload failed: ' + e.message));
        await page.waitForSelector('[data-iup-id]', { state: 'attached', timeout: parseInt(process.env.IUP_WAIT) || 20000 })
          .catch(() => logs.push('(timeout waiting for an IUP element after reload)'));
        await page.waitForTimeout(parseInt(arg) || 300);
      }
      else if (cmd === 'wait') await page.waitForTimeout(parseInt(arg) || 0);
      else if (cmd === 'drag') {
        const p = arg.split('##'), o = {};
        if (p[2]) { const xy = p[2].split(','); o.targetPosition = { x: +xy[0], y: +xy[1] }; }
        await page.dragAndDrop(p[0], p[1], o).catch((e) => logs.push('drag failed: ' + e.message));
      }
      else if (cmd === 'mdrag') {
        const p = arg.split('##'), d = (p[1] || '0,0').split(',');
        const box = await page.locator(p[0]).boundingBox().catch(() => null);
        if (box) {
          const cx = box.x + box.width / 2, cy = box.y + box.height / 2;
          await page.mouse.move(cx, cy); await page.mouse.down();
          await page.mouse.move(cx + (+d[0]), cy + (+d[1]), { steps: 12 }); await page.mouse.up();
        } else logs.push('mdrag: element not found: ' + p[0]);
      }
      else if (cmd === 'shot') { await fitViewport(); const f = shot.replace(/\.png$/, '_step' + (++n) + '.png'); await capture(f); logs.push('step ' + n + ': ' + f); }
    }
    await fitViewport();
    const after = shot.replace(/\.png$/, '_after.png');
    await capture(after);
    logs.push('sequence done: ' + after);
  }

  fs.writeFileSync(logFile, logs.join('\n') + '\n');
  await browser.close();
  server.close();
  console.log(logs.join('\n'));
})().catch((e) => { console.error(e); process.exit(1); });
