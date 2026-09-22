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
  Fl_Offscreen clip_offscreen;
  int in_offscreen;
  int clip_pushed;

  std::vector<unsigned char> clip_mask;

  int clip_x1, clip_y1, clip_x2, clip_y2;
  IupDrawMatrix matrix;
};

static int fltkDrawIdentity(const IdrawCanvas* dc)
{
  return dc->matrix.a == 1 && dc->matrix.b == 0 && dc->matrix.c == 0 &&
         dc->matrix.d == 1 && dc->matrix.e == 0 && dc->matrix.f == 0;
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
  *tx1 = (int)floor(min_x) - 2;
  *ty1 = (int)floor(min_y) - 2;
  *tx2 = (int)ceil(max_x) + 2;
  *ty2 = (int)ceil(max_y) + 2;
}

static int fltkDrawVisiblePixel(const IdrawCanvas* dc, int x, int y)
{
  if (x < 0 || y < 0 || x >= dc->w || y >= dc->h)
    return 0;
  return dc->clip_mask.empty() || dc->clip_mask[(size_t)y * dc->w + x];
}

static void fltkDrawSetColor(long color)
{
  unsigned char r = iupDrawRed(color);
  unsigned char g = iupDrawGreen(color);
  unsigned char b = iupDrawBlue(color);
  unsigned char a = iupDrawAlpha(color);

  if (a < 255)
  {
    Fl::set_color(FL_FREE_COLOR, r, g, b, a);
    fl_color(FL_FREE_COLOR);
    return;
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
  int closed;
};

typedef std::vector<FltkSubpath> FltkShape;

struct FltkSpan
{
  int y, x1, x2;
};

static double fltkDrawMatrixScale(const IupDrawMatrix* matrix)
{
  return sqrt(fabs(matrix->a * matrix->d - matrix->b * matrix->c));
}

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
  int steps = (int)ceil(length * fltkDrawMatrixScale(matrix) / 3.0);
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
    double t = (double)i / steps, u = 1 - t;
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

static std::vector<FltkSpan> fltkShapeSpans(const IdrawCanvas* dc, const FltkShape& shape, int rule)
{
  std::vector<FltkSpan> spans;
  std::vector<FltkPathEdge> edges;
  double min_y = 1e30, max_y = -1e30;

  for (size_t s = 0; s < shape.size(); s++)
  {
    const std::vector<FltkPoint>& pts = shape[s].points;
    size_t n = pts.size();
    if (n < 2)
      continue;
    for (size_t i = 0; i < n; i++)
    {
      const FltkPoint& p0 = pts[i];
      const FltkPoint& p1 = pts[(i + 1) % n];
      double x0, y0, x1, y1;
      fltkDrawTransformPoint(&dc->matrix, p0.x, p0.y, &x0, &y0);
      fltkDrawTransformPoint(&dc->matrix, p1.x, p1.y, &x1, &y1);
      if (y0 != y1)
        edges.push_back({x0, y0, x1, y1});
      if (y0 < min_y) min_y = y0;
      if (y0 > max_y) max_y = y0;
    }
  }

  if (edges.empty())
    return spans;

  int row1 = (int)floor(min_y);
  int row2 = (int)ceil(max_y);
  if (row1 < 0) row1 = 0;
  if (row2 > dc->h - 1) row2 = dc->h - 1;

  std::vector<std::pair<double, int> > crosses;
  for (int y = row1; y <= row2; y++)
  {
    double py = y + 0.5;
    crosses.clear();
    for (size_t i = 0; i < edges.size(); i++)
    {
      const FltkPathEdge& e = edges[i];
      if ((e.y1 <= py && e.y2 > py) || (e.y2 <= py && e.y1 > py))
        crosses.push_back(std::make_pair(e.x1 + (py - e.y1) * (e.x2 - e.x1) / (e.y2 - e.y1), e.y2 > e.y1 ? 1 : -1));
    }
    std::sort(crosses.begin(), crosses.end());
    int winding = 0;
    for (size_t i = 0; i + 1 < crosses.size(); i++)
    {
      winding = rule == IUP_PATH_RULE_EVENODD ? winding ^ 1 : winding + crosses[i].second;
      if (winding == 0)
        continue;
      int x1 = (int)ceil(crosses[i].first - 0.5);
      int x2 = (int)ceil(crosses[i + 1].first - 0.5);
      if (x1 < 0) x1 = 0;
      if (x2 > dc->w) x2 = dc->w;
      if (x2 > x1)
      {
        if (!spans.empty() && spans.back().y == y && spans.back().x2 == x1)
          spans.back().x2 = x2;
        else
          spans.push_back({y, x1, x2});
      }
    }
  }

  return spans;
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
      unsigned char r1 = iupDrawRed(colors[i]), g1 = iupDrawGreen(colors[i]), b1 = iupDrawBlue(colors[i]), a1 = iupDrawAlpha(colors[i]);
      unsigned char r2 = iupDrawRed(colors[i+1]), g2 = iupDrawGreen(colors[i+1]), b2 = iupDrawBlue(colors[i+1]), a2 = iupDrawAlpha(colors[i+1]);
      return iupDrawColor((unsigned char)(r1 + lt * (r2 - r1)), (unsigned char)(g1 + lt * (g2 - g1)), (unsigned char)(b1 + lt * (b2 - b1)), (unsigned char)(a1 + lt * (a2 - a1)));
    }
  }
  return colors[count - 1];
}

