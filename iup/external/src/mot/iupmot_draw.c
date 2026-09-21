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
  int pixmap_fresh;
  GC pixmap_gc, gc;
  Picture pict;

  int clip_x1, clip_y1, clip_x2, clip_y2;
};

static int motDrawGetGeometry(Ihandle* ih, Drawable wnd, int* _w, int* _h, int* _d)
{
  Window root;
  int x, y;
  unsigned int w = 0, h = 0, b = 0, d = 0;
  Status status;

  if (ih && ih->handle && XtIsRealized(ih->handle) && XtWindow(ih->handle) == wnd)
  {
    Dimension ww = 0, wh = 0;
    Cardinal wd = 0;
    XtVaGetValues(ih->handle, XmNwidth, &ww, XmNheight, &wh, XtNdepth, &wd, NULL);
    if (ww > 0 && wh > 0 && wd > 0)
    {
      *_w = (int)ww;
      *_h = (int)wh;
      *_d = (int)wd;
      return 1;
    }
  }

  status = XGetGeometry(iupmot_display, wnd, &root, &x, &y, &w, &h, &b, &d);
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

  if (!motDrawGetGeometry(ih, dc->wnd, &dc->w, &dc->h, &depth))
  {
    free(dc);
    return NULL;
  }

  if (dc->w <= 0) dc->w = 1;
  if (dc->h <= 0) dc->h = 1;

  dc->pixmap = (Pixmap)(size_t)iupAttribGet(ih, "_IUPMOT_CANVAS_PIXMAP");
  if (dc->pixmap &&
      (iupAttribGetInt(ih, "_IUPMOT_CANVAS_PIXMAP_W") != dc->w ||
       iupAttribGetInt(ih, "_IUPMOT_CANVAS_PIXMAP_H") != dc->h))
  {
    XFreePixmap(iupmot_display, dc->pixmap);
    iupAttribSet(ih, "_IUPMOT_CANVAS_PIXMAP", NULL);
    dc->pixmap = None;
  }

  if (!dc->pixmap)
  {
    dc->pixmap = XCreatePixmap(iupmot_display, dc->wnd, dc->w, dc->h, depth);
    if (!dc->pixmap)
    {
      free(dc);
      return NULL;
    }
    iupAttribSet(ih, "_IUPMOT_CANVAS_PIXMAP", (char*)(size_t)dc->pixmap);
    iupAttribSetInt(ih, "_IUPMOT_CANVAS_PIXMAP_W", dc->w);
    iupAttribSetInt(ih, "_IUPMOT_CANVAS_PIXMAP_H", dc->h);
    dc->pixmap_fresh = 1;
  }

  dc->pixmap_gc = XCreateGC(iupmot_display, dc->pixmap, 0, NULL);
  if (!dc->pixmap_gc)
  {
    free(dc);
    return NULL;
  }

  dc->gc = XCreateGC(iupmot_display, dc->wnd, 0, NULL);
  if (!dc->gc)
  {
    XFreeGC(iupmot_display, dc->pixmap_gc);
    free(dc);
    return NULL;
  }

  motDrawCreatePicture(dc);

  iupAttribSet(ih, "DRAWDRIVER", "X11");

  if (dc->pixmap_fresh)
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
  if (dc->gc)
    XFreeGC(iupmot_display, dc->gc);

  free(dc);
}

IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  int w = 0, h = 0, depth = 0;

  if (!dc || !dc->wnd)
    return;

  if (!motDrawGetGeometry(dc->ih, dc->wnd, &w, &h, &depth))
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
    iupAttribSet(dc->ih, "_IUPMOT_CANVAS_PIXMAP", NULL);

    dc->pixmap = XCreatePixmap(iupmot_display, dc->wnd, dc->w, dc->h, depth);
    if (!dc->pixmap)
    {
      dc->pixmap_gc = NULL;
      dc->w = 0;
      dc->h = 0;
      return;
    }
    iupAttribSet(dc->ih, "_IUPMOT_CANVAS_PIXMAP", (char*)(size_t)dc->pixmap);
    iupAttribSetInt(dc->ih, "_IUPMOT_CANVAS_PIXMAP_W", dc->w);
    iupAttribSetInt(dc->ih, "_IUPMOT_CANVAS_PIXMAP_H", dc->h);

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

IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int* w, int* h)
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

/* X11 encodes coordinates as 16 bit, a primitive far outside the canvas wraps around */
#define MOT_DRAW_LIMIT 16384

static int motDrawClamp(int c)
{
  if (c < -MOT_DRAW_LIMIT)
    return -MOT_DRAW_LIMIT;
  if (c > MOT_DRAW_LIMIT)
    return MOT_DRAW_LIMIT;
  return c;
}

