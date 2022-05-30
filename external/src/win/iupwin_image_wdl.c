/** \file
* \brief Draw Functions for DirectD2 using WinDrawLib
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
#include "iup_str.h"

#include "iupwin_drv.h"
#include "iupwin_info.h"
#include "iupwin_draw.h"
#include "iupwin_str.h"

#include "wdl.h"
#include "backend-d2d.h"
#include "backend-wic.h"
#include "backend-gdix.h"



static int wdlImageDestroy(Ihandle* ih)
{
  WD_HIMAGE hImage = (WD_HIMAGE)ih;
  wdDestroyImage(hImage);
  return 0;
}

void iupwinWdlImageInit(void)
{
  IupSetFunction("_IUPIMAGE_WD_IMAGEDESTROY", wdlImageDestroy);
}

static void wdBufferRGB2Bitmap(BYTE* Scan0, INT dstStride, INT srcStride, UINT channels, UINT width, UINT height, const BYTE *rgb, const char* bgcolor, int make_inactive)
{
  UINT i, j;
  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;

  iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

  for (j = 0; j < height; j++)
  {
    UINT line_offset = j * srcStride;
    const BYTE* rgb_line = rgb + line_offset;
    BYTE* line_data = Scan0 + j * dstStride;

    for (i = 0; i < width; i++)
    {
      int offset = i * 3;
      int offset_data = i * channels;
      unsigned char r = rgb_line[offset + 0];
      unsigned char g = rgb_line[offset + 1];
      unsigned char b = rgb_line[offset + 2];

      if (make_inactive)
        iupImageColorMakeInactive(&r, &g, &b, bg_r, bg_g, bg_b);

      line_data[offset_data + 0] = b;  /* Blue */
      line_data[offset_data + 1] = g;  /* Green */
      line_data[offset_data + 2] = r;  /* Red */

      if (channels == 4)
        line_data[offset_data + 3] = 255;
    }
  }
}

static void wdBufferRGBA2Bitmap(BYTE* Scan0, INT dstStride, INT srcStride, BOOL PreAlpha, UINT width, UINT height, const BYTE* rgba, const char* bgcolor, int make_inactive)
{
  UINT i, j;
  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;

  iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

  for (j = 0; j < height; j++)
  {
    UINT line_offset = j * srcStride;
    const BYTE* rgba_line = rgba + line_offset;
    BYTE* line_data = Scan0 + j * dstStride;

    for (i = 0; i < width; i++)
    {
      int offset = i * 4;
      unsigned char r = rgba_line[offset + 0];
      unsigned char g = rgba_line[offset + 1];
      unsigned char b = rgba_line[offset + 2];
      unsigned char a = rgba_line[offset + 3];

      if (make_inactive)
        iupImageColorMakeInactive(&r, &g, &b, bg_r, bg_g, bg_b);

      if (PreAlpha)
      {
        line_data[offset + 0] = (b * a) / 255;  /* Blue */
        line_data[offset + 1] = (g * a) / 255;  /* Green */
        line_data[offset + 2] = (r * a) / 255;  /* Red */
      }
      else
      {
        line_data[offset + 0] = b;  /* Blue */
        line_data[offset + 1] = g;  /* Green */
        line_data[offset + 2] = r;  /* Red */
      }

      line_data[offset + 3] = a;  /* Alpha */
    }
  }
}

static void wdBufferMap2Bitmap(BYTE* Scan0, INT dstStride, INT srcStride, UINT channels, UINT width, UINT height, const BYTE* map, const iupColor* palette)
{
  UINT i, j;

  for (j = 0; j < height; j++)
  {
    UINT line_offset = j * srcStride;
    const BYTE* map_line = map + line_offset;
    BYTE* line_data = Scan0 + j * dstStride;

    for (i = 0; i < width; i++)
    {
      int map_index = map_line[i];
      iupColor color = palette[map_index];

      int offset_data = i * channels;
      line_data[offset_data + 0] = color.b;
      line_data[offset_data + 1] = color.g;
      line_data[offset_data + 2] = color.r;

      if (channels == 4)
        line_data[offset_data + 3] = 255;
    }
  }
}

