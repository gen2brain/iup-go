/** \file
 * \brief SVG Draw Driver
 *
 * SVG drawing canvas that mirrors the IUP draw API.
 * All drawing operations build an SVG document string in memory.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "iup.h"

#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_draw_svg.h"
#include "iup_str.h"


/* ---- String Buffer ---- */

typedef struct _iSvgBuffer
{
  char* data;
  int len;
  int cap;
} iSvgBuffer;

static int iSvgBufInit(iSvgBuffer* buf, int initial_cap)
{
  buf->cap = initial_cap;
  buf->data = (char*)malloc(buf->cap);
  if (!buf->data)
  {
    buf->cap = 0;
    buf->len = 0;
    return 0;
  }
  buf->data[0] = '\0';
  buf->len = 0;
  return 1;
}

static void iSvgBufFree(iSvgBuffer* buf)
{
  free(buf->data);
  buf->data = NULL;
  buf->len = 0;
  buf->cap = 0;
}

static int iSvgBufGrow(iSvgBuffer* buf, int needed)
{
  if (buf->len + needed + 1 > buf->cap)
  {
    int new_cap = buf->cap;
    char* new_data;
    while (new_cap < buf->len + needed + 1)
      new_cap *= 2;
    new_data = (char*)realloc(buf->data, new_cap);
    if (!new_data)
      return 0;
    buf->data = new_data;
    buf->cap = new_cap;
  }
  return 1;
}

static void iSvgBufAppend(iSvgBuffer* buf, const char* str)
{
  int slen = (int)strlen(str);
  if (!iSvgBufGrow(buf, slen))
    return;
  memcpy(buf->data + buf->len, str, slen + 1);
  buf->len += slen;
}

static void iSvgBufPrintf(iSvgBuffer* buf, const char* fmt, ...)
{
  va_list ap;
  int needed;
  char tmp[512];

  va_start(ap, fmt);
  needed = vsnprintf(tmp, sizeof(tmp), fmt, ap);
  va_end(ap);

  if (needed < 0)
    return;

  if (needed < (int)sizeof(tmp))
  {
    if (!iSvgBufGrow(buf, needed))
      return;
    memcpy(buf->data + buf->len, tmp, needed + 1);
    buf->len += needed;
  }
  else
  {
    char* big = (char*)malloc(needed + 1);
    if (!big)
      return;
    va_start(ap, fmt);
    vsnprintf(big, needed + 1, fmt, ap);
    va_end(ap);
    if (!iSvgBufGrow(buf, needed))
    {
      free(big);
      return;
    }
    memcpy(buf->data + buf->len, big, needed + 1);
    buf->len += needed;
    free(big);
  }
}

/* ---- Canvas Struct ---- */

struct _iSvgCanvas
{
  int w, h;

  iSvgBuffer buf;
  int id_counter;
  int finalized;

  int clip_x1, clip_y1, clip_x2, clip_y2;
  int clip_active;
  int clip_id;
};

/* ---- Color Parsing ---- */

static void iSvgParseColor(const char* color, int* r, int* g, int* b, int* a)
{
  *r = 0; *g = 0; *b = 0; *a = 255;
  if (!color || !color[0])
    return;

  if (sscanf(color, "%d %d %d %d", r, g, b, a) >= 3)
    return;

  /* fallback: all zeros */
  *r = 0; *g = 0; *b = 0; *a = 255;
}

/* ---- Helpers ---- */

#define iSvgSwapCoord(c1, c2) { if ((c1) > (c2)) { int t_ = (c2); (c2) = (c1); (c1) = t_; } }

static double iSvgAlphaVal(int a)
{
  return a / 255.0;
}

static const char* iSvgDashArray(int style)
{
  switch (style)
  {
  case IUP_DRAW_STROKE_DASH:         return " stroke-dasharray=\"9,3\"";
  case IUP_DRAW_STROKE_DOT:          return " stroke-dasharray=\"1,2\"";
  case IUP_DRAW_STROKE_DASH_DOT:     return " stroke-dasharray=\"7,3,1,3\"";
  case IUP_DRAW_STROKE_DASH_DOT_DOT: return " stroke-dasharray=\"7,3,1,3,1,3\"";
  default: return "";
  }
}

