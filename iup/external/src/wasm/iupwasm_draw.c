/** \file
 * \brief WebAssembly Draw API
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"

#include "iupwasm_drv.h"


typedef struct _IwasmDrawLayer
{
  int clip_x1, clip_y1, clip_x2, clip_y2;
  struct _IwasmDrawLayer* next;
} IwasmDrawLayer;

struct _IdrawCanvas
{
  Ihandle* ih;
  int cid;          /* canvas element id */
  int w, h;
  int clip_x1, clip_y1, clip_x2, clip_y2;
  IwasmDrawLayer* layers;
};

EM_JS(void, iupwasmJsCanvasInit, (void), {
  if (globalThis.__iupRGBA) return;
  globalThis.__iupRGBA = function(r, g, b, a) {
    return "rgba(" + r + "," + g + "," + b + "," + (a / 255) + ")";
  };
  globalThis.__iupDash = function(ctx, strokePtr, lw) {
    var s = strokePtr >> 3;
    var count = HEAPF64[s + 4], dash = [];
    for (var i = 0; i < count; i++)
      dash.push(HEAPF64[s + 5 + i]);
    ctx.lineCap = ["butt", "round", "square"][HEAPF64[s]];
    ctx.lineJoin = ["miter", "round", "bevel"][HEAPF64[s + 1]];
    ctx.miterLimit = HEAPF64[s + 2];
    ctx.setLineDash(dash);
    ctx.lineDashOffset = HEAPF64[s + 3];
    ctx.lineWidth = lw < 1 ? 1 : lw;
  };
  globalThis.__iupLayers = {};
  globalThis.__iupBaseCanvasOf = function(cid) {
    if (typeof document === 'undefined') {
      globalThis.__iupLocal = globalThis.__iupLocal || {};
      if (!globalThis.__iupLocal[cid]) globalThis.__iupLocal[cid] = new OffscreenCanvas(1, 1);
      return globalThis.__iupLocal[cid];
    }
    var el = globalThis.__iup.els[cid];
    return el && el.__iupCanvas ? el.__iupCanvas : el;
  };
  globalThis.__iupCanvasOf = function(cid) {
    var stack = globalThis.__iupLayers[cid];
    if (stack && stack.length) return stack[stack.length - 1];
    return globalThis.__iupBaseCanvasOf(cid);
  };
  globalThis.__iupCtx = function(cid) {
    var cv = globalThis.__iupCanvasOf(cid);
    return cv ? cv.getContext("2d") : null;
  };
})

EM_JS(void, iupwasmJsCanvasReset, (int cid, int w, int h), {
  delete globalThis.__iupLayers[cid];
  var el = globalThis.__iupBaseCanvasOf(cid); if (!el) return;
  el.width = w;
  el.height = h;
  var ctx = el.getContext("2d");
  ctx.setLineDash([]);
  ctx.__iupTransform = [1, 0, 0, 1, 0, 0];
  ctx.save();
})

EM_JS(void, iupwasmJsSetTransform, (int cid, double a, double b, double c, double d, double e, double f), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  ctx.__iupTransform = [a, b, c, d, e, f];
  ctx.setTransform(a, b, c, d, e, f);
})

EM_JS(int, iupwasmJsCanvasClientW, (int cid), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'canvasclientw', id: cid });
  var el = globalThis.__iup.els[cid];
  return el ? (el.clientWidth || el.width || 0) : 0;
})

EM_JS(int, iupwasmJsCanvasClientH, (int cid), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'canvasclienth', id: cid });
  var el = globalThis.__iup.els[cid];
  return el ? (el.clientHeight || el.height || 0) : 0;
})

EM_JS(void, iupwasmJsDrawLine, (int cid, int x1, int y1, int x2, int y2, int r, int g, int b, int a, int strokePtr, int lw), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  globalThis.__iupDash(ctx, strokePtr, lw);
  ctx.strokeStyle = globalThis.__iupRGBA(r, g, b, a);
  var o = (ctx.lineWidth % 2) ? 0.5 : 0;
  ctx.beginPath();
  ctx.moveTo(x1 + o, y1 + o);
  ctx.lineTo(x2 + o, y2 + o);
  ctx.stroke();
})

EM_JS(void, iupwasmJsDrawRect, (int cid, int x1, int y1, int x2, int y2, int r, int g, int b, int a, int style, int strokePtr, int lw), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var col = globalThis.__iupRGBA(r, g, b, a);
  if (style == 0) {  /* FILL */
    ctx.setLineDash([]);
    ctx.fillStyle = col;
    ctx.fillRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  } else {
    globalThis.__iupDash(ctx, strokePtr, lw);
    ctx.strokeStyle = col;
    var o = (ctx.lineWidth % 2) ? 0.5 : 0;
    ctx.strokeRect(x1 + o, y1 + o, x2 - x1, y2 - y1);
  }
})

EM_JS(void, iupwasmJsDrawArc, (int cid, double xc, double yc, double rx, double ry, double a1, double a2, int r, int g, int b, int a, int style, int strokePtr, int lw), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var col = globalThis.__iupRGBA(r, g, b, a);
  ctx.beginPath();
  if (style == 0) ctx.moveTo(xc, yc);
  ctx.ellipse(xc, yc, rx, ry, 0, a1, a2, false);
  if (style == 0) {
    ctx.setLineDash([]);
    ctx.closePath();
    ctx.fillStyle = col;
    ctx.fill();
  } else {
    globalThis.__iupDash(ctx, strokePtr, lw);
    ctx.strokeStyle = col;
    ctx.stroke();
  }
})

