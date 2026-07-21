/** \file
 * \brief Draw Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <Xm/Xm.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#ifdef IUP_USE_XFT
#include <X11/Xft/Xft.h>
#endif

#include "iup.h"

#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_object.h"
#include "iup_image.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


struct _IdrawCanvas{
  Ihandle* ih;
  int w, h;

  Window wnd;
  Pixmap pixmap;
  GC pixmap_gc, gc;
  Picture pict;

  int clip_x1, clip_y1, clip_x2, clip_y2;
};

static int motDrawGetGeometry(Display *dpy, Drawable wnd, int *_w, int *_h, int *_d)
{
  Window root;
  int x, y;
  unsigned int w = 0, h = 0, b = 0, d = 0;
  Status status = XGetGeometry(dpy, wnd, &root, &x, &y, &w, &h, &b, &d);
  if (status == 0)
  {
    *_w = 0;
    *_h = 0;
    *_d = 0;
    return 0;
  }
  *_w = (int)w;
  *_h = (int)h;
  *_d = (int)d;
  return 1;
}

/* XCreatePixmap memory is uninitialized, so fill the buffer with the canvas background */
static void motDrawClearBackground(IdrawCanvas* dc)
{
  unsigned char r, g, b;
  char* bgcolor = iupAttribGetStr(dc->ih, "BGCOLOR");
  if (!iupStrToRGB(bgcolor, &r, &g, &b))
  {
    char* global = bgcolor ? IupGetGlobal(bgcolor) : NULL;  /* resolve names like DLGBGCOLOR */
    if (!global || !iupStrToRGB(global, &r, &g, &b))
      r = g = b = 255;
  }
  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(r, g, b));
  XFillRectangle(iupmot_display, dc->pixmap, dc->pixmap_gc, 0, 0, dc->w, dc->h);
}

static int motDrawHasRender(void)
{
  static int has_render = -1;
  if (has_render == -1)
  {
    int event_base, error_base;
    has_render = XRenderQueryExtension(iupmot_display, &event_base, &error_base) ? 1 : 0;
  }
  return has_render;
}

/* XRender wants premultiplied 16-bit components */
static XRenderColor motDrawRenderColor(long color)
{
  XRenderColor rc;
  unsigned int a = iupDrawAlpha(color);
  rc.red = (unsigned short)((iupDrawRed(color) * a * 257) / 255);
  rc.green = (unsigned short)((iupDrawGreen(color) * a * 257) / 255);
  rc.blue = (unsigned short)((iupDrawBlue(color) * a * 257) / 255);
  rc.alpha = (unsigned short)(a * 257);
  return rc;
}

static void motDrawCreatePicture(IdrawCanvas* dc)
{
  XRenderPictFormat* fmt;

  if (!motDrawHasRender())
    return;

  fmt = XRenderFindVisualFormat(iupmot_display, iupmot_visual);
  if (fmt)
    dc->pict = XRenderCreatePicture(iupmot_display, dc->pixmap, fmt, 0, NULL);
}

static int motDrawAlphaColor(IdrawCanvas* dc, long color)
{
  return dc->pict != None && iupDrawAlpha(color) != 255;
}

typedef struct {
  Pixmap pixmap;
  GC gc;
  int x, y, w, h;
} ImotAlphaMask;

static int motDrawAlphaMaskBegin(IdrawCanvas* dc, ImotAlphaMask* m, int x1, int y1, int x2, int y2, int margin)
{
  m->x = x1 - margin;
  m->y = y1 - margin;
  if (m->x < 0) m->x = 0;
  if (m->y < 0) m->y = 0;

  x2 += margin;
  y2 += margin;
  if (x2 > dc->w - 1) x2 = dc->w - 1;
  if (y2 > dc->h - 1) y2 = dc->h - 1;

  m->w = x2 - m->x + 1;
  m->h = y2 - m->y + 1;
  if (m->w <= 0 || m->h <= 0)
    return 0;

  m->pixmap = XCreatePixmap(iupmot_display, dc->pixmap, dc->w, dc->h, 8);
  if (!m->pixmap)
    return 0;

  m->gc = XCreateGC(iupmot_display, m->pixmap, 0, NULL);
  if (!m->gc)
  {
    XFreePixmap(iupmot_display, m->pixmap);
    return 0;
  }

  XSetForeground(iupmot_display, m->gc, 0);
  XFillRectangle(iupmot_display, m->pixmap, m->gc, m->x, m->y, m->w, m->h);
  XSetForeground(iupmot_display, m->gc, 255);
  return 1;
}

static void motDrawAlphaMaskComposite(IdrawCanvas* dc, ImotAlphaMask* m, Picture src)
{
  Picture mask = XRenderCreatePicture(iupmot_display, m->pixmap, XRenderFindStandardFormat(iupmot_display, PictStandardA8), 0, NULL);
  XRenderComposite(iupmot_display, PictOpOver, src, mask, dc->pict, m->x, m->y, m->x, m->y, m->x, m->y, m->w, m->h);
  XRenderFreePicture(iupmot_display, mask);
  XFreeGC(iupmot_display, m->gc);
  XFreePixmap(iupmot_display, m->pixmap);
}

static void motDrawAlphaMaskEnd(IdrawCanvas* dc, ImotAlphaMask* m, long color)
{
  XRenderColor rc = motDrawRenderColor(color);
  Picture src = XRenderCreateSolidFill(iupmot_display, &rc);
  motDrawAlphaMaskComposite(dc, m, src);
  XRenderFreePicture(iupmot_display, src);
}

IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc;
  int depth;

  dc = calloc(1, sizeof(IdrawCanvas));
  if (!dc)
    return NULL;

  dc->ih = ih;
  dc->wnd = (Window)IupGetAttribute(ih, "DRAWABLE");
  if (!dc->wnd)
  {
    free(dc);
    return NULL;
  }

  if (!motDrawGetGeometry(iupmot_display, dc->wnd, &dc->w, &dc->h, &depth))
  {
    free(dc);
    return NULL;
  }

  if (dc->w <= 0) dc->w = 1;
  if (dc->h <= 0) dc->h = 1;

  dc->pixmap = XCreatePixmap(iupmot_display, dc->wnd, dc->w, dc->h, depth);
  if (!dc->pixmap)
  {
    free(dc);
    return NULL;
  }

  dc->pixmap_gc = XCreateGC(iupmot_display, dc->pixmap, 0, NULL);
  if (!dc->pixmap_gc)
  {
    XFreePixmap(iupmot_display, dc->pixmap);
    free(dc);
    return NULL;
  }

  dc->gc = XCreateGC(iupmot_display, dc->wnd, 0, NULL);
  if (!dc->gc)
  {
    XFreeGC(iupmot_display, dc->pixmap_gc);
    XFreePixmap(iupmot_display, dc->pixmap);
    free(dc);
    return NULL;
  }

  motDrawCreatePicture(dc);

  iupAttribSet(ih, "DRAWDRIVER", "X11");

  motDrawClearBackground(dc);

  return dc;
}

IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  if (!dc)
    return;

  if (dc->pict)
    XRenderFreePicture(iupmot_display, dc->pict);
  if (dc->pixmap_gc)
    XFreeGC(iupmot_display, dc->pixmap_gc);
  if (dc->pixmap)
    XFreePixmap(iupmot_display, dc->pixmap);
  if (dc->gc)
    XFreeGC(iupmot_display, dc->gc);

  free(dc);
}

IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  int w = 0, h = 0, depth = 0;

  if (!dc || !dc->wnd)
    return;

  if (!motDrawGetGeometry(iupmot_display, dc->wnd, &w, &h, &depth))
    return;

  if (w <= 0 || h <= 0)
    return;

  if (w != dc->w || h != dc->h)
  {
    dc->w = w;
    dc->h = h;

    if (dc->pict)
      XRenderFreePicture(iupmot_display, dc->pict);
    dc->pict = None;
    if (dc->pixmap_gc)
      XFreeGC(iupmot_display, dc->pixmap_gc);
    if (dc->pixmap)
      XFreePixmap(iupmot_display, dc->pixmap);

    dc->pixmap = XCreatePixmap(iupmot_display, dc->wnd, dc->w, dc->h, depth);
    if (!dc->pixmap)
    {
      dc->pixmap_gc = NULL;
      dc->w = 0;
      dc->h = 0;
      return;
    }

    dc->pixmap_gc = XCreateGC(iupmot_display, dc->pixmap, 0, NULL);
    if (!dc->pixmap_gc)
    {
      XFreePixmap(iupmot_display, dc->pixmap);
      dc->pixmap = None;
      dc->w = 0;
      dc->h = 0;
      return;
    }

    motDrawCreatePicture(dc);

    motDrawClearBackground(dc);
  }
}

IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc)
{
  if (!dc || !dc->wnd || !dc->pixmap || !dc->gc)
    return;

  XCopyArea(iupmot_display, dc->pixmap, dc->wnd, dc->gc, 0, 0, dc->w, dc->h, 0, 0);
}

IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int *w, int *h)
{
  if (!dc)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    return;
  }

  if (w) *w = dc->w;
  if (h) *h = dc->h;
}

static void iDrawSetLineStyleAndWidth(GC gc, int style, int line_width)
{
  XGCValues gcval;

  if (line_width == 1)
    gcval.line_width = 0;
  else
    gcval.line_width = line_width;

  if (style == IUP_DRAW_STROKE || style == IUP_DRAW_FILL)
    gcval.line_style = LineSolid;
  else
  {
    if (style == IUP_DRAW_STROKE_DASH)
    {
      char dashes[2] = { 9, 3 };
      XSetDashes(iupmot_display, gc, 0, dashes, 2);
    }
    else if (style == IUP_DRAW_STROKE_DOT)
    {
      char dashes[2] = { 1, 2 };
      XSetDashes(iupmot_display, gc, 0, dashes, 2);
    }
    else if (style == IUP_DRAW_STROKE_DASH_DOT)
    {
      char dashes[4] = { 7, 3, 1, 3 };
      XSetDashes(iupmot_display, gc, 0, dashes, 4);
    }
    else if (style == IUP_DRAW_STROKE_DASH_DOT_DOT)
    {
      char dashes[6] = { 7, 3, 1, 3, 1, 3 };
      XSetDashes(iupmot_display, gc, 0, dashes, 6);
    }

    gcval.line_style = LineOnOffDash;
  }

  XChangeGC(iupmot_display, gc, GCLineWidth | GCLineStyle, &gcval);
}

IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  if (motDrawAlphaColor(dc, color))
  {
    if (style == IUP_DRAW_FILL)
    {
      XRenderColor rc = motDrawRenderColor(color);
      XRenderFillRectangle(iupmot_display, PictOpOver, dc->pict, &rc, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
    }
    else
    {
      ImotAlphaMask m;
      if (motDrawAlphaMaskBegin(dc, &m, x1, y1, x2, y2, line_width))
      {
        iDrawSetLineStyleAndWidth(m.gc, style, line_width);
        XDrawRectangle(iupmot_display, m.pixmap, m.gc, x1, y1, x2 - x1, y2 - y1);
        motDrawAlphaMaskEnd(dc, &m, color);
      }
    }
    return;
  }

  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));

  if (style==IUP_DRAW_FILL)
    XFillRectangle(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  else
  {
    iDrawSetLineStyleAndWidth(dc->pixmap_gc, style, line_width);

    XDrawRectangle(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1, y2 - y1);
  }
}

IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (motDrawAlphaColor(dc, color))
  {
    ImotAlphaMask m;
    int bx1 = x1 < x2 ? x1 : x2;
    int by1 = y1 < y2 ? y1 : y2;
    int bx2 = x1 > x2 ? x1 : x2;
    int by2 = y1 > y2 ? y1 : y2;
    if (motDrawAlphaMaskBegin(dc, &m, bx1, by1, bx2, by2, line_width))
    {
      iDrawSetLineStyleAndWidth(m.gc, style, line_width);
      XDrawLine(iupmot_display, m.pixmap, m.gc, x1, y1, x2, y2);
      motDrawAlphaMaskEnd(dc, &m, color);
    }
    return;
  }

  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));

  iDrawSetLineStyleAndWidth(dc->pixmap_gc, style, line_width);

  XDrawLine(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2, y2);
}

IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  if (motDrawAlphaColor(dc, color))
  {
    ImotAlphaMask m;
    if (motDrawAlphaMaskBegin(dc, &m, x1, y1, x2, y2, line_width))
    {
      if (style == IUP_DRAW_FILL)
      {
        XSetArcMode(iupmot_display, m.gc, ArcPieSlice);
        XFillArc(iupmot_display, m.pixmap, m.gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1, iupRound(a1 * 64), iupRound((a2 - a1) * 64));
      }
      else
      {
        iDrawSetLineStyleAndWidth(m.gc, style, line_width);
        XDrawArc(iupmot_display, m.pixmap, m.gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1, iupRound(a1 * 64), iupRound((a2 - a1) * 64));
      }
      motDrawAlphaMaskEnd(dc, &m, color);
    }
    return;
  }

  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));

  if (style==IUP_DRAW_FILL)
  {
    XSetArcMode(iupmot_display, dc->pixmap_gc, ArcPieSlice);
    XFillArc(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1, iupRound(a1 * 64), iupRound((a2 - a1) * 64));
  }
  else
  {
    iDrawSetLineStyleAndWidth(dc->pixmap_gc, style, line_width);

    XDrawArc(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1, iupRound(a1 * 64), iupRound((a2 - a1) * 64));   /* angle = 1/64ths of a degree */
  }
}

IUP_SDK_API void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  if (motDrawAlphaColor(dc, color))
  {
    ImotAlphaMask m;
    if (motDrawAlphaMaskBegin(dc, &m, x1, y1, x2, y2, line_width))
    {
      if (style == IUP_DRAW_FILL)
      {
        XSetArcMode(iupmot_display, m.gc, ArcPieSlice);
        XFillArc(iupmot_display, m.pixmap, m.gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1, 0, 23040);
      }
      else
      {
        iDrawSetLineStyleAndWidth(m.gc, style, line_width);
        XDrawArc(iupmot_display, m.pixmap, m.gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1, 0, 23040);
      }
      motDrawAlphaMaskEnd(dc, &m, color);
    }
    return;
  }

  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));

  /* Draw full ellipse using X11 arc with 360 degree span */
  /* angle in X11 is 1/64ths of a degree, so 360*64 = 23040 */
  if (style == IUP_DRAW_FILL)
  {
    XSetArcMode(iupmot_display, dc->pixmap_gc, ArcPieSlice);
    XFillArc(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1, 0, 23040);
  }
  else
  {
    iDrawSetLineStyleAndWidth(dc->pixmap_gc, style, line_width);
    XDrawArc(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1, 0, 23040);
  }
}

IUP_SDK_API void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  int i;
  XPoint stack_pnt[256]; /* Stack buffer for small polygons - avoid malloc overhead */
  XPoint* pnt;
  int use_heap = 0;
  int pnt_count;

  /* Use stack buffer for small polygons, heap for large ones */
  /* For stroked polygons, we need count+1 points to close the path */
  pnt_count = (style == IUP_DRAW_FILL) ? count : count + 1;

  if (pnt_count <= 256)
    pnt = stack_pnt;
  else
  {
    pnt = (XPoint*)malloc(pnt_count * sizeof(XPoint)); /* XPoint uses short for coordinates */
    use_heap = 1;
  }

  for (i = 0; i < count; i++)
  {
    pnt[i].x = (short)points[2*i];
    pnt[i].y = (short)points[2*i+1];
  }

  /* For stroked polygons, close the path by adding first point at the end */
  if (style != IUP_DRAW_FILL)
  {
    pnt[count].x = pnt[0].x;
    pnt[count].y = pnt[0].y;
  }

  if (motDrawAlphaColor(dc, color))
  {
    ImotAlphaMask m;
    int bx1 = pnt[0].x, by1 = pnt[0].y, bx2 = pnt[0].x, by2 = pnt[0].y;
    for (i = 1; i < count; i++)
    {
      if (pnt[i].x < bx1) bx1 = pnt[i].x;
      if (pnt[i].y < by1) by1 = pnt[i].y;
      if (pnt[i].x > bx2) bx2 = pnt[i].x;
      if (pnt[i].y > by2) by2 = pnt[i].y;
    }
    if (motDrawAlphaMaskBegin(dc, &m, bx1, by1, bx2, by2, line_width))
    {
      if (style == IUP_DRAW_FILL)
        XFillPolygon(iupmot_display, m.pixmap, m.gc, pnt, count, Complex, CoordModeOrigin);
      else
      {
        iDrawSetLineStyleAndWidth(m.gc, style, line_width);
        XDrawLines(iupmot_display, m.pixmap, m.gc, pnt, pnt_count, CoordModeOrigin);
      }
      motDrawAlphaMaskEnd(dc, &m, color);
    }

    if (use_heap)
      free(pnt);
    return;
  }

  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));

  if (style==IUP_DRAW_FILL)
    XFillPolygon(iupmot_display, dc->pixmap, dc->pixmap_gc, pnt, count, Complex, CoordModeOrigin);
  else
  {
    iDrawSetLineStyleAndWidth(dc->pixmap_gc, style, line_width);

    XDrawLines(iupmot_display, dc->pixmap, dc->pixmap_gc, pnt, pnt_count, CoordModeOrigin);
  }

  if (use_heap)
    free(pnt);
}

