/** \file
 * \brief Image Resource for WinUI Driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drvinfo.h"
}

#include "iupwinui_drv.h"

#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Windows::Storage::Streams;


#define iupALPHAPRE(_src, _alpha) (((_src)*(_alpha))/255)

static void winuiImageSetPixelBGRA(uint8_t* pixels, int pos, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  pixels[pos + 0] = iupALPHAPRE(b, a);
  pixels[pos + 1] = iupALPHAPRE(g, a);
  pixels[pos + 2] = iupALPHAPRE(r, a);
  pixels[pos + 3] = a;
}

extern "C" void* iupdrvImageCreateImage(Ihandle *ih, const char* bgcolor, int make_inactive)
{
  int width = ih->currentwidth;
  int height = ih->currentheight;
  int bpp = iupAttribGetInt(ih, "BPP");
  int channels = iupAttribGetInt(ih, "CHANNELS");
  unsigned char* imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");
  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;
  iupColor colors[256];
  int colors_count = 0;
  int has_alpha = 0;

  if (!imgdata)
    return NULL;

  if (bgcolor)
    iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

  if (bpp == 8)
    has_alpha = iupImageInitColorTable(ih, colors, &colors_count);
  else if (bpp == 32)
    has_alpha = 1;

  WriteableBitmap bitmap(width, height);
  IBuffer buffer = bitmap.PixelBuffer();

  uint8_t* pixels = buffer.data();
  int stride = width * 4;

  if (bpp == 8)
  {
    if (make_inactive)
    {
      for (int i = 0; i < colors_count; i++)
      {
        if (colors[i].a == 0)
        {
          colors[i].r = bg_r;
          colors[i].g = bg_g;
          colors[i].b = bg_b;
          colors[i].a = 255;
        }
        iupImageColorMakeInactive(&colors[i].r, &colors[i].g, &colors[i].b, bg_r, bg_g, bg_b);
      }
    }

    for (int y = 0; y < height; y++)
    {
      unsigned char* line = imgdata + y * width;
      for (int x = 0; x < width; x++)
      {
        unsigned char index = line[x];
        iupColor* c = &colors[index];
        int pos = y * stride + x * 4;

        uint8_t a = has_alpha ? c->a : 255;
        winuiImageSetPixelBGRA(pixels, pos, c->r, c->g, c->b, a);
      }
    }
  }
  else
  {
    for (int y = 0; y < height; y++)
    {
      unsigned char* line = imgdata + y * width * channels;
      for (int x = 0; x < width; x++)
      {
        int src_pos = x * channels;
        int dst_pos = y * stride + x * 4;

        uint8_t r = line[src_pos];
        uint8_t g = line[src_pos + 1];
        uint8_t b = line[src_pos + 2];
        uint8_t a = (channels == 4) ? line[src_pos + 3] : 255;

        if (make_inactive)
        {
          if (has_alpha && a != 255)
          {
            r = iupALPHABLEND(r, bg_r, a);
            g = iupALPHABLEND(g, bg_g, a);
            b = iupALPHABLEND(b, bg_b, a);
            a = 255;
          }
          iupImageColorMakeInactive(&r, &g, &b, bg_r, bg_g, bg_b);
        }

        winuiImageSetPixelBGRA(pixels, dst_pos, r, g, b, a);
      }
    }
  }

  bitmap.Invalidate();

  void* handle = nullptr;
  winrt::copy_to_abi(bitmap, handle);
  return handle;
}

static HICON winuiImageCreateCursorIcon(Ihandle *ih, int is_cursor)
{
  int width = ih->currentwidth;
  int height = ih->currentheight;
  int bpp = iupAttribGetInt(ih, "BPP");
  int channels = iupAttribGetInt(ih, "CHANNELS");
  unsigned char* imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");
  iupColor colors[256];
  int colors_count = 0;
  int has_alpha = 0;

  if (!imgdata)
    return NULL;

  if (bpp == 8)
    has_alpha = iupImageInitColorTable(ih, colors, &colors_count);
  else if (bpp == 32)
    has_alpha = 1;

  BITMAPV5HEADER bi;
  ZeroMemory(&bi, sizeof(bi));
  bi.bV5Size = sizeof(bi);
  bi.bV5Width = width;
  bi.bV5Height = -height;
  bi.bV5Planes = 1;
  bi.bV5BitCount = 32;
  bi.bV5Compression = BI_BITFIELDS;
  bi.bV5RedMask = 0x00FF0000;
  bi.bV5GreenMask = 0x0000FF00;
  bi.bV5BlueMask = 0x000000FF;
  bi.bV5AlphaMask = 0xFF000000;

  uint8_t* pixels = NULL;
  HDC hdc = GetDC(NULL);
  HBITMAP hBitmap = CreateDIBSection(hdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&pixels, NULL, 0);
  ReleaseDC(NULL, hdc);

  if (!hBitmap)
    return NULL;

  if (bpp == 8)
  {
    for (int y = 0; y < height; y++)
    {
      unsigned char* line = imgdata + y * width;
      for (int x = 0; x < width; x++)
      {
        unsigned char index = line[x];
        iupColor* c = &colors[index];
        int pos = (y * width + x) * 4;
        uint8_t a = has_alpha ? c->a : 255;

        pixels[pos + 0] = c->b;
        pixels[pos + 1] = c->g;
        pixels[pos + 2] = c->r;
        pixels[pos + 3] = a;
      }
    }
  }
  else
  {
    for (int y = 0; y < height; y++)
    {
      unsigned char* line = imgdata + y * width * channels;
      for (int x = 0; x < width; x++)
      {
        int src_pos = x * channels;
        int dst_pos = (y * width + x) * 4;

        uint8_t r = line[src_pos];
        uint8_t g = line[src_pos + 1];
        uint8_t b = line[src_pos + 2];
        uint8_t a = (channels == 4) ? line[src_pos + 3] : 255;

        pixels[dst_pos + 0] = b;
        pixels[dst_pos + 1] = g;
        pixels[dst_pos + 2] = r;
        pixels[dst_pos + 3] = a;
      }
    }
  }

  HBITMAP hMask = CreateBitmap(width, height, 1, 1, NULL);
  if (!hMask)
  {
    DeleteObject(hBitmap);
    return NULL;
  }

  ICONINFO iconinfo;
  iconinfo.hbmMask = hMask;
  iconinfo.hbmColor = hBitmap;

  if (is_cursor)
  {
    int x = 0, y = 0;
    iupStrToIntInt(iupAttribGet(ih, "HOTSPOT"), &x, &y, ':');
    iconinfo.xHotspot = x;
    iconinfo.yHotspot = y;
    iconinfo.fIcon = FALSE;
  }
  else
  {
    iconinfo.xHotspot = 0;
    iconinfo.yHotspot = 0;
    iconinfo.fIcon = TRUE;
  }

  HICON icon = CreateIconIndirect(&iconinfo);

  DeleteObject(hBitmap);
  DeleteObject(hMask);

  return icon;
}

extern "C" void* iupdrvImageCreateIcon(Ihandle *ih)
{
  return (void*)winuiImageCreateCursorIcon(ih, 0);
}

extern "C" void* iupdrvImageCreateCursor(Ihandle *ih)
{
  return (void*)winuiImageCreateCursorIcon(ih, 1);
}

extern "C" void* iupdrvImageLoad(const char* name, int type)
{
  int iup2win[3] = {IMAGE_BITMAP, IMAGE_ICON, IMAGE_CURSOR};
  HANDLE hImage;

  hImage = LoadImageA(GetModuleHandle(NULL), name, iup2win[type], 0, 0, type == IUPIMAGE_IMAGE ? LR_CREATEDIBSECTION : 0);

  if (!hImage)
    hImage = LoadImageA(NULL, name, iup2win[type], 0, 0, LR_LOADFROMFILE | (type == IUPIMAGE_IMAGE ? LR_CREATEDIBSECTION : 0));

  return hImage;
}

WriteableBitmap winuiGetBitmapFromHandle(void* handle)
{
  if (!handle)
    return nullptr;

  Windows::Foundation::IInspectable obj{nullptr};
  winrt::copy_from_abi(obj, handle);
  return obj.try_as<WriteableBitmap>();
}

extern "C" int iupdrvImageGetInfo(void* handle, int *w, int *h, int *bpp)
{
  WriteableBitmap bitmap = winuiGetBitmapFromHandle(handle);
  if (!bitmap)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    if (bpp) *bpp = 0;
    return 0;
  }

  if (w) *w = bitmap.PixelWidth();
  if (h) *h = bitmap.PixelHeight();
  if (bpp) *bpp = 32;
  return 1;
}

extern "C" void iupdrvImageGetData(void* handle, unsigned char* imgdata)
{
  if (!handle || !imgdata)
    return;

  WriteableBitmap bitmap = winuiGetBitmapFromHandle(handle);
  if (!bitmap)
    return;

  int width = bitmap.PixelWidth();
  int height = bitmap.PixelHeight();

  IBuffer buffer = bitmap.PixelBuffer();
  uint8_t* pixels = buffer.data();

  int planesize = width * height;

  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      int src_pos = y * width * 4 + x * 4;
      int dst_pos = y * width + x;

      imgdata[dst_pos] = pixels[src_pos + 2];
      imgdata[dst_pos + planesize] = pixels[src_pos + 1];
      imgdata[dst_pos + 2*planesize] = pixels[src_pos];
    }
  }
}

extern "C" IUP_SDK_API void iupdrvImageGetRawData(void* handle, unsigned char* imgdata)
{
  if (!handle || !imgdata)
    return;

  WriteableBitmap bitmap = winuiGetBitmapFromHandle(handle);
  if (!bitmap)
    return;

  int width = bitmap.PixelWidth();
  int height = bitmap.PixelHeight();

  IBuffer buffer = bitmap.PixelBuffer();
  uint8_t* pixels = buffer.data();

  int planesize = width * height;
  unsigned char* r = imgdata;
  unsigned char* g = imgdata + planesize;
  unsigned char* b = imgdata + 2 * planesize;
  unsigned char* a = imgdata + 3 * planesize;

  for (int y = 0; y < height; y++)
  {
    int lineoffset = (height - 1 - y) * width;
    for (int x = 0; x < width; x++)
    {
      int src_pos = y * width * 4 + x * 4;
      r[lineoffset + x] = pixels[src_pos + 2];
      g[lineoffset + x] = pixels[src_pos + 1];
      b[lineoffset + x] = pixels[src_pos];
      a[lineoffset + x] = pixels[src_pos + 3];
    }
  }
}

extern "C" IUP_SDK_API void* iupdrvImageCreateImageRaw(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{
  (void)colors_count;

  WriteableBitmap bitmap(width, height);
  IBuffer buffer = bitmap.PixelBuffer();
  uint8_t* pixels = buffer.data();
  int stride = width * 4;

  if (bpp == 8)
  {
    for (int y = 0; y < height; y++)
    {
      int lineoffset = (height - 1 - y) * width;
      for (int x = 0; x < width; x++)
      {
        unsigned char index = imgdata[lineoffset + x];
        iupColor* c = &colors[index];
        int pos = y * stride + x * 4;

        winuiImageSetPixelBGRA(pixels, pos, c->r, c->g, c->b, c->a);
      }
    }
  }
  else
  {
    int planesize = width * height;
    unsigned char* r = imgdata;
    unsigned char* g = imgdata + planesize;
    unsigned char* b = imgdata + 2 * planesize;
    unsigned char* a = imgdata + 3 * planesize;

    for (int y = 0; y < height; y++)
    {
      int lineoffset = (height - 1 - y) * width;
      for (int x = 0; x < width; x++)
      {
        int pos = y * stride + x * 4;
        uint8_t alpha = (bpp == 32) ? a[lineoffset + x] : 255;

        winuiImageSetPixelBGRA(pixels, pos,
                               r[lineoffset + x],
                               g[lineoffset + x],
                               b[lineoffset + x],
                               alpha);
      }
    }
  }

  bitmap.Invalidate();

  void* handle = nullptr;
  winrt::copy_to_abi(bitmap, handle);
  return handle;
}

extern "C" IUP_SDK_API int iupdrvImageGetRawInfo(void* handle, int *w, int *h, int *bpp, iupColor* colors, int *colors_count)
{
  (void)colors;
  if (colors_count) *colors_count = 0;
  return iupdrvImageGetInfo(handle, w, h, bpp);
}

extern "C" IUP_SDK_API void iupdrvImageDestroy(void* handle, int type)
{
  if (!handle)
    return;

  switch (type)
  {
  case IUPIMAGE_ICON:
    DestroyIcon((HICON)handle);
    break;
  case IUPIMAGE_CURSOR:
    DestroyCursor((HCURSOR)handle);
    break;
  default:
  {
    Windows::Foundation::IInspectable obj{nullptr};
    winrt::attach_abi(obj, handle);
    break;
  }
  }
}
