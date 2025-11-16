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

/* This was build for GTK3 only, 
   but since 3.25 work with GTK2 too. */

struct _IdrawCanvas
{
  Ihandle* ih;
  int w, h;

  GtkWidget* widget;
  int release_cr;
  cairo_t *cr, *image_cr;
#if !GTK_CHECK_VERSION(3, 0, 0)
  GdkWindow* wnd;
  int draw_focus,
    focus_x1, focus_y1, focus_x2, focus_y2;
#endif

  int clip_x1, clip_y1, clip_x2, clip_y2;
};

IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = calloc(1, sizeof(IdrawCanvas));
  cairo_surface_t* surface;
  cairo_surface_t* buffer;

  dc->ih = ih;

  dc->widget = (GtkWidget*)IupGetAttribute(ih, "WID");

#if !GTK_CHECK_VERSION(3, 0, 0)
  dc->wnd = (GdkWindow*)IupGetAttribute(ih, "DRAWABLE");
  gdk_drawable_get_size(dc->wnd, &dc->w, &dc->h);
#else
  dc->w = gtk_widget_get_allocated_width(dc->widget);
  dc->h = gtk_widget_get_allocated_height(dc->widget);
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
  /* Check if we have persistent offscreen buffer */
  buffer = (cairo_surface_t*)iupAttribGet(ih, "_IUPGTK3_CANVAS_BUFFER");
  if (buffer)
  {
    /* Check if size changed - recreate buffer if needed */
    int buf_w = cairo_image_surface_get_width(buffer);
    int buf_h = cairo_image_surface_get_height(buffer);
    if (buf_w != dc->w || buf_h != dc->h)
    {
      cairo_surface_destroy(buffer);
      buffer = NULL;
    }
  }

  /* Create new buffer if needed */
  if (!buffer)
  {
    buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, dc->w, dc->h);
    iupAttribSet(ih, "_IUPGTK3_CANVAS_BUFFER", (char*)buffer);
  }
#endif

  /* valid only inside the ACTION callback of an IupCanvas */
  dc->cr = (cairo_t*)IupGetAttribute(ih, "CAIRO_CR");
  if (!dc->cr)
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    /* Outside ACTION callback - draw to persistent buffer */
    dc->cr = cairo_create(buffer);
#else
    dc->cr = gdk_cairo_create(dc->wnd);
#endif
    dc->release_cr = 1;
  }

  /* Create image surface for double-buffering the current operation */
  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, dc->w, dc->h);
  dc->image_cr = cairo_create(surface);
  cairo_surface_destroy(surface);

  iupAttribSet(ih, "DRAWDRIVER", "CAIRO");

  return dc;
}

IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  cairo_destroy(dc->image_cr);
  if (dc->release_cr)
    cairo_destroy(dc->cr);

  free(dc);
}

IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  int w, h;
#if !GTK_CHECK_VERSION(3, 0, 0)
  gdk_drawable_get_size(dc->wnd, &w, &h);
#else
  w = gtk_widget_get_allocated_width(dc->widget);
  h = gtk_widget_get_allocated_height(dc->widget);
#endif

  if (w != dc->w || h != dc->h)
  {
    cairo_surface_t* surface;

    dc->w = w;
    dc->h = h;

    cairo_destroy(dc->image_cr);

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, dc->w, dc->h);
    dc->image_cr = cairo_create(surface);
    cairo_surface_destroy(surface);
  }
}

#if !GTK_CHECK_VERSION(3, 0, 0)
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
#endif

IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc)
{
  /* Reset clip on image buffer to ensure clean state for next frame */
  iupdrvDrawResetClip(dc);

  /* Copy the image buffer to the target surface */
  cairo_set_source_surface(dc->cr, cairo_get_target(dc->image_cr), 0, 0);
  cairo_set_operator(dc->cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint(dc->cr);  /* paints the entire source surface */

#if !GTK_CHECK_VERSION(3, 0, 0)
  if (dc->draw_focus)
  {
    gdkDrawFocusRect(dc->ih, dc->focus_x1, dc->focus_y1, dc->focus_x2 - dc->focus_x1 + 1, dc->focus_y2 - dc->focus_y1 + 1);
    dc->draw_focus = 0;
  }
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
  /* If drawing to persistent buffer (outside ACTION), trigger widget repaint */
  if (dc->release_cr)
  {
    cairo_surface_t* buffer = (cairo_surface_t*)iupAttribGet(dc->ih, "_IUPGTK3_CANVAS_BUFFER");
    if (buffer)
    {
      cairo_surface_flush(buffer);
      /* Mark buffer as updated so gtkCanvasDraw knows to use it */
      iupAttribSet(dc->ih, "_IUPGTK3_BUFFER_DIRTY", "1");
    }

    /* Trigger widget repaint to copy buffer to screen (in ACTION callback) */
    gtk_widget_queue_draw(dc->widget);
  }
#endif
}

IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int *w, int *h)
{
  if (w) *w = dc->w;
  if (h) *h = dc->h;
}

