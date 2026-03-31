/** \file
 * \brief Image Resource - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_RGB_Image.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_PNG_Image.H>

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drvinfo.h"
}

#include "iupfltk_drv.h"


/****************************************************************************
 * Image Data Extraction
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvImageGetData(void* handle, unsigned char* imgdata)
{
  Fl_RGB_Image* image = (Fl_RGB_Image*)handle;

  if (!image || !image->data() || !image->data()[0])
    return;

  int w = image->data_w();
  int h = image->data_h();
  int d = image->d();
  const unsigned char* src = (const unsigned char*)image->data()[0];
  int ld = image->ld();
  if (ld == 0) ld = w * d;

  int channels = (d >= 4) ? 4 : 3;
  int line_size = w * channels;

  for (int y = 0; y < h; y++)
  {
    const unsigned char* src_line = src + y * ld;
    unsigned char* dst_line = imgdata + y * line_size;

    for (int x = 0; x < w; x++)
    {
      dst_line[x * channels + 0] = src_line[x * d + 0];
      dst_line[x * channels + 1] = src_line[x * d + 1];
      dst_line[x * channels + 2] = src_line[x * d + 2];
      if (channels == 4)
        dst_line[x * channels + 3] = (d >= 4) ? src_line[x * d + 3] : 255;
    }
  }
}

extern "C" IUP_SDK_API void iupdrvImageGetRawData(void* handle, unsigned char* imgdata)
{
  Fl_RGB_Image* image = (Fl_RGB_Image*)handle;

  if (!image || !image->data() || !image->data()[0])
    return;

  int w = image->data_w();
  int h = image->data_h();
  int d = image->d();
  const unsigned char* src = (const unsigned char*)image->data()[0];
  int ld = image->ld();
  if (ld == 0) ld = w * d;

  size_t planesize = (size_t)w * h;
  unsigned char* r = imgdata;
  unsigned char* g = imgdata + planesize;
  unsigned char* b = imgdata + 2 * planesize;
  unsigned char* a = imgdata + 3 * planesize;

  for (int y = 0; y < h; y++)
  {
    int lineoffset = (h - 1 - y) * w;
    const unsigned char* src_line = src + y * ld;

    for (int x = 0; x < w; x++)
    {
      r[lineoffset + x] = src_line[x * d + 0];
      g[lineoffset + x] = src_line[x * d + 1];
      b[lineoffset + x] = src_line[x * d + 2];

      if (d >= 4)
        a[lineoffset + x] = src_line[x * d + 3];
    }
  }
}

/****************************************************************************
 * Image Creation from Raw Data
 ****************************************************************************/

