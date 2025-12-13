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

#include "iupgtk4_drv.h"


struct _IdrawCanvas
{
  Ihandle* ih;
  int w, h;

  GtkWidget* widget;
  int release_cr;
  cairo_t *cr, *image_cr;

  int clip_x1, clip_y1, clip_x2, clip_y2;
};

IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = calloc(1, sizeof(IdrawCanvas));
  cairo_surface_t* buffer;

  dc->ih = ih;

  /* Use ih->handle directly instead of WID attribute to avoid dangling pointers */
  dc->widget = (GtkWidget*)ih->handle;

  /* Use stored width/height from gtk4CanvasDraw instead of calling
   * gtk_widget_get_width() which triggers CSS recalculation and
   * frees the style object that GTK is currently using for rendering. */
  dc->w = iupAttribGetInt(ih, "_IUPGTK4_DRAW_WIDTH");
  dc->h = iupAttribGetInt(ih, "_IUPGTK4_DRAW_HEIGHT");

  /* Fallback for calls outside ACTION callback */
  if (dc->w == 0) dc->w = gtk_widget_get_width(dc->widget);
  if (dc->h == 0) dc->h = gtk_widget_get_height(dc->widget);

  /* Check if we have persistent offscreen buffer */
  buffer = (cairo_surface_t*)iupAttribGet(ih, "_IUPGTK4_CANVAS_BUFFER");
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
    iupAttribSet(ih, "_IUPGTK4_CANVAS_BUFFER", (char*)buffer);
  }

  /* Always draw to our persistent image buffer first.
   * We copy the buffer to GTK's recording surface in DrawFlush. */
  dc->cr = cairo_create(buffer);
  dc->image_cr = dc->cr;
  dc->release_cr = 1;

  iupAttribSet(ih, "DRAWDRIVER", "CAIRO");

  return dc;
}

IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  /* We always create our own cairo context from the image buffer */
  if (dc->release_cr)
    cairo_destroy(dc->cr);

  free(dc);
}

IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  int w, h;

  w = gtk_widget_get_width(dc->widget);
  h = gtk_widget_get_height(dc->widget);

  if (w != dc->w || h != dc->h)
  {
    dc->w = w;
    dc->h = h;
  }
}

IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc)
{
  cairo_surface_t* buffer;
  cairo_t* gtk_cr;

  /* Reset clip to ensure clean state */
  iupdrvDrawResetClip(dc);

  /* Flush our image buffer */
  buffer = (cairo_surface_t*)iupAttribGet(dc->ih, "_IUPGTK4_CANVAS_BUFFER");
  if (buffer)
    cairo_surface_flush(buffer);

  /* If we're inside ACTION callback, copy our buffer to GTK's recording surface */
  gtk_cr = (cairo_t*)IupGetAttribute(dc->ih, "CAIRO_CR");
  if (gtk_cr && buffer)
  {
    /* Copy our image surface to GTK's recording surface */
    cairo_set_source_surface(gtk_cr, buffer, 0, 0);
    cairo_paint(gtk_cr);

    /* Clear the source pattern to release the surface reference.
     * cairo_set_source_surface() creates a pattern that holds a reference to the buffer. */
    cairo_set_source_rgb(gtk_cr, 0, 0, 0);
  }
  else
  {
    /* Outside ACTION callback - trigger widget repaint */
    gtk_widget_queue_draw(dc->widget);
  }
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
  cairo_set_source_rgba(dc->image_cr, iupgtk4ColorToDouble(iupDrawRed(color)), iupgtk4ColorToDouble(iupDrawGreen(color)), iupgtk4ColorToDouble(iupDrawBlue(color)), iupgtk4ColorToDouble(iupDrawAlpha(color)));

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
  cairo_set_source_rgba(dc->image_cr, iupgtk4ColorToDouble(iupDrawRed(color)), iupgtk4ColorToDouble(iupDrawGreen(color)), iupgtk4ColorToDouble(iupDrawBlue(color)), iupgtk4ColorToDouble(iupDrawAlpha(color)));

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

  cairo_set_source_rgba(dc->image_cr, iupgtk4ColorToDouble(iupDrawRed(color)), iupgtk4ColorToDouble(iupDrawGreen(color)), iupgtk4ColorToDouble(iupDrawBlue(color)), iupgtk4ColorToDouble(iupDrawAlpha(color)));

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

IUP_SDK_API void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  double xc, yc, w, h;

  cairo_set_source_rgba(dc->image_cr, iupgtk4ColorToDouble(iupDrawRed(color)), iupgtk4ColorToDouble(iupDrawGreen(color)), iupgtk4ColorToDouble(iupDrawBlue(color)), iupgtk4ColorToDouble(iupDrawAlpha(color)));

  if (style != IUP_DRAW_FILL)
  {
    iDrawSetLineWidth(dc, line_width);
    iDrawSetLineStyle(dc, style);
  }

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  w = x2 - x1;
  h = y2 - y1;
  xc = x1 + w/2.0;
  yc = y1 + h/2.0;

  if (w == h)
  {
    /* Circle: simple arc */
    cairo_new_path(dc->image_cr);
    cairo_arc(dc->image_cr, xc, yc, 0.5*w, 0, 2*G_PI);

    if (style == IUP_DRAW_FILL)
      cairo_fill(dc->image_cr);
    else
      cairo_stroke(dc->image_cr);
  }
  else
  {
    /* Ellipse: use scale transform to create from circle */
    cairo_save(dc->image_cr);

    cairo_new_path(dc->image_cr);

    cairo_translate(dc->image_cr, xc, yc);
    cairo_scale(dc->image_cr, w/h, 1.0);
    cairo_translate(dc->image_cr, -xc, -yc);

    cairo_arc(dc->image_cr, xc, yc, 0.5*h, 0, 2*G_PI);

    if (style == IUP_DRAW_FILL)
      cairo_fill(dc->image_cr);
    else
      cairo_stroke(dc->image_cr);

    cairo_restore(dc->image_cr);
  }
}

IUP_SDK_API void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  int i;

  cairo_set_source_rgba(dc->image_cr, iupgtk4ColorToDouble(iupDrawRed(color)), iupgtk4ColorToDouble(iupDrawGreen(color)), iupgtk4ColorToDouble(iupDrawBlue(color)), iupgtk4ColorToDouble(iupDrawAlpha(color)));

  if (style!=IUP_DRAW_FILL)
  {
    iDrawSetLineWidth(dc, line_width);
    iDrawSetLineStyle(dc, style);
  }

  cairo_new_path(dc->image_cr);

  cairo_move_to(dc->image_cr, points[0], points[1]);
  for (i=1; i<count; i++)  /* Start at 1 to avoid redundant line to first point */
    cairo_line_to(dc->image_cr, points[2*i], points[2*i+1]);

  cairo_close_path(dc->image_cr);  /* Close polygon by connecting last point to first */

  if (style==IUP_DRAW_FILL)
    cairo_fill(dc->image_cr);
  else
    cairo_stroke(dc->image_cr);
}

IUP_SDK_API void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
{
  cairo_set_source_rgba(dc->image_cr,
    iupgtk4ColorToDouble(iupDrawRed(color)),
    iupgtk4ColorToDouble(iupDrawGreen(color)),
    iupgtk4ColorToDouble(iupDrawBlue(color)),
    iupgtk4ColorToDouble(iupDrawAlpha(color)));

  cairo_new_path(dc->image_cr);
  cairo_rectangle(dc->image_cr, x, y, 1, 1);
  cairo_fill(dc->image_cr);
}