static int motDrawClipLine(int* x1, int* y1, int* x2, int* y2)
{
  double dx = (double)(*x2 - *x1), dy = (double)(*y2 - *y1);
  double t0 = 0.0, t1 = 1.0;
  double p[4], q[4];
  int i, ox = *x1, oy = *y1;

  p[0] = -dx; q[0] = (double)(*x1 + MOT_DRAW_LIMIT);
  p[1] =  dx; q[1] = (double)(MOT_DRAW_LIMIT - *x1);
  p[2] = -dy; q[2] = (double)(*y1 + MOT_DRAW_LIMIT);
  p[3] =  dy; q[3] = (double)(MOT_DRAW_LIMIT - *y1);

  for (i = 0; i < 4; i++)
  {
    if (p[i] == 0.0)
    {
      if (q[i] < 0.0)
        return 0;
      continue;
    }

    {
      double t = q[i] / p[i];
      if (p[i] < 0.0)
      {
        if (t > t1) return 0;
        if (t > t0) t0 = t;
      }
      else
      {
        if (t < t0) return 0;
        if (t < t1) t1 = t;
      }
    }
  }

  *x2 = ox + iupROUND(t1 * dx);
  *y2 = oy + iupROUND(t1 * dy);
  *x1 = ox + iupROUND(t0 * dx);
  *y1 = oy + iupROUND(t0 * dy);
  return 1;
}

IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  x1 = motDrawClamp(x1); y1 = motDrawClamp(y1);
  x2 = motDrawClamp(x2); y2 = motDrawClamp(y2);

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
  if (!motDrawClipLine(&x1, &y1, &x2, &y2))
    return;

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
  x1 = motDrawClamp(x1); y1 = motDrawClamp(y1);
  x2 = motDrawClamp(x2); y2 = motDrawClamp(y2);

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
  x1 = motDrawClamp(x1); y1 = motDrawClamp(y1);
  x2 = motDrawClamp(x2); y2 = motDrawClamp(y2);

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

  /* X11 angles are in 1/64ths of a degree */
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
  XPoint stack_pnt[256];
  XPoint* pnt;
  int use_heap = 0;
  int pnt_count;

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
    pnt[i].x = (short)motDrawClamp(points[2*i]);
    pnt[i].y = (short)motDrawClamp(points[2*i+1]);
  }

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
  if (x < -MOT_DRAW_LIMIT || x > MOT_DRAW_LIMIT || y < -MOT_DRAW_LIMIT || y > MOT_DRAW_LIMIT)
    return;

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
  x1 = motDrawClamp(x1); y1 = motDrawClamp(y1);
  x2 = motDrawClamp(x2); y2 = motDrawClamp(y2);

  int diameter, max_radius, use_alpha;
  ImotAlphaMask m;
  Drawable target;
  GC gc;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

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
    XFillArc(iupmot_display, target, gc, x2 - diameter, y1, diameter, diameter, 0 * 64, 90 * 64);
    XFillArc(iupmot_display, target, gc, x2 - diameter, y2 - diameter, diameter, diameter, 270 * 64, 90 * 64);
    XFillArc(iupmot_display, target, gc, x1, y2 - diameter, diameter, diameter, 180 * 64, 90 * 64);
    XFillArc(iupmot_display, target, gc, x1, y1, diameter, diameter, 90 * 64, 90 * 64);

    XFillRectangle(iupmot_display, target, gc, x1 + corner_radius, y1, x2 - x1 - diameter + 1, y2 - y1 + 1);
    XFillRectangle(iupmot_display, target, gc, x1, y1 + corner_radius, corner_radius, y2 - y1 - diameter + 1);
    XFillRectangle(iupmot_display, target, gc, x2 - corner_radius + 1, y1 + corner_radius, corner_radius, y2 - y1 - diameter + 1);
  }
  else
  {
    iDrawSetLineStyleAndWidth(gc, style, line_width);

    XDrawArc(iupmot_display, target, gc, x2 - diameter, y1, diameter, diameter, 0 * 64, 90 * 64);
    XDrawArc(iupmot_display, target, gc, x2 - diameter, y2 - diameter, diameter, diameter, 270 * 64, 90 * 64);
    XDrawArc(iupmot_display, target, gc, x1, y2 - diameter, diameter, diameter, 180 * 64, 90 * 64);
    XDrawArc(iupmot_display, target, gc, x1, y1, diameter, diameter, 90 * 64, 90 * 64);

    XDrawLine(iupmot_display, target, gc, x1 + corner_radius, y1, x2 - corner_radius, y1);
    XDrawLine(iupmot_display, target, gc, x2, y1 + corner_radius, x2, y2 - corner_radius);
    XDrawLine(iupmot_display, target, gc, x2 - corner_radius, y2, x1 + corner_radius, y2);
    XDrawLine(iupmot_display, target, gc, x1, y2 - corner_radius, x1, y1 + corner_radius);
  }

  if (use_alpha)
    motDrawAlphaMaskEnd(dc, &m, color);
}

IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int* x1, int* y1, int* x2, int* y2)
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

  max_radius = ((x2 - x1) < (y2 - y1)) ? (x2 - x1) / 2 : (y2 - y1) / 2;
  if (corner_radius > max_radius)
    corner_radius = max_radius;

  step = 90.0 / 8.0;

  for (i = 0; i <= 8; i++)
  {
    angle = (270.0 - i * step) * pi / 180.0;
    points[num_points].x = (short)(x1 + corner_radius + (int)(corner_radius * cos(angle)));
    points[num_points].y = (short)(y1 + corner_radius + (int)(corner_radius * sin(angle)));
    num_points++;
  }

  for (i = 1; i <= 8; i++)
  {
    angle = (180.0 - i * step) * pi / 180.0;
    points[num_points].x = (short)(x1 + corner_radius + (int)(corner_radius * cos(angle)));
    points[num_points].y = (short)(y2 - corner_radius + (int)(corner_radius * sin(angle)));
    num_points++;
  }

  for (i = 1; i <= 8; i++)
  {
    angle = (90.0 - i * step) * pi / 180.0;
    points[num_points].x = (short)(x2 - corner_radius + (int)(corner_radius * cos(angle)));
    points[num_points].y = (short)(y2 - corner_radius + (int)(corner_radius * sin(angle)));
    num_points++;
  }

  for (i = 1; i <= 8; i++)
  {
    angle = (0.0 - i * step) * pi / 180.0;
    points[num_points].x = (short)(x2 - corner_radius + (int)(corner_radius * cos(angle)));
    points[num_points].y = (short)(y1 + corner_radius + (int)(corner_radius * sin(angle)));
    num_points++;
  }

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

typedef int (*ImotTextWidth)(void* font, const char* text, int len);

static int motDrawUtf8Back(const char* text, int n)
{
  n--;
  while (n > 0 && ((unsigned char)text[n] & 0xC0) == 0x80)
    n--;
  return n;
}

static int motDrawFitWidth(ImotTextWidth text_width, void* font, const char* text, int len, int width, int wrap)
{
  int n = len;
  if (text_width(font, text, len) <= width)
    return len;

  while (n > 0)
  {
    n = motDrawUtf8Back(text, n);
    if (text_width(font, text, n) <= width)
      break;
  }

  if (wrap)
  {
    int k = n;
    while (k > 0 && text[k] != ' ')
      k--;
    if (k > 0)
      n = k;
  }

  if (n == 0)
  {
    n = 1;
    while (n < len && ((unsigned char)text[n] & 0xC0) == 0x80)
      n++;
  }
  return n;
}

typedef struct _ImotDrawLine
{
  char* text;
  int len;
} ImotDrawLine;

static int motDrawBreakLines(ImotTextWidth text_width, void* font, const char* text, int len, int w, int flags, ImotDrawLine** lines_ret)
{
  int count = 0, cap = 8;
  ImotDrawLine* lines = (ImotDrawLine*)malloc(cap * sizeof(ImotDrawLine));
  const char* p = text;
  const char* end = text + len;

  while (p <= end)
  {
    const char* q = (const char*)memchr(p, '\n', end - p);
    const char* line_end = q ? q : end;
    const char* seg = p;

    do
    {
      int seg_len = (int)(line_end - seg);
      int take = seg_len;
      int extra = 0;

      if ((flags & IUP_DRAW_WRAP) && w > 0)
        take = motDrawFitWidth(text_width, font, seg, seg_len, w, 1);
      else if ((flags & IUP_DRAW_ELLIPSIS) && w > 0 && text_width(font, seg, seg_len) > w)
      {
        int dots = text_width(font, "...", 3);
        take = motDrawFitWidth(text_width, font, seg, seg_len, w - dots, 0);
        extra = 3;
      }

      if (count == cap)
      {
        cap *= 2;
        lines = (ImotDrawLine*)realloc(lines, cap * sizeof(ImotDrawLine));
      }
      lines[count].text = (char*)malloc(take + extra + 1);
      memcpy(lines[count].text, seg, take);
      if (extra)
        memcpy(lines[count].text + take, "...", 3);
      lines[count].text[take + extra] = 0;
      lines[count].len = take + extra;
      count++;

      if (extra)
        break;
      seg += take;
      while (seg < line_end && *seg == ' ')
        seg++;
    } while (seg < line_end);

    if (!q)
      break;
    p = q + 1;
  }

  *lines_ret = lines;
  return count;
}

#ifdef IUP_USE_XFT
static XftFont* motDrawGetRotatedXftFont(XftFont* xftfont, double angle)
{
  static struct { XftFont* base; double angle; XftFont* font; } cache[16];
  static int next;
  FcPattern* pattern;
  FcMatrix matrix;
  XftFont* rotated;
  double rad = angle * IUP_DEG2RAD;
  int i;

  for (i = 0; i < 16; i++)
  {
    if (cache[i].base == xftfont && cache[i].angle == angle)
      return cache[i].font;
  }

  pattern = FcPatternDuplicate(xftfont->pattern);
  matrix.xx = cos(rad);
  matrix.xy = -sin(rad);
  matrix.yx = sin(rad);
  matrix.yy = cos(rad);
  FcPatternDel(pattern, FC_MATRIX);
  FcPatternAddMatrix(pattern, FC_MATRIX, &matrix);
  rotated = XftFontOpenPattern(iupmot_display, pattern);
  if (!rotated)
  {
    FcPatternDestroy(pattern);
    return xftfont;
  }

  if (cache[next].font)
    XftFontClose(iupmot_display, cache[next].font);
  cache[next].base = xftfont;
  cache[next].angle = angle;
  cache[next].font = rotated;
  next = (next + 1) % 16;
  return rotated;
}

