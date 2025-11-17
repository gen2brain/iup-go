/** \file
 * \brief Draw Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

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

IUP_SDK_API void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
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

  /* Draw full ellipse using gdk_draw_arc with 360 degree span */
  /* angle in GDK is 1/64ths of a degree, so 360*64 = 23040 */
  gdk_draw_arc(dc->pixmap, dc->pixmap_gc, style == IUP_DRAW_FILL, x1, y1, x2 - x1, y2 - y1, 0, 23040);
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
  {
    gdk_draw_polygon(dc->pixmap, dc->pixmap_gc, TRUE, (GdkPoint*)points, count);
  }
  else
  {
    /* For stroked polygons, need to close the path by adding first point at the end */
    GdkPoint* closed_points;
    int use_heap = 0;
    GdkPoint stack_points[256];

    if (count + 1 <= 256)
      closed_points = stack_points;
    else
    {
      closed_points = (GdkPoint*)malloc((count + 1) * sizeof(GdkPoint));
      use_heap = 1;
    }

    /* Copy all points */
    memcpy(closed_points, points, count * sizeof(GdkPoint));
    /* Add first point at the end to close the polygon */
    closed_points[count] = closed_points[0];

    gdk_draw_lines(dc->pixmap, dc->pixmap_gc, closed_points, count + 1);

    if (use_heap)
      free(closed_points);
  }
}

IUP_SDK_API void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
{
  GdkColor c;
  iupgdkColorSet(&c, color);
  gdk_gc_set_rgb_fg_color(dc->pixmap_gc, &c);

  gdk_draw_point(dc->pixmap, dc->pixmap_gc, x, y);
}