static long fltkDrawSourceColor(const IupDrawSource* src, double x, double y)
{
  if (src->type == IUP_SOURCE_SOLID)
    return src->color;
  if (src->type == IUP_SOURCE_RADIAL_GRADIENT)
    return fltkInterpolateStops(src->colors, src->offsets, src->count, (float)(sqrt((double)(x - src->cx) * (x - src->cx) + (double)(y - src->cy) * (y - src->cy)) / src->radius));

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
  float t = length2 > 0 ? (float)(((x - x0) * dx + (y - y0) * dy) / length2) : 0.0f;
  return fltkInterpolateStops(src->colors, src->offsets, src->count, t);
}

static void fltkDrawPaintSpans(IdrawCanvas* dc, const std::vector<FltkSpan>& spans, const IupDrawSource* src)
{
  if (src->type == IUP_SOURCE_SOLID)
  {
    fltkDrawSetColor(src->color);
    for (size_t i = 0; i < spans.size(); i++)
      fl_rectf(spans[i].x1, spans[i].y, spans[i].x2 - spans[i].x1, 1);
    return;
  }

  for (size_t i = 0; i < spans.size(); i++)
    for (int x = spans[i].x1; x < spans[i].x2; x++)
    {
      double ux = x + 0.5, uy = spans[i].y + 0.5;
      if (!fltkDrawIdentity(dc))
        fltkDrawInversePoint(&dc->matrix, ux, uy, &ux, &uy);
      fltkDrawSetColor(fltkDrawSourceColor(src, ux, uy));
      fl_point(x, spans[i].y);
    }
}

static void fltkDrawShapeFill(IdrawCanvas* dc, const FltkShape& shape, const IupDrawSource* src, int rule)
{
  fltkDrawPaintSpans(dc, fltkShapeSpans(dc, shape, rule), src);
}