IUP_SDK_API void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
{
  if (motDrawAlphaColor(dc, color))
  {
    XRenderColor rc = motDrawRenderColor(color);
    XRenderFillRectangle(iupmot_display, PictOpOver, dc->pict, &rc, x, y, 1, 1);
    return;
  }

  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color)));
  XDrawPoint(iupmot_display, dc->pixmap, dc->pixmap_gc, x, y);
}

IUP_SDK_API void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius, long color, int style, int line_width)
{
  int diameter, max_radius, use_alpha;
  ImotAlphaMask m;
  Drawable target;
  GC gc;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  /* Clamp radius to prevent oversized corners */
  max_radius = ((x2 - x1) < (y2 - y1)) ? (x2 - x1) / 2 : (y2 - y1) / 2;
  if (corner_radius > max_radius)
    corner_radius = max_radius;

  diameter = corner_radius * 2;

  use_alpha = motDrawAlphaColor(dc, color);
  if (use_alpha)
  {
    if (!motDrawAlphaMaskBegin(dc, &m, x1, y1, x2, y2, line_width))
      return;
    target = m.pixmap;
    gc = m.gc;
  }
  else
  {
    XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color)));
    target = dc->pixmap;
    gc = dc->pixmap_gc;
  }

  if (style == IUP_DRAW_FILL)
  {
    /* Fill rounded rectangle by drawing filled arcs and rectangles */
    /* Top-right arc */
    XFillArc(iupmot_display, target, gc, x2 - diameter, y1, diameter, diameter, 0 * 64, 90 * 64);
    /* Bottom-right arc */
    XFillArc(iupmot_display, target, gc, x2 - diameter, y2 - diameter, diameter, diameter, 270 * 64, 90 * 64);
    /* Bottom-left arc */
    XFillArc(iupmot_display, target, gc, x1, y2 - diameter, diameter, diameter, 180 * 64, 90 * 64);
    /* Top-left arc */
    XFillArc(iupmot_display, target, gc, x1, y1, diameter, diameter, 90 * 64, 90 * 64);

    /* Fill center rectangle */
    XFillRectangle(iupmot_display, target, gc, x1 + corner_radius, y1, x2 - x1 - diameter + 1, y2 - y1 + 1);
    /* Fill left rectangle */
    XFillRectangle(iupmot_display, target, gc, x1, y1 + corner_radius, corner_radius, y2 - y1 - diameter + 1);
    /* Fill right rectangle */
    XFillRectangle(iupmot_display, target, gc, x2 - corner_radius + 1, y1 + corner_radius, corner_radius, y2 - y1 - diameter + 1);
  }
  else
  {
    iDrawSetLineStyleAndWidth(gc, style, line_width);

    /* Draw rounded rectangle by drawing arcs and lines */
    /* Top-right arc */
    XDrawArc(iupmot_display, target, gc, x2 - diameter, y1, diameter, diameter, 0 * 64, 90 * 64);
    /* Bottom-right arc */
    XDrawArc(iupmot_display, target, gc, x2 - diameter, y2 - diameter, diameter, diameter, 270 * 64, 90 * 64);
    /* Bottom-left arc */
    XDrawArc(iupmot_display, target, gc, x1, y2 - diameter, diameter, diameter, 180 * 64, 90 * 64);
    /* Top-left arc */
    XDrawArc(iupmot_display, target, gc, x1, y1, diameter, diameter, 90 * 64, 90 * 64);

    /* Draw connecting lines */
    /* Top line */
    XDrawLine(iupmot_display, target, gc, x1 + corner_radius, y1, x2 - corner_radius, y1);
    /* Right line */
    XDrawLine(iupmot_display, target, gc, x2, y1 + corner_radius, x2, y2 - corner_radius);
    /* Bottom line */
    XDrawLine(iupmot_display, target, gc, x2 - corner_radius, y2, x1 + corner_radius, y2);
    /* Left line */
    XDrawLine(iupmot_display, target, gc, x1, y2 - corner_radius, x1, y1 + corner_radius);
  }

  if (use_alpha)
    motDrawAlphaMaskEnd(dc, &m, color);
}

IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int *x1, int *y1, int *x2, int *y2)
{
  if (x1) *x1 = dc->clip_x1;
  if (y1) *y1 = dc->clip_y1;
  if (x2) *x2 = dc->clip_x2;
  if (y2) *y2 = dc->clip_y2;
}

IUP_SDK_API void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  XRectangle rect;

  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  rect.x = (short)x1;
  rect.y      = (short)y1;
  rect.width = (unsigned short)(x2 - x1 + 1);
  rect.height = (unsigned short)(y2 - y1 + 1);

  XSetClipRectangles(iupmot_display, dc->pixmap_gc, 0, 0, &rect, 1, Unsorted);
  if (dc->pict)
    XRenderSetPictureClipRectangles(iupmot_display, dc->pict, 0, 0, &rect, 1);

  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
}

