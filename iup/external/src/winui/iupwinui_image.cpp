/** \file
 * \brief Image Resource for WinUI Driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>

#include <wincodec.h>

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
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Windows::Foundation;
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

static void* winuiLoadImageFile(const char* name)
{
  std::wstring wname = iupwinuiStringToWString(name);

  HANDLE hFile = CreateFileW(wname.c_str(), GENERIC_READ, FILE_SHARE_READ,
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE)
    return nullptr;

  DWORD fileSize = GetFileSize(hFile, NULL);
  if (fileSize == INVALID_FILE_SIZE || fileSize == 0)
  {
    CloseHandle(hFile);
    return nullptr;
  }

  uint8_t* fileData = (uint8_t*)malloc(fileSize);
  if (!fileData)
  {
    CloseHandle(hFile);
    return nullptr;
  }

  DWORD bytesRead;
  BOOL readOk = ReadFile(hFile, fileData, fileSize, &bytesRead, NULL);
  CloseHandle(hFile);

  if (!readOk || bytesRead != fileSize)
  {
    free(fileData);
    return nullptr;
  }

  void* handle = nullptr;

  InMemoryRandomAccessStream stream;
  DataWriter writer(stream);
  writer.WriteBytes(winrt::array_view<uint8_t const>(fileData, fileData + fileSize));

  auto storeOp = writer.StoreAsync();
  if (storeOp.Status() == AsyncStatus::Error)
  {
    free(fileData);
    return nullptr;
  }
  storeOp.get();
  writer.DetachStream();
  free(fileData);

  stream.Seek(0);

  auto decoderOp = Windows::Graphics::Imaging::BitmapDecoder::CreateAsync(stream);
  if (decoderOp.Status() == AsyncStatus::Error)
    return nullptr;
  auto decoder = decoderOp.get();

  uint32_t width = decoder.PixelWidth();
  uint32_t height = decoder.PixelHeight();

  auto transform = Windows::Graphics::Imaging::BitmapTransform();
  auto pixelDataOp = decoder.GetPixelDataAsync(
    Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,
    Windows::Graphics::Imaging::BitmapAlphaMode::Premultiplied,
    transform,
    Windows::Graphics::Imaging::ExifOrientationMode::RespectExifOrientation,
    Windows::Graphics::Imaging::ColorManagementMode::ColorManageToSRgb);
  if (pixelDataOp.Status() == AsyncStatus::Error)
    return nullptr;
  auto pixelDataProvider = pixelDataOp.get();
  auto pixelArray = pixelDataProvider.DetachPixelData();

  WriteableBitmap bitmap(width, height);
  IBuffer pixelBuffer = bitmap.PixelBuffer();
  memcpy(pixelBuffer.data(), pixelArray.data(), (size_t)width * height * 4);
  bitmap.Invalidate();

  winrt::copy_to_abi(bitmap, handle);

  return handle;
}

extern "C" void* iupdrvImageLoad(const char* name, int type)
{
  if (type == IUPIMAGE_IMAGE)
    return winuiLoadImageFile(name);

  HANDLE hImage;

  hImage = LoadImageA(GetModuleHandle(NULL), name, type == IUPIMAGE_ICON ? IMAGE_ICON : IMAGE_CURSOR, 0, 0, 0);

  if (!hImage)
    hImage = LoadImageA(NULL, name, type == IUPIMAGE_ICON ? IMAGE_ICON : IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE);

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

  for (int y = 0; y < height; y++)
  {
    unsigned char* line_data = imgdata + y * width * 4;
    for (int x = 0; x < width; x++)
    {
      int src_pos = y * width * 4 + x * 4;

      line_data[0] = pixels[src_pos + 2];
      line_data[1] = pixels[src_pos + 1];
      line_data[2] = pixels[src_pos];
      line_data[3] = pixels[src_pos + 3];
      line_data += 4;
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

  size_t planesize = (size_t)width * height;
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

static IWICImagingFactory* iWinUIGetWicFactory(void)
{
  static IWICImagingFactory* factory = NULL;
  if (!factory)
  {
    CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
  }
  return factory;
}

static const GUID* iWinUIImageGetContainerFormat(const char* format)
{
  if (iupStrEqualNoCase(format, "PNG"))  return &GUID_ContainerFormatPng;
  if (iupStrEqualNoCase(format, "JPEG")) return &GUID_ContainerFormatJpeg;
  if (iupStrEqualNoCase(format, "BMP"))  return &GUID_ContainerFormatBmp;
  return NULL;
}

static unsigned char* iWinUIImageExpandPalette(unsigned char* imgdata, int width, int height, iupColor* colors, int colors_count)
{
  size_t count = (size_t)width * height;
  int i;
  unsigned char* rgba = (unsigned char*)malloc(count * 4);
  if (!rgba) return NULL;

  (void)colors_count;

  for (i = 0; i < count; i++)
  {
    int idx = imgdata[i];
    rgba[i * 4]     = colors[idx].r;
    rgba[i * 4 + 1] = colors[idx].g;
    rgba[i * 4 + 2] = colors[idx].b;
    rgba[i * 4 + 3] = colors[idx].a;
  }

  return rgba;
}

static int iWinUIImageSaveToStream(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* format, IStream* stream)
{
  IWICImagingFactory* factory = iWinUIGetWicFactory();
  IWICBitmapEncoder* encoder = NULL;
  IWICBitmapFrameEncode* frame = NULL;
  IPropertyBag2* props = NULL;
  const GUID* container;
  HRESULT hr;
  unsigned char* data = imgdata;
  unsigned char* pixbuf = NULL;
  int channels, dst_channels, stride, i;
  int is_jpeg = iupStrEqualNoCase(format, "JPEG");
  WICPixelFormatGUID pixelFormat;

  if (!factory) return 0;

  container = iWinUIImageGetContainerFormat(format);
  if (!container) return 0;

  if (bpp == 8)
  {
    data = iWinUIImageExpandPalette(imgdata, width, height, colors, colors_count);
    if (!data) return 0;
    bpp = 32;
  }

  channels = (bpp == 32) ? 4 : 3;
  dst_channels = is_jpeg ? 3 : 4;
  stride = width * dst_channels;

  if (is_jpeg)
    pixelFormat = GUID_WICPixelFormat24bppBGR;
  else
    pixelFormat = GUID_WICPixelFormat32bppBGRA;

  pixbuf = (unsigned char*)malloc((size_t)width * height * dst_channels);
  if (!pixbuf)
  {
    if (data != imgdata) free(data);
    return 0;
  }

  for (i = 0; i < width * height; i++)
  {
    pixbuf[i * dst_channels]     = data[i * channels + 2];
    pixbuf[i * dst_channels + 1] = data[i * channels + 1];
    pixbuf[i * dst_channels + 2] = data[i * channels];
    if (!is_jpeg)
      pixbuf[i * 4 + 3] = (channels == 4) ? data[i * channels + 3] : 255;
  }

  if (data != imgdata) free(data);

  hr = factory->CreateEncoder(*container, NULL, &encoder);
  if (FAILED(hr)) { free(pixbuf); return 0; }

  hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);
  if (FAILED(hr)) goto cleanup;

  hr = encoder->CreateNewFrame(&frame, &props);
  if (FAILED(hr)) goto cleanup;

  if (is_jpeg && props)
  {
    PROPBAG2 option = {};
    VARIANT varValue;
    const char* q = IupGetGlobal("IMAGESAVEQUALITY");
    float quality = 0.85f;
    if (q) quality = (float)atof(q) / 100.0f;

    option.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
    VariantInit(&varValue);
    varValue.vt = VT_R4;
    varValue.fltVal = quality;
    props->Write(1, &option, &varValue);
  }

  hr = frame->Initialize(props);
  if (FAILED(hr)) goto cleanup;

  hr = frame->SetSize(width, height);
  if (FAILED(hr)) goto cleanup;

  hr = frame->SetPixelFormat(&pixelFormat);
  if (FAILED(hr)) goto cleanup;

  hr = frame->WritePixels(height, stride, stride * height, pixbuf);
  if (FAILED(hr)) goto cleanup;

  hr = frame->Commit();
  if (FAILED(hr)) goto cleanup;

  hr = encoder->Commit();

cleanup:
  free(pixbuf);
  if (frame) frame->Release();
  if (props) props->Release();
  encoder->Release();

  return SUCCEEDED(hr) ? 1 : 0;
}

extern "C" int iupdrvImageSave(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* filename, const char* format)
{
  IStream* stream = NULL;
  HGLOBAL hGlobal;
  STATSTG stat;
  HANDLE hFile;
  HRESULT hr;
  int ret;
  void* pData;
  DWORD written;
  wchar_t wfilename[4096];

  hr = CreateStreamOnHGlobal(NULL, FALSE, &stream);
  if (FAILED(hr)) return 0;

  ret = iWinUIImageSaveToStream(imgdata, width, height, bpp, colors, colors_count, format, stream);
  if (!ret)
  {
    stream->Release();
    return 0;
  }

  hr = GetHGlobalFromStream(stream, &hGlobal);
  if (FAILED(hr))
  {
    stream->Release();
    return 0;
  }

  hr = stream->Stat(&stat, STATFLAG_NONAME);
  if (FAILED(hr))
  {
    stream->Release();
    GlobalFree(hGlobal);
    return 0;
  }

  MultiByteToWideChar(CP_UTF8, 0, filename, -1, wfilename, 4096);
  hFile = CreateFileW(wfilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE)
  {
    stream->Release();
    GlobalFree(hGlobal);
    return 0;
  }

  pData = GlobalLock(hGlobal);
  WriteFile(hFile, pData, stat.cbSize.LowPart, &written, NULL);
  GlobalUnlock(hGlobal);
  CloseHandle(hFile);

  stream->Release();
  GlobalFree(hGlobal);
  return (written == stat.cbSize.LowPart) ? 1 : 0;
}

extern "C" unsigned char* iupdrvImageSaveToBuffer(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* format, int* size)
{
  IStream* stream = NULL;
  HRESULT hr;
  HGLOBAL hGlobal;
  STATSTG stat;
  unsigned char* result;
  void* pData;

  hr = CreateStreamOnHGlobal(NULL, FALSE, &stream);
  if (FAILED(hr)) return NULL;

  if (!iWinUIImageSaveToStream(imgdata, width, height, bpp, colors, colors_count, format, stream))
  {
    stream->Release();
    return NULL;
  }

  hr = GetHGlobalFromStream(stream, &hGlobal);
  if (FAILED(hr))
  {
    stream->Release();
    return NULL;
  }

  hr = stream->Stat(&stat, STATFLAG_NONAME);
  if (FAILED(hr))
  {
    stream->Release();
    GlobalFree(hGlobal);
    return NULL;
  }

  *size = (int)stat.cbSize.LowPart;
  result = (unsigned char*)malloc(*size);
  if (!result)
  {
    stream->Release();
    GlobalFree(hGlobal);
    return NULL;
  }

  pData = GlobalLock(hGlobal);
  memcpy(result, pData, *size);
  GlobalUnlock(hGlobal);

  stream->Release();
  GlobalFree(hGlobal);
  return result;
}