static void fltkDrawShapeLines(IdrawCanvas* dc, const FltkShape& shape, int style, int line_width)
{
  const IupDrawMatrix* m = &dc->matrix;
  int width = (int)lround((line_width > 0 ? line_width : 1) * fltkDrawMatrixScale(m));
  if (width < 1) width = 1;

  fltkDrawSetLineStyle(style, width);
  fl_push_matrix();
  fl_mult_matrix(m->a, m->b, m->c, m->d, m->e + 0.5 * (m->a + m->c) - 0.5, m->f + 0.5 * (m->b + m->d) - 0.5);
  for (size_t s = 0; s < shape.size(); s++)
  {
    const FltkSubpath& sub = shape[s];
    if (sub.points.size() < 2)
      continue;
    if (sub.closed)
      fl_begin_loop();
    else
      fl_begin_line();
    for (size_t i = 0; i < sub.points.size(); i++)
      fl_vertex(sub.points[i].x, sub.points[i].y);
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
  if (src->type == IUP_SOURCE_SOLID)
  {
    fltkDrawSetColor(src->color);
    fltkDrawShapeLines(dc, shape, style, line_width);
    return;
  }

  Fl_Offscreen target = dc->clip_offscreen ? dc->clip_offscreen : dc->offscreen;
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
  fl_rectf(0, 0, dc->w, dc->h);
  fl_color(FL_BLACK);
  fltkDrawShapeLines(dc, shape, style, line_width);
  fl_pop_clip();
  uchar* pixels = fl_read_image(NULL, 0, 0, dc->w, dc->h);
  fl_end_offscreen();
  fl_delete_offscreen(mask_offscreen);
  fl_begin_offscreen(target);
  dc->in_offscreen = 1;

  if (pixels)
  {
    std::vector<FltkSpan> spans;
    for (int y = 0; y < dc->h; y++)
      for (int x = 0; x < dc->w; x++)
      {
        if (pixels[((size_t)y * dc->w + x) * 3] >= 128)
          continue;
        if (!spans.empty() && spans.back().y == y && spans.back().x2 == x)
          spans.back().x2 = x + 1;
        else
          spans.push_back({y, x, x + 1});
      }
    delete[] pixels;
    fltkDrawPaintSpans(dc, spans, src);
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

  fl_begin_offscreen(dc->offscreen);
  dc->in_offscreen = 1;

  for (int y = 0; y < dc->h; y++)
  {
    int x = 0;
    while (x < dc->w)
    {
      while (x < dc->w && !dc->clip_mask[(size_t)y * dc->w + x])
        x++;
      int x1 = x;
      while (x < dc->w && dc->clip_mask[(size_t)y * dc->w + x])
        x++;
      if (x > x1)
        fl_copy_offscreen(x1, y, x - x1, 1, dc->clip_offscreen, x1, y);
    }
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
  fl_copy_offscreen(0, 0, dc->w, dc->h, dc->offscreen, 0, 0);
}

extern "C" IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = new IdrawCanvas();

  dc->ih = ih;
  dc->widget = (Fl_Widget*)ih->handle;

  dc->w = dc->widget->w();
  dc->h = dc->widget->h();
  if (dc->w <= 0) dc->w = 1;
  if (dc->h <= 0) dc->h = 1;

  dc->offscreen = (Fl_Offscreen)(size_t)iupAttribGet(ih, "_IUP_FLTK_OFFSCREEN");
  if (dc->offscreen &&
      (iupAttribGetInt(ih, "_IUP_FLTK_OFFSCREEN_W") != dc->w || iupAttribGetInt(ih, "_IUP_FLTK_OFFSCREEN_H") != dc->h))
  {
    fl_delete_offscreen(dc->offscreen);
    dc->offscreen = 0;
  }

  if (!dc->offscreen)
  {
    dc->offscreen = fl_create_offscreen(dc->w, dc->h);
    iupAttribSet(ih, "_IUP_FLTK_OFFSCREEN", (char*)(size_t)dc->offscreen);
    iupAttribSetInt(ih, "_IUP_FLTK_OFFSCREEN_W", dc->w);
    iupAttribSetInt(ih, "_IUP_FLTK_OFFSCREEN_H", dc->h);
  }

  iupAttribSet(ih, "DRAWDRIVER", "FLTK");

  fl_begin_offscreen(dc->offscreen);
  dc->in_offscreen = 1;

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

    if (dc->clip_offscreen)
    {
      fl_delete_offscreen(dc->clip_offscreen);
      dc->clip_offscreen = 0;
    }

    if (dc->offscreen)
      fl_delete_offscreen(dc->offscreen);

    dc->w = new_w;
    dc->h = new_h;
    dc->clip_mask.clear();

    dc->offscreen = fl_create_offscreen(dc->w, dc->h);
    iupAttribSet(dc->ih, "_IUP_FLTK_OFFSCREEN", (char*)(size_t)dc->offscreen);
    iupAttribSetInt(dc->ih, "_IUP_FLTK_OFFSCREEN_W", dc->w);
    iupAttribSetInt(dc->ih, "_IUP_FLTK_OFFSCREEN_H", dc->h);

    fl_begin_offscreen(dc->offscreen);
    dc->in_offscreen = 1;
  }
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

  if (!fltkDrawIdentity(dc))
  {
    FltkShape shape;
    IupDrawSource src = fltkDrawSolidSource(color);
    fltkShapeMove(shape, x1, y1);
    fltkShapeLine(shape, x2, y2);
    fltkDrawShapeStroke(dc, shape, &src, style, line_width);
    return;
  }

  fltkDrawSetColor(color);
  fltkDrawSetLineStyle(style, line_width);
  fl_line(x1, y1, x2, y2);
}

extern "C" IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc) return;

  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);
  if (!fltkDrawIdentity(dc))
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
    fltkDrawSetLineStyle(style, line_width);
    fl_rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  while (a2 < a1)
    a2 += 360;

  if (!dc) return;

  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);
  if (!fltkDrawIdentity(dc))
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

  if (!fltkDrawIdentity(dc))
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
  if (!fltkDrawIdentity(dc))
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
  if (!fltkDrawIdentity(dc))
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
    fltkDrawSetLineStyle(style, line_width);
    fl_rounded_rect(x1, y1, w, h, corner_radius);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
  if (!dc) return;

  if (!fltkDrawIdentity(dc))
  {
    FltkShape shape;
    IupDrawSource src = fltkDrawSolidSource(color);
    fltkShapeMove(shape, x1, y1);
    fltkShapeCubic(shape, &dc->matrix, x2, y2, x3, y3, x4, y4);
    fltkDrawShapeStroke(dc, shape, &src, style, line_width);
    return;
  }

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

