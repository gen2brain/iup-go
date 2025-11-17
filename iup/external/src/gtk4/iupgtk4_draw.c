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
   * gtk_widget_get_allocated_width() which triggers CSS recalculation and
   * frees the style object that GTK is currently using for rendering. */
  dc->w = iupAttribGetInt(ih, "_IUPGTK4_DRAW_WIDTH");
  dc->h = iupAttribGetInt(ih, "_IUPGTK4_DRAW_HEIGHT");

  /* Fallback for calls outside ACTION callback */
  if (dc->w == 0) dc->w = gtk_widget_get_allocated_width(dc->widget);
  if (dc->h == 0) dc->h = gtk_widget_get_allocated_height(dc->widget);

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

  w = gtk_widget_get_allocated_width(dc->widget);
  h = gtk_widget_get_allocated_height(dc->widget);

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