IUP_SDK_API void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius)
{
  Region region;
  XPoint points[100];
  int num_points = 0;
  int i;
  double angle, step;
  int max_radius;
  double pi = 3.14159265359;

  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  /* Clamp radius to prevent oversized corners */
  max_radius = ((x2 - x1) < (y2 - y1)) ? (x2 - x1) / 2 : (y2 - y1) / 2;
  if (corner_radius > max_radius)
    corner_radius = max_radius;

  /* Create a polygon approximation going clockwise from top-left */
  /* 8 points per corner arc */
  step = 90.0 / 8.0;

  /* Top-left corner arc: from top edge (270) to left edge (180) going clockwise */
  for (i = 0; i <= 8; i++)
  {
    angle = (270.0 - i * step) * pi / 180.0;
    points[num_points].x = (short)(x1 + corner_radius + (int)(corner_radius * cos(angle)));
    points[num_points].y = (short)(y1 + corner_radius + (int)(corner_radius * sin(angle)));
    num_points++;
  }

  /* Bottom-left corner arc: from left edge (180) to bottom edge (90) going clockwise */
  for (i = 1; i <= 8; i++)
  {
    angle = (180.0 - i * step) * pi / 180.0;
    points[num_points].x = (short)(x1 + corner_radius + (int)(corner_radius * cos(angle)));
    points[num_points].y = (short)(y2 - corner_radius + (int)(corner_radius * sin(angle)));
    num_points++;
  }

  /* Bottom-right corner arc: from bottom edge (90) to right edge (0) going clockwise */
  for (i = 1; i <= 8; i++)
  {
    angle = (90.0 - i * step) * pi / 180.0;
    points[num_points].x = (short)(x2 - corner_radius + (int)(corner_radius * cos(angle)));
    points[num_points].y = (short)(y2 - corner_radius + (int)(corner_radius * sin(angle)));
    num_points++;
  }

  /* Top-right corner arc: from right edge (0) to top edge (270 = -90) going clockwise */
  for (i = 1; i <= 8; i++)
  {
    angle = (0.0 - i * step) * pi / 180.0;
    points[num_points].x = (short)(x2 - corner_radius + (int)(corner_radius * cos(angle)));
    points[num_points].y = (short)(y1 + corner_radius + (int)(corner_radius * sin(angle)));
    num_points++;
  }

  /* Create X11 region from polygon */
  region = XPolygonRegion(points, num_points, WindingRule);
  XSetRegion(iupmot_display, dc->pixmap_gc, region);
  if (dc->pict)
    XRenderSetPictureClipRegion(iupmot_display, dc->pict, region);
  XDestroyRegion(region);

  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
}

IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  XSetClipMask(iupmot_display, dc->pixmap_gc, None);
  if (dc->pict)
  {
    XRenderPictureAttributes pa;
    pa.clip_mask = None;
    XRenderChangePicture(iupmot_display, dc->pict, CPClipMask, &pa);
  }

  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = 0;
  dc->clip_y2 = 0;
}

#ifdef IUP_USE_XFT
static void iDrawTextXft(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, XftFont* xftfont, int flags)
{
  int num_line, off_x;
  XftDraw* xftdraw;
  XftColor xftcolor;
  XRenderColor rendercolor;
  XGlyphInfo extents;

  if (!motDrawHasRender())
    color &= 0x00FFFFFF;  /* stored alpha 0 = opaque */
  rendercolor = motDrawRenderColor(color);
  XftColorAllocValue(iupmot_display, iupmot_visual, DefaultColormap(iupmot_display, iupmot_screen), &rendercolor, &xftcolor);

  xftdraw = XftDrawCreate(iupmot_display, dc->pixmap, iupmot_visual, DefaultColormap(iupmot_display, iupmot_screen));
  if (!xftdraw)
  {
    XftColorFree(iupmot_display, iupmot_visual, DefaultColormap(iupmot_display, iupmot_screen), &xftcolor);
    return;
  }

  if (flags & IUP_DRAW_CLIP)
  {
    XRectangle rect;
    rect.x = (short)x;
    rect.y = (short)y;
    rect.width = (unsigned short)w;
    rect.height = (unsigned short)h;
    XftDrawSetClipRectangles(xftdraw, 0, 0, &rect, 1);
  }

  num_line = iupStrLineCount(text, len);

  if (num_line == 1)
  {
    off_x = 0;
    if (flags & (IUP_DRAW_RIGHT | IUP_DRAW_CENTER))
    {
      XftTextExtentsUtf8(iupmot_display, xftfont, (FcChar8*)text, len, &extents);
      if (flags & IUP_DRAW_RIGHT)
      {
        off_x = w - extents.xOff;
        if (off_x < 0) off_x = 0;
      }
      else
      {
        off_x = (w - extents.xOff) / 2;
        if (off_x < 0) off_x = 0;
      }
    }

    XftDrawStringUtf8(xftdraw, &xftcolor, xftfont, x + off_x, y + xftfont->ascent, (FcChar8*)text, len);
  }
  else
  {
    int i, line_height, l_len, sum_len = 0;
    const char *p, *q;

    line_height = xftfont->ascent + xftfont->descent;

    p = text;
    for (i = 0; i < num_line; i++)
    {
      q = strchr(p, '\n');
      if (q)
        l_len = (int)(q - p);
      else
        l_len = (int)strlen(p);

      if (sum_len + l_len > len)
        l_len = len - sum_len;

      if (l_len)
      {
        off_x = 0;
        if (flags & (IUP_DRAW_RIGHT | IUP_DRAW_CENTER))
        {
          XftTextExtentsUtf8(iupmot_display, xftfont, (FcChar8*)p, l_len, &extents);
          if (flags & IUP_DRAW_RIGHT)
          {
            off_x = w - extents.xOff;
            if (off_x < 0) off_x = 0;
          }
          else
          {
            off_x = (w - extents.xOff) / 2;
            if (off_x < 0) off_x = 0;
          }
        }

        XftDrawStringUtf8(xftdraw, &xftcolor, xftfont, x + off_x, y + xftfont->ascent, (FcChar8*)p, l_len);
      }

      if (q)
        p = q + 1;

      sum_len += l_len;
      if (sum_len == len)
        break;

      y += line_height;
    }
  }

  XftDrawDestroy(xftdraw);
  XftColorFree(iupmot_display, iupmot_visual, DefaultColormap(iupmot_display, iupmot_screen), &xftcolor);
}
#endif