IUP_SDK_API void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius, long color, int style, int line_width)
{
  GdkColor c;
  int diameter;

  iupgdkColorSet(&c, color);
  gdk_gc_set_rgb_fg_color(dc->pixmap_gc, &c);

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  /* Clamp radius to prevent oversized corners */
  int max_radius = ((x2 - x1) < (y2 - y1)) ? (x2 - x1) / 2 : (y2 - y1) / 2;
  if (corner_radius > max_radius)
    corner_radius = max_radius;

  diameter = corner_radius * 2;

  if (style != IUP_DRAW_FILL)
  {
    iDrawSetLineWidth(dc, line_width);
    iDrawSetLineStyle(dc, style);
  }

  if (style == IUP_DRAW_FILL)
  {
    /* Fill rounded rectangle by drawing filled arcs and rectangles */
    /* Top-right arc */
    gdk_draw_arc(dc->pixmap, dc->pixmap_gc, TRUE, x2 - diameter, y1, diameter, diameter, 0 * 64, 90 * 64);
    /* Bottom-right arc */
    gdk_draw_arc(dc->pixmap, dc->pixmap_gc, TRUE, x2 - diameter, y2 - diameter, diameter, diameter, 270 * 64, 90 * 64);
    /* Bottom-left arc */
    gdk_draw_arc(dc->pixmap, dc->pixmap_gc, TRUE, x1, y2 - diameter, diameter, diameter, 180 * 64, 90 * 64);
    /* Top-left arc */
    gdk_draw_arc(dc->pixmap, dc->pixmap_gc, TRUE, x1, y1, diameter, diameter, 90 * 64, 90 * 64);

    /* Fill center rectangle */
    gdk_draw_rectangle(dc->pixmap, dc->pixmap_gc, TRUE, x1 + corner_radius, y1, x2 - x1 - diameter + 1, y2 - y1 + 1);
    /* Fill left rectangle */
    gdk_draw_rectangle(dc->pixmap, dc->pixmap_gc, TRUE, x1, y1 + corner_radius, corner_radius, y2 - y1 - diameter + 1);
    /* Fill right rectangle */
    gdk_draw_rectangle(dc->pixmap, dc->pixmap_gc, TRUE, x2 - corner_radius + 1, y1 + corner_radius, corner_radius, y2 - y1 - diameter + 1);
  }
  else
  {
    /* Draw rounded rectangle by drawing arcs and lines */
    /* Top-right arc */
    gdk_draw_arc(dc->pixmap, dc->pixmap_gc, FALSE, x2 - diameter, y1, diameter, diameter, 0 * 64, 90 * 64);
    /* Bottom-right arc */
    gdk_draw_arc(dc->pixmap, dc->pixmap_gc, FALSE, x2 - diameter, y2 - diameter, diameter, diameter, 270 * 64, 90 * 64);
    /* Bottom-left arc */
    gdk_draw_arc(dc->pixmap, dc->pixmap_gc, FALSE, x1, y2 - diameter, diameter, diameter, 180 * 64, 90 * 64);
    /* Top-left arc */
    gdk_draw_arc(dc->pixmap, dc->pixmap_gc, FALSE, x1, y1, diameter, diameter, 90 * 64, 90 * 64);

    /* Draw connecting lines */
    /* Top line */
    gdk_draw_line(dc->pixmap, dc->pixmap_gc, x1 + corner_radius, y1, x2 - corner_radius, y1);
    /* Right line */
    gdk_draw_line(dc->pixmap, dc->pixmap_gc, x2, y1 + corner_radius, x2, y2 - corner_radius);
    /* Bottom line */
    gdk_draw_line(dc->pixmap, dc->pixmap_gc, x2 - corner_radius, y2, x1 + corner_radius, y2);
    /* Left line */
    gdk_draw_line(dc->pixmap, dc->pixmap_gc, x1, y2 - corner_radius, x1, y1 + corner_radius);
  }
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

IUP_SDK_API void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius)
{
  GdkRegion *region;
  GdkPoint points[100];
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
    points[num_points].x = x1 + corner_radius + (int)(corner_radius * cos(angle));
    points[num_points].y = y1 + corner_radius + (int)(corner_radius * sin(angle));
    num_points++;
  }

  /* Bottom-left corner arc: from left edge (180) to bottom edge (90) going clockwise */
  for (i = 1; i <= 8; i++)
  {
    angle = (180.0 - i * step) * pi / 180.0;
    points[num_points].x = x1 + corner_radius + (int)(corner_radius * cos(angle));
    points[num_points].y = y2 - corner_radius + (int)(corner_radius * sin(angle));
    num_points++;
  }

  /* Bottom-right corner arc: from bottom edge (90) to right edge (0) going clockwise */
  for (i = 1; i <= 8; i++)
  {
    angle = (90.0 - i * step) * pi / 180.0;
    points[num_points].x = x2 - corner_radius + (int)(corner_radius * cos(angle));
    points[num_points].y = y2 - corner_radius + (int)(corner_radius * sin(angle));
    num_points++;
  }

  /* Top-right corner arc: from right edge (0) to top edge (270 = -90) going clockwise */
  for (i = 1; i <= 8; i++)
  {
    angle = (0.0 - i * step) * pi / 180.0;
    points[num_points].x = x2 - corner_radius + (int)(corner_radius * cos(angle));
    points[num_points].y = y1 + corner_radius + (int)(corner_radius * sin(angle));
    num_points++;
  }

  /* Create region from polygon */
  region = gdk_region_polygon(points, num_points, GDK_WINDING_RULE);
  gdk_gc_set_clip_region(dc->pixmap_gc, region);
  gdk_region_destroy(region);

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

IUP_SDK_API void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
  /* GTK2 GDK does not have native Bezier support - use line approximation */
  GdkPoint points[21]; /* 20 segments should give smooth curve */
  int i, num_segments = 20;
  GdkColor c;

  iupgdkColorSet(&c, color);
  gdk_gc_set_rgb_fg_color(dc->pixmap_gc, &c);
  gdk_gc_set_line_attributes(dc->pixmap_gc, line_width, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);

  /* Generate points along Bezier curve using parametric equation */
  for (i = 0; i <= num_segments; i++)
  {
    double t = (double)i / num_segments;
    double t1 = 1.0 - t;
    double t1_3 = t1 * t1 * t1;
    double t1_2_t = 3.0 * t1 * t1 * t;
    double t1_t_2 = 3.0 * t1 * t * t;
    double t_3 = t * t * t;

    points[i].x = (gint)(t1_3 * x1 + t1_2_t * x2 + t1_t_2 * x3 + t_3 * x4);
    points[i].y = (gint)(t1_3 * y1 + t1_2_t * y2 + t1_t_2 * y3 + t_3 * y4);
  }

  if (style == IUP_DRAW_FILL)
  {
    /* Fill as polygon */
    gdk_draw_polygon(dc->pixmap, dc->pixmap_gc, TRUE, points, num_segments + 1);
  }
  else
  {
    /* Draw as polyline */
    gdk_draw_lines(dc->pixmap, dc->pixmap_gc, points, num_segments + 1);
  }
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