extern "C" IUP_SDK_API void* iupdrvImageCreateImageRaw(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{
  int channels = (bpp == 32) ? 4 : 3;
  unsigned char* data = (unsigned char*)malloc((size_t)width * height * channels);

  if (!data)
    return NULL;

  if (bpp == 8)
  {
    for (int y = 0; y < height; y++)
    {
      int src_y = height - 1 - y;
      unsigned char* dst_line = data + y * width * channels;

      for (int x = 0; x < width; x++)
      {
        unsigned char index = imgdata[src_y * width + x];
        if (index < colors_count)
        {
          iupColor* c = &colors[index];
          dst_line[x * channels + 0] = c->r;
          dst_line[x * channels + 1] = c->g;
          dst_line[x * channels + 2] = c->b;
          if (channels == 4)
            dst_line[x * channels + 3] = c->a;
        }
      }
    }
  }
  else
  {
    size_t planesize = (size_t)width * height;
    unsigned char* r_plane = imgdata;
    unsigned char* g_plane = imgdata + planesize;
    unsigned char* b_plane = imgdata + 2 * planesize;
    unsigned char* a_plane = imgdata + 3 * planesize;

    for (int y = 0; y < height; y++)
    {
      int src_y = height - 1 - y;
      unsigned char* dst_line = data + y * width * channels;

      for (int x = 0; x < width; x++)
      {
        dst_line[x * channels + 0] = r_plane[src_y * width + x];
        dst_line[x * channels + 1] = g_plane[src_y * width + x];
        dst_line[x * channels + 2] = b_plane[src_y * width + x];
        if (channels == 4)
          dst_line[x * channels + 3] = a_plane[src_y * width + x];
      }
    }
  }

  Fl_RGB_Image* image = new Fl_RGB_Image(data, width, height, channels);
  image->alloc_array = 1;

  IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
  if (cb)
    cb(image, (char*)"Fl_RGB_Image");

  return image;
}

/****************************************************************************
 * Image Creation from IUP Image
 ****************************************************************************/

extern "C" IUP_SDK_API void* iupdrvImageCreateImage(Ihandle *ih, const char* bgcolor, int make_inactive)
{
  int bpp, colors_count = 0, has_alpha = 0;
  iupColor colors[256];
  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;
  int flat_alpha = iupAttribGetBoolean(ih, "FLAT_ALPHA");

  bpp = iupAttribGetInt(ih, "BPP");

  if (bpp == 8)
    has_alpha = iupImageInitColorTable(ih, colors, &colors_count);
  else if (bpp == 32)
    has_alpha = 1;

  int channels = has_alpha ? 4 : 3;

  unsigned char* data = (unsigned char*)malloc((size_t)ih->currentwidth * ih->currentheight * channels);
  if (!data)
    return NULL;

  unsigned char* imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");

  if (make_inactive || flat_alpha)
    iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

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

        iupImageColorMakeInactive(&(colors[i].r), &(colors[i].g), &(colors[i].b), bg_r, bg_g, bg_b);
      }
    }

    for (int y = 0; y < ih->currentheight; y++)
    {
      unsigned char* line_data = imgdata + y * ih->currentwidth;
      unsigned char* dst = data + y * ih->currentwidth * channels;

      for (int x = 0; x < ih->currentwidth; x++)
      {
        unsigned char index = line_data[x];
        iupColor* c = &colors[index];

        dst[x * channels + 0] = c->r;
        dst[x * channels + 1] = c->g;
        dst[x * channels + 2] = c->b;
        if (channels == 4)
          dst[x * channels + 3] = c->a;
      }
    }
  }
  else
  {
    int src_channels = (bpp == 32) ? 4 : 3;

    for (int y = 0; y < ih->currentheight; y++)
    {
      unsigned char* line_data = imgdata + y * ih->currentwidth * src_channels;
      unsigned char* dst = data + y * ih->currentwidth * channels;

      for (int x = 0; x < ih->currentwidth; x++)
      {
        unsigned char r = line_data[x * src_channels + 0];
        unsigned char g = line_data[x * src_channels + 1];
        unsigned char b = line_data[x * src_channels + 2];
        unsigned char a = (bpp == 32) ? line_data[x * src_channels + 3] : 255;

        if (flat_alpha && has_alpha && a != 255)
        {
          r = iupALPHABLEND(r, bg_r, a);
          g = iupALPHABLEND(g, bg_g, a);
          b = iupALPHABLEND(b, bg_b, a);
          a = 255;
        }

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

        dst[x * channels + 0] = r;
        dst[x * channels + 1] = g;
        dst[x * channels + 2] = b;
        if (channels == 4)
          dst[x * channels + 3] = a;
      }
    }
  }

  Fl_RGB_Image* image = new Fl_RGB_Image(data, ih->currentwidth, ih->currentheight, channels);
  image->alloc_array = 1;

  if (make_inactive || (has_alpha && flat_alpha))
    iupAttribSet(ih, "_IUP_BGCOLOR_DEPEND", "1");

  IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
  if (cb)
    cb(image, (char*)"Fl_RGB_Image");

  return image;
}