static void iDrawTextX11(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, XFontStruct* xfont, int flags)
{
  int num_line, width, off_x;

  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color)));
  XSetFont(iupmot_display, dc->pixmap_gc, xfont->fid);

  if (flags & IUP_DRAW_CLIP)
  {
    XRectangle rect;
    rect.x = (short)x;
    rect.y = (short)y;
    rect.width = (unsigned short)w;
    rect.height = (unsigned short)h;
    XSetClipRectangles(iupmot_display, dc->pixmap_gc, 0, 0, &rect, 1, Unsorted);
  }

  num_line = iupStrLineCount(text, len);

  if (num_line == 1)
  {
    off_x = 0;
    if (flags & (IUP_DRAW_RIGHT | IUP_DRAW_CENTER))
    {
      width = XTextWidth(xfont, text, len);
      if (flags & IUP_DRAW_RIGHT)
      {
        off_x = w - width;
        if (off_x < 0) off_x = 0;
      }
      else
      {
        off_x = (w - width) / 2;
        if (off_x < 0) off_x = 0;
      }
    }

    XDrawString(iupmot_display, dc->pixmap, dc->pixmap_gc, x + off_x, y + xfont->ascent, text, len);
  }
  else
  {
    int i, line_height, l_len, sum_len = 0;
    const char *p, *q;

    line_height = xfont->ascent + xfont->descent;

    p = text;
    for (i = 0; i < num_line; i++)
    {
      q = strchr(p, '\n');
      if (q)
        l_len = (int)(q - p);
      else
        l_len = (int)strlen(p);

      if (sum_len + l_len > len)
        l_len = len - sum_len;

      if (l_len)
      {
        off_x = 0;
        if (flags & (IUP_DRAW_RIGHT | IUP_DRAW_CENTER))
        {
          width = XTextWidth(xfont, p, l_len);
          if (flags & IUP_DRAW_RIGHT)
          {
            off_x = w - width;
            if (off_x < 0) off_x = 0;
          }
          else
          {
            off_x = (w - width) / 2;
            if (off_x < 0) off_x = 0;
          }
        }

        XDrawString(iupmot_display, dc->pixmap, dc->pixmap_gc, x + off_x, y + xfont->ascent, p, l_len);
      }

      if (q)
        p = q + 1;

      sum_len += l_len;
      if (sum_len == len)
        break;

      y += line_height;
    }
  }

  if (flags & IUP_DRAW_CLIP)
    XSetClipMask(iupmot_display, dc->pixmap_gc, None);
}

IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  (void)text_orientation;

#ifdef IUP_USE_XFT
  {
    XftFont* xftfont = (XftFont*)iupmotGetXftFont(font);
    if (xftfont)
    {
      iDrawTextXft(dc, text, len, x, y, w, h, color, xftfont, flags);
      return;
    }
  }
#endif

  {
    XFontStruct* xfont = (XFontStruct*)iupmotGetFontStruct(font);
    if (xfont)
      iDrawTextX11(dc, text, len, x, y, w, h, color, xfont, flags);
  }
}

IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, long tint, int opacity, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int quality)
{
  int img_w, img_h;
  int bpp;
  Pixmap pixmap = (Pixmap)iupImageGetImageTint(name, dc->ih, make_inactive, bgcolor, tint);
  if (!pixmap)
    return;

  /* must use this info, since image can be a driver image loaded from resources */
  iupdrvImageGetInfo((void*)pixmap, &img_w, &img_h, &bpp);

  if (sw <= 0 || sh <= 0)
  {
    sx = 0;
    sy = 0;
    sw = img_w;
    sh = img_h;
  }
  if (w == -1 || w == 0) w = sw;
  if (h == -1 || h == 0) h = sh;

  if (dc->pict)
  {
    XRenderPictFormat* fmt = XRenderFindVisualFormat(iupmot_display, iupmot_visual);
    if (fmt)
    {
      Picture src = XRenderCreatePicture(iupmot_display, pixmap, fmt, 0, NULL);
      XTransform xf;
      memset(&xf, 0, sizeof(xf));
      xf.matrix[0][0] = XDoubleToFixed((double)sw / w);
      xf.matrix[0][2] = XDoubleToFixed(sx);
      xf.matrix[1][1] = XDoubleToFixed((double)sh / h);
      xf.matrix[1][2] = XDoubleToFixed(sy);
      xf.matrix[2][2] = XDoubleToFixed(1.0);
      XRenderSetPictureTransform(iupmot_display, src, &xf);
      XRenderSetPictureFilter(iupmot_display, src, quality == IUP_DRAW_IMAGE_NEAREST ? FilterNearest : FilterBilinear, NULL, 0);
      if (opacity < 255)
      {
        XRenderColor rc = { 0, 0, 0, (unsigned short)(opacity * 257) };
        Picture msk = XRenderCreateSolidFill(iupmot_display, &rc);
        XRenderComposite(iupmot_display, PictOpOver, src, msk, dc->pict, 0, 0, 0, 0, x, y, w, h);
        XRenderFreePicture(iupmot_display, msk);
      }
      else
        XRenderComposite(iupmot_display, PictOpOver, src, None, dc->pict, 0, 0, 0, 0, x, y, w, h);
      XRenderFreePicture(iupmot_display, src);
      return;
    }
  }

  if (w != sw || h != sh)
  {
    XImage* src = XGetImage(iupmot_display, pixmap, 0, 0, img_w, img_h, AllPlanes, ZPixmap);
    if (src)
    {
      XImage* dst = XCreateImage(iupmot_display, iupmot_visual, src->depth, ZPixmap, 0, NULL, w, h, src->bitmap_pad, 0);
      if (dst && (dst->data = (char*)malloc((size_t)dst->bytes_per_line * h)) != NULL)
      {
        int dx, dy;
        for (dy = 0; dy < h; dy++)
        {
          int py = sy + dy * sh / h;
          for (dx = 0; dx < w; dx++)
            XPutPixel(dst, dx, dy, XGetPixel(src, sx + dx * sw / w, py));
        }
        XPutImage(iupmot_display, dc->pixmap, dc->pixmap_gc, dst, 0, 0, x, y, w, h);
      }
      if (dst) XDestroyImage(dst);
      XDestroyImage(src);
      return;
    }
  }

  XCopyArea(iupmot_display, pixmap, dc->pixmap, dc->pixmap_gc, sx, sy, sw, sh, x, y);
}