static void iSvgStrokeAttrs(iSvgBuffer* buf, int r, int g, int b, int a, int style, int line_width)
{
  iSvgBufPrintf(buf, " fill=\"none\" stroke=\"rgb(%d,%d,%d)\" stroke-opacity=\"%.3g\" stroke-width=\"%d\"%s",
                r, g, b, iSvgAlphaVal(a), line_width, iSvgDashArray(style));
}

static void iSvgFillAttrs(iSvgBuffer* buf, int r, int g, int b, int a)
{
  iSvgBufPrintf(buf, " fill=\"rgb(%d,%d,%d)\" fill-opacity=\"%.3g\" stroke=\"none\"",
                r, g, b, iSvgAlphaVal(a));
}

static void iSvgStyleAttrs(iSvgBuffer* buf, int r, int g, int b, int a, int style, int line_width)
{
  if (style == IUP_DRAW_FILL)
    iSvgFillAttrs(buf, r, g, b, a);
  else
    iSvgStrokeAttrs(buf, r, g, b, a, style, line_width);
}

static void iSvgClipRef(iSvgCanvas* dc, iSvgBuffer* buf)
{
  if (dc->clip_active)
    iSvgBufPrintf(buf, " clip-path=\"url(#clip%d)\"", dc->clip_id);
}

/* ---- Public API ---- */

iSvgCanvas* iupSvgDrawCreateCanvas(int w, int h)
{
  iSvgCanvas* dc = (iSvgCanvas*)calloc(1, sizeof(iSvgCanvas));
  if (!dc)
    return NULL;

  dc->w = w;
  dc->h = h;

  if (!iSvgBufInit(&dc->buf, 4096))
  {
    free(dc);
    return NULL;
  }
  iSvgBufPrintf(&dc->buf,
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\n",
    w, h, w, h);

  return dc;
}

void iupSvgDrawKillCanvas(iSvgCanvas* dc)
{
  iSvgBufFree(&dc->buf);
  free(dc);
}

const char* iupSvgDrawGetString(iSvgCanvas* dc)
{
  if (!dc->finalized)
  {
    iSvgBufAppend(&dc->buf, "</svg>\n");
    dc->finalized = 1;
  }
  return dc->buf.data;
}

int iupSvgDrawGetStringLength(iSvgCanvas* dc)
{
  return dc->buf.len;
}

void iupSvgDrawGetSize(iSvgCanvas* dc, int* w, int* h)
{
  if (w) *w = dc->w;
  if (h) *h = dc->h;
}

/* ---- Line ---- */

void iupSvgDrawLine(iSvgCanvas* dc, int x1, int y1, int x2, int y2, const char* color, int style, int line_width)
{
  int r, g, b, a;
  iSvgParseColor(color, &r, &g, &b, &a);

  iSvgBufPrintf(&dc->buf, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\"", x1, y1, x2, y2);
  iSvgStrokeAttrs(&dc->buf, r, g, b, a, style, line_width);
  iSvgClipRef(dc, &dc->buf);
  iSvgBufAppend(&dc->buf, "/>\n");
}

/* ---- Rectangle ---- */

void iupSvgDrawRectangle(iSvgCanvas* dc, int x1, int y1, int x2, int y2, const char* color, int style, int line_width)
{
  int r, g, b, a, rw, rh;
  iSvgParseColor(color, &r, &g, &b, &a);

  iSvgSwapCoord(x1, x2);
  iSvgSwapCoord(y1, y2);

  rw = x2 - x1 + (style == IUP_DRAW_FILL ? 1 : 0);
  rh = y2 - y1 + (style == IUP_DRAW_FILL ? 1 : 0);

  iSvgBufPrintf(&dc->buf, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\"", x1, y1, rw, rh);
  iSvgStyleAttrs(&dc->buf, r, g, b, a, style, line_width);
  iSvgClipRef(dc, &dc->buf);
  iSvgBufAppend(&dc->buf, "/>\n");
}

/* ---- Rounded Rectangle ---- */

