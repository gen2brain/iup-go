/** \file
 * \brief Draw Functions for GDI
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "iup.h"

#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_image.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"

#include "iupwin_drv.h"
#include "iupwin_info.h"
#include "iupwin_draw.h"
#include "iupwin_str.h"

#include "backend-gdix.h"
#define iupColor2ARGB(_c) WD_ARGB(iupDrawAlpha(_c), iupDrawRed(_c), iupDrawGreen(_c), iupDrawBlue(_c))

/* From iupwin_draw_wdl.c */
int iupwinDrawInitWDL(void);
int iupwinDrawFinishWDL(void);
IdrawCanvas* iupdrvDrawCreateCanvasWDL(Ihandle* ih);
void iupdrvDrawKillCanvasWDL(IdrawCanvas* dc);
void iupdrvDrawFlushWDL(IdrawCanvas* dc);
void iupdrvDrawUpdateSizeWDL(IdrawCanvas* dc);
void iupdrvDrawGetSizeWDL(IdrawCanvas* dc, int *w, int *h);
void iupdrvDrawLineWDL(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width);
void iupdrvDrawRectangleWDL(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width);
void iupdrvDrawArcWDL(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width);
void iupdrvDrawPolygonWDL(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width);
void iupdrvDrawTextWDL(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation);
void iupdrvDrawImageWDL(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, int x, int y, int w, int h);
void iupdrvDrawSetClipRectWDL(IdrawCanvas* dc, int x1, int y1, int x2, int y2);
void iupdrvDrawResetClipWDL(IdrawCanvas* dc);
void iupdrvDrawGetClipRectWDL(IdrawCanvas* dc, int *x1, int *y1, int *x2, int *y2);
void iupdrvDrawSelectRectWDL(IdrawCanvas* dc, int x1, int y1, int x2, int y2);
void iupdrvDrawFocusRectWDL(IdrawCanvas* dc, int x1, int y1, int x2, int y2);

struct _IdrawCanvas{
  Ihandle* ih;
  int w, h;

  HWND hWnd;
  int release_dc;
  HBITMAP hBitmap, hOldBitmap;
  HDC hBitmapDC, hDC;

  int clip_x1, clip_y1, clip_x2, clip_y2;

  int sim_aa;

  IdrawCanvas* wdl_gc;
};

void iupwinDrawInit(void)
{
  iupwinDrawThemeInit();

  if (iupwinDrawInitWDL())  /* if WDL is using Direct2D must manually initialize GDI+ */
    gdix_init();
}

void iupwinDrawFinish(void)
{
  if (iupwinDrawFinishWDL())  /* if WDL is using Direct2D must manually terminates GDI+ */
    gdix_fini();
}

IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = calloc(1, sizeof(IdrawCanvas));
  RECT rect;

  dc->ih = ih;

  if (iupAttribGetBoolean(ih, "DRAWUSEDIRECT2D") || IupGetInt(NULL, "DRAWUSEDIRECT2D"))
  {
    dc->wdl_gc = iupdrvDrawCreateCanvasWDL(ih);
    return dc;
  }

  dc->hWnd = (HWND)IupGetAttribute(ih, "HWND");  /* Use the attribute, so it can work with FileDlg preview area */

  /* valid only inside the ACTION callback of an IupCanvas */
  dc->hDC = (HDC)IupGetAttribute(ih, "HDC_WMPAINT");
  if (!dc->hDC)
  {
    dc->hDC = GetDC(dc->hWnd);
    dc->release_dc = 1;
  }

  GetClientRect(dc->hWnd, &rect);
  dc->w = rect.right - rect.left;
  dc->h = rect.bottom - rect.top;

  dc->hBitmap = CreateCompatibleBitmap(dc->hDC, dc->w, dc->h);
  dc->hBitmapDC = CreateCompatibleDC(dc->hDC);
  dc->hOldBitmap = SelectObject(dc->hBitmapDC, dc->hBitmap);

  SetBkMode(dc->hBitmapDC, TRANSPARENT);
  SetTextAlign(dc->hBitmapDC, TA_TOP|TA_LEFT);

  iupAttribSet(ih, "DRAWDRIVER", "GDI");
  dc->sim_aa = iupAttribGetInt(ih, "DRAWANTIALIAS");

  return dc;
}

IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  if (dc->wdl_gc)
  {
    iupdrvDrawKillCanvasWDL(dc->wdl_gc);

    memset(dc, 0, sizeof(IdrawCanvas));
    free(dc);
    return;
  }

  SelectObject(dc->hBitmapDC, dc->hOldBitmap);
  DeleteObject(dc->hBitmap);
  DeleteDC(dc->hBitmapDC);  /* to match CreateCompatibleDC */
  if (dc->release_dc)
    ReleaseDC(dc->hWnd, dc->hDC);  /* to match GetDC */

  free(dc);
}

IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  int w, h;
  RECT rect;

  if (dc->wdl_gc)
  {
    iupdrvDrawUpdateSizeWDL(dc->wdl_gc);
    return;
  }

  GetClientRect(dc->hWnd, &rect);
  w = rect.right - rect.left;
  h = rect.bottom - rect.top;

  if (w != dc->w || h != dc->h)
  {
    dc->w = w;
    dc->h = h;

    SelectObject(dc->hBitmapDC, dc->hOldBitmap);
    DeleteObject(dc->hBitmap);
    DeleteDC(dc->hBitmapDC);  /* to match CreateCompatibleDC */

    dc->hBitmap = CreateCompatibleBitmap(dc->hDC, dc->w, dc->h);
    dc->hBitmapDC = CreateCompatibleDC(dc->hDC);
    dc->hOldBitmap = SelectObject(dc->hBitmapDC, dc->hBitmap);

    SetBkMode(dc->hBitmapDC, TRANSPARENT);
    SetTextAlign(dc->hBitmapDC, TA_TOP|TA_LEFT);
  }
}

IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc)
{
  if (dc->wdl_gc)
  {
    iupdrvDrawFlushWDL(dc->wdl_gc);
    return;
  }

  BitBlt(dc->hDC, 0, 0, dc->w, dc->h, dc->hBitmapDC, 0, 0, SRCCOPY);
}

IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int *w, int *h)
{
  if (dc->wdl_gc)
  {
    iupdrvDrawGetSizeWDL(dc->wdl_gc, w, h);
    return;
  }

  if (w) *w = dc->w;
  if (h) *h = dc->h;
}

static HPEN iDrawCreatePen(long color, int style, int line_width)
{
  LOGBRUSH LogBrush;
  LogBrush.lbStyle = BS_SOLID;
  LogBrush.lbColor = RGB(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color));
  LogBrush.lbHatch = 0;

  if (style == IUP_DRAW_STROKE || style == IUP_DRAW_FILL)
    return ExtCreatePen(PS_GEOMETRIC | PS_SOLID, line_width, &LogBrush, 0, NULL);
  else if (style == IUP_DRAW_STROKE_DASH)
  {
    /* for some reason, dashes and spaces in GDI must be changed -1 and +1 to be exactly what specified */
    DWORD dashes[2] = { 9-1, 3+1 };
    return ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, line_width, &LogBrush, 2, dashes);
  }
  else if (style == IUP_DRAW_STROKE_DOT)
  {
    DWORD dashes[2] = { 1-1, 2+1 };
    return ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, line_width, &LogBrush, 2, dashes);
  }
  else if (style == IUP_DRAW_STROKE_DASH_DOT)
  {
    DWORD dashes[4] = { 7-1, 3+1, 1-1, 3+1 };
    return ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, line_width, &LogBrush, 4, dashes);
  }
  else if (style == IUP_DRAW_STROKE_DASH_DOT_DOT)
  {
    DWORD dashes[6] = { 7-1, 3+1, 1-1, 3+1, 1-1, 3+1 };
    return ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, line_width, &LogBrush, 6, dashes);
  }
  return NULL;
}