static int motDrawXftWidth(void* font, const char* text, int len)
{
  XGlyphInfo extents;
  if (len <= 0)
    return 0;
  XftTextExtentsUtf8(iupmot_display, (XftFont*)font, (FcChar8*)text, len, &extents);
  return extents.xOff;
}

static void iDrawTextXft(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, XftFont* xftfont, int flags, double text_orientation)
{
  XftDraw* xftdraw;
  XftColor xftcolor;
  XRenderColor rendercolor;
  ImotDrawLine* lines;
  int count, i, line_height, layout_w = 0, layout_h;

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

  count = motDrawBreakLines(motDrawXftWidth, xftfont, text, len, w, flags, &lines);
  line_height = xftfont->ascent + xftfont->descent;
  layout_h = count * line_height;
  for (i = 0; i < count; i++)
  {
    int line_w = motDrawXftWidth(xftfont, lines[i].text, lines[i].len);
    if (line_w > layout_w)
      layout_w = line_w;
  }

  if (text_orientation != 0)
  {
    XftFont* rotated = motDrawGetRotatedXftFont(xftfont, text_orientation);
    double rad = text_orientation * IUP_DEG2RAD, c = cos(rad), sn = sin(rad);
    double px, py, lx0, ly0;

    if (flags & IUP_DRAW_LAYOUTCENTER)
    {
      px = x + w / 2.0;
      py = y + h / 2.0;
      lx0 = px - layout_w / 2.0;
      ly0 = py - layout_h / 2.0;
    }
    else
    {
      px = x;
      py = y;
      lx0 = x;
      ly0 = y;
    }

    for (i = 0; i < count; i++)
    {
      int line_w = motDrawXftWidth(xftfont, lines[i].text, lines[i].len);
      double bx = lx0, by = ly0 + i * line_height + xftfont->ascent;
      double dx, dy;
      if (flags & IUP_DRAW_CENTER)
        bx += (layout_w - line_w) / 2.0;
      else if (flags & IUP_DRAW_RIGHT)
        bx += layout_w - line_w;
      dx = bx - px;
      dy = by - py;
      XftDrawStringUtf8(xftdraw, &xftcolor, rotated, (int)floor(px + dx * c + dy * sn + 0.5), (int)floor(py - dx * sn + dy * c + 0.5), (FcChar8*)lines[i].text, lines[i].len);
    }
  }
  else
  {
    int box_w = w > 0 ? w : layout_w;
    for (i = 0; i < count; i++)
    {
      int off_x = 0;
      if (flags & (IUP_DRAW_RIGHT | IUP_DRAW_CENTER))
      {
        int line_w = motDrawXftWidth(xftfont, lines[i].text, lines[i].len);
        off_x = (flags & IUP_DRAW_RIGHT) ? box_w - line_w : (box_w - line_w) / 2;
        if (off_x < 0) off_x = 0;
      }
      XftDrawStringUtf8(xftdraw, &xftcolor, xftfont, x + off_x, y + i * line_height + xftfont->ascent, (FcChar8*)lines[i].text, lines[i].len);
    }
  }

  for (i = 0; i < count; i++)
    free(lines[i].text);
  free(lines);

  XftDrawDestroy(xftdraw);
  XftColorFree(iupmot_display, iupmot_visual, DefaultColormap(iupmot_display, iupmot_screen), &xftcolor);
}
#endif

static int motDrawX11Width(void* font, const char* text, int len)
{
  if (len <= 0)
    return 0;
  return XTextWidth((XFontStruct*)font, text, len);
}

static int motDrawLineOffset(int flags, int box_w, int line_w)
{
  int off_x = 0;
  if (flags & IUP_DRAW_RIGHT)
    off_x = box_w - line_w;
  else if (flags & IUP_DRAW_CENTER)
    off_x = (box_w - line_w) / 2;
  if (off_x < 0)
    off_x = 0;
  return off_x;
}