static WD_HIMAGE wdlCreateImageFromBuffer(UINT uWidth, UINT uHeight, UINT srcStride, const BYTE* pBuffer, int pixelFormat, const iupColor* cPalette, const char* bgcolor, int make_inactive)
{
  if (d2d_enabled()) {
    IWICBitmap* bitmap = NULL;
    HRESULT hr;
    WICRect rect;
    IWICBitmapLock *bitmap_lock = NULL;
    UINT cbBufferSize = 0;
    UINT dstStride = 0;
    BYTE *Scan0 = NULL;

    if (wic_factory == NULL) {
      WD_TRACE("wdlCreateImageFromBuffer: Image API disabled.");
      return NULL;
    }

    hr = IWICImagingFactory_CreateBitmap(wic_factory, uWidth, uHeight, &wic_pixel_format, WICBitmapCacheOnDemand, &bitmap);   /* GUID_WICPixelFormat32bppPBGRA - pre-multiplied alpha, BGRA order */
    if (FAILED(hr)) {
      WD_TRACE_HR("wdlCreateImageFromBuffer: "
                  "IWICImagingFactory::CreateBitmap() failed.");
      return NULL;
    }

    rect.X = 0;
    rect.Y = 0;
    rect.Width = uWidth;
    rect.Height = uHeight;

    hr = IWICBitmap_Lock(bitmap, &rect, WICBitmapLockWrite, &bitmap_lock);
    if (FAILED(hr)) {
      WD_TRACE_HR("wdlCreateImageFromBuffer: "
                  "IWICBitmap::Lock() failed.");
      IWICBitmap_Release(bitmap);
      return NULL;
    }

    IWICBitmapLock_GetStride(bitmap_lock, &dstStride);
    IWICBitmapLock_GetDataPointer(bitmap_lock, &cbBufferSize, &Scan0);

    if (pixelFormat == WD_PIXELFORMAT_PALETTE) {
      if (srcStride == 0) srcStride = uWidth * 1;
      wdBufferMap2Bitmap(Scan0, dstStride, srcStride, 4, uWidth, uHeight, pBuffer, cPalette);
    }
    else
    {
      if (pixelFormat == WD_PIXELFORMAT_R8G8B8) {
        if (srcStride == 0) srcStride = uWidth * 3;
        wdBufferRGB2Bitmap(Scan0, dstStride, srcStride, 4, uWidth, uHeight, pBuffer, bgcolor, make_inactive);
      }
      else if (pixelFormat == WD_PIXELFORMAT_R8G8B8A8) {
        if (srcStride == 0) srcStride = uWidth * 4;
        wdBufferRGBA2Bitmap(Scan0, dstStride, srcStride, TRUE, uWidth, uHeight, pBuffer, bgcolor, make_inactive);
      }
    }

    IWICBitmapLock_Release(bitmap_lock);
    return (WD_HIMAGE)bitmap;
  }
  else {
    dummy_GpPixelFormat format;
    int status;
    dummy_GpBitmap *bitmap = NULL;
    dummy_GpBitmapData bitmapData;
    dummy_GpRectI rect;

    if (pixelFormat == WD_PIXELFORMAT_R8G8B8 || pixelFormat == WD_PIXELFORMAT_PALETTE)
      format = dummy_PixelFormat24bppRGB;
    else if (pixelFormat == WD_PIXELFORMAT_R8G8B8A8)
      format = dummy_PixelFormat32bppARGB;
    else
      format = dummy_PixelFormat32bppPARGB;

    status = gdix_vtable->fn_CreateBitmapFromScan0(uWidth, uHeight, 0, format, NULL, &bitmap);
    if (status != 0) {
      WD_TRACE("wdlCreateImageFromBuffer: "
               "GdipCreateBitmapFromScan0() failed. [%d]", status);
      return NULL;
    }

    rect.x = 0;
    rect.y = 0;
    rect.w = uWidth;
    rect.h = uHeight;

    gdix_vtable->fn_BitmapLockBits(bitmap, &rect, dummy_ImageLockModeWrite, format, &bitmapData);

    if (pixelFormat == WD_PIXELFORMAT_PALETTE) {
      if (srcStride == 0) srcStride = uWidth * 1;
      wdBufferMap2Bitmap((BYTE*)bitmapData.Scan0, bitmapData.Stride, srcStride, 3, uWidth, uHeight, pBuffer, cPalette);
    }
    else
    {
      if (pixelFormat == WD_PIXELFORMAT_R8G8B8) {
        if (srcStride == 0) srcStride = uWidth * 3;
        wdBufferRGB2Bitmap((BYTE*)bitmapData.Scan0, bitmapData.Stride, srcStride, 3, uWidth, uHeight, pBuffer, bgcolor, make_inactive);
      }
      else if (pixelFormat == WD_PIXELFORMAT_R8G8B8A8) {
        if (srcStride == 0) srcStride = uWidth * 4;
        wdBufferRGBA2Bitmap((BYTE*)bitmapData.Scan0, bitmapData.Stride, srcStride, FALSE, uWidth, uHeight, pBuffer, bgcolor, make_inactive);
      }
    }

    gdix_vtable->fn_BitmapUnlockBits(bitmap, &bitmapData);

    return (WD_HIMAGE)bitmap;
  }
}

static WD_HIMAGE wdlImageLoad(const char* name)
{
  WD_HIMAGE hImage = wdLoadImageFromResource(iupwin_hinstance, RT_BITMAP, iupwinStrToSystem(name));
  if (!hImage && iupwin_dll_hinstance)
    hImage = wdLoadImageFromResource(iupwin_dll_hinstance, RT_BITMAP, iupwinStrToSystem(name));
  if (!hImage)
    hImage = wdLoadImageFromFile(iupwinStrToSystemFilename(name));
  return hImage;
}