void iupSvgDrawRoundedRectangle(iSvgCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius, const char* color, int style, int line_width)
{
  int r, g, b, a;
  double radius = (double)corner_radius;
  double max_radius;

  iSvgParseColor(color, &r, &g, &b, &a);

  iSvgSwapCoord(x1, x2);
  iSvgSwapCoord(y1, y2);

  max_radius = ((x2 - x1) < (y2 - y1)) ? (x2 - x1) / 2.0 : (y2 - y1) / 2.0;
  if (radius > max_radius)
    radius = max_radius;

  iSvgBufPrintf(&dc->buf, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"%.1f\" ry=\"%.1f\"",
                x1, y1, x2 - x1, y2 - y1, radius, radius);
  iSvgStyleAttrs(&dc->buf, r, g, b, a, style, line_width);
  iSvgClipRef(dc, &dc->buf);
  iSvgBufAppend(&dc->buf, "/>\n");
}

/* ---- Ellipse ---- */

void iupSvgDrawEllipse(iSvgCanvas* dc, int x1, int y1, int x2, int y2, const char* color, int style, int line_width)
{
  int r, g, b, a;
  double cx, cy, rx, ry;

  iSvgParseColor(color, &r, &g, &b, &a);

  iSvgSwapCoord(x1, x2);
  iSvgSwapCoord(y1, y2);

  rx = (x2 - x1) / 2.0;
  ry = (y2 - y1) / 2.0;
  cx = x1 + rx;
  cy = y1 + ry;

  iSvgBufPrintf(&dc->buf, "<ellipse cx=\"%.1f\" cy=\"%.1f\" rx=\"%.1f\" ry=\"%.1f\"",
                cx, cy, rx, ry);
  iSvgStyleAttrs(&dc->buf, r, g, b, a, style, line_width);
  iSvgClipRef(dc, &dc->buf);
  iSvgBufAppend(&dc->buf, "/>\n");
}

/* ---- Arc ---- */

void iupSvgDrawArc(iSvgCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, const char* color, int style, int line_width)
{
  int r, g, b, a;
  double cx, cy, rx, ry;
  double a1_rad, a2_rad;
  double sx, sy, ex, ey;
  double span;
  int large_arc, sweep;

  iSvgParseColor(color, &r, &g, &b, &a);

  iSvgSwapCoord(x1, x2);
  iSvgSwapCoord(y1, y2);

  rx = (x2 - x1) / 2.0;
  ry = (y2 - y1) / 2.0;
  cx = x1 + rx;
  cy = y1 + ry;

  a1_rad = a1 * IUP_DEG2RAD;
  a2_rad = a2 * IUP_DEG2RAD;

  sx = cx + rx * cos(a1_rad);
  sy = cy - ry * sin(a1_rad);
  ex = cx + rx * cos(a2_rad);
  ey = cy - ry * sin(a2_rad);

  span = a2 - a1;
  if (span < 0) span += 360;
  large_arc = (span > 180) ? 1 : 0;
  sweep = 0;

  if (style == IUP_DRAW_FILL)
  {
    iSvgBufPrintf(&dc->buf, "<path d=\"M%.1f,%.1f L%.1f,%.1f A%.1f,%.1f 0 %d,%d %.1f,%.1f Z\"",
                  cx, cy, sx, sy, rx, ry, large_arc, sweep, ex, ey);
  }
  else
  {
    iSvgBufPrintf(&dc->buf, "<path d=\"M%.1f,%.1f A%.1f,%.1f 0 %d,%d %.1f,%.1f\"",
                  sx, sy, rx, ry, large_arc, sweep, ex, ey);
  }

  iSvgStyleAttrs(&dc->buf, r, g, b, a, style, line_width);
  iSvgClipRef(dc, &dc->buf);
  iSvgBufAppend(&dc->buf, "/>\n");
}

/* ---- Polygon ---- */

void iupSvgDrawPolygon(iSvgCanvas* dc, int* points, int count, const char* color, int style, int line_width)
{
  int i, r, g, b, a;

  if (count < 2)
    return;

  iSvgParseColor(color, &r, &g, &b, &a);

  iSvgBufAppend(&dc->buf, "<polygon points=\"");
  for (i = 0; i < count; i++)
  {
    if (i > 0)
      iSvgBufAppend(&dc->buf, " ");
    iSvgBufPrintf(&dc->buf, "%d,%d", points[2 * i], points[2 * i + 1]);
  }
  iSvgBufAppend(&dc->buf, "\"");
  iSvgStyleAttrs(&dc->buf, r, g, b, a, style, line_width);
  iSvgClipRef(dc, &dc->buf);
  iSvgBufAppend(&dc->buf, "/>\n");
}