/* core fonts have no rotation: the unrotated layout is drawn into a 1-bit mask that XRender composites rotated */
static void motDrawTextRotatedX11(IdrawCanvas* dc, ImotDrawLine* lines, int count, XFontStruct* xfont, int layout_w, int layout_h, int x, int y, int w, int h, long color, int flags, double angle)
{
  Pixmap mask;
  GC mask_gc;
  Picture mask_pict, src;
  XTransform xf;
  XRenderColor rendercolor;
  double rad = angle * IUP_DEG2RAD, c = cos(rad), sn = sin(rad);
  double px, py, lx0, ly0, minx, maxx, miny, maxy;
  int line_height = xfont->ascent + xfont->descent;
  int i, dst_x, dst_y, dst_w, dst_h;

  mask = XCreatePixmap(iupmot_display, dc->pixmap, layout_w, layout_h, 1);
  mask_gc = XCreateGC(iupmot_display, mask, 0, NULL);
  XSetForeground(iupmot_display, mask_gc, 0);
  XFillRectangle(iupmot_display, mask, mask_gc, 0, 0, layout_w, layout_h);
  XSetForeground(iupmot_display, mask_gc, 1);
  XSetFont(iupmot_display, mask_gc, xfont->fid);
  for (i = 0; i < count; i++)
  {
    int off_x = motDrawLineOffset(flags, layout_w, XTextWidth(xfont, lines[i].text, lines[i].len));
    XDrawString(iupmot_display, mask, mask_gc, off_x, i * line_height + xfont->ascent, lines[i].text, lines[i].len);
  }
  XFreeGC(iupmot_display, mask_gc);

  if (flags & IUP_DRAW_LAYOUTCENTER)
  {
    px = x + w / 2.0;
    py = y + h / 2.0;
    lx0 = px - layout_w / 2.0;
    ly0 = py - layout_h / 2.0;
  }
  else
  {
    px = x;
    py = y;
    lx0 = x;
    ly0 = y;
  }

  /* destination pixel -> mask pixel: undo the rotation around the pivot */
  memset(&xf, 0, sizeof(xf));
  xf.matrix[0][0] = XDoubleToFixed(c);
  xf.matrix[0][1] = XDoubleToFixed(-sn);
  xf.matrix[0][2] = XDoubleToFixed(px - (c * px - sn * py) - lx0);
  xf.matrix[1][0] = XDoubleToFixed(sn);
  xf.matrix[1][1] = XDoubleToFixed(c);
  xf.matrix[1][2] = XDoubleToFixed(py - (sn * px + c * py) - ly0);
  xf.matrix[2][2] = XDoubleToFixed(1.0);

  mask_pict = XRenderCreatePicture(iupmot_display, mask, XRenderFindStandardFormat(iupmot_display, PictStandardA1), 0, NULL);
  XRenderSetPictureTransform(iupmot_display, mask_pict, &xf);
  XRenderSetPictureFilter(iupmot_display, mask_pict, FilterBilinear, NULL, 0);

  rendercolor = motDrawRenderColor(color);
  src = XRenderCreateSolidFill(iupmot_display, &rendercolor);

  minx = maxx = px;
  miny = maxy = py;
  for (i = 0; i < 4; i++)
  {
    double ux = (i == 1 || i == 2) ? lx0 + layout_w : lx0;
    double uy = (i >= 2) ? ly0 + layout_h : ly0;
    double dx = ux - px, dy = uy - py;
    double sx = px + dx * c + dy * sn, sy = py - dx * sn + dy * c;
    if (sx < minx) minx = sx;
    if (sx > maxx) maxx = sx;
    if (sy < miny) miny = sy;
    if (sy > maxy) maxy = sy;
  }
  dst_x = (int)floor(minx) - 1;
  dst_y = (int)floor(miny) - 1;
  dst_w = (int)ceil(maxx) + 1 - dst_x;
  dst_h = (int)ceil(maxy) + 1 - dst_y;
  if (flags & IUP_DRAW_CLIP)
  {
    int x2 = dst_x + dst_w < x + w ? dst_x + dst_w : x + w;
    int y2 = dst_y + dst_h < y + h ? dst_y + dst_h : y + h;
    if (dst_x < x) dst_x = x;
    if (dst_y < y) dst_y = y;
    dst_w = x2 - dst_x;
    dst_h = y2 - dst_y;
  }

  if (dst_w > 0 && dst_h > 0)
    XRenderComposite(iupmot_display, PictOpOver, src, mask_pict, dc->pict, 0, 0, dst_x, dst_y, dst_x, dst_y, dst_w, dst_h);

  XRenderFreePicture(iupmot_display, src);
  XRenderFreePicture(iupmot_display, mask_pict);
  XFreePixmap(iupmot_display, mask);
}

static void iDrawTextX11(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, XFontStruct* xfont, int flags, double text_orientation)
{
  ImotDrawLine* lines;
  int count, i, line_height, layout_w = 0, layout_h;

  count = motDrawBreakLines(motDrawX11Width, xfont, text, len, w, flags, &lines);
  line_height = xfont->ascent + xfont->descent;
  layout_h = count * line_height;
  for (i = 0; i < count; i++)
  {
    int line_w = XTextWidth(xfont, lines[i].text, lines[i].len);
    if (line_w > layout_w)
      layout_w = line_w;
  }

  if (text_orientation != 0 && dc->pict && layout_w > 0 && layout_h > 0)
    motDrawTextRotatedX11(dc, lines, count, xfont, layout_w, layout_h, x, y, w, h, color, flags, text_orientation);
  else
  {
    int box_w = w > 0 ? w : layout_w;

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

    for (i = 0; i < count; i++)
    {
      int off_x = motDrawLineOffset(flags, box_w, XTextWidth(xfont, lines[i].text, lines[i].len));
      XDrawString(iupmot_display, dc->pixmap, dc->pixmap_gc, x + off_x, y + i * line_height + xfont->ascent, lines[i].text, lines[i].len);
    }

    if (flags & IUP_DRAW_CLIP)
      XSetClipMask(iupmot_display, dc->pixmap_gc, None);
  }

  for (i = 0; i < count; i++)
    free(lines[i].text);
  free(lines);
}

IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
#ifdef IUP_USE_XFT
  {
    XftFont* xftfont = (XftFont*)iupmotGetXftFont(font);
    if (xftfont)
    {
      iDrawTextXft(dc, text, len, x, y, w, h, color, xftfont, flags, text_orientation);
      return;
    }
  }