extern "C" IUP_SDK_API void* iupdrvImageCreateIcon(Ihandle *ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

extern "C" IUP_SDK_API void* iupdrvImageCreateCursor(Ihandle *ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

/****************************************************************************
 * Image Loading
 ****************************************************************************/

extern "C" IUP_SDK_API void* iupdrvImageLoad(const char* name, int type)
{
  if (!name)
    return NULL;

  (void)type;

  fl_register_images();

  Fl_Shared_Image* shared = Fl_Shared_Image::get(name);
  if (!shared)
    return NULL;

  Fl_RGB_Image* rgb = (Fl_RGB_Image*)shared;

  int w = rgb->data_w();
  int h = rgb->data_h();
  int d = rgb->d();

  if (d < 3 || !rgb->data() || !rgb->data()[0])
  {
    shared->release();
    return NULL;
  }

  const unsigned char* src = (const unsigned char*)rgb->data()[0];
  int ld = rgb->ld();
  if (ld == 0) ld = w * d;

  unsigned char* data = (unsigned char*)malloc((size_t)w * h * d);
  if (!data)
  {
    shared->release();
    return NULL;
  }

  for (int y = 0; y < h; y++)
    memcpy(data + y * w * d, src + y * ld, w * d);

  shared->release();

  Fl_RGB_Image* result = new Fl_RGB_Image(data, w, h, d);
  result->alloc_array = 1;

  IFvs cb = (IFvs)IupGetFunction("IMAGECREATE_CB");
  if (cb)
    cb(result, (char*)"Fl_RGB_Image");

  return result;
}

/****************************************************************************
 * Image Information
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvImageGetInfo(void* handle, int *w, int *h, int *bpp)
{
  Fl_RGB_Image* image = (Fl_RGB_Image*)handle;

  if (!image)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    if (bpp) *bpp = 0;
    return 0;
  }

  if (w) *w = image->data_w();
  if (h) *h = image->data_h();

  if (bpp)
  {
    int d = image->d();
    if (d >= 4)
      *bpp = iupImageNormBpp(32);
    else if (d == 3)
      *bpp = iupImageNormBpp(24);
    else
      *bpp = iupImageNormBpp(8);
  }

  return 1;
}

extern "C" IUP_SDK_API int iupdrvImageGetRawInfo(void* handle, int *w, int *h, int *bpp, iupColor* colors, int *colors_count)
{
  (void)colors;
  (void)colors_count;
  return iupdrvImageGetInfo(handle, w, h, bpp);
}

/****************************************************************************
 * Image Destruction
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvImageDestroy(void* handle, int type)
{
  if (!handle)
    return;

  const char* type_str = "Fl_RGB_Image";
  if (type == IUPIMAGE_CURSOR)
    type_str = "CURSOR";
  else if (type == IUPIMAGE_ICON)
    type_str = "ICON";

  IFvs cb = (IFvs)IupGetFunction("IMAGEDESTROY_CB");
  if (cb)
    cb(handle, (char*)type_str);

  Fl_RGB_Image* image = (Fl_RGB_Image*)handle;
  delete image;
}

/****************************************************************************
 * Image Save
 ****************************************************************************/

static void fltkBmpWriteU16(unsigned char* p, unsigned short v)
{
  p[0] = (unsigned char)(v & 0xFF);
  p[1] = (unsigned char)((v >> 8) & 0xFF);
}

static void fltkBmpWriteU32(unsigned char* p, unsigned int v)
{
  p[0] = (unsigned char)(v & 0xFF);
  p[1] = (unsigned char)((v >> 8) & 0xFF);
  p[2] = (unsigned char)((v >> 16) & 0xFF);
  p[3] = (unsigned char)((v >> 24) & 0xFF);
}

#define FLTK_BMP_FILE_HEADER_SIZE 14
#define FLTK_BMP_INFO_HEADER_SIZE 40

static unsigned char* fltkImageWriteBmp(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, int* out_size)
{
  int has_palette = (bpp == 8);
  int pixel_bytes = has_palette ? 1 : 3;
  int palette_size = has_palette ? colors_count * 4 : 0;

  int row_bytes = width * pixel_bytes;
  int pad = (4 - (row_bytes % 4)) % 4;
  row_bytes += pad;

  unsigned int data_offset = FLTK_BMP_FILE_HEADER_SIZE + FLTK_BMP_INFO_HEADER_SIZE + palette_size;
  unsigned int file_size = data_offset + row_bytes * height;

  unsigned char* buffer = (unsigned char*)malloc(file_size);
  if (!buffer) return NULL;

  memset(buffer, 0, data_offset);

  unsigned char* ptr = buffer;
  fltkBmpWriteU16(ptr + 0, 0x4D42);
  fltkBmpWriteU32(ptr + 2, file_size);
  fltkBmpWriteU32(ptr + 10, data_offset);

  ptr = buffer + FLTK_BMP_FILE_HEADER_SIZE;
  fltkBmpWriteU32(ptr + 0, FLTK_BMP_INFO_HEADER_SIZE);
  fltkBmpWriteU32(ptr + 4, (unsigned int)width);
  fltkBmpWriteU32(ptr + 8, (unsigned int)height);
  fltkBmpWriteU16(ptr + 12, 1);
  fltkBmpWriteU16(ptr + 14, has_palette ? 8 : 24);
  fltkBmpWriteU32(ptr + 20, row_bytes * height);
  if (has_palette)
    fltkBmpWriteU32(ptr + 32, colors_count);

  if (has_palette)
  {
    unsigned char* pal = buffer + FLTK_BMP_FILE_HEADER_SIZE + FLTK_BMP_INFO_HEADER_SIZE;
    for (int i = 0; i < colors_count; i++)
    {
      pal[i * 4]     = colors[i].b;
      pal[i * 4 + 1] = colors[i].g;
      pal[i * 4 + 2] = colors[i].r;
      pal[i * 4 + 3] = 0;
    }
  }

  ptr = buffer + data_offset;
  for (int y = height - 1; y >= 0; y--)
  {
    if (has_palette)
    {
      unsigned char* src = imgdata + y * width;
      memcpy(ptr, src, width);
      memset(ptr + width, 0, pad);
      ptr += width + pad;
    }
    else if (bpp == 24)
    {
      unsigned char* src = imgdata + y * width * 3;
      for (int x = 0; x < width; x++)
      {
        ptr[x * 3]     = src[x * 3 + 2];
        ptr[x * 3 + 1] = src[x * 3 + 1];
        ptr[x * 3 + 2] = src[x * 3];
      }
      memset(ptr + width * 3, 0, pad);
      ptr += width * 3 + pad;
    }
    else
    {
      unsigned char* src = imgdata + y * width * 4;
      for (int x = 0; x < width; x++)
      {
        ptr[x * 3]     = src[x * 4 + 2];
        ptr[x * 3 + 1] = src[x * 4 + 1];
        ptr[x * 3 + 2] = src[x * 4];
      }
      memset(ptr + width * 3, 0, pad);
      ptr += width * 3 + pad;
    }
  }

  *out_size = (int)file_size;
  return buffer;
}

static unsigned char* fltkImageToRGB(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, int* out_channels)
{
  int channels;
  unsigned char* data;

  if (bpp == 8)
  {
    channels = 3;
    data = (unsigned char*)malloc((size_t)width * height * 3);
    if (!data) return NULL;

    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
      {
        int idx = imgdata[y * width + x];
        if (idx < colors_count)
        {
          data[(y * width + x) * 3 + 0] = colors[idx].r;
          data[(y * width + x) * 3 + 1] = colors[idx].g;
          data[(y * width + x) * 3 + 2] = colors[idx].b;
        }
      }
    }
  }
  else
  {
    channels = (bpp == 32) ? 4 : 3;
    data = (unsigned char*)malloc((size_t)width * height * channels);
    if (!data) return NULL;
    memcpy(data, imgdata, (size_t)width * height * channels);
  }

  *out_channels = channels;
  return data;
}

extern "C" IUP_SDK_API int iupdrvImageSave(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* filename, const char* format)
{
  if (iupStrEqualNoCase(format, "PNG"))
  {
    int channels;
    unsigned char* data = fltkImageToRGB(imgdata, width, height, bpp, colors, colors_count, &channels);
    if (!data) return 0;

    int ret = fl_write_png(filename, data, width, height, channels);
    free(data);
    return (ret == 0) ? 1 : 0;
  }

  if (iupStrEqualNoCase(format, "BMP"))
  {
    int size;
    unsigned char* bmp_data = fltkImageWriteBmp(imgdata, width, height, bpp, colors, colors_count, &size);
    if (!bmp_data) return 0;

    FILE* f = fopen(filename, "wb");
    if (!f) { free(bmp_data); return 0; }

    int written = (int)fwrite(bmp_data, 1, size, f);
    fclose(f);
    free(bmp_data);
    return (written == size) ? 1 : 0;
  }

  return 0;
}

extern "C" IUP_SDK_API unsigned char* iupdrvImageSaveToBuffer(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* format, int* size)
{
  if (iupStrEqualNoCase(format, "BMP"))
    return fltkImageWriteBmp(imgdata, width, height, bpp, colors, colors_count, size);

  return NULL;
}

extern "C" IUP_SDK_API int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
{
  Fl_RGB_Image* image = NULL;

  (void)ih;

  if (!value)
    return 0;

  image = (Fl_RGB_Image*)iupImageGetIcon(value);
  if (!image || !image->data() || !image->data()[0])
    return 0;

  int w = image->data_w();
  int h = image->data_h();
  int d = image->d();
  const unsigned char* src = (const unsigned char*)image->data()[0];
  int ld = image->ld();
  if (ld == 0) ld = w * d;

  unsigned char* dst = (unsigned char*)malloc((size_t)w * h * 4);
  if (!dst)
    return 0;

  for (int y = 0; y < h; y++)
  {
    const unsigned char* src_line = src + y * ld;
    for (int x = 0; x < w; x++)
    {
      int offset = (y * w + x) * 4;

      dst[offset + 0] = (d >= 4) ? src_line[x * d + 3] : 255;
      dst[offset + 1] = src_line[x * d + 0];
      dst[offset + 2] = src_line[x * d + 1];
      dst[offset + 3] = src_line[x * d + 2];
    }
  }

  *width = w;
  *height = h;
  *pixels = dst;

  return 1;
}
