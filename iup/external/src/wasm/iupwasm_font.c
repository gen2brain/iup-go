/** \file
 * \brief WebAssembly Font Management
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"

#include "iupwasm_drv.h"


EM_JS(void, iupwasmJsSetFont, (int id, const char* css), {
  globalThis.__iupApply({ op: 'setfont', id: id, css: UTF8ToString(css) });
})

/* measureText via OffscreenCanvas works on a Worker, so font sizing needs no round-trip to main */
EM_JS(int, iupwasmMeasureWidth, (const char* css, const char* txt), {
  var ctx = globalThis.__iupMeasure;
  if (!ctx) {
    var cv = (typeof document !== 'undefined') ? document.createElement('canvas') : new OffscreenCanvas(8, 8);
    ctx = globalThis.__iupMeasure = cv.getContext('2d');
  }
  ctx.font = UTF8ToString(css);
  return Math.ceil(ctx.measureText(UTF8ToString(txt)).width);
})

EM_JS(int, iupwasmMeasureHeight, (const char* css), {
  if (typeof document === 'undefined') {
    var ctx = globalThis.__iupMeasure;
    if (!ctx) ctx = globalThis.__iupMeasure = new OffscreenCanvas(8, 8).getContext('2d');
    ctx.font = UTF8ToString(css);
    var m = ctx.measureText('Mg');
    return Math.ceil((m.fontBoundingBoxAscent + m.fontBoundingBoxDescent) ||
                     (m.actualBoundingBoxAscent + m.actualBoundingBoxDescent)) || 16;
  }
  var s = globalThis.__iupProbe;
  if (!s)
  {
    s = document.createElement('span');
    s.style.visibility = 'hidden';
    s.style.position = 'absolute';
    s.style.whiteSpace = 'nowrap';
    s.textContent = 'Mg';
    document.body.appendChild(s);
    globalThis.__iupProbe = s;
  }
  s.style.font = UTF8ToString(css);
  return s.offsetHeight || 16;
})

void iupwasmFontToCss(const char* iupfont, char* css, int csslen)
{
  char typeface[1024] = "";
  int size = 0, bold = 0, italic = 0, underline = 0, strikeout = 0;
  int px;
  const char* fallback = "sans-serif";

  if (!iupfont || !iupGetFontInfo(iupfont, typeface, &size, &bold, &italic, &underline, &strikeout))
  {
    snprintf(css, csslen, "12px sans-serif");
    return;
  }

  /* IUP size: negative is pixels, positive is points. */
  px = (size < 0) ? -size : (int)(size * 96.0 / 72.0 + 0.5);
  if (px <= 0)
    px = 12;

  if (iupStrEqualNoCase(typeface, "Courier") || iupStrEqualNoCase(typeface, "Courier New") ||
      iupStrEqualNoCase(typeface, "Monospace") || iupStrEqualNoCase(typeface, "Mono"))
    fallback = "monospace";
  else if (iupStrEqualNoCase(typeface, "Times") || iupStrEqualNoCase(typeface, "Times New Roman") ||
           iupStrEqualNoCase(typeface, "Serif"))
    fallback = "serif";

  snprintf(css, csslen, "%s%s%dpx \"%s\", %s",
           italic ? "italic " : "", bold ? "bold " : "", px, typeface, fallback);
}

static void wasmFontForIh(Ihandle* ih, char* css, int csslen)
{
  char* font = ih ? IupGetAttribute(ih, "FONT") : NULL;
  iupwasmFontToCss(font, css, csslen);
}

EM_JS(void, iupwasmJsInstallFontBase, (const char* css, int line), {
  globalThis.__iupApply({ op: 'fontbase', css: UTF8ToString(css), line: line });
})

IUP_SDK_API void iupdrvFontInit(void)
{
  char css[1100];
  int line;
  iupwasmFontToCss(iupdrvGetSystemFont(), css, sizeof(css));
  line = iupwasmMeasureHeight(css);  /* one line box; text rows are pinned to it so VISIBLELINES is exact */
  iupwasmJsInstallFontBase(css, line);
  iupwasmJsInstallMarkup();
}

IUP_SDK_API void iupdrvFontFinish(void)
{
}

IUP_SDK_API int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
  char css[1100];
  int id = iupwasmIdOf(ih);
  iupwasmFontToCss(value, css, sizeof(css));
  if (id)
    iupwasmJsSetFont(id, css);
  return 1;
}

IUP_SDK_API void iupdrvFontGetCharSize(Ihandle* ih, int *charwidth, int *charheight)
{
  char css[1100];
  wasmFontForIh(ih, css, sizeof(css));
  if (charwidth)
    *charwidth = (iupwasmMeasureWidth(css, "0123456789") + 5) / 10;
  if (charheight)
    *charheight = iupwasmMeasureHeight(css);
}