/* ---- Pixel ---- */

void iupSvgDrawPixel(iSvgCanvas* dc, int x, int y, const char* color)
{
  int r, g, b, a;
  iSvgParseColor(color, &r, &g, &b, &a);

  iSvgBufPrintf(&dc->buf, "<rect x=\"%d\" y=\"%d\" width=\"1\" height=\"1\"", x, y);
  iSvgFillAttrs(&dc->buf, r, g, b, a);
  iSvgClipRef(dc, &dc->buf);
  iSvgBufAppend(&dc->buf, "/>\n");
}

/* ---- Bezier ---- */

void iupSvgDrawBezier(iSvgCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, const char* color, int style, int line_width)
{
  int r, g, b, a;
  iSvgParseColor(color, &r, &g, &b, &a);

  iSvgBufPrintf(&dc->buf, "<path d=\"M%d,%d C%d,%d %d,%d %d,%d\"",
                x1, y1, x2, y2, x3, y3, x4, y4);
  iSvgStyleAttrs(&dc->buf, r, g, b, a, style, line_width);
  iSvgClipRef(dc, &dc->buf);
  iSvgBufAppend(&dc->buf, "/>\n");
}

void iupSvgDrawQuadraticBezier(iSvgCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, const char* color, int style, int line_width)
{
  int r, g, b, a;
  iSvgParseColor(color, &r, &g, &b, &a);

  iSvgBufPrintf(&dc->buf, "<path d=\"M%d,%d Q%d,%d %d,%d\"",
                x1, y1, x2, y2, x3, y3);
  iSvgStyleAttrs(&dc->buf, r, g, b, a, style, line_width);
  iSvgClipRef(dc, &dc->buf);
  iSvgBufAppend(&dc->buf, "/>\n");
}

/* ---- Gradients ---- */

void iupSvgDrawLinearGradient(iSvgCanvas* dc, int x1, int y1, int x2, int y2, float angle, const char* color1, const char* color2)
{
  int r1, g1, b1, a1, r2, g2, b2, a2;
  int gid;
  float rad, w, h;
  float gx1, gy1, gx2, gy2;

  iSvgParseColor(color1, &r1, &g1, &b1, &a1);
  iSvgParseColor(color2, &r2, &g2, &b2, &a2);

  iSvgSwapCoord(x1, x2);
  iSvgSwapCoord(y1, y2);

  w = (float)(x2 - x1);
  h = (float)(y2 - y1);
  if (w <= 0 || h <= 0)
    return;

  rad = angle * (float)IUP_DEG2RAD;

  gx1 = 0.5f - cosf(rad) / 2.0f;
  gy1 = 0.5f - sinf(rad) / 2.0f;
  gx2 = 0.5f + cosf(rad) / 2.0f;
  gy2 = 0.5f + sinf(rad) / 2.0f;

  gid = dc->id_counter++;

  iSvgBufPrintf(&dc->buf,
    "<defs><linearGradient id=\"lg%d\" x1=\"%.3f\" y1=\"%.3f\" x2=\"%.3f\" y2=\"%.3f\">"
    "<stop offset=\"0%%\" stop-color=\"rgb(%d,%d,%d)\" stop-opacity=\"%.3g\"/>"
    "<stop offset=\"100%%\" stop-color=\"rgb(%d,%d,%d)\" stop-opacity=\"%.3g\"/>"
    "</linearGradient></defs>\n",
    gid, gx1, gy1, gx2, gy2,
    r1, g1, b1, iSvgAlphaVal(a1), r2, g2, b2, iSvgAlphaVal(a2));

  iSvgBufPrintf(&dc->buf, "<rect x=\"%d\" y=\"%d\" width=\"%.0f\" height=\"%.0f\" fill=\"url(#lg%d)\" stroke=\"none\"",
                x1, y1, w, h, gid);
  iSvgClipRef(dc, &dc->buf);
  iSvgBufAppend(&dc->buf, "/>\n");
}

