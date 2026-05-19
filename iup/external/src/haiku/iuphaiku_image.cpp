/** \file
 * \brief Haiku Image Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <Bitmap.h>
#include <BitmapStream.h>
#include <Cursor.h>
#include <DataIO.h>
#include <File.h>
#include <Path.h>
#include <Rect.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>

extern "C" {
#include "iup.h"
#include "iup_drv.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
}

#include "iuphaiku_drv.h"


/* BBitmap is always B_RGBA32 internally. Memory order is BGRA on little-endian; IUP source is RGBA, so the channel swap is byte 0<->2 per pixel. */
static BBitmap* haikuBuildBitmap(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char* imgdata,
                                 unsigned char bg_r, unsigned char bg_g, unsigned char bg_b, int make_inactive)
{
  if (width <= 0 || height <= 0 || !imgdata)
    return NULL;

  BBitmap* bm = new BBitmap(BRect(0, 0, width - 1, height - 1), 0, B_RGBA32);
  if (bm->InitCheck() != B_OK)
  {
    delete bm;
    return NULL;
  }

  uint8* dst = (uint8*)bm->Bits();
  int32 stride = bm->BytesPerRow();

  if (bpp == 8)
  {
    /* Optionally fold the bg color into transparent palette entries before inactive blend. */
    iupColor pal[256];
    if (colors_count > 256) colors_count = 256;
    memcpy(pal, colors, colors_count * sizeof(iupColor));

    if (make_inactive)
    {
      for (int i = 0; i < colors_count; ++i)
      {
        if (pal[i].a == 0) { pal[i].r = bg_r; pal[i].g = bg_g; pal[i].b = bg_b; pal[i].a = 255; }
        iupImageColorMakeInactive(&pal[i].r, &pal[i].g, &pal[i].b, bg_r, bg_g, bg_b);
      }
    }

    for (int y = 0; y < height; ++y)
    {
      uint8* line = dst + y * stride;
      unsigned char* src = imgdata + y * width;
      for (int x = 0; x < width; ++x)
      {
        unsigned char idx = src[x];
        iupColor* c = &pal[idx];
        line[x*4 + 0] = c->b;
        line[x*4 + 1] = c->g;
        line[x*4 + 2] = c->r;
        line[x*4 + 3] = c->a;
      }
    }
  }
  else /* 24 or 32 */
  {
    int channels = bpp / 8;
    for (int y = 0; y < height; ++y)
    {
      uint8* line = dst + y * stride;
      unsigned char* src = imgdata + y * width * channels;
      for (int x = 0; x < width; ++x)
      {
        unsigned char r = src[x*channels + 0];
        unsigned char g = src[x*channels + 1];
        unsigned char b = src[x*channels + 2];
        unsigned char a = (channels == 4) ? src[x*channels + 3] : 255;

        if (make_inactive)
        {
          if (a != 255)
          {
            r = iupALPHABLEND(r, bg_r, a);
            g = iupALPHABLEND(g, bg_g, a);
            b = iupALPHABLEND(b, bg_b, a);
            a = 255;
          }
          iupImageColorMakeInactive(&r, &g, &b, bg_r, bg_g, bg_b);
        }

        line[x*4 + 0] = b;
        line[x*4 + 1] = g;
        line[x*4 + 2] = r;
        line[x*4 + 3] = a;
      }
    }
  }

  return bm;
}

/* Driver hooks - create */

extern "C" IUP_SDK_API void* iupdrvImageCreateImage(Ihandle *ih, const char* bgcolor, int make_inactive)
{
  int bpp = iupAttribGetInt(ih, "BPP");
  int colors_count = 0;
  iupColor colors[256];

  if (bpp == 8)
    iupImageInitColorTable(ih, colors, &colors_count);

  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;
  if (make_inactive && bgcolor)
    iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

  unsigned char* imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");

  return haikuBuildBitmap(ih->currentwidth, ih->currentheight, bpp, colors, colors_count, imgdata, bg_r, bg_g, bg_b, make_inactive);
}

