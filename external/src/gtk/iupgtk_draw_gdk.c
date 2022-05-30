/** \file
 * \brief Draw Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

#include <gtk/gtk.h>

#include "iup.h"

#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_object.h"
#include "iup_image.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"

#include "iupgtk_drv.h"


struct _IdrawCanvas{
  Ihandle* ih;
  int w, h;

  GdkDrawable* wnd;
  GdkPixmap* pixmap;
  GdkGC *gc, *pixmap_gc;

  int draw_focus, 
    focus_x1, focus_y1, focus_x2, focus_y2;

  int clip_x1, clip_y1, clip_x2, clip_y2;
};

static void iupgdkColorSet(GdkColor* c, long color)
{
  c->red = iupCOLOR8TO16(iupDrawRed(color));
  c->green = iupCOLOR8TO16(iupDrawGreen(color));
  c->blue = iupCOLOR8TO16(iupDrawBlue(color));
  c->pixel = 0;
}

IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = calloc(1, sizeof(IdrawCanvas));

  dc->ih = ih;
  dc->wnd = (GdkWindow*)IupGetAttribute(ih, "DRAWABLE");
  dc->gc = gdk_gc_new(dc->wnd);

  gdk_drawable_get_size(dc->wnd, &dc->w, &dc->h);

  dc->pixmap = gdk_pixmap_new(dc->wnd, dc->w, dc->h, gdk_drawable_get_depth(dc->wnd));
  dc->pixmap_gc = gdk_gc_new(dc->pixmap);

  iupAttribSet(ih, "DRAWDRIVER", "GDK");

  return dc;
}

IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  g_object_unref(dc->pixmap_gc); 
  g_object_unref(dc->pixmap); 
  g_object_unref(dc->gc); 

  free(dc);
}

IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  int w, h;
  gdk_drawable_get_size(dc->wnd, &w, &h);

  if (w != dc->w || h != dc->h)
  {
    dc->w = w;
    dc->h = h;

    g_object_unref(dc->pixmap_gc); 
    g_object_unref(dc->pixmap); 

    dc->pixmap = gdk_pixmap_new(dc->wnd, dc->w, dc->h, gdk_drawable_get_depth(dc->wnd));
    dc->pixmap_gc = gdk_gc_new(dc->pixmap);
  }
}

static void gdkDrawFocusRect(Ihandle* ih, int x, int y, int w, int h)
{
  GdkWindow* window = iupgtkGetWindow(ih->handle);
  GtkStyle *style = gtk_widget_get_style(ih->handle);
#if GTK_CHECK_VERSION(2, 18, 0)
  GtkStateType state = gtk_widget_get_state(ih->handle);
#else
  GtkStateType state = GTK_WIDGET_STATE(ih->handle);
#endif
  gtk_paint_focus(style, window, state, NULL, ih->handle, NULL, x, y, w, h);
}

IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc)
{
  gdk_draw_drawable(dc->wnd, dc->gc, dc->pixmap, 0, 0, 0, 0, dc->w, dc->h);

  if (dc->draw_focus)
  {
    gdkDrawFocusRect(dc->ih, dc->focus_x1, dc->focus_y1, dc->focus_x2 - dc->focus_x1 + 1, dc->focus_y2 - dc->focus_y1 + 1);
    dc->draw_focus = 0;
  }
}

IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int *w, int *h)
{
  if (w) *w = dc->w;
  if (h) *h = dc->h;
}

static void iDrawSetLineStyle(IdrawCanvas* dc, int style)
{
  GdkGCValues gcval;
  if (style == IUP_DRAW_STROKE || style == IUP_DRAW_FILL)
    gcval.line_style = GDK_LINE_SOLID;
  else
  {
    if (style == IUP_DRAW_STROKE_DASH)
    {
      gint8 dashes[2] = { 9, 3 };
      gdk_gc_set_dashes(dc->pixmap_gc, 0, dashes, 2);
    }
    else if (style == IUP_DRAW_STROKE_DOT)
    {
      gint8 dashes[2] = { 1, 2 };
      gdk_gc_set_dashes(dc->pixmap_gc, 0, dashes, 2);
    }
    else if (style == IUP_DRAW_STROKE_DASH_DOT)
    {
      gint8 dashes[4] = { 7, 3, 1, 3 };
      gdk_gc_set_dashes(dc->pixmap_gc, 0, dashes, 4);
    }
    else if (style == IUP_DRAW_STROKE_DASH_DOT_DOT)
    {
      gint8 dashes[6] = { 7, 3, 1, 3, 1, 3 };
      gdk_gc_set_dashes(dc->pixmap_gc, 0, dashes, 6);
    }

    gcval.line_style = GDK_LINE_ON_OFF_DASH;
  }

  gdk_gc_set_values(dc->pixmap_gc, &gcval, GDK_GC_LINE_STYLE);
}

static void iDrawSetLineWidth(IdrawCanvas* dc, int line_width)
{
  GdkGCValues gcval;

  if (line_width == 1)
    gcval.line_width = 0;
  else
    gcval.line_width = line_width;

  gdk_gc_set_values(dc->pixmap_gc, &gcval, GDK_GC_LINE_WIDTH);
}

IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  GdkColor c;
  iupgdkColorSet(&c, color);
  gdk_gc_set_rgb_fg_color(dc->pixmap_gc, &c);

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  if (style==IUP_DRAW_FILL)
    gdk_draw_rectangle(dc->pixmap, dc->pixmap_gc, TRUE, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  else
  {
    iDrawSetLineWidth(dc, line_width);
    iDrawSetLineStyle(dc, style);

    gdk_draw_rectangle(dc->pixmap, dc->pixmap_gc, FALSE, x1, y1, x2 - x1, y2 - y1);  /* outlined rectangle is actually of size w+1,h+1 */
  }
}

IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  GdkColor c;
  iupgdkColorSet(&c, color);
  gdk_gc_set_rgb_fg_color(dc->pixmap_gc, &c);

  iDrawSetLineWidth(dc, line_width);
  iDrawSetLineStyle(dc, style);

  gdk_draw_line(dc->pixmap, dc->pixmap_gc, x1, y1, x2, y2);
}

IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  GdkColor c;
  iupgdkColorSet(&c, color);
  gdk_gc_set_rgb_fg_color(dc->pixmap_gc, &c);

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  if (style != IUP_DRAW_FILL)
  {
    iDrawSetLineWidth(dc, line_width);
    iDrawSetLineStyle(dc, style);
  }

  /* using x2-x1+1 was resulting in a pixel larger arc */
  gdk_draw_arc(dc->pixmap, dc->pixmap_gc, style == IUP_DRAW_FILL, x1, y1, x2 - x1, y2 - y1, iupRound(a1 * 64), iupRound((a2 - a1) * 64));    /* angle = 1/64ths of a degree */
}

IUP_SDK_API void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  GdkColor c;
  iupgdkColorSet(&c, color);
  gdk_gc_set_rgb_fg_color(dc->pixmap_gc, &c);

  if (style != IUP_DRAW_FILL)
  {
    iDrawSetLineWidth(dc, line_width);
    iDrawSetLineStyle(dc, style);
  }

  if (style == IUP_DRAW_FILL)
    gdk_draw_polygon(dc->pixmap, dc->pixmap_gc, TRUE, (GdkPoint*)points, count);
  else
    gdk_draw_lines(dc->pixmap, dc->pixmap_gc, (GdkPoint*)points, count);
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
  GdkRectangle rect;

  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }

  /* make it an empty region */
  if (x1 >= x2) x1 = x2;
  if (y1 >= y2) y1 = y2;

  rect.x = x1;
  rect.y = y1;
  rect.width = x2 - x1 + 1;
  rect.height = y2 - y1 + 1;

  gdk_gc_set_clip_rectangle(dc->pixmap_gc, &rect);

  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
}

IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  gdk_gc_set_clip_region(dc->pixmap_gc, NULL);

  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = 0;
  dc->clip_y2 = 0;
}

IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  PangoLayout* fontlayout = (PangoLayout*)iupgtkGetPangoLayout(font);
  PangoAlignment alignment = PANGO_ALIGN_LEFT;
  GdkColor c;
  PangoContext* fontcontext = NULL;
  int layout_w = w, layout_h = h;
  int layout_center = flags & IUP_DRAW_LAYOUTCENTER;

  if (text_orientation && layout_center)
    iupDrawGetTextSize(dc->ih, text, len, &layout_w, &layout_h, 0);

  iupgdkColorSet(&c, color);
  gdk_gc_set_rgb_fg_color(dc->pixmap_gc, &c);

  text = iupgtkStrConvertToSystemLen(text, &len);
  pango_layout_set_text(fontlayout, text, len);

  if (flags & IUP_DRAW_RIGHT)
    alignment = PANGO_ALIGN_RIGHT;
  else if (flags & IUP_DRAW_CENTER)
    alignment = PANGO_ALIGN_CENTER;

  if (flags & IUP_DRAW_WRAP)
  {
    pango_layout_set_width(fontlayout, iupGTK_PIXELS2PANGOUNITS(layout_w));
#ifdef PANGO_VERSION_CHECK
#if PANGO_VERSION_CHECK(1,2,0)  
    pango_layout_set_height(fontlayout, iupGTK_PIXELS2PANGOUNITS(layout_h));
#endif
#endif
  }
  else if (flags & IUP_DRAW_ELLIPSIS)
  {
    pango_layout_set_width(fontlayout, iupGTK_PIXELS2PANGOUNITS(layout_w));
#ifdef PANGO_VERSION_CHECK
#if PANGO_VERSION_CHECK(1,2,0)  
    pango_layout_set_height(fontlayout, iupGTK_PIXELS2PANGOUNITS(layout_h));
#endif
#endif
    pango_layout_set_ellipsize(fontlayout, PANGO_ELLIPSIZE_END);
  }

  pango_layout_set_alignment(fontlayout, alignment);

  if (flags & IUP_DRAW_CLIP)
  {
    GdkRectangle rect;
    rect.x = x;
    rect.y = y;
    rect.width = w;
    rect.height = h;
    gdk_gc_set_clip_rectangle(dc->pixmap_gc, &rect);
  }

  if (text_orientation)
  {
    PangoRectangle rect;
    PangoMatrix fontmatrix = PANGO_MATRIX_INIT;
    fontcontext = pango_layout_get_context(fontlayout);

    pango_matrix_rotate(&fontmatrix, text_orientation);

    pango_context_set_matrix(fontcontext, &fontmatrix);
    pango_layout_context_changed(fontlayout);

    pango_layout_get_pixel_extents(fontlayout, NULL, &rect);
#ifdef PANGO_VERSION_CHECK
#if PANGO_VERSION_CHECK(1,16,0)
    pango_matrix_transform_pixel_rectangle(&fontmatrix, &rect);
#endif
#endif

    /* Adjust the position considering the Pango rectangle transformed */
    if (layout_center)
    {
      x += (w - rect.width) / 2;
      y += (h - rect.height) / 2;
    }
    else
    {
      x += (int)rect.x;
      y += (int)rect.y;
    }
  }

  gdk_draw_layout(dc->pixmap, dc->pixmap_gc, x, y, fontlayout);

  /* restore settings */
  if ((flags & IUP_DRAW_WRAP) || (flags & IUP_DRAW_ELLIPSIS))
  {
    pango_layout_set_width(fontlayout, -1);
#ifdef PANGO_VERSION_CHECK
#if PANGO_VERSION_CHECK(1,2,0)  
    pango_layout_set_height(fontlayout, -1);
#endif
#endif
  }
  if (flags & IUP_DRAW_ELLIPSIS)
    pango_layout_set_ellipsize(fontlayout, PANGO_ELLIPSIZE_NONE);
  if (flags & IUP_DRAW_CLIP)
    gdk_gc_set_clip_region(dc->pixmap_gc, NULL);
  if (text_orientation)
    pango_context_set_matrix(fontcontext, NULL);
}

IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, int x, int y, int w, int h)
{
  int bpp, img_w, img_h;
  GdkPixbuf* pixbuf = iupImageGetImage(name, dc->ih, make_inactive, bgcolor);
  if (!pixbuf)
    return;

  /* must use this info, since image can be a driver image loaded from resources */
  iupdrvImageGetInfo(pixbuf, &img_w, &img_h, &bpp);

  gdk_draw_pixbuf(dc->pixmap, dc->pixmap_gc, pixbuf, 0, 0, x, y, img_w, img_h, GDK_RGB_DITHER_NORMAL, 0, 0);

  /* zoom not supported */
  (void)w;
  (void)h;
}

IUP_SDK_API void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  GdkColor c;
  iupgdkColorSetRGB(&c, 255, 255, 255);
  gdk_gc_set_rgb_fg_color(dc->pixmap_gc, &c);

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  gdk_gc_set_function(dc->pixmap_gc, GDK_XOR);
  gdk_draw_rectangle(dc->pixmap, dc->pixmap_gc, TRUE, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  gdk_gc_set_function(dc->pixmap_gc, GDK_COPY);
}

IUP_SDK_API void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  dc->draw_focus = 1;  /* draw focus on the next flush */
  dc->focus_x1 = x1;
  dc->focus_y1 = y1;
  dc->focus_x2 = x2;
  dc->focus_y2 = y2;
}