IUP_SDK_API void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  XSetFunction(iupmot_display, dc->pixmap_gc, GXxor);
  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(255, 255, 255));
  XFillRectangle(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  XSetFunction(iupmot_display, dc->pixmap_gc, GXcopy);
}

#include <Xm/XmP.h>
#include <Xm/DrawP.h>

IUP_SDK_API void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  XmeDrawHighlight(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1, 1);
}

IUP_SDK_API void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
  /* X11/Motif does not have native Bezier support - use line approximation */
  XPoint points[21]; /* 20 segments should give smooth curve */
  int i, num_segments = 20;
  int use_alpha;
  ImotAlphaMask m;
  Drawable target;
  GC gc;

  /* Generate points along Bezier curve using parametric equation */
  for (i = 0; i <= num_segments; i++)
  {
    double t = (double)i / num_segments;
    double t1 = 1.0 - t;
    double t1_3 = t1 * t1 * t1;
    double t1_2_t = 3.0 * t1 * t1 * t;
    double t1_t_2 = 3.0 * t1 * t * t;
    double t_3 = t * t * t;

    points[i].x = (short)(t1_3 * x1 + t1_2_t * x2 + t1_t_2 * x3 + t_3 * x4);
    points[i].y = (short)(t1_3 * y1 + t1_2_t * y2 + t1_t_2 * y3 + t_3 * y4);
  }

  use_alpha = motDrawAlphaColor(dc, color);
  if (use_alpha)
  {
    /* curve stays inside the control-point hull */
    int bx1 = x1, by1 = y1, bx2 = x1, by2 = y1;
    int px[3], py[3];
    px[0] = x2; py[0] = y2;
    px[1] = x3; py[1] = y3;
    px[2] = x4; py[2] = y4;
    for (i = 0; i < 3; i++)
    {
      if (px[i] < bx1) bx1 = px[i];
      if (py[i] < by1) by1 = py[i];
      if (px[i] > bx2) bx2 = px[i];
      if (py[i] > by2) by2 = py[i];
    }
    if (!motDrawAlphaMaskBegin(dc, &m, bx1, by1, bx2, by2, line_width + 1))
      return;
    target = m.pixmap;
    gc = m.gc;
  }
  else
  {
    XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color)));
    target = dc->pixmap;
    gc = dc->pixmap_gc;
  }

  XSetLineAttributes(iupmot_display, gc, line_width, LineSolid, CapRound, JoinRound);

  if (style == IUP_DRAW_FILL)
    XFillPolygon(iupmot_display, target, gc, points, num_segments + 1, Nonconvex, CoordModeOrigin);
  else
    XDrawLines(iupmot_display, target, gc, points, num_segments + 1, CoordModeOrigin);

  if (use_alpha)
    motDrawAlphaMaskEnd(dc, &m, color);
}

IUP_SDK_API void iupdrvDrawQuadraticBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, long color, int style, int line_width)
{
  /* Convert quadratic Bezier to cubic Bezier using the 2/3 formula */
  int cx1, cy1, cx2, cy2;

  cx1 = x1 + ((2 * (x2 - x1)) / 3);
  cy1 = y1 + ((2 * (y2 - y1)) / 3);
  cx2 = x3 + ((2 * (x2 - x3)) / 3);
  cy2 = y3 + ((2 * (y2 - y3)) / 3);

  iupdrvDrawBezier(dc, x1, y1, cx1, cy1, cx2, cy2, x3, y3, color, style, line_width);
}

static long x11InterpolateColor(long color1, long color2, float t)
{
  unsigned char r1 = iupDrawRed(color1), g1 = iupDrawGreen(color1), b1 = iupDrawBlue(color1), a1 = iupDrawAlpha(color1);
  unsigned char r2 = iupDrawRed(color2), g2 = iupDrawGreen(color2), b2 = iupDrawBlue(color2), a2 = iupDrawAlpha(color2);
  unsigned char r = (unsigned char)(r1 + t * (r2 - r1));
  unsigned char g = (unsigned char)(g1 + t * (g2 - g1));
  unsigned char b = (unsigned char)(b1 + t * (b2 - b1));
  unsigned char a = (unsigned char)(a1 + t * (a2 - a1));
  return iupDrawColor(r, g, b, a);
}

static long x11InterpolateStops(const long* colors, const float* offsets, int count, float t)
{
  int i;
  if (t <= offsets[0]) return colors[0];
  if (t >= offsets[count - 1]) return colors[count - 1];
  for (i = 0; i < count - 1; i++)
  {
    if (t <= offsets[i + 1])
    {
      float span = offsets[i + 1] - offsets[i];
      float lt = span > 0 ? (t - offsets[i]) / span : 0.0f;
      return x11InterpolateColor(colors[i], colors[i + 1], lt);
    }
  }
  return colors[count - 1];
}

IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, const long* colors, const float* offsets, int count)
{
  int i, steps;
  float frac, dx, dy, length;
  int px1, py1, px2, py2;
  unsigned long pixel;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  /* Calculate gradient direction */
  float rad = angle * 3.14159265359f / 180.0f;
  dx = (float)cos(rad);
  dy = (float)sin(rad);

  if (dc->pict)
  {
    float w = (float)(x2 - x1);
    float h = (float)(y2 - y1);
    float gx = (w * dx) / 2.0f;
    float gy = (h * dy) / 2.0f;

    if (gx != 0 || gy != 0)
    {
      XLinearGradient grad;
      XFixed stops[IUP_GRADIENT_MAX_STOPS];
      XRenderColor rcolors[IUP_GRADIENT_MAX_STOPS];
      Picture src;
      XRenderPictureAttributes pa;
      int si;

      grad.p1.x = XDoubleToFixed(x1 + w / 2.0f - gx);
      grad.p1.y = XDoubleToFixed(y1 + h / 2.0f - gy);
      grad.p2.x = XDoubleToFixed(x1 + w / 2.0f + gx);
      grad.p2.y = XDoubleToFixed(y1 + h / 2.0f + gy);

      for (si = 0; si < count; si++)
      {
        stops[si] = XDoubleToFixed(offsets[si]);
        rcolors[si] = motDrawRenderColor(colors[si]);
      }

      src = XRenderCreateLinearGradient(iupmot_display, &grad, stops, rcolors, count);
      pa.repeat = RepeatPad;
      XRenderChangePicture(iupmot_display, src, CPRepeat, &pa);
      XRenderComposite(iupmot_display, PictOpOver, src, None, dc->pict, x1, y1, 0, 0, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
      XRenderFreePicture(iupmot_display, src);
      return;
    }
  }

  /* Number of steps for smooth gradient */
  length = (float)sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
  steps = (int)length;
  if (steps < 2) steps = 2;
  if (steps > 256) steps = 256;

  /* Draw gradient strips */
  for (i = 0; i < steps; i++)
  {
    frac = (float)i / (float)(steps - 1);
    long color = x11InterpolateStops(colors, offsets, count, frac);

    pixel = iupmotColorGetPixel(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color));
    XSetForeground(iupmot_display, dc->pixmap_gc, pixel);

    /* Calculate strip position */
    if (fabs(dx) > fabs(dy))  /* More horizontal */
    {
      px1 = x1 + (int)(frac * (x2 - x1));
      px2 = x1 + (int)((frac + 1.0f / steps) * (x2 - x1));
      py1 = y1;
      py2 = y2;
    }
    else  /* More vertical */
    {
      px1 = x1;
      px2 = x2;
      py1 = y1 + (int)(frac * (y2 - y1));
      py2 = y1 + (int)((frac + 1.0f / steps) * (y2 - y1));
    }

    XFillRectangle(iupmot_display, dc->pixmap, dc->pixmap_gc, px1, py1, px2 - px1 + 1, py2 - py1 + 1);
  }
}

IUP_SDK_API void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, const long* colors, const float* offsets, int count)
{
  int i, steps;
  float t, r;
  unsigned long pixel;

  if (dc->pict && radius > 0)
  {
    ImotAlphaMask m;
    if (motDrawAlphaMaskBegin(dc, &m, cx - radius, cy - radius, cx + radius, cy + radius, 0))
    {
      XRadialGradient grad;
      XFixed stops[IUP_GRADIENT_MAX_STOPS];
      XRenderColor rcolors[IUP_GRADIENT_MAX_STOPS];
      Picture src;
      int si;

      grad.inner.x = XDoubleToFixed(cx);
      grad.inner.y = XDoubleToFixed(cy);
      grad.inner.radius = 0;
      grad.outer.x = XDoubleToFixed(cx);
      grad.outer.y = XDoubleToFixed(cy);
      grad.outer.radius = XDoubleToFixed(radius);

      for (si = 0; si < count; si++)
      {
        stops[si] = XDoubleToFixed(offsets[si]);
        rcolors[si] = motDrawRenderColor(colors[si]);
      }

      src = XRenderCreateRadialGradient(iupmot_display, &grad, stops, rcolors, count);
      XFillArc(iupmot_display, m.pixmap, m.gc, cx - radius, cy - radius, 2 * radius, 2 * radius, 0, 23040);
      motDrawAlphaMaskComposite(dc, &m, src);
      XRenderFreePicture(iupmot_display, src);
      return;
    }
  }

  /* Number of steps for smooth gradient */
  steps = radius;
  if (steps < 2) steps = 2;
  if (steps > 256) steps = 256;

  /* Draw from outside to inside */
  for (i = steps - 1; i >= 0; i--)
  {
    t = (float)i / (float)(steps - 1);
    long color = x11InterpolateStops(colors, offsets, count, t);
    r = (float)radius * t;

    pixel = iupmotColorGetPixel(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color));
    XSetForeground(iupmot_display, dc->pixmap_gc, pixel);

    XSetArcMode(iupmot_display, dc->pixmap_gc, ArcPieSlice);
    XFillArc(iupmot_display, dc->pixmap, dc->pixmap_gc, (int)(cx - r), (int)(cy - r), (int)(2 * r), (int)(2 * r), 0, 23040);
  }
}

static void iX11CopyPixelsToRgba(unsigned char* dst, XImage* ximage, int w, int h)
{
  int x, y;
  for (y = 0; y < h; y++)
  {
    unsigned char* dst_line = dst + y * w * 4;
    for (x = 0; x < w; x++)
    {
      unsigned long pixel = XGetPixel(ximage, x, y);
      dst_line[x * 4 + 0] = (unsigned char)((pixel >> 16) & 0xFF);
      dst_line[x * 4 + 1] = (unsigned char)((pixel >> 8) & 0xFF);
      dst_line[x * 4 + 2] = (unsigned char)((pixel >> 0) & 0xFF);
      dst_line[x * 4 + 3] = 255;
    }
  }
}

IUP_SDK_API int iupdrvDrawGetImageData(IdrawCanvas* dc, unsigned char* data)
{
  XImage* ximage = XGetImage(iupmot_display, dc->pixmap, 0, 0, dc->w, dc->h, AllPlanes, ZPixmap);
  if (!ximage)
    return 0;

  iX11CopyPixelsToRgba(data, ximage, dc->w, dc->h);

  XDestroyImage(ximage);
  return 1;
}

IUP_SDK_API int iupdrvCanvasGetImageData(Ihandle* ih, unsigned char* data, int w, int h)
{
  XImage* ximage;

  Window wnd = XtWindow((Widget)ih->handle);
  if (!wnd)
    return 0;

  ximage = XGetImage(iupmot_display, wnd, 0, 0, w, h, AllPlanes, ZPixmap);
  if (!ximage)
    return 0;

  iX11CopyPixelsToRgba(data, ximage, w, h);

  XDestroyImage(ximage);
  return 1;
}