static WD_HIMAGE wdlImageCreateImage(Ihandle *ih, const char* bgcolor, int make_inactive)
{
  int width, height, channels, bgcolor_depend = 0;
  unsigned char* data;
  WD_HIMAGE hImage = NULL;

  width = IupGetInt(ih, "WIDTH");
  height = IupGetInt(ih, "HEIGHT");
  channels = IupGetInt(ih, "CHANNELS");
  data = (unsigned char*)iupAttribGet(ih, "WID");

  if (channels == 4)
    hImage = wdlCreateImageFromBuffer(width, height, 0, data, WD_PIXELFORMAT_R8G8B8A8, NULL, bgcolor, make_inactive);
  else if (channels == 3)
    hImage = wdlCreateImageFromBuffer(width, height, 0, data, WD_PIXELFORMAT_R8G8B8, NULL, bgcolor, make_inactive);
  else
  {
    iupColor colors[256];
    int i, colors_count;
    unsigned char bg_r = 0, bg_g = 0, bg_b = 0;

    iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);
    iupImageInitColorTable(ih, colors, &colors_count);

    for (i = 0; i < colors_count; i++)
    {
      if (colors[i].a == 0) /* full transparent alpha */
      {
        colors[i].r = bg_r;
        colors[i].g = bg_g;
        colors[i].b = bg_b;
        bgcolor_depend = 1;
      }

      if (make_inactive)
        iupImageColorMakeInactive(&(colors[i].r), &(colors[i].g), &(colors[i].b), bg_r, bg_g, bg_b);
    }

    hImage = wdlCreateImageFromBuffer(width, height, 0, data, WD_PIXELFORMAT_PALETTE, colors, bgcolor, make_inactive);
  }

  if (make_inactive || bgcolor_depend)
    iupAttribSet(ih, "_IUP_BGCOLOR_DEPEND", "1");

  return hImage;
}

static void wdlImageSetHandleFromLoaded(const char* name, void* handle)
{
  Ihandle* ih;
  iupImageSetHandleFromLoaded(name, handle);
  ih = IupGetHandle(name);
  iupAttribSet(ih, "_IUPIMAGE_LOADED_HANDLE", NULL);
  iupAttribSet(ih, "_IUPIMAGE_LOADED_WD_HANDLE", (char*)handle);
}

WD_HIMAGE iupwinWdlImageGetImage(const char* name, Ihandle* ih_parent, int make_inactive, const char* bgcolor)
{
  char cache_name[100] = "_IUPIMAGE_WD_IMAGE";
  char* img_bgcolor;
  WD_HIMAGE handle;
  Ihandle *ih;
  int bg_concat = 0;

  if (!name)
    return NULL;

  ih = iupImageGetImageFromName(name);
  if (!ih)
  {
    const char* native_name = NULL;

    /* Check in the system resources. */
    handle = wdlImageLoad(name);
    if (handle)
    {
      wdlImageSetHandleFromLoaded(name, handle);  /* next time iupImageGetImageFromName will return the new handle */
      return handle;
    }

    /* Check in the stock images. */
    iupImageStockGet(name, &ih, &native_name);
    if (native_name)
    {
      handle = wdlImageLoad(native_name);
      if (handle)
      {
        wdlImageSetHandleFromLoaded(name, handle);  /* next time iupImageGetImageFromName will return the new handle */
        return handle;
      }
    }

    if (!ih)
      return NULL;
  }

  handle = (WD_HIMAGE)iupAttribGet(ih, "_IUPIMAGE_LOADED_WD_HANDLE");
  if (handle)
    return handle;

  img_bgcolor = iupAttribGet(ih, "BGCOLOR");
  if (ih_parent && !img_bgcolor)
  {
    if (!bgcolor)
      bgcolor = IupGetAttribute(ih_parent, "BGCOLOR"); /* Use IupGetAttribute to use inheritance and native implementation */
  }
  else
    bgcolor = img_bgcolor;

  if (make_inactive)
    strcat(cache_name, "_INACTIVE");

  if (iupAttribGet(ih, "_IUP_BGCOLOR_DEPEND") && bgcolor)
  {
    strcat(cache_name, "(");
    strcat(cache_name, bgcolor);
    strcat(cache_name, ")");
    bg_concat = 1;
  }

  /* Check for an already created native image */
  handle = (void*)iupAttribGet(ih, cache_name);
  if (handle)
    return handle;

  /* Creates the native image */
  handle = wdlImageCreateImage(ih, bgcolor, make_inactive);

  if (iupAttribGet(ih, "_IUP_BGCOLOR_DEPEND") && bgcolor && !bg_concat)  /* _IUP_BGCOLOR_DEPEND could be set during creation */
  {
    strcat(cache_name, "(");
    strcat(cache_name, bgcolor);
    strcat(cache_name, ")");
  }

  /* save the native image in the cache */
  iupAttribSet(ih, cache_name, (char*)handle);

  return handle;
}