extern "C" IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, const long* colors, const float* offsets, int count)
{
  if (!dc) return;

  iupDrawOrderMinMax(&x1, &y1, &x2, &y2);
  if (!fltkDrawIdentity(dc))
  {
    IupDrawSource src = {};
    src.type = IUP_SOURCE_LINEAR_GRADIENT;
    src.x1 = x1; src.y1 = y1; src.x2 = x2; src.y2 = y2; src.angle = angle; src.count = count;
    memcpy(src.colors, colors, (size_t)count * sizeof(long));
    memcpy(src.offsets, offsets, (size_t)count * sizeof(float));
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

  if (!fltkDrawIdentity(dc))
  {
    IupDrawSource src = {};
    src.type = IUP_SOURCE_RADIAL_GRADIENT;
    src.cx = cx; src.cy = cy; src.radius = radius; src.count = count;
    memcpy(src.colors, colors, (size_t)count * sizeof(long));
    memcpy(src.offsets, offsets, (size_t)count * sizeof(float));
    FltkShape shape;
    fltkShapeArc(shape, &dc->matrix, cx + 0.5, cy + 0.5, radius, radius, 0, 360, 0);
    shape.back().closed = 1;
    fltkDrawShapeFill(dc, shape, &src, IUP_PATH_RULE_WINDING);
    return;
  }

  for (int r = radius; r >= 0; r--)
  {
    float t = (float)r / (float)radius;

    long c = fltkInterpolateStops(colors, offsets, count, t);
    fl_color(iupDrawRed(c), iupDrawGreen(c), iupDrawBlue(c));

    fl_pie(cx - r, cy - r, 2 * r + 1, 2 * r + 1, 0, 360);
  }
}

static void fltkDrawTextDecoration(const char* text, int len, int tx, int baseline, int underline, int strikeout)
{
  int tw = (int)(fl_width(text, len) + 0.5);
  int ascent = (int)fl_height() - (int)fl_descent();

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
    const char* q = (const char*)memchr(p, '\n', end - p);
    if (!q)
    {
      lines.push_back(std::string(p, end - p));
      break;
    }
    lines.push_back(std::string(p, q - p));
    p = q + 1;
  }
  return lines;
}

