/** \file
 * \brief WinUI Driver - Drawing Functions (Direct2D)
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdio>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_image.h"
#include "iup_drvfont.h"
#include "iup_class.h"
}

#include "iupwinui_drv.h"

#include <d2d1_1.h>
#include <d2d1helper.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dwrite.h>
#include "winrt/microsoft.ui.xaml.media.dxinterop.h"
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Windows::Foundation;


EXTERN_C const IID IID_ISurfaceImageSourceNativeWithD2D = {0xcb833102, 0xd5d1, 0x448b, {0xa3, 0x1a, 0x52, 0xa9, 0x50, 0x9f, 0x24, 0xe6}};

static com_ptr<ID3D11Device>    g_d3dDevice;
static com_ptr<IDXGIDevice>     g_dxgiDevice;
static com_ptr<ID2D1Factory1>   g_d2dFactory;
static com_ptr<ID2D1Device>     g_d2dDevice;
static com_ptr<IDWriteFactory>  g_dwriteFactory;

static bool winuiDrawEnsureDevices(void)
{
  if (g_d2dDevice)
    return true;

  UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
  D3D_FEATURE_LEVEL featureLevel;
  HRESULT hr = D3D11CreateDevice(
    nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
    flags, nullptr, 0, D3D11_SDK_VERSION,
    g_d3dDevice.put(), &featureLevel, nullptr);
  if (FAILED(hr))
  {
    hr = D3D11CreateDevice(
      nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
      flags, nullptr, 0, D3D11_SDK_VERSION,
      g_d3dDevice.put(), &featureLevel, nullptr);
    if (FAILED(hr))
      return false;
  }

  hr = g_d3dDevice->QueryInterface(g_dxgiDevice.put());
  if (FAILED(hr))
    return false;

  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, g_d2dFactory.put());
  if (FAILED(hr))
    return false;

  hr = g_d2dFactory->CreateDevice(g_dxgiDevice.get(), g_d2dDevice.put());
  if (FAILED(hr))
    return false;

  hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
    __uuidof(IDWriteFactory), reinterpret_cast<::IUnknown**>(g_dwriteFactory.put()));
  if (FAILED(hr))
    return false;

  return true;
}

extern "C" void iupwinuiDrawCleanup(void)
{
  g_dwriteFactory = nullptr;
  g_d2dDevice = nullptr;
  g_d2dFactory = nullptr;
  g_dxgiDevice = nullptr;
  g_d3dDevice = nullptr;
}


enum WinUIClipType
{
  WINUI_CLIP_NONE,
  WINUI_CLIP_RECT,
  WINUI_CLIP_LAYER
};

struct _IdrawCanvas
{
  Ihandle* ih;
  int w, h;

  SurfaceImageSource sis{nullptr};
  com_ptr<ISurfaceImageSourceNativeWithD2D> sisNative;
  ID2D1DeviceContext* d2dContext;
  POINT drawOffset;

  com_ptr<ID2D1SolidColorBrush> solidBrush;

  WinUIClipType clipType;
  int clip_x1, clip_y1, clip_x2, clip_y2;
};

static D2D1_COLOR_F winuiDrawColor(long color)
{
  return D2D1::ColorF(
    iupDrawRed(color) / 255.0f,
    iupDrawGreen(color) / 255.0f,
    iupDrawBlue(color) / 255.0f,
    iupDrawAlpha(color) / 255.0f);
}

static com_ptr<ID2D1StrokeStyle> winuiDrawStrokeStyle(int style)
{
  D2D1_DASH_STYLE dashStyle;
  switch (style)
  {
    case IUP_DRAW_STROKE_DASH:         dashStyle = D2D1_DASH_STYLE_DASH; break;
    case IUP_DRAW_STROKE_DOT:          dashStyle = D2D1_DASH_STYLE_DOT; break;
    case IUP_DRAW_STROKE_DASH_DOT:     dashStyle = D2D1_DASH_STYLE_DASH_DOT; break;
    case IUP_DRAW_STROKE_DASH_DOT_DOT: dashStyle = D2D1_DASH_STYLE_DASH_DOT_DOT; break;
    default:                           dashStyle = D2D1_DASH_STYLE_SOLID; break;
  }

  D2D1_STROKE_STYLE_PROPERTIES props = D2D1::StrokeStyleProperties(
    D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT,
    D2D1_LINE_JOIN_MITER, 10.0f, dashStyle, 0.0f);

  com_ptr<ID2D1StrokeStyle> strokeStyle;
  g_d2dFactory->CreateStrokeStyle(props, nullptr, 0, strokeStyle.put());
  return strokeStyle;
}

static void winuiDrawGetWidgetSize(Ihandle* ih, int* w, int* h)
{
  *w = ih->currentwidth;
  *h = ih->currentheight;

  if (iupClassMatch(ih->iclass, "canvas"))
  {
    IupWinUICanvasAux* aux = winuiGetAux<IupWinUICanvasAux>(ih, IUPWINUI_CANVAS_AUX);
    if (aux)
    {
      if (aux->sbVert && aux->sbVert.Visibility() == Visibility::Visible)
        *w -= iupdrvGetScrollbarSize();
      if (aux->sbHoriz && aux->sbHoriz.Visibility() == Visibility::Visible)
        *h -= iupdrvGetScrollbarSize();
    }
  }
}

static bool winuiDrawBeginSession(IdrawCanvas* dc)
{
  RECT updateRect = {0, 0, dc->w, dc->h};
  POINT offset = {};
  ID2D1DeviceContext* ctx = nullptr;

  HRESULT hr = dc->sisNative->BeginDraw(
    updateRect,
    __uuidof(ID2D1DeviceContext),
    reinterpret_cast<void**>(&ctx),
    &offset);

  if (FAILED(hr) || !ctx)
    return false;

  dc->d2dContext = ctx;
  dc->drawOffset = offset;

  dc->d2dContext->SetTransform(
    D2D1::Matrix3x2F::Translation((float)offset.x, (float)offset.y));

  dc->d2dContext->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), dc->solidBrush.put());

  return true;
}

extern "C" IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  if (!winuiDrawEnsureDevices())
    return nullptr;

  IdrawCanvas* dc = new IdrawCanvas();

  dc->ih = ih;
  winuiDrawGetWidgetSize(ih, &dc->w, &dc->h);
  if (dc->w <= 0) dc->w = 1;
  if (dc->h <= 0) dc->h = 1;

  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = 0;
  dc->clip_y2 = 0;
  dc->clipType = WINUI_CLIP_NONE;
  dc->d2dContext = nullptr;

  dc->sis = SurfaceImageSource(dc->w, dc->h);

  com_ptr<::IUnknown> sisUnknown;
  winrt::copy_to_abi(dc->sis, *sisUnknown.put_void());
  sisUnknown->QueryInterface(IID_ISurfaceImageSourceNativeWithD2D,
    dc->sisNative.put_void());

  dc->sisNative->SetDevice(g_d2dDevice.get());

  if (!winuiDrawBeginSession(dc))
  {
    delete dc;
    return nullptr;
  }

  iupAttribSet(ih, "DRAWDRIVER", "D2D");

  return dc;
}

extern "C" void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  if (!dc)
    return;

  if (dc->d2dContext)
  {
    if (dc->clipType == WINUI_CLIP_RECT)
      dc->d2dContext->PopAxisAlignedClip();
    else if (dc->clipType == WINUI_CLIP_LAYER)
      dc->d2dContext->PopLayer();

    dc->solidBrush = nullptr;
    dc->d2dContext->Release();
    dc->d2dContext = nullptr;

    dc->sisNative->EndDraw();
  }

  dc->sisNative = nullptr;
  dc->sis = nullptr;

  delete dc;
}

extern "C" void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  if (!dc)
    return;

  int w, h;
  winuiDrawGetWidgetSize(dc->ih, &w, &h);
  if (w <= 0) w = 1;
  if (h <= 0) h = 1;

  if (w != dc->w || h != dc->h)
  {
    if (dc->d2dContext)
    {
      if (dc->clipType == WINUI_CLIP_RECT)
        dc->d2dContext->PopAxisAlignedClip();
      else if (dc->clipType == WINUI_CLIP_LAYER)
        dc->d2dContext->PopLayer();

      dc->solidBrush = nullptr;
      dc->d2dContext->Release();
      dc->d2dContext = nullptr;
      dc->sisNative->EndDraw();
    }

    dc->w = w;
    dc->h = h;
    dc->clipType = WINUI_CLIP_NONE;
    dc->clip_x1 = dc->clip_y1 = dc->clip_x2 = dc->clip_y2 = 0;

    dc->sisNative = nullptr;
    dc->sis = SurfaceImageSource(dc->w, dc->h);

    com_ptr<::IUnknown> sisUnknown;
    winrt::copy_to_abi(dc->sis, *sisUnknown.put_void());
    sisUnknown->QueryInterface(IID_ISurfaceImageSourceNativeWithD2D,
      dc->sisNative.put_void());

    dc->sisNative->SetDevice(g_d2dDevice.get());
    winuiDrawBeginSession(dc);
  }
}

extern "C" void iupdrvDrawFlush(IdrawCanvas* dc)
{
  if (!dc)
    return;

  if (dc->d2dContext)
  {
    if (dc->clipType == WINUI_CLIP_RECT)
      dc->d2dContext->PopAxisAlignedClip();
    else if (dc->clipType == WINUI_CLIP_LAYER)
      dc->d2dContext->PopLayer();
    dc->clipType = WINUI_CLIP_NONE;

    dc->solidBrush = nullptr;
    dc->d2dContext->Release();
    dc->d2dContext = nullptr;
  }

  dc->sisNative->EndDraw();

  int isCanvas = iupClassMatch(dc->ih->iclass, "canvas");

  if (isCanvas)
  {
    IupWinUICanvasAux* aux = winuiGetAux<IupWinUICanvasAux>(dc->ih, IUPWINUI_CANVAS_AUX);
    if (aux && aux->displayImage)
    {
      aux->displayImage.Source(dc->sis);

      Canvas canvas = winuiGetHandle<Canvas>(dc->ih);
      if (canvas)
        canvas.InvalidateArrange();
    }
  }
  else
  {
    iupAttribSet(dc->ih, "_IUPWINUI_DRAW_BITMAP",
      reinterpret_cast<const char*>(winrt::detach_abi(dc->sis)));
  }
}

extern "C" void iupdrvDrawGetSize(IdrawCanvas* dc, int* w, int* h)
{
  if (dc)
  {
    if (w) *w = dc->w;
    if (h) *h = dc->h;
  }
  else
  {
    if (w) *w = 0;
    if (h) *h = 0;
  }
}

extern "C" void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc || !dc->d2dContext)
    return;

  dc->solidBrush->SetColor(winuiDrawColor(color));

  if (style == IUP_DRAW_STROKE || style == IUP_DRAW_FILL)
  {
    dc->d2dContext->DrawLine(
      D2D1::Point2F((float)x1, (float)y1),
      D2D1::Point2F((float)x2, (float)y2),
      dc->solidBrush.get(), (float)line_width);
  }
  else
  {
    auto strokeStyle = winuiDrawStrokeStyle(style);
    dc->d2dContext->DrawLine(
      D2D1::Point2F((float)x1, (float)y1),
      D2D1::Point2F((float)x2, (float)y2),
      dc->solidBrush.get(), (float)line_width, strokeStyle.get());
  }
}

extern "C" void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc || !dc->d2dContext)
    return;

  dc->solidBrush->SetColor(winuiDrawColor(color));

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  D2D1_RECT_F rect = D2D1::RectF((float)x1, (float)y1, (float)(x2 + 1), (float)(y2 + 1));

  if (style == IUP_DRAW_FILL)
  {
    dc->d2dContext->FillRectangle(rect, dc->solidBrush.get());
  }
  else if (style == IUP_DRAW_STROKE)
  {
    dc->d2dContext->DrawRectangle(rect, dc->solidBrush.get(), (float)line_width);
  }
  else
  {
    auto strokeStyle = winuiDrawStrokeStyle(style);
    dc->d2dContext->DrawRectangle(rect, dc->solidBrush.get(), (float)line_width, strokeStyle.get());
  }
}

extern "C" void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  if (!dc || !dc->d2dContext)
    return;

  dc->solidBrush->SetColor(winuiDrawColor(color));

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  float w = (float)(x2 - x1);
  float h = (float)(y2 - y1);
  float cx = x1 + w / 2.0f;
  float cy = y1 + h / 2.0f;
  float rx = w / 2.0f;
  float ry = h / 2.0f;

  float sweep = (float)(a2 - a1);
  while (sweep < 0) sweep += 360.0f;
  while (sweep > 360) sweep = 360.0f;

  if (sweep >= 359.99f)
  {
    D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(cx, cy), rx, ry);
    if (style == IUP_DRAW_FILL)
    {
      dc->d2dContext->FillEllipse(ellipse, dc->solidBrush.get());
    }
    else if (style == IUP_DRAW_STROKE)
    {
      dc->d2dContext->DrawEllipse(ellipse, dc->solidBrush.get(), (float)line_width);
    }
    else
    {
      auto strokeStyle = winuiDrawStrokeStyle(style);
      dc->d2dContext->DrawEllipse(ellipse, dc->solidBrush.get(), (float)line_width, strokeStyle.get());
    }
    return;
  }

  float startRad = (float)(a1 * IUP_DEG2RAD);
  float endRad = (float)(a2 * IUP_DEG2RAD);

  float startX = cx + rx * cosf(startRad);
  float startY = cy - ry * sinf(startRad);
  float endX = cx + rx * cosf(endRad);
  float endY = cy - ry * sinf(endRad);

  D2D1_SWEEP_DIRECTION sweepDir = D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;
  D2D1_ARC_SIZE arcSize = (sweep > 180.0f) ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL;

  com_ptr<ID2D1PathGeometry> pathGeometry;
  g_d2dFactory->CreatePathGeometry(pathGeometry.put());

  com_ptr<ID2D1GeometrySink> sink;
  pathGeometry->Open(sink.put());

  if (style == IUP_DRAW_FILL)
  {
    sink->BeginFigure(D2D1::Point2F(cx, cy), D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLine(D2D1::Point2F(startX, startY));
    D2D1_ARC_SEGMENT arc = {D2D1::Point2F(endX, endY), D2D1::SizeF(rx, ry), 0.0f, sweepDir, arcSize};
    sink->AddArc(arc);
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
  }
  else
  {
    sink->BeginFigure(D2D1::Point2F(startX, startY), D2D1_FIGURE_BEGIN_HOLLOW);
    D2D1_ARC_SEGMENT arc = {D2D1::Point2F(endX, endY), D2D1::SizeF(rx, ry), 0.0f, sweepDir, arcSize};
    sink->AddArc(arc);
    sink->EndFigure(D2D1_FIGURE_END_OPEN);
  }

  sink->Close();

  if (style == IUP_DRAW_FILL)
  {
    dc->d2dContext->FillGeometry(pathGeometry.get(), dc->solidBrush.get());
  }
  else if (style == IUP_DRAW_STROKE)
  {
    dc->d2dContext->DrawGeometry(pathGeometry.get(), dc->solidBrush.get(), (float)line_width);
  }
  else
  {
    auto strokeStyle = winuiDrawStrokeStyle(style);
    dc->d2dContext->DrawGeometry(pathGeometry.get(), dc->solidBrush.get(), (float)line_width, strokeStyle.get());
  }
}

extern "C" void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc || !dc->d2dContext)
    return;

  dc->solidBrush->SetColor(winuiDrawColor(color));

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  float w = (float)(x2 - x1);
  float h = (float)(y2 - y1);
  float cx = x1 + w / 2.0f;
  float cy = y1 + h / 2.0f;
  float rx = w / 2.0f;
  float ry = h / 2.0f;

  D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(cx, cy), rx, ry);

  if (style == IUP_DRAW_FILL)
  {
    dc->d2dContext->FillEllipse(ellipse, dc->solidBrush.get());
  }
  else if (style == IUP_DRAW_STROKE)
  {
    dc->d2dContext->DrawEllipse(ellipse, dc->solidBrush.get(), (float)line_width);
  }
  else
  {
    auto strokeStyle = winuiDrawStrokeStyle(style);
    dc->d2dContext->DrawEllipse(ellipse, dc->solidBrush.get(), (float)line_width, strokeStyle.get());
  }
}

extern "C" void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  if (!dc || !dc->d2dContext || count < 2)
    return;

  dc->solidBrush->SetColor(winuiDrawColor(color));

  com_ptr<ID2D1PathGeometry> pathGeometry;
  g_d2dFactory->CreatePathGeometry(pathGeometry.put());

  com_ptr<ID2D1GeometrySink> sink;
  pathGeometry->Open(sink.put());

  sink->BeginFigure(
    D2D1::Point2F((float)points[0], (float)points[1]),
    (style == IUP_DRAW_FILL) ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);

  for (int i = 1; i < count; i++)
    sink->AddLine(D2D1::Point2F((float)points[2 * i], (float)points[2 * i + 1]));

  sink->EndFigure(D2D1_FIGURE_END_CLOSED);
  sink->Close();

  if (style == IUP_DRAW_FILL)
  {
    dc->d2dContext->FillGeometry(pathGeometry.get(), dc->solidBrush.get());
  }
  else if (style == IUP_DRAW_STROKE)
  {
    dc->d2dContext->DrawGeometry(pathGeometry.get(), dc->solidBrush.get(), (float)line_width);
  }
  else
  {
    auto strokeStyle = winuiDrawStrokeStyle(style);
    dc->d2dContext->DrawGeometry(pathGeometry.get(), dc->solidBrush.get(), (float)line_width, strokeStyle.get());
  }
}

extern "C" void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
{
  if (!dc || !dc->d2dContext)
    return;

  dc->solidBrush->SetColor(winuiDrawColor(color));
  dc->d2dContext->FillRectangle(D2D1::RectF((float)x, (float)y, (float)(x + 1), (float)(y + 1)), dc->solidBrush.get());
}

extern "C" void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int radius, long color, int style, int line_width)
{
  if (!dc || !dc->d2dContext)
    return;

  dc->solidBrush->SetColor(winuiDrawColor(color));

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  int max_radius = ((x2 - x1) < (y2 - y1)) ? (x2 - x1) / 2 : (y2 - y1) / 2;
  if (radius > max_radius)
    radius = max_radius;

  D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
    D2D1::RectF((float)x1, (float)y1, (float)(x2 + 1), (float)(y2 + 1)),
    (float)radius, (float)radius);

  if (style == IUP_DRAW_FILL)
  {
    dc->d2dContext->FillRoundedRectangle(roundedRect, dc->solidBrush.get());
  }
  else if (style == IUP_DRAW_STROKE)
  {
    dc->d2dContext->DrawRoundedRectangle(roundedRect, dc->solidBrush.get(), (float)line_width);
  }
  else
  {
    auto strokeStyle = winuiDrawStrokeStyle(style);
    dc->d2dContext->DrawRoundedRectangle(roundedRect, dc->solidBrush.get(), (float)line_width, strokeStyle.get());
  }
}

extern "C" void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
  if (!dc || !dc->d2dContext)
    return;

  dc->solidBrush->SetColor(winuiDrawColor(color));

  com_ptr<ID2D1PathGeometry> pathGeometry;
  g_d2dFactory->CreatePathGeometry(pathGeometry.put());

  com_ptr<ID2D1GeometrySink> sink;
  pathGeometry->Open(sink.put());

  sink->BeginFigure(D2D1::Point2F((float)x1, (float)y1), D2D1_FIGURE_BEGIN_HOLLOW);

  D2D1_BEZIER_SEGMENT bezier = {
    D2D1::Point2F((float)x2, (float)y2),
    D2D1::Point2F((float)x3, (float)y3),
    D2D1::Point2F((float)x4, (float)y4)};
  sink->AddBezier(bezier);

  sink->EndFigure(D2D1_FIGURE_END_OPEN);
  sink->Close();

  if (style == IUP_DRAW_FILL)
  {
    dc->d2dContext->FillGeometry(pathGeometry.get(), dc->solidBrush.get());
  }
  else if (style == IUP_DRAW_STROKE)
  {
    dc->d2dContext->DrawGeometry(pathGeometry.get(), dc->solidBrush.get(), (float)line_width);
  }
  else
  {
    auto strokeStyle = winuiDrawStrokeStyle(style);
    dc->d2dContext->DrawGeometry(pathGeometry.get(), dc->solidBrush.get(), (float)line_width, strokeStyle.get());
  }
}

extern "C" void iupdrvDrawQuadraticBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, long color, int style, int line_width)
{
  if (!dc || !dc->d2dContext)
    return;

  dc->solidBrush->SetColor(winuiDrawColor(color));

  com_ptr<ID2D1PathGeometry> pathGeometry;
  g_d2dFactory->CreatePathGeometry(pathGeometry.put());

  com_ptr<ID2D1GeometrySink> sink;
  pathGeometry->Open(sink.put());

  sink->BeginFigure(D2D1::Point2F((float)x1, (float)y1), D2D1_FIGURE_BEGIN_HOLLOW);

  D2D1_QUADRATIC_BEZIER_SEGMENT qbezier = {
    D2D1::Point2F((float)x2, (float)y2),
    D2D1::Point2F((float)x3, (float)y3)};
  sink->AddQuadraticBezier(qbezier);

  sink->EndFigure(D2D1_FIGURE_END_OPEN);
  sink->Close();

  if (style == IUP_DRAW_FILL)
  {
    dc->d2dContext->FillGeometry(pathGeometry.get(), dc->solidBrush.get());
  }
  else if (style == IUP_DRAW_STROKE)
  {
    dc->d2dContext->DrawGeometry(pathGeometry.get(), dc->solidBrush.get(), (float)line_width);
  }
  else
  {
    auto strokeStyle = winuiDrawStrokeStyle(style);
    dc->d2dContext->DrawGeometry(pathGeometry.get(), dc->solidBrush.get(), (float)line_width, strokeStyle.get());
  }
}

extern "C" void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  if (!dc || !dc->d2dContext || !text)
    return;

  dc->solidBrush->SetColor(winuiDrawColor(color));

  char typeface[256] = "";
  int size = 12;
  int is_bold = 0, is_italic = 0, is_underline = 0, is_strikeout = 0;

  if (font)
    iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout);

  wchar_t wTypeface[256];
  if (typeface[0])
    MultiByteToWideChar(CP_UTF8, 0, typeface, -1, wTypeface, 256);
  else
    wcscpy(wTypeface, L"Segoe UI");

  com_ptr<IDWriteTextFormat> textFormat;
  g_dwriteFactory->CreateTextFormat(
    wTypeface, nullptr,
    is_bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
    is_italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
    DWRITE_FONT_STRETCH_NORMAL,
    (float)size, L"", textFormat.put());

  if (!textFormat)
    return;

  if (flags & IUP_DRAW_CENTER)
    textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
  else if (flags & IUP_DRAW_RIGHT)
    textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
  else
    textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

  if (flags & IUP_DRAW_WRAP)
    textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
  else
    textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

  wchar_t* wtext = nullptr;
  int wlen = 0;

  if (len > 0)
  {
    wlen = MultiByteToWideChar(CP_UTF8, 0, text, len, nullptr, 0);
    wtext = (wchar_t*)malloc((wlen + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, text, len, wtext, wlen);
    wtext[wlen] = 0;
  }
  else
  {
    wlen = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0) - 1;
    wtext = (wchar_t*)malloc((wlen + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wtext, wlen + 1);
  }

  float fx = (float)x;
  float fy = (float)y;
  float fw = (w > 0) ? (float)w : (float)dc->w;
  float fh = (h > 0) ? (float)h : (float)dc->h;

  D2D1_RECT_F layoutRect = D2D1::RectF(fx, fy, fx + fw, fy + fh);

  if (text_orientation != 0.0)
  {
    D2D1_MATRIX_3X2_F oldTransform;
    dc->d2dContext->GetTransform(&oldTransform);

    float tcx = fx + fw / 2.0f;
    float tcy = fy + fh / 2.0f;
    float rad = (float)(-text_orientation * IUP_DEG2RAD);

    D2D1_MATRIX_3X2_F rotation = D2D1::Matrix3x2F::Rotation(
      (float)(-text_orientation), D2D1::Point2F(tcx, tcy));

    dc->d2dContext->SetTransform(oldTransform * rotation);
    dc->d2dContext->DrawText(wtext, wlen, textFormat.get(), layoutRect, dc->solidBrush.get());
    dc->d2dContext->SetTransform(oldTransform);
  }
  else
  {
    if (flags & IUP_DRAW_CLIP)
    {
      dc->d2dContext->PushAxisAlignedClip(layoutRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
      dc->d2dContext->DrawText(wtext, wlen, textFormat.get(), layoutRect, dc->solidBrush.get());
      dc->d2dContext->PopAxisAlignedClip();
    }
    else
    {
      dc->d2dContext->DrawText(wtext, wlen, textFormat.get(), layoutRect, dc->solidBrush.get());
    }
  }

  free(wtext);
}

extern "C" void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, int x, int y, int w, int h)
{
  if (!dc || !dc->d2dContext || !name)
    return;

  void* handle = iupImageGetImage(name, dc->ih, make_inactive, bgcolor);
  if (!handle)
    return;

  int img_w = 0, img_h = 0, img_bpp = 0;
  iupdrvImageGetInfo(handle, &img_w, &img_h, &img_bpp);
  if (img_w == 0 || img_h == 0)
    return;

  if (w == -1 || w == 0) w = img_w;
  if (h == -1 || h == 0) h = img_h;

  WriteableBitmap wb = winuiGetBitmapFromHandle(handle);
  if (!wb)
    return;

  Windows::Storage::Streams::IBuffer pixelBuffer = wb.PixelBuffer();
  uint8_t* srcPixels = pixelBuffer.data();
  uint32_t srcSize = pixelBuffer.Length();

  D2D1_BITMAP_PROPERTIES bitmapProps = D2D1::BitmapProperties(
    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

  com_ptr<ID2D1Bitmap> d2dBitmap;
  HRESULT hr = dc->d2dContext->CreateBitmap(
    D2D1::SizeU(img_w, img_h),
    srcPixels, img_w * 4,
    bitmapProps, d2dBitmap.put());

  if (FAILED(hr) || !d2dBitmap)
    return;

  D2D1_RECT_F destRect = D2D1::RectF((float)x, (float)y, (float)(x + w), (float)(y + h));
  dc->d2dContext->DrawBitmap(d2dBitmap.get(), destRect);
}

extern "C" void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc || !dc->d2dContext)
    return;

  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }

  if (dc->clipType == WINUI_CLIP_RECT)
    dc->d2dContext->PopAxisAlignedClip();
  else if (dc->clipType == WINUI_CLIP_LAYER)
    dc->d2dContext->PopLayer();

  if (x1 >= x2) x1 = x2;
  if (y1 >= y2) y1 = y2;

  D2D1_RECT_F clipRect = D2D1::RectF((float)x1, (float)y1, (float)(x2 + 1), (float)(y2 + 1));
  dc->d2dContext->PushAxisAlignedClip(clipRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

  dc->clipType = WINUI_CLIP_RECT;
  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
}

extern "C" void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int radius)
{
  if (!dc || !dc->d2dContext)
    return;

  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }

  if (dc->clipType == WINUI_CLIP_RECT)
    dc->d2dContext->PopAxisAlignedClip();
  else if (dc->clipType == WINUI_CLIP_LAYER)
    dc->d2dContext->PopLayer();

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  int max_radius = ((x2 - x1) < (y2 - y1)) ? (x2 - x1) / 2 : (y2 - y1) / 2;
  if (radius > max_radius)
    radius = max_radius;

  D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
    D2D1::RectF((float)x1, (float)y1, (float)x2, (float)y2),
    (float)radius, (float)radius);

  com_ptr<ID2D1RoundedRectangleGeometry> geometry;
  g_d2dFactory->CreateRoundedRectangleGeometry(roundedRect, geometry.put());

  D2D1_LAYER_PARAMETERS layerParams = D2D1::LayerParameters(
    D2D1::InfiniteRect(), geometry.get());

  dc->d2dContext->PushLayer(layerParams, nullptr);

  dc->clipType = WINUI_CLIP_LAYER;
  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
}

extern "C" void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  if (!dc || !dc->d2dContext)
    return;

  if (dc->clipType == WINUI_CLIP_RECT)
    dc->d2dContext->PopAxisAlignedClip();
  else if (dc->clipType == WINUI_CLIP_LAYER)
    dc->d2dContext->PopLayer();

  dc->clipType = WINUI_CLIP_NONE;
  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = 0;
  dc->clip_y2 = 0;
}

extern "C" void iupdrvDrawGetClipRect(IdrawCanvas* dc, int* x1, int* y1, int* x2, int* y2)
{
  if (dc)
  {
    if (x1) *x1 = dc->clip_x1;
    if (y1) *y1 = dc->clip_y1;
    if (x2) *x2 = dc->clip_x2;
    if (y2) *y2 = dc->clip_y2;
  }
  else
  {
    if (x1) *x1 = 0;
    if (y1) *y1 = 0;
    if (x2) *x2 = 0;
    if (y2) *y2 = 0;
  }
}

extern "C" void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc || !dc->d2dContext)
    return;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  using namespace Windows::UI::ViewManagement;
  UISettings settings;
  auto accent = settings.GetColorValue(UIColorType::Accent);

  D2D1_COLOR_F c = D2D1::ColorF(accent.R / 255.0f, accent.G / 255.0f, accent.B / 255.0f, 153.0f / 255.0f);
  dc->solidBrush->SetColor(c);

  dc->d2dContext->FillRectangle(
    D2D1::RectF((float)x1, (float)y1, (float)(x2 + 1), (float)(y2 + 1)),
    dc->solidBrush.get());
}

extern "C" void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc || !dc->d2dContext)
    return;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  iupdrvDrawRectangle(dc, x1, y1, x2, y2, iupDrawColor(0, 0, 0, 224), IUP_DRAW_STROKE_DOT, 1);
}

extern "C" void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, long color1, long color2)
{
  if (!dc || !dc->d2dContext)
    return;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  float w = (float)(x2 - x1);
  float h = (float)(y2 - y1);
  float cx = x1 + w / 2.0f;
  float cy = y1 + h / 2.0f;

  float rad = angle * (float)IUP_DEG2RAD;
  float sx = cx - (w * cosf(rad)) / 2.0f;
  float sy = cy - (h * sinf(rad)) / 2.0f;
  float ex = cx + (w * cosf(rad)) / 2.0f;
  float ey = cy + (h * sinf(rad)) / 2.0f;

  D2D1_COLOR_F c1 = winuiDrawColor(color1);
  D2D1_COLOR_F c2 = winuiDrawColor(color2);

  D2D1_GRADIENT_STOP stops[2] = {{0.0f, c1}, {1.0f, c2}};
  com_ptr<ID2D1GradientStopCollection> collection;
  dc->d2dContext->CreateGradientStopCollection(stops, 2, collection.put());

  com_ptr<ID2D1LinearGradientBrush> brush;
  dc->d2dContext->CreateLinearGradientBrush(
    D2D1::LinearGradientBrushProperties(D2D1::Point2F(sx, sy), D2D1::Point2F(ex, ey)),
    collection.get(), brush.put());

  dc->d2dContext->FillRectangle(
    D2D1::RectF((float)x1, (float)y1, (float)x1 + w, (float)y1 + h),
    brush.get());
}

extern "C" void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, long colorCenter, long colorEdge)
{
  if (!dc || !dc->d2dContext)
    return;

  D2D1_COLOR_F c1 = winuiDrawColor(colorCenter);
  D2D1_COLOR_F c2 = winuiDrawColor(colorEdge);

  D2D1_GRADIENT_STOP stops[2] = {{0.0f, c1}, {1.0f, c2}};
  com_ptr<ID2D1GradientStopCollection> collection;
  dc->d2dContext->CreateGradientStopCollection(stops, 2, collection.put());

  com_ptr<ID2D1RadialGradientBrush> brush;
  dc->d2dContext->CreateRadialGradientBrush(
    D2D1::RadialGradientBrushProperties(
      D2D1::Point2F((float)cx, (float)cy),
      D2D1::Point2F(0.0f, 0.0f),
      (float)radius, (float)radius),
    collection.get(), brush.put());

  dc->d2dContext->FillEllipse(
    D2D1::Ellipse(D2D1::Point2F((float)cx, (float)cy), (float)radius, (float)radius),
    brush.get());
}