void iupSvgDrawRadialGradient(iSvgCanvas* dc, int cx, int cy, int radius, const char* color_center, const char* color_edge)
{
  int r1, g1, b1, a1, r2, g2, b2, a2;
  int gid = dc->id_counter++;

  iSvgParseColor(color_center, &r1, &g1, &b1, &a1);
  iSvgParseColor(color_edge, &r2, &g2, &b2, &a2);

  iSvgBufPrintf(&dc->buf,
    "<defs><radialGradient id=\"rg%d\" cx=\"50%%\" cy=\"50%%\" r=\"50%%\">"
    "<stop offset=\"0%%\" stop-color=\"rgb(%d,%d,%d)\" stop-opacity=\"%.3g\"/>"
    "<stop offset=\"100%%\" stop-color=\"rgb(%d,%d,%d)\" stop-opacity=\"%.3g\"/>"
    "</radialGradient></defs>\n",
    gid, r1, g1, b1, iSvgAlphaVal(a1), r2, g2, b2, iSvgAlphaVal(a2));

  iSvgBufPrintf(&dc->buf, "<circle cx=\"%d\" cy=\"%d\" r=\"%d\" fill=\"url(#rg%d)\" stroke=\"none\"",
                cx, cy, radius, gid);
  iSvgClipRef(dc, &dc->buf);
  iSvgBufAppend(&dc->buf, "/>\n");
}

/* ---- Text ---- */

static void iSvgParseFontAttrs(iSvgBuffer* buf, const char* font)
{
  /* IUP font format: "Family, Style Size" or "Family, Size" or just "Family"
     Examples: "Helvetica, Bold 16", "Arial, 12", "Courier" */
  char family[128] = "";
  char weight[32] = "";
  char style[32] = "";
  int size = 0;
  const char* comma;
  const char* p;

  if (!font || !font[0])
    return;

  comma = strchr(font, ',');
  if (comma)
  {
    int flen = (int)(comma - font);
    if (flen > (int)sizeof(family) - 1)
      flen = (int)sizeof(family) - 1;
    strncpy(family, font, flen);
    family[flen] = '\0';

    p = comma + 1;
    while (*p == ' ') p++;

    /* Parse style words and size from remainder */
    while (*p)
    {
      while (*p == ' ') p++;
      if (*p == '\0') break;

      if (*p >= '0' && *p <= '9')
      {
        size = 0;
        while (*p >= '0' && *p <= '9')
        {
          size = size * 10 + (*p - '0');
          p++;
        }
      }
      else
      {
        char word[32];
        int wlen = 0;
        while (*p && *p != ' ' && wlen < (int)sizeof(word) - 1)
        {
          word[wlen++] = *p;
          p++;
        }
        word[wlen] = '\0';

        if (strcasecmp(word, "Bold") == 0)
          iupStrCopyN(weight, sizeof(weight), "bold");
        else if (strcasecmp(word, "Italic") == 0)
          iupStrCopyN(style, sizeof(style), "italic");
        else if (strcasecmp(word, "Oblique") == 0)
          iupStrCopyN(style, sizeof(style), "oblique");
      }
    }
  }
  else
  {
    /* No comma, could be "Family Size" or just "Family" */
    const char* last_space = strrchr(font, ' ');
    if (last_space && last_space[1] >= '0' && last_space[1] <= '9')
    {
      int flen = (int)(last_space - font);
      if (flen > (int)sizeof(family) - 1)
        flen = (int)sizeof(family) - 1;
      strncpy(family, font, flen);
      family[flen] = '\0';
      size = atoi(last_space + 1);
    }
    else
    {
      strncpy(family, font, sizeof(family) - 1);
      family[sizeof(family) - 1] = '\0';
    }
  }

  if (family[0])
    iSvgBufPrintf(buf, " font-family=\"%s\"", family);
  if (size > 0)
    iSvgBufPrintf(buf, " font-size=\"%dpt\"", size);
  if (weight[0])
    iSvgBufPrintf(buf, " font-weight=\"%s\"", weight);
  if (style[0])
    iSvgBufPrintf(buf, " font-style=\"%s\"", style);
}