static long gdkInterpolateColor(long color1, long color2, float t)
{
  unsigned char r1 = iupDrawRed(color1), g1 = iupDrawGreen(color1), b1 = iupDrawBlue(color1), a1 = iupDrawAlpha(color1);
  unsigned char r2 = iupDrawRed(color2), g2 = iupDrawGreen(color2), b2 = iupDrawBlue(color2), a2 = iupDrawAlpha(color2);
  unsigned char r = (unsigned char)(r1 + t * (r2 - r1));
  unsigned char g = (unsigned char)(g1 + t * (g2 - g1));
  unsigned char b = (unsigned char)(b1 + t * (b2 - b1));
  unsigned char a = (unsigned char)(a1 + t * (a2 - a1));
  return iupDrawColor(r, g, b, a);
}

IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, long color1, long color2)
{
  int i, steps;
  float t, dx, dy, length;
  int px1, py1, px2, py2;
  GdkColor gdk_color;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  /* Calculate gradient direction */
  float rad = angle * 3.14159265359f / 180.0f;
  dx = (float)cos(rad);
  dy = (float)sin(rad);

  /* Number of steps for smooth gradient */
  length = (float)sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
  steps = (int)length;
  if (steps < 2) steps = 2;
  if (steps > 256) steps = 256;

  /* Draw gradient strips */
  for (i = 0; i < steps; i++)
  {
    t = (float)i / (float)(steps - 1);
    long color = gdkInterpolateColor(color1, color2, t);

    gdk_color.red = iupDrawRed(color) * 257;
    gdk_color.green = iupDrawGreen(color) * 257;
    gdk_color.blue = iupDrawBlue(color) * 257;
    gdk_gc_set_rgb_fg_color(dc->pixmap_gc, &gdk_color);

    /* Calculate strip position */
    if (fabs(dx) > fabs(dy))  /* More horizontal */
    {
      px1 = x1 + (int)(t * (x2 - x1));
      px2 = x1 + (int)((t + 1.0f / steps) * (x2 - x1));
      py1 = y1;
      py2 = y2;
    }
    else  /* More vertical */
    {
      px1 = x1;
      px2 = x2;
      py1 = y1 + (int)(t * (y2 - y1));
      py2 = y1 + (int)((t + 1.0f / steps) * (y2 - y1));
    }

    gdk_draw_rectangle(dc->pixmap, dc->pixmap_gc, TRUE, px1, py1, px2 - px1 + 1, py2 - py1 + 1);
  }
}

IUP_SDK_API void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, long colorCenter, long colorEdge)
{
  int i, steps;
  float t, r;
  GdkColor gdk_color;

  /* Number of steps for smooth gradient */
  steps = radius;
  if (steps < 2) steps = 2;
  if (steps > 256) steps = 256;

  /* Draw from outside to inside */
  for (i = steps - 1; i >= 0; i--)
  {
    t = (float)i / (float)(steps - 1);
    long color = gdkInterpolateColor(colorCenter, colorEdge, t);
    r = (float)radius * t;

    gdk_color.red = iupDrawRed(color) * 257;
    gdk_color.green = iupDrawGreen(color) * 257;
    gdk_color.blue = iupDrawBlue(color) * 257;
    gdk_gc_set_rgb_fg_color(dc->pixmap_gc, &gdk_color);

    gdk_draw_arc(dc->pixmap, dc->pixmap_gc, TRUE, (int)(cx - r), (int)(cy - r), (int)(2 * r), (int)(2 * r), 0, 23040);
  }
}

