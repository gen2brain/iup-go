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


/* offscreen render: BView attached to BBitmap via B_BITMAP_ACCEPTS_VIEWS; iupdrvDrawFlush blits onto the canvas */

struct _IdrawCanvas
{
  Ihandle* ih;
  BBitmap* bm;
  BView* view;
  int w;
  int h;
};

/* helpers */

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

/* Canvas lifecycle */

extern "C" IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = (IdrawCanvas*)calloc(1, sizeof(IdrawCanvas));
  dc->ih = ih;
  iupdrvDrawUpdateSize(dc);
  return dc;
}

extern "C" IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  if (!dc) return;
  if (dc->bm)
  {
    /* deleting the BBitmap takes its attached BView down with it */
    delete dc->bm;
  }
  free(dc);
}

extern "C" IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  if (!dc) return;
  int w = dc->ih->currentwidth  > 0 ? dc->ih->currentwidth  : 1;
  int h = dc->ih->currentheight > 0 ? dc->ih->currentheight : 1;
  if (iupAttribGet(dc->ih, "_IUPHAIKU_CANVAS_INNER"))
  {
    int iw = iupAttribGetInt(dc->ih, "_IUPHAIKU_CANVAS_INNER_W");
    int ih_ = iupAttribGetInt(dc->ih, "_IUPHAIKU_CANVAS_INNER_H");
    if (iw > 0) w = iw;
    if (ih_ > 0) h = ih_;
  }
  if (dc->bm && dc->w == w && dc->h == h) return;

  if (dc->bm) { delete dc->bm; dc->bm = NULL; dc->view = NULL; }
  dc->w = w; dc->h = h;

  dc->bm = new BBitmap(BRect(0, 0, w - 1, h - 1), B_BITMAP_ACCEPTS_VIEWS, B_RGBA32);
  dc->view = new BView(dc->bm->Bounds(), "iup_dc", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
  dc->bm->AddChild(dc->view);
  dc->bm->Lock();
  /* Seed buffer with current BGCOLOR so untouched pixels match the docs default. */
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

  BView* canvas = (BView*)iupAttribGet(dc->ih, "_IUPHAIKU_CANVAS_INNER");
  if (!canvas) canvas = (BView*)dc->ih->handle;
  if (!canvas) return;

  canvas->DrawBitmap(dc->bm, BPoint(0, 0));
}

extern "C" IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int *w, int *h)
{
  if (w) *w = dc ? dc->w : 0;
  if (h) *h = dc ? dc->h : 0;
}

/* primitives */

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
  /* Promote quadratic to cubic so we reuse BView's StrokeBezier. */
  int cx1 = x1 + (2 * (x2 - x1)) / 3;
  int cy1 = y1 + (2 * (y2 - y1)) / 3;
  int cx2 = x3 + (2 * (x2 - x3)) / 3;
  int cy2 = y3 + (2 * (y2 - y3)) / 3;
  iupdrvDrawBezier(dc, x1, y1, cx1, cy1, cx2, cy2, x3, y3, color, style, line_width);
}