EM_JS(void, iupwasmJsDrawEllipse, (int cid, double xc, double yc, double rx, double ry, int r, int g, int b, int a, int style, int strokePtr, int lw), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var col = globalThis.__iupRGBA(r, g, b, a);
  ctx.beginPath();
  ctx.ellipse(xc, yc, rx, ry, 0, 0, 2 * Math.PI, false);
  if (style == 0) {
    ctx.setLineDash([]);
    ctx.fillStyle = col;
    ctx.fill();
  } else {
    globalThis.__iupDash(ctx, strokePtr, lw);
    ctx.strokeStyle = col;
    ctx.stroke();
  }
})

EM_JS(void, iupwasmJsDrawPolygon, (int cid, int ptr, int count, int r, int g, int b, int a, int style, int strokePtr, int lw), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var col = globalThis.__iupRGBA(r, g, b, a);
  ctx.beginPath();
  ctx.moveTo(HEAP32[ptr >> 2], HEAP32[(ptr >> 2) + 1]);
  for (var i = 1; i < count; i++)
    ctx.lineTo(HEAP32[(ptr >> 2) + 2 * i], HEAP32[(ptr >> 2) + 2 * i + 1]);
  ctx.closePath();
  if (style == 0) {
    ctx.setLineDash([]);
    ctx.fillStyle = col;
    ctx.fill();
  } else {
    globalThis.__iupDash(ctx, strokePtr, lw);
    ctx.strokeStyle = col;
    ctx.stroke();
  }
})

EM_JS(void, iupwasmJsDrawPixel, (int cid, int x, int y, int r, int g, int b, int a), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  ctx.setLineDash([]);
  ctx.fillStyle = globalThis.__iupRGBA(r, g, b, a);
  ctx.fillRect(x, y, 1, 1);
})

EM_JS(void, iupwasmJsDrawRoundRect, (int cid, int x1, int y1, int x2, int y2, int radius, int r, int g, int b, int a, int style, int strokePtr, int lw), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var col = globalThis.__iupRGBA(r, g, b, a);
  var w = x2 - x1 + 1, h = y2 - y1 + 1;
  var rr = Math.min(radius, w / 2, h / 2);
  ctx.beginPath();
  if (ctx.roundRect) ctx.roundRect(x1, y1, w, h, rr);
  else {
    ctx.moveTo(x1 + rr, y1);
    ctx.arcTo(x2 + 1, y1, x2 + 1, y2 + 1, rr);
    ctx.arcTo(x2 + 1, y2 + 1, x1, y2 + 1, rr);
    ctx.arcTo(x1, y2 + 1, x1, y1, rr);
    ctx.arcTo(x1, y1, x2 + 1, y1, rr);
    ctx.closePath();
  }
  if (style == 0) {
    ctx.setLineDash([]);
    ctx.fillStyle = col;
    ctx.fill();
  } else {
    globalThis.__iupDash(ctx, strokePtr, lw);
    ctx.strokeStyle = col;
    ctx.stroke();
  }
})

EM_JS(void, iupwasmJsDrawBezier, (int cid, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, int r, int g, int b, int a, int strokePtr, int lw), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  globalThis.__iupDash(ctx, strokePtr, lw);
  ctx.strokeStyle = globalThis.__iupRGBA(r, g, b, a);
  ctx.beginPath();
  ctx.moveTo(x1, y1);
  ctx.bezierCurveTo(x2, y2, x3, y3, x4, y4);
  ctx.stroke();
})

EM_JS(void, iupwasmJsDrawQuadBezier, (int cid, int x1, int y1, int x2, int y2, int x3, int y3, int r, int g, int b, int a, int strokePtr, int lw), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  globalThis.__iupDash(ctx, strokePtr, lw);
  ctx.strokeStyle = globalThis.__iupRGBA(r, g, b, a);
  ctx.beginPath();
  ctx.moveTo(x1, y1);
  ctx.quadraticCurveTo(x2, y2, x3, y3);
  ctx.stroke();
})

EM_JS(void, iupwasmJsDrawLinearGradient, (int cid, int x1, int y1, int x2, int y2, double angle, int rgbaPtr, int offsPtr, int count), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var w = x2 - x1, h = y2 - y1;
  var rad = angle * Math.PI / 180;
  var x0 = x1 + w / 2 - (w * Math.cos(rad)) / 2;
  var y0 = y1 + h / 2 - (h * Math.sin(rad)) / 2;
  var x3 = x1 + w / 2 + (w * Math.cos(rad)) / 2;
  var y3 = y1 + h / 2 + (h * Math.sin(rad)) / 2;
  var grad = ctx.createLinearGradient(x0, y0, x3, y3);
  for (var i = 0; i < count; i++)
    grad.addColorStop(HEAPF32[(offsPtr >> 2) + i],
      globalThis.__iupRGBA(HEAPU8[rgbaPtr+i*4], HEAPU8[rgbaPtr+i*4+1], HEAPU8[rgbaPtr+i*4+2], HEAPU8[rgbaPtr+i*4+3]));
  ctx.setLineDash([]);
  ctx.fillStyle = grad;
  ctx.fillRect(x1, y1, w, h);
})

