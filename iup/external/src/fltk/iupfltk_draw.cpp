/** \file
 * \brief Drawing Functions - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Image_Surface.H>
#include <FL/platform.H>

#define _USE_MATH_DEFINES
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
}

#include "iupfltk_drv.h"


struct FltkClipMask
{
  std::vector<unsigned char> coverage;
  int w = 0, h = 0;
  float scale = 1;
  int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
};

struct FltkDrawLayer
{
  Fl_Offscreen parent = 0;
  int grouped = 0;
  int clip_pushed = 0;
  int clip_x1 = 0, clip_y1 = 0, clip_x2 = 0, clip_y2 = 0;
  IupDrawMatrix matrix{};
  FltkClipMask clip_mask;
};

struct _IdrawCanvas
{
  Ihandle* ih = nullptr;
  Fl_Widget* widget = nullptr;
  int w = 0, h = 0;

  Fl_Offscreen offscreen = 0;
  Fl_Offscreen target = 0;
  Fl_Offscreen clip_offscreen = 0;
  std::vector<FltkDrawLayer> layers;
  int in_offscreen = 0;
  int clip_pushed = 0;

  FltkClipMask clip_mask;

  int clip_x1 = 0, clip_y1 = 0, clip_x2 = 0, clip_y2 = 0;
  IupDrawMatrix matrix{};
};

static int fltkDrawIdentity(const IdrawCanvas* dc)
{
  return dc->matrix.a == 1 && dc->matrix.b == 0 && dc->matrix.c == 0 &&
         dc->matrix.d == 1 && dc->matrix.e == 0 && dc->matrix.f == 0;
}

static Fl_Offscreen fltkDrawCurrentOffscreen(const IdrawCanvas* dc)
{
  return dc->clip_offscreen ? dc->clip_offscreen : dc->target;
}

static void fltkDrawTransformPoint(const IupDrawMatrix* matrix, double x, double y, double* tx, double* ty)
{
  *tx = matrix->a * x + matrix->c * y + matrix->e;
  *ty = matrix->b * x + matrix->d * y + matrix->f;
}

static void fltkDrawInversePoint(const IupDrawMatrix* matrix, double x, double y, double* tx, double* ty)
{
  double determinant = matrix->a * matrix->d - matrix->b * matrix->c;
  double dx = x - matrix->e;
  double dy = y - matrix->f;
  *tx = (matrix->d * dx - matrix->c * dy) / determinant;
  *ty = (-matrix->b * dx + matrix->a * dy) / determinant;
}

static double fltkDrawMatrixScale(const IupDrawMatrix* matrix)
{
  return sqrt(fabs(matrix->a * matrix->d - matrix->b * matrix->c));
}

static void fltkDrawTransformBounds(const IupDrawMatrix* matrix, double x1, double y1, double x2, double y2, int* tx1, int* ty1, int* tx2, int* ty2)
{
  double px[4], py[4];
  fltkDrawTransformPoint(matrix, x1, y1, &px[0], &py[0]);
  fltkDrawTransformPoint(matrix, x2, y1, &px[1], &py[1]);
  fltkDrawTransformPoint(matrix, x2, y2, &px[2], &py[2]);
  fltkDrawTransformPoint(matrix, x1, y2, &px[3], &py[3]);
  double min_x = px[0], max_x = px[0], min_y = py[0], max_y = py[0];
  for (int i = 1; i < 4; i++)
  {
    if (px[i] < min_x) min_x = px[i];
    if (px[i] > max_x) max_x = px[i];
    if (py[i] < min_y) min_y = py[i];
    if (py[i] > max_y) max_y = py[i];
  }
  *tx1 = static_cast<int>(floor(min_x)) - 2;
  *ty1 = static_cast<int>(floor(min_y)) - 2;
  *tx2 = static_cast<int>(ceil(max_x)) + 2;
  *ty2 = static_cast<int>(ceil(max_y)) + 2;
}

static void fltkDrawSetColor(long color)
{
  fl_color(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color));
}

static int fltkDrawStopsOpaque(const long* colors, int count)
{
  for (int i = 0; i < count; i++)
    if (iupDrawAlpha(colors[i]) < 255)
      return 0;
  return 1;
}

static void fltkDrawSetLineStyle(IdrawCanvas* dc, int style, int line_width)
{
  if (line_width <= 0) line_width = 1;

  IupDrawStroke stroke;
  iupDrawGetStroke(dc->ih, style, &stroke);

  int fltk_style = FL_SOLID;
  fltk_style |= stroke.cap == IUP_DRAW_CAP_ROUND ? FL_CAP_ROUND :
                stroke.cap == IUP_DRAW_CAP_SQUARE ? FL_CAP_SQUARE : FL_CAP_FLAT;
  fltk_style |= stroke.join == IUP_DRAW_JOIN_ROUND ? FL_JOIN_ROUND :
                stroke.join == IUP_DRAW_JOIN_BEVEL ? FL_JOIN_BEVEL : FL_JOIN_MITER;

  char dashes[IUP_DRAW_MAX_DASHES + 1];
  double scale = fltkDrawMatrixScale(&dc->matrix);
  int count = 0;

  for (int i = 0; i < stroke.dash_count; i++)
  {
    long len = lround(stroke.dashes[i] * scale);
    if (len < 1) len = 1;
    if (len > 127) len = 127;
    dashes[count++] = static_cast<char>(len);
  }
  dashes[count] = 0;

  fl_line_style(fltk_style, line_width, count ? dashes : nullptr);
}

static void iupDrawOrderMinMax(int* x1, int* y1, int* x2, int* y2)
{
  int t;
  if (*x1 > *x2) { t = *x1; *x1 = *x2; *x2 = t; }
  if (*y1 > *y2) { t = *y1; *y1 = *y2; *y2 = t; }
}

static IupDrawSource fltkDrawSolidSource(long color)
{
  IupDrawSource src;
  memset(&src, 0, sizeof(src));
  src.type = IUP_SOURCE_SOLID;
  src.color = color;
  return src;
}

struct FltkPathEdge
{
  double x1, y1, x2, y2;
};

struct FltkPoint
{
  double x, y;
};

struct FltkSubpath
{
  std::vector<FltkPoint> points;
  int closed = 0;
};

typedef std::vector<FltkSubpath> FltkShape;

struct FltkDeviceShape
{
  std::vector<FltkPathEdge> edges;
  int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
};

static void fltkShapeMove(FltkShape& shape, double x, double y)
{
  FltkSubpath sub;
  sub.closed = 0;
  sub.points.push_back({x, y});
  shape.push_back(sub);
}

static void fltkShapeLine(FltkShape& shape, double x, double y)
{
  if (shape.empty())
    fltkShapeMove(shape, x, y);
  else
    shape.back().points.push_back({x, y});
}

static int fltkShapeSteps(const IupDrawMatrix* matrix, double length)
{
  int steps = static_cast<int>(ceil(length * fltkDrawMatrixScale(matrix) / 3.0));
  if (steps < 4) steps = 4;
  if (steps > 256) steps = 256;
  return steps;
}

static void fltkShapeArc(FltkShape& shape, const IupDrawMatrix* matrix, double cx, double cy, double rx, double ry, double a1, double span, int connect)
{
  int steps = fltkShapeSteps(matrix, fabs(span) * M_PI / 180.0 * (rx > ry ? rx : ry));
  for (int i = 0; i <= steps; i++)
  {
    double angle = (a1 + span * i / steps) * M_PI / 180.0;
    double x = cx + rx * cos(angle), y = cy - ry * sin(angle);
    if (i == 0 && !connect)
      fltkShapeMove(shape, x, y);
    else
      fltkShapeLine(shape, x, y);
  }
}

static void fltkShapeCubic(FltkShape& shape, const IupDrawMatrix* matrix, double x1, double y1, double x2, double y2, double x3, double y3)
{
  FltkPoint p0 = shape.back().points.back();
  double length = hypot(x1 - p0.x, y1 - p0.y) + hypot(x2 - x1, y2 - y1) + hypot(x3 - x2, y3 - y2);
  int steps = fltkShapeSteps(matrix, length);
  for (int i = 1; i <= steps; i++)
  {
    double t = static_cast<double>(i) / steps, u = 1 - t;
    fltkShapeLine(shape, u * u * u * p0.x + 3 * u * u * t * x1 + 3 * u * t * t * x2 + t * t * t * x3,
                         u * u * u * p0.y + 3 * u * u * t * y1 + 3 * u * t * t * y2 + t * t * t * y3);
  }
}

static FltkShape fltkShapeFromPath(const IupPathSeg* segs, int count, const IupDrawMatrix* matrix)
{
  FltkShape shape;
  double sub_x = 0, sub_y = 0;
  int reopen = 0;

  for (int i = 0; i < count; i++)
  {
    const IupPathSeg& seg = segs[i];
    if (seg.op != IUP_PATHSEG_MOVE_TO && seg.op != IUP_PATHSEG_CLOSE && (reopen || shape.empty()))
    {
      fltkShapeMove(shape, sub_x, sub_y);
      reopen = 0;
    }

    switch (seg.op)
    {
    case IUP_PATHSEG_MOVE_TO:
      fltkShapeMove(shape, seg.x1, seg.y1);
      sub_x = seg.x1;
      sub_y = seg.y1;
      reopen = 0;
      break;
    case IUP_PATHSEG_LINE_TO:
      fltkShapeLine(shape, seg.x1, seg.y1);
      break;
    case IUP_PATHSEG_CURVE_TO:
      fltkShapeCubic(shape, matrix, seg.x1, seg.y1, seg.x2, seg.y2, seg.x3, seg.y3);
      break;
    case IUP_PATHSEG_QUAD_TO:
    {
      FltkPoint p0 = shape.back().points.back();
      fltkShapeCubic(shape, matrix, p0.x + 2.0 / 3.0 * (seg.x1 - p0.x), p0.y + 2.0 / 3.0 * (seg.y1 - p0.y),
                     seg.x2 + 2.0 / 3.0 * (seg.x1 - seg.x2), seg.y2 + 2.0 / 3.0 * (seg.y1 - seg.y2), seg.x2, seg.y2);
      break;
    }
    case IUP_PATHSEG_ARC_TO:
    {
      double span = seg.a2 - seg.a1;
      while (span < 0) span += 360.0;
      while (span > 360.0) span -= 360.0;
      if (span >= 0.01 && seg.x2 > 0 && seg.y2 > 0)
        fltkShapeArc(shape, matrix, seg.x1, seg.y1, seg.x2, seg.y2, seg.a1, span, 1);
      break;
    }
    case IUP_PATHSEG_CLOSE:
      if (!shape.empty() && !reopen)
      {
        shape.back().closed = 1;
        reopen = 1;
      }
      break;
    }
  }

  return shape;
}

static void fltkShapeRect(FltkShape& shape, double x1, double y1, double x2, double y2)
{
  fltkShapeMove(shape, x1, y1);
  fltkShapeLine(shape, x2, y1);
  fltkShapeLine(shape, x2, y2);
  fltkShapeLine(shape, x1, y2);
  shape.back().closed = 1;
}

static void fltkShapeRoundedRect(FltkShape& shape, const IupDrawMatrix* matrix, double x1, double y1, double x2, double y2, double radius)
{
  double max_radius = ((x2 - x1) < (y2 - y1) ? (x2 - x1) : (y2 - y1)) / 2;
  if (radius > max_radius) radius = max_radius;
  if (radius <= 0)
  {
    fltkShapeRect(shape, x1, y1, x2, y2);
    return;
  }
  fltkShapeArc(shape, matrix, x2 - radius, y1 + radius, radius, radius, 0, 90, 0);
  fltkShapeArc(shape, matrix, x1 + radius, y1 + radius, radius, radius, 90, 90, 1);
  fltkShapeArc(shape, matrix, x1 + radius, y2 - radius, radius, radius, 180, 90, 1);
  fltkShapeArc(shape, matrix, x2 - radius, y2 - radius, radius, radius, 270, 90, 1);
  shape.back().closed = 1;
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
      auto r1 = iupDrawRed(colors[i]), g1 = iupDrawGreen(colors[i]), b1 = iupDrawBlue(colors[i]), a1 = iupDrawAlpha(colors[i]);
      auto r2 = iupDrawRed(colors[i+1]), g2 = iupDrawGreen(colors[i+1]), b2 = iupDrawBlue(colors[i+1]), a2 = iupDrawAlpha(colors[i+1]);
      return iupDrawColor(static_cast<unsigned char>(r1 + lt * (r2 - r1)), static_cast<unsigned char>(g1 + lt * (g2 - g1)), static_cast<unsigned char>(b1 + lt * (b2 - b1)), static_cast<unsigned char>(a1 + lt * (a2 - a1)));
    }
  }
  return colors[count - 1];
}

static long fltkDrawSourceColor(const IupDrawSource* src, double x, double y)
{
  if (src->type == IUP_SOURCE_SOLID)
    return src->color;
  if (src->type == IUP_SOURCE_RADIAL_GRADIENT)
    return fltkInterpolateStops(src->colors, src->offsets, src->count, static_cast<float>(sqrt((x - src->cx) * (x - src->cx) + (y - src->cy) * (y - src->cy)) / src->radius));

  int x1 = src->x1, y1 = src->y1, x2 = src->x2, y2 = src->y2;
  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);
  double rad = src->angle * M_PI / 180.0;
  double dx = (x2 - x1) * cos(rad);
  double dy = (y2 - y1) * sin(rad);
  double cx = x1 + (x2 - x1) / 2.0;
  double cy = y1 + (y2 - y1) / 2.0;
  double x0 = cx - dx / 2.0;
  double y0 = cy - dy / 2.0;
  double length2 = dx * dx + dy * dy;
  float t = length2 > 0 ? static_cast<float>(((x - x0) * dx + (y - y0) * dy) / length2) : 0.0f;
  return fltkInterpolateStops(src->colors, src->offsets, src->count, t);
}

static Fl_RGB_Image* fltkDrawReadOffscreen(Fl_Offscreen offscreen, int x, int y, int w, int h)
{
  Fl_Image_Surface surface(w, h, 1);
  Fl_Surface_Device::push_current(&surface);
  fl_copy_offscreen(0, 0, w, h, offscreen, x, y);
  Fl_Surface_Device::pop_current();
  return surface.image();
}

static const uchar* fltkDrawImagePixel(const Fl_RGB_Image* image, int x, int y)
{
  int ld = image->ld() ? image->ld() : image->data_w() * image->d();
  return reinterpret_cast<const uchar*>(image->data()[0]) + static_cast<size_t>(y) * ld + static_cast<size_t>(x) * image->d();
}

static void fltkDrawDeviceImage(uchar* rgba, int w, int h, int x, int y, int logical_w, int logical_h)
{
  Fl_RGB_Image image(rgba, w, h, 4);
  image.alloc_array = 1;
  image.scale(logical_w, logical_h, 0, 1);
  image.draw(x, y);
}

static void fltkDrawCoveragePixel(const IdrawCanvas* dc, const IupDrawSource* src, uchar* p, double ux, double uy, unsigned coverage)
{
  long color = 0;
  if (coverage)
  {
    if (src->type != IUP_SOURCE_SOLID && !fltkDrawIdentity(dc))
      fltkDrawInversePoint(&dc->matrix, ux, uy, &ux, &uy);
    color = fltkDrawSourceColor(src, ux, uy);
  }
  p[0] = iupDrawRed(color);
  p[1] = iupDrawGreen(color);
  p[2] = iupDrawBlue(color);
  p[3] = static_cast<uchar>((iupDrawAlpha(color) * coverage + 127) / 255);
}

static int fltkDrawDeviceFloor(int v, float s)
{
  return s == 1 ? v : static_cast<int>(static_cast<float>(v) * s + 0.001f);
}

template <typename F>
static void fltkDrawDevicePixels(int x1, int y1, int x2, int y2, F pixel)
{
  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 <= x1 || y2 <= y1)
    return;
  int lx, ly, lw, lh;
  fl_clip_box(x1, y1, x2 - x1, y2 - y1, lx, ly, lw, lh);
  if (lw <= 0 || lh <= 0)
    return;

  float s = fl_graphics_driver->scale();
  int dx1 = fltkDrawDeviceFloor(lx, s), dy1 = fltkDrawDeviceFloor(ly, s);
  int dw = fltkDrawDeviceFloor(lx + lw, s) - dx1, dh = fltkDrawDeviceFloor(ly + lh, s) - dy1;
  if (dw <= 0 || dh <= 0)
    return;

  auto* rgba = new uchar[static_cast<size_t>(dw) * dh * 4];
  uchar* p = rgba;
  for (int y = 0; y < dh; y++)
    for (int x = 0; x < dw; x++, p += 4)
      pixel((dx1 + x + 0.5) / s, (dy1 + y + 0.5) / s, p);
  fltkDrawDeviceImage(rgba, dw, dh, lx, ly, lw, lh);
}

static int fltkShapeDevice(const IdrawCanvas* dc, const FltkShape& shape, float s, FltkDeviceShape& ds)
{
  double min_x = 1e30, min_y = 1e30, max_x = -1e30, max_y = -1e30;

  for (const auto& sp : shape)
  {
    const std::vector<FltkPoint>& pts = sp.points;
    size_t n = pts.size();
    if (n < 2)
      continue;
    for (size_t i = 0; i < n; i++)
    {
      double x0, y0, x1, y1;
      fltkDrawTransformPoint(&dc->matrix, pts[i].x, pts[i].y, &x0, &y0);
      fltkDrawTransformPoint(&dc->matrix, pts[(i + 1) % n].x, pts[(i + 1) % n].y, &x1, &y1);
      x0 *= s; y0 *= s; x1 *= s; y1 *= s;
      if (y0 != y1)
        ds.edges.push_back({x0, y0, x1, y1});
      if (x0 < min_x) min_x = x0;
      if (x0 > max_x) max_x = x0;
      if (y0 < min_y) min_y = y0;
      if (y0 > max_y) max_y = y0;
    }
  }

  if (ds.edges.empty())
    return 0;

  ds.x1 = std::max(0, static_cast<int>(floor(min_x / s)));
  ds.y1 = std::max(0, static_cast<int>(floor(min_y / s)));
  ds.x2 = std::min(dc->w, static_cast<int>(ceil(max_x / s)));
  ds.y2 = std::min(dc->h, static_cast<int>(ceil(max_y / s)));
  return ds.x2 > ds.x1 && ds.y2 > ds.y1;
}

static void fltkShapeCoverage(const FltkDeviceShape& ds, int rule, int dx1, int dy1, int dw, int dh, uchar* coverage, int stride)
{
  const int sub = 16;
  const float weight = 1.0f / sub;
  std::vector<float> cover(static_cast<size_t>(dw) + 1), run(static_cast<size_t>(dw) + 2);
  std::vector<std::pair<double, int> > crosses;

  for (int row = 0; row < dh; row++)
  {
    std::fill(cover.begin(), cover.end(), 0.0f);
    std::fill(run.begin(), run.end(), 0.0f);
    for (int k = 0; k < sub; k++)
    {
      double py = dy1 + row + (k + 0.5) / sub;
      crosses.clear();
      for (const auto& e : ds.edges)
      {
        if ((e.y1 <= py && e.y2 > py) || (e.y2 <= py && e.y1 > py))
          crosses.emplace_back(e.x1 + (py - e.y1) * (e.x2 - e.x1) / (e.y2 - e.y1), e.y2 > e.y1 ? 1 : -1);
      }
      std::sort(crosses.begin(), crosses.end());
      int winding = 0;
      for (size_t i = 0; i + 1 < crosses.size(); i++)
      {
        winding = rule == IUP_PATH_RULE_EVENODD ? winding ^ 1 : winding + crosses[i].second;
        if (winding == 0)
          continue;
        double xa = std::max(crosses[i].first - dx1, 0.0);
        double xb = std::min(crosses[i + 1].first - dx1, static_cast<double>(dw));
        if (xb <= xa)
          continue;
        int ia = static_cast<int>(xa), ib = static_cast<int>(xb);
        if (ia == ib)
          cover[ia] += static_cast<float>(xb - xa) * weight;
        else
        {
          cover[ia] += static_cast<float>(ia + 1 - xa) * weight;
          run[ia + 1] += weight;
          run[ib] -= weight;
          cover[ib] += static_cast<float>(xb - ib) * weight;
        }
      }
    }

    float acc = 0;
    uchar* p = coverage + static_cast<size_t>(row) * stride;
    for (int x = 0; x < dw; x++)
    {
      acc += run[x];
      float c = cover[x] + acc;
      p[x] = c <= 0 ? 0 : c >= 1 ? 255 : static_cast<uchar>(c * 255 + 0.5f);
    }
  }
}

static void fltkDrawShapeFill(IdrawCanvas* dc, const FltkShape& shape, const IupDrawSource* src, int rule)
{
  float s = fl_graphics_driver->scale();
  FltkDeviceShape ds;
  if (!fltkShapeDevice(dc, shape, s, ds))
    return;

  int lx1, ly1, lw, lh;
  fl_clip_box(ds.x1, ds.y1, ds.x2 - ds.x1, ds.y2 - ds.y1, lx1, ly1, lw, lh);
  if (lw <= 0 || lh <= 0)
    return;

  int dx1 = fltkDrawDeviceFloor(lx1, s), dy1 = fltkDrawDeviceFloor(ly1, s);
  int dw = fltkDrawDeviceFloor(lx1 + lw, s) - dx1, dh = fltkDrawDeviceFloor(ly1 + lh, s) - dy1;
  if (dw <= 0 || dh <= 0)
    return;

  std::vector<uchar> coverage(static_cast<size_t>(dw) * dh);
  fltkShapeCoverage(ds, rule, dx1, dy1, dw, dh, coverage.data(), dw);

  auto* rgba = new uchar[static_cast<size_t>(dw) * dh * 4];
  const double inv_s = 1.0 / s;
  const uchar solid_r = iupDrawRed(src->color), solid_g = iupDrawGreen(src->color), solid_b = iupDrawBlue(src->color);
  const unsigned solid_a = iupDrawAlpha(src->color);
  uchar* p = rgba;
  const uchar* c = coverage.data();
  for (int y = 0; y < dh; y++)
    for (int x = 0; x < dw; x++, p += 4, c++)
    {
      if (src->type != IUP_SOURCE_SOLID)
        fltkDrawCoveragePixel(dc, src, p, (dx1 + x + 0.5) * inv_s, (dy1 + y + 0.5) * inv_s, *c);
      else
      {
        p[0] = solid_r;
        p[1] = solid_g;
        p[2] = solid_b;
        p[3] = static_cast<uchar>((solid_a * *c + 127) / 255);
      }
    }

  fltkDrawDeviceImage(rgba, dw, dh, lx1, ly1, lw, lh);
}

static void fltkDrawShapeLines(IdrawCanvas* dc, const FltkShape& shape, int style, int line_width)
{
  const IupDrawMatrix* m = &dc->matrix;
  int width = static_cast<int>(lround((line_width > 0 ? line_width : 1) * fltkDrawMatrixScale(m)));
  if (width < 1) width = 1;

  fltkDrawSetLineStyle(dc, style, width);
  fl_push_matrix();
  fl_mult_matrix(m->a, m->b, m->c, m->d, m->e + 0.5 * (m->a + m->c) - 0.5, m->f + 0.5 * (m->b + m->d) - 0.5);
  for (const auto& sub : shape)
  {
    if (sub.points.size() < 2)
      continue;
    if (sub.closed)
      fl_begin_loop();
    else
      fl_begin_line();
    for (const auto& point : sub.points)
      fl_vertex(point.x, point.y);
    if (sub.closed)
      fl_end_loop();
    else
      fl_end_line();
  }
  fl_pop_matrix();
  fl_line_style(FL_SOLID, 0);
}

static void fltkDrawShapeStroke(IdrawCanvas* dc, const FltkShape& shape, const IupDrawSource* src, int style, int line_width)
{
  if (src->type == IUP_SOURCE_SOLID && iupDrawAlpha(src->color) == 255)
  {
    fltkDrawSetColor(src->color);
    fltkDrawShapeLines(dc, shape, style, line_width);
    return;
  }

  double min_x = 1e30, min_y = 1e30, max_x = -1e30, max_y = -1e30;
  for (const auto& sp : shape)
    for (const auto& pt : sp.points)
    {
      double tx, ty;
      fltkDrawTransformPoint(&dc->matrix, pt.x, pt.y, &tx, &ty);
      if (tx < min_x) min_x = tx;
      if (tx > max_x) max_x = tx;
      if (ty < min_y) min_y = ty;
      if (ty > max_y) max_y = ty;
    }
  if (min_x > max_x)
    return;
  double pad = fltkDrawMatrixScale(&dc->matrix) * (line_width > 0 ? line_width : 1) + 2;
  int bx1 = std::max(0, static_cast<int>(floor(min_x - pad))), by1 = std::max(0, static_cast<int>(floor(min_y - pad)));
  int bx2 = std::min(dc->w, static_cast<int>(ceil(max_x + pad))), by2 = std::min(dc->h, static_cast<int>(ceil(max_y + pad)));
  if (bx2 <= bx1 || by2 <= by1)
    return;
  int lx, ly, lw, lh;
  fl_clip_box(bx1, by1, bx2 - bx1, by2 - by1, lx, ly, lw, lh);
  if (lw <= 0 || lh <= 0)
    return;

  Fl_Offscreen target = fltkDrawCurrentOffscreen(dc);
  fl_end_offscreen();
  dc->in_offscreen = 0;
  Fl_Offscreen mask_offscreen = fl_create_offscreen(dc->w, dc->h);
  if (!mask_offscreen)
  {
    fl_begin_offscreen(target);
    dc->in_offscreen = 1;
    return;
  }
  fl_begin_offscreen(mask_offscreen);
  fl_push_no_clip();
  fl_color(FL_WHITE);
  fl_rectf(lx, ly, lw, lh);
  fl_color(FL_BLACK);
  fltkDrawShapeLines(dc, shape, style, line_width);
  fl_pop_clip();
  Fl_RGB_Image* mask = fltkDrawReadOffscreen(mask_offscreen, lx, ly, lw, lh);
  fl_end_offscreen();
  fl_delete_offscreen(mask_offscreen);
  fl_begin_offscreen(target);
  dc->in_offscreen = 1;

  if (mask)
  {
    int w = mask->data_w(), h = mask->data_h();
    auto* rgba = new uchar[static_cast<size_t>(w) * h * 4];
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++)
        fltkDrawCoveragePixel(dc, src, rgba + (static_cast<size_t>(y) * w + x) * 4,
                              lx + (x + 0.5) * lw / w, ly + (y + 0.5) * lh / h, 255u - fltkDrawImagePixel(mask, x, y)[0]);
    delete mask;
    fltkDrawDeviceImage(rgba, w, h, lx, ly, lw, lh);
  }
}

static void fltkDrawCommitClipMask(IdrawCanvas* dc)
{
  if (!dc->clip_offscreen)
    return;

  if (dc->in_offscreen)
  {
    fl_end_offscreen();
    dc->in_offscreen = 0;
  }

  const FltkClipMask& mask = dc->clip_mask;
  int lw = mask.x2 - mask.x1, lh = mask.y2 - mask.y1;
  Fl_RGB_Image* image = lw > 0 && lh > 0 ? fltkDrawReadOffscreen(dc->clip_offscreen, mask.x1, mask.y1, lw, lh) : nullptr;

  fl_begin_offscreen(dc->target);
  dc->in_offscreen = 1;

  if (image)
  {
    int w = image->data_w(), h = image->data_h();
    int dx1 = fltkDrawDeviceFloor(mask.x1, mask.scale), dy1 = fltkDrawDeviceFloor(mask.y1, mask.scale);
    auto* rgba = new uchar[static_cast<size_t>(w) * h * 4];
    for (int y = 0; y < h; y++)
    {
      int my = std::min(mask.h - 1, dy1 + y);
      for (int x = 0; x < w; x++)
      {
        const uchar* g = fltkDrawImagePixel(image, x, y);
        uchar* p = rgba + (static_cast<size_t>(y) * w + x) * 4;
        p[0] = g[0];
        p[1] = g[1];
        p[2] = g[2];
        p[3] = mask.coverage[static_cast<size_t>(my) * mask.w + std::min(mask.w - 1, dx1 + x)];
      }
    }
    delete image;
    fltkDrawDeviceImage(rgba, w, h, mask.x1, mask.y1, lw, lh);
  }

  fl_delete_offscreen(dc->clip_offscreen);
  dc->clip_offscreen = 0;
}

static void fltkDrawBeginClipMask(IdrawCanvas* dc)
{
  dc->clip_offscreen = fl_create_offscreen(dc->w, dc->h);
  if (!dc->clip_offscreen)
    return;

  if (dc->in_offscreen)
  {
    fl_end_offscreen();
    dc->in_offscreen = 0;
  }

  fl_begin_offscreen(dc->clip_offscreen);
  dc->in_offscreen = 1;
  fl_copy_offscreen(0, 0, dc->w, dc->h, dc->target, 0, 0);
}

static void fltkDrawSetClipShape(IdrawCanvas* dc, const FltkShape& shape, int rule)
{
  FltkClipMask& mask = dc->clip_mask;
  mask.scale = fl_graphics_driver->scale();
  mask.w = fltkDrawDeviceFloor(dc->w, mask.scale);
  mask.h = fltkDrawDeviceFloor(dc->h, mask.scale);
  mask.coverage.assign(static_cast<size_t>(mask.w) * mask.h, 0);
  mask.x1 = mask.y1 = mask.x2 = mask.y2 = 0;

  FltkDeviceShape ds;
  if (fltkShapeDevice(dc, shape, mask.scale, ds))
  {
    int dx1 = fltkDrawDeviceFloor(ds.x1, mask.scale), dy1 = fltkDrawDeviceFloor(ds.y1, mask.scale);
    int dx2 = std::min(mask.w, fltkDrawDeviceFloor(ds.x2, mask.scale)), dy2 = std::min(mask.h, fltkDrawDeviceFloor(ds.y2, mask.scale));
    if (dx2 > dx1 && dy2 > dy1)
    {
      fltkShapeCoverage(ds, rule, dx1, dy1, dx2 - dx1, dy2 - dy1, &mask.coverage[static_cast<size_t>(dy1) * mask.w + dx1], mask.w);
      mask.x1 = ds.x1;
      mask.y1 = ds.y1;
      mask.x2 = ds.x2;
      mask.y2 = ds.y2;
    }
  }

  fltkDrawBeginClipMask(dc);
}

static void fltkDrawDropLayers(IdrawCanvas* dc)
{
  while (!dc->layers.empty())
  {
    FltkDrawLayer& layer = dc->layers.back();

    if (dc->in_offscreen)
    {
      fl_end_offscreen();
      dc->in_offscreen = 0;
    }

    if (dc->clip_offscreen)
    {
      fl_delete_offscreen(dc->clip_offscreen);
      dc->clip_offscreen = 0;
    }

    if (dc->clip_pushed)
    {
      fl_begin_offscreen(dc->target);
      fl_pop_clip();
      fl_end_offscreen();
      dc->clip_pushed = 0;
    }

    if (layer.grouped)
      fl_delete_offscreen(dc->target);

    dc->target = layer.parent;
    dc->clip_x1 = layer.clip_x1;
    dc->clip_y1 = layer.clip_y1;
    dc->clip_x2 = layer.clip_x2;
    dc->clip_y2 = layer.clip_y2;
    dc->matrix = layer.matrix;
    std::swap(dc->clip_mask, layer.clip_mask);
    dc->layers.pop_back();

    fl_begin_offscreen(dc->target);
    dc->in_offscreen = 1;
  }
}

extern "C" IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  auto* dc = new IdrawCanvas();

  dc->ih = ih;
  dc->widget = reinterpret_cast<Fl_Widget*>(ih->handle);

  dc->w = dc->widget->w();
  dc->h = dc->widget->h();
  if (dc->w <= 0) dc->w = 1;
  if (dc->h <= 0) dc->h = 1;

  dc->offscreen = static_cast<Fl_Offscreen>(reinterpret_cast<size_t>(iupAttribGet(ih, "_IUP_FLTK_OFFSCREEN")));
  if (dc->offscreen &&
      (iupAttribGetInt(ih, "_IUP_FLTK_OFFSCREEN_W") != dc->w || iupAttribGetInt(ih, "_IUP_FLTK_OFFSCREEN_H") != dc->h))
  {
    fl_delete_offscreen(dc->offscreen);
    dc->offscreen = 0;
  }

  int fresh = 0;
  if (!dc->offscreen)
  {
    dc->offscreen = fl_create_offscreen(dc->w, dc->h);
    iupAttribSet(ih, "_IUP_FLTK_OFFSCREEN", reinterpret_cast<char*>(static_cast<size_t>(dc->offscreen)));
    iupAttribSetInt(ih, "_IUP_FLTK_OFFSCREEN_W", dc->w);
    iupAttribSetInt(ih, "_IUP_FLTK_OFFSCREEN_H", dc->h);
    fresh = 1;
  }

  iupAttribSet(ih, "DRAWDRIVER", "FLTK");

  dc->target = dc->offscreen;
  fl_begin_offscreen(dc->offscreen);
  dc->in_offscreen = 1;

  if (fresh)
  {
    unsigned char r, g, b;
    char* bgcolor = iupAttribGetStr(ih, "BGCOLOR");
    if (!iupStrToRGB(bgcolor, &r, &g, &b))
    {
      char* global = bgcolor ? IupGetGlobal(bgcolor) : nullptr;
      if (!global || !iupStrToRGB(global, &r, &g, &b))
        r = g = b = 255;
    }
    fl_color(fl_rgb_color(r, g, b));
    fl_rectf(0, 0, dc->w, dc->h);
  }

  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = 0;
  dc->clip_y2 = 0;
  dc->matrix.a = 1;
  dc->matrix.b = 0;
  dc->matrix.c = 0;
  dc->matrix.d = 1;
  dc->matrix.e = 0;
  dc->matrix.f = 0;

  return dc;
}

extern "C" IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  if (!dc) return;

  fltkDrawDropLayers(dc);
  fltkDrawCommitClipMask(dc);

  if (dc->clip_pushed)
  {
    fl_pop_clip();
    dc->clip_pushed = 0;
  }

  if (dc->in_offscreen)
  {
    fl_end_offscreen();
    dc->in_offscreen = 0;
  }

  delete dc;
}

extern "C" IUP_SDK_API void iupdrvDrawSetTransform(IdrawCanvas* dc, const IupDrawMatrix* matrix)
{
  if (!dc || !matrix) return;
  dc->matrix = *matrix;
}

extern "C" IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc)
{
  if (!dc || !dc->widget) return;

  fltkDrawCommitClipMask(dc);

  if (dc->clip_pushed)
  {
    fl_pop_clip();
    dc->clip_pushed = 0;
  }

  if (dc->in_offscreen)
  {
    fl_end_offscreen();
    dc->in_offscreen = 0;
  }

  Fl_Window* win = dc->widget->as_window();
  if (!win) win = dc->widget->window();

  if (!win || !win->shown())
    return;

  if (iupfltkCanvasDeferBlit(dc->ih))
    return;

  if (Fl_Window::current() != win)
    win->make_current();

  fl_copy_offscreen(dc->widget->x(), dc->widget->y(), dc->w, dc->h, dc->offscreen, 0, 0);
}

extern "C" IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int* w, int* h)
{
  if (!dc) return;
  if (w) *w = dc->w;
  if (h) *h = dc->h;
}

extern "C" IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc) return;

  if (!fltkDrawIdentity(dc) || iupDrawAlpha(color) < 255)
  {
    FltkShape shape;
    IupDrawSource src = fltkDrawSolidSource(color);
    fltkShapeMove(shape, x1, y1);
    fltkShapeLine(shape, x2, y2);
    fltkDrawShapeStroke(dc, shape, &src, style, line_width);
    return;
  }

  fltkDrawSetColor(color);
  fltkDrawSetLineStyle(dc, style, line_width);
  fl_line(x1, y1, x2, y2);
}

extern "C" IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc) return;

  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);
  if (!fltkDrawIdentity(dc) || iupDrawAlpha(color) < 255)
  {
    FltkShape shape;
    IupDrawSource src = fltkDrawSolidSource(color);
    if (style == IUP_DRAW_FILL)
    {
      fltkShapeRect(shape, x1, y1, x2 + 1, y2 + 1);
      fltkDrawShapeFill(dc, shape, &src, IUP_PATH_RULE_WINDING);
    }
    else
    {
      fltkShapeRect(shape, x1, y1, x2, y2);
      fltkDrawShapeStroke(dc, shape, &src, style, line_width);
    }
    return;
  }
  fltkDrawSetColor(color);

  if (style == IUP_DRAW_FILL)
    fl_rectf(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  else
  {
    fltkDrawSetLineStyle(dc, style, line_width);
    fl_rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  while (a2 < a1)
    a2 += 360;

  if (!dc) return;

  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);
  if (!fltkDrawIdentity(dc) || iupDrawAlpha(color) < 255)
  {
    FltkShape shape;
    IupDrawSource src = fltkDrawSolidSource(color);
    if (style == IUP_DRAW_FILL)
    {
      double cx = (x1 + x2 + 1) / 2.0, cy = (y1 + y2 + 1) / 2.0;
      fltkShapeMove(shape, cx, cy);
      fltkShapeArc(shape, &dc->matrix, cx, cy, (x2 - x1 + 1) / 2.0, (y2 - y1 + 1) / 2.0, a1, a2 - a1, 1);
      shape.back().closed = 1;
      fltkDrawShapeFill(dc, shape, &src, IUP_PATH_RULE_WINDING);
    }
    else
    {
      fltkShapeArc(shape, &dc->matrix, (x1 + x2) / 2.0, (y1 + y2) / 2.0, (x2 - x1) / 2.0, (y2 - y1) / 2.0, a1, a2 - a1, 0);
      if (a2 - a1 >= 360)
        shape.back().closed = 1;
      fltkDrawShapeStroke(dc, shape, &src, style, line_width);
    }
    return;
  }
  fltkDrawSetColor(color);

  int w = x2 - x1 + 1;
  int h = y2 - y1 + 1;

  if (style == IUP_DRAW_FILL)
    fl_pie(x1, y1, w, h, a1, a2);
  else
  {
    fltkDrawSetLineStyle(dc, style, line_width);
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

  if (!fltkDrawIdentity(dc) || iupDrawAlpha(color) < 255)
  {
    FltkShape shape;
    IupDrawSource src = fltkDrawSolidSource(color);
    for (int i = 0; i < count; i++)
    {
      if (i)
        fltkShapeLine(shape, points[2 * i], points[2 * i + 1]);
      else
        fltkShapeMove(shape, points[0], points[1]);
    }
    shape.back().closed = 1;
    if (style == IUP_DRAW_FILL)
      fltkDrawShapeFill(dc, shape, &src, IUP_PATH_RULE_WINDING);
    else
      fltkDrawShapeStroke(dc, shape, &src, style, line_width);
    return;
  }

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
    fltkDrawSetLineStyle(dc, style, line_width);
    fl_begin_loop();
    for (int i = 0; i < count; i++)
      fl_vertex(points[2 * i], points[2 * i + 1]);
    fl_end_loop();
  }
}

extern "C" IUP_SDK_API void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
{
  if (!dc) return;
  if (!fltkDrawIdentity(dc) || iupDrawAlpha(color) < 255)
  {
    FltkShape shape;
    IupDrawSource src = fltkDrawSolidSource(color);
    fltkShapeRect(shape, x, y, x + 1, y + 1);
    fltkDrawShapeFill(dc, shape, &src, IUP_PATH_RULE_WINDING);
    return;
  }
  fltkDrawSetColor(color);
  fl_point(x, y);
}

extern "C" IUP_SDK_API void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius, long color, int style, int line_width)
{
  if (!dc) return;

  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);
  if (!fltkDrawIdentity(dc) || iupDrawAlpha(color) < 255)
  {
    FltkShape shape;
    IupDrawSource src = fltkDrawSolidSource(color);
    if (style == IUP_DRAW_FILL)
    {
      fltkShapeRoundedRect(shape, &dc->matrix, x1, y1, x2 + 1, y2 + 1, corner_radius);
      fltkDrawShapeFill(dc, shape, &src, IUP_PATH_RULE_WINDING);
    }
    else
    {
      fltkShapeRoundedRect(shape, &dc->matrix, x1, y1, x2, y2, corner_radius);
      fltkDrawShapeStroke(dc, shape, &src, style, line_width);
    }
    return;
  }
  fltkDrawSetColor(color);

  int w = x2 - x1 + 1;
  int h = y2 - y1 + 1;

  if (style == IUP_DRAW_FILL)
    fl_rounded_rectf(x1, y1, w, h, corner_radius);
  else
  {
    fltkDrawSetLineStyle(dc, style, line_width);
    fl_rounded_rect(x1, y1, w, h, corner_radius);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
  if (!dc) return;

  if (!fltkDrawIdentity(dc) || iupDrawAlpha(color) < 255)
  {
    FltkShape shape;
    IupDrawSource src = fltkDrawSolidSource(color);
    fltkShapeMove(shape, x1, y1);
    fltkShapeCubic(shape, &dc->matrix, x2, y2, x3, y3, x4, y4);
    fltkDrawShapeStroke(dc, shape, &src, style, line_width);
    return;
  }

  fltkDrawSetColor(color);
  fltkDrawSetLineStyle(dc, style, line_width);

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

extern "C" IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, const long* colors, const float* offsets, int count)
{
  if (!dc) return;

  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);
  if (!fltkDrawIdentity(dc) || !fltkDrawStopsOpaque(colors, count))
  {
    IupDrawSource src = {};
    src.type = IUP_SOURCE_LINEAR_GRADIENT;
    src.x1 = x1; src.y1 = y1; src.x2 = x2; src.y2 = y2; src.angle = angle; src.count = count;
    memcpy(src.colors, colors, static_cast<size_t>(count) * sizeof(long));
    memcpy(src.offsets, offsets, static_cast<size_t>(count) * sizeof(float));
    FltkShape shape;
    fltkShapeRect(shape, x1, y1, x2 + 1, y2 + 1);
    fltkDrawShapeFill(dc, shape, &src, IUP_PATH_RULE_WINDING);
    return;
  }

  int w = x2 - x1 + 1;
  int h = y2 - y1 + 1;
  int horizontal = (angle == 0 || angle == 180);
  int steps = horizontal ? w : h;
  if (steps <= 0) steps = 1;

  for (int i = 0; i < steps; i++)
  {
    float t = (steps > 1) ? static_cast<float>(i) / static_cast<float>(steps - 1) : 0.0f;
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

  if (!fltkDrawIdentity(dc) || !fltkDrawStopsOpaque(colors, count))
  {
    IupDrawSource src = {};
    src.type = IUP_SOURCE_RADIAL_GRADIENT;
    src.cx = cx; src.cy = cy; src.radius = radius; src.count = count;
    memcpy(src.colors, colors, static_cast<size_t>(count) * sizeof(long));
    memcpy(src.offsets, offsets, static_cast<size_t>(count) * sizeof(float));
    FltkShape shape;
    fltkShapeArc(shape, &dc->matrix, cx + 0.5, cy + 0.5, radius, radius, 0, 360, 0);
    shape.back().closed = 1;
    fltkDrawShapeFill(dc, shape, &src, IUP_PATH_RULE_WINDING);
    return;
  }

  for (int r = radius; r >= 0; r--)
  {
    float t = static_cast<float>(r) / static_cast<float>(radius);

    long c = fltkInterpolateStops(colors, offsets, count, t);
    fl_color(iupDrawRed(c), iupDrawGreen(c), iupDrawBlue(c));

    fl_pie(cx - r, cy - r, 2 * r + 1, 2 * r + 1, 0, 360);
  }
}

static void fltkDrawTextDecoration(const char* text, int len, int tx, int baseline, int underline, int strikeout)
{
  int tw = static_cast<int>(fl_width(text, len) + 0.5);
  int ascent = fl_height() - fl_descent();

  if (tw <= 0)
    return;

  fl_line_style(FL_SOLID, 1);
  if (underline)
    fl_line(tx, baseline + 1, tx + tw - 1, baseline + 1);
  if (strikeout)
    fl_line(tx, baseline - ascent / 3, tx + tw - 1, baseline - ascent / 3);
  fl_line_style(FL_SOLID, 0);
}

static std::vector<std::string> fltkDrawSplitLines(const char* text, int len)
{
  std::vector<std::string> lines;
  const char* p = text;
  const char* end = text + len;
  while (p <= end)
  {
    const char* q = static_cast<const char*>(memchr(p, '\n', end - p));
    if (!q)
    {
      lines.emplace_back(p, end - p);
      break;
    }
    lines.emplace_back(p, q - p);
    p = q + 1;
  }
  return lines;
}

static std::string fltkDrawElide(const std::string& line, int w)
{
  if (fl_width(line.c_str(), static_cast<int>(line.size())) <= w)
    return line;

  std::string cut = line;
  while (!cut.empty())
  {
    size_t n = cut.size() - 1;
    while (n > 0 && (static_cast<unsigned char>(cut[n]) & 0xC0) == 0x80)
      n--;
    cut.resize(n);
    std::string candidate = cut + "...";
    if (fl_width(candidate.c_str(), static_cast<int>(candidate.size())) <= w)
      return candidate;
  }
  return "...";
}

static void fltkDrawTextRotated(const char* text, int len, int x, int y, int w, int h, int flags, double angle, int box_w, int box_h)
{
  std::vector<std::string> lines = fltkDrawSplitLines(text, len);
  double rad = angle * M_PI / 180.0, c = cos(rad), sn = sin(rad);
  int line_h = fl_height();
  int baseline = fl_height() - fl_descent();
  int layout_w = 0, layout_h = static_cast<int>(lines.size()) * line_h;
  double px, py, lx0, ly0;
  size_t i;

  for (i = 0; i < lines.size(); i++)
  {
    int line_w = static_cast<int>(ceil(fl_width(lines[i].c_str(), static_cast<int>(lines[i].size()))));
    if (line_w > layout_w)
      layout_w = line_w;
  }

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
    if (box_h > 0 && (flags & IUP_DRAW_CENTER))
      ly0 += (box_h - layout_h) / 2.0;
  }

  if (box_w > 0)
    layout_w = box_w;

  for (i = 0; i < lines.size(); i++)
  {
    double line_w = fl_width(lines[i].c_str(), static_cast<int>(lines[i].size()));
    double bx = lx0, by = ly0 + i * line_h + baseline;
    double dx, dy;
    if (flags & IUP_DRAW_CENTER)
      bx += (layout_w - line_w) / 2;
    else if (flags & IUP_DRAW_RIGHT)
      bx += layout_w - line_w;
    dx = bx - px;
    dy = by - py;
    fl_draw(static_cast<int>(angle), lines[i].c_str(), static_cast<int>(lines[i].size()), static_cast<int>(lround(px + dx * c + dy * sn)), static_cast<int>(lround(py - dx * sn + dy * c)));
  }
}

static void fltkDrawImageNative(Fl_Image* image, int img_w, int img_h, int x, int y, int w, int h, int sx, int sy, int sw, int sh)
{
  if (sx == 0 && sy == 0 && sw == img_w && sh == img_h)
  {
    if (image->w() != img_w || image->h() != img_h)
    {
      int old_w = image->w(), old_h = image->h();
      image->scale(w, h, 0, 1);
      image->draw(x, y);
      image->scale(old_w, old_h, 0, 1);
    }
    else if (w != sw || h != sh)
    {
      Fl_Image* scaled = image->copy(w, h);
      if (scaled) { scaled->draw(x, y); delete scaled; }
    }
    else image->draw(x, y);
  }
  else if (image->count() == 1 && image->d() >= 1)
  {
    auto* rgb = static_cast<Fl_RGB_Image*>(image);
    int d = rgb->d();
    int ld = rgb->ld() ? rgb->ld() : img_w * d;
    Fl_RGB_Image sub(rgb->array + static_cast<size_t>(sy) * ld + static_cast<size_t>(sx) * d, sw, sh, d, ld);
    if (w != sw || h != sh)
    {
      Fl_Image* scaled = sub.copy(w, h);
      if (scaled) { scaled->draw(x, y); delete scaled; }
    }
    else sub.draw(x, y);
  }
  else if (w == sw && h == sh)
    image->draw(x, y, sw, sh, sx, sy);
  else
  {
    Fl_Image* scaled = image->copy(img_w * w / sw, img_h * h / sh);
    if (scaled) { scaled->draw(x, y, w, h, sx * w / sw, sy * h / sh); delete scaled; }
  }
}

static std::vector<unsigned char> fltkDrawImagePixels(IdrawCanvas* dc, Fl_Image* image, int img_w, int img_h, int w, int h, int sx, int sy, int sw, int sh, int quality)
{
  std::vector<unsigned char> rgba;
  if (w <= 0 || h <= 0) return rgba;
  uchar* samples[2] = {nullptr, nullptr};
  Fl_RGB_Scaling old_scaling = Fl_Image::RGB_scaling();
  Fl_Image::RGB_scaling(quality == IUP_DRAW_IMAGE_NEAREST ? FL_RGB_SCALING_NEAREST : FL_RGB_SCALING_BILINEAR);
  fl_end_offscreen();
  dc->in_offscreen = 0;
  for (int pass = 0; pass < 2; pass++)
  {
    Fl_Offscreen offscreen = fl_create_offscreen(w, h);
    if (!offscreen) break;
    fl_begin_offscreen(offscreen);
    fl_push_no_clip();
    fl_color(pass ? FL_WHITE : FL_BLACK);
    fl_rectf(0, 0, w, h);
    fltkDrawImageNative(image, img_w, img_h, 0, 0, w, h, sx, sy, sw, sh);
    samples[pass] = fl_read_image(nullptr, 0, 0, w, h);
    fl_pop_clip();
    fl_end_offscreen();
    fl_delete_offscreen(offscreen);
  }
  fl_begin_offscreen(fltkDrawCurrentOffscreen(dc));
  dc->in_offscreen = 1;
  Fl_Image::RGB_scaling(old_scaling);
  if (samples[0] && samples[1])
  {
    rgba.resize(static_cast<size_t>(w) * h * 4);
    for (size_t i = 0; i < static_cast<size_t>(w) * h; i++)
    {
      int delta = 0;
      for (int c = 0; c < 3; c++) delta += samples[1][i * 3 + c] - samples[0][i * 3 + c];
      int alpha = 255 - delta / 3;
      if (alpha < 0) alpha = 0;
      if (alpha > 255) alpha = 255;
      for (int c = 0; c < 3; c++) rgba[i * 4 + c] = alpha ? static_cast<unsigned char>((samples[0][i * 3 + c] * 255 + alpha / 2) / alpha > 255 ? 255 : (samples[0][i * 3 + c] * 255 + alpha / 2) / alpha) : 0;
      rgba[i * 4 + 3] = static_cast<unsigned char>(alpha);
    }
  }
  delete[] samples[0];
  delete[] samples[1];
  return rgba;
}

struct FltkDrawMask
{
  int x = 0, y = 0, w = 0, h = 0, scale = 0;
  std::vector<unsigned char> pixels;
};

static Fl_Offscreen fltkDrawMaskBegin(IdrawCanvas* dc, FltkDrawMask* mask, int x1, int y1, int x2, int y2)
{
  double ux[4], uy[4];
  fltkDrawInversePoint(&dc->matrix, 0, 0, &ux[0], &uy[0]);
  fltkDrawInversePoint(&dc->matrix, dc->w, 0, &ux[1], &uy[1]);
  fltkDrawInversePoint(&dc->matrix, dc->w, dc->h, &ux[2], &uy[2]);
  fltkDrawInversePoint(&dc->matrix, 0, dc->h, &ux[3], &uy[3]);
  double min_x = ux[0], max_x = ux[0], min_y = uy[0], max_y = uy[0];
  for (int i = 1; i < 4; i++)
  {
    if (ux[i] < min_x) min_x = ux[i];
    if (ux[i] > max_x) max_x = ux[i];
    if (uy[i] < min_y) min_y = uy[i];
    if (uy[i] > max_y) max_y = uy[i];
  }
  if (x1 < static_cast<int>(floor(min_x)) - 1) x1 = static_cast<int>(floor(min_x)) - 1;
  if (y1 < static_cast<int>(floor(min_y)) - 1) y1 = static_cast<int>(floor(min_y)) - 1;
  if (x2 > static_cast<int>(ceil(max_x)) + 1) x2 = static_cast<int>(ceil(max_x)) + 1;
  if (y2 > static_cast<int>(ceil(max_y)) + 1) y2 = static_cast<int>(ceil(max_y)) + 1;
  if (x2 < x1 || y2 < y1)
    return 0;

  mask->scale = static_cast<int>(ceil(2 * fltkDrawMatrixScale(&dc->matrix)));
  if (mask->scale < 2) mask->scale = 2;
  if (mask->scale > 8) mask->scale = 8;
  mask->x = x1;
  mask->y = y1;
  mask->w = (x2 - x1 + 1) * mask->scale;
  mask->h = (y2 - y1 + 1) * mask->scale;
  if (mask->w < 1 || mask->h < 1 || static_cast<double>(mask->w) * mask->h > 64.0 * 1024 * 1024)
    return 0;
  Fl_Offscreen offscreen = fl_create_offscreen(mask->w, mask->h);
  if (!offscreen)
    return 0;
  fl_end_offscreen();
  dc->in_offscreen = 0;
  fl_begin_offscreen(offscreen);
  fl_push_no_clip();
  fl_color(FL_WHITE);
  fl_rectf(0, 0, mask->w, mask->h);
  fl_color(FL_BLACK);
  fl_push_matrix();
  fl_scale(mask->scale);
  fl_translate(-mask->x, -mask->y);
  return offscreen;
}

static void fltkDrawMaskEnd(IdrawCanvas* dc, FltkDrawMask* mask, Fl_Offscreen offscreen)
{
  fl_pop_matrix();
  fl_pop_clip();
  uchar* pixels = fl_read_image(nullptr, 0, 0, mask->w, mask->h);
  if (pixels)
  {
    mask->pixels.resize(static_cast<size_t>(mask->w) * mask->h);
    for (size_t i = 0; i < mask->pixels.size(); i++)
      mask->pixels[i] = static_cast<unsigned char>(255 - pixels[i * 3]);
    delete[] pixels;
  }
  fl_end_offscreen();
  fl_delete_offscreen(offscreen);
  fl_begin_offscreen(fltkDrawCurrentOffscreen(dc));
  dc->in_offscreen = 1;
}

static unsigned char fltkDrawMaskSample(const FltkDrawMask* mask, double x, double y)
{
  double sx = (x - mask->x) * mask->scale;
  double sy = (y - mask->y) * mask->scale;
  int x0 = static_cast<int>(floor(sx)), y0 = static_cast<int>(floor(sy));
  double fx = sx - x0, fy = sy - y0;
  if (x0 < 0 || y0 < 0 || x0 >= mask->w || y0 >= mask->h)
    return 0;
  int x1 = x0 + 1 < mask->w ? x0 + 1 : x0;
  int y1 = y0 + 1 < mask->h ? y0 + 1 : y0;
  double top = mask->pixels[static_cast<size_t>(y0) * mask->w + x0] * (1 - fx) + mask->pixels[static_cast<size_t>(y0) * mask->w + x1] * fx;
  double bottom = mask->pixels[static_cast<size_t>(y1) * mask->w + x0] * (1 - fx) + mask->pixels[static_cast<size_t>(y1) * mask->w + x1] * fx;
  return static_cast<unsigned char>(lround(top * (1 - fy) + bottom * fy));
}

static void fltkDrawPaintMask(IdrawCanvas* dc, const FltkDrawMask* mask, const IupDrawSource* src)
{
  if (mask->pixels.empty()) return;
  int x1, y1, x2, y2;
  fltkDrawTransformBounds(&dc->matrix, mask->x, mask->y, mask->x + mask->w / mask->scale, mask->y + mask->h / mask->scale, &x1, &y1, &x2, &y2);
  fltkDrawDevicePixels(x1, y1, std::min(x2 + 1, dc->w), std::min(y2 + 1, dc->h), [&](double cx, double cy, uchar* p)
  {
    double ux, uy;
    fltkDrawInversePoint(&dc->matrix, cx, cy, &ux, &uy);
    fltkDrawCoveragePixel(dc, src, p, cx, cy, fltkDrawMaskSample(mask, ux, uy));
  });
}

static void fltkDrawTransformedText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, int flags, double angle, int font_id, int font_size, int underline, int strikeout)
{
  int measured_w = (flags & IUP_DRAW_WRAP) && w > 0 ? w : 0, measured_h = 0;
  fl_font(font_id, font_size);
  fl_measure(text, measured_w, measured_h, 0);
  int layout_w = w > 0 ? w : measured_w;
  int layout_h = h > 0 ? h : measured_h;
  if (layout_w < 1) layout_w = 1;
  if (layout_h < 1) layout_h = fl_height();
  int pad = font_size + 4;
  int bx1 = x - pad, by1 = y - pad;
  int bx2 = x + (layout_w > measured_w ? layout_w : measured_w) + pad;
  int by2 = y + (layout_h > measured_h ? layout_h : measured_h) + pad;
  if (angle != 0)
  {
    int radius = static_cast<int>(ceil(sqrt(static_cast<double>(layout_w) * layout_w + static_cast<double>(layout_h) * layout_h))) + pad;
    double px = (flags & IUP_DRAW_LAYOUTCENTER) ? x + w / 2.0 : x;
    double py = (flags & IUP_DRAW_LAYOUTCENTER) ? y + h / 2.0 : y;
    bx1 = static_cast<int>(floor(px)) - radius;
    by1 = static_cast<int>(floor(py)) - radius;
    bx2 = static_cast<int>(ceil(px)) + radius;
    by2 = static_cast<int>(ceil(py)) + radius;
  }
  FltkDrawMask mask;
  Fl_Offscreen offscreen = fltkDrawMaskBegin(dc, &mask, bx1, by1, bx2, by2);
  if (!offscreen) return;
  int scale = mask.scale;
  int tx = (x - mask.x) * scale;
  int ty = (y - mask.y) * scale;
  int tw = w > 0 ? w * scale : measured_w * scale;
  int th = h > 0 ? h * scale : measured_h * scale;
  fl_font(font_id, font_size * scale);
  fl_color(FL_BLACK);
  if (flags & IUP_DRAW_CLIP)
    fl_push_clip(tx, ty, tw, th);
  if (angle != 0)
    fltkDrawTextRotated(text, len, tx, ty, tw, th, flags, angle, 0, 0);
  else
  {
    Fl_Align align = FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE;
    if (flags & IUP_DRAW_CENTER) align = FL_ALIGN_CENTER | FL_ALIGN_INSIDE;
    else if (flags & IUP_DRAW_RIGHT) align = FL_ALIGN_RIGHT | FL_ALIGN_TOP | FL_ALIGN_INSIDE;
    if (flags & IUP_DRAW_WRAP) align |= FL_ALIGN_WRAP;
    if (flags & IUP_DRAW_CLIP) align |= FL_ALIGN_CLIP;
    fl_draw(text, tx, ty, tw, th, align, nullptr, 0);
    if ((underline || strikeout) && !(flags & IUP_DRAW_WRAP))
    {
      int text_w = 0, text_h = 0;
      int dx = tx, dy = ty;
      fl_measure(text, text_w, text_h, 0);
      if (flags & IUP_DRAW_CENTER) { dx = tx + (tw - text_w) / 2; dy = ty + (th - text_h) / 2; }
      else if (flags & IUP_DRAW_RIGHT) dx = tx + tw - text_w;
      fltkDrawTextDecoration(text, len, dx, dy + fl_height() - fl_descent(), underline, strikeout);
    }
  }
  if (flags & IUP_DRAW_CLIP)
    fl_pop_clip();
  fltkDrawMaskEnd(dc, &mask, offscreen);
  IupDrawSource src = fltkDrawSolidSource(color);
  fltkDrawPaintMask(dc, &mask, &src);
}

static int fltkDrawTextSimilarity(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, int flags, double angle, int font_id, int font_size, int underline, int strikeout)
{
  const IupDrawMatrix* m = &dc->matrix;
  if (fabs(m->a - m->d) > 1e-9 || fabs(m->b + m->c) > 1e-9)
    return 0;

  double scale = sqrt(m->a * m->a + m->b * m->b);
  if (scale <= 0)
    return 0;

  double total = angle + atan2(-m->b, m->a) * 180.0 / M_PI;
  total -= 360.0 * floor(total / 360.0 + 0.5);
  if (fabs(total) < 1e-6)
    total = 0;

  if (total != 0 && (flags & (IUP_DRAW_WRAP | IUP_DRAW_CLIP)))
    return 0;

  int size = static_cast<int>(lround(font_size * scale));
  if (size < 1) size = 1;
  int box_w = w > 0 ? static_cast<int>(lround(w * scale)) : w;
  int box_h = h > 0 ? static_cast<int>(lround(h * scale)) : h;
  double px, py;

  fl_font(font_id, size);

  if (total == 0)
  {
    fltkDrawTransformPoint(m, x, y, &px, &py);
    int tx = static_cast<int>(lround(px)), ty = static_cast<int>(lround(py));
    Fl_Align align = FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE;
    if (flags & IUP_DRAW_CENTER) align = FL_ALIGN_CENTER | FL_ALIGN_INSIDE;
    else if (flags & IUP_DRAW_RIGHT) align = FL_ALIGN_RIGHT | FL_ALIGN_TOP | FL_ALIGN_INSIDE;
    if (flags & IUP_DRAW_WRAP) align |= FL_ALIGN_WRAP;
    if (flags & IUP_DRAW_CLIP) align |= FL_ALIGN_CLIP;
    fl_draw(text, tx, ty, box_w, box_h, align, nullptr, 0);
    if ((underline || strikeout) && !(flags & IUP_DRAW_WRAP))
    {
      int text_w = 0, text_h = 0;
      int dx = tx, dy = ty;
      fl_measure(text, text_w, text_h, 0);
      if (flags & IUP_DRAW_CENTER) { dx = tx + (box_w - text_w) / 2; dy = ty + (box_h - text_h) / 2; }
      else if (flags & IUP_DRAW_RIGHT) dx = tx + box_w - text_w;
      fltkDrawTextDecoration(text, len, dx, dy + fl_height() - fl_descent(), underline, strikeout);
    }
    return 1;
  }

  if (angle != 0 && (flags & IUP_DRAW_LAYOUTCENTER))
  {
    fltkDrawTransformPoint(m, x + w / 2.0, y + h / 2.0, &px, &py);
    fltkDrawTextRotated(text, len, static_cast<int>(lround(px - box_w / 2.0)), static_cast<int>(lround(py - box_h / 2.0)), box_w, box_h, flags, total, 0, 0);
  }
  else
  {
    fltkDrawTransformPoint(m, x, y, &px, &py);
    fltkDrawTextRotated(text, len, static_cast<int>(lround(px)), static_cast<int>(lround(py)), box_w, box_h, flags & ~IUP_DRAW_LAYOUTCENTER, total, angle == 0 ? box_w : 0, angle == 0 ? box_h : 0);
  }
  return 1;
}

extern "C" IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  char stack_buf[512];
  char* text_buf = stack_buf;

  if (!dc || !text) return;

  if (len < 0)
    len = static_cast<int>(strlen(text));
  if (len >= static_cast<int>(sizeof(stack_buf)))
    text_buf = static_cast<char*>(malloc(len + 1));
  memcpy(text_buf, text, len);
  text_buf[len] = 0;
  text = text_buf;

  fltkDrawSetColor(color);

  int fl_font_id = FL_HELVETICA;
  int fl_size = FL_NORMAL_SIZE;

  int underline = 0, strikeout = 0;

  if (font)
    iupfltkGetFontFromString(font, &fl_font_id, &fl_size);
  else
    iupfltkGetFont(dc->ih, &fl_font_id, &fl_size);

  iupfltkGetFontDecoration(dc->ih, font, &underline, &strikeout);

  fl_font(fl_font_id, fl_size);

  std::string elided;
  if ((flags & IUP_DRAW_ELLIPSIS) && !(flags & IUP_DRAW_WRAP) && w > 0)
  {
    std::vector<std::string> lines = fltkDrawSplitLines(text, len);
    for (size_t i = 0; i < lines.size(); i++)
    {
      if (i)
        elided += '\n';
      elided += fltkDrawElide(lines[i], w);
    }
    text = elided.c_str();
    len = static_cast<int>(elided.size());
  }

  if (!fltkDrawIdentity(dc) || iupDrawAlpha(color) < 255)
  {
    if (iupDrawAlpha(color) < 255 || !fltkDrawTextSimilarity(dc, text, len, x, y, w, h, flags, text_orientation, fl_font_id, fl_size, underline, strikeout))
      fltkDrawTransformedText(dc, text, len, x, y, w, h, color, flags, text_orientation, fl_font_id, fl_size, underline, strikeout);
    if (text_buf != stack_buf)
      free(text_buf);
    return;
  }

  if (text_orientation != 0)
  {
    if (flags & IUP_DRAW_CLIP)
      fl_push_clip(x, y, w, h);
    fltkDrawTextRotated(text, len, x, y, w, h, flags, text_orientation, 0, 0);
    if (flags & IUP_DRAW_CLIP)
      fl_pop_clip();
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

    fl_draw(text, x, y, w, h, align, nullptr, 0);

    if ((underline || strikeout) && !(flags & IUP_DRAW_WRAP))
    {
      int tw = 0, th = 0;
      int tx = x, ty = y;

      fl_measure(text, tw, th, 0);

      if (flags & IUP_DRAW_CENTER)
      {
        tx = x + (w - tw) / 2;
        ty = y + (h - th) / 2;
      }
      else if (flags & IUP_DRAW_RIGHT)
        tx = x + w - tw;

      fltkDrawTextDecoration(text, len, tx, ty + fl_height() - fl_descent(), underline, strikeout);
    }
  }

  if (text_buf != stack_buf)
    free(text_buf);
}

extern "C" IUP_SDK_API int iupdrvDrawBitmap(IdrawCanvas* dc, const unsigned char* data, int width, int height, int stride, int x, int y, int w, int h, int quality)
{
  if (!dc || !fltkDrawIdentity(dc))
    return 0;

  std::vector<unsigned char> rgb(static_cast<size_t>(width) * height * 3);
  for (int yy = 0; yy < height; yy++)
  {
    const unsigned char* sp = data + static_cast<size_t>(yy) * stride;
    unsigned char* dp = rgb.data() + static_cast<size_t>(yy) * width * 3;
    for (int xx = 0; xx < width; xx++, sp += 4, dp += 3)
    {
      dp[0] = sp[2];
      dp[1] = sp[1];
      dp[2] = sp[0];
    }
  }

  Fl_RGB_Image image(rgb.data(), width, height, 3);
  Fl_RGB_Scaling old_scaling = Fl_Image::RGB_scaling();
  Fl_Image::RGB_scaling(quality == IUP_DRAW_IMAGE_NEAREST ? FL_RGB_SCALING_NEAREST : FL_RGB_SCALING_BILINEAR);
  image.scale(w, h, 0, 1);
  image.draw(x, y);
  Fl_Image::RGB_scaling(old_scaling);
  return 1;
}

extern "C" IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, long tint, int opacity, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int quality)
{
  if (!dc || !name) return;

  auto* image = static_cast<Fl_Image*>(iupImageGetImageTint(name, dc->ih, make_inactive, bgcolor, tint));
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
  if (w <= 0) w = static_cast<int>(lround(static_cast<double>(sw) * image->w() / img_w));
  if (h <= 0) h = static_cast<int>(lround(static_cast<double>(sh) * image->h() / img_h));

  if (!fltkDrawIdentity(dc))
  {
    std::vector<unsigned char> pixels = fltkDrawImagePixels(dc, image, img_w, img_h, w, h, sx, sy, sw, sh, quality);
    if (pixels.empty()) return;
    int dx1, dy1, dx2, dy2;
    fltkDrawTransformBounds(&dc->matrix, x, y, x + w, y + h, &dx1, &dy1, &dx2, &dy2);
    fltkDrawDevicePixels(dx1, dy1, std::min(dx2 + 1, dc->w), std::min(dy2 + 1, dc->h), [&](double cx, double cy, uchar* p)
    {
      p[3] = 0;
      double ux, uy;
      fltkDrawInversePoint(&dc->matrix, cx, cy, &ux, &uy);
      double px = ux - x - 0.5, py = uy - y - 0.5;
      if (px < -0.5 || py < -0.5 || px >= w - 0.5 || py >= h - 0.5) return;
      int ix = static_cast<int>(floor(px)), iy = static_cast<int>(floor(py));
      double fx = px - ix, fy = py - iy;
      if (quality == IUP_DRAW_IMAGE_NEAREST)
      {
        ix = static_cast<int>(floor(px + 0.5)); iy = static_cast<int>(floor(py + 0.5)); fx = fy = 0;
      }
      if (ix < 0) { ix = 0; fx = 0; }
      if (iy < 0) { iy = 0; fy = 0; }
      int ix1 = ix + 1 < w ? ix + 1 : ix;
      int iy1 = iy + 1 < h ? iy + 1 : iy;
      double weights[4] = {(1 - fx) * (1 - fy), fx * (1 - fy), (1 - fx) * fy, fx * fy};
      size_t indices[4] = {static_cast<size_t>(iy) * w + ix, static_cast<size_t>(iy) * w + ix1, static_cast<size_t>(iy1) * w + ix, static_cast<size_t>(iy1) * w + ix1};
      double alpha = 0, premul[3] = {0, 0, 0};
      for (int i = 0; i < 4; i++)
      {
        double a = pixels[indices[i] * 4 + 3] * weights[i];
        alpha += a;
        for (int c = 0; c < 3; c++) premul[c] += pixels[indices[i] * 4 + c] * a;
      }
      alpha *= opacity / 255.0;
      if (alpha <= 0) return;
      p[0] = static_cast<uchar>(lround(premul[0] / (alpha * 255.0 / opacity)));
      p[1] = static_cast<uchar>(lround(premul[1] / (alpha * 255.0 / opacity)));
      p[2] = static_cast<uchar>(lround(premul[2] / (alpha * 255.0 / opacity)));
      p[3] = static_cast<uchar>(lround(alpha));
    });
    return;
  }

  Fl_RGB_Image* faded = nullptr;
  if (opacity < 255 && image->count() == 1 && image->d() >= 3)
  {
    auto* rgbsrc = static_cast<Fl_RGB_Image*>(image);
    int d = rgbsrc->d();
    int ld = rgbsrc->ld() ? rgbsrc->ld() : img_w * d;
    auto* data = new uchar[static_cast<size_t>(img_w) * img_h * 4];
    for (int yy = 0; yy < img_h; yy++)
    {
      const uchar* sp = rgbsrc->array + static_cast<size_t>(yy) * ld;
      uchar* dp = data + static_cast<size_t>(yy) * img_w * 4;
      for (int xx = 0; xx < img_w; xx++)
      {
        dp[0] = sp[0];
        dp[1] = sp[1];
        dp[2] = sp[2];
        dp[3] = static_cast<uchar>((((d == 4) ? sp[3] : 255) * opacity) / 255);
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

  fltkDrawImageNative(image, img_w, img_h, x, y, w, h, sx, sy, sw, sh);

  Fl_Image::RGB_scaling(old_scaling);

  delete faded;
}

extern "C" IUP_SDK_API void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc) return;

  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }

  fltkDrawCommitClipMask(dc);

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

  dc->clip_mask = FltkClipMask();

  if (!fltkDrawIdentity(dc))
  {
    FltkShape shape;
    fltkShapeRect(shape, x1, y1, x2 + 1, y2 + 1);
    fltkDrawSetClipShape(dc, shape, IUP_PATH_RULE_WINDING);
    return;
  }

  fl_push_clip(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  dc->clip_pushed = 1;
}

extern "C" IUP_SDK_API void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius)
{
  if (!dc) return;
  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }
  fltkDrawCommitClipMask(dc);
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
  FltkShape shape;
  fltkShapeRoundedRect(shape, &dc->matrix, x1, y1, x2 + 1, y2 + 1, corner_radius < 0 ? 0 : corner_radius);
  fltkDrawSetClipShape(dc, shape, IUP_PATH_RULE_WINDING);
}

extern "C" IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  if (!dc) return;

  fltkDrawCommitClipMask(dc);

  if (dc->clip_pushed)
  {
    fl_pop_clip();
    dc->clip_pushed = 0;
  }

  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = 0;
  dc->clip_y2 = 0;
  dc->clip_mask = FltkClipMask();
}

extern "C" IUP_SDK_API int iupdrvDrawBeginLayer(IdrawCanvas* dc, int alpha)
{
  (void)alpha;
  if (!dc) return 0;

  fltkDrawCommitClipMask(dc);

  FltkDrawLayer layer;
  layer.parent = dc->target;
  layer.grouped = 0;
  layer.clip_pushed = dc->clip_pushed;
  layer.clip_x1 = dc->clip_x1;
  layer.clip_y1 = dc->clip_y1;
  layer.clip_x2 = dc->clip_x2;
  layer.clip_y2 = dc->clip_y2;
  layer.matrix = dc->matrix;
  std::swap(layer.clip_mask, dc->clip_mask);

  if (dc->clip_pushed)
  {
    fl_pop_clip();
    dc->clip_pushed = 0;
  }

  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = 0;
  dc->clip_y2 = 0;

  Fl_Offscreen group = fl_create_offscreen(dc->w, dc->h);
  if (group)
  {
    if (dc->in_offscreen)
    {
      fl_end_offscreen();
      dc->in_offscreen = 0;
    }

    fl_begin_offscreen(group);
    dc->in_offscreen = 1;
    fl_copy_offscreen(0, 0, dc->w, dc->h, dc->target, 0, 0);
    dc->target = group;
    layer.grouped = 1;
  }

  dc->layers.push_back(std::move(layer));
  return 1;
}

extern "C" IUP_SDK_API void iupdrvDrawEndLayer(IdrawCanvas* dc, int alpha)
{
  if (!dc || dc->layers.empty()) return;

  fltkDrawCommitClipMask(dc);

  if (!dc->in_offscreen)
  {
    fl_begin_offscreen(dc->target);
    dc->in_offscreen = 1;
  }

  if (dc->clip_pushed)
  {
    fl_pop_clip();
    dc->clip_pushed = 0;
  }

  FltkDrawLayer& layer = dc->layers.back();
  Fl_RGB_Image* group_image = nullptr;
  Fl_RGB_Image* parent_image = nullptr;

  if (layer.grouped)
  {
    group_image = fltkDrawReadOffscreen(dc->target, 0, 0, dc->w, dc->h);
    fl_end_offscreen();
    fl_delete_offscreen(dc->target);
    dc->target = layer.parent;
    fl_begin_offscreen(dc->target);
    parent_image = fltkDrawReadOffscreen(dc->target, 0, 0, dc->w, dc->h);
  }

  int clip_pushed = layer.clip_pushed;
  dc->clip_x1 = layer.clip_x1;
  dc->clip_y1 = layer.clip_y1;
  dc->clip_x2 = layer.clip_x2;
  dc->clip_y2 = layer.clip_y2;
  dc->matrix = layer.matrix;
  std::swap(dc->clip_mask, layer.clip_mask);
  dc->layers.pop_back();

  if (clip_pushed)
  {
    fl_push_clip(dc->clip_x1, dc->clip_y1, dc->clip_x2 - dc->clip_x1 + 1, dc->clip_y2 - dc->clip_y1 + 1);
    dc->clip_pushed = 1;
  }
  else if (!dc->clip_mask.coverage.empty())
    fltkDrawBeginClipMask(dc);

  if (group_image && parent_image && alpha > 0 &&
      group_image->data_w() == parent_image->data_w() && group_image->data_h() == parent_image->data_h())
  {
    int w = group_image->data_w(), h = group_image->data_h();
    auto* data = new uchar[static_cast<size_t>(w) * h * 4];
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++)
      {
        const uchar* g = fltkDrawImagePixel(group_image, x, y);
        uchar* d = data + (static_cast<size_t>(y) * w + x) * 4;
        d[0] = g[0];
        d[1] = g[1];
        d[2] = g[2];
        d[3] = memcmp(g, fltkDrawImagePixel(parent_image, x, y), 3) ? static_cast<uchar>(alpha) : 0;
      }
    fltkDrawDeviceImage(data, w, h, 0, 0, dc->w, dc->h);
  }

  delete group_image;
  delete parent_image;
}

extern "C" IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int* x1, int* y1, int* x2, int* y2)
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

  if (!fltkDrawIdentity(dc))
  {
    unsigned char r, g, b;
    Fl::get_color(FL_SELECTION_COLOR, r, g, b);
    iupdrvDrawRectangle(dc, x1, y1, x2, y2, iupDrawColor(r, g, b, 255), IUP_DRAW_STROKE, 1);
    return;
  }

  fl_color(FL_SELECTION_COLOR);
  fl_line_style(FL_SOLID, 1);
  fl_rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

extern "C" IUP_SDK_API void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc) return;

  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);

  if (!fltkDrawIdentity(dc))
  {
    iupdrvDrawRectangle(dc, x1, y1, x2, y2, iupDrawColor(0, 0, 0, 255), IUP_DRAW_STROKE_DOT, 1);
    return;
  }

  fl_color(FL_BLACK);
  fl_line_style(FL_DOT, 1);
  fl_rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  fl_line_style(FL_SOLID, 0);
}

extern "C" IUP_SDK_API void iupdrvDrawPathFill(IdrawCanvas* dc, const IupPathSeg* segs, int count, const IupDrawSource* src, int rule)
{
  if (!dc || count <= 0) return;

  fltkDrawShapeFill(dc, fltkShapeFromPath(segs, count, &dc->matrix), src, rule);
}

extern "C" IUP_SDK_API void iupdrvDrawPathStroke(IdrawCanvas* dc, const IupPathSeg* segs, int count, const IupDrawSource* src, int style, int line_width)
{
  if (!dc || count <= 0) return;

  fltkDrawShapeStroke(dc, fltkShapeFromPath(segs, count, &dc->matrix), src, style, line_width);
}

extern "C" IUP_SDK_API void iupdrvDrawSetClipPath(IdrawCanvas* dc, const IupPathSeg* segs, int count, int rule)
{
  int x1, y1, x2, y2;

  if (!dc || count <= 0) return;

  iupDrawPathGetBBox(segs, count, &x1, &y1, &x2, &y2);
  fltkDrawCommitClipMask(dc);
  if (dc->clip_pushed)
  {
    fl_pop_clip();
    dc->clip_pushed = 0;
  }
  fltkDrawSetClipShape(dc, fltkShapeFromPath(segs, count, &dc->matrix), rule);
  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
}

extern "C" IUP_SDK_API int iupdrvDrawGetImageData(IdrawCanvas* dc, unsigned char* data)
{
  if (!dc || !data) return 0;

  int restart_clip = dc->clip_offscreen != 0;
  fltkDrawCommitClipMask(dc);

  if (!dc->in_offscreen)
  {
    fl_begin_offscreen(dc->target);
    dc->in_offscreen = 1;
  }

  uchar* pixels = fl_read_image(nullptr, 0, 0, dc->w, dc->h);
  if (!pixels)
  {
    if (restart_clip)
      fltkDrawBeginClipMask(dc);
    return 0;
  }

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
  if (restart_clip)
    fltkDrawBeginClipMask(dc);
  return 1;
}

extern "C" IUP_SDK_API int iupdrvCanvasGetImageData(Ihandle* ih, unsigned char* data, int w, int h)
{
  if (!ih || !data) return 0;

  auto offscreen = static_cast<Fl_Offscreen>(reinterpret_cast<size_t>(iupAttribGet(ih, "_IUP_FLTK_OFFSCREEN")));
  if (!offscreen) return 0;

  int buf_w = iupAttribGetInt(ih, "_IUP_FLTK_OFFSCREEN_W");
  int buf_h = iupAttribGetInt(ih, "_IUP_FLTK_OFFSCREEN_H");

  if (w > buf_w) w = buf_w;
  if (h > buf_h) h = buf_h;

  fl_begin_offscreen(offscreen);

  uchar* pixels = fl_read_image(nullptr, 0, 0, w, h);

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
