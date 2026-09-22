/** \file
 * \brief Haiku Draw API (offscreen BBitmap + attached BView)
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <AffineTransform.h>
#include <Bitmap.h>
#include <Font.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <Picture.h>
#include <Point.h>
#include <Region.h>
#include <Shape.h>
#include <String.h>
#include <View.h>


extern "C" {
#include "iup.h"
#include "iup_drvdraw.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_draw.h"
#include "iup_image.h"
}

#include "iuphaiku_drv.h"



struct _IdrawCanvas
{
  Ihandle* ih;
  BBitmap* bm;
  BView* view;
  int w;
  int h;
  int clip_state;
  IupDrawMatrix matrix;
};

static rgb_color haikuColorFromLong(long c)
{
  rgb_color rc;
  rc.red   = iupDrawRed(c);
  rc.green = iupDrawGreen(c);
  rc.blue  = iupDrawBlue(c);
  rc.alpha = iupDrawAlpha(c);
  return rc;
}


static void haikuSetTransform(IdrawCanvas* dc, const IupDrawMatrix* matrix)
{
  dc->matrix = *matrix;
  if (dc->view)
    dc->view->SetTransform(BAffineTransform(matrix->a, matrix->b, matrix->c, matrix->d, matrix->e, matrix->f));
}

static int haikuIsIdentity(const IdrawCanvas* dc)
{
  return dc->matrix.a == 1 && dc->matrix.b == 0 && dc->matrix.c == 0 &&
         dc->matrix.d == 1 && dc->matrix.e == 0 && dc->matrix.f == 0;
}

static void haikuSetLineMode(BView* view, const IupDrawStroke* stroke)
{
  cap_mode cap = stroke->cap == IUP_DRAW_CAP_ROUND ? B_ROUND_CAP :
                 stroke->cap == IUP_DRAW_CAP_SQUARE ? B_SQUARE_CAP : B_BUTT_CAP;
  join_mode join = stroke->join == IUP_DRAW_JOIN_ROUND ? B_ROUND_JOIN :
                   stroke->join == IUP_DRAW_JOIN_BEVEL ? B_BEVEL_JOIN : B_MITER_JOIN;
  view->SetLineMode(cap, join, (float)IUP_DRAW_MITER_LIMIT);
}

static void haikuBeginStroke(IdrawCanvas* dc, long color, int style, int line_width, int center, IupDrawStroke* stroke)
{
  const IupDrawMatrix* m = &dc->matrix;
  iupDrawGetStroke(dc->ih, style, stroke);
  dc->view->SetHighColor(haikuColorFromLong(color));
  dc->view->SetPenSize(line_width > 0 ? (float)line_width : 1.0f);
  haikuSetLineMode(dc->view, stroke);
  if (center && !haikuIsIdentity(dc) && (line_width <= 0 || line_width % 2))
    dc->view->SetTransform(BAffineTransform(m->a, m->b, m->c, m->d, m->e + 0.5 * (m->a + m->c), m->f + 0.5 * (m->b + m->d)));
}

static void haikuEndStroke(IdrawCanvas* dc, int center)
{
  if (center && !haikuIsIdentity(dc))
    haikuSetTransform(dc, &dc->matrix);
}

static void haikuBeginClip(IdrawCanvas* dc)
{
  if (dc->clip_state)
    dc->view->PopState();
  dc->view->SetTransform(BAffineTransform());
  dc->view->PushState();
  dc->clip_state = 1;
  haikuSetTransform(dc, &dc->matrix);
}

static int haikuIsTranslation(const IdrawCanvas* dc)
{
  return dc->matrix.a == 1 && dc->matrix.b == 0 && dc->matrix.c == 0 && dc->matrix.d == 1;
}

static void haikuClipToRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (haikuIsTranslation(dc))
  {
    dc->view->ClipToRect(BRect(x1, y1, x2, y2));
    return;
  }

  BShape shape;
  shape.MoveTo(BPoint(x1, y1));
  shape.LineTo(BPoint(x2 + 1, y1));
  shape.LineTo(BPoint(x2 + 1, y2 + 1));
  shape.LineTo(BPoint(x1, y2 + 1));
  shape.Close();
  dc->view->ClipToShape(&shape);
}

extern "C" IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = (IdrawCanvas*)calloc(1, sizeof(IdrawCanvas));
  dc->ih = ih;
  dc->matrix.a = 1;
  dc->matrix.d = 1;
  iupdrvDrawUpdateSize(dc);
  iupAttribSet(ih, "DRAWDRIVER", "HAIKU");
  return dc;
}

extern "C" IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  if (!dc) return;
  if (dc->bm)
  {
    delete dc->bm;
  }
  free(dc);
}

extern "C" IUP_SDK_API void iupdrvDrawSetTransform(IdrawCanvas* dc, const IupDrawMatrix* matrix)
{
  if (!dc) return;
  if (dc->bm) dc->bm->Lock();
  haikuSetTransform(dc, matrix);
  if (dc->bm) dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  if (!dc) return;
  int w = dc->ih->currentwidth  > 0 ? dc->ih->currentwidth  : 1;
  int h = dc->ih->currentheight > 0 ? dc->ih->currentheight : 1;

  if (BView* view = (BView*)dc->ih->handle)
  {
    BRect bounds = view->Bounds();
    if (bounds.IntegerWidth() > 0)  w = bounds.IntegerWidth() + 1;
    if (bounds.IntegerHeight() > 0) h = bounds.IntegerHeight() + 1;
  }
  if (dc->bm && dc->w == w && dc->h == h) return;

  if (dc->bm) { delete dc->bm; dc->bm = NULL; dc->view = NULL; dc->clip_state = 0; }
  dc->w = w; dc->h = h;

  dc->bm = new BBitmap(BRect(0, 0, w - 1, h - 1), B_BITMAP_ACCEPTS_VIEWS, B_RGBA32);
  dc->view = new BView(dc->bm->Bounds(), "iup_dc", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
  dc->bm->AddChild(dc->view);
  dc->bm->Lock();
  unsigned char r, g, b;
  if (iupStrToRGB(iupAttribGetStr(dc->ih, "BGCOLOR"), &r, &g, &b))
  {
    rgb_color bg = { r, g, b, 255 };
    dc->view->SetHighColor(bg);
    dc->view->FillRect(dc->bm->Bounds());
  }
  dc->view->SetDrawingMode(B_OP_ALPHA);
  dc->view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
  haikuSetTransform(dc, &dc->matrix);
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc)
{
  if (!dc || !dc->bm) return;

  dc->bm->Lock();
  dc->view->Sync();
  dc->bm->Unlock();

  BView* canvas = (BView*)dc->ih->handle;
  if (!canvas) return;

  canvas->DrawBitmap(dc->bm, BPoint(0, 0));
}

extern "C" IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int* w, int* h)
{
  if (w) *w = dc ? dc->w : 0;
  if (h) *h = dc ? dc->h : 0;
}

typedef struct _HaikuDash
{
  const double* pattern;
  int count;
  int index;
  double remain;
} HaikuDash;

static int haikuDashInit(HaikuDash* dash, const IupDrawStroke* stroke)
{
  double offset, total = 0.0;
  int i;

  if (stroke->dash_count < 1)
    return 0;

  dash->pattern = stroke->dashes;
  dash->count = stroke->dash_count;
  dash->index = 0;
  dash->remain = stroke->dashes[0];

  for (i = 0; i < dash->count; i++)
    total += stroke->dashes[i];
  if (total <= 0.0)
    return 0;

  offset = fmod(stroke->dash_offset, total);
  if (offset < 0.0)
    offset += total;

  while (offset > 0.0)
  {
    double len = dash->pattern[dash->index];
    if (offset < len)
    {
      dash->remain = len - offset;
      break;
    }
    offset -= len;
    dash->index = (dash->index + 1) % dash->count;
    dash->remain = dash->pattern[dash->index];
  }

  return 1;
}

static void haikuStrokeDashSegment(BView* view, HaikuDash* dash, double x1, double y1, double x2, double y2)
{
  double dx = x2 - x1;
  double dy = y2 - y1;
  double length = sqrt(dx * dx + dy * dy);
  double offset = 0.0;

  if (length <= 0.0)
    return;

  while (offset < length)
  {
    double step = dash->remain < length - offset ? dash->remain : length - offset;
    double t1 = offset / length;
    double t2 = (offset + step) / length;
    if (step > 0.0 && (dash->index & 1) == 0)
      view->StrokeLine(BPoint(x1 + dx * t1, y1 + dy * t1), BPoint(x1 + dx * t2, y1 + dy * t2));
    offset += step;
    dash->remain -= step;
    if (dash->remain <= 0.0001)
    {
      dash->index = (dash->index + 1) % dash->count;
      dash->remain = dash->pattern[dash->index];
    }
  }
}

static void haikuStrokeDashCubic(BView* view, HaikuDash* dash, double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3)
{
  double px = x0, py = y0;
  int i;

  for (i = 1; i <= 20; i++)
  {
    double t = (double)i / 20.0;
    double mt = 1.0 - t;
    double x = mt * mt * mt * x0 + 3.0 * mt * mt * t * x1 + 3.0 * mt * t * t * x2 + t * t * t * x3;
    double y = mt * mt * mt * y0 + 3.0 * mt * mt * t * y1 + 3.0 * mt * t * t * y2 + t * t * t * y3;
    haikuStrokeDashSegment(view, dash, px, py, x, y);
    px = x;
    py = y;
  }
}

static void haikuStrokeDashedPath(BView* view, const IupPathSeg* segs, int count, const IupDrawStroke* stroke)
{
  HaikuDash dash;
  double cur_x = 0.0, cur_y = 0.0, sub_x = 0.0, sub_y = 0.0;
  int has_current = 0;
  int i;

  if (!haikuDashInit(&dash, stroke))
    return;

  for (i = 0; i < count; i++)
  {
    switch (segs[i].op)
    {
    case IUP_PATHSEG_MOVE_TO:
      cur_x = sub_x = segs[i].x1;
      cur_y = sub_y = segs[i].y1;
      has_current = 1;
      break;
    case IUP_PATHSEG_LINE_TO:
      if (has_current)
        haikuStrokeDashSegment(view, &dash, cur_x, cur_y, segs[i].x1, segs[i].y1);
      cur_x = segs[i].x1;
      cur_y = segs[i].y1;
      has_current = 1;
      break;
    case IUP_PATHSEG_CURVE_TO:
      if (has_current)
        haikuStrokeDashCubic(view, &dash, cur_x, cur_y, segs[i].x1, segs[i].y1, segs[i].x2, segs[i].y2, segs[i].x3, segs[i].y3);
      cur_x = segs[i].x3;
      cur_y = segs[i].y3;
      has_current = 1;
      break;
    case IUP_PATHSEG_QUAD_TO:
      if (has_current)
      {
        double c1x = cur_x + 2.0 / 3.0 * (segs[i].x1 - cur_x);
        double c1y = cur_y + 2.0 / 3.0 * (segs[i].y1 - cur_y);
        double c2x = segs[i].x2 + 2.0 / 3.0 * (segs[i].x1 - segs[i].x2);
        double c2y = segs[i].y2 + 2.0 / 3.0 * (segs[i].y1 - segs[i].y2);
        haikuStrokeDashCubic(view, &dash, cur_x, cur_y, c1x, c1y, c2x, c2y, segs[i].x2, segs[i].y2);
      }
      cur_x = segs[i].x2;
      cur_y = segs[i].y2;
      has_current = 1;
      break;
    case IUP_PATHSEG_ARC_TO:
    {
      double bez[24];
      int j, n = iupDrawPathArcToCurves(&segs[i], bez);
      for (j = 0; j < n; j++)
      {
        const double* c = bez + j * 6;
        haikuStrokeDashCubic(view, &dash, cur_x, cur_y, c[0], c[1], c[2], c[3], c[4], c[5]);
        cur_x = c[4];
        cur_y = c[5];
      }
      has_current = 1;
      break;
    }
    case IUP_PATHSEG_CLOSE:
      if (has_current)
        haikuStrokeDashSegment(view, &dash, cur_x, cur_y, sub_x, sub_y);
      cur_x = sub_x;
      cur_y = sub_y;
      break;
    }
  }
}

static IupPathSeg haikuSegPoint(int op, int x, int y)
{
  IupPathSeg seg;
  memset(&seg, 0, sizeof(seg));
  seg.op = (unsigned char)op;
  seg.x1 = x;
  seg.y1 = y;
  return seg;
}

static IupPathSeg haikuSegCurve(int x1, int y1, int x2, int y2, int x3, int y3)
{
  IupPathSeg seg;
  memset(&seg, 0, sizeof(seg));
  seg.op = IUP_PATHSEG_CURVE_TO;
  seg.x1 = x1;
  seg.y1 = y1;
  seg.x2 = x2;
  seg.y2 = y2;
  seg.x3 = x3;
  seg.y3 = y3;
  return seg;
}

static IupPathSeg haikuSegArc(int cx, int cy, int rx, int ry, double a1, double a2)
{
  IupPathSeg seg;
  memset(&seg, 0, sizeof(seg));
  seg.op = IUP_PATHSEG_ARC_TO;
  seg.x1 = cx;
  seg.y1 = cy;
  seg.x2 = rx;
  seg.y2 = ry;
  seg.a1 = a1;
  seg.a2 = a2;
  return seg;
}

static IupPathSeg haikuSegClose()
{
  IupPathSeg seg;
  memset(&seg, 0, sizeof(seg));
  seg.op = IUP_PATHSEG_CLOSE;
  return seg;
}

static void haikuArcStart(int cx, int cy, int rx, int ry, double angle, int* x, int* y)
{
  *x = (int)floor(cx + rx * cos(angle * IUP_DEG2RAD) + 0.5);
  *y = (int)floor(cy - ry * sin(angle * IUP_DEG2RAD) + 0.5);
}

extern "C" IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  IupDrawStroke stroke;
  if (!dc || !dc->bm) return;
  dc->bm->Lock();
  haikuBeginStroke(dc, color, style, line_width, 0, &stroke);
  if (stroke.dash_count > 0)
  {
    IupPathSeg segs[2];
    segs[0] = haikuSegPoint(IUP_PATHSEG_MOVE_TO, x1, y1);
    segs[1] = haikuSegPoint(IUP_PATHSEG_LINE_TO, x2, y2);
    haikuStrokeDashedPath(dc->view, segs, 2, &stroke);
  }
  else
    dc->view->StrokeLine(BPoint(x1, y1), BPoint(x2, y2));
  haikuEndStroke(dc, 0);
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc || !dc->bm) return;
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  BRect r(x1, y1, x2, y2);
  dc->bm->Lock();
  if (style == IUP_DRAW_FILL)
  {
    dc->view->SetHighColor(haikuColorFromLong(color));
    dc->view->FillRect(r);
  }
  else
  {
    IupDrawStroke stroke;
    haikuBeginStroke(dc, color, style, line_width, 1, &stroke);
    if (stroke.dash_count > 0)
    {
      IupPathSeg segs[5];
      segs[0] = haikuSegPoint(IUP_PATHSEG_MOVE_TO, x1, y1);
      segs[1] = haikuSegPoint(IUP_PATHSEG_LINE_TO, x2, y1);
      segs[2] = haikuSegPoint(IUP_PATHSEG_LINE_TO, x2, y2);
      segs[3] = haikuSegPoint(IUP_PATHSEG_LINE_TO, x1, y2);
      segs[4] = haikuSegClose();
      haikuStrokeDashedPath(dc->view, segs, 5, &stroke);
    }
    else
      dc->view->StrokeRect(r);
    haikuEndStroke(dc, 1);
  }
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius, long color, int style, int line_width)
{
  if (!dc || !dc->bm) return;
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  BRect r(x1, y1, x2, y2);
  float rad = (float)corner_radius;
  dc->bm->Lock();
  if (style == IUP_DRAW_FILL)
  {
    dc->view->SetHighColor(haikuColorFromLong(color));
    dc->view->FillRoundRect(r, rad, rad);
  }
  else
  {
    IupDrawStroke stroke;
    haikuBeginStroke(dc, color, style, line_width, 0, &stroke);
    if (stroke.dash_count > 0 && corner_radius > 0)
    {
      IupPathSeg segs[10];
      int max_radius = ((x2 - x1) < (y2 - y1) ? (x2 - x1) : (y2 - y1)) / 2;
      if (corner_radius > max_radius) corner_radius = max_radius;
      segs[0] = haikuSegPoint(IUP_PATHSEG_MOVE_TO, x2, y2 - corner_radius);
      segs[1] = haikuSegPoint(IUP_PATHSEG_LINE_TO, x2, y1 + corner_radius);
      segs[2] = haikuSegArc(x2 - corner_radius, y1 + corner_radius, corner_radius, corner_radius, 0, 90);
      segs[3] = haikuSegPoint(IUP_PATHSEG_LINE_TO, x1 + corner_radius, y1);
      segs[4] = haikuSegArc(x1 + corner_radius, y1 + corner_radius, corner_radius, corner_radius, 90, 180);
      segs[5] = haikuSegPoint(IUP_PATHSEG_LINE_TO, x1, y2 - corner_radius);
      segs[6] = haikuSegArc(x1 + corner_radius, y2 - corner_radius, corner_radius, corner_radius, 180, 270);
      segs[7] = haikuSegPoint(IUP_PATHSEG_LINE_TO, x2 - corner_radius, y2);
      segs[8] = haikuSegArc(x2 - corner_radius, y2 - corner_radius, corner_radius, corner_radius, 270, 360);
      segs[9] = haikuSegClose();
      haikuStrokeDashedPath(dc->view, segs, 10, &stroke);
    }
    else if (stroke.dash_count > 0)
    {
      IupPathSeg segs[5];
      segs[0] = haikuSegPoint(IUP_PATHSEG_MOVE_TO, x1, y1);
      segs[1] = haikuSegPoint(IUP_PATHSEG_LINE_TO, x2, y1);
      segs[2] = haikuSegPoint(IUP_PATHSEG_LINE_TO, x2, y2);
      segs[3] = haikuSegPoint(IUP_PATHSEG_LINE_TO, x1, y2);
      segs[4] = haikuSegClose();
      haikuStrokeDashedPath(dc->view, segs, 5, &stroke);
    }
    else
      dc->view->StrokeRoundRect(r, rad, rad);
    haikuEndStroke(dc, 0);
  }
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  if (!dc || !dc->bm) return;
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  BRect r(x1, y1, x2, y2);
  while (a2 < a1)
    a2 += 360;
  float start = (float)a1;
  float sweep = (float)(a2 - a1);
  dc->bm->Lock();
  if (style == IUP_DRAW_FILL)
  {
    dc->view->SetHighColor(haikuColorFromLong(color));
    dc->view->FillArc(r, start, sweep);
  }
  else
  {
    IupDrawStroke stroke;
    haikuBeginStroke(dc, color, style, line_width, 0, &stroke);
    if (stroke.dash_count > 0)
    {
      IupPathSeg segs[2];
      int cx = (x1 + x2) / 2, cy = (y1 + y2) / 2;
      int rx = (x2 - x1) / 2, ry = (y2 - y1) / 2;
      int sx, sy;
      haikuArcStart(cx, cy, rx, ry, a1, &sx, &sy);
      segs[0] = haikuSegPoint(IUP_PATHSEG_MOVE_TO, sx, sy);
      segs[1] = haikuSegArc(cx, cy, rx, ry, a1, a2);
      haikuStrokeDashedPath(dc->view, segs, 2, &stroke);
    }
    else
      dc->view->StrokeArc(r, start, sweep);
    haikuEndStroke(dc, 0);
  }
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc || !dc->bm) return;
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  BRect r(x1, y1, x2, y2);
  dc->bm->Lock();
  if (style == IUP_DRAW_FILL)
  {
    dc->view->SetHighColor(haikuColorFromLong(color));
    dc->view->FillEllipse(r);
  }
  else
  {
    IupDrawStroke stroke;
    haikuBeginStroke(dc, color, style, line_width, 0, &stroke);
    if (stroke.dash_count > 0)
    {
      IupPathSeg segs[2];
      int cx = (x1 + x2) / 2, cy = (y1 + y2) / 2;
      int rx = (x2 - x1) / 2, ry = (y2 - y1) / 2;
      segs[0] = haikuSegPoint(IUP_PATHSEG_MOVE_TO, cx + rx, cy);
      segs[1] = haikuSegArc(cx, cy, rx, ry, 0, 360);
      haikuStrokeDashedPath(dc->view, segs, 2, &stroke);
    }
    else
      dc->view->StrokeEllipse(r);
    haikuEndStroke(dc, 0);
  }
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  if (!dc || !dc->bm || !points || count < 2) return;
  BPoint* pts = (BPoint*)malloc(sizeof(BPoint) * count);
  for (int i = 0; i < count; ++i)
    pts[i] = BPoint((float)points[i*2], (float)points[i*2 + 1]);

  dc->bm->Lock();
  if (style == IUP_DRAW_FILL)
  {
    dc->view->SetHighColor(haikuColorFromLong(color));
    dc->view->FillPolygon(pts, count);
  }
  else
  {
    IupDrawStroke stroke;
    haikuBeginStroke(dc, color, style, line_width, 1, &stroke);
    if (stroke.dash_count > 0)
    {
      IupPathSeg* segs = (IupPathSeg*)malloc(sizeof(IupPathSeg) * (count + 1));
      for (int i = 0; i < count; ++i)
        segs[i] = haikuSegPoint(i == 0 ? IUP_PATHSEG_MOVE_TO : IUP_PATHSEG_LINE_TO, points[i*2], points[i*2 + 1]);
      segs[count] = haikuSegClose();
      haikuStrokeDashedPath(dc->view, segs, count + 1, &stroke);
      free(segs);
    }
    else
      dc->view->StrokePolygon(pts, count, true);
    haikuEndStroke(dc, 1);
  }
  dc->bm->Unlock();
  free(pts);
}

extern "C" IUP_SDK_API void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
{
  if (!dc || !dc->bm) return;
  dc->bm->Lock();
  dc->view->SetHighColor(haikuColorFromLong(color));
  dc->view->FillRect(BRect(x, y, x, y));
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
  if (!dc || !dc->bm) return;
  BPoint cp[4] = { BPoint(x1,y1), BPoint(x2,y2), BPoint(x3,y3), BPoint(x4,y4) };
  dc->bm->Lock();
  if (style == IUP_DRAW_FILL)
  {
    dc->view->SetHighColor(haikuColorFromLong(color));
    dc->view->FillBezier(cp);
  }
  else
  {
    IupDrawStroke stroke;
    haikuBeginStroke(dc, color, style, line_width, 0, &stroke);
    if (stroke.dash_count > 0)
    {
      IupPathSeg segs[2];
      segs[0] = haikuSegPoint(IUP_PATHSEG_MOVE_TO, x1, y1);
      segs[1] = haikuSegCurve(x2, y2, x3, y3, x4, y4);
      haikuStrokeDashedPath(dc->view, segs, 2, &stroke);
    }
    else
      dc->view->StrokeBezier(cp);
    haikuEndStroke(dc, 0);
  }
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawQuadraticBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, long color, int style, int line_width)
{
  int cx1 = x1 + (2 * (x2 - x1)) / 3;
  int cy1 = y1 + (2 * (y2 - y1)) / 3;
  int cx2 = x3 + (2 * (x2 - x3)) / 3;
  int cy2 = y3 + (2 * (y2 - y3)) / 3;
  iupdrvDrawBezier(dc, x1, y1, cx1, cy1, cx2, cy2, x3, y3, color, style, line_width);
}

extern "C" IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, const long* colors, const float* offsets, int count)
{
  if (!dc || !dc->bm) return;
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  BRect r(x1, y1, x2, y2);

  float cx = (x1 + x2) / 2.0f, cy = (y1 + y2) / 2.0f;
  float rad = angle * 3.14159265f / 180.0f;
  float dx = cosf(rad) * r.Width() * 0.5f;
  float dy = sinf(rad) * r.Height() * 0.5f;

  BGradientLinear grad(BPoint(cx - dx, cy - dy), BPoint(cx + dx, cy + dy));
  for (int i = 0; i < count; i++)
    grad.AddColor(haikuColorFromLong(colors[i]), offsets[i] * 255.0f);

  dc->bm->Lock();
  dc->view->FillRect(r, grad);
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, const long* colors, const float* offsets, int count)
{
  if (!dc || !dc->bm) return;
  BPoint c(cx, cy);
  BGradientRadial grad(c, (float)radius);
  for (int i = 0; i < count; i++)
    grad.AddColor(haikuColorFromLong(colors[i]), offsets[i] * 255.0f);

  BRect r(cx - radius, cy - radius, cx + radius, cy + radius);
  dc->bm->Lock();
  dc->view->FillEllipse(r, grad);
  dc->bm->Unlock();
}

static BShape* haikuBuildShape(const IupPathSeg* segs, int count)
{
  BShape* shape = new BShape();
  float cur_x = 0.0f, cur_y = 0.0f;
  float sub_x = 0.0f, sub_y = 0.0f;

  for (int i = 0; i < count; i++)
  {
    switch (segs[i].op)
    {
    case IUP_PATHSEG_MOVE_TO:
      shape->MoveTo(BPoint((float)segs[i].x1, (float)segs[i].y1));
      cur_x = sub_x = segs[i].x1;
      cur_y = sub_y = segs[i].y1;
      break;
    case IUP_PATHSEG_LINE_TO:
      shape->LineTo(BPoint((float)segs[i].x1, (float)segs[i].y1));
      cur_x = segs[i].x1;
      cur_y = segs[i].y1;
      break;
    case IUP_PATHSEG_CURVE_TO:
      shape->BezierTo(BPoint((float)segs[i].x1, (float)segs[i].y1),
                      BPoint((float)segs[i].x2, (float)segs[i].y2),
                      BPoint((float)segs[i].x3, (float)segs[i].y3));
      cur_x = segs[i].x3;
      cur_y = segs[i].y3;
      break;
    case IUP_PATHSEG_QUAD_TO:
    {
      float c1x = cur_x + 2.0f / 3.0f * ((float)segs[i].x1 - cur_x);
      float c1y = cur_y + 2.0f / 3.0f * ((float)segs[i].y1 - cur_y);
      float c2x = (float)segs[i].x2 + 2.0f / 3.0f * ((float)segs[i].x1 - (float)segs[i].x2);
      float c2y = (float)segs[i].y2 + 2.0f / 3.0f * ((float)segs[i].y1 - (float)segs[i].y2);
      shape->BezierTo(BPoint(c1x, c1y), BPoint(c2x, c2y), BPoint((float)segs[i].x2, (float)segs[i].y2));
      cur_x = segs[i].x2;
      cur_y = segs[i].y2;
      break;
    }
    case IUP_PATHSEG_ARC_TO:
    {
      double bez[24];
      int j, n = iupDrawPathArcToCurves(&segs[i], bez);
      for (j = 0; j < n; j++)
        shape->BezierTo(BPoint((float)bez[j * 6], (float)bez[j * 6 + 1]),
                        BPoint((float)bez[j * 6 + 2], (float)bez[j * 6 + 3]),
                        BPoint((float)bez[j * 6 + 4], (float)bez[j * 6 + 5]));
      if (n > 0)
      {
        cur_x = (float)bez[n * 6 - 2];
        cur_y = (float)bez[n * 6 - 1];
      }
      break;
    }
    case IUP_PATHSEG_CLOSE:
      shape->Close();
      cur_x = sub_x;
      cur_y = sub_y;
      break;
    }
  }

  return shape;
}

static BGradient* haikuBuildGradient(const IupDrawSource* src)
{
  if (src->type == IUP_SOURCE_LINEAR_GRADIENT)
  {
    int gx1 = src->x1, gy1 = src->y1, gx2 = src->x2, gy2 = src->y2;
    iupDrawCheckSwapCoord(gx1, gx2);
    iupDrawCheckSwapCoord(gy1, gy2);

    float cx = (gx1 + gx2) / 2.0f, cy = (gy1 + gy2) / 2.0f;
    float rad = src->angle * 3.14159265f / 180.0f;
    float dx = cosf(rad) * (gx2 - gx1) * 0.5f;
    float dy = sinf(rad) * (gy2 - gy1) * 0.5f;

    BGradientLinear* g = new BGradientLinear(BPoint(cx - dx, cy - dy), BPoint(cx + dx, cy + dy));
    for (int i = 0; i < src->count; i++)
      g->AddColor(haikuColorFromLong(src->colors[i]), src->offsets[i] * 255.0f);
    return g;
  }
  else
  {
    BGradientRadial* g = new BGradientRadial(BPoint((float)src->cx, (float)src->cy), (float)src->radius);
    for (int i = 0; i < src->count; i++)
      g->AddColor(haikuColorFromLong(src->colors[i]), src->offsets[i] * 255.0f);
    return g;
  }
}

extern "C" IUP_SDK_API void iupdrvDrawPathFill(IdrawCanvas* dc, const IupPathSeg* segs, int count, const IupDrawSource* src, int rule)
{
  if (!dc || !dc->bm) return;
  BShape* shape = haikuBuildShape(segs, count);
  if (!shape) return;

  dc->bm->Lock();
  dc->view->MovePenTo(0, 0);
  dc->view->SetFillRule(rule == IUP_PATH_RULE_EVENODD ? B_EVEN_ODD : B_NONZERO);
  if (src->type == IUP_SOURCE_SOLID)
  {
    dc->view->SetHighColor(haikuColorFromLong(src->color));
    dc->view->FillShape(shape);
  }
  else
  {
    BGradient* grad = haikuBuildGradient(src);
    dc->view->FillShape(shape, *grad);
    delete grad;
  }
  dc->view->SetFillRule(B_NONZERO);
  dc->bm->Unlock();

  delete shape;
}

extern "C" IUP_SDK_API void iupdrvDrawPathStroke(IdrawCanvas* dc, const IupPathSeg* segs, int count, const IupDrawSource* src, int style, int line_width)
{
  IupDrawStroke stroke;
  if (!dc || !dc->bm) return;
  BShape* shape = haikuBuildShape(segs, count);
  if (!shape) return;

  iupDrawGetStroke(dc->ih, style, &stroke);

  dc->bm->Lock();
  dc->view->MovePenTo(0, 0);
  dc->view->SetPenSize(line_width > 0 ? (float)line_width : 1.0f);
  haikuSetLineMode(dc->view, &stroke);
  if (src->type == IUP_SOURCE_SOLID && stroke.dash_count > 0)
  {
    dc->view->SetHighColor(haikuColorFromLong(src->color));
    haikuStrokeDashedPath(dc->view, segs, count, &stroke);
  }
  else if (src->type == IUP_SOURCE_SOLID)
  {
    dc->view->SetHighColor(haikuColorFromLong(src->color));
    dc->view->StrokeShape(shape);
  }
  else
  {
    BGradient* grad = haikuBuildGradient(src);
    dc->view->StrokeShape(shape, *grad);
    delete grad;
  }
  dc->bm->Unlock();

  delete shape;
}

extern "C" IUP_SDK_API void iupdrvDrawSetClipPath(IdrawCanvas* dc, const IupPathSeg* segs, int count, int rule)
{
  if (!dc || !dc->bm) return;
  BShape* shape = haikuBuildShape(segs, count);
  if (!shape) return;

  int x1, y1, x2, y2;
  iupDrawPathGetBBox(segs, count, &x1, &y1, &x2, &y2);

  dc->bm->Lock();
  dc->view->MovePenTo(0, 0);
  haikuBeginClip(dc);
  if (rule == IUP_PATH_RULE_EVENODD)
  {
    BPicture picture;
    dc->view->SetFillRule(B_EVEN_ODD);
    dc->view->BeginPicture(&picture);
    dc->view->FillShape(shape);
    dc->view->EndPicture();
    dc->view->ClipToPicture(&picture);
    dc->view->SetFillRule(B_NONZERO);
  }
  else
    dc->view->ClipToShape(shape);
  dc->bm->Unlock();

  iupAttribSetStrf(dc->ih, "_IUPHAIKU_CLIP", "%d %d %d %d", x1, y1, x2, y2);

  delete shape;
}

extern "C" IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  if (!dc || !dc->bm || !text) return;

  BFont* bfont = iuphaikuGetBFont(font);

  dc->bm->Lock();
  dc->view->PushState();
  if (bfont) dc->view->SetFont(bfont);
  dc->view->SetHighColor(haikuColorFromLong(color));

  font_height fh;
  dc->view->GetFontHeight(&fh);

  if (flags & IUP_DRAW_CLIP)
    haikuClipToRect(dc, x, y, x + w - 1, y + h - 1);

  if (text_orientation != 0.0)
  {
    BAffineTransform trans;
    double px = x, py = y;
    if (flags & IUP_DRAW_LAYOUTCENTER)
    {
      int layout_w = 0, layout_h = 0;
      iupDrawGetTextSize(dc->ih, text, len, &layout_w, &layout_h, 0);
      px = x + w / 2.0;
      py = y + h / 2.0;
      x = (int)floor(px - layout_w / 2.0 + 0.5);
      y = (int)floor(py - layout_h / 2.0 + 0.5);
      w = layout_w;
    }
    trans.RotateBy(BPoint((float)px, (float)py), -text_orientation * 3.14159265358979323846 / 180.0);
    dc->view->SetTransform(trans);
  }

  int line_h = (int)(fh.ascent + fh.descent + fh.leading + 0.5f);
  int total_len = (len < 0) ? (int)strlen(text) : len;
  const char* line = text;
  const char* end  = text + total_len;
  int line_y = y;

  while (line <= end)
  {
    const char* nl = line;
    while (nl < end && *nl != '\n') ++nl;
    int line_len = (int)(nl - line);

    const char* draw_text = line;
    int draw_len = line_len;
    BString truncated;

    if ((flags & IUP_DRAW_ELLIPSIS) && w > 0 && dc->view->StringWidth(line, line_len) > (float)w)
    {
      BFont view_font;
      dc->view->GetFont(&view_font);
      truncated.SetTo(line, line_len);
      view_font.TruncateString(&truncated, B_TRUNCATE_END, (float)w);
      draw_text = truncated.String();
      draw_len = (int)truncated.Length();
    }

    int line_x = x;
    if ((flags & 0x000F) == IUP_DRAW_CENTER || (flags & 0x000F) == IUP_DRAW_RIGHT)
    {
      float lw = dc->view->StringWidth(draw_text, draw_len);
      if ((flags & 0x000F) == IUP_DRAW_CENTER) line_x = x + (w - (int)lw) / 2;
      else                                     line_x = x + (w - (int)lw);
    }

    dc->view->DrawString(draw_text, draw_len, BPoint((float)line_x, (float)line_y + fh.ascent));

    if (nl >= end) break;
    line = nl + 1;
    line_y += line_h;
  }

  dc->view->PopState();
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, long tint, int opacity, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int quality)
{
  if (!dc || !dc->bm || !name) return;
  BBitmap* img = (BBitmap*)iupImageGetImageTint(name, dc->ih, make_inactive, bgcolor, tint);
  if (!img) return;

  int img_w = (int)(img->Bounds().Width() + 1);
  int img_h = (int)(img->Bounds().Height() + 1);

  if (sw <= 0 || sh <= 0)
  {
    sx = 0;
    sy = 0;
    sw = img_w;
    sh = img_h;
  }
  if (w <= 0) w = sw;
  if (h <= 0) h = sh;

  BBitmap* faded = NULL;
  if (opacity < 255 && img->ColorSpace() == B_RGBA32)
  {
    faded = new BBitmap(img->Bounds(), B_RGBA32);
    if (faded->IsValid())
    {
      uint8* d = (uint8*)faded->Bits();
      uint8* s = (uint8*)img->Bits();
      int32 len = img->BitsLength();
      memcpy(d, s, len);
      for (int32 i = 3; i < len; i += 4)
        d[i] = (uint8)((d[i] * opacity) / 255);
      img = faded;
    }
    else
    {
      delete faded;
      faded = NULL;
    }
  }

  BRect src(sx, sy, sx + sw - 1, sy + sh - 1);
  BRect dst(x, y, x + w - 1, y + h - 1);
  dc->bm->Lock();
  dc->view->SetDrawingMode(B_OP_ALPHA);
  dc->view->DrawBitmap(img, src, dst, quality == IUP_DRAW_IMAGE_NEAREST ? 0 : B_FILTER_BITMAP_BILINEAR);
  dc->bm->Unlock();

  delete faded;
}

extern "C" IUP_SDK_API void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc || !dc->bm) return;
  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  iupAttribSetStrf(dc->ih, "_IUPHAIKU_CLIP", "%d %d %d %d", x1, y1, x2, y2);

  dc->bm->Lock();
  haikuBeginClip(dc);
  haikuClipToRect(dc, x1, y1, x2, y2);
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius)
{
  if (!dc || !dc->bm) return;
  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  float r = (float)corner_radius;
  float max_r = ((x2 - x1 + 1) < (y2 - y1 + 1) ? (x2 - x1 + 1) : (y2 - y1 + 1)) / 2.0f;
  if (r > max_r) r = max_r;
  if (r <= 0)
  {
    iupdrvDrawSetClipRect(dc, x1, y1, x2, y2);
    return;
  }

  iupAttribSetStrf(dc->ih, "_IUPHAIKU_CLIP", "%d %d %d %d", x1, y1, x2, y2);

  float l = (float)x1, t = (float)y1, rt = (float)(x2 + 1), b = (float)(y2 + 1), k = r * 0.5522847f;
  BShape shape;
  shape.MoveTo(BPoint(l + r, t));
  shape.LineTo(BPoint(rt - r, t));
  shape.BezierTo(BPoint(rt - r + k, t), BPoint(rt, t + r - k), BPoint(rt, t + r));
  shape.LineTo(BPoint(rt, b - r));
  shape.BezierTo(BPoint(rt, b - r + k), BPoint(rt - r + k, b), BPoint(rt - r, b));
  shape.LineTo(BPoint(l + r, b));
  shape.BezierTo(BPoint(l + r - k, b), BPoint(l, b - r + k), BPoint(l, b - r));
  shape.LineTo(BPoint(l, t + r));
  shape.BezierTo(BPoint(l, t + r - k), BPoint(l + r - k, t), BPoint(l + r, t));
  shape.Close();

  dc->bm->Lock();
  haikuBeginClip(dc);
  dc->view->ClipToShape(&shape);
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  if (!dc || !dc->bm) return;
  iupAttribSet(dc->ih, "_IUPHAIKU_CLIP", NULL);
  dc->bm->Lock();
  if (dc->clip_state)
  {
    dc->view->PopState();
    dc->clip_state = 0;
    haikuSetTransform(dc, &dc->matrix);
  }
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int* x1, int* y1, int* x2, int* y2)
{
  if (x1) *x1 = 0;
  if (y1) *y1 = 0;
  if (x2) *x2 = 0;
  if (y2) *y2 = 0;
  if (!dc) return;
  char* s = iupAttribGet(dc->ih, "_IUPHAIKU_CLIP");
  if (s) sscanf(s, "%d %d %d %d", x1, y1, x2, y2);
}

extern "C" IUP_SDK_API void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc || !dc->bm) return;
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  dc->bm->Lock();
  dc->view->SetDrawingMode(B_OP_INVERT);
  dc->view->FillRect(BRect(x1, y1, x2, y2), B_MIXED_COLORS);
  dc->view->SetDrawingMode(B_OP_ALPHA);
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc || !dc->bm) return;
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  dc->bm->Lock();
  IupDrawStroke stroke;
  haikuBeginStroke(dc, iupDrawColor(0, 0, 0, 255), IUP_DRAW_STROKE, 1, 1, &stroke);
  dc->view->StrokeRect(BRect(x1, y1, x2, y2), B_MIXED_COLORS);
  haikuEndStroke(dc, 1);
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API int iupdrvDrawGetImageData(IdrawCanvas* dc, unsigned char* data)
{
  if (!dc || !dc->bm || !data) return 0;
  dc->bm->Lock();
  dc->view->Sync();
  dc->bm->Unlock();
  iupdrvImageGetData(dc->bm, data);
  return 1;
}

extern "C" IUP_SDK_API int iupdrvCanvasGetImageData(Ihandle* ih, unsigned char* data, int /*w*/, int /*h*/)
{
  (void)ih; (void)data;
  return 0;
}
