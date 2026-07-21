/** \file
 * \brief WebAssembly Draw API
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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


struct _IdrawCanvas
{
  Ihandle* ih;
  int cid;          /* canvas element id */
  int w, h;
};

EM_JS(void, iupwasmJsCanvasInit, (void), {
  if (globalThis.__iupRGBA) return;
  globalThis.__iupRGBA = function(r, g, b, a) {
    return "rgba(" + r + "," + g + "," + b + "," + (a / 255) + ")";
  };
  globalThis.__iupDash = function(ctx, style, lw) {
    /* IUP_DRAW style: 1=STROKE 2=DASH 3=DOT 4=DASH_DOT 5=DASH_DOT_DOT */
    if (style == 2) ctx.setLineDash([9, 3]);
    else if (style == 3) ctx.setLineDash([1, 2]);
    else if (style == 4) ctx.setLineDash([7, 3, 1, 3]);
    else if (style == 5) ctx.setLineDash([7, 3, 1, 3, 1, 3]);
    else ctx.setLineDash([]);
    ctx.lineWidth = lw < 1 ? 1 : lw;
  };
  globalThis.__iupCanvasOf = function(cid) {
    /* worker-local OffscreenCanvas, blitted to the on-screen canvas on flush (survives a blocking modal) */
    if (typeof document === 'undefined') {
      globalThis.__iupLocal = globalThis.__iupLocal || {};
      if (!globalThis.__iupLocal[cid]) globalThis.__iupLocal[cid] = new OffscreenCanvas(1, 1);
      return globalThis.__iupLocal[cid];
    }
    var el = globalThis.__iup.els[cid];
    return el && el.__iupCanvas ? el.__iupCanvas : el;
  };
  globalThis.__iupCtx = function(cid) {
    var cv = globalThis.__iupCanvasOf(cid);
    return cv ? cv.getContext("2d") : null;
  };
})

/* setting width/height clears the canvas; save() is the clip baseline */
EM_JS(void, iupwasmJsCanvasReset, (int cid, int w, int h), {
  var el = globalThis.__iupCanvasOf(cid); if (!el) return;
  el.width = w;
  el.height = h;
  var ctx = el.getContext("2d");
  ctx.setLineDash([]);
  ctx.save();
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

EM_JS(void, iupwasmJsDrawLine, (int cid, int x1, int y1, int x2, int y2, int r, int g, int b, int a, int style, int lw), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  globalThis.__iupDash(ctx, style, lw);
  ctx.strokeStyle = globalThis.__iupRGBA(r, g, b, a);
  var o = (ctx.lineWidth % 2) ? 0.5 : 0;
  ctx.beginPath();
  ctx.moveTo(x1 + o, y1 + o);
  ctx.lineTo(x2 + o, y2 + o);
  ctx.stroke();
})

EM_JS(void, iupwasmJsDrawRect, (int cid, int x1, int y1, int x2, int y2, int r, int g, int b, int a, int style, int lw), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var col = globalThis.__iupRGBA(r, g, b, a);
  if (style == 0) {  /* FILL */
    ctx.setLineDash([]);
    ctx.fillStyle = col;
    ctx.fillRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  } else {
    globalThis.__iupDash(ctx, style, lw);
    ctx.strokeStyle = col;
    var o = (ctx.lineWidth % 2) ? 0.5 : 0;
    ctx.strokeRect(x1 + o, y1 + o, x2 - x1, y2 - y1);
  }
})

EM_JS(void, iupwasmJsDrawArc, (int cid, double xc, double yc, double rx, double ry, double a1, double a2, int r, int g, int b, int a, int style, int lw), {
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
    globalThis.__iupDash(ctx, style, lw);
    ctx.strokeStyle = col;
    ctx.stroke();
  }
})

EM_JS(void, iupwasmJsDrawEllipse, (int cid, double xc, double yc, double rx, double ry, int r, int g, int b, int a, int style, int lw), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  var col = globalThis.__iupRGBA(r, g, b, a);
  ctx.beginPath();
  ctx.ellipse(xc, yc, rx, ry, 0, 0, 2 * Math.PI, false);
  if (style == 0) {
    ctx.setLineDash([]);
    ctx.fillStyle = col;
    ctx.fill();
  } else {
    globalThis.__iupDash(ctx, style, lw);
    ctx.strokeStyle = col;
    ctx.stroke();
  }
})

EM_JS(void, iupwasmJsDrawPolygon, (int cid, int ptr, int count, int r, int g, int b, int a, int style, int lw), {
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
    globalThis.__iupDash(ctx, style, lw);
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

EM_JS(void, iupwasmJsDrawRoundRect, (int cid, int x1, int y1, int x2, int y2, int radius, int r, int g, int b, int a, int style, int lw), {
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
    globalThis.__iupDash(ctx, style, lw);
    ctx.strokeStyle = col;
    ctx.stroke();
  }
})

EM_JS(void, iupwasmJsDrawBezier, (int cid, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, int r, int g, int b, int a, int style, int lw), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  globalThis.__iupDash(ctx, style, lw);
  ctx.strokeStyle = globalThis.__iupRGBA(r, g, b, a);
  ctx.beginPath();
  ctx.moveTo(x1, y1);
  ctx.bezierCurveTo(x2, y2, x3, y3, x4, y4);
  ctx.stroke();
})