#endif

  {
    XFontStruct* xfont = (XFontStruct*)iupmotGetFontStruct(font);
    if (xfont)
      iDrawTextX11(dc, text, len, x, y, w, h, color, xfont, flags, text_orientation);
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
  x1 = motDrawClamp(x1); y1 = motDrawClamp(y1);
  x2 = motDrawClamp(x2); y2 = motDrawClamp(y2);

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
  x1 = motDrawClamp(x1); y1 = motDrawClamp(y1);
  x2 = motDrawClamp(x2); y2 = motDrawClamp(y2);

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  XmeDrawHighlight(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1, 1);
}

IUP_SDK_API void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
  /* X11/Motif does not have native Bezier support - use line approximation */
  XPoint points[21];
  int i, num_segments = 20;
  int use_alpha;
  ImotAlphaMask m;
  Drawable target;
  GC gc;

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

  length = (float)sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
  steps = (int)length;
  if (steps < 2) steps = 2;
  if (steps > 256) steps = 256;

  for (i = 0; i < steps; i++)
  {
    frac = (float)i / (float)(steps - 1);
    long color = x11InterpolateStops(colors, offsets, count, frac);

    pixel = iupmotColorGetPixel(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color));
    XSetForeground(iupmot_display, dc->pixmap_gc, pixel);

    if (fabs(dx) > fabs(dy))
    {
      px1 = x1 + (int)(frac * (x2 - x1));
      px2 = x1 + (int)((frac + 1.0f / steps) * (x2 - x1));
      py1 = y1;
      py2 = y2;
    }
    else
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

  steps = radius;
  if (steps < 2) steps = 2;
  if (steps > 256) steps = 256;

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

static Picture motDrawCreateLinearGradientPicture(int x1, int y1, int x2, int y2, float angle, const long* colors, const float* offsets, int count)
{
  float w, h, rad, dx, dy, gx, gy;
  XLinearGradient grad;
  XFixed stops[IUP_GRADIENT_MAX_STOPS];
  XRenderColor rcolors[IUP_GRADIENT_MAX_STOPS];
  XRenderPictureAttributes pa;
  Picture src;
  int i;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  w = (float)(x2 - x1);
  h = (float)(y2 - y1);
  rad = angle * 3.14159265359f / 180.0f;
  dx = (float)cos(rad);
  dy = (float)sin(rad);
  gx = (w * dx) / 2.0f;
  gy = (h * dy) / 2.0f;

  grad.p1.x = XDoubleToFixed(x1 + w / 2.0f - gx);
  grad.p1.y = XDoubleToFixed(y1 + h / 2.0f - gy);
  grad.p2.x = XDoubleToFixed(x1 + w / 2.0f + gx);
  grad.p2.y = XDoubleToFixed(y1 + h / 2.0f + gy);

  for (i = 0; i < count; i++)
  {
    stops[i] = XDoubleToFixed(offsets[i]);
    rcolors[i] = motDrawRenderColor(colors[i]);
  }

  src = XRenderCreateLinearGradient(iupmot_display, &grad, stops, rcolors, count);
  pa.repeat = RepeatPad;
  XRenderChangePicture(iupmot_display, src, CPRepeat, &pa);
  return src;
}

static Picture motDrawCreateRadialGradientPicture(int cx, int cy, int radius, const long* colors, const float* offsets, int count)
{
  XRadialGradient grad;
  XFixed stops[IUP_GRADIENT_MAX_STOPS];
  XRenderColor rcolors[IUP_GRADIENT_MAX_STOPS];
  XRenderPictureAttributes pa;
  Picture src;
  int i;

  grad.inner.x = XDoubleToFixed(cx);
  grad.inner.y = XDoubleToFixed(cy);
  grad.inner.radius = 0;
  grad.outer.x = XDoubleToFixed(cx);
  grad.outer.y = XDoubleToFixed(cy);
  grad.outer.radius = XDoubleToFixed(radius);

  for (i = 0; i < count; i++)
  {
    stops[i] = XDoubleToFixed(offsets[i]);
    rcolors[i] = motDrawRenderColor(colors[i]);
  }

  src = XRenderCreateRadialGradient(iupmot_display, &grad, stops, rcolors, count);
  pa.repeat = RepeatPad;
  XRenderChangePicture(iupmot_display, src, CPRepeat, &pa);
  return src;
}

static Picture motDrawCreateSourcePicture(const IupDrawSource* src)
{
  if (src->type == IUP_SOURCE_LINEAR_GRADIENT)
    return motDrawCreateLinearGradientPicture(src->x1, src->y1, src->x2, src->y2, src->angle, src->colors, src->offsets, src->count);
  if (src->type == IUP_SOURCE_RADIAL_GRADIENT)
    return motDrawCreateRadialGradientPicture(src->cx, src->cy, src->radius, src->colors, src->offsets, src->count);
  {
    XRenderColor rc = motDrawRenderColor(src->color);
    return XRenderCreateSolidFill(iupmot_display, &rc);
  }
}

static void motDrawFillRegion(Drawable target, GC gc, Region region)
{
  XRectangle box;
  XClipBox(region, &box);
  if (box.width <= 0 || box.height <= 0)
    return;

  XSetRegion(iupmot_display, gc, region);
  XFillRectangle(iupmot_display, target, gc, box.x, box.y, box.width, box.height);
  XSetClipMask(iupmot_display, gc, None);
}