EM_JS(void, iupwasmJsDrawRadialGradient, (int cid, int cx, int cy, int radius, int rgbaPtr, int offsPtr, int count), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var grad = ctx.createRadialGradient(cx, cy, 0, cx, cy, radius);
  for (var i = 0; i < count; i++)
    grad.addColorStop(HEAPF32[(offsPtr >> 2) + i],
      globalThis.__iupRGBA(HEAPU8[rgbaPtr+i*4], HEAPU8[rgbaPtr+i*4+1], HEAPU8[rgbaPtr+i*4+2], HEAPU8[rgbaPtr+i*4+3]));
  ctx.setLineDash([]);
  ctx.fillStyle = grad;
  ctx.beginPath();
  ctx.arc(cx, cy, radius, 0, 2 * Math.PI);
  ctx.fill();
})

EM_JS(void, iupwasmJsDrawText, (int cid, const char* txt, int x, int y, int w, int h, int r, int g, int b, int a, const char* css, int flags, double orient), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var s = UTF8ToString(txt);
  var f = UTF8ToString(css);
  ctx.save();
  ctx.setLineDash([]);
  ctx.font = f;
  ctx.fillStyle = globalThis.__iupRGBA(r, g, b, a);
  ctx.textBaseline = "middle";  /* box top stays at y, glyph centered in its line box */
  var m = f.match(/([0-9]+)px/);
  var lh = Math.round((m ? parseInt(m[1]) : 14) * 1.25);
  var lines = s.split("\n");
  if (flags & 0x4) {  /* WRAP */
    var wrapped = [];
    for (var i = 0; i < lines.length; i++) {
      var words = lines[i].split(" "), line = "";
      for (var j = 0; j < words.length; j++) {
        var test = line ? line + " " + words[j] : words[j];
        if (line && ctx.measureText(test).width > w) { wrapped.push(line); line = words[j]; }
        else line = test;
        while (line.length > 1 && ctx.measureText(line).width > w) {
          var cut = line.length - 1;
          while (cut > 1 && ctx.measureText(line.substring(0, cut)).width > w) cut--;
          wrapped.push(line.substring(0, cut));
          line = line.substring(cut);
        }
      }
      wrapped.push(line);
    }
    lines = wrapped;
  } else if (flags & 0x8) {  /* ELLIPSIS */
    for (var i = 0; i < lines.length; i++) {
      var t = lines[i];
      if (ctx.measureText(t).width <= w) continue;
      var n = t.length;
      while (n > 0 && ctx.measureText(t.substring(0, n) + "\u2026").width > w) n--;
      lines[i] = t.substring(0, n) + "\u2026";
    }
  }
  var lw = w, lhh = h;
  if (orient && (flags & 0x20)) {  /* LAYOUTCENTER: rotate the unrotated layout box around the icon center */
    lw = 0;
    for (var i = 0; i < lines.length; i++) lw = Math.max(lw, Math.ceil(ctx.measureText(lines[i]).width));
    lhh = lines.length * lh;
  }
  if (flags & 0x10) { ctx.beginPath(); ctx.rect(x, y, w, h); ctx.clip(); }  /* CLIP */
  if (orient) {
    var cx = x, cy = y;
    if (flags & 0x20) { ctx.translate((w - lw) / 2, (h - lhh) / 2); cx = x + lw / 2; cy = y + lhh / 2; }
    ctx.translate(cx, cy); ctx.rotate(-orient * Math.PI / 180); ctx.translate(-cx, -cy);
  }
  var ax = x, align = "left";
  if (flags & 0x1) { ax = x + lw / 2; align = "center"; }      /* CENTER */
  else if (flags & 0x2) { ax = x + lw; align = "right"; }      /* RIGHT */
  ctx.textAlign = align;
  for (var i = 0; i < lines.length; i++)
    ctx.fillText(lines[i], ax, y + i * lh + lh / 2);
  ctx.restore();
})

EM_JS(void, iupwasmJsDrawImage, (int cid, int imgId, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int smooth, int opacity), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var im = globalThis.__iupImg && globalThis.__iupImg.map[imgId];
  if (!im || !im.canvas) return;
  var old = ctx.imageSmoothingEnabled;
  var oldAlpha = ctx.globalAlpha;
  ctx.imageSmoothingEnabled = !!smooth;
  ctx.globalAlpha = oldAlpha * (opacity / 255);
  ctx.drawImage(im.canvas, sx, sy, sw, sh, x, y, w, h);
  ctx.globalAlpha = oldAlpha;
  ctx.imageSmoothingEnabled = old;
})

EM_JS(void, iupwasmJsClipRect, (int cid, int x, int y, int w, int h), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var m = ctx.__iupTransform || [1, 0, 0, 1, 0, 0];
  ctx.restore();   /* drop any previous clip before reclipping */
  ctx.setTransform(m[0], m[1], m[2], m[3], m[4], m[5]);
  ctx.save();
  ctx.beginPath();
  ctx.rect(x, y, w, h);
  ctx.clip();
})