extern "C" IUP_SDK_API void* iupdrvImageCreateImageRaw(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{
  return haikuBuildBitmap(width, height, bpp, colors, colors_count, imgdata, 0, 0, 0, 0);
}

extern "C" IUP_SDK_API void* iupdrvImageCreateIcon(Ihandle *ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

extern "C" IUP_SDK_API void* iupdrvImageCreateCursor(Ihandle *ih)
{
  BBitmap* bm = (BBitmap*)iupdrvImageCreateImage(ih, NULL, 0);
  if (!bm) return NULL;

  int hx = 0, hy = 0;
  iupStrToIntInt(iupAttribGet(ih, "HOTSPOT"), &hx, &hy, ':');

  BCursor* cursor = new BCursor(bm, BPoint((float)hx, (float)hy));
  delete bm;
  return cursor;
}

extern "C" IUP_SDK_API void iupdrvImageDestroy(void* handle, int type)
{
  if (!handle) return;
  if (type == IUPIMAGE_CURSOR)
    delete (BCursor*)handle;
  else
    delete (BBitmap*)handle;
}

/* Driver hooks - introspection */

extern "C" IUP_SDK_API int iupdrvImageGetInfo(void* handle, int *w, int *h, int *bpp)
{
  BBitmap* bm = (BBitmap*)handle;
  if (!bm || bm->InitCheck() != B_OK)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    if (bpp) *bpp = 0;
    return 0;
  }
  BRect b = bm->Bounds();
  if (w) *w = (int)(b.IntegerWidth() + 1);
  if (h) *h = (int)(b.IntegerHeight() + 1);
  if (bpp) *bpp = 32;  /* always B_RGBA32 internally */
  return 1;
}

extern "C" IUP_SDK_API int iupdrvImageGetRawInfo(void* handle, int *w, int *h, int *bpp, iupColor* colors, int *colors_count)
{
  (void)colors;
  if (colors_count) *colors_count = 0;
  return iupdrvImageGetInfo(handle, w, h, bpp);
}

extern "C" IUP_SDK_API void iupdrvImageGetData(void* handle, unsigned char* imgdata)
{
  BBitmap* bm = (BBitmap*)handle;
  if (!bm || !imgdata || bm->InitCheck() != B_OK) return;

  BRect b = bm->Bounds();
  int w = b.IntegerWidth() + 1;
  int h = b.IntegerHeight() + 1;
  int32 stride = bm->BytesPerRow();
  uint8* src = (uint8*)bm->Bits();

  for (int y = 0; y < h; ++y)
  {
    uint8* line = src + y * stride;
    unsigned char* dst = imgdata + y * w * 4;
    for (int x = 0; x < w; ++x)
    {
      dst[x*4 + 0] = line[x*4 + 2];  /* R */
      dst[x*4 + 1] = line[x*4 + 1];  /* G */
      dst[x*4 + 2] = line[x*4 + 0];  /* B */
      dst[x*4 + 3] = line[x*4 + 3];  /* A */
    }
  }
}

extern "C" IUP_SDK_API void iupdrvImageGetRawData(void* handle, unsigned char* imgdata)
{
  iupdrvImageGetData(handle, imgdata);
}

/* Driver hooks - load */

extern "C" IUP_SDK_API void* iupdrvImageLoad(const char* name, int type)
{
  if (!name) return NULL;

  /* Try resource first, then file path. BTranslationUtils handles both PNG/JPEG/BMP/etc. */
  BBitmap* bm = BTranslationUtils::GetBitmap(name);
  if (!bm)
    bm = BTranslationUtils::GetBitmapFile(name);
  if (!bm)
    return NULL;

  if (type == IUPIMAGE_CURSOR)
  {
    BCursor* cursor = new BCursor(bm, BPoint(0, 0));
    delete bm;
    return cursor;
  }
  return bm;
}

/* Driver hooks - save (BTranslatorRoster) */

static uint32 haikuFormatId(const char* format)
{
  if (!format) return 0;
  if (iupStrEqualNoCase(format, "BMP"))  return B_BMP_FORMAT;
  if (iupStrEqualNoCase(format, "PNG"))  return B_PNG_FORMAT;
  if (iupStrEqualNoCase(format, "JPEG")) return B_JPEG_FORMAT;
  if (iupStrEqualNoCase(format, "JPG"))  return B_JPEG_FORMAT;
  if (iupStrEqualNoCase(format, "GIF"))  return B_GIF_FORMAT;
  if (iupStrEqualNoCase(format, "TIFF")) return B_TIFF_FORMAT;
  if (iupStrEqualNoCase(format, "TGA"))  return B_TGA_FORMAT;
  if (iupStrEqualNoCase(format, "PPM"))  return B_PPM_FORMAT;
  return 0;
}

static int haikuTranslate(BBitmap* bm, uint32 format_id, BPositionIO* out)
{
  BBitmapStream stream(bm);
  BTranslatorRoster* roster = BTranslatorRoster::Default();
  status_t st = roster->Translate(&stream, NULL, NULL, out, format_id);
  BBitmap* detached = NULL;
  stream.DetachBitmap(&detached);  /* prevent BBitmapStream from deleting bm */
  return (st == B_OK) ? 1 : 0;
}

extern "C" IUP_SDK_API int iupdrvImageSave(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count,
                                            const char* filename, const char* format)
{
  uint32 fid = haikuFormatId(format);
  if (!fid || !filename) return 0;

  BBitmap* bm = haikuBuildBitmap(width, height, bpp, colors, colors_count, imgdata, 0, 0, 0, 0);
  if (!bm) return 0;

  BFile file(filename, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
  int ok = 0;
  if (file.InitCheck() == B_OK)
    ok = haikuTranslate(bm, fid, &file);

  delete bm;
  return ok;
}

/* Driver hooks - icon pixels (used by dialog ICON setter) */

extern "C" IUP_SDK_API int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
{
  (void)ih;
  if (!value) return 0;

  BBitmap* bm = (BBitmap*)iupImageGetIcon(value);
  if (!bm || bm->InitCheck() != B_OK) return 0;

  BRect b = bm->Bounds();
  int w = b.IntegerWidth() + 1;
  int h = b.IntegerHeight() + 1;

  unsigned char* dst = (unsigned char*)malloc(w * h * 4);
  if (!dst) return 0;

  iupdrvImageGetData(bm, dst);

  if (width)  *width = w;
  if (height) *height = h;
  if (pixels) *pixels = dst;
  return 1;
}

extern "C" IUP_SDK_API unsigned char* iupdrvImageSaveToBuffer(unsigned char* imgdata, int width, int height, int bpp,
                                                              iupColor* colors, int colors_count, const char* format, int* size)
{
  if (size) *size = 0;
  uint32 fid = haikuFormatId(format);
  if (!fid) return NULL;

  BBitmap* bm = haikuBuildBitmap(width, height, bpp, colors, colors_count, imgdata, 0, 0, 0, 0);
  if (!bm) return NULL;

  BMallocIO mem;
  int ok = haikuTranslate(bm, fid, &mem);
  delete bm;
  if (!ok) return NULL;

  size_t n = mem.BufferLength();
  unsigned char* buf = (unsigned char*)malloc(n);
  if (!buf) return NULL;
  memcpy(buf, mem.Buffer(), n);
  if (size) *size = (int)n;
  return buf;
}