extern "C" IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, long color1, long color2)
{
  if (!dc || !dc->bm) return;
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  BRect r(x1, y1, x2, y2);

  /* Compute gradient endpoints from angle relative to rect centre. */
  float cx = (x1 + x2) / 2.0f, cy = (y1 + y2) / 2.0f;
  float len = (r.Width() > r.Height() ? r.Width() : r.Height());
  float rad = angle * 3.14159265f / 180.0f;
  float dx = cosf(rad) * len * 0.5f;
  float dy = sinf(rad) * len * 0.5f;

  BGradientLinear grad(BPoint(cx - dx, cy - dy), BPoint(cx + dx, cy + dy));
  grad.AddColor(haikuColorFromLong(color1), 0.0f);
  grad.AddColor(haikuColorFromLong(color2), 255.0f);

  dc->bm->Lock();
  dc->view->FillRect(r, grad);
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, long colorCenter, long colorEdge)
{
  if (!dc || !dc->bm) return;
  BPoint c(cx, cy);
  BGradientRadial grad(c, (float)radius);
  grad.AddColor(haikuColorFromLong(colorCenter), 0.0f);
  grad.AddColor(haikuColorFromLong(colorEdge), 255.0f);

  BRect r(cx - radius, cy - radius, cx + radius, cy + radius);
  dc->bm->Lock();
  dc->view->FillEllipse(r, grad);
  dc->bm->Unlock();
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
    /* IUP angle is degrees CCW; BAffineTransform::RotateBy takes radians.
     * Rotate around (x, y) so text origin stays put. */
    BAffineTransform trans;
    trans.RotateBy(BPoint((float)x, (float)y), -text_orientation * 3.14159265358979323846 / 180.0);
    dc->view->SetTransform(trans);
  }

  /* Iterate lines (BView::DrawString is single-line). */
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

    int line_x = x;
    if ((flags & 0x000F) == IUP_DRAW_CENTER || (flags & 0x000F) == IUP_DRAW_RIGHT)
    {
      float lw = dc->view->StringWidth(line, line_len);
      if ((flags & 0x000F) == IUP_DRAW_CENTER) line_x = x + (w - (int)lw) / 2;
      else                                     line_x = x + (w - (int)lw);
    }

    dc->view->DrawString(line, line_len, BPoint((float)line_x, (float)line_y + fh.ascent));

    if (nl >= end) break;
    line = nl + 1;
    line_y += line_h;
  }

  if (text_orientation != 0.0)
    dc->view->SetTransform(BAffineTransform());  /* identity */

  if (flags & IUP_DRAW_CLIP)
    dc->view->ConstrainClippingRegion(NULL);
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, int x, int y, int w, int h)
{
  if (!dc || !dc->bm || !name) return;
  BBitmap* img = (BBitmap*)iupImageGetImage(name, dc->ih, make_inactive, bgcolor);
  if (!img) return;

  if (w <= 0) w = (int)(img->Bounds().Width() + 1);
  if (h <= 0) h = (int)(img->Bounds().Height() + 1);

  BRect dst(x, y, x + w - 1, y + h - 1);
  dc->bm->Lock();
  dc->view->SetDrawingMode(B_OP_ALPHA);
  dc->view->DrawBitmap(img, dst);
  dc->bm->Unlock();
}

/* clipping */

extern "C" IUP_SDK_API void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc || !dc->bm) return;
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  iupAttribSetStrf(dc->ih, "_IUPHAIKU_CLIP", "%d %d %d %d", x1, y1, x2, y2);

  BRegion region;
  region.Include(BRect(x1, y1, x2, y2));
  dc->bm->Lock();
  dc->view->ConstrainClippingRegion(&region);
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int /*corner_radius*/)
{
  /* BRegion is rectangular only; degrade to plain rect clip. */
  iupdrvDrawSetClipRect(dc, x1, y1, x2, y2);
}

extern "C" IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  if (!dc || !dc->bm) return;
  iupAttribSet(dc->ih, "_IUPHAIKU_CLIP", NULL);
  dc->bm->Lock();
  dc->view->ConstrainClippingRegion(NULL);
  dc->bm->Unlock();
}

extern "C" IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int *x1, int *y1, int *x2, int *y2)
{
  if (x1) *x1 = 0;
  if (y1) *y1 = 0;
  if (x2) *x2 = dc ? dc->w - 1 : 0;
  if (y2) *y2 = dc ? dc->h - 1 : 0;
  if (!dc) return;
  char* s = iupAttribGet(dc->ih, "_IUPHAIKU_CLIP");
  if (s) sscanf(s, "%d %d %d %d", x1, y1, x2, y2);
}

extern "C" IUP_SDK_API void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  /* Selection rectangle: 50/50 stipple in inverted colors. */
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
  /* Dotted focus outline. */
  if (!dc || !dc->bm) return;
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);
  dc->bm->Lock();
  dc->view->SetHighColor(0, 0, 0, 255);
  dc->view->StrokeRect(BRect(x1, y1, x2, y2), B_MIXED_COLORS);
  dc->bm->Unlock();
}

/* image-data export */

extern "C" IUP_SDK_API int iupdrvDrawGetImageData(IdrawCanvas* dc, unsigned char* data)
{
  if (!dc || !dc->bm || !data) return 0;
  dc->bm->Lock();
  dc->view->Sync();
  dc->bm->Unlock();
  iupdrvImageGetData(dc->bm, data);  /* reuses image.cpp's BGRA->RGBA path */
  return 1;
}

extern "C" IUP_SDK_API int iupdrvCanvasGetImageData(Ihandle* ih, unsigned char* data, int /*w*/, int /*h*/)
{
  /* Persistent buffer reads aren't tracked yet. */
  (void)ih; (void)data;
  return 0;
}