EM_JS(void, iupwasmJsClipRoundRect, (int cid, int x1, int y1, int x2, int y2, int radius), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var m = ctx.__iupTransform || [1, 0, 0, 1, 0, 0];
  ctx.restore();
  ctx.setTransform(m[0], m[1], m[2], m[3], m[4], m[5]);
  ctx.save();
  var w = x2 - x1 + 1, h = y2 - y1 + 1;
  var rr = Math.min(radius, w / 2, h / 2);
  ctx.beginPath();
  if (ctx.roundRect) ctx.roundRect(x1, y1, w, h, rr);
  else {
    ctx.moveTo(x1 + rr, y1);
    ctx.arcTo(x2 + 1, y1, x2 + 1, y2 + 1, rr);
    ctx.arcTo(x2 + 1, y2 + 1, x1, y2 + 1, rr);
    ctx.arcTo(x1, y2 + 1, x1, y1, rr);
    ctx.arcTo(x1, y1, x2 + 1, y1, rr);
    ctx.closePath();
  }
  ctx.clip();
})

EM_JS(void, iupwasmJsResetClip, (int cid), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var m = ctx.__iupTransform || [1, 0, 0, 1, 0, 0];
  ctx.restore();
  ctx.setTransform(m[0], m[1], m[2], m[3], m[4], m[5]);
  ctx.save();
})

EM_JS(void, iupwasmJsBeginLayer, (int cid), {
  var cv = globalThis.__iupCanvasOf(cid); if (!cv) return;
  var oc;
  if (typeof document !== 'undefined') {
    oc = document.createElement('canvas');
    oc.width = cv.width || 1;
    oc.height = cv.height || 1;
  } else
    oc = new OffscreenCanvas(cv.width || 1, cv.height || 1);
  var ctx = oc.getContext("2d");
  ctx.__iupTransform = [1, 0, 0, 1, 0, 0];
  ctx.save();
  var stack = globalThis.__iupLayers[cid] || (globalThis.__iupLayers[cid] = []);
  stack.push(oc);
})

EM_JS(void, iupwasmJsEndLayer, (int cid, int alpha), {
  var stack = globalThis.__iupLayers[cid];
  if (!stack || !stack.length) return;
  var oc = stack.pop();
  if (!stack.length) delete globalThis.__iupLayers[cid];
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  ctx.save();
  ctx.setTransform(1, 0, 0, 1, 0, 0);
  ctx.globalAlpha = alpha / 255;
  ctx.drawImage(oc, 0, 0);
  ctx.restore();
})

EM_JS(void, iupwasmJsLayerClear, (int cid), {
  delete globalThis.__iupLayers[cid];
})

EM_JS(int, iupwasmJsCanvasGetImageData, (int cid, int ptr, int w, int h), {
  var cv = globalThis.__iupBaseCanvasOf(cid); if (!cv) return 0;
  var ctx = cv.getContext("2d");
  var img = ctx.getImageData(0, 0, w, h);
  HEAPU8.set(img.data, ptr);
  return 1;
})


IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = calloc(1, sizeof(IdrawCanvas));
  dc->ih = ih;
  dc->cid = iupwasmIdOf(ih);

  iupwasmJsCanvasInit();
  dc->w = iupwasmJsCanvasClientW(dc->cid);
  dc->h = iupwasmJsCanvasClientH(dc->cid);
  iupwasmJsCanvasReset(dc->cid, dc->w, dc->h);

  iupAttribSet(ih, "DRAWDRIVER", "CANVAS2D");
  return dc;
}

IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  if (!dc)
    return;

  if (dc->layers)
    iupwasmJsLayerClear(dc->cid);

  while (dc->layers)
  {
    IwasmDrawLayer* layer = dc->layers;
    dc->layers = layer->next;
    free(layer);
  }

  free(dc);
}

IUP_SDK_API void iupdrvDrawSetTransform(IdrawCanvas* dc, const IupDrawMatrix* matrix)
{
  iupwasmJsSetTransform(dc->cid, matrix->a, matrix->b, matrix->c, matrix->d, matrix->e, matrix->f);
}

IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  if (!dc) return;
  dc->w = iupwasmJsCanvasClientW(dc->cid);
  dc->h = iupwasmJsCanvasClientH(dc->cid);
}

EM_JS(void, iupwasmJsCanvasBlit, (int cid), {
  if (typeof document !== 'undefined') return;
  var local = globalThis.__iupLocal && globalThis.__iupLocal[cid];
  if (!local || !local.width || !local.height) return;
  var bmp = local.transferToImageBitmap();
  self.postMessage({ __iupBlit: 1, cid: cid, bitmap: bmp }, [bmp]);
})

IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc)
{
  if (dc)
    iupwasmJsCanvasBlit(dc->cid);
}

IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int* w, int* h)
{
  if (w) *w = dc ? dc->w : 0;
  if (h) *h = dc ? dc->h : 0;
}

#define IUPWASM_STROKE_DATA (IUP_DRAW_MAX_DASHES + 5)

static void wasmDrawStrokeArray(IdrawCanvas* dc, int style, double* stroke_data)
{
  IupDrawStroke stroke;
  int i;

  iupDrawGetStroke(dc->ih, style, &stroke);

  stroke_data[0] = stroke.cap;
  stroke_data[1] = stroke.join;
  stroke_data[2] = IUP_DRAW_MITER_LIMIT;
  stroke_data[3] = stroke.dash_offset;
  stroke_data[4] = stroke.dash_count;
  for (i = 0; i < stroke.dash_count; i++)
    stroke_data[5 + i] = stroke.dashes[i];
}

IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  double stroke_data[IUPWASM_STROKE_DATA];
  wasmDrawStrokeArray(dc, style, stroke_data);
  iupwasmJsDrawLine(dc->cid, x1, y1, x2, y2, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), (int)(intptr_t)stroke_data, line_width);
}

IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  double stroke_data[IUPWASM_STROKE_DATA];
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  wasmDrawStrokeArray(dc, style, stroke_data);
  iupwasmJsDrawRect(dc->cid, x1, y1, x2, y2, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), style, (int)(intptr_t)stroke_data, line_width);
}

IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  double xc, yc, w, h, s1, s2, tmp;
  double stroke_data[IUPWASM_STROKE_DATA];

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  w = x2 - x1;
  h = y2 - y1;
  xc = x1 + w / 2.0;
  yc = y1 + h / 2.0;

  while (a2 < a1)
    a2 += 360;

  s1 = -a1 * IUP_DEG2RAD;
  s2 = -a2 * IUP_DEG2RAD;
  if (s1 > s2) { tmp = s1; s1 = s2; s2 = tmp; }

  wasmDrawStrokeArray(dc, style, stroke_data);
  iupwasmJsDrawArc(dc->cid, xc, yc, w / 2.0, h / 2.0, s1, s2, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), style, (int)(intptr_t)stroke_data, line_width);
}

IUP_SDK_API void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  double xc, yc, w, h;
  double stroke_data[IUPWASM_STROKE_DATA];

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  w = x2 - x1;
  h = y2 - y1;
  xc = x1 + w / 2.0;
  yc = y1 + h / 2.0;

  wasmDrawStrokeArray(dc, style, stroke_data);
  iupwasmJsDrawEllipse(dc->cid, xc, yc, w / 2.0, h / 2.0, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), style, (int)(intptr_t)stroke_data, line_width);
}

IUP_SDK_API void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  double stroke_data[IUPWASM_STROKE_DATA];
  if (!points || count < 2)
    return;
  wasmDrawStrokeArray(dc, style, stroke_data);
  iupwasmJsDrawPolygon(dc->cid, (int)(intptr_t)points, count, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), style, (int)(intptr_t)stroke_data, line_width);
}

IUP_SDK_API void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
{
  iupwasmJsDrawPixel(dc->cid, x, y, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color));
}

IUP_SDK_API void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius, long color, int style, int line_width)
{
  double stroke_data[IUPWASM_STROKE_DATA];
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  wasmDrawStrokeArray(dc, style, stroke_data);
  iupwasmJsDrawRoundRect(dc->cid, x1, y1, x2, y2, corner_radius, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), style, (int)(intptr_t)stroke_data, line_width);
}

IUP_SDK_API void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
  double stroke_data[IUPWASM_STROKE_DATA];
  wasmDrawStrokeArray(dc, style, stroke_data);
  iupwasmJsDrawBezier(dc->cid, x1, y1, x2, y2, x3, y3, x4, y4, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), (int)(intptr_t)stroke_data, line_width);
}

IUP_SDK_API void iupdrvDrawQuadraticBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, long color, int style, int line_width)
{
  double stroke_data[IUPWASM_STROKE_DATA];
  wasmDrawStrokeArray(dc, style, stroke_data);
  iupwasmJsDrawQuadBezier(dc->cid, x1, y1, x2, y2, x3, y3, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), (int)(intptr_t)stroke_data, line_width);
}

static void wasmDrawGradientArrays(const long* colors, const float* offsets, int count, unsigned char* rgba, float* offs)
{
  int i;
  for (i = 0; i < count; i++)
  {
    rgba[i*4+0] = iupDrawRed(colors[i]);
    rgba[i*4+1] = iupDrawGreen(colors[i]);
    rgba[i*4+2] = iupDrawBlue(colors[i]);
    rgba[i*4+3] = iupDrawAlpha(colors[i]);
    offs[i] = offsets[i];
  }
}

IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, const long* colors, const float* offsets, int count)
{
  unsigned char rgba[IUP_GRADIENT_MAX_STOPS * 4];
  float offs[IUP_GRADIENT_MAX_STOPS];
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  wasmDrawGradientArrays(colors, offsets, count, rgba, offs);
  iupwasmJsDrawLinearGradient(dc->cid, x1, y1, x2, y2, angle, (int)(intptr_t)rgba, (int)(intptr_t)offs, count);
}

IUP_SDK_API void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, const long* colors, const float* offsets, int count)
{
  unsigned char rgba[IUP_GRADIENT_MAX_STOPS * 4];
  float offs[IUP_GRADIENT_MAX_STOPS];
  wasmDrawGradientArrays(colors, offsets, count, rgba, offs);
  iupwasmJsDrawRadialGradient(dc->cid, cx, cy, radius, (int)(intptr_t)rgba, (int)(intptr_t)offs, count);
}

IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  char css[1100];
  char* buf = NULL;

  if (len >= 0)
  {
    buf = (char*)malloc(len + 1);
    memcpy(buf, text, len);
    buf[len] = 0;
    text = buf;
  }

  iupwasmFontToCss(font, css, sizeof(css));
  iupwasmJsDrawText(dc->cid, text, x, y, w, h, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), css, flags, text_orientation);

  if (buf) free(buf);
}

IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, long tint, int opacity, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int quality)
{
  int img_w, img_h, bpp;
  void* handle = iupImageGetImageTint(name, dc->ih, make_inactive, bgcolor, tint);
  if (!handle)
    return;

  iupdrvImageGetInfo(handle, &img_w, &img_h, &bpp);

  if (sw <= 0 || sh <= 0)
  {
    sx = 0;
    sy = 0;
    sw = img_w;
    sh = img_h;
  }
  if (w <= 0) w = sw;
  if (h <= 0) h = sh;

  iupwasmJsDrawImage(dc->cid, (int)(intptr_t)handle, x, y, w, h, sx, sy, sw, sh, quality != IUP_DRAW_IMAGE_NEAREST, opacity);
}

static void iupwasmDrawStoreClip(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
}

IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  iupwasmJsResetClip(dc->cid);
  iupwasmDrawStoreClip(dc, 0, 0, 0, 0);
}

IUP_SDK_API int iupdrvDrawBeginLayer(IdrawCanvas* dc, int alpha)
{
  IwasmDrawLayer* layer;
  (void)alpha;

  if (!dc)
    return 0;

  layer = calloc(1, sizeof(IwasmDrawLayer));
  if (!layer)
    return 0;
  layer->clip_x1 = dc->clip_x1;
  layer->clip_y1 = dc->clip_y1;
  layer->clip_x2 = dc->clip_x2;
  layer->clip_y2 = dc->clip_y2;
  layer->next = dc->layers;
  dc->layers = layer;

  iupwasmJsBeginLayer(dc->cid);
  iupwasmDrawStoreClip(dc, 0, 0, 0, 0);
  return 1;
}

IUP_SDK_API void iupdrvDrawEndLayer(IdrawCanvas* dc, int alpha)
{
  IwasmDrawLayer* layer;

  if (!dc || !dc->layers)
    return;

  layer = dc->layers;
  dc->layers = layer->next;

  iupwasmJsEndLayer(dc->cid, alpha);
  iupwasmDrawStoreClip(dc, layer->clip_x1, layer->clip_y1, layer->clip_x2, layer->clip_y2);

  free(layer);
}

IUP_SDK_API void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  iupwasmJsClipRect(dc->cid, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  iupwasmDrawStoreClip(dc, x1, y1, x2, y2);
}

IUP_SDK_API void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius)
{
  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  iupwasmJsClipRoundRect(dc->cid, x1, y1, x2, y2, corner_radius);
  iupwasmDrawStoreClip(dc, x1, y1, x2, y2);
}

IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int* x1, int* y1, int* x2, int* y2)
{
  if (x1) *x1 = dc->clip_x1;
  if (y1) *y1 = dc->clip_y1;
  if (x2) *x2 = dc->clip_x2;
  if (y2) *y2 = dc->clip_y2;
}

IUP_SDK_API void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  double stroke_data[IUPWASM_STROKE_DATA];
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  wasmDrawStrokeArray(dc, IUP_DRAW_FILL, stroke_data);
  iupwasmJsDrawRect(dc->cid, x1, y1, x2, y2, 0, 0, 255, 153, IUP_DRAW_FILL, (int)(intptr_t)stroke_data, 1);
}

IUP_SDK_API void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  iupdrvDrawRectangle(dc, x1, y1, x2, y2, iupDrawColor(0, 0, 0, 224), IUP_DRAW_STROKE_DOT, 1);
}

IUP_SDK_API int iupdrvDrawGetImageData(IdrawCanvas* dc, unsigned char* data)
{
  if (!dc || !data)
    return 0;
  return iupwasmJsCanvasGetImageData(dc->cid, (int)(intptr_t)data, dc->w, dc->h);
}

typedef char iupwasmPathSegLayoutCheck[(sizeof(IupPathSeg) == 72 && offsetof(IupPathSeg, op) == 0 && offsetof(IupPathSeg, x1) == 8 && offsetof(IupPathSeg, a1) == 56) ? 1 : -1];

