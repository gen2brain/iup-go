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

static void haikuApplyStroke(BView* v, long color, int line_width, int /*style*/)
{
  v->SetHighColor(haikuColorFromLong(color));
  v->SetPenSize(line_width > 0 ? (float)line_width : 1.0f);
}

extern "C" IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = (IdrawCanvas*)calloc(1, sizeof(IdrawCanvas));
  dc->ih = ih;
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

extern "C" IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc || !dc->bm) return;
  dc->bm->Lock();
  haikuApplyStroke(dc->view, color, line_width, style);
  dc->view->StrokeLine(BPoint(x1, y1), BPoint(x2, y2));
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
    haikuApplyStroke(dc->view, color, line_width, style);
    dc->view->StrokeRect(r);
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
    haikuApplyStroke(dc->view, color, line_width, style);
    dc->view->StrokeRoundRect(r, rad, rad);
  }
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  if (!dc || !dc->bm) return;
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  BRect r(x1, y1, x2, y2);
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
    haikuApplyStroke(dc->view, color, line_width, style);
    dc->view->StrokeArc(r, start, sweep);
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
    haikuApplyStroke(dc->view, color, line_width, style);
    dc->view->StrokeEllipse(r);
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
    haikuApplyStroke(dc->view, color, line_width, style);
    dc->view->StrokePolygon(pts, count, true);
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
    haikuApplyStroke(dc->view, color, line_width, style);
    dc->view->StrokeBezier(cp);
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
      cur_x = sub_x = (float)segs[i].x1;
      cur_y = sub_y = (float)segs[i].y1;
      break;
    case IUP_PATHSEG_LINE_TO:
      shape->LineTo(BPoint((float)segs[i].x1, (float)segs[i].y1));
      cur_x = (float)segs[i].x1;
      cur_y = (float)segs[i].y1;
      break;
    case IUP_PATHSEG_CURVE_TO:
      shape->BezierTo(BPoint((float)segs[i].x1, (float)segs[i].y1),
                      BPoint((float)segs[i].x2, (float)segs[i].y2),
                      BPoint((float)segs[i].x3, (float)segs[i].y3));
      cur_x = (float)segs[i].x3;
      cur_y = (float)segs[i].y3;
      break;
    case IUP_PATHSEG_QUAD_TO:
    {
      float c1x = cur_x + 2.0f / 3.0f * ((float)segs[i].x1 - cur_x);
      float c1y = cur_y + 2.0f / 3.0f * ((float)segs[i].y1 - cur_y);
      float c2x = (float)segs[i].x2 + 2.0f / 3.0f * ((float)segs[i].x1 - (float)segs[i].x2);
      float c2y = (float)segs[i].y2 + 2.0f / 3.0f * ((float)segs[i].y1 - (float)segs[i].y2);
      shape->BezierTo(BPoint(c1x, c1y), BPoint(c2x, c2y), BPoint((float)segs[i].x2, (float)segs[i].y2));
      cur_x = (float)segs[i].x2;
      cur_y = (float)segs[i].y2;
      break;
    }
    case IUP_PATHSEG_ARC_TO:
    {
      IupPathSeg bez[4];
      int j, n = iupDrawPathArcToBeziers(&segs[i], bez);
      for (j = 0; j < n; j++)
        shape->BezierTo(BPoint((float)bez[j].x1, (float)bez[j].y1),
                        BPoint((float)bez[j].x2, (float)bez[j].y2),
                        BPoint((float)bez[j].x3, (float)bez[j].y3));
      if (n > 0)
      {
        cur_x = (float)bez[n - 1].x3;
        cur_y = (float)bez[n - 1].y3;
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

typedef struct _HaikuDash
{
  const float* pattern;
  int count;
  int index;
  float remain;
} HaikuDash;

static int haikuDashInit(HaikuDash* dash, int style)
{
  static const float dash_pattern[] = {9.0f, 3.0f};
  static const float dot_pattern[] = {1.0f, 2.0f};
  static const float dash_dot_pattern[] = {7.0f, 3.0f, 1.0f, 3.0f};
  static const float dash_dot_dot_pattern[] = {7.0f, 3.0f, 1.0f, 3.0f, 1.0f, 3.0f};

  switch (style)
  {
  case IUP_DRAW_STROKE_DASH:
    dash->pattern = dash_pattern;
    dash->count = 2;
    break;
  case IUP_DRAW_STROKE_DOT:
    dash->pattern = dot_pattern;
    dash->count = 2;
    break;
  case IUP_DRAW_STROKE_DASH_DOT:
    dash->pattern = dash_dot_pattern;
    dash->count = 4;
    break;
  case IUP_DRAW_STROKE_DASH_DOT_DOT:
    dash->pattern = dash_dot_dot_pattern;
    dash->count = 6;
    break;
  default:
    return 0;
  }

  dash->index = 0;
  dash->remain = dash->pattern[0];
  return 1;
}

static void haikuStrokeDashSegment(BView* view, HaikuDash* dash, float x1, float y1, float x2, float y2)
{
  float dx = x2 - x1;
  float dy = y2 - y1;
  float length = sqrtf(dx * dx + dy * dy);
  float offset = 0.0f;

  if (length <= 0.0f)
    return;

  while (offset < length)
  {
    float step = dash->remain < length - offset ? dash->remain : length - offset;
    float t1 = offset / length;
    float t2 = (offset + step) / length;
    if ((dash->index & 1) == 0)
      view->StrokeLine(BPoint(x1 + dx * t1, y1 + dy * t1), BPoint(x1 + dx * t2, y1 + dy * t2));
    offset += step;
    dash->remain -= step;
    if (dash->remain <= 0.0001f)
    {
      dash->index = (dash->index + 1) % dash->count;
      dash->remain = dash->pattern[dash->index];
    }
  }
}

static void haikuStrokeDashCubic(BView* view, HaikuDash* dash, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3)
{
  float px = x0, py = y0;
  int i;

  for (i = 1; i <= 20; i++)
  {
    float t = (float)i / 20.0f;
    float mt = 1.0f - t;
    float x = mt * mt * mt * x0 + 3.0f * mt * mt * t * x1 + 3.0f * mt * t * t * x2 + t * t * t * x3;
    float y = mt * mt * mt * y0 + 3.0f * mt * mt * t * y1 + 3.0f * mt * t * t * y2 + t * t * t * y3;
    haikuStrokeDashSegment(view, dash, px, py, x, y);
    px = x;
    py = y;
  }
}

static void haikuStrokeDashedPath(BView* view, const IupPathSeg* segs, int count, int style)
{
  HaikuDash dash;
  float cur_x = 0.0f, cur_y = 0.0f, sub_x = 0.0f, sub_y = 0.0f;
  int has_current = 0;
  int i;

  if (!haikuDashInit(&dash, style))
    return;

  for (i = 0; i < count; i++)
  {
    switch (segs[i].op)
    {
    case IUP_PATHSEG_MOVE_TO:
      cur_x = sub_x = (float)segs[i].x1;
      cur_y = sub_y = (float)segs[i].y1;
      has_current = 1;
      break;
    case IUP_PATHSEG_LINE_TO:
      if (has_current)
        haikuStrokeDashSegment(view, &dash, cur_x, cur_y, (float)segs[i].x1, (float)segs[i].y1);
      cur_x = (float)segs[i].x1;
      cur_y = (float)segs[i].y1;
      has_current = 1;
      break;
    case IUP_PATHSEG_CURVE_TO:
      if (has_current)
        haikuStrokeDashCubic(view, &dash, cur_x, cur_y, (float)segs[i].x1, (float)segs[i].y1, (float)segs[i].x2, (float)segs[i].y2, (float)segs[i].x3, (float)segs[i].y3);
      cur_x = (float)segs[i].x3;
      cur_y = (float)segs[i].y3;
      has_current = 1;
      break;
    case IUP_PATHSEG_QUAD_TO:
      if (has_current)
      {
        float c1x = cur_x + 2.0f / 3.0f * ((float)segs[i].x1 - cur_x);
        float c1y = cur_y + 2.0f / 3.0f * ((float)segs[i].y1 - cur_y);
        float c2x = (float)segs[i].x2 + 2.0f / 3.0f * ((float)segs[i].x1 - (float)segs[i].x2);
        float c2y = (float)segs[i].y2 + 2.0f / 3.0f * ((float)segs[i].y1 - (float)segs[i].y2);
        haikuStrokeDashCubic(view, &dash, cur_x, cur_y, c1x, c1y, c2x, c2y, (float)segs[i].x2, (float)segs[i].y2);
      }
      cur_x = (float)segs[i].x2;
      cur_y = (float)segs[i].y2;
      has_current = 1;
      break;
    case IUP_PATHSEG_ARC_TO:
    {
      IupPathSeg bez[4];
      int j, n = iupDrawPathArcToBeziers(&segs[i], bez);
      for (j = 0; j < n; j++)
        haikuStrokeDashCubic(view, &dash, cur_x, cur_y, (float)bez[j].x1, (float)bez[j].y1, (float)bez[j].x2, (float)bez[j].y2, (float)bez[j].x3, (float)bez[j].y3);
      if (n > 0)
      {
        cur_x = (float)bez[n - 1].x3;
        cur_y = (float)bez[n - 1].y3;
      }
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
  dc->bm->Unlock();

  delete shape;
}

extern "C" IUP_SDK_API void iupdrvDrawPathStroke(IdrawCanvas* dc, const IupPathSeg* segs, int count, const IupDrawSource* src, int style, int line_width)
{
  if (!dc || !dc->bm) return;
  BShape* shape = haikuBuildShape(segs, count);
  if (!shape) return;

  dc->bm->Lock();
  dc->view->MovePenTo(0, 0);
  dc->view->SetPenSize(line_width > 0 ? (float)line_width : 1.0f);
  if (src->type == IUP_SOURCE_SOLID && style >= IUP_DRAW_STROKE_DASH && style <= IUP_DRAW_STROKE_DASH_DOT_DOT)
  {
    dc->view->SetHighColor(haikuColorFromLong(src->color));
    haikuStrokeDashedPath(dc->view, segs, count, style);
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
  if (dc->clip_state)
    dc->view->PopState();
  dc->view->PushState();
  dc->clip_state = 1;
  dc->view->SetFillRule(rule == IUP_PATH_RULE_EVENODD ? B_EVEN_ODD : B_NONZERO);
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
  if (bfont) dc->view->SetFont(bfont);
  dc->view->SetHighColor(haikuColorFromLong(color));

  font_height fh;
  dc->view->GetFontHeight(&fh);

  if (flags & IUP_DRAW_CLIP)
  {
    BRegion clip;
    clip.Include(BRect(x, y, x + w - 1, y + h - 1));
    dc->view->ConstrainClippingRegion(&clip);
  }

  if (text_orientation != 0.0)
  {
    BAffineTransform trans;
    trans.RotateBy(BPoint((float)x, (float)y), -text_orientation * 3.14159265358979323846 / 180.0);
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

  if (text_orientation != 0.0)
    dc->view->SetTransform(BAffineTransform());

  if (flags & IUP_DRAW_CLIP)
    dc->view->ConstrainClippingRegion(NULL);
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
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  iupAttribSetStrf(dc->ih, "_IUPHAIKU_CLIP", "%d %d %d %d", x1, y1, x2, y2);

  BRegion region;
  region.Include(BRect(x1, y1, x2, y2));
  dc->bm->Lock();
  if (dc->clip_state)
    dc->view->PopState();
  dc->view->PushState();
  dc->clip_state = 1;
  dc->view->ConstrainClippingRegion(&region);
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int /*corner_radius*/)
{
  iupdrvDrawSetClipRect(dc, x1, y1, x2, y2);
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
  dc->view->SetHighColor(0, 0, 0, 255);
  dc->view->StrokeRect(BRect(x1, y1, x2, y2), B_MIXED_COLORS);
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