static void iDrawSetLineStyle(IdrawCanvas* dc, int style)
{
  if (style == IUP_DRAW_STROKE || style == IUP_DRAW_FILL)
    cairo_set_dash(dc->image_cr, 0, 0, 0);
  else
  {
    if (style == IUP_DRAW_STROKE_DASH)
    {
      double dashes[2] = { 9.0, 3.0 };
      cairo_set_dash(dc->image_cr, dashes, 2, 0);
    }
    else if (style == IUP_DRAW_STROKE_DOT)
    {
      double dashes[2] = { 1.0, 2.0 };
      cairo_set_dash(dc->image_cr, dashes, 2, 0);
    }
    else if (style == IUP_DRAW_STROKE_DASH_DOT)
    {
      double dashes[4] = { 7.0, 3.0, 1.0, 3.0 };
      cairo_set_dash(dc->image_cr, dashes, 4, 0);
    }
    else if (style == IUP_DRAW_STROKE_DASH_DOT_DOT)
    {
      double dashes[6] = { 7.0, 3.0, 1.0, 3.0, 1.0, 3.0 };
      cairo_set_dash(dc->image_cr, dashes, 6, 0);
    }
  }
}

static void iDrawSetLineWidth(IdrawCanvas* dc, int line_width)
{
  cairo_set_line_width(dc->image_cr, (double)line_width);
}

static void iDrawVerticalLineW1(IdrawCanvas* dc, int x, int y1, int y2)
{
  /* Used only when line_width=1 */
  /* Use 0.5 to draw full pixel lines, add 1 to include the last pixel */
  iupDrawCheckSwapCoord(y1, y2);
  cairo_move_to(dc->image_cr, x + 0.5, y1);
  cairo_line_to(dc->image_cr, x + 0.5, y2 + 1);
}

static void iDrawHorizontalLineW1(IdrawCanvas* dc, int x1, int x2, int y)
{
  /* Used only when line_width=1 */
  /* Use 0.5 to draw full pixel lines, add 1 to include the last pixel */
  iupDrawCheckSwapCoord(x1, x2);
  cairo_move_to(dc->image_cr, x1, y + 0.5);
  cairo_line_to(dc->image_cr, x2 + 1, y + 0.5);
}

IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  cairo_set_source_rgba(dc->image_cr, iupgtkColorToDouble(iupDrawRed(color)), iupgtkColorToDouble(iupDrawGreen(color)), iupgtkColorToDouble(iupDrawBlue(color)), iupgtkColorToDouble(iupDrawAlpha(color)));

  if (style==IUP_DRAW_FILL)
  {
    iupDrawCheckSwapCoord(x1, x2);
    iupDrawCheckSwapCoord(y1, y2);

    cairo_new_path(dc->image_cr);
    cairo_rectangle(dc->image_cr, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
    cairo_fill(dc->image_cr);
  }
  else
  {
    iDrawSetLineWidth(dc, line_width);
    iDrawSetLineStyle(dc, style);

    cairo_new_path(dc->image_cr);

    if (line_width == 1)
    {
      iDrawHorizontalLineW1(dc, x1, x2, y1);
      iDrawVerticalLineW1  (dc, x2, y1, y2);
      iDrawHorizontalLineW1(dc, x2, x1, y2);
      iDrawVerticalLineW1  (dc, x1, y2, y1);
    }
    else
    {
      cairo_move_to(dc->image_cr, x1, y1);
      cairo_line_to(dc->image_cr, x2, y1);
      cairo_line_to(dc->image_cr, x2, y2);
      cairo_line_to(dc->image_cr, x1, y2);
      cairo_close_path(dc->image_cr);
    }

    cairo_stroke(dc->image_cr);
  }
}

IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  cairo_set_source_rgba(dc->image_cr, iupgtkColorToDouble(iupDrawRed(color)), iupgtkColorToDouble(iupDrawGreen(color)), iupgtkColorToDouble(iupDrawBlue(color)), iupgtkColorToDouble(iupDrawAlpha(color)));

  iDrawSetLineWidth(dc, line_width);
  iDrawSetLineStyle(dc, style);

  cairo_new_path(dc->image_cr);

  if (x1 == x2 && line_width == 1)
    iDrawVerticalLineW1(dc, x1, y1, y2);
  else if (y1 == y2 && line_width == 1)
    iDrawHorizontalLineW1(dc, x1, x2, y1);
  else
  {
    cairo_move_to(dc->image_cr, x1, y1);
    cairo_line_to(dc->image_cr, x2, y2);
  }

  cairo_stroke(dc->image_cr);
}

static void iFixAngles(double *a1, double *a2)
{
  /* Cairo angles are clock-wise by default, in radians */

  /* change orientation */
  *a1 *= -1;
  *a2 *= -1;

  /* swap, so the start angle is the smaller */
  {
    double t = *a1;
    *a1 = *a2;
    *a2 = t;
  }

  /* convert to radians */
  *a1 *= IUP_DEG2RAD;
  *a2 *= IUP_DEG2RAD;
}

IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  double xc, yc, w, h;

  cairo_set_source_rgba(dc->image_cr, iupgtkColorToDouble(iupDrawRed(color)), iupgtkColorToDouble(iupDrawGreen(color)), iupgtkColorToDouble(iupDrawBlue(color)), iupgtkColorToDouble(iupDrawAlpha(color)));

  if (style != IUP_DRAW_FILL)
  {
    iDrawSetLineWidth(dc, line_width);
    iDrawSetLineStyle(dc, style);
  }

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  /* using x2-x1+1 was resulting in a pixel larger arc */
  w = x2 - x1;
  h = y2 - y1;
  xc = x1 + w/2.0;
  yc = y1 + h/2.0;

  iFixAngles(&a1, &a2);

  if (w == h)
  {
    cairo_new_path(dc->image_cr);

    if (style == IUP_DRAW_FILL)
      cairo_move_to(dc->image_cr, xc, yc);

    cairo_arc(dc->image_cr, xc, yc, 0.5*w, a1, a2);

    if (style==IUP_DRAW_FILL)
      cairo_fill(dc->image_cr);
    else
      cairo_stroke(dc->image_cr);
  }
  else  /* Ellipse: change the scale to create from the circle */
  {
    cairo_save(dc->image_cr);  /* save to use the local transform */

    cairo_new_path(dc->image_cr);

    cairo_translate(dc->image_cr, xc, yc);
    cairo_scale(dc->image_cr, w/h, 1.0);
    cairo_translate(dc->image_cr, -xc, -yc);

    if (style == IUP_DRAW_FILL)
      cairo_move_to(dc->image_cr, xc, yc);

    cairo_arc(dc->image_cr, xc, yc, 0.5*h, a1, a2);

    if (style==IUP_DRAW_FILL)
      cairo_fill(dc->image_cr);
    else
      cairo_stroke(dc->image_cr);

    cairo_restore(dc->image_cr);  /* restore from local */
  }
}

IUP_SDK_API void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  int i;

  cairo_set_source_rgba(dc->image_cr, iupgtkColorToDouble(iupDrawRed(color)), iupgtkColorToDouble(iupDrawGreen(color)), iupgtkColorToDouble(iupDrawBlue(color)), iupgtkColorToDouble(iupDrawAlpha(color)));

  if (style!=IUP_DRAW_FILL)
  {
    iDrawSetLineWidth(dc, line_width);
    iDrawSetLineStyle(dc, style);
  }

  cairo_new_path(dc->image_cr);

  cairo_move_to(dc->image_cr, points[0], points[1]);
  for (i=1; i<count; i++)  /* Start at 1 to avoid redundant line to first point */
    cairo_line_to(dc->image_cr, points[2*i], points[2*i+1]);

  if (style==IUP_DRAW_FILL)
    cairo_fill(dc->image_cr);
  else
    cairo_stroke(dc->image_cr);
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
  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }

  /* make it an empty region */
  if (x1 >= x2) x1 = x2;
  if (y1 >= y2) y1 = y2;

  iupdrvDrawResetClip(dc);

  cairo_rectangle(dc->image_cr, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  cairo_clip(dc->image_cr);  /* intersect with the current clipping */

  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
}

IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  cairo_reset_clip(dc->image_cr);

  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = 0;
  dc->clip_y2 = 0;
}

IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  PangoLayout* fontlayout = (PangoLayout*)iupgtkGetPangoLayout(font);
  PangoAlignment alignment = PANGO_ALIGN_LEFT;
  int layout_w = w, layout_h = h;
  int layout_center = flags & IUP_DRAW_LAYOUTCENTER;

  if (text_orientation && layout_center)
    iupDrawGetTextSize(dc->ih, text, len, &layout_w, &layout_h, 0);

  text = iupgtkStrConvertToSystemLen(text, &len);
  pango_layout_set_text(fontlayout, text, len);

  if (flags & IUP_DRAW_RIGHT)
    alignment = PANGO_ALIGN_RIGHT;
  else if (flags & IUP_DRAW_CENTER)
    alignment = PANGO_ALIGN_CENTER;

  pango_layout_set_alignment(fontlayout, alignment);

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
  
  cairo_set_source_rgba(dc->image_cr, iupgtkColorToDouble(iupDrawRed(color)), iupgtkColorToDouble(iupDrawGreen(color)), iupgtkColorToDouble(iupDrawBlue(color)), iupgtkColorToDouble(iupDrawAlpha(color)));

  if (flags & IUP_DRAW_CLIP)
  {
    cairo_save(dc->image_cr);
    cairo_rectangle(dc->image_cr, x, y, w, h);
    cairo_clip(dc->image_cr); /* intersect with the current clipping */
  }

  if (text_orientation)
  {
    if (layout_center)
    {
      cairo_translate(dc->image_cr, (w - layout_w) / 2, (h - layout_h) / 2);   /* transformations in Cairo is always prepend */

      cairo_translate(dc->image_cr, x + layout_w / 2, y + layout_h / 2);
      cairo_rotate(dc->image_cr, -text_orientation*IUP_DEG2RAD);  /* counterclockwise */
      cairo_translate(dc->image_cr, -(x + layout_w / 2), -(y + layout_h / 2));
    }
    else
    {
      cairo_translate(dc->image_cr, x, y);
      cairo_rotate(dc->image_cr, -text_orientation*IUP_DEG2RAD);  /* counterclockwise */
      cairo_translate(dc->image_cr, -x, -y);
    }
  }

  pango_cairo_update_layout(dc->image_cr, fontlayout);

  cairo_move_to(dc->image_cr, x, y);
  pango_cairo_show_layout(dc->image_cr, fontlayout);

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
  if (text_orientation)
    cairo_identity_matrix(dc->image_cr);
  if (flags & IUP_DRAW_CLIP)
    cairo_restore(dc->image_cr);
}

IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, int x, int y, int w, int h)
{
  int bpp, img_w, img_h;
  GdkPixbuf* pixbuf = iupImageGetImage(name, dc->ih, make_inactive, bgcolor);
  if (!pixbuf)
    return;

  /* must use this info, since image can be a driver image loaded from resources */
  iupdrvImageGetInfo(pixbuf, &img_w, &img_h, &bpp);

  if (w == -1 || w == 0) w = img_w;
  if (h == -1 || h == 0) h = img_h;

  cairo_save (dc->image_cr);

  cairo_rectangle(dc->image_cr, x, y, w, h);
  cairo_clip(dc->image_cr); /* intersect with the current clipping */

  if (w != img_w || h != img_h)
  {
    /* Scale *before* setting the source surface (1) */
    cairo_translate(dc->image_cr, x, y);
    cairo_scale(dc->image_cr, (double)w / img_w, (double)h / img_h);
    cairo_translate(dc->image_cr, -x, -y);
  }

  gdk_cairo_set_source_pixbuf(dc->image_cr, pixbuf, x, y);
  cairo_paint(dc->image_cr);  /* paints the current source everywhere within the current clip region. */

  /* must restore clipping */
  cairo_restore(dc->image_cr);
}

IUP_SDK_API void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  cairo_set_source_rgba(dc->image_cr, 0, 0, 1, 0.6);  /* R=0, G=0, B=255, A=153 (blue semi-transparent) */

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  cairo_new_path(dc->image_cr);
  cairo_rectangle(dc->image_cr, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  cairo_fill(dc->image_cr);
}

IUP_SDK_API void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
#if 0
#if GTK_CHECK_VERSION(3, 0, 0)
  GtkStyleContext* context = gtk_widget_get_style_context(dc->widget);
#endif

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

#if !GTK_CHECK_VERSION(3, 0, 0)
  dc->draw_focus = 1;  /* draw focus on the next flush */
  dc->focus_x1 = x1;
  dc->focus_y1 = y1;
  dc->focus_x2 = x2;
  dc->focus_y2 = y2;
#else
  gtk_render_focus(context, dc->image_cr, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
#endif
#else
  iupdrvDrawRectangle(dc, x1, y1, x2, y2, iupDrawColor(0, 0, 0, 224), IUP_DRAW_STROKE_DOT, 1);
#endif
}