EM_JS(void, iupwasmJsDrawQuadBezier, (int cid, int x1, int y1, int x2, int y2, int x3, int y3, int r, int g, int b, int a, int style, int lw), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  globalThis.__iupDash(ctx, style, lw);
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
  if (flags & 0x10) { ctx.beginPath(); ctx.rect(x, y, w, h); ctx.clip(); }  /* CLIP */
  if (orient) { ctx.translate(x, y); ctx.rotate(-orient * Math.PI / 180); ctx.translate(-x, -y); }
  var ax = x, align = "left";
  if (flags & 0x1) { ax = x + w / 2; align = "center"; }       /* CENTER */
  else if (flags & 0x2) { ax = x + w; align = "right"; }       /* RIGHT */
  ctx.textAlign = align;
  var m = f.match(/([0-9]+)px/);
  var lh = Math.round((m ? parseInt(m[1]) : 14) * 1.25);
  var lines = s.split("\n");
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
  ctx.restore();   /* drop any previous clip before reclipping */
  ctx.save();
  ctx.beginPath();
  ctx.rect(x, y, w, h);
  ctx.clip();
})

EM_JS(void, iupwasmJsClipRoundRect, (int cid, int x1, int y1, int x2, int y2, int radius), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return;
  ctx.restore();
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
  ctx.restore();
  ctx.save();
})

EM_JS(int, iupwasmJsCanvasGetImageData, (int cid, int ptr, int w, int h), {
  var ctx = globalThis.__iupCtx(cid); if (!ctx) return 0;
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
  if (dc) free(dc);
}

IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  if (!dc) return;
  dc->w = iupwasmJsCanvasClientW(dc->cid);
  dc->h = iupwasmJsCanvasClientH(dc->cid);
}

/* ship the worker-local frame to the on-screen canvas; no-op in main mode */
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

IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int *w, int *h)
{
  if (w) *w = dc ? dc->w : 0;
  if (h) *h = dc ? dc->h : 0;
}

IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  iupwasmJsDrawLine(dc->cid, x1, y1, x2, y2, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), style, line_width);
}

IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  iupwasmJsDrawRect(dc->cid, x1, y1, x2, y2, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), style, line_width);
}

IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  double xc, yc, w, h, s1, s2, tmp;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  w = x2 - x1;
  h = y2 - y1;
  xc = x1 + w / 2.0;
  yc = y1 + h / 2.0;

  /* IUP angles are CCW degrees; canvas y-down makes positive CW, so negate + order ascending */
  s1 = -a1 * IUP_DEG2RAD;
  s2 = -a2 * IUP_DEG2RAD;
  if (s1 > s2) { tmp = s1; s1 = s2; s2 = tmp; }

  iupwasmJsDrawArc(dc->cid, xc, yc, w / 2.0, h / 2.0, s1, s2, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), style, line_width);
}

IUP_SDK_API void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  double xc, yc, w, h;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  w = x2 - x1;
  h = y2 - y1;
  xc = x1 + w / 2.0;
  yc = y1 + h / 2.0;

  iupwasmJsDrawEllipse(dc->cid, xc, yc, w / 2.0, h / 2.0, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), style, line_width);
}

IUP_SDK_API void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  if (!points || count < 2)
    return;
  iupwasmJsDrawPolygon(dc->cid, (int)(intptr_t)points, count, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), style, line_width);
}

IUP_SDK_API void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
{
  iupwasmJsDrawPixel(dc->cid, x, y, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color));
}

IUP_SDK_API void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius, long color, int style, int line_width)
{
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  iupwasmJsDrawRoundRect(dc->cid, x1, y1, x2, y2, corner_radius, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), style, line_width);
}

IUP_SDK_API void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
  iupwasmJsDrawBezier(dc->cid, x1, y1, x2, y2, x3, y3, x4, y4, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), style, line_width);
}

IUP_SDK_API void iupdrvDrawQuadraticBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, long color, int style, int line_width)
{
  iupwasmJsDrawQuadBezier(dc->cid, x1, y1, x2, y2, x3, y3, iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), iupDrawAlpha(color), style, line_width);
}

static void wasmGradientArrays(const long* colors, const float* offsets, int count, unsigned char* rgba, float* offs)
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
  wasmGradientArrays(colors, offsets, count, rgba, offs);
  iupwasmJsDrawLinearGradient(dc->cid, x1, y1, x2, y2, angle, (int)(intptr_t)rgba, (int)(intptr_t)offs, count);
}

IUP_SDK_API void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, const long* colors, const float* offsets, int count)
{
  unsigned char rgba[IUP_GRADIENT_MAX_STOPS * 4];
  float offs[IUP_GRADIENT_MAX_STOPS];
  wasmGradientArrays(colors, offsets, count, rgba, offs);
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

IUP_SDK_API void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)  /* (0,0,0,0) means no clip (the IUP convention) */
  {
    iupwasmJsResetClip(dc->cid);
    return;
  }
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  iupwasmJsClipRect(dc->cid, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

IUP_SDK_API void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius)
{
  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupwasmJsResetClip(dc->cid);
    return;
  }
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  iupwasmJsClipRoundRect(dc->cid, x1, y1, x2, y2, corner_radius);
}

IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  iupwasmJsResetClip(dc->cid);
}

IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int *x1, int *y1, int *x2, int *y2)
{
  (void)dc;
  if (x1) *x1 = 0;
  if (y1) *y1 = 0;
  if (x2) *x2 = 0;
  if (y2) *y2 = 0;
}

IUP_SDK_API void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  iupwasmJsDrawRect(dc->cid, x1, y1, x2, y2, 0, 0, 255, 153, IUP_DRAW_FILL, 1);
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

IUP_SDK_API int iupdrvCanvasGetImageData(Ihandle* ih, unsigned char* data, int w, int h)
{
  int cid = iupwasmIdOf(ih);
  if (!cid || !data)
    return 0;
  return iupwasmJsCanvasGetImageData(cid, (int)(intptr_t)data, w, h);
}