void iupSvgDrawText(iSvgCanvas* dc, const char* text, int len, int x, int y, int w, int h, const char* color, const char* font, int flags, double text_orientation)
{
  int r, g, b, a;
  const char* anchor = "start";
  int tx = x;
  int i, tlen;

  iSvgParseColor(color, &r, &g, &b, &a);

  if (w > 0 && (flags & IUP_DRAW_RIGHT))
  {
    anchor = "end";
    tx = x + w;
  }
  else if (w > 0 && (flags & IUP_DRAW_CENTER))
  {
    anchor = "middle";
    tx = x + w / 2;
  }

  iSvgBufPrintf(&dc->buf, "<text x=\"%d\" y=\"%d\"", tx, y);
  iSvgBufPrintf(&dc->buf, " fill=\"rgb(%d,%d,%d)\" fill-opacity=\"%.3g\" text-anchor=\"%s\"",
                r, g, b, iSvgAlphaVal(a), anchor);

  iSvgParseFontAttrs(&dc->buf, font);

  iSvgBufAppend(&dc->buf, " dominant-baseline=\"hanging\"");

  if (text_orientation != 0)
    iSvgBufPrintf(&dc->buf, " transform=\"rotate(%.1f,%d,%d)\"", -text_orientation, tx, y);

  iSvgClipRef(dc, &dc->buf);
  iSvgBufAppend(&dc->buf, ">");

  tlen = (len > 0 && len < (int)strlen(text)) ? len : (int)strlen(text);
  for (i = 0; i < tlen; i++)
  {
    switch (text[i])
    {
    case '&':  iSvgBufAppend(&dc->buf, "&amp;");  break;
    case '<':  iSvgBufAppend(&dc->buf, "&lt;");   break;
    case '>':  iSvgBufAppend(&dc->buf, "&gt;");   break;
    case '"':  iSvgBufAppend(&dc->buf, "&quot;");  break;
    case '\'': iSvgBufAppend(&dc->buf, "&apos;");  break;
    default:   { char c[2] = { text[i], '\0' }; iSvgBufAppend(&dc->buf, c); } break;
    }
  }

  iSvgBufAppend(&dc->buf, "</text>\n");
}

/* ---- Clipping ---- */

void iupSvgDrawSetClipRect(iSvgCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupSvgDrawResetClip(dc);
    return;
  }

  iSvgSwapCoord(x1, x2);
  iSvgSwapCoord(y1, y2);

  dc->clip_id = dc->id_counter++;
  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
  dc->clip_active = 1;

  iSvgBufPrintf(&dc->buf,
    "<defs><clipPath id=\"clip%d\"><rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\"/></clipPath></defs>\n",
    dc->clip_id, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

void iupSvgDrawSetClipRoundedRect(iSvgCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius)
{
  double radius = (double)corner_radius;
  double max_radius;

  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupSvgDrawResetClip(dc);
    return;
  }

  iSvgSwapCoord(x1, x2);
  iSvgSwapCoord(y1, y2);

  max_radius = ((x2 - x1) < (y2 - y1)) ? (x2 - x1) / 2.0 : (y2 - y1) / 2.0;
  if (radius > max_radius)
    radius = max_radius;

  dc->clip_id = dc->id_counter++;
  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
  dc->clip_active = 1;

  iSvgBufPrintf(&dc->buf,
    "<defs><clipPath id=\"clip%d\"><rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"%.1f\" ry=\"%.1f\"/></clipPath></defs>\n",
    dc->clip_id, x1, y1, x2 - x1 + 1, y2 - y1 + 1, radius, radius);
}

void iupSvgDrawResetClip(iSvgCanvas* dc)
{
  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = 0;
  dc->clip_y2 = 0;
  dc->clip_active = 0;
}

void iupSvgDrawGetClipRect(iSvgCanvas* dc, int* x1, int* y1, int* x2, int* y2)
{
  if (x1) *x1 = dc->clip_x1;
  if (y1) *y1 = dc->clip_y1;
  if (x2) *x2 = dc->clip_x2;
  if (y2) *y2 = dc->clip_y2;
}

/* ---- Embedded Image (PNG + Base64) ---- */

static unsigned int iSvgCrc32Table[256];
static int iSvgCrc32Init = 0;