static void iwinDrawSetLineStyleSimAA(int style, dummy_GpPen* pen)
{
  if (style != IUP_DRAW_STROKE)
  {
    float* dashes = NULL;
    int dashes_count = 0;

    if (style == IUP_DRAW_STROKE_DASH)
    {
      static float s_dashes[2] = { 9.0f, 3.0f };
      dashes = s_dashes;
      dashes_count = 2;
    }
    else if (style == IUP_DRAW_STROKE_DOT)
    {
      static float s_dashes[2] = { 1.0f, 2.0f };
      dashes = s_dashes;
      dashes_count = 2;
    }
    else if (style == IUP_DRAW_STROKE_DASH_DOT)
    {
      static float s_dashes[4] = { 7.0f, 3.0f, 1.0f, 3.0f };
      dashes = s_dashes;
      dashes_count = 4;
    }
    else if (style == IUP_DRAW_STROKE_DASH_DOT_DOT)
    {
      static float s_dashes[6] = { 7.0f, 3.0f, 1.0f, 3.0f, 1.0f, 3.0f };
      dashes = s_dashes;
      dashes_count = 6;
    }
    else
      return;

    gdix_vtable->fn_SetPenDashOffset(pen, 0.5f);
    gdix_vtable->fn_SetPenDashArray(pen, dashes, dashes_count);
    gdix_vtable->fn_SetPenDashStyle(pen, dummy_DashStyleCustom);
  }
}

static int colorHasAlpha(long color)
{
  return iupDrawAlpha(color) != 255;
}

static void iwinDrawRectangleSimAA(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  dummy_GpGraphics* graphics;

  gdix_vtable->fn_CreateFromHDC(dc->hBitmapDC, &graphics);
  gdix_vtable->fn_SetSmoothingMode(graphics, dummy_SmoothingModeAntiAlias);

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  if (style == IUP_DRAW_FILL)
  {
    dummy_GpBrush* brush;
    gdix_vtable->fn_CreateSolidFill(iupColor2ARGB(color), &brush);

    gdix_vtable->fn_FillRectangle(graphics, brush, (float)(x1 - 0.5f), (float)(y1 - 0.5f), (float)(x2 - x1 + 1), (float)(y2 - y1 + 1));   /* in this case Size = Max - Min  */

    gdix_vtable->fn_DeleteBrush(brush);
  }
  else
  {
    dummy_GpPen* pen;
    gdix_vtable->fn_CreatePen1(iupColor2ARGB(color), (float)line_width, dummy_UnitPixel, &pen);
    iwinDrawSetLineStyleSimAA(style, pen);

    gdix_vtable->fn_DrawRectangle(graphics, pen, (float)x1, (float)y1, (float)(x2 - x1), (float)(y2 - y1));   /* in this case Size = Max - Min */

    gdix_vtable->fn_DeletePen(pen);
  }

  gdix_vtable->fn_DeleteGraphics(graphics);
}

IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (dc->wdl_gc)
  {
    iupdrvDrawRectangleWDL(dc->wdl_gc, x1, y1, x2, y2, color, style, line_width);
    return;
  }

  if (dc->sim_aa && colorHasAlpha(color))
    iwinDrawRectangleSimAA(dc, x1, y1, x2, y2, color, style, line_width);
  else
  {
    SetDCBrushColor(dc->hBitmapDC, RGB(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color)));

    if (style == IUP_DRAW_FILL)
    {
      RECT rect;
      iupDrawCheckSwapCoord(x1, x2);
      iupDrawCheckSwapCoord(y1, y2);
      SetRect(&rect, x1, y1, x2 + 1, y2 + 1);
      FillRect(dc->hBitmapDC, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
    }
    else if (style == IUP_DRAW_STROKE && line_width == 1)
    {
      RECT rect;
      iupDrawCheckSwapCoord(x1, x2);
      iupDrawCheckSwapCoord(y1, y2);
      SetRect(&rect, x1, y1, x2 + 1, y2 + 1);
      FrameRect(dc->hBitmapDC, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
    }
    else
    {
      POINT line_poly[5];
      HPEN hPen = iDrawCreatePen(color, style, line_width);
      HPEN hPenOld = SelectObject(dc->hBitmapDC, hPen);
      line_poly[0].x = x1;
      line_poly[0].y = y1;
      line_poly[1].x = x1;
      line_poly[1].y = y2;
      line_poly[2].x = x2;
      line_poly[2].y = y2;
      line_poly[3].x = x2;
      line_poly[3].y = y1;
      line_poly[4].x = x1;
      line_poly[4].y = y1;
      Polyline(dc->hBitmapDC, line_poly, 5);
      SelectObject(dc->hBitmapDC, hPenOld);
      DeleteObject(hPen);
    }
  }
}