IUP_SDK_API void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2,
                                             int corner_radius, long color, int style, int line_width)
{
  double radius = (double)corner_radius;
  double degrees = IUP_DEG2RAD;

  cairo_set_source_rgba(dc->image_cr,
    iupgtk4ColorToDouble(iupDrawRed(color)),
    iupgtk4ColorToDouble(iupDrawGreen(color)),
    iupgtk4ColorToDouble(iupDrawBlue(color)),
    iupgtk4ColorToDouble(iupDrawAlpha(color)));

  if (style != IUP_DRAW_FILL)
  {
    iDrawSetLineWidth(dc, line_width);
    iDrawSetLineStyle(dc, style);
  }

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  /* Clamp radius to half of smallest dimension */
  double max_radius = ((x2 - x1) < (y2 - y1)) ? (x2 - x1) / 2.0 : (y2 - y1) / 2.0;
  if (radius > max_radius)
    radius = max_radius;

  cairo_new_path(dc->image_cr);

  /* Top-right corner */
  cairo_arc(dc->image_cr, x2 - radius, y1 + radius, radius, -90 * degrees, 0);

  /* Right side + bottom-right corner */
  cairo_line_to(dc->image_cr, x2, y2 - radius);
  cairo_arc(dc->image_cr, x2 - radius, y2 - radius, radius, 0, 90 * degrees);

  /* Bottom side + bottom-left corner */
  cairo_line_to(dc->image_cr, x1 + radius, y2);
  cairo_arc(dc->image_cr, x1 + radius, y2 - radius, radius, 90 * degrees, 180 * degrees);

  /* Left side + top-left corner */
  cairo_line_to(dc->image_cr, x1, y1 + radius);
  cairo_arc(dc->image_cr, x1 + radius, y1 + radius, radius, 180 * degrees, 270 * degrees);

  cairo_close_path(dc->image_cr);

  if (style == IUP_DRAW_FILL)
    cairo_fill(dc->image_cr);
  else
    cairo_stroke(dc->image_cr);
}

IUP_SDK_API void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
  /* Set color */
  cairo_set_source_rgba(dc->image_cr,
                        iupgtk4ColorToDouble(iupDrawRed(color)),
                        iupgtk4ColorToDouble(iupDrawGreen(color)),
                        iupgtk4ColorToDouble(iupDrawBlue(color)),
                        iupgtk4ColorToDouble(iupDrawAlpha(color)));

  /* Set line style for stroked curves */
  if (style != IUP_DRAW_FILL)
  {
    iDrawSetLineWidth(dc, line_width);
    iDrawSetLineStyle(dc, style);
  }

  /* Draw cubic Bezier curve */
  cairo_move_to(dc->image_cr, x1, y1);
  cairo_curve_to(dc->image_cr, x2, y2, x3, y3, x4, y4);

  if (style == IUP_DRAW_FILL)
    cairo_fill(dc->image_cr);
  else
    cairo_stroke(dc->image_cr);
}