IUP_SDK_API int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
  char css[1100];
  if (!str || !str[0])
    return 0;
  wasmFontForIh(ih, css, sizeof(css));
  return iupwasmMeasureWidth(css, str);
}

/* measure the rendered markup HTML, not the raw <span> source (which over-measures ~7x) */
EM_JS(void, iupwasmJsMeasureMarkup, (const char* css, const char* markup, int* wp, int* hp), {
  var css2 = UTF8ToString(css);
  var html = globalThis.__iupMarkup(UTF8ToString(markup));
  if (typeof document === 'undefined')
  {
    var r = globalThis.__iupReadSync({ op: 'measuremarkup', css: css2, html: html });
    HEAP32[wp >> 2] = r[0];
    HEAP32[hp >> 2] = r[1] || 16;
    return;
  }
  var s = globalThis.__iupMarkupProbe;
  if (!s)
  {
    s = document.createElement('span');
    s.style.position = 'absolute'; s.style.visibility = 'hidden'; s.style.whiteSpace = 'nowrap';
    document.body.appendChild(s);
    globalThis.__iupMarkupProbe = s;
  }
  s.style.font = css2;
  s.innerHTML = html;
  HEAP32[wp >> 2] = s.offsetWidth;
  HEAP32[hp >> 2] = s.offsetHeight || 16;
})

IUP_SDK_API void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int *w, int *h)
{
  if (ih && str && iupAttribGetBoolean(ih, "MARKUP"))
  {
    char css[1100];
    int mw = 0, mh = 0;
    iupwasmFontToCss(IupGetAttribute(ih, "FONT"), css, sizeof(css));
    iupwasmJsInstallMarkup();
    iupwasmJsMeasureMarkup(css, str, &mw, &mh);
    if (w) *w = mw;
    if (h) *h = mh;
    return;
  }
  iupdrvFontGetTextSize(IupGetAttribute(ih, "FONT"), str, -1, w, h);
}

IUP_SDK_API void iupdrvFontGetTextSize(const char* font, const char* str, int len, int *w, int *h)
{
  char css[1100];
  int total, max_w = 0, lines = 1, start, i, line_h;
  char* buf;

  iupwasmFontToCss(font, css, sizeof(css));
  line_h = iupwasmMeasureHeight(css);

  if (!str)
  {
    if (w) *w = 0;
    if (h) *h = line_h;
    return;
  }

  total = (len < 0) ? (int)strlen(str) : len;
  buf = (char*)malloc(total + 1);

  for (i = 0, start = 0; i <= total; i++)
  {
    if (i == total || str[i] == '\n')
    {
      int n = i - start, lw;
      memcpy(buf, str + start, n);
      buf[n] = 0;
      lw = iupwasmMeasureWidth(css, buf);
      if (lw > max_w) max_w = lw;
      if (i < total) lines++;
      start = i + 1;
    }
  }

  free(buf);
  if (w) *w = max_w;
  if (h) *h = lines * line_h;
}

IUP_SDK_API void iupdrvFontGetFontDim(const char* font, int *max_width, int *line_height, int *ascent, int *descent)
{
  char css[1100];
  int lh;
  iupwasmFontToCss(font, css, sizeof(css));
  lh = iupwasmMeasureHeight(css);
  if (max_width) *max_width = (iupwasmMeasureWidth(css, "0123456789") + 5) / 10;
  if (line_height) *line_height = lh;
  if (ascent) *ascent = (lh * 4) / 5;
  if (descent) *descent = lh / 5;
}

IUP_SDK_API char* iupdrvGetSystemFont(void)
{
  return "Helvetica, 9";
}

/* web-safe families; Local Font Access is async so can't feed this sync getter */
EM_JS(int, iupwasmJsFontListCount, (void), {
  if (!globalThis.__iupFontList) globalThis.__iupFontList = [
    "Arial", "Arial Black", "Comic Sans MS", "Courier New", "Georgia",
    "Impact", "Lucida Console", "Palatino Linotype", "Tahoma",
    "Times New Roman", "Trebuchet MS", "Verdana",
    "cursive", "monospace", "sans-serif", "serif"];
  return globalThis.__iupFontList.length;
})

EM_JS(int, iupwasmJsFontListGet, (int i), {
  var s = globalThis.__iupFontList[i] || "";
  var len = lengthBytesUTF8(s) + 1, ptr = _malloc(len);
  stringToUTF8(s, ptr, len);
  return ptr;
})

IUP_SDK_API int iupdrvFontGetFamilyList(char*** list)
{
  int i, count = iupwasmJsFontListCount();
  char** families = malloc(count * sizeof(char*));
  for (i = 0; i < count; i++)
    families[i] = (char*)(intptr_t)iupwasmJsFontListGet(i);  /* _malloc'd; caller frees */
  if (list)
    *list = families;
  return count;
}
