/** \file
 * \brief Draw Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

#include <Xm/Xm.h>
#include <X11/Xlib.h>

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

  iupAttribSet(ih, "DRAWDRIVER", "X11");

  return dc;
}

IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  if (!dc)
    return;

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

static void iDrawSetLineStyle(IdrawCanvas* dc, int style)
{
  XGCValues gcval;
  if (style == IUP_DRAW_STROKE || style == IUP_DRAW_FILL)
    gcval.line_style = LineSolid;
  else
  {
    if (style == IUP_DRAW_STROKE_DASH)
    {
      char dashes[2] = { 9, 3 };
      XSetDashes(iupmot_display, dc->pixmap_gc, 0, dashes, 2);
    }
    else if (style == IUP_DRAW_STROKE_DOT)
    {
      char dashes[2] = { 1, 2 };
      XSetDashes(iupmot_display, dc->pixmap_gc, 0, dashes, 2);
    }
    else if (style == IUP_DRAW_STROKE_DASH_DOT)
    {
      char dashes[4] = { 7, 3, 1, 3 };
      XSetDashes(iupmot_display, dc->pixmap_gc, 0, dashes, 4);
    }
    else if (style == IUP_DRAW_STROKE_DASH_DOT_DOT)
    {
      char dashes[6] = { 7, 3, 1, 3, 1, 3 };
      XSetDashes(iupmot_display, dc->pixmap_gc, 0, dashes, 6);
    }

    gcval.line_style = LineOnOffDash;
  }

  XChangeGC(iupmot_display, dc->pixmap_gc, GCLineStyle, &gcval);
}

static void iDrawSetLineWidth(IdrawCanvas* dc, int line_width)
{
  XGCValues gcval;

  if (line_width == 1)
    gcval.line_width = 0;
  else
    gcval.line_width = line_width;

  XChangeGC(iupmot_display, dc->pixmap_gc, GCLineWidth, &gcval);
}

/* Combined function to set both line style and width in one XChangeGC call */
static void iDrawSetLineStyleAndWidth(IdrawCanvas* dc, int style, int line_width)
{
  XGCValues gcval;

  /* Set line width */
  if (line_width == 1)
    gcval.line_width = 0;
  else
    gcval.line_width = line_width;

  /* Set line style */
  if (style == IUP_DRAW_STROKE || style == IUP_DRAW_FILL)
    gcval.line_style = LineSolid;
  else
  {
    if (style == IUP_DRAW_STROKE_DASH)
    {
      char dashes[2] = { 9, 3 };
      XSetDashes(iupmot_display, dc->pixmap_gc, 0, dashes, 2);
    }
    else if (style == IUP_DRAW_STROKE_DOT)
    {
      char dashes[2] = { 1, 2 };
      XSetDashes(iupmot_display, dc->pixmap_gc, 0, dashes, 2);
    }
    else if (style == IUP_DRAW_STROKE_DASH_DOT)
    {
      char dashes[4] = { 7, 3, 1, 3 };
      XSetDashes(iupmot_display, dc->pixmap_gc, 0, dashes, 4);
    }
    else if (style == IUP_DRAW_STROKE_DASH_DOT_DOT)
    {
      char dashes[6] = { 7, 3, 1, 3, 1, 3 };
      XSetDashes(iupmot_display, dc->pixmap_gc, 0, dashes, 6);
    }

    gcval.line_style = LineOnOffDash;
  }

  /* Batch update both line width and style in one call */
  XChangeGC(iupmot_display, dc->pixmap_gc, GCLineWidth | GCLineStyle, &gcval);
}

IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  if (style==IUP_DRAW_FILL)
    XFillRectangle(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  else
  {
    iDrawSetLineStyleAndWidth(dc, style, line_width);  /* Batch GC update */

    XDrawRectangle(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1, y2 - y1);
  }
}

IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));

  iDrawSetLineStyleAndWidth(dc, style, line_width);  /* Batch GC update */

  XDrawLine(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2, y2);
}

IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  if (style==IUP_DRAW_FILL)
  {
    XSetArcMode(iupmot_display, dc->pixmap_gc, ArcPieSlice);
    XFillArc(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1, iupRound(a1 * 64), iupRound((a2 - a1) * 64));
  }
  else
  {
    iDrawSetLineStyleAndWidth(dc, style, line_width);  /* Batch GC update */

    XDrawArc(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1, iupRound(a1 * 64), iupRound((a2 - a1) * 64));   /* angle = 1/64ths of a degree */
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

  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));

  if (style==IUP_DRAW_FILL)
    XFillPolygon(iupmot_display, dc->pixmap, dc->pixmap_gc, pnt, count, Complex, CoordModeOrigin);
  else
  {
    iDrawSetLineStyleAndWidth(dc, style, line_width);  /* Batch GC update */

    XDrawLines(iupmot_display, dc->pixmap, dc->pixmap_gc, pnt, pnt_count, CoordModeOrigin);
  }

  if (use_heap)
    free(pnt);
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

  /* make it an empty region */
  if (x1 >= x2) x1 = x2;
  if (y1 >= y2) y1 = y2;

  rect.x = (short)x1;
  rect.y      = (short)y1;
  rect.width = (unsigned short)(x2 - x1 + 1);
  rect.height = (unsigned short)(y2 - y1 + 1);

  XSetClipRectangles(iupmot_display, dc->pixmap_gc, 0, 0, &rect, 1, Unsorted);

  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
}

IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  XSetClipMask(iupmot_display, dc->pixmap_gc, None);

  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = 0;
  dc->clip_y2 = 0;
}

IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  int num_line, width, off_x;
  XFontStruct* xfont = (XFontStruct*)iupmotGetFontStruct(font);

  /* IUP_DRAW_LAYOUTCENTER, IUP_DRAW_ELLIPSIS and IUP_DRAW_WRAP are not supported */
  (void)text_orientation;  /* not supported */

  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));
  XSetFont(iupmot_display, dc->pixmap_gc, xfont->fid);

  num_line = iupStrLineCount(text, len);

  if (flags & IUP_DRAW_CLIP)
  {
    XRectangle rect;
    rect.x = (short)x;
    rect.y = (short)y;
    rect.width = (unsigned short)w;
    rect.height = (unsigned short)h;
    XSetClipRectangles(iupmot_display, dc->pixmap_gc, 0, 0, &rect, 1, Unsorted);
  }

  if (num_line == 1)
  {
    off_x = 0;
    if (flags & (IUP_DRAW_RIGHT | IUP_DRAW_CENTER))
    {
      /* Calculate width once for both center and right alignment */
      width = XTextWidth(xfont, text, len);
      if (flags & IUP_DRAW_RIGHT)
      {
        off_x = w - width;
        if (off_x < 0) off_x = 0;
      }
      else /* IUP_DRAW_CENTER */
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
        l_len = (int)(q - p);  /* Cut the string to contain only one line */
      else
        l_len = (int)strlen(p);  /* use the remaining characters */

      if (sum_len + l_len > len)
        l_len = len - sum_len;

      if (l_len)
      {
        off_x = 0;
        if (flags & (IUP_DRAW_RIGHT | IUP_DRAW_CENTER))
        {
          /* Calculate width once for both center and right alignment */
          width = XTextWidth(xfont, p, l_len);
          if (flags & IUP_DRAW_RIGHT)
          {
            off_x = w - width;
            if (off_x < 0) off_x = 0;
          }
          else /* IUP_DRAW_CENTER */
          {
            off_x = (w - width) / 2;
            if (off_x < 0) off_x = 0;
          }
        }

        /* Draw the line */
        XDrawString(iupmot_display, dc->pixmap, dc->pixmap_gc, x + off_x, y + xfont->ascent, p, l_len);
      }

      /* Advance the string */
      if (q)
        p = q + 1;

      sum_len += l_len;
      if (sum_len == len)
        break;

      /* Advance a line */
      y += line_height;
    }
  }

  /* restore settings */
  if (flags & IUP_DRAW_CLIP)
    XSetClipMask(iupmot_display, dc->pixmap_gc, None);
}

IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, int x, int y, int w, int h)
{
  int img_w, img_h;
  int bpp;
  Pixmap pixmap = (Pixmap)iupImageGetImage(name, dc->ih, make_inactive, bgcolor);
  if (!pixmap)
    return;

  /* must use this info, since image can be a driver image loaded from resources */
  iupdrvImageGetInfo((void*)pixmap, &img_w, &img_h, &bpp);

  /* Image scaling not supported in X11 driver */
  if (w == -1 || w == 0) w = img_w;
  if (h == -1 || h == 0) h = img_h;
  (void)w;
  (void)h;

  XCopyArea(iupmot_display, pixmap, dc->pixmap, dc->pixmap_gc, 0, 0, img_w, img_h, x, y);
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