static void iwinDrawLineSimAA(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  dummy_GpGraphics* graphics;
  dummy_GpPen* pen;

  gdix_vtable->fn_CreateFromHDC(dc->hBitmapDC, &graphics);
  gdix_vtable->fn_SetSmoothingMode(graphics, dummy_SmoothingModeAntiAlias);

  gdix_vtable->fn_CreatePen1(iupColor2ARGB(color), (float)line_width, dummy_UnitPixel, &pen);
  iwinDrawSetLineStyleSimAA(style, pen);

  gdix_vtable->fn_DrawLine(graphics, pen, (float)x1, (float)y1, (float)x2, (float)y2);

  gdix_vtable->fn_DeletePen(pen);

  gdix_vtable->fn_DeleteGraphics(graphics);
}

IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (dc->wdl_gc)
  {
    iupdrvDrawLineWDL(dc->wdl_gc, x1, y1, x2, y2, color, style, line_width);
    return;
  }

  if (dc->sim_aa && ((x1 != x2 && y1 != y2) || colorHasAlpha(color)))
    iwinDrawLineSimAA(dc, x1, y1, x2, y2, color, style, line_width);
  else
  {
    POINT line_poly[2];
    HPEN hPen = iDrawCreatePen(color, style, line_width);
    HPEN hPenOld = SelectObject(dc->hBitmapDC, hPen);

    line_poly[0].x = x1;
    line_poly[0].y = y1;
    line_poly[1].x = x2;
    line_poly[1].y = y2;
    Polyline(dc->hBitmapDC, line_poly, 2);
    SetPixelV(dc->hBitmapDC, x2, y2, RGB(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color)));

    SelectObject(dc->hBitmapDC, hPenOld);
    DeleteObject(hPen);
  }
}

static int winDrawCalcArc(int c1, int c2, double a, int horiz)
{
  double proj;
  int pos;
  if (horiz)
    proj = cos(IUP_DEG2RAD * a);
  else
    proj = - sin(IUP_DEG2RAD * a);
  pos = (c2 + c1) / 2 + iupROUND((c2 - c1 + 1) * proj / 2.0);
  return pos;
}

static void iwinDrawArcSimAA(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  float xc, yc, rx, ry, dx, dy;
  float baseAngle, sweepAngle;
  dummy_GpGraphics* graphics;

  gdix_vtable->fn_CreateFromHDC(dc->hBitmapDC, &graphics);
  gdix_vtable->fn_SetSmoothingMode(graphics, dummy_SmoothingModeAntiAlias);

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  dx = (float)(x2 - x1);
  dy = (float)(y2 - y1);

  rx = dx / 2.0f;
  ry = dy / 2.0f;

  xc = (float)x1 + rx;
  yc = (float)y1 + ry;

  baseAngle = (float)(360.0 - a2);
  sweepAngle = (float)(a2 - a1);

  if (style == IUP_DRAW_FILL)
  {
    dummy_GpBrush* brush;
    gdix_vtable->fn_CreateSolidFill(iupColor2ARGB(color), &brush);

    if (sweepAngle == 360.0f)
      gdix_vtable->fn_FillEllipse(graphics, brush, xc - rx, yc - ry, dx, dy);
    else
      gdix_vtable->fn_FillPie(graphics, brush, xc - rx, yc - ry, dx, dy, baseAngle, sweepAngle);

    gdix_vtable->fn_DeleteBrush(brush);
  }
  else
  {
    dummy_GpPen* pen;
    gdix_vtable->fn_CreatePen1(iupColor2ARGB(color), (float)line_width, dummy_UnitPixel, &pen);
    iwinDrawSetLineStyleSimAA(style, pen);

    if (sweepAngle == 360.0f)
      gdix_vtable->fn_DrawEllipse(graphics, pen, xc - rx, yc - ry, dx, dy);
    else
      gdix_vtable->fn_DrawArc(graphics, pen, xc - rx, yc - ry, dx, dy, baseAngle, sweepAngle);

    gdix_vtable->fn_DeletePen(pen);
  }

  gdix_vtable->fn_DeleteGraphics(graphics);
}

IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  int XStartArc, XEndArc, YStartArc, YEndArc;

  if (dc->wdl_gc)
  {
    iupdrvDrawArcWDL(dc->wdl_gc, x1, y1, x2, y2, a1, a2, color, style, line_width);
    return;
  }

  if (dc->sim_aa)
  {
    iwinDrawArcSimAA(dc, x1, y1, x2, y2, a1, a2, color, style, line_width);
    return;
  }

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  XStartArc = winDrawCalcArc(x1, x2, a1, 1);
  XEndArc = winDrawCalcArc(x1, x2, a2, 1);
  YStartArc = winDrawCalcArc(y1, y2, a1, 0);
  YEndArc = winDrawCalcArc(y1, y2, a2, 0);

  if (style==IUP_DRAW_FILL)
  {
    HBRUSH hBrush = CreateSolidBrush(RGB(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));
    HBRUSH hBrushOld = SelectObject(dc->hBitmapDC, hBrush);
    BeginPath(dc->hBitmapDC); 
    Pie(dc->hBitmapDC, x1, y1, x2 + 1, y2 + 1, XStartArc, YStartArc, XEndArc, YEndArc);
    EndPath(dc->hBitmapDC);
    FillPath(dc->hBitmapDC);
    SelectObject(dc->hBitmapDC, hBrushOld);
    DeleteObject(hBrush);
  }
  else
  {
    HPEN hPen = iDrawCreatePen(color, style, line_width);
    HPEN hPenOld = SelectObject(dc->hBitmapDC, hPen);
    Arc(dc->hBitmapDC, x1, y1, x2 + 1, y2 + 1, XStartArc, YStartArc, XEndArc, YEndArc);
    SelectObject(dc->hBitmapDC, hPenOld);
    DeleteObject(hPen);
  }
}

static void iwinDrawPolygonSimAA(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  int i;
  dummy_GpGraphics* graphics;
  dummy_GpPath* path;

  gdix_vtable->fn_CreateFromHDC(dc->hBitmapDC, &graphics);
  gdix_vtable->fn_CreatePath(dummy_FillModeAlternate, &path);
  gdix_vtable->fn_SetSmoothingMode(graphics, dummy_SmoothingModeAntiAlias);

  for (i = 0; i < count - 1; i++)
  {
    int j1 = 2 * i;
    int j2 = 2 * (i + 1);
    gdix_vtable->fn_AddPathLine(path, (float)points[j1], (float)points[j1 + 1], (float)points[j2], (float)points[j2 + 1]);
  }

  if (style == IUP_DRAW_FILL)
  {
    dummy_GpBrush* brush;
    gdix_vtable->fn_CreateSolidFill(iupColor2ARGB(color), &brush);

    gdix_vtable->fn_FillPath(graphics, brush, path);

    gdix_vtable->fn_DeleteBrush(brush);
  }
  else
  {
    dummy_GpPen* pen;
    gdix_vtable->fn_CreatePen1(iupColor2ARGB(color), (float)line_width, dummy_UnitPixel, &pen);
    iwinDrawSetLineStyleSimAA(style, pen);

    gdix_vtable->fn_DrawPath(graphics, pen, path);

    gdix_vtable->fn_DeletePen(pen);
  }

  gdix_vtable->fn_DeletePath(path);
  gdix_vtable->fn_DeleteGraphics(graphics);
}