EM_JS(void, iupwasmJsDrawPathFill, (int cid, int segsPtr, int count, int sourceType, long color, int x1, int y1, int x2, int y2, float angle, int cx, int cy, int radius, int rgbaPtr, int offsPtr, int gradCount, int rule), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var p = new Path2D();
  for (var i = 0; i < count; i++)
  {
    var base = (segsPtr >> 3) + i * 9 + 1;
    var op = HEAPU8[segsPtr + i * 72];
    switch (op)
    {
    case 0:
      p.moveTo(HEAPF64[base], HEAPF64[base + 1]);
      break;
    case 1:
      p.lineTo(HEAPF64[base], HEAPF64[base + 1]);
      break;
    case 2:
      p.bezierCurveTo(HEAPF64[base], HEAPF64[base + 1], HEAPF64[base + 2], HEAPF64[base + 3], HEAPF64[base + 4], HEAPF64[base + 5]);
      break;
    case 3:
      p.quadraticCurveTo(HEAPF64[base], HEAPF64[base + 1], HEAPF64[base + 2], HEAPF64[base + 3]);
      break;
    case 4:
    {
      var centX = HEAPF64[base], centY = HEAPF64[base + 1];
      var rx = HEAPF64[base + 2], ry = HEAPF64[base + 3];
      var sa = HEAPF64[base + 6];
      var ea = HEAPF64[base + 7];
      var span = ea - sa;
      while (span < 0) span += 360;
      while (span > 360) span -= 360;
      if (span >= 0.01)
        p.ellipse(centX, centY, rx, ry, 0, -sa * Math.PI / 180, -(sa + span) * Math.PI / 180, true);
      break;
    }
    case 5:
      p.closePath();
      break;
    }
  }
  if (sourceType == 0)
    ctx.fillStyle = globalThis.__iupRGBA((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (~(color >> 24)) & 0xFF);
  else if (sourceType == 1)
  {
    var w = x2 - x1, h = y2 - y1;
    var rad = angle * Math.PI / 180;
    var x0 = x1 + w / 2 - (w * Math.cos(rad)) / 2;
    var y0 = y1 + h / 2 - (h * Math.sin(rad)) / 2;
    var x3 = x1 + w / 2 + (w * Math.cos(rad)) / 2;
    var y3 = y1 + h / 2 + (h * Math.sin(rad)) / 2;
    var grad = ctx.createLinearGradient(x0, y0, x3, y3);
    for (var k = 0; k < gradCount; k++)
      grad.addColorStop(HEAPF32[(offsPtr >> 2) + k],
        globalThis.__iupRGBA(HEAPU8[rgbaPtr+k*4], HEAPU8[rgbaPtr+k*4+1], HEAPU8[rgbaPtr+k*4+2], HEAPU8[rgbaPtr+k*4+3]));
    ctx.fillStyle = grad;
  }
  else if (sourceType == 2)
  {
    var grad = ctx.createRadialGradient(cx, cy, 0, cx, cy, radius);
    for (var k = 0; k < gradCount; k++)
      grad.addColorStop(HEAPF32[(offsPtr >> 2) + k],
        globalThis.__iupRGBA(HEAPU8[rgbaPtr+k*4], HEAPU8[rgbaPtr+k*4+1], HEAPU8[rgbaPtr+k*4+2], HEAPU8[rgbaPtr+k*4+3]));
    ctx.fillStyle = grad;
  }
  ctx.setLineDash([]);
  ctx.fill(p, rule == 1 ? "evenodd" : "nonzero");
})

EM_JS(void, iupwasmJsDrawPathStroke, (int cid, int segsPtr, int count, int sourceType, long color, int x1, int y1, int x2, int y2, float angle, int cx, int cy, int radius, int rgbaPtr, int offsPtr, int gradCount, int strokePtr, int lw), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var p = new Path2D();
  for (var i = 0; i < count; i++)
  {
    var base = (segsPtr >> 3) + i * 9 + 1;
    var op = HEAPU8[segsPtr + i * 72];
    switch (op)
    {
    case 0:
      p.moveTo(HEAPF64[base], HEAPF64[base + 1]);
      break;
    case 1:
      p.lineTo(HEAPF64[base], HEAPF64[base + 1]);
      break;
    case 2:
      p.bezierCurveTo(HEAPF64[base], HEAPF64[base + 1], HEAPF64[base + 2], HEAPF64[base + 3], HEAPF64[base + 4], HEAPF64[base + 5]);
      break;
    case 3:
      p.quadraticCurveTo(HEAPF64[base], HEAPF64[base + 1], HEAPF64[base + 2], HEAPF64[base + 3]);
      break;
    case 4:
    {
      var centX = HEAPF64[base], centY = HEAPF64[base + 1];
      var rx = HEAPF64[base + 2], ry = HEAPF64[base + 3];
      var sa = HEAPF64[base + 6];
      var ea = HEAPF64[base + 7];
      var span = ea - sa;
      while (span < 0) span += 360;
      while (span > 360) span -= 360;
      if (span >= 0.01)
        p.ellipse(centX, centY, rx, ry, 0, -sa * Math.PI / 180, -(sa + span) * Math.PI / 180, true);
      break;
    }
    case 5:
      p.closePath();
      break;
    }
  }
  globalThis.__iupDash(ctx, strokePtr, lw);
  if (sourceType == 0)
    ctx.strokeStyle = globalThis.__iupRGBA((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (~(color >> 24)) & 0xFF);
  else if (sourceType == 1)
  {
    var w = x2 - x1, h = y2 - y1;
    var rad = angle * Math.PI / 180;
    var x0 = x1 + w / 2 - (w * Math.cos(rad)) / 2;
    var y0 = y1 + h / 2 - (h * Math.sin(rad)) / 2;
    var x3 = x1 + w / 2 + (w * Math.cos(rad)) / 2;
    var y3 = y1 + h / 2 + (h * Math.sin(rad)) / 2;
    var grad = ctx.createLinearGradient(x0, y0, x3, y3);
    for (var k = 0; k < gradCount; k++)
      grad.addColorStop(HEAPF32[(offsPtr >> 2) + k],
        globalThis.__iupRGBA(HEAPU8[rgbaPtr+k*4], HEAPU8[rgbaPtr+k*4+1], HEAPU8[rgbaPtr+k*4+2], HEAPU8[rgbaPtr+k*4+3]));
    ctx.strokeStyle = grad;
  }
  else if (sourceType == 2)
  {
    var grad = ctx.createRadialGradient(cx, cy, 0, cx, cy, radius);
    for (var k = 0; k < gradCount; k++)
      grad.addColorStop(HEAPF32[(offsPtr >> 2) + k],
        globalThis.__iupRGBA(HEAPU8[rgbaPtr+k*4], HEAPU8[rgbaPtr+k*4+1], HEAPU8[rgbaPtr+k*4+2], HEAPU8[rgbaPtr+k*4+3]));
    ctx.strokeStyle = grad;
  }
  ctx.stroke(p);
})