static std::string fltkDrawElide(const std::string& line, int w)
{
  if (fl_width(line.c_str(), (int)line.size()) <= w)
    return line;

  std::string cut = line;
  while (!cut.empty())
  {
    size_t n = cut.size() - 1;
    while (n > 0 && ((unsigned char)cut[n] & 0xC0) == 0x80)
      n--;
    cut.resize(n);
    std::string candidate = cut + "...";
    if (fl_width(candidate.c_str(), (int)candidate.size()) <= w)
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
  int layout_w = 0, layout_h = (int)lines.size() * line_h;
  double px, py, lx0, ly0;
  size_t i;

  for (i = 0; i < lines.size(); i++)
  {
    int line_w = (int)ceil(fl_width(lines[i].c_str(), (int)lines[i].size()));
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
    double line_w = fl_width(lines[i].c_str(), (int)lines[i].size());
    double bx = lx0, by = ly0 + i * line_h + baseline;
    double dx, dy;
    if (flags & IUP_DRAW_CENTER)
      bx += (layout_w - line_w) / 2;
    else if (flags & IUP_DRAW_RIGHT)
      bx += layout_w - line_w;
    dx = bx - px;
    dy = by - py;
    fl_draw((int)angle, lines[i].c_str(), (int)lines[i].size(), (int)lround(px + dx * c + dy * sn), (int)lround(py - dx * sn + dy * c));
  }
}

static void fltkDrawImageNative(Fl_Image* image, int img_w, int img_h, int x, int y, int w, int h, int sx, int sy, int sw, int sh)
{
  if (sx == 0 && sy == 0 && sw == img_w && sh == img_h)
  {
    if (w != sw || h != sh)
    {
      Fl_Image* scaled = image->copy(w, h);
      if (scaled) { scaled->draw(x, y); delete scaled; }
    }
    else image->draw(x, y);
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
  uchar* samples[2] = {NULL, NULL};
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
    samples[pass] = fl_read_image(NULL, 0, 0, w, h);
    fl_pop_clip();
    fl_end_offscreen();
    fl_delete_offscreen(offscreen);
  }
  fl_begin_offscreen(dc->clip_offscreen ? dc->clip_offscreen : dc->offscreen);
  dc->in_offscreen = 1;
  Fl_Image::RGB_scaling(old_scaling);
  if (samples[0] && samples[1])
  {
    rgba.resize((size_t)w * h * 4);
    for (size_t i = 0; i < (size_t)w * h; i++)
    {
      int delta = 0;
      for (int c = 0; c < 3; c++) delta += samples[1][i * 3 + c] - samples[0][i * 3 + c];
      int alpha = 255 - delta / 3;
      if (alpha < 0) alpha = 0;
      if (alpha > 255) alpha = 255;
      for (int c = 0; c < 3; c++) rgba[i * 4 + c] = alpha ? (unsigned char)((samples[0][i * 3 + c] * 255 + alpha / 2) / alpha > 255 ? 255 : (samples[0][i * 3 + c] * 255 + alpha / 2) / alpha) : 0;
      rgba[i * 4 + 3] = (unsigned char)alpha;
    }
  }
  delete[] samples[0];
  delete[] samples[1];
  return rgba;
}

struct FltkDrawMask
{
  int x, y, w, h, scale;
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
  if (x1 < (int)floor(min_x) - 1) x1 = (int)floor(min_x) - 1;
  if (y1 < (int)floor(min_y) - 1) y1 = (int)floor(min_y) - 1;
  if (x2 > (int)ceil(max_x) + 1) x2 = (int)ceil(max_x) + 1;
  if (y2 > (int)ceil(max_y) + 1) y2 = (int)ceil(max_y) + 1;
  if (x2 < x1 || y2 < y1)
    return 0;

  mask->scale = (int)ceil(2 * fltkDrawMatrixScale(&dc->matrix));
  if (mask->scale < 2) mask->scale = 2;
  if (mask->scale > 8) mask->scale = 8;
  mask->x = x1;
  mask->y = y1;
  mask->w = (x2 - x1 + 1) * mask->scale;
  mask->h = (y2 - y1 + 1) * mask->scale;
  if (mask->w < 1 || mask->h < 1 || (double)mask->w * mask->h > 64.0 * 1024 * 1024)
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
  uchar* pixels = fl_read_image(NULL, 0, 0, mask->w, mask->h);
  if (pixels)
  {
    mask->pixels.resize((size_t)mask->w * mask->h);
    for (size_t i = 0; i < mask->pixels.size(); i++)
      mask->pixels[i] = (unsigned char)(255 - pixels[i * 3]);
    delete[] pixels;
  }
  fl_end_offscreen();
  fl_delete_offscreen(offscreen);
  fl_begin_offscreen(dc->clip_offscreen ? dc->clip_offscreen : dc->offscreen);
  dc->in_offscreen = 1;
}

static unsigned char fltkDrawMaskSample(const FltkDrawMask* mask, double x, double y)
{
  double sx = (x - mask->x) * mask->scale;
  double sy = (y - mask->y) * mask->scale;
  int x0 = (int)floor(sx), y0 = (int)floor(sy);
  double fx = sx - x0, fy = sy - y0;
  if (x0 < 0 || y0 < 0 || x0 >= mask->w || y0 >= mask->h)
    return 0;
  int x1 = x0 + 1 < mask->w ? x0 + 1 : x0;
  int y1 = y0 + 1 < mask->h ? y0 + 1 : y0;
  double top = mask->pixels[(size_t)y0 * mask->w + x0] * (1 - fx) + mask->pixels[(size_t)y0 * mask->w + x1] * fx;
  double bottom = mask->pixels[(size_t)y1 * mask->w + x0] * (1 - fx) + mask->pixels[(size_t)y1 * mask->w + x1] * fx;
  return (unsigned char)lround(top * (1 - fy) + bottom * fy);
}

static void fltkDrawPaintMask(IdrawCanvas* dc, const FltkDrawMask* mask, const IupDrawSource* src)
{
  if (mask->pixels.empty()) return;
  int x1, y1, x2, y2;
  fltkDrawTransformBounds(&dc->matrix, mask->x, mask->y, mask->x + mask->w / mask->scale, mask->y + mask->h / mask->scale, &x1, &y1, &x2, &y2);
  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 >= dc->w) x2 = dc->w - 1;
  if (y2 >= dc->h) y2 = dc->h - 1;
  for (int y = y1; y <= y2; y++)
    for (int x = x1; x <= x2; x++)
    {
      if (!fltkDrawVisiblePixel(dc, x, y)) continue;
      double ux, uy;
      fltkDrawInversePoint(&dc->matrix, x + 0.5, y + 0.5, &ux, &uy);
      unsigned char coverage = fltkDrawMaskSample(mask, ux, uy);
      if (!coverage) continue;
      long color = fltkDrawSourceColor(src, ux, uy);
      unsigned char alpha = (unsigned char)((iupDrawAlpha(color) * coverage + 127) / 255);
      fltkDrawSetColor(iupDrawColor(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), alpha));
      fl_point(x, y);
    }
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
    int radius = (int)ceil(sqrt((double)layout_w * layout_w + (double)layout_h * layout_h)) + pad;
    double px = (flags & IUP_DRAW_LAYOUTCENTER) ? x + w / 2.0 : x;
    double py = (flags & IUP_DRAW_LAYOUTCENTER) ? y + h / 2.0 : y;
    bx1 = (int)floor(px) - radius;
    by1 = (int)floor(py) - radius;
    bx2 = (int)ceil(px) + radius;
    by2 = (int)ceil(py) + radius;
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
    fl_draw(text, tx, ty, tw, th, align, NULL, 0);
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

  int size = (int)lround(font_size * scale);
  if (size < 1) size = 1;
  int box_w = w > 0 ? (int)lround(w * scale) : w;
  int box_h = h > 0 ? (int)lround(h * scale) : h;
  double px, py;

  fl_font(font_id, size);

  if (total == 0)
  {
    fltkDrawTransformPoint(m, x, y, &px, &py);
    int tx = (int)lround(px), ty = (int)lround(py);
    Fl_Align align = FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE;
    if (flags & IUP_DRAW_CENTER) align = FL_ALIGN_CENTER | FL_ALIGN_INSIDE;
    else if (flags & IUP_DRAW_RIGHT) align = FL_ALIGN_RIGHT | FL_ALIGN_TOP | FL_ALIGN_INSIDE;
    if (flags & IUP_DRAW_WRAP) align |= FL_ALIGN_WRAP;
    if (flags & IUP_DRAW_CLIP) align |= FL_ALIGN_CLIP;
    fl_draw(text, tx, ty, box_w, box_h, align, NULL, 0);
    if ((underline || strikeout) && !(flags & IUP_DRAW_WRAP))
    {
      int text_w = 0, text_h = 0;
      int dx = tx, dy = ty;
      fl_measure(text, text_w, text_h, 0);
      if (flags & IUP_DRAW_CENTER) { dx = tx + (box_w - text_w) / 2; dy = ty + (box_h - text_h) / 2; }
      else if (flags & IUP_DRAW_RIGHT) dx = tx + box_w - text_w;
      fltkDrawTextDecoration(text, len, dx, dy + (int)fl_height() - (int)fl_descent(), underline, strikeout);
    }
    return 1;
  }

  if (angle != 0 && (flags & IUP_DRAW_LAYOUTCENTER))
  {
    fltkDrawTransformPoint(m, x + w / 2.0, y + h / 2.0, &px, &py);
    fltkDrawTextRotated(text, len, (int)lround(px - box_w / 2.0), (int)lround(py - box_h / 2.0), box_w, box_h, flags, total, 0, 0);
  }
  else
  {
    fltkDrawTransformPoint(m, x, y, &px, &py);
    fltkDrawTextRotated(text, len, (int)lround(px), (int)lround(py), box_w, box_h, flags & ~IUP_DRAW_LAYOUTCENTER, total, angle == 0 ? box_w : 0, angle == 0 ? box_h : 0);
  }
  return 1;
}