static void motDrawStrokePathShape(Drawable target, GC gc, const IupPathSeg* flat, int fcount)
{
  XPoint* pts;
  int np = 0, i, sub_open = 0;
  short start_x = 0, start_y = 0;

  pts = (XPoint*)malloc(((size_t)fcount + 1) * sizeof(XPoint));
  if (!pts)
    return;

  for (i = 0; i < fcount; i++)
  {
    switch (flat[i].op)
    {
    case IUP_PATHSEG_MOVE_TO:
      if (sub_open && np >= 2)
        XDrawLines(iupmot_display, target, gc, pts, np, CoordModeOrigin);
      pts[0].x = (short)motDrawClamp(flat[i].x1);
      pts[0].y = (short)motDrawClamp(flat[i].y1);
      start_x = pts[0].x;
      start_y = pts[0].y;
      np = 1;
      sub_open = 1;
      break;
    case IUP_PATHSEG_LINE_TO:
      if (sub_open)
      {
        pts[np].x = (short)motDrawClamp(flat[i].x1);
        pts[np].y = (short)motDrawClamp(flat[i].y1);
        np++;
      }
      break;
    case IUP_PATHSEG_CLOSE:
      if (sub_open && np >= 1)
      {
        pts[np].x = start_x;
        pts[np].y = start_y;
        XDrawLines(iupmot_display, target, gc, pts, np + 1, CoordModeOrigin);
        pts[0].x = start_x;
        pts[0].y = start_y;
        np = 1;
      }
      break;
    }
  }

  if (sub_open && np >= 2)
    XDrawLines(iupmot_display, target, gc, pts, np, CoordModeOrigin);

  free(pts);
}

typedef struct _ImotPathEdge
{
  int x1, y1, x2, y2;
} ImotPathEdge;

typedef struct _ImotPathCross
{
  double x;
  int winding;
} ImotPathCross;

static int motDrawPathCrossCompare(const void* a, const void* b)
{
  const ImotPathCross* ca = (const ImotPathCross*)a;
  const ImotPathCross* cb = (const ImotPathCross*)b;
  if (ca->x < cb->x)
    return -1;
  if (ca->x > cb->x)
    return 1;
  return 0;
}

static void motDrawPathAddEdge(ImotPathEdge* edges, int* edge_count, int x1, int y1, int x2, int y2)
{
  if (x1 == x2 && y1 == y2)
    return;

  edges[*edge_count].x1 = x1;
  edges[*edge_count].y1 = y1;
  edges[*edge_count].x2 = x2;
  edges[*edge_count].y2 = y2;
  (*edge_count)++;
}

static Region motDrawPathRegion(const IupPathSeg* flat, int fcount, int rule)
{
  Region region = XCreateRegion();
  ImotPathEdge* edges;
  ImotPathCross* crosses;
  int edge_count = 0, cross_count, min_y = MOT_DRAW_LIMIT, max_y = -MOT_DRAW_LIMIT;
  int start_x = 0, start_y = 0, cur_x = 0, cur_y = 0, sub_open = 0, has_segment = 0;
  int i, y;

  edges = (ImotPathEdge*)malloc((size_t)fcount * sizeof(ImotPathEdge));
  crosses = (ImotPathCross*)malloc((size_t)fcount * sizeof(ImotPathCross));
  if (!edges || !crosses)
  {
    free(edges);
    free(crosses);
    return region;
  }

  for (i = 0; i < fcount; i++)
  {
    switch (flat[i].op)
    {
    case IUP_PATHSEG_MOVE_TO:
      if (sub_open && has_segment)
        motDrawPathAddEdge(edges, &edge_count, cur_x, cur_y, start_x, start_y);
      start_x = cur_x = motDrawClamp(flat[i].x1);
      start_y = cur_y = motDrawClamp(flat[i].y1);
      sub_open = 1;
      has_segment = 0;
      break;
    case IUP_PATHSEG_LINE_TO:
      if (sub_open)
      {
        int x = motDrawClamp(flat[i].x1);
        int y = motDrawClamp(flat[i].y1);
        motDrawPathAddEdge(edges, &edge_count, cur_x, cur_y, x, y);
        cur_x = x;
        cur_y = y;
        has_segment = 1;
      }
      break;
    case IUP_PATHSEG_CLOSE:
      if (sub_open && has_segment)
      {
        motDrawPathAddEdge(edges, &edge_count, cur_x, cur_y, start_x, start_y);
        cur_x = start_x;
        cur_y = start_y;
      }
      break;
    }
  }

  if (sub_open && has_segment)
    motDrawPathAddEdge(edges, &edge_count, cur_x, cur_y, start_x, start_y);

  for (i = 0; i < edge_count; i++)
  {
    if (edges[i].y1 < min_y) min_y = edges[i].y1;
    if (edges[i].y2 < min_y) min_y = edges[i].y2;
    if (edges[i].y1 > max_y) max_y = edges[i].y1;
    if (edges[i].y2 > max_y) max_y = edges[i].y2;
  }

  for (y = min_y; y < max_y; y++)
  {
    int state = 0;
    double left = 0;

    cross_count = 0;
    for (i = 0; i < edge_count; i++)
    {
      int y1 = edges[i].y1, y2 = edges[i].y2;
      if (y1 != y2 && ((y1 <= y && y2 > y) || (y2 <= y && y1 > y)))
      {
        crosses[cross_count].x = edges[i].x1 + ((double)(y * 2 + 1 - y1 * 2) * (edges[i].x2 - edges[i].x1)) / (2.0 * (y2 - y1));
        crosses[cross_count].winding = y1 < y2 ? 1 : -1;
        cross_count++;
      }
    }

    qsort(crosses, cross_count, sizeof(ImotPathCross), motDrawPathCrossCompare);
    for (i = 0; i < cross_count; )
    {
      int j = i, was_filled = state != 0;
      while (j < cross_count && crosses[j].x == crosses[i].x)
      {
        if (rule == IUP_PATH_RULE_EVENODD)
          state ^= 1;
        else
          state += crosses[j].winding;
        j++;
      }

      if (!was_filled && state != 0)
        left = crosses[i].x;
      else if (was_filled && state == 0)
      {
        int x1 = (int)ceil(left - 0.5);
        int x2 = (int)ceil(crosses[i].x - 0.5);
        if (x1 < x2)
        {
          XRectangle rect;
          rect.x = (short)x1;
          rect.y = (short)y;
          rect.width = (unsigned short)(x2 - x1);
          rect.height = 1;
          XUnionRectWithRegion(&rect, region, region);
        }
      }
      i = j;
    }
  }

  free(crosses);
  free(edges);
  return region;
}

