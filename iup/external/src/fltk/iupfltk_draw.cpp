/** \file
 * \brief Drawing Functions - FLTK Implementation
 *
 * Uses FLTK's offscreen drawing (fl_begin_offscreen / fl_end_offscreen).
 *
 * Flow: CreateCanvas -> fl_begin_offscreen (push offscreen surface)
 *       Drawing -> all fl_* calls go to offscreen
 *       Flush -> fl_end_offscreen (pop to window surface) + fl_copy_offscreen
 *       Kill -> cleanup (offscreen already ended by Flush)
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>
#include <FL/platform.H>

#if defined(FLTK_USE_WAYLAND)
#include <FL/wayland.H>
#endif

#if defined(FLTK_USE_CAIRO) || defined(FLTK_USE_WAYLAND)
#include <dlfcn.h>
typedef struct _cairo cairo_t;
static void (*_cairo_set_source_rgba)(cairo_t*, double, double, double, double) = NULL;
static int cairo_alpha_loaded = 0;

static void fltkDrawLoadCairo(void)
{
  if (cairo_alpha_loaded) return;
  cairo_alpha_loaded = 1;

  void* lib = dlopen("libcairo.so.2", RTLD_LAZY);
  if (!lib) lib = dlopen("libcairo.so", RTLD_LAZY);
  if (!lib) return;

  _cairo_set_source_rgba = (void (*)(cairo_t*, double, double, double, double))dlsym(lib, "cairo_set_source_rgba");
}
#endif

#include <cstdlib>
#include <cmath>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_image.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
}

#include "iupfltk_drv.h"


struct _IdrawCanvas
{
  Ihandle* ih;
  Fl_Widget* widget;
  int w, h;

  Fl_Offscreen offscreen;
  int in_offscreen;
  int clip_pushed;

  int clip_x1, clip_y1, clip_x2, clip_y2;
};

static void fltkDrawSetColor(long color)
{
  unsigned char r = iupDrawRed(color);
  unsigned char g = iupDrawGreen(color);
  unsigned char b = iupDrawBlue(color);
  unsigned char a = iupDrawAlpha(color);

  if (a < 255)
  {
#if defined(FLTK_USE_CAIRO) || defined(FLTK_USE_WAYLAND)
    fltkDrawLoadCairo();
    if (_cairo_set_source_rgba)
    {
      cairo_t* cr = NULL;
#if defined(FLTK_USE_WAYLAND)
      if (iupfltkIsWayland())
        cr = fl_wl_gc();
      else
#endif
#if defined(FLTK_USE_X11) && defined(FLTK_USE_CAIRO)
        cr = fl_cairo_gc();
#endif
      if (cr)
      {
        _cairo_set_source_rgba(cr, r / 255.0, g / 255.0, b / 255.0, a / 255.0);
        return;
      }
    }
#endif

    unsigned char bg_r = 255, bg_g = 255, bg_b = 255;
    Fl::get_color(FL_BACKGROUND_COLOR, bg_r, bg_g, bg_b);
    r = (unsigned char)((r * a + bg_r * (255 - a)) / 255);
    g = (unsigned char)((g * a + bg_g * (255 - a)) / 255);
    b = (unsigned char)((b * a + bg_b * (255 - a)) / 255);
  }

  fl_color(r, g, b);
}

static void fltkDrawSetLineStyle(int style, int line_width)
{
  if (line_width <= 0) line_width = 1;

  int fltk_style = FL_SOLID;

  switch (style)
  {
    case IUP_DRAW_STROKE_DASH:         fltk_style = FL_DASH; break;
    case IUP_DRAW_STROKE_DOT:          fltk_style = FL_DOT; break;
    case IUP_DRAW_STROKE_DASH_DOT:     fltk_style = FL_DASHDOT; break;
    case IUP_DRAW_STROKE_DASH_DOT_DOT: fltk_style = FL_DASHDOTDOT; break;
    default:                           fltk_style = FL_SOLID; break;
  }

  fl_line_style(fltk_style | FL_CAP_FLAT | FL_JOIN_MITER, line_width);
}

static void iupDrawOrderMinMax(int *x1, int *y1, int *x2, int *y2)
{
  int t;
  if (*x1 > *x2) { t = *x1; *x1 = *x2; *x2 = t; }
  if (*y1 > *y2) { t = *y1; *y1 = *y2; *y2 = t; }
}

extern "C" IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = (IdrawCanvas*)calloc(1, sizeof(IdrawCanvas));

  dc->ih = ih;
  dc->widget = (Fl_Widget*)ih->handle;

  dc->w = dc->widget->w();
  dc->h = dc->widget->h();
  if (dc->w <= 0) dc->w = 1;
  if (dc->h <= 0) dc->h = 1;

  Fl_Offscreen old_offscreen = (Fl_Offscreen)(size_t)iupAttribGet(ih, "_IUP_FLTK_OFFSCREEN");
  if (old_offscreen)
    fl_delete_offscreen(old_offscreen);

  dc->offscreen = fl_create_offscreen(dc->w, dc->h);
  iupAttribSet(ih, "_IUP_FLTK_OFFSCREEN", (char*)(size_t)dc->offscreen);
  iupAttribSet(ih, "DRAWDRIVER", "FLTK");

  fl_begin_offscreen(dc->offscreen);
  dc->in_offscreen = 1;

  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = dc->w - 1;
  dc->clip_y2 = dc->h - 1;

  return dc;
}

extern "C" IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  if (!dc) return;

  if (dc->in_offscreen)
  {
    fl_end_offscreen();
    dc->in_offscreen = 0;
  }

  free(dc);
}

extern "C" IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc)
{
  if (!dc || !dc->widget) return;

  if (dc->in_offscreen)
  {
    fl_end_offscreen();
    dc->in_offscreen = 0;
  }

  Fl_Window* win = dc->widget->as_window();
  if (!win) win = dc->widget->window();

  if (win && Fl_Window::current() != win)
    win->make_current();

  fl_copy_offscreen(dc->widget->x(), dc->widget->y(), dc->w, dc->h, dc->offscreen, 0, 0);
}

extern "C" IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  if (!dc || !dc->widget) return;

  int new_w = dc->widget->w();
  int new_h = dc->widget->h();
  if (new_w <= 0) new_w = 1;
  if (new_h <= 0) new_h = 1;

  if (new_w != dc->w || new_h != dc->h)
  {
    if (dc->in_offscreen)
    {
      fl_end_offscreen();
      dc->in_offscreen = 0;
    }

    if (dc->offscreen)
      fl_delete_offscreen(dc->offscreen);

    dc->w = new_w;
    dc->h = new_h;

    dc->offscreen = fl_create_offscreen(dc->w, dc->h);
    iupAttribSet(dc->ih, "_IUP_FLTK_OFFSCREEN", (char*)(size_t)dc->offscreen);
    iupAttribSetInt(dc->ih, "_IUP_FLTK_OFFSCREEN_W", dc->w);
    iupAttribSetInt(dc->ih, "_IUP_FLTK_OFFSCREEN_H", dc->h);

    fl_begin_offscreen(dc->offscreen);
    dc->in_offscreen = 1;
  }
}

extern "C" IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int *w, int *h)
{
  if (!dc) return;
  if (w) *w = dc->w;
  if (h) *h = dc->h;
}

extern "C" IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc) return;

  fltkDrawSetColor(color);
  fltkDrawSetLineStyle(style, line_width);
  fl_line(x1, y1, x2, y2);
}

extern "C" IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc) return;

  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);
  fltkDrawSetColor(color);

  if (style == IUP_DRAW_FILL)
    fl_rectf(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  else
  {
    fltkDrawSetLineStyle(style, line_width);
    fl_rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  if (!dc) return;

  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);
  fltkDrawSetColor(color);

  int w = x2 - x1 + 1;
  int h = y2 - y1 + 1;

  if (style == IUP_DRAW_FILL)
    fl_pie(x1, y1, w, h, a1, a2);
  else
  {
    fltkDrawSetLineStyle(style, line_width);
    fl_arc(x1, y1, w, h, a1, a2);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  iupdrvDrawArc(dc, x1, y1, x2, y2, 0.0, 360.0, color, style, line_width);
}

extern "C" IUP_SDK_API void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  if (!dc || count < 2) return;

  fltkDrawSetColor(color);

  if (style == IUP_DRAW_FILL)
  {
    fl_begin_complex_polygon();
    for (int i = 0; i < count; i++)
      fl_vertex(points[2 * i], points[2 * i + 1]);
    fl_end_complex_polygon();
  }
  else
  {
    fltkDrawSetLineStyle(style, line_width);
    fl_begin_loop();
    for (int i = 0; i < count; i++)
      fl_vertex(points[2 * i], points[2 * i + 1]);
    fl_end_loop();
  }
}

extern "C" IUP_SDK_API void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
{
  if (!dc) return;
  fltkDrawSetColor(color);
  fl_point(x, y);
}

extern "C" IUP_SDK_API void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius, long color, int style, int line_width)
{
  if (!dc) return;

  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);
  fltkDrawSetColor(color);

  int w = x2 - x1 + 1;
  int h = y2 - y1 + 1;

  if (style == IUP_DRAW_FILL)
    fl_rounded_rectf(x1, y1, w, h, corner_radius);
  else
  {
    fltkDrawSetLineStyle(style, line_width);
    fl_rounded_rect(x1, y1, w, h, corner_radius);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
  if (!dc) return;

  fltkDrawSetColor(color);
  fltkDrawSetLineStyle(style, line_width);

  fl_begin_line();
  fl_vertex(x1, y1);
  fl_curve(x1, y1, x2, y2, x3, y3, x4, y4);
  fl_end_line();
}

extern "C" IUP_SDK_API void iupdrvDrawQuadraticBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, long color, int style, int line_width)
{
  int cx1 = x1 + 2 * (x2 - x1) / 3;
  int cy1 = y1 + 2 * (y2 - y1) / 3;
  int cx2 = x3 + 2 * (x2 - x3) / 3;
  int cy2 = y3 + 2 * (y2 - y3) / 3;

  iupdrvDrawBezier(dc, x1, y1, cx1, cy1, cx2, cy2, x3, y3, color, style, line_width);
}

static long fltkInterpolateStops(const long* colors, const float* offsets, int count, float t)
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
      unsigned char r1 = iupDrawRed(colors[i]), g1 = iupDrawGreen(colors[i]), b1 = iupDrawBlue(colors[i]);
      unsigned char r2 = iupDrawRed(colors[i+1]), g2 = iupDrawGreen(colors[i+1]), b2 = iupDrawBlue(colors[i+1]);
      return iupDrawColor((unsigned char)(r1 + lt * (r2 - r1)), (unsigned char)(g1 + lt * (g2 - g1)), (unsigned char)(b1 + lt * (b2 - b1)), 255);
    }
  }
  return colors[count - 1];
}

extern "C" IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, const long* colors, const float* offsets, int count)
{
  if (!dc) return;

  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);

  int w = x2 - x1 + 1;
  int h = y2 - y1 + 1;
  int horizontal = (angle == 0 || angle == 180);
  int steps = horizontal ? w : h;
  if (steps <= 0) steps = 1;

  for (int i = 0; i < steps; i++)
  {
    float t = (steps > 1) ? (float)i / (float)(steps - 1) : 0.0f;
    if (angle == 180 || angle == 270) t = 1.0f - t;

    long c = fltkInterpolateStops(colors, offsets, count, t);
    fl_color(iupDrawRed(c), iupDrawGreen(c), iupDrawBlue(c));

    if (horizontal)
      fl_line(x1 + i, y1, x1 + i, y2);
    else
      fl_line(x1, y1 + i, x2, y1 + i);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, const long* colors, const float* offsets, int count)
{
  if (!dc || radius <= 0) return;

  for (int r = radius; r >= 0; r--)
  {
    float t = (float)r / (float)radius;

    long c = fltkInterpolateStops(colors, offsets, count, t);
    fl_color(iupDrawRed(c), iupDrawGreen(c), iupDrawBlue(c));

    fl_pie(cx - r, cy - r, 2 * r + 1, 2 * r + 1, 0, 360);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  if (!dc || !text) return;

  fltkDrawSetColor(color);

  int fl_font_id = FL_HELVETICA;
  int fl_size = FL_NORMAL_SIZE;

  if (font)
    iupfltkGetFontFromString(font, &fl_font_id, &fl_size);
  else
    iupfltkGetFont(dc->ih, &fl_font_id, &fl_size);

  fl_font(fl_font_id, fl_size);

  if (text_orientation != 0)
  {
    fl_push_matrix();
    fl_translate(x, y);
    fl_rotate(-text_orientation);
    fl_draw(text, len, 0, (int)fl_height() - (int)fl_descent());
    fl_pop_matrix();
  }
  else
  {
    Fl_Align align = FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE;

    if (flags & IUP_DRAW_CENTER)
      align = FL_ALIGN_CENTER | FL_ALIGN_INSIDE;
    else if (flags & IUP_DRAW_RIGHT)
      align = FL_ALIGN_RIGHT | FL_ALIGN_TOP | FL_ALIGN_INSIDE;

    if (flags & IUP_DRAW_WRAP)
      align |= FL_ALIGN_WRAP;

    if (flags & IUP_DRAW_CLIP)
      align |= FL_ALIGN_CLIP;

    fl_draw(text, x, y, w, h, align, NULL, 0);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, long tint, int opacity, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int quality)
{
  if (!dc || !name) return;

  Fl_Image* image = (Fl_Image*)iupImageGetImageTint(name, dc->ih, make_inactive, bgcolor, tint);
  if (!image) return;

  int img_w = image->data_w();
  int img_h = image->data_h();

  if (sw <= 0 || sh <= 0)
  {
    sx = 0;
    sy = 0;
    sw = img_w;
    sh = img_h;
  }
  if (w <= 0) w = sw;
  if (h <= 0) h = sh;

  Fl_RGB_Image* faded = NULL;
  if (opacity < 255 && image->count() == 1 && image->d() >= 3)
  {
    Fl_RGB_Image* rgbsrc = (Fl_RGB_Image*)image;
    int d = rgbsrc->d();
    int ld = rgbsrc->ld() ? rgbsrc->ld() : img_w * d;
    uchar* data = new uchar[(size_t)img_w * img_h * 4];
    for (int yy = 0; yy < img_h; yy++)
    {
      const uchar* sp = rgbsrc->array + (size_t)yy * ld;
      uchar* dp = data + (size_t)yy * img_w * 4;
      for (int xx = 0; xx < img_w; xx++)
      {
        dp[0] = sp[0];
        dp[1] = sp[1];
        dp[2] = sp[2];
        dp[3] = (uchar)((((d == 4) ? sp[3] : 255) * opacity) / 255);
        sp += d;
        dp += 4;
      }
    }
    faded = new Fl_RGB_Image(data, img_w, img_h, 4);
    faded->alloc_array = 1;
    image = faded;
  }

  Fl_RGB_Scaling old_scaling = Fl_Image::RGB_scaling();
  Fl_Image::RGB_scaling(quality == IUP_DRAW_IMAGE_NEAREST ? FL_RGB_SCALING_NEAREST : FL_RGB_SCALING_BILINEAR);

  if (sx == 0 && sy == 0 && sw == img_w && sh == img_h)
  {
    if (w != sw || h != sh)
    {
      Fl_Image* scaled = image->copy(w, h);
      if (scaled)
      {
        scaled->draw(x, y);
        delete scaled;
      }
    }
    else
      image->draw(x, y);
  }
  else if (image->count() == 1 && image->d() >= 1)
  {
    Fl_RGB_Image* rgb = (Fl_RGB_Image*)image;
    int d = rgb->d();
    int ld = rgb->ld() ? rgb->ld() : img_w * d;
    Fl_RGB_Image sub(rgb->array + (size_t)sy * ld + (size_t)sx * d, sw, sh, d, ld);
    if (w != sw || h != sh)
    {
      Fl_Image* scaled = sub.copy(w, h);
      if (scaled)
      {
        scaled->draw(x, y);
        delete scaled;
      }
    }
    else
      sub.draw(x, y);
  }
  else if (w == sw && h == sh)
    image->draw(x, y, sw, sh, sx, sy);
  else
  {
    Fl_Image* scaled = image->copy(img_w * w / sw, img_h * h / sh);
    if (scaled)
    {
      scaled->draw(x, y, w, h, sx * w / sw, sy * h / sh);
      delete scaled;
    }
  }

  Fl_Image::RGB_scaling(old_scaling);

  if (faded)
    delete faded;
}

extern "C" IUP_SDK_API void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc) return;

  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);

  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;

  if (dc->clip_pushed)
  {
    fl_pop_clip();
    dc->clip_pushed = 0;
  }

  fl_push_clip(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  dc->clip_pushed = 1;
}

extern "C" IUP_SDK_API void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius)
{
  (void)corner_radius;
  iupdrvDrawSetClipRect(dc, x1, y1, x2, y2);
}

extern "C" IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  if (!dc) return;

  if (dc->clip_pushed)
  {
    fl_pop_clip();
    dc->clip_pushed = 0;
  }

  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = dc->w - 1;
  dc->clip_y2 = dc->h - 1;
}

extern "C" IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int *x1, int *y1, int *x2, int *y2)
{
  if (!dc) return;
  if (x1) *x1 = dc->clip_x1;
  if (y1) *y1 = dc->clip_y1;
  if (x2) *x2 = dc->clip_x2;
  if (y2) *y2 = dc->clip_y2;
}

extern "C" IUP_SDK_API void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc) return;

  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);

  fl_color(FL_SELECTION_COLOR);
  fl_line_style(FL_SOLID, 1);
  fl_rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

extern "C" IUP_SDK_API void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc) return;

  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);

  fl_color(FL_BLACK);
  fl_line_style(FL_DOT, 1);
  fl_rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  fl_line_style(FL_SOLID, 0);
}

extern "C" IUP_SDK_API int iupdrvDrawGetImageData(IdrawCanvas* dc, unsigned char* data)
{
  if (!dc || !data) return 0;

  if (!dc->in_offscreen)
  {
    fl_begin_offscreen(dc->offscreen);
    dc->in_offscreen = 1;
  }

  uchar* pixels = fl_read_image(NULL, 0, 0, dc->w, dc->h);
  if (!pixels)
    return 0;

  int d = 3;
  for (int y = 0; y < dc->h; y++)
  {
    const uchar* src_line = pixels + y * dc->w * d;
    unsigned char* dst_line = data + y * dc->w * 4;

    for (int x = 0; x < dc->w; x++)
    {
      dst_line[x * 4 + 0] = src_line[x * d + 0];
      dst_line[x * 4 + 1] = src_line[x * d + 1];
      dst_line[x * 4 + 2] = src_line[x * d + 2];
      dst_line[x * 4 + 3] = 255;
    }
  }

  delete[] pixels;
  return 1;
}

extern "C" IUP_SDK_API int iupdrvCanvasGetImageData(Ihandle* ih, unsigned char* data, int w, int h)
{
  if (!ih || !data) return 0;

  Fl_Offscreen offscreen = (Fl_Offscreen)(size_t)iupAttribGet(ih, "_IUP_FLTK_OFFSCREEN");
  if (!offscreen) return 0;

  int buf_w = iupAttribGetInt(ih, "_IUP_FLTK_OFFSCREEN_W");
  int buf_h = iupAttribGetInt(ih, "_IUP_FLTK_OFFSCREEN_H");

  if (w > buf_w) w = buf_w;
  if (h > buf_h) h = buf_h;

  fl_begin_offscreen(offscreen);

  uchar* pixels = fl_read_image(NULL, 0, 0, w, h);

  fl_end_offscreen();

  if (!pixels) return 0;

  int d = 3;
  for (int y = 0; y < h; y++)
  {
    const uchar* src_line = pixels + y * w * d;
    unsigned char* dst_line = data + y * w * 4;

    for (int x = 0; x < w; x++)
    {
      dst_line[x * 4 + 0] = src_line[x * d + 0];
      dst_line[x * 4 + 1] = src_line[x * d + 1];
      dst_line[x * 4 + 2] = src_line[x * d + 2];
      dst_line[x * 4 + 3] = 255;
    }
  }

  delete[] pixels;
  return 1;
}
