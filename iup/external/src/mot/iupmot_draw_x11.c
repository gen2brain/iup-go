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

static void motDrawGetGeometry(Display *dpy, Drawable wnd, int *_w, int *_h, int *_d)
{
  Window root;
  int x, y;
  unsigned int w, h, b, d;
  XGetGeometry(dpy, wnd, &root, &x, &y, &w, &h, &b, &d);
  *_w = w;
  *_h = h;
  *_d = d;
}

IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = calloc(1, sizeof(IdrawCanvas));
  int depth;

  dc->ih = ih;
  dc->wnd = (Window)IupGetAttribute(ih, "DRAWABLE");

  dc->gc = XCreateGC(iupmot_display, dc->wnd, 0, NULL);

  motDrawGetGeometry(iupmot_display, dc->wnd, &dc->w, &dc->h, &depth);

  dc->pixmap = XCreatePixmap(iupmot_display, dc->wnd, dc->w, dc->h, depth);
  dc->pixmap_gc = XCreateGC(iupmot_display, dc->pixmap, 0, NULL);

  iupAttribSet(ih, "DRAWDRIVER", "X11");

  return dc;
}

IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  XFreeGC(iupmot_display, dc->pixmap_gc);
  XFreePixmap(iupmot_display, dc->pixmap);
  XFreeGC(iupmot_display, dc->gc);

  free(dc);
}

IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  int w, h, depth;

  motDrawGetGeometry(iupmot_display, dc->wnd, &w, &h, &depth);

  if (w != dc->w || h != dc->h)
  {
    dc->w = w;
    dc->h = h;

    XFreeGC(iupmot_display, dc->pixmap_gc);
    XFreePixmap(iupmot_display, dc->pixmap);

    dc->pixmap = XCreatePixmap(iupmot_display, dc->wnd, dc->w, dc->h, depth);
    dc->pixmap_gc = XCreateGC(iupmot_display, dc->pixmap, 0, NULL);
  }
}

IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc)
{
  XCopyArea(iupmot_display, dc->pixmap, dc->wnd, dc->gc, 0, 0, dc->w, dc->h, 0, 0);
}

IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int *w, int *h)
{
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

IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  if (style==IUP_DRAW_FILL)
    XFillRectangle(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  else
  {
    iDrawSetLineWidth(dc, line_width);
    iDrawSetLineStyle(dc, style);

    XDrawRectangle(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1, y2 - y1);
  }
}

IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));

  iDrawSetLineWidth(dc, line_width);
  iDrawSetLineStyle(dc, style);

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
    iDrawSetLineWidth(dc, line_width);
    iDrawSetLineStyle(dc, style);

    XDrawArc(iupmot_display, dc->pixmap, dc->pixmap_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1, iupRound(a1 * 64), iupRound((a2 - a1) * 64));   /* angle = 1/64ths of a degree */
  }
}

IUP_SDK_API void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  int i;
  XPoint* pnt = (XPoint*)malloc(count*sizeof(XPoint)); /* XPoint uses short for coordinates */

  for (i = 0; i < count; i++)
  {
    pnt[i].x = (short)points[2*i];
    pnt[i].y = (short)points[2*i+1];
  }

  XSetForeground(iupmot_display, dc->pixmap_gc, iupmotColorGetPixel(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));

  if (style==IUP_DRAW_FILL)
    XFillPolygon(iupmot_display, dc->pixmap, dc->pixmap_gc, pnt, count, Complex, CoordModeOrigin);
  else
  {
    iDrawSetLineWidth(dc, line_width);
    iDrawSetLineStyle(dc, style);

    XDrawLines(iupmot_display, dc->pixmap, dc->pixmap_gc, pnt, count, CoordModeOrigin);
  }

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
    if (flags & IUP_DRAW_RIGHT)
    {
      width = XTextWidth(xfont, text, len);
      off_x = w - width;
      if (off_x < 0) off_x = 0;
    }
    else if (flags & IUP_DRAW_CENTER)
    {
      width = XTextWidth(xfont, text, len);
      off_x = (w - width) / 2;
      if (off_x < 0) off_x = 0;
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
        if (flags & IUP_DRAW_RIGHT)
        {
          width = XTextWidth(xfont, p, l_len);
          off_x = w - width;
          if (off_x < 0) off_x = 0;
        }
        else if (flags & IUP_DRAW_CENTER)
        {
          width = XTextWidth(xfont, p, l_len);
          off_x = (w - width) / 2;
          if (off_x < 0) off_x = 0;
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

  XCopyArea(iupmot_display, pixmap, dc->pixmap, dc->pixmap_gc, 0, 0, img_w, img_h, x, y);

  /* zoom not supported */
  (void)w;
  (void)h;
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