IUP_SDK_API void iupdrvDrawPathFill(IdrawCanvas* dc, const IupPathSeg* segs, int count, const IupDrawSource* src, int rule)
{
  IupPathSeg* flat;
  int fcount;
  int x1, y1, x2, y2;
  Region region;

  fcount = iupDrawPathFlatten(segs, count, &flat);
  if (fcount < 1)
    return;

  region = motDrawPathRegion(flat, fcount, rule);
  free(flat);
  if (!region)
    return;

  iupDrawPathGetBBox(segs, count, &x1, &y1, &x2, &y2);

  if (src->type == IUP_SOURCE_SOLID && iupDrawAlpha(src->color) == 255 && !dc->pict)
  {
    XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(src->color), iupDrawGreen(src->color), iupDrawBlue(src->color)));
    motDrawFillRegion(dc->pixmap, dc->pixmap_gc, region);
  }
  else
  {
    ImotAlphaMask m;
    if (motDrawAlphaMaskBegin(dc, &m, x1, y1, x2, y2, 0))
    {
      Picture src_p = motDrawCreateSourcePicture(src);
      motDrawFillRegion(m.pixmap, m.gc, region);
      motDrawAlphaMaskComposite(dc, &m, src_p);
      XRenderFreePicture(iupmot_display, src_p);
    }
  }

  XDestroyRegion(region);
}

IUP_SDK_API void iupdrvDrawPathStroke(IdrawCanvas* dc, const IupPathSeg* segs, int count, const IupDrawSource* src, int style, int line_width)
{
  IupPathSeg* flat;
  int fcount;
  int x1, y1, x2, y2;

  fcount = iupDrawPathFlatten(segs, count, &flat);
  if (fcount < 1)
    return;

  iupDrawPathGetBBox(segs, count, &x1, &y1, &x2, &y2);

  if (src->type == IUP_SOURCE_SOLID && !motDrawAlphaColor(dc, src->color))
  {
    XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(src->color), iupDrawGreen(src->color), iupDrawBlue(src->color)));
    iDrawSetLineStyleAndWidth(dc->pixmap_gc, style, line_width);
    motDrawStrokePathShape(dc->pixmap, dc->pixmap_gc, flat, fcount);
  }
  else
  {
    ImotAlphaMask m;
    if (motDrawAlphaMaskBegin(dc, &m, x1, y1, x2, y2, line_width + 1))
    {
      Picture src_p = motDrawCreateSourcePicture(src);
      iDrawSetLineStyleAndWidth(m.gc, style, line_width);
      motDrawStrokePathShape(m.pixmap, m.gc, flat, fcount);
      motDrawAlphaMaskComposite(dc, &m, src_p);
      XRenderFreePicture(iupmot_display, src_p);
    }
  }

  free(flat);
}

IUP_SDK_API void iupdrvDrawSetClipPath(IdrawCanvas* dc, const IupPathSeg* segs, int count, int rule)
{
  IupPathSeg* flat;
  int fcount;
  int x1, y1, x2, y2;
  Region region;

  fcount = iupDrawPathFlatten(segs, count, &flat);
  if (fcount < 1)
    return;

  region = motDrawPathRegion(flat, fcount, rule);
  XSetRegion(iupmot_display, dc->pixmap_gc, region);
  if (dc->pict)
    XRenderSetPictureClipRegion(iupmot_display, dc->pict, region);
  XDestroyRegion(region);
  free(flat);

  iupDrawPathGetBBox(segs, count, &x1, &y1, &x2, &y2);
  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
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
