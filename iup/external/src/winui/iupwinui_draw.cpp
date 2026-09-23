/** \file
 * \brief WinUI Driver - Drawing Functions (Direct2D)
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstring>
#include <cmath>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_image.h"
#include "iup_drvfont.h"
#include "iup_str.h"
#include "iup_class.h"
}

#include "iupwinui_drv.h"

#include <d2d1_1.h>
#include <d2d1helper.h>
#include <d3d11.h>
#include <dwrite.h>

#include "winrt/microsoft.ui.xaml.media.dxinterop.h"
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

static com_ptr<ID2D1StrokeStyle> g_strokeStyle;
static IupDrawStroke g_strokeStyleKey;
static float g_strokeStyleWidth = 0;
static bool g_strokeStyleValid = false;

IUP_DRV_API void iupwinuiDrawCleanup(void)
{
  g_strokeStyle = nullptr;
  g_strokeStyleValid = false;
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

struct WinUIDrawLayer
{
  WinUIClipType clipType;
  int clip_x1, clip_y1, clip_x2, clip_y2;
  D2D1_TEXT_ANTIALIAS_MODE textAntialias;
  WinUIDrawLayer* next;
};

struct _IdrawCanvas
{
  Ihandle* ih;
  int w, h;

  SurfaceImageSource sis{nullptr};
  bool sisIsNew{false};
  com_ptr<ISurfaceImageSourceNativeWithD2D> sisNative;
  ID2D1DeviceContext* d2dContext;
  POINT drawOffset;
  D2D1_MATRIX_3X2_F baseTransform;
  D2D1_MATRIX_3X2_F userTransform{D2D1::Matrix3x2F::Identity()};

  bool partial{false};
  int px1{0}, py1{0}, px2{0}, py2{0};

  com_ptr<ID2D1SolidColorBrush> solidBrush;

  WinUIClipType clipType;
  int clip_x1, clip_y1, clip_x2, clip_y2;
  WinUIDrawLayer* layers{nullptr};
};

static D2D1_COLOR_F winuiDrawColor(long color)
{
  return D2D1::ColorF(
    iupDrawRed(color) / 255.0f,
    iupDrawGreen(color) / 255.0f,
    iupDrawBlue(color) / 255.0f,
    iupDrawAlpha(color) / 255.0f);
}

static bool winuiDrawStrokeEqual(const IupDrawStroke& a, const IupDrawStroke& b)
{
  if (a.cap != b.cap || a.join != b.join || a.dash_count != b.dash_count || a.dash_offset != b.dash_offset)
    return false;

  for (int i = 0; i < a.dash_count; i++)
  {
    if (a.dashes[i] != b.dashes[i])
      return false;
  }

  return true;
}

static ID2D1StrokeStyle* winuiDrawStrokeStyle(IdrawCanvas* dc, int style, int line_width)
{
  IupDrawStroke stroke;
  float dashes[IUP_DRAW_MAX_DASHES];
  float width = (float)line_width;

  if (width <= 0)
    width = 1.0f;

  iupDrawGetStroke(dc->ih, style, &stroke);

  if (stroke.dash_count == 0 && stroke.cap == IUP_DRAW_CAP_BUTT && stroke.join == IUP_DRAW_JOIN_MITER)
    return nullptr;

  if (g_strokeStyleValid && g_strokeStyleWidth == width && winuiDrawStrokeEqual(g_strokeStyleKey, stroke))
    return g_strokeStyle.get();

  for (int i = 0; i < stroke.dash_count; i++)
    dashes[i] = (float)(stroke.dashes[i] / width);

  D2D1_CAP_STYLE cap = stroke.cap == IUP_DRAW_CAP_ROUND ? D2D1_CAP_STYLE_ROUND :
                       stroke.cap == IUP_DRAW_CAP_SQUARE ? D2D1_CAP_STYLE_SQUARE : D2D1_CAP_STYLE_FLAT;
  D2D1_LINE_JOIN join = stroke.join == IUP_DRAW_JOIN_ROUND ? D2D1_LINE_JOIN_ROUND :
                        stroke.join == IUP_DRAW_JOIN_BEVEL ? D2D1_LINE_JOIN_BEVEL : D2D1_LINE_JOIN_MITER;

  D2D1_STROKE_STYLE_PROPERTIES props = D2D1::StrokeStyleProperties(
    cap, cap, cap, join, (float)IUP_DRAW_MITER_LIMIT,
    stroke.dash_count > 0 ? D2D1_DASH_STYLE_CUSTOM : D2D1_DASH_STYLE_SOLID,
    stroke.dash_count > 0 ? (float)(stroke.dash_offset / width) : 0.0f);

  g_strokeStyle = nullptr;
  g_d2dFactory->CreateStrokeStyle(props, stroke.dash_count > 0 ? dashes : nullptr, (UINT32)stroke.dash_count, g_strokeStyle.put());

  g_strokeStyleKey = stroke;
  g_strokeStyleWidth = width;
  g_strokeStyleValid = g_strokeStyle != nullptr;

  return g_strokeStyle.get();
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

  if (dc->partial)
    updateRect = {dc->px1, dc->py1, dc->px2 + 1, dc->py2 + 1};

  HRESULT hr = dc->sisNative->BeginDraw(
    updateRect,
    __uuidof(ID2D1DeviceContext),
    reinterpret_cast<void**>(&ctx),
    &offset);

  if (FAILED(hr) || !ctx)
    return false;

  dc->d2dContext = ctx;
  dc->drawOffset = offset;

  dc->baseTransform = D2D1::Matrix3x2F::Translation((float)(offset.x - updateRect.left), (float)(offset.y - updateRect.top));
  dc->d2dContext->SetTransform(dc->userTransform * dc->baseTransform);

  if (dc->partial)
    dc->d2dContext->PushAxisAlignedClip(
      D2D1::RectF((float)dc->px1, (float)dc->py1, (float)(dc->px2 + 1), (float)(dc->py2 + 1)),
      D2D1_ANTIALIAS_MODE_ALIASED);

  dc->d2dContext->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), dc->solidBrush.put());

  return true;
}

extern "C" IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
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

  IupWinUICanvasAux* aux = iupClassMatch(ih->iclass, "canvas")
    ? winuiGetAux<IupWinUICanvasAux>(ih, IUPWINUI_CANVAS_AUX) : nullptr;

  if (aux && aux->sis && (aux->sisWidth != dc->w || aux->sisHeight != dc->h))
    aux->sis = nullptr;

  if (aux && aux->sis)
    dc->sis = aux->sis;
  else
  {
    dc->sis = SurfaceImageSource(dc->w, dc->h);
    dc->sisIsNew = true;

    if (aux)
    {
      aux->sis = dc->sis;
      aux->sisWidth = dc->w;
      aux->sisHeight = dc->h;
    }
  }

  com_ptr<::IUnknown> sisUnknown;
  winrt::copy_to_abi(dc->sis, *sisUnknown.put_void());
  sisUnknown->QueryInterface(IID_ISurfaceImageSourceNativeWithD2D,
    dc->sisNative.put_void());

  if (dc->sisIsNew)
    dc->sisNative->SetDevice(g_d2dDevice.get());

  if (aux)
  {
    int x1, y1, x2, y2;
    char* clip = iupAttribGet(ih, "CLIPRECT");
    if (clip && sscanf(clip, "%d %d %d %d", &x1, &y1, &x2, &y2) == 4
        && !(x1 <= 0 && y1 <= 0 && x2 >= dc->w - 1 && y2 >= dc->h - 1))
    {
      if (dc->sisIsNew)
        iupdrvPostRedraw(ih);
      else
      {
        if (x1 < 0) x1 = 0;
        if (y1 < 0) y1 = 0;
        if (x2 > dc->w - 1) x2 = dc->w - 1;
        if (y2 > dc->h - 1) y2 = dc->h - 1;
        if (x1 <= x2 && y1 <= y2)
        {
          dc->partial = true;
          dc->px1 = x1; dc->py1 = y1;
          dc->px2 = x2; dc->py2 = y2;
        }
      }
    }
  }

  if (!winuiDrawBeginSession(dc))
  {
    delete dc;
    return nullptr;
  }

  iupAttribSet(ih, "DRAWDRIVER", "D2D");

  return dc;
}

extern "C" IUP_SDK_API void iupdrvDrawSetTransform(IdrawCanvas* dc, const IupDrawMatrix* matrix)
{
  if (!dc || !dc->d2dContext)
    return;
  dc->userTransform = D2D1::Matrix3x2F((float)matrix->a, (float)matrix->b,
                                       (float)matrix->c, (float)matrix->d,
                                       (float)matrix->e, (float)matrix->f);
  dc->d2dContext->SetTransform(dc->userTransform * dc->baseTransform);
}

extern "C" IUP_SDK_API int iupdrvDrawBeginLayer(IdrawCanvas* dc, int alpha)
{
  if (!dc || !dc->d2dContext)
    return 0;

  WinUIDrawLayer* layer = new WinUIDrawLayer();
  layer->clipType = dc->clipType;
  layer->clip_x1 = dc->clip_x1;
  layer->clip_y1 = dc->clip_y1;
  layer->clip_x2 = dc->clip_x2;
  layer->clip_y2 = dc->clip_y2;
  layer->textAntialias = dc->d2dContext->GetTextAntialiasMode();
  layer->next = dc->layers;
  dc->layers = layer;

  dc->clipType = WINUI_CLIP_NONE;
  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = 0;
  dc->clip_y2 = 0;

  dc->d2dContext->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                                  D2D1::IdentityMatrix(), alpha / 255.0f), nullptr);
  dc->d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
  return 1;
}

extern "C" IUP_SDK_API void iupdrvDrawEndLayer(IdrawCanvas* dc, int alpha)
{
  (void)alpha;

  if (!dc || !dc->layers)
    return;

  WinUIDrawLayer* layer = dc->layers;

  if (dc->d2dContext)
  {
    if (dc->clipType == WINUI_CLIP_RECT)
      dc->d2dContext->PopAxisAlignedClip();
    else if (dc->clipType == WINUI_CLIP_LAYER)
      dc->d2dContext->PopLayer();

    dc->d2dContext->PopLayer();
    dc->d2dContext->SetTextAntialiasMode(layer->textAntialias);
  }

  dc->clipType = layer->clipType;
  dc->clip_x1 = layer->clip_x1;
  dc->clip_y1 = layer->clip_y1;
  dc->clip_x2 = layer->clip_x2;
  dc->clip_y2 = layer->clip_y2;
  dc->layers = layer->next;
  delete layer;
}

static void winuiDrawEndAllLayers(IdrawCanvas* dc)
{
  while (dc->layers)
    iupdrvDrawEndLayer(dc, 255);
}

extern "C" IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  if (!dc)
    return;

  winuiDrawEndAllLayers(dc);

  if (dc->d2dContext)
  {
    if (dc->clipType == WINUI_CLIP_RECT)
      dc->d2dContext->PopAxisAlignedClip();
    else if (dc->clipType == WINUI_CLIP_LAYER)
      dc->d2dContext->PopLayer();

    if (dc->partial)
      dc->d2dContext->PopAxisAlignedClip();

    dc->solidBrush = nullptr;
    dc->d2dContext->Release();
    dc->d2dContext = nullptr;

    dc->sisNative->EndDraw();
  }

  dc->sisNative = nullptr;
  dc->sis = nullptr;

  delete dc;
}

extern "C" IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  if (!dc)
    return;

  int w, h;
  winuiDrawGetWidgetSize(dc->ih, &w, &h);
  if (w <= 0) w = 1;
  if (h <= 0) h = 1;

  if (w != dc->w || h != dc->h)
  {
    winuiDrawEndAllLayers(dc);

    if (dc->d2dContext)
    {
      if (dc->clipType == WINUI_CLIP_RECT)
        dc->d2dContext->PopAxisAlignedClip();
      else if (dc->clipType == WINUI_CLIP_LAYER)
        dc->d2dContext->PopLayer();

      if (dc->partial)
        dc->d2dContext->PopAxisAlignedClip();

      dc->solidBrush = nullptr;
      dc->d2dContext->Release();
      dc->d2dContext = nullptr;
      dc->sisNative->EndDraw();
    }

    dc->w = w;
    dc->h = h;
    dc->partial = false;
    dc->clipType = WINUI_CLIP_NONE;
    dc->clip_x1 = dc->clip_y1 = dc->clip_x2 = dc->clip_y2 = 0;

    dc->sisNative = nullptr;
    dc->sis = SurfaceImageSource(dc->w, dc->h);
    dc->sisIsNew = true;

    {
      IupWinUICanvasAux* aux = iupClassMatch(dc->ih->iclass, "canvas")
        ? winuiGetAux<IupWinUICanvasAux>(dc->ih, IUPWINUI_CANVAS_AUX) : nullptr;
      if (aux)
      {
        aux->sis = dc->sis;
        aux->sisWidth = dc->w;
        aux->sisHeight = dc->h;
      }
    }

    com_ptr<::IUnknown> sisUnknown;
    winrt::copy_to_abi(dc->sis, *sisUnknown.put_void());
    sisUnknown->QueryInterface(IID_ISurfaceImageSourceNativeWithD2D,
      dc->sisNative.put_void());

    dc->sisNative->SetDevice(g_d2dDevice.get());
    winuiDrawBeginSession(dc);
  }
}

static void winuiDrawCopyToBuffer(IdrawCanvas* dc)
{
  int isCanvas = iupClassMatch(dc->ih->iclass, "canvas");
  if (!isCanvas || !dc->d2dContext)
    return;

  dc->d2dContext->Flush();

  ID2D1Bitmap1* oldBuffer = (ID2D1Bitmap1*)iupAttribGet(dc->ih, "_IUPWINUI_CANVAS_BUFFER");

  if (dc->partial)
  {
    if (oldBuffer)
    {
      D2D1_SIZE_U oldSize = oldBuffer->GetPixelSize();
      if (oldSize.width == (UINT32)dc->w && oldSize.height == (UINT32)dc->h)
      {
        com_ptr<ID2D1Image> target;
        dc->d2dContext->GetTarget(target.put());
        if (target)
        {
          com_ptr<ID2D1Bitmap1> targetBitmap;
          target->QueryInterface(targetBitmap.put());
          if (targetBitmap)
          {
            D2D1_POINT_2U destPoint = {(UINT32)dc->px1, (UINT32)dc->py1};
            D2D1_RECT_U srcRect = {
              (UINT32)dc->drawOffset.x, (UINT32)dc->drawOffset.y,
              (UINT32)(dc->drawOffset.x + (dc->px2 - dc->px1 + 1)),
              (UINT32)(dc->drawOffset.y + (dc->py2 - dc->py1 + 1))
            };
            oldBuffer->CopyFromBitmap(&destPoint, targetBitmap.get(), &srcRect);
          }
        }
      }
    }
    return;
  }

  if (oldBuffer)
  {
    oldBuffer->Release();
    iupAttribSet(dc->ih, "_IUPWINUI_CANVAS_BUFFER", NULL);
  }

  com_ptr<ID2D1Image> target;
  dc->d2dContext->GetTarget(target.put());
  if (!target)
    return;

  com_ptr<ID2D1Bitmap1> targetBitmap;
  target->QueryInterface(targetBitmap.put());
  if (!targetBitmap)
    return;

  D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
    D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

  ID2D1Bitmap1* stagingBitmap = nullptr;
  HRESULT hr = dc->d2dContext->CreateBitmap(
    D2D1::SizeU(dc->w, dc->h), nullptr, 0, props, &stagingBitmap);
  if (FAILED(hr) || !stagingBitmap)
    return;

  D2D1_POINT_2U destPoint = {0, 0};
  D2D1_RECT_U srcRect = {
    (UINT32)dc->drawOffset.x, (UINT32)dc->drawOffset.y,
    (UINT32)(dc->drawOffset.x + dc->w), (UINT32)(dc->drawOffset.y + dc->h)
  };

  hr = stagingBitmap->CopyFromBitmap(&destPoint, targetBitmap.get(), &srcRect);
  if (FAILED(hr))
  {
    stagingBitmap->Release();
    return;
  }

  iupAttribSet(dc->ih, "_IUPWINUI_CANVAS_BUFFER", (char*)stagingBitmap);
}

extern "C" IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc)
{
  if (!dc)
    return;

  winuiDrawEndAllLayers(dc);

  if (dc->d2dContext)
  {
    winuiDrawCopyToBuffer(dc);

    if (dc->clipType == WINUI_CLIP_RECT)
      dc->d2dContext->PopAxisAlignedClip();
    else if (dc->clipType == WINUI_CLIP_LAYER)
      dc->d2dContext->PopLayer();
    dc->clipType = WINUI_CLIP_NONE;

    if (dc->partial)
      dc->d2dContext->PopAxisAlignedClip();

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
      if (dc->sisIsNew)
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

extern "C" IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int* w, int* h)
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

extern "C" IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc || !dc->d2dContext)
    return;

  dc->solidBrush->SetColor(winuiDrawColor(color));

  D2D1_POINT_2F p1 = D2D1::Point2F((float)x1, (float)y1);
  D2D1_POINT_2F p2 = D2D1::Point2F((float)x2, (float)y2);
  if (line_width == 1 && (x1 == x2 || y1 == y2))
  {
    iupDrawCheckSwapCoord(x1, x2);
    iupDrawCheckSwapCoord(y1, y2);
    if (x1 == x2)
    {
      p1 = D2D1::Point2F(x1 + 0.5f, (float)y1);
      p2 = D2D1::Point2F(x1 + 0.5f, (float)(y2 + 1));
    }
    else
    {
      p1 = D2D1::Point2F((float)x1, y1 + 0.5f);
      p2 = D2D1::Point2F((float)(x2 + 1), y1 + 0.5f);
    }
  }

  dc->d2dContext->DrawLine(p1, p2, dc->solidBrush.get(), (float)line_width, winuiDrawStrokeStyle(dc, style, line_width));
}

extern "C" IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  if (!dc || !dc->d2dContext)
    return;

  dc->solidBrush->SetColor(winuiDrawColor(color));

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  D2D1_RECT_F rect = D2D1::RectF((float)x1, (float)y1, (float)(x2 + 1), (float)(y2 + 1));
  if (style != IUP_DRAW_FILL)
  {
    float offset = (line_width % 2) ? 0.5f : 0.0f;
    rect = D2D1::RectF(x1 + offset, y1 + offset, x2 + offset, y2 + offset);
  }

  if (style == IUP_DRAW_FILL)
  {
    dc->d2dContext->FillRectangle(rect, dc->solidBrush.get());
  }
  else
  {
    auto strokeStyle = winuiDrawStrokeStyle(dc, style, line_width);
    dc->d2dContext->DrawRectangle(rect, dc->solidBrush.get(), (float)line_width, strokeStyle);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
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
    else
    {
      auto strokeStyle = winuiDrawStrokeStyle(dc, style, line_width);
      dc->d2dContext->DrawEllipse(ellipse, dc->solidBrush.get(), (float)line_width, strokeStyle);
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
  else
  {
    auto strokeStyle = winuiDrawStrokeStyle(dc, style, line_width);
    dc->d2dContext->DrawGeometry(pathGeometry.get(), dc->solidBrush.get(), (float)line_width, strokeStyle);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
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
  else
  {
    auto strokeStyle = winuiDrawStrokeStyle(dc, style, line_width);
    dc->d2dContext->DrawEllipse(ellipse, dc->solidBrush.get(), (float)line_width, strokeStyle);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
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
  else
  {
    auto strokeStyle = winuiDrawStrokeStyle(dc, style, line_width);
    dc->d2dContext->DrawGeometry(pathGeometry.get(), dc->solidBrush.get(), (float)line_width, strokeStyle);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
{
  if (!dc || !dc->d2dContext)
    return;

  dc->solidBrush->SetColor(winuiDrawColor(color));
  dc->d2dContext->FillRectangle(D2D1::RectF((float)x, (float)y, (float)(x + 1), (float)(y + 1)), dc->solidBrush.get());
}

extern "C" IUP_SDK_API void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int radius, long color, int style, int line_width)
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
  else
  {
    auto strokeStyle = winuiDrawStrokeStyle(dc, style, line_width);
    dc->d2dContext->DrawRoundedRectangle(roundedRect, dc->solidBrush.get(), (float)line_width, strokeStyle);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
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
  else
  {
    auto strokeStyle = winuiDrawStrokeStyle(dc, style, line_width);
    dc->d2dContext->DrawGeometry(pathGeometry.get(), dc->solidBrush.get(), (float)line_width, strokeStyle);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawQuadraticBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, long color, int style, int line_width)
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
  else
  {
    auto strokeStyle = winuiDrawStrokeStyle(dc, style, line_width);
    dc->d2dContext->DrawGeometry(pathGeometry.get(), dc->solidBrush.get(), (float)line_width, strokeStyle);
  }
}

static bool winuiDrawAxisAligned(IdrawCanvas* dc)
{
  return dc->userTransform._12 == 0 && dc->userTransform._21 == 0;
}

static bool winuiDrawPushRectClip(IdrawCanvas* dc, const D2D1_RECT_F& rect)
{
  if (winuiDrawAxisAligned(dc))
  {
    dc->d2dContext->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    return false;
  }

  com_ptr<ID2D1RectangleGeometry> geometry;
  g_d2dFactory->CreateRectangleGeometry(rect, geometry.put());
  dc->d2dContext->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), geometry.get()), nullptr);
  return true;
}

static void winuiDrawPopRectClip(IdrawCanvas* dc, bool layer)
{
  if (layer)
    dc->d2dContext->PopLayer();
  else
    dc->d2dContext->PopAxisAlignedClip();
}

extern "C" IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  if (!dc || !dc->d2dContext || !text)
    return;

  dc->solidBrush->SetColor(winuiDrawColor(color));

  char typeface[256] = "";
  int size = 12;
  int is_bold = 0, is_italic = 0, is_underline = 0, is_strikeout = 0;
  float font_size;

  if (font)
  {
    iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout);

    const char* mapped_name = iupFontGetWinName(typeface);
    if (mapped_name)
      iupStrCopyN(typeface, sizeof(typeface), mapped_name);
  }

  font_size = (size < 0) ? (float)(-size) : iupWINUI_PT2PIXEL((float)size, winui_screen_dpi);
  if (font_size <= 0)
    return;

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
    font_size, L"", textFormat.put());

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

  if (flags & IUP_DRAW_ELLIPSIS)
  {
    DWRITE_TRIMMING trimming = { DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
    com_ptr<IDWriteInlineObject> ellipsisSign;
    if (SUCCEEDED(g_dwriteFactory->CreateEllipsisTrimmingSign(textFormat.get(), ellipsisSign.put())))
      textFormat->SetTrimming(&trimming, ellipsisSign.get());
  }

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

  int layout_w = w, layout_h = h;
  int layout_center = flags & IUP_DRAW_LAYOUTCENTER;

  if (text_orientation)
    iupDrawGetTextSize(dc->ih, text, len, &layout_w, &layout_h, 0);

  float flw = (layout_w > 0) ? (float)layout_w : (float)dc->w;
  float flh = (layout_h > 0) ? (float)layout_h : (float)dc->h;

  com_ptr<IDWriteTextLayout> textLayout;
  g_dwriteFactory->CreateGdiCompatibleTextLayout(wtext, wlen, textFormat.get(), flw, flh, 1.0f, nullptr, FALSE, textLayout.put());
  if (textLayout)
  {
    if (is_underline || is_strikeout)
    {
      DWRITE_TEXT_RANGE range = {0, (UINT32)wlen};
      if (is_underline)
        textLayout->SetUnderline(TRUE, range);
      if (is_strikeout)
        textLayout->SetStrikethrough(TRUE, range);
    }
  }

  D2D1_POINT_2F drawPoint = D2D1::Point2F(fx, fy);

  if (text_orientation != 0.0)
  {
    D2D1_MATRIX_3X2_F oldTransform;
    bool clipLayer = false;
    dc->d2dContext->GetTransform(&oldTransform);

    if (flags & IUP_DRAW_CLIP)
      clipLayer = winuiDrawPushRectClip(dc, D2D1::RectF(fx, fy, fx + fw, fy + fh));

    if (layout_center)
    {
      float tcx = fx + fw / 2.0f;
      float tcy = fy + fh / 2.0f;

      D2D1_MATRIX_3X2_F rotation = D2D1::Matrix3x2F::Rotation(
        (float)(-text_orientation), D2D1::Point2F(tcx, tcy));

      dc->d2dContext->SetTransform(rotation * oldTransform);

      drawPoint = D2D1::Point2F(tcx - flw / 2.0f, tcy - flh / 2.0f);
    }
    else
    {
      D2D1_MATRIX_3X2_F rotation = D2D1::Matrix3x2F::Rotation(
        (float)(-text_orientation), D2D1::Point2F(fx, fy));

      dc->d2dContext->SetTransform(rotation * oldTransform);
    }

    if (textLayout)
      dc->d2dContext->DrawTextLayout(drawPoint, textLayout.get(), dc->solidBrush.get());
    else
      dc->d2dContext->DrawText(wtext, wlen, textFormat.get(), D2D1::RectF(drawPoint.x, drawPoint.y, drawPoint.x + flw, drawPoint.y + flh), dc->solidBrush.get());

    dc->d2dContext->SetTransform(oldTransform);

    if (flags & IUP_DRAW_CLIP)
      winuiDrawPopRectClip(dc, clipLayer);
  }
  else
  {
    bool clipLayer = false;
    if (flags & IUP_DRAW_CLIP)
      clipLayer = winuiDrawPushRectClip(dc, D2D1::RectF(fx, fy, fx + fw, fy + fh));

    if (textLayout)
      dc->d2dContext->DrawTextLayout(drawPoint, textLayout.get(), dc->solidBrush.get());
    else
      dc->d2dContext->DrawText(wtext, wlen, textFormat.get(), D2D1::RectF(fx, fy, fx + fw, fy + fh), dc->solidBrush.get());

    if (flags & IUP_DRAW_CLIP)
      winuiDrawPopRectClip(dc, clipLayer);
  }

  free(wtext);
}

extern "C" IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, long tint, int opacity, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int quality)
{
  if (!dc || !dc->d2dContext || !name)
    return;

  void* handle = iupImageGetImageTint(name, dc->ih, make_inactive, bgcolor, tint);
  if (!handle)
    return;

  int img_w = 0, img_h = 0, img_bpp = 0;
  iupdrvImageGetInfo(handle, &img_w, &img_h, &img_bpp);
  if (img_w == 0 || img_h == 0)
    return;

  if (sw <= 0 || sh <= 0)
  {
    sx = 0;
    sy = 0;
    sw = img_w;
    sh = img_h;
  }
  if (w == -1 || w == 0) w = sw;
  if (h == -1 || h == 0) h = sh;

  WriteableBitmap wb = winuiGetBitmapFromHandle(handle);
  if (!wb)
    return;

  Windows::Storage::Streams::IBuffer pixelBuffer = wb.PixelBuffer();
  uint8_t* srcPixels = pixelBuffer.data();

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
  D2D1_RECT_F srcRect = D2D1::RectF((float)sx, (float)sy, (float)(sx + sw), (float)(sy + sh));
  dc->d2dContext->DrawBitmap(d2dBitmap.get(), destRect, opacity / 255.0f,
    quality == IUP_DRAW_IMAGE_NEAREST ? D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR : D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
    srcRect);
}

extern "C" IUP_SDK_API void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
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

  D2D1_RECT_F clipRect = D2D1::RectF((float)x1, (float)y1, (float)(x2 + 1), (float)(y2 + 1));
  dc->clipType = winuiDrawPushRectClip(dc, clipRect) ? WINUI_CLIP_LAYER : WINUI_CLIP_RECT;

  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
}

extern "C" IUP_SDK_API void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int radius)
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

extern "C" IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc)
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

extern "C" IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int* x1, int* y1, int* x2, int* y2)
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

extern "C" IUP_SDK_API void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
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

extern "C" IUP_SDK_API void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (!dc || !dc->d2dContext)
    return;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  iupdrvDrawRectangle(dc, x1, y1, x2, y2, iupDrawColor(0, 0, 0, 224), IUP_DRAW_STROKE_DOT, 1);
}

extern "C" IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, const long* colors, const float* offsets, int count)
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

  D2D1_GRADIENT_STOP stops[IUP_GRADIENT_MAX_STOPS];
  for (int i = 0; i < count; i++)
    stops[i] = { offsets[i], winuiDrawColor(colors[i]) };
  com_ptr<ID2D1GradientStopCollection> collection;
  dc->d2dContext->CreateGradientStopCollection(stops, count, collection.put());

  com_ptr<ID2D1LinearGradientBrush> brush;
  dc->d2dContext->CreateLinearGradientBrush(
    D2D1::LinearGradientBrushProperties(D2D1::Point2F(sx, sy), D2D1::Point2F(ex, ey)),
    collection.get(), brush.put());

  dc->d2dContext->FillRectangle(
    D2D1::RectF((float)x1, (float)y1, (float)x1 + w, (float)y1 + h),
    brush.get());
}

extern "C" IUP_SDK_API void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, const long* colors, const float* offsets, int count)
{
  if (!dc || !dc->d2dContext)
    return;

  D2D1_GRADIENT_STOP stops[IUP_GRADIENT_MAX_STOPS];
  for (int i = 0; i < count; i++)
    stops[i] = { offsets[i], winuiDrawColor(colors[i]) };
  com_ptr<ID2D1GradientStopCollection> collection;
  dc->d2dContext->CreateGradientStopCollection(stops, count, collection.put());

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

static com_ptr<ID2D1PathGeometry> winuiDrawBuildPathGeometry(const IupPathSeg* segs, int count, int rule)
{
  com_ptr<ID2D1PathGeometry> pathGeometry;
  g_d2dFactory->CreatePathGeometry(pathGeometry.put());
  if (!pathGeometry)
    return nullptr;

  com_ptr<ID2D1GeometrySink> sink;
  if (FAILED(pathGeometry->Open(sink.put())))
    return nullptr;

  sink->SetFillMode(rule == IUP_PATH_RULE_EVENODD ? D2D1_FILL_MODE_ALTERNATE : D2D1_FILL_MODE_WINDING);

  float sub_x = 0.0f, sub_y = 0.0f;
  bool in_figure = false;

  for (int i = 0; i < count; i++)
  {
    switch (segs[i].op)
    {
    case IUP_PATHSEG_MOVE_TO:
      if (in_figure)
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
      sink->BeginFigure(D2D1::Point2F((float)segs[i].x1, (float)segs[i].y1), D2D1_FIGURE_BEGIN_FILLED);
      sub_x = (float)segs[i].x1;
      sub_y = (float)segs[i].y1;
      in_figure = true;
      break;
    case IUP_PATHSEG_LINE_TO:
      if (!in_figure)
      {
        sink->BeginFigure(D2D1::Point2F(sub_x, sub_y), D2D1_FIGURE_BEGIN_FILLED);
        in_figure = true;
      }
      sink->AddLine(D2D1::Point2F((float)segs[i].x1, (float)segs[i].y1));
      break;
    case IUP_PATHSEG_CURVE_TO:
      if (!in_figure)
      {
        sink->BeginFigure(D2D1::Point2F(sub_x, sub_y), D2D1_FIGURE_BEGIN_FILLED);
        in_figure = true;
      }
      sink->AddBezier(D2D1::BezierSegment(
        D2D1::Point2F((float)segs[i].x1, (float)segs[i].y1),
        D2D1::Point2F((float)segs[i].x2, (float)segs[i].y2),
        D2D1::Point2F((float)segs[i].x3, (float)segs[i].y3)));
      break;
    case IUP_PATHSEG_QUAD_TO:
      if (!in_figure)
      {
        sink->BeginFigure(D2D1::Point2F(sub_x, sub_y), D2D1_FIGURE_BEGIN_FILLED);
        in_figure = true;
      }
      sink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(
        D2D1::Point2F((float)segs[i].x1, (float)segs[i].y1),
        D2D1::Point2F((float)segs[i].x2, (float)segs[i].y2)));
      break;
    case IUP_PATHSEG_ARC_TO:
    {
      double bez[24];
      int j, n = iupDrawPathArcToCurves(&segs[i], bez);
      if (!in_figure)
      {
        sink->BeginFigure(D2D1::Point2F(sub_x, sub_y), D2D1_FIGURE_BEGIN_FILLED);
        in_figure = true;
      }
      for (j = 0; j < n; j++)
        sink->AddBezier(D2D1::BezierSegment(
          D2D1::Point2F((float)bez[j * 6], (float)bez[j * 6 + 1]),
          D2D1::Point2F((float)bez[j * 6 + 2], (float)bez[j * 6 + 3]),
          D2D1::Point2F((float)bez[j * 6 + 4], (float)bez[j * 6 + 5])));
      break;
    }
    case IUP_PATHSEG_CLOSE:
      if (in_figure)
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
      in_figure = false;
      break;
    }
  }

  if (in_figure)
    sink->EndFigure(D2D1_FIGURE_END_OPEN);

  sink->Close();
  return pathGeometry;
}

static void winuiDrawFillWithSource(IdrawCanvas* dc, ID2D1Geometry* geometry, const IupDrawSource* src)
{
  if (src->type == IUP_SOURCE_SOLID)
  {
    dc->solidBrush->SetColor(winuiDrawColor(src->color));
    dc->d2dContext->FillGeometry(geometry, dc->solidBrush.get());
    return;
  }

  D2D1_GRADIENT_STOP stops[IUP_GRADIENT_MAX_STOPS];
  for (int i = 0; i < src->count; i++)
    stops[i] = { src->offsets[i], winuiDrawColor(src->colors[i]) };
  com_ptr<ID2D1GradientStopCollection> collection;
  dc->d2dContext->CreateGradientStopCollection(stops, src->count, collection.put());

  if (src->type == IUP_SOURCE_LINEAR_GRADIENT)
  {
    int gx1 = src->x1, gy1 = src->y1, gx2 = src->x2, gy2 = src->y2;
    iupDrawCheckSwapCoord(gx1, gx2);
    iupDrawCheckSwapCoord(gy1, gy2);

    float w = (float)(gx2 - gx1);
    float h = (float)(gy2 - gy1);
    float cx = gx1 + w / 2.0f;
    float cy = gy1 + h / 2.0f;
    float rad = src->angle * (float)IUP_DEG2RAD;
    float sx = cx - (w * cosf(rad)) / 2.0f;
    float sy = cy - (h * sinf(rad)) / 2.0f;
    float ex = cx + (w * cosf(rad)) / 2.0f;
    float ey = cy + (h * sinf(rad)) / 2.0f;

    com_ptr<ID2D1LinearGradientBrush> brush;
    dc->d2dContext->CreateLinearGradientBrush(
      D2D1::LinearGradientBrushProperties(D2D1::Point2F(sx, sy), D2D1::Point2F(ex, ey)),
      collection.get(), brush.put());
    dc->d2dContext->FillGeometry(geometry, brush.get());
  }
  else
  {
    com_ptr<ID2D1RadialGradientBrush> brush;
    dc->d2dContext->CreateRadialGradientBrush(
      D2D1::RadialGradientBrushProperties(
        D2D1::Point2F((float)src->cx, (float)src->cy),
        D2D1::Point2F(0.0f, 0.0f),
        (float)src->radius, (float)src->radius),
      collection.get(), brush.put());
    dc->d2dContext->FillGeometry(geometry, brush.get());
  }
}

static void winuiDrawStrokeWithSource(IdrawCanvas* dc, ID2D1Geometry* geometry, const IupDrawSource* src, int style, int line_width)
{
  ID2D1StrokeStyle* strokeStyle = winuiDrawStrokeStyle(dc, style, line_width);

  if (src->type == IUP_SOURCE_SOLID)
  {
    dc->solidBrush->SetColor(winuiDrawColor(src->color));
    dc->d2dContext->DrawGeometry(geometry, dc->solidBrush.get(), (float)line_width, strokeStyle);
    return;
  }

  D2D1_GRADIENT_STOP stops[IUP_GRADIENT_MAX_STOPS];
  for (int i = 0; i < src->count; i++)
    stops[i] = { src->offsets[i], winuiDrawColor(src->colors[i]) };
  com_ptr<ID2D1GradientStopCollection> collection;
  dc->d2dContext->CreateGradientStopCollection(stops, src->count, collection.put());

  if (src->type == IUP_SOURCE_LINEAR_GRADIENT)
  {
    int gx1 = src->x1, gy1 = src->y1, gx2 = src->x2, gy2 = src->y2;
    iupDrawCheckSwapCoord(gx1, gx2);
    iupDrawCheckSwapCoord(gy1, gy2);

    float w = (float)(gx2 - gx1);
    float h = (float)(gy2 - gy1);
    float cx = gx1 + w / 2.0f;
    float cy = gy1 + h / 2.0f;
    float rad = src->angle * (float)IUP_DEG2RAD;
    float sx = cx - (w * cosf(rad)) / 2.0f;
    float sy = cy - (h * sinf(rad)) / 2.0f;
    float ex = cx + (w * cosf(rad)) / 2.0f;
    float ey = cy + (h * sinf(rad)) / 2.0f;

    com_ptr<ID2D1LinearGradientBrush> brush;
    dc->d2dContext->CreateLinearGradientBrush(
      D2D1::LinearGradientBrushProperties(D2D1::Point2F(sx, sy), D2D1::Point2F(ex, ey)),
      collection.get(), brush.put());
    dc->d2dContext->DrawGeometry(geometry, brush.get(), (float)line_width, strokeStyle);
  }
  else
  {
    com_ptr<ID2D1RadialGradientBrush> brush;
    dc->d2dContext->CreateRadialGradientBrush(
      D2D1::RadialGradientBrushProperties(
        D2D1::Point2F((float)src->cx, (float)src->cy),
        D2D1::Point2F(0.0f, 0.0f),
        (float)src->radius, (float)src->radius),
      collection.get(), brush.put());
    dc->d2dContext->DrawGeometry(geometry, brush.get(), (float)line_width, strokeStyle);
  }
}

extern "C" IUP_SDK_API void iupdrvDrawPathFill(IdrawCanvas* dc, const IupPathSeg* segs, int count, const IupDrawSource* src, int rule)
{
  if (!dc || !dc->d2dContext)
    return;

  com_ptr<ID2D1PathGeometry> geometry = winuiDrawBuildPathGeometry(segs, count, rule);
  if (!geometry)
    return;

  winuiDrawFillWithSource(dc, geometry.get(), src);
}

extern "C" IUP_SDK_API void iupdrvDrawPathStroke(IdrawCanvas* dc, const IupPathSeg* segs, int count, const IupDrawSource* src, int style, int line_width)
{
  if (!dc || !dc->d2dContext)
    return;

  com_ptr<ID2D1PathGeometry> geometry = winuiDrawBuildPathGeometry(segs, count, IUP_PATH_RULE_WINDING);
  if (!geometry)
    return;

  winuiDrawStrokeWithSource(dc, geometry.get(), src, style, line_width);
}

extern "C" IUP_SDK_API void iupdrvDrawSetClipPath(IdrawCanvas* dc, const IupPathSeg* segs, int count, int rule)
{
  if (!dc || !dc->d2dContext)
    return;

  if (dc->clipType == WINUI_CLIP_RECT)
    dc->d2dContext->PopAxisAlignedClip();
  else if (dc->clipType == WINUI_CLIP_LAYER)
    dc->d2dContext->PopLayer();

  com_ptr<ID2D1PathGeometry> geometry = winuiDrawBuildPathGeometry(segs, count, rule);
  if (!geometry)
  {
    dc->clipType = WINUI_CLIP_NONE;
    return;
  }

  D2D1_LAYER_PARAMETERS layerParams = D2D1::LayerParameters(D2D1::InfiniteRect(), geometry.get());
  dc->d2dContext->PushLayer(layerParams, nullptr);

  dc->clipType = WINUI_CLIP_LAYER;

  int x1, y1, x2, y2;
  iupDrawPathGetBBox(segs, count, &x1, &y1, &x2, &y2);
  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
}

static void iD2DCopyBgraPremulToRgba(unsigned char* dst, const unsigned char* src, int w, int h, int dst_w, int src_pitch)
{
  for (int y = 0; y < h; y++)
  {
    const unsigned char* src_line = src + y * src_pitch;
    unsigned char* dst_line = dst + y * dst_w * 4;
    for (int x = 0; x < w; x++)
    {
      unsigned char b = src_line[x * 4 + 0];
      unsigned char g = src_line[x * 4 + 1];
      unsigned char r = src_line[x * 4 + 2];
      unsigned char a = src_line[x * 4 + 3];

      if (a != 0 && a != 255)
      {
        r = (unsigned char)((r * 255) / a);
        g = (unsigned char)((g * 255) / a);
        b = (unsigned char)((b * 255) / a);
      }

      dst_line[x * 4 + 0] = r;
      dst_line[x * 4 + 1] = g;
      dst_line[x * 4 + 2] = b;
      dst_line[x * 4 + 3] = a;
    }
  }
}

extern "C" IUP_SDK_API int iupdrvDrawGetImageData(IdrawCanvas* dc, unsigned char* data)
{
  if (!dc || !dc->d2dContext || dc->layers)
    return 0;

  dc->d2dContext->Flush();

  com_ptr<ID2D1Image> target;
  dc->d2dContext->GetTarget(target.put());
  if (!target)
    return 0;

  com_ptr<ID2D1Bitmap1> targetBitmap;
  target->QueryInterface(targetBitmap.put());
  if (!targetBitmap)
    return 0;

  D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
    D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

  com_ptr<ID2D1Bitmap1> stagingBitmap;
  HRESULT hr = dc->d2dContext->CreateBitmap(
    D2D1::SizeU(dc->w, dc->h), nullptr, 0, props, stagingBitmap.put());
  if (FAILED(hr))
    return 0;

  D2D1_POINT_2U destPoint = {0, 0};
  D2D1_RECT_U srcRect = {
    (UINT32)dc->drawOffset.x, (UINT32)dc->drawOffset.y,
    (UINT32)(dc->drawOffset.x + dc->w), (UINT32)(dc->drawOffset.y + dc->h)
  };

  hr = stagingBitmap->CopyFromBitmap(&destPoint, targetBitmap.get(), &srcRect);
  if (FAILED(hr))
    return 0;

  D2D1_MAPPED_RECT mapped;
  hr = stagingBitmap->Map(D2D1_MAP_OPTIONS_READ, &mapped);
  if (FAILED(hr))
    return 0;

  iD2DCopyBgraPremulToRgba(data, mapped.bits, dc->w, dc->h, dc->w, mapped.pitch);

  stagingBitmap->Unmap();
  return 1;
}

extern "C" IUP_SDK_API int iupdrvCanvasGetImageData(Ihandle* ih, unsigned char* data, int w, int h)
{
  ID2D1Bitmap1* buffer = (ID2D1Bitmap1*)iupAttribGet(ih, "_IUPWINUI_CANVAS_BUFFER");
  if (!buffer)
    return 0;

  D2D1_MAPPED_RECT mapped;
  HRESULT hr = buffer->Map(D2D1_MAP_OPTIONS_READ, &mapped);
  if (FAILED(hr))
    return 0;

  D2D1_SIZE_U size = buffer->GetPixelSize();
  int copyW = (w < (int)size.width) ? w : (int)size.width;
  int copyH = (h < (int)size.height) ? h : (int)size.height;

  iD2DCopyBgraPremulToRgba(data, mapped.bits, copyW, copyH, w, mapped.pitch);

  buffer->Unmap();
  return 1;
}