static void iSvgCrc32InitTable(void)
{
  unsigned int c;
  int n, k;
  for (n = 0; n < 256; n++)
  {
    c = (unsigned int)n;
    for (k = 0; k < 8; k++)
    {
      if (c & 1)
        c = 0xEDB88320u ^ (c >> 1);
      else
        c = c >> 1;
    }
    iSvgCrc32Table[n] = c;
  }
  iSvgCrc32Init = 1;
}

static unsigned int iSvgCrc32(unsigned int crc, const unsigned char* buf, int len)
{
  int i;
  if (!iSvgCrc32Init)
    iSvgCrc32InitTable();
  crc = crc ^ 0xFFFFFFFFu;
  for (i = 0; i < len; i++)
    crc = iSvgCrc32Table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
  return crc ^ 0xFFFFFFFFu;
}

static unsigned int iSvgAdler32(const unsigned char* buf, int len)
{
  unsigned int a = 1, b = 0;
  int i;
  for (i = 0; i < len; i++)
  {
    a = (a + buf[i]) % 65521;
    b = (b + a) % 65521;
  }
  return (b << 16) | a;
}

static void iSvgPngWriteBe32(unsigned char* p, unsigned int v)
{
  p[0] = (unsigned char)(v >> 24);
  p[1] = (unsigned char)(v >> 16);
  p[2] = (unsigned char)(v >> 8);
  p[3] = (unsigned char)(v);
}

static void iSvgPngWriteChunk(iSvgBuffer* buf, const char* type, const unsigned char* data, int len)
{
  unsigned char header[8];
  unsigned char crc_buf[4];
  unsigned int crc;

  iSvgPngWriteBe32(header, (unsigned int)len);
  memcpy(header + 4, type, 4);

  if (!iSvgBufGrow(buf, 8 + len + 4))
    return;
  memcpy(buf->data + buf->len, header, 8);
  buf->len += 8;

  crc = iSvgCrc32(0, header + 4, 4);

  if (len > 0)
  {
    memcpy(buf->data + buf->len, data, len);
    buf->len += len;
    crc = iSvgCrc32(crc, data, len);
  }

  iSvgPngWriteBe32(crc_buf, crc);
  memcpy(buf->data + buf->len, crc_buf, 4);
  buf->len += 4;
  buf->data[buf->len] = '\0';
}

static int iSvgPngEncode(iSvgBuffer* out, const unsigned char* rgba, int w, int h)
{
  static const unsigned char png_sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
  unsigned char ihdr[13];
  unsigned char* filtered;
  unsigned char* idat;
  int row_bytes = w * 4;
  int filtered_len = h * (1 + row_bytes);
  int max_block = 65535;
  int idat_len, num_blocks, i, offset;
  unsigned int adler;

  if (!iSvgBufGrow(out, 8))
    return 0;
  memcpy(out->data + out->len, png_sig, 8);
  out->len += 8;
  out->data[out->len] = '\0';

  iSvgPngWriteBe32(ihdr, (unsigned int)w);
  iSvgPngWriteBe32(ihdr + 4, (unsigned int)h);
  ihdr[8] = 8;   /* bit depth */
  ihdr[9] = 6;   /* color type: RGBA */
  ihdr[10] = 0;  /* compression */
  ihdr[11] = 0;  /* filter */
  ihdr[12] = 0;  /* interlace */
  iSvgPngWriteChunk(out, "IHDR", ihdr, 13);

  filtered = (unsigned char*)malloc(filtered_len);
  if (!filtered)
    return 0;

  for (i = 0; i < h; i++)
  {
    filtered[i * (1 + row_bytes)] = 0;
    memcpy(filtered + i * (1 + row_bytes) + 1, rgba + i * row_bytes, row_bytes);
  }

  adler = iSvgAdler32(filtered, filtered_len);

  num_blocks = (filtered_len + max_block - 1) / max_block;
  idat_len = 2 + filtered_len + num_blocks * 5 + 4;

  idat = (unsigned char*)malloc(idat_len);
  if (!idat)
  {
    free(filtered);
    return 0;
  }

  idat[0] = 0x78;
  idat[1] = 0x01;
  offset = 2;

  for (i = 0; i < num_blocks; i++)
  {
    int block_start = i * max_block;
    int block_len = filtered_len - block_start;
    int is_last;
    if (block_len > max_block) block_len = max_block;
    is_last = (i == num_blocks - 1) ? 1 : 0;

    idat[offset++] = (unsigned char)is_last;
    idat[offset++] = (unsigned char)(block_len & 0xFF);
    idat[offset++] = (unsigned char)((block_len >> 8) & 0xFF);
    idat[offset++] = (unsigned char)(~block_len & 0xFF);
    idat[offset++] = (unsigned char)((~block_len >> 8) & 0xFF);
    memcpy(idat + offset, filtered + block_start, block_len);
    offset += block_len;
  }

  iSvgPngWriteBe32(idat + offset, adler);
  offset += 4;

  iSvgPngWriteChunk(out, "IDAT", idat, offset);
  iSvgPngWriteChunk(out, "IEND", NULL, 0);

  free(filtered);
  free(idat);
  return 1;
}