IUP_SDK_API void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  if (dc->wdl_gc)
  {
    iupdrvDrawPolygonWDL(dc->wdl_gc, points, count, color, style, line_width);
    return;
  }

  if (dc->sim_aa)
  {
    iwinDrawPolygonSimAA(dc, points, count, color, style, line_width);
    return;
  }

  if (style == IUP_DRAW_FILL)
  {
    HBRUSH hBrush = CreateSolidBrush(RGB(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));
    HBRUSH hBrushOld = SelectObject(dc->hBitmapDC, hBrush);
    BeginPath(dc->hBitmapDC); 
    Polygon(dc->hBitmapDC, (POINT*)points, count);
    EndPath(dc->hBitmapDC);
    FillPath(dc->hBitmapDC);
    SelectObject(dc->hBitmapDC, hBrushOld);
    DeleteObject(hBrush);
  }
  else
  {
    HPEN hPen = iDrawCreatePen(color, style, line_width);
    HPEN hPenOld = SelectObject(dc->hBitmapDC, hPen);
    Polyline(dc->hBitmapDC, (POINT*)points, count);
    SelectObject(dc->hBitmapDC, hPenOld);
    DeleteObject(hPen);
  }
}

IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int *x1, int *y1, int *x2, int *y2)
{
  if (dc->wdl_gc)
  {
    iupdrvDrawGetClipRectWDL(dc->wdl_gc, x1, y1, x2, y2);
    return;
  }

  if (x1) *x1 = dc->clip_x1;
  if (y1) *y1 = dc->clip_y1;
  if (x2) *x2 = dc->clip_x2;
  if (y2) *y2 = dc->clip_y2;
}

IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  if (dc->wdl_gc)
  {
    iupdrvDrawResetClipWDL(dc->wdl_gc);
    return;
  }

  SelectClipRgn(dc->hBitmapDC, NULL);

  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = 0;
  dc->clip_y2 = 0;
}

IUP_SDK_API void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  HRGN clip_hrgn;

  if (dc->wdl_gc)
  {
    iupdrvDrawSetClipRectWDL(dc->wdl_gc, x1, y1, x2, y2);
    return;
  }

  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }

  /* make it an empty region */
  if (x1 >= x2) x1 = x2;
  if (y1 >= y2) y1 = y2;

  clip_hrgn = CreateRectRgn(x1, y1, x2 + 1, y2 + 1);
  SelectClipRgn(dc->hBitmapDC, clip_hrgn);
  DeleteObject(clip_hrgn);

  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
}

static void gdiRotateWorld(HDC hDC, int xc, int yc, double angle)
{
  XFORM xForm;

  SetGraphicsMode(hDC, GM_ADVANCED);
  ModifyWorldTransform(hDC, NULL, MWT_IDENTITY);

  xForm.eM11 = (FLOAT)cos(angle);
  xForm.eM12 = (FLOAT)sin(angle);
  xForm.eM21 = (FLOAT)-xForm.eM12;
  xForm.eM22 = (FLOAT)xForm.eM11;
  xForm.eDx = (FLOAT)xc;
  xForm.eDy = (FLOAT)yc;
  ModifyWorldTransform(hDC, &xForm, MWT_LEFTMULTIPLY);

  xForm.eM11 = (FLOAT)1;
  xForm.eM12 = (FLOAT)0;
  xForm.eM21 = (FLOAT)0;
  xForm.eM22 = (FLOAT)1;
  xForm.eDx = (FLOAT)-xc;
  xForm.eDy = (FLOAT)-yc;
  ModifyWorldTransform(hDC, &xForm, MWT_LEFTMULTIPLY);
}

static void gdiTranslateWorld(HDC hDC, int x, int y)
{
  XFORM xForm;
  xForm.eM11 = (FLOAT)1;
  xForm.eM12 = (FLOAT)0;
  xForm.eM21 = (FLOAT)0;
  xForm.eM22 = (FLOAT)1;
  xForm.eDx = (FLOAT)x;
  xForm.eDy = (FLOAT)y;
  ModifyWorldTransform(hDC, &xForm, MWT_RIGHTMULTIPLY);
}

static void gdiResetWorld(HDC hDC)
{
  ModifyWorldTransform(hDC, NULL, MWT_IDENTITY);
  SetGraphicsMode(hDC, GM_COMPATIBLE);
}

IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  if (dc->wdl_gc)
    iupdrvDrawTextWDL(dc->wdl_gc, text, len, x, y, w, h, color, font, flags, text_orientation);
  else
  {
    RECT rect;
    TCHAR* wtext;
    UINT uFormat = 0;
    int layout_w = w, layout_h = h;
    int layout_center = flags & IUP_DRAW_LAYOUTCENTER;

    HFONT hOldFont, hFont = (HFONT)iupwinGetHFont(font);
    SetTextColor(dc->hBitmapDC, RGB(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color)));
    hOldFont = SelectObject(dc->hBitmapDC, hFont);

    if (text_orientation && layout_center)
      iupDrawGetTextSize(dc->ih, text, len, &layout_w, &layout_h, 0);

    rect.left = x;
    rect.right = x + layout_w;
    rect.top = y;
    rect.bottom = y + layout_h;

    wtext = iupwinStrToSystemLen(text, &len);

    uFormat |= DT_LEFT;
    if (flags & IUP_DRAW_RIGHT)
      uFormat = DT_RIGHT;
    else if (flags & IUP_DRAW_CENTER)
      uFormat = DT_CENTER;

    if (flags & IUP_DRAW_WRAP)
      uFormat |= DT_WORDBREAK;
    else if (flags & IUP_DRAW_ELLIPSIS)
      uFormat |= DT_END_ELLIPSIS;

    if (!(flags & IUP_DRAW_CLIP))
      uFormat |= DT_NOCLIP;

    if (text_orientation)
    {
      if (layout_center)
      {
        gdiRotateWorld(dc->hBitmapDC, x + layout_w / 2, y + layout_h / 2, -IUP_DEG2RAD*text_orientation);  /* counterclockwise */
        gdiTranslateWorld(dc->hBitmapDC, (w - layout_w) / 2, (h - layout_h) / 2);  /* append the transform */
      }
      else
        gdiRotateWorld(dc->hBitmapDC, x, y, -IUP_DEG2RAD*text_orientation);  /* counterclockwise */
    }

    DrawText(dc->hBitmapDC, wtext, len, &rect, uFormat);

    /* restore settings */
    SelectObject(dc->hBitmapDC, hOldFont);
    if (text_orientation)
      gdiResetWorld(dc->hBitmapDC);
  }
}

IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, int x, int y, int w, int h)
{
  int bpp, img_w, img_h;
  HBITMAP hBitmap;

  if (dc->wdl_gc)
  {
    iupdrvDrawImageWDL(dc->wdl_gc, name, make_inactive, bgcolor, x, y, w, h);
    return;
  }

  hBitmap = (HBITMAP)iupImageGetImage(name, dc->ih, make_inactive, bgcolor);
  if (!hBitmap)
    return;

  /* must use this info, since image can be a driver image loaded from resources */
  iupdrvImageGetInfo(hBitmap, &img_w, &img_h, &bpp);

  if (w == -1 || w == 0) w = img_w;
  if (h == -1 || h == 0) h = img_h;

  iupwinDrawBitmap(dc->hBitmapDC, hBitmap, x, y, w, h, img_w, img_h, bpp);
}

IUP_SDK_API void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (dc->wdl_gc)
  {
    iupdrvDrawSelectRectWDL(dc->wdl_gc, x1, y1, x2, y2);
    return;
  }

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  BitBlt(dc->hBitmapDC, x1, y1, x2 - x1 + 1, y2 - y1 + 1, dc->hBitmapDC, x1, y1, DSTINVERT);
}

IUP_SDK_API void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  RECT rect;

  if (dc->wdl_gc)
  {
    iupdrvDrawFocusRectWDL(dc->wdl_gc, x1, y1, x2, y2);
    return;
  }

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  rect.left = x1;  
  rect.right = x2 + 1;
  rect.top = y1;
  rect.bottom = y2 + 1;

  /* for some reason text color affects focus drawing */
  SetTextColor(dc->hBitmapDC, RGB(0, 0, 0));

  DrawFocusRect(dc->hBitmapDC, &rect);
}