extern "C" IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  char stack_buf[512];
  char* text_buf = stack_buf;

  if (!dc || !text) return;

  if (len < 0)
    len = (int)strlen(text);
  if (len >= (int)sizeof(stack_buf))
    text_buf = (char*)malloc(len + 1);
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
    len = (int)elided.size();
  }

  if (!fltkDrawIdentity(dc))
  {
    if (!fltkDrawTextSimilarity(dc, text, len, x, y, w, h, flags, text_orientation, fl_font_id, fl_size, underline, strikeout))
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

    fl_draw(text, x, y, w, h, align, NULL, 0);

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

      fltkDrawTextDecoration(text, len, tx, ty + (int)fl_height() - (int)fl_descent(), underline, strikeout);
    }
  }

  if (text_buf != stack_buf)
    free(text_buf);
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

  if (!fltkDrawIdentity(dc))
  {
    std::vector<unsigned char> pixels = fltkDrawImagePixels(dc, image, img_w, img_h, w, h, sx, sy, sw, sh, quality);
    if (pixels.empty()) return;
    int dx1, dy1, dx2, dy2;
    fltkDrawTransformBounds(&dc->matrix, x, y, x + w, y + h, &dx1, &dy1, &dx2, &dy2);
    if (dx1 < 0) dx1 = 0;
    if (dy1 < 0) dy1 = 0;
    if (dx2 >= dc->w) dx2 = dc->w - 1;
    if (dy2 >= dc->h) dy2 = dc->h - 1;
    for (int dy = dy1; dy <= dy2; dy++)
      for (int dx = dx1; dx <= dx2; dx++)
      {
        if (!fltkDrawVisiblePixel(dc, dx, dy)) continue;
        double ux, uy;
        fltkDrawInversePoint(&dc->matrix, dx + 0.5, dy + 0.5, &ux, &uy);
        double px = ux - x - 0.5, py = uy - y - 0.5;
        if (px < -0.5 || py < -0.5 || px >= w - 0.5 || py >= h - 0.5) continue;
        int ix = (int)floor(px), iy = (int)floor(py);
        double fx = px - ix, fy = py - iy;
        if (quality == IUP_DRAW_IMAGE_NEAREST)
        {
          ix = (int)floor(px + 0.5); iy = (int)floor(py + 0.5); fx = fy = 0;
        }
        if (ix < 0) { ix = 0; fx = 0; }
        if (iy < 0) { iy = 0; fy = 0; }
        int ix1 = ix + 1 < w ? ix + 1 : ix;
        int iy1 = iy + 1 < h ? iy + 1 : iy;
        double weights[4] = {(1 - fx) * (1 - fy), fx * (1 - fy), (1 - fx) * fy, fx * fy};
        size_t indices[4] = {(size_t)iy * w + ix, (size_t)iy * w + ix1, (size_t)iy1 * w + ix, (size_t)iy1 * w + ix1};
        double alpha = 0, premul[3] = {0, 0, 0};
        for (int i = 0; i < 4; i++)
        {
          double a = pixels[indices[i] * 4 + 3] * weights[i];
          alpha += a;
          for (int c = 0; c < 3; c++) premul[c] += pixels[indices[i] * 4 + c] * a;
        }
        alpha *= opacity / 255.0;
        if (alpha <= 0) continue;
        unsigned char r = (unsigned char)lround(premul[0] / (alpha * 255.0 / opacity));
        unsigned char g = (unsigned char)lround(premul[1] / (alpha * 255.0 / opacity));
        unsigned char b = (unsigned char)lround(premul[2] / (alpha * 255.0 / opacity));
        fltkDrawSetColor(iupDrawColor(r, g, b, (unsigned char)lround(alpha)));
        fl_point(dx, dy);
      }
    return;
  }

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

  fltkDrawImageNative(image, img_w, img_h, x, y, w, h, sx, sy, sw, sh);

  Fl_Image::RGB_scaling(old_scaling);

  if (faded)
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

  dc->clip_mask.clear();

  if (!fltkDrawIdentity(dc))
  {
    dc->clip_mask.resize((size_t)dc->w * dc->h, 0);
    for (int y = 0; y < dc->h; y++)
      for (int x = 0; x < dc->w; x++)
      {
        double ux, uy;
        fltkDrawInversePoint(&dc->matrix, x + 0.5, y + 0.5, &ux, &uy);
        if (ux >= x1 && ux < x2 + 1.0 && uy >= y1 && uy < y2 + 1.0)
          dc->clip_mask[(size_t)y * dc->w + x] = 1;
      }
    fltkDrawBeginClipMask(dc);
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
  int radius = corner_radius;
  int max_radius = ((x2 - x1 + 1) < (y2 - y1 + 1) ? (x2 - x1 + 1) : (y2 - y1 + 1)) / 2;
  if (radius < 0) radius = 0;
  if (radius > max_radius) radius = max_radius;
  dc->clip_mask.assign((size_t)dc->w * dc->h, 0);
  for (int y = 0; y < dc->h; y++)
    for (int x = 0; x < dc->w; x++)
    {
      double ux = x + 0.5, uy = y + 0.5;
      if (!fltkDrawIdentity(dc))
        fltkDrawInversePoint(&dc->matrix, ux, uy, &ux, &uy);
      double qx = ux < x1 + radius ? x1 + radius - ux : (ux > x2 + 1 - radius ? ux - (x2 + 1 - radius) : 0);
      double qy = uy < y1 + radius ? y1 + radius - uy : (uy > y2 + 1 - radius ? uy - (y2 + 1 - radius) : 0);
      if (ux >= x1 && ux < x2 + 1.0 && uy >= y1 && uy < y2 + 1.0 && qx * qx + qy * qy <= (double)radius * radius)
        dc->clip_mask[(size_t)y * dc->w + x] = 1;
    }
  fltkDrawBeginClipMask(dc);
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
  dc->clip_mask.clear();
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
  std::vector<FltkSpan> spans = fltkShapeSpans(dc, fltkShapeFromPath(segs, count, &dc->matrix), rule);
  dc->clip_mask.assign((size_t)dc->w * dc->h, 0);
  for (size_t i = 0; i < spans.size(); i++)
    memset(&dc->clip_mask[(size_t)spans[i].y * dc->w + spans[i].x1], 1, (size_t)(spans[i].x2 - spans[i].x1));
  fltkDrawBeginClipMask(dc);
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
    fl_begin_offscreen(dc->offscreen);
    dc->in_offscreen = 1;
  }

  uchar* pixels = fl_read_image(NULL, 0, 0, dc->w, dc->h);
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