static const char iSvgBase64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void iSvgBase64Encode(iSvgBuffer* out, const unsigned char* data, int len)
{
  int i, out_len;
  char* p;

  out_len = ((len + 2) / 3) * 4;
  if (!iSvgBufGrow(out, out_len))
    return;
  p = out->data + out->len;

  for (i = 0; i < len - 2; i += 3)
  {
    *p++ = iSvgBase64Chars[(data[i] >> 2) & 0x3F];
    *p++ = iSvgBase64Chars[((data[i] & 0x3) << 4) | ((data[i + 1] >> 4) & 0xF)];
    *p++ = iSvgBase64Chars[((data[i + 1] & 0xF) << 2) | ((data[i + 2] >> 6) & 0x3)];
    *p++ = iSvgBase64Chars[data[i + 2] & 0x3F];
  }
  if (i < len)
  {
    *p++ = iSvgBase64Chars[(data[i] >> 2) & 0x3F];
    if (i + 1 < len)
    {
      *p++ = iSvgBase64Chars[((data[i] & 0x3) << 4) | ((data[i + 1] >> 4) & 0xF)];
      *p++ = iSvgBase64Chars[((data[i + 1] & 0xF) << 2)];
    }
    else
    {
      *p++ = iSvgBase64Chars[((data[i] & 0x3) << 4)];
      *p++ = '=';
    }
    *p++ = '=';
  }

  out->len += out_len;
  out->data[out->len] = '\0';
}

void iupSvgDrawImageRGBA(iSvgCanvas* dc, const unsigned char* rgba, int img_w, int img_h, int x, int y, int w, int h)
{
  iSvgBuffer png_buf;

  if (!rgba || img_w <= 0 || img_h <= 0)
    return;

  if (w <= 0) w = img_w;
  if (h <= 0) h = img_h;

  if (!iSvgBufInit(&png_buf, img_w * img_h * 4 + 256))
    return;

  if (!iSvgPngEncode(&png_buf, rgba, img_w, img_h))
  {
    iSvgBufFree(&png_buf);
    return;
  }

  iSvgBufPrintf(&dc->buf, "<image x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" href=\"data:image/png;base64,",
                x, y, w, h);
  iSvgBase64Encode(&dc->buf, (const unsigned char*)png_buf.data, png_buf.len);
  iSvgBufAppend(&dc->buf, "\"");
  if (w != img_w || h != img_h)
    iSvgBufAppend(&dc->buf, " preserveAspectRatio=\"none\"");
  iSvgClipRef(dc, &dc->buf);
  iSvgBufAppend(&dc->buf, "/>\n");

  iSvgBufFree(&png_buf);
}

/* ---- Selection / Focus Rectangles ---- */

void iupSvgDrawSelectRect(iSvgCanvas* dc, int x1, int y1, int x2, int y2)
{
  iSvgSwapCoord(x1, x2);
  iSvgSwapCoord(y1, y2);

  iSvgBufPrintf(&dc->buf,
    "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" fill=\"rgb(0,0,255)\" fill-opacity=\"0.6\" stroke=\"none\"",
    x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  iSvgClipRef(dc, &dc->buf);
  iSvgBufAppend(&dc->buf, "/>\n");
}

void iupSvgDrawFocusRect(iSvgCanvas* dc, int x1, int y1, int x2, int y2)
{
  iupSvgDrawRectangle(dc, x1, y1, x2, y2, "0 0 0 224", IUP_DRAW_STROKE_DOT, 1);
}