IUP_SDK_API void iupdrvDrawQuadraticBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, long color, int style, int line_width)
{
  /* Convert quadratic Bezier to cubic Bezier using the 2/3 formula:
   * Given quadratic: Q(t) with control points q0, q1, q2
   * Convert to cubic: C(t) with control points c0, c1, c2, c3
   *
   * c0 = q0                        (start point)
   * c1 = q0 + (2/3) * (q1 - q0)   (first control point)
   * c2 = q2 + (2/3) * (q1 - q2)   (second control point)
   * c3 = q2                        (end point)
   */
  int cx1, cy1, cx2, cy2;

  /* Calculate cubic control points from quadratic */
  cx1 = x1 + ((2 * (x2 - x1)) / 3);
  cy1 = y1 + ((2 * (y2 - y1)) / 3);
  cx2 = x3 + ((2 * (x2 - x3)) / 3);
  cy2 = y3 + ((2 * (y2 - y3)) / 3);

  /* Draw as cubic Bezier */
  iupdrvDrawBezier(dc, x1, y1, cx1, cy1, cx2, cy2, x3, y3, color, style, line_width);
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

  cairo_new_path(dc->image_cr);
  cairo_rectangle(dc->image_cr, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  cairo_clip(dc->image_cr);  /* intersect with the current clipping */

  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
}

IUP_SDK_API void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius)
{
  double radius = (double)corner_radius;
  double degrees = IUP_DEG2RAD;
  double max_radius;

  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }

  /* Clamp radius to prevent oversized corners */
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  max_radius = ((x2 - x1) < (y2 - y1)) ? (x2 - x1) / 2.0 : (y2 - y1) / 2.0;
  if (radius > max_radius)
    radius = max_radius;

  iupdrvDrawResetClip(dc);

  /* Draw rounded rectangle path with arcs at corners */
  cairo_new_path(dc->image_cr);
  cairo_arc(dc->image_cr, x2 - radius, y1 + radius, radius, -90 * degrees, 0 * degrees);
  cairo_line_to(dc->image_cr, x2, y2 - radius);
  cairo_arc(dc->image_cr, x2 - radius, y2 - radius, radius, 0 * degrees, 90 * degrees);
  cairo_line_to(dc->image_cr, x1 + radius, y2);
  cairo_arc(dc->image_cr, x1 + radius, y2 - radius, radius, 90 * degrees, 180 * degrees);
  cairo_line_to(dc->image_cr, x1, y1 + radius);
  cairo_arc(dc->image_cr, x1 + radius, y1 + radius, radius, 180 * degrees, 270 * degrees);
  cairo_close_path(dc->image_cr);
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
  PangoLayout* fontlayout = (PangoLayout*)iupgtk4GetPangoLayout(font);
  PangoAlignment alignment = PANGO_ALIGN_LEFT;
  int layout_w = w, layout_h = h;
  int layout_center = flags & IUP_DRAW_LAYOUTCENTER;

  if (text_orientation && layout_center)
    iupDrawGetTextSize(dc->ih, text, len, &layout_w, &layout_h, 0);

  text = iupgtk4StrConvertToSystemLen(text, &len);
  pango_layout_set_text(fontlayout, text, len);

  if (flags & IUP_DRAW_RIGHT)
    alignment = PANGO_ALIGN_RIGHT;
  else if (flags & IUP_DRAW_CENTER)
    alignment = PANGO_ALIGN_CENTER;

  pango_layout_set_alignment(fontlayout, alignment);

  if (flags & IUP_DRAW_WRAP)
  {
    pango_layout_set_width(fontlayout, iupGTK4_PIXELS2PANGOUNITS(layout_w));
    pango_layout_set_height(fontlayout, iupGTK4_PIXELS2PANGOUNITS(layout_h));
  }
  else if (flags & IUP_DRAW_ELLIPSIS)
  {
    pango_layout_set_width(fontlayout, iupGTK4_PIXELS2PANGOUNITS(layout_w));
    pango_layout_set_height(fontlayout, iupGTK4_PIXELS2PANGOUNITS(layout_h));
    pango_layout_set_ellipsize(fontlayout, PANGO_ELLIPSIZE_END);
  }

  cairo_set_source_rgba(dc->image_cr, iupgtk4ColorToDouble(iupDrawRed(color)), iupgtk4ColorToDouble(iupDrawGreen(color)), iupgtk4ColorToDouble(iupDrawBlue(color)), iupgtk4ColorToDouble(iupDrawAlpha(color)));

  if (flags & IUP_DRAW_CLIP)
  {
    cairo_save(dc->image_cr);
    cairo_new_path(dc->image_cr);
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
    pango_layout_set_height(fontlayout, -1);
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
  GdkTexture* texture = iupImageGetImage(name, dc->ih, make_inactive, bgcolor);
  cairo_surface_t* surface;
  guchar* pixels;
  int stride;

  if (!texture)
    return;

  /* Get texture info and download pixels */
  iupdrvImageGetInfo(texture, &img_w, &img_h, &bpp);

  if (w == -1 || w == 0) w = img_w;
  if (h == -1 || h == 0) h = img_h;

  /* Download texture data to create Cairo surface */
  stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, img_w);
  pixels = g_malloc(stride * img_h);
  gdk_texture_download(texture, pixels, stride);

  /* Create Cairo surface from pixel data */
  surface = cairo_image_surface_create_for_data(pixels, CAIRO_FORMAT_ARGB32,
                                                 img_w, img_h, stride);

  cairo_save (dc->image_cr);

  cairo_new_path(dc->image_cr);
  cairo_rectangle(dc->image_cr, x, y, w, h);
  cairo_clip(dc->image_cr); /* intersect with the current clipping */

  if (w != img_w || h != img_h)
  {
    /* Scale *before* setting the source surface */
    cairo_translate(dc->image_cr, x, y);
    cairo_scale(dc->image_cr, (double)w / img_w, (double)h / img_h);
    cairo_translate(dc->image_cr, -x, -y);
  }

  cairo_set_source_surface(dc->image_cr, surface, x, y);
  cairo_paint(dc->image_cr);  /* paints the current source everywhere within the current clip region. */

  /* must restore clipping */
  cairo_restore(dc->image_cr);

  cairo_surface_destroy(surface);
  g_free(pixels);
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
  /* Use simple dotted rectangle for focus */
  iupdrvDrawRectangle(dc, x1, y1, x2, y2, iupDrawColor(0, 0, 0, 224), IUP_DRAW_STROKE_DOT, 1);
}

IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, long color1, long color2)
{
  cairo_pattern_t *pattern;
  float rad, x0, y0, x3, y3;
  float w, h;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  w = (float)(x2 - x1);
  h = (float)(y2 - y1);

  /* Calculate gradient endpoints based on angle */
  /* 0 = left to right, 90 = top to bottom, 180 = right to left, 270 = bottom to top */
  rad = angle * G_PI / 180.0f;

  /* Start point (x0, y0) and end point (x3, y3) */
  x0 = x1 + w / 2.0f - (w * cos(rad)) / 2.0f;
  y0 = y1 + h / 2.0f - (h * sin(rad)) / 2.0f;
  x3 = x1 + w / 2.0f + (w * cos(rad)) / 2.0f;
  y3 = y1 + h / 2.0f + (h * sin(rad)) / 2.0f;

  pattern = cairo_pattern_create_linear(x0, y0, x3, y3);
  cairo_pattern_add_color_stop_rgba(pattern, 0.0,
    iupDrawRed(color1) / 255.0, iupDrawGreen(color1) / 255.0, iupDrawBlue(color1) / 255.0, iupDrawAlpha(color1) / 255.0);
  cairo_pattern_add_color_stop_rgba(pattern, 1.0,
    iupDrawRed(color2) / 255.0, iupDrawGreen(color2) / 255.0, iupDrawBlue(color2) / 255.0, iupDrawAlpha(color2) / 255.0);

  cairo_set_source(dc->image_cr, pattern);
  cairo_rectangle(dc->image_cr, x1, y1, w, h);
  cairo_fill(dc->image_cr);
  cairo_pattern_destroy(pattern);
}

IUP_SDK_API void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, long colorCenter, long colorEdge)
{
  cairo_pattern_t *pattern;

  pattern = cairo_pattern_create_radial(cx, cy, 0, cx, cy, radius);
  cairo_pattern_add_color_stop_rgba(pattern, 0.0,
    iupDrawRed(colorCenter) / 255.0, iupDrawGreen(colorCenter) / 255.0, iupDrawBlue(colorCenter) / 255.0, iupDrawAlpha(colorCenter) / 255.0);
  cairo_pattern_add_color_stop_rgba(pattern, 1.0,
    iupDrawRed(colorEdge) / 255.0, iupDrawGreen(colorEdge) / 255.0, iupDrawBlue(colorEdge) / 255.0, iupDrawAlpha(colorEdge) / 255.0);

  cairo_set_source(dc->image_cr, pattern);
  cairo_arc(dc->image_cr, cx, cy, radius, 0, 2 * G_PI);
  cairo_fill(dc->image_cr);
  cairo_pattern_destroy(pattern);
}