EM_JS(void, iupwasmJsClipPath, (int cid, int segsPtr, int count, int rule), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var m = ctx.__iupTransform || [1, 0, 0, 1, 0, 0];
  ctx.restore();
  ctx.setTransform(m[0], m[1], m[2], m[3], m[4], m[5]);
  ctx.save();
  var p = new Path2D();
  for (var i = 0; i < count; i++)
  {
    var base = (segsPtr >> 3) + i * 9 + 1;
    var op = HEAPU8[segsPtr + i * 72];
    switch (op)
    {
    case 0:
      p.moveTo(HEAPF64[base], HEAPF64[base + 1]);
      break;
    case 1:
      p.lineTo(HEAPF64[base], HEAPF64[base + 1]);
      break;
    case 2:
      p.bezierCurveTo(HEAPF64[base], HEAPF64[base + 1], HEAPF64[base + 2], HEAPF64[base + 3], HEAPF64[base + 4], HEAPF64[base + 5]);
      break;
    case 3:
      p.quadraticCurveTo(HEAPF64[base], HEAPF64[base + 1], HEAPF64[base + 2], HEAPF64[base + 3]);
      break;
    case 4:
    {
      var centX = HEAPF64[base], centY = HEAPF64[base + 1];
      var rx = HEAPF64[base + 2], ry = HEAPF64[base + 3];
      var sa = HEAPF64[base + 6];
      var ea = HEAPF64[base + 7];
      var span = ea - sa;
      while (span < 0) span += 360;
      while (span > 360) span -= 360;
      if (span >= 0.01)
        p.ellipse(centX, centY, rx, ry, 0, -sa * Math.PI / 180, -(sa + span) * Math.PI / 180, true);
      break;
    }
    case 5:
      p.closePath();
      break;
    }
  }
  ctx.clip(p, rule == 1 ? "evenodd" : "nonzero");
})

IUP_SDK_API int iupdrvCanvasGetImageData(Ihandle* ih, unsigned char* data, int w, int h)
{
  int cid = iupwasmIdOf(ih);
  if (!cid || !data)
    return 0;
  return iupwasmJsCanvasGetImageData(cid, (int)(intptr_t)data, w, h);
}

static void wasmDrawSourceArrays(const IupDrawSource* src, unsigned char* rgba, float* offs)
{
  int i;
  for (i = 0; i < src->count; i++)
  {
    rgba[i*4+0] = iupDrawRed(src->colors[i]);
    rgba[i*4+1] = iupDrawGreen(src->colors[i]);
    rgba[i*4+2] = iupDrawBlue(src->colors[i]);
    rgba[i*4+3] = iupDrawAlpha(src->colors[i]);
    offs[i] = src->offsets[i];
  }
}

IUP_SDK_API void iupdrvDrawPathFill(IdrawCanvas* dc, const IupPathSeg* segs, int count, const IupDrawSource* src, int rule)
{
  unsigned char rgba[IUP_GRADIENT_MAX_STOPS * 4];
  float offs[IUP_GRADIENT_MAX_STOPS];
  if (src->type != IUP_SOURCE_SOLID)
    wasmDrawSourceArrays(src, rgba, offs);
  iupwasmJsDrawPathFill(dc->cid, (int)(intptr_t)segs, count, src->type, src->color,
    src->x1, src->y1, src->x2, src->y2, src->angle,
    src->cx, src->cy, src->radius,
    (int)(intptr_t)rgba, (int)(intptr_t)offs, src->count, rule);
}

IUP_SDK_API void iupdrvDrawPathStroke(IdrawCanvas* dc, const IupPathSeg* segs, int count, const IupDrawSource* src, int style, int line_width)
{
  unsigned char rgba[IUP_GRADIENT_MAX_STOPS * 4];
  float offs[IUP_GRADIENT_MAX_STOPS];
  double stroke_data[IUPWASM_STROKE_DATA];
  if (src->type != IUP_SOURCE_SOLID)
    wasmDrawSourceArrays(src, rgba, offs);
  wasmDrawStrokeArray(dc, style, stroke_data);
  iupwasmJsDrawPathStroke(dc->cid, (int)(intptr_t)segs, count, src->type, src->color,
    src->x1, src->y1, src->x2, src->y2, src->angle,
    src->cx, src->cy, src->radius,
    (int)(intptr_t)rgba, (int)(intptr_t)offs, src->count, (int)(intptr_t)stroke_data, line_width);
}

IUP_SDK_API void iupdrvDrawSetClipPath(IdrawCanvas* dc, const IupPathSeg* segs, int count, int rule)
{
  int x1, y1, x2, y2;
  iupwasmJsClipPath(dc->cid, (int)(intptr_t)segs, count, rule);
  iupDrawPathGetBBox(segs, count, &x1, &y1, &x2, &y2);
  iupwasmDrawStoreClip(dc, x1, y1, x2, y2);
}
