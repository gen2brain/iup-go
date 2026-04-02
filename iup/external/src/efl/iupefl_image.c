/** \file
 * \brief Image Resource
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <unistd.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupefl_drv.h"

static void eflImageSetPixels(Eo* img, unsigned int* pixels, int width, int height, Eina_Bool has_alpha)
{
  unsigned int* dst;

  evas_object_image_alpha_set(img, has_alpha);
  evas_object_image_size_set(img, width, height);

  dst = evas_object_image_data_get(img, EINA_TRUE);
  if (dst)
  {
    memcpy(dst, pixels, width * height * sizeof(unsigned int));
    evas_object_image_data_set(img, dst);
    evas_object_image_data_update_add(img, 0, 0, width, height);
  }

  evas_object_image_filled_set(img, EINA_TRUE);
  efl_gfx_hint_size_min_set(img, EINA_SIZE2D(width, height));
  efl_gfx_hint_size_max_set(img, EINA_SIZE2D(width, height));
  efl_gfx_entity_size_set(img, EINA_SIZE2D(width, height));
}

IUP_SDK_API void iupdrvImageGetData(void* handle, unsigned char* imgdata)
{
  Eo* img = (Eo*)handle;
  int w, h, bpp, x, y;
  int stride = 0;
  int channels;
  Eina_Rw_Slice mapped;

  if (!iupdrvImageGetInfo(handle, &w, &h, &bpp))
    return;

  mapped = efl_gfx_buffer_map(img, EFL_GFX_BUFFER_ACCESS_MODE_READ,
                               NULL, EFL_GFX_COLORSPACE_ARGB8888, 0, &stride);
  if (!mapped.mem)
    return;

  channels = bpp / 8;

  for (y = 0; y < h; y++)
  {
    unsigned char* line_data = imgdata + y * w * channels;
    unsigned int* pix_line = (unsigned int*)((unsigned char*)mapped.mem + y * stride);

    if (channels == 4)
    {
      memcpy(line_data, pix_line, w * 4);
    }
    else
    {
      for (x = 0; x < w; x++)
      {
        unsigned int pix = pix_line[x];
        line_data[x * 3]     = (pix >> 16) & 0xFF;
        line_data[x * 3 + 1] = (pix >> 8) & 0xFF;
        line_data[x * 3 + 2] = pix & 0xFF;
      }
    }
  }

  efl_gfx_buffer_unmap(img, mapped);
}

IUP_SDK_API void iupdrvImageGetRawData(void* handle, unsigned char* imgdata)
{
  Eo* img = (Eo*)handle;
  int w, h, x, y, bpp;
  int stride = 0;
  int planesize;
  unsigned char *r, *g, *b, *a;
  Eina_Rw_Slice mapped;

  if (!iupdrvImageGetInfo(handle, &w, &h, &bpp))
    return;

  if (bpp == 8)
    return;

  mapped = efl_gfx_buffer_map(img, EFL_GFX_BUFFER_ACCESS_MODE_READ,
                               NULL, EFL_GFX_COLORSPACE_ARGB8888, 0, &stride);
  if (!mapped.mem)
    return;

  planesize = w * h;
  r = imgdata;
  g = imgdata + planesize;
  b = imgdata + 2 * planesize;
  a = imgdata + 3 * planesize;

  for (y = 0; y < h; y++)
  {
    int lineoffset = (h - 1 - y) * w;
    unsigned int* pix_line = (unsigned int*)((unsigned char*)mapped.mem + y * stride);

    for (x = 0; x < w; x++)
    {
      unsigned int pix = pix_line[x];
      r[lineoffset + x] = (pix >> 16) & 0xFF;
      g[lineoffset + x] = (pix >> 8) & 0xFF;
      b[lineoffset + x] = pix & 0xFF;

      if (bpp == 32)
        a[lineoffset + x] = (pix >> 24) & 0xFF;
    }
  }

  efl_gfx_buffer_unmap(img, mapped);
}

IUP_SDK_API void* iupdrvImageCreateImageRaw(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char* imgdata)
{
  Eo* img;
  unsigned int* pixels;
  int x, y;

  (void)colors_count;

  Evas_Object* win = iupeflGetMainWindow();
  if (!win)
    return NULL;

  img = efl_add_ref(EFL_CANVAS_IMAGE_CLASS, win);
  if (!img)
    return NULL;
  efl_gfx_entity_visible_set(img, EINA_FALSE);

  pixels = (unsigned int*)malloc((size_t)width * height * sizeof(unsigned int));
  if (!pixels)
  {
    efl_unref(img);
    return NULL;
  }

  if (bpp == 8)
  {
    for (y = 0; y < height; y++)
    {
      unsigned char* line_data = imgdata + (height - 1 - y) * width;
      unsigned int* pix_line = pixels + y * width;

      for (x = 0; x < width; x++)
      {
        unsigned char index = line_data[x];
        iupColor* c = &colors[index];
        unsigned char pa = c->a;
        unsigned char pr = (c->r * pa) / 255;
        unsigned char pg = (c->g * pa) / 255;
        unsigned char pb = (c->b * pa) / 255;
        pix_line[x] = (pa << 24) | (pr << 16) | (pg << 8) | pb;
      }
    }
  }
  else
  {
    int planesize = width * height;
    unsigned char* r_plane = imgdata;
    unsigned char* g_plane = imgdata + planesize;
    unsigned char* b_plane = imgdata + 2 * planesize;
    unsigned char* a_plane = imgdata + 3 * planesize;

    for (y = 0; y < height; y++)
    {
      int lineoffset = (height - 1 - y) * width;
      unsigned int* pix_line = pixels + y * width;

      for (x = 0; x < width; x++)
      {
        unsigned char r = r_plane[lineoffset + x];
        unsigned char g = g_plane[lineoffset + x];
        unsigned char b = b_plane[lineoffset + x];
        unsigned char a = (bpp == 32) ? a_plane[lineoffset + x] : 255;
        unsigned char pr = (r * a) / 255;
        unsigned char pg = (g * a) / 255;
        unsigned char pb = (b * a) / 255;
        pix_line[x] = (a << 24) | (pr << 16) | (pg << 8) | pb;
      }
    }
  }

  eflImageSetPixels(img, pixels, width, height, (bpp == 32) ? EINA_TRUE : EINA_FALSE);
  free(pixels);

  return img;
}

static void eflImageFillPixels(unsigned int* pixels, unsigned char* imgdata, int width, int height, int bpp,
                               iupColor* colors, int colors_count, int has_alpha, int make_inactive,
                               unsigned char bg_r, unsigned char bg_g, unsigned char bg_b)
{
  int x, y;

  if (bpp == 8)
  {
    if (make_inactive)
    {
      int i;
      for (i = 0; i < colors_count; i++)
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

    for (y = 0; y < height; y++)
    {
      unsigned char* line_data = imgdata + y * width;
      unsigned int* pix_line = pixels + y * width;

      for (x = 0; x < width; x++)
      {
        unsigned char index = line_data[x];
        iupColor* c = &colors[index];
        unsigned char pa = c->a;
        unsigned char pr = (c->r * pa) / 255;
        unsigned char pg = (c->g * pa) / 255;
        unsigned char pb = (c->b * pa) / 255;
        pix_line[x] = (pa << 24) | (pr << 16) | (pg << 8) | pb;
      }
    }
  }
  else
  {
    int channels = (bpp == 32) ? 4 : 3;

    for (y = 0; y < height; y++)
    {
      unsigned char* line_data = imgdata + y * width * channels;
      unsigned int* pix_line = pixels + y * width;

      for (x = 0; x < width; x++)
      {
        unsigned char r = line_data[x * channels];
        unsigned char g = line_data[x * channels + 1];
        unsigned char b = line_data[x * channels + 2];
        unsigned char a = (bpp == 32) ? line_data[x * channels + 3] : 255;

        if (make_inactive)
        {
          if (a == 0 && has_alpha)
          {
            r = bg_r;
            g = bg_g;
            b = bg_b;
            a = 255;
          }
          iupImageColorMakeInactive(&r, &g, &b, bg_r, bg_g, bg_b);
        }

        {
          unsigned char pr = (r * a) / 255;
          unsigned char pg = (g * a) / 255;
          unsigned char pb = (b * a) / 255;
          pix_line[x] = (a << 24) | (pr << 16) | (pg << 8) | pb;
        }
      }
    }
  }
}

IUP_SDK_API void* iupdrvImageCreateImage(Ihandle* ih, const char* bgcolor, int make_inactive)
{
  Eo* img;
  unsigned int* pixels;
  unsigned char* imgdata;
  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;
  int bpp, colors_count = 0, has_alpha = 0;
  iupColor colors[256];
  int width = ih->currentwidth;
  int height = ih->currentheight;

  Evas_Object* win = iupeflGetMainWindow();
  if (!win)
    return NULL;

  bpp = iupAttribGetInt(ih, "BPP");

  if (bpp == 8)
    has_alpha = iupImageInitColorTable(ih, colors, &colors_count);
  else if (bpp == 32)
    has_alpha = 1;

  img = efl_add_ref(EFL_CANVAS_IMAGE_CLASS, win);
  if (!img)
    return NULL;
  efl_gfx_entity_visible_set(img, EINA_FALSE);

  pixels = (unsigned int*)malloc((size_t)width * height * sizeof(unsigned int));
  if (!pixels)
  {
    efl_unref(img);
    return NULL;
  }

  imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");

  if (make_inactive)
    iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

  eflImageFillPixels(pixels, imgdata, width, height, bpp, colors, colors_count, has_alpha, make_inactive, bg_r, bg_g, bg_b);

  eflImageSetPixels(img, pixels, width, height, has_alpha ? EINA_TRUE : EINA_FALSE);
  free(pixels);

  return img;
}

IUP_SDK_API void* iupdrvImageCreateIcon(Ihandle* ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

IUP_SDK_API void* iupdrvImageCreateCursor(Ihandle* ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

IUP_SDK_API void iupdrvImageDestroy(void* handle, int type)
{
  Eo* image = (Eo*)handle;

  (void)type;

  if (image)
  {
    if (efl_parent_get(image))
      efl_parent_set(image, NULL);
    efl_unref(image);
  }
}

IUP_SDK_API int iupdrvImageGetInfo(void* handle, int* w, int* h, int* bpp)
{
  Eo* img = (Eo*)handle;
  Eina_Size2D size;
  Eina_Bool has_alpha;

  if (!img)
    return 0;

  size = efl_gfx_buffer_size_get(img);
  has_alpha = efl_gfx_buffer_alpha_get(img);

  if (w) *w = size.w;
  if (h) *h = size.h;
  if (bpp) *bpp = has_alpha ? 32 : 24;

  return 1;
}

IUP_SDK_API int iupdrvImageGetRawInfo(void* handle, int* w, int* h, int* bpp, iupColor* colors, int* colors_count)
{
  (void)colors;
  (void)colors_count;

  return iupdrvImageGetInfo(handle, w, h, bpp);
}

IUP_SDK_API void* iupdrvImageLoad(const char* name, int type)
{
  Eo* img;
  Evas_Object* win;

  (void)type;

  win = iupeflGetMainWindow();
  if (!win)
    return NULL;

  img = efl_add_ref(EFL_CANVAS_IMAGE_CLASS, win);
  if (!img)
    return NULL;

  if (efl_file_set(img, name) || efl_file_load(img))
  {
    efl_unref(img);
    return NULL;
  }

  evas_object_image_filled_set(img, EINA_TRUE);
  efl_gfx_entity_visible_set(img, EINA_FALSE);

  return img;
}

static Eo* eflImageCreateFromIhandle(Ihandle* img_ih, Evas_Object* parent, int make_inactive, const char* bgcolor)
{
  Eo* img;
  unsigned int* pixels;
  unsigned char* imgdata;
  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;
  int bpp, colors_count = 0, has_alpha = 0;
  iupColor colors[256];
  int width = img_ih->currentwidth;
  int height = img_ih->currentheight;

  if (!parent)
    return NULL;

  bpp = iupAttribGetInt(img_ih, "BPP");

  if (bpp == 8)
    has_alpha = iupImageInitColorTable(img_ih, colors, &colors_count);
  else if (bpp == 32)
    has_alpha = 1;

  img = efl_add(EFL_CANVAS_IMAGE_CLASS, parent);
  if (!img)
    return NULL;
  efl_gfx_entity_visible_set(img, EINA_FALSE);

  pixels = (unsigned int*)malloc((size_t)width * height * sizeof(unsigned int));
  if (!pixels)
  {
    efl_del(img);
    return NULL;
  }

  imgdata = (unsigned char*)iupAttribGetStr(img_ih, "WID");

  if (make_inactive)
    iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

  eflImageFillPixels(pixels, imgdata, width, height, bpp, colors, colors_count, has_alpha, make_inactive, bg_r, bg_g, bg_b);

  eflImageSetPixels(img, pixels, width, height, has_alpha ? EINA_TRUE : EINA_FALSE);
  free(pixels);

  return img;
}

static int eflImageUpdateFromIhandle(Eo* img, Ihandle* img_ih, int make_inactive, const char* bgcolor)
{
  unsigned int* pixels;
  unsigned char* imgdata;
  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;
  int bpp, colors_count = 0, has_alpha = 0;
  iupColor colors[256];
  int width = img_ih->currentwidth;
  int height = img_ih->currentheight;

  if (!img || !efl_isa(img, EFL_CANVAS_IMAGE_INTERNAL_CLASS))
    return 0;

  bpp = iupAttribGetInt(img_ih, "BPP");

  if (bpp == 8)
    has_alpha = iupImageInitColorTable(img_ih, colors, &colors_count);
  else if (bpp == 32)
    has_alpha = 1;

  pixels = (unsigned int*)malloc((size_t)width * height * sizeof(unsigned int));
  if (!pixels)
    return 0;

  imgdata = (unsigned char*)iupAttribGetStr(img_ih, "WID");

  if (make_inactive)
    iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

  eflImageFillPixels(pixels, imgdata, width, height, bpp, colors, colors_count, has_alpha, make_inactive, bg_r, bg_g, bg_b);

  eflImageSetPixels(img, pixels, width, height, has_alpha ? EINA_TRUE : EINA_FALSE);
  free(pixels);

  return 1;
}

IUP_DRV_API int iupeflImageUpdateImage(Eo* img, const char* name, Ihandle* ih_parent, int make_inactive)
{
  Ihandle* img_ih;
  const char* bgcolor = NULL;

  if (!img || !name || !name[0])
    return 0;

  img_ih = iupImageGetImageFromName(name);
  if (!img_ih)
    return 0;

  if (ih_parent)
    bgcolor = IupGetAttribute(ih_parent, "BGCOLOR");

  return eflImageUpdateFromIhandle(img, img_ih, make_inactive, bgcolor);
}

IUP_DRV_API Eo* iupeflImageGetImage(const char* name, Ihandle* ih_parent, int make_inactive)
{
  Ihandle* img_ih;
  const char* bgcolor = NULL;

  if (!name || !name[0])
    return NULL;

  img_ih = iupImageGetImageFromName(name);
  if (!img_ih)
    return NULL;

  if (ih_parent)
    bgcolor = IupGetAttribute(ih_parent, "BGCOLOR");

  return eflImageCreateFromIhandle(img_ih, iupeflGetMainWindow(), make_inactive, bgcolor);
}

IUP_DRV_API Eo* iupeflImageGetImageForParent(const char* name, Ihandle* ih_parent, int make_inactive, Eo* parent)
{
  Ihandle* img_ih;
  const char* bgcolor = NULL;

  if (!name || !name[0])
    return NULL;

  img_ih = iupImageGetImageFromName(name);
  if (!img_ih)
    return NULL;

  if (ih_parent)
    bgcolor = IupGetAttribute(ih_parent, "BGCOLOR");

  return eflImageCreateFromIhandle(img_ih, parent, make_inactive, bgcolor);
}

static const char* iEflImageGetExtension(const char* format)
{
  if (iupStrEqualNoCase(format, "PNG"))  return ".png";
  if (iupStrEqualNoCase(format, "JPEG")) return ".jpg";
  if (iupStrEqualNoCase(format, "BMP"))  return ".bmp";
  return NULL;
}

static Eo* iEflImageCreateTemp(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count)
{
  Evas_Object* win = iupeflGetMainWindow();
  Eo* img;
  unsigned int* pixels;
  int x, y;
  size_t count;

  if (!win) return NULL;

  img = efl_add_ref(EFL_CANVAS_IMAGE_CLASS, win);
  if (!img) return NULL;

  efl_gfx_entity_visible_set(img, EINA_FALSE);

  count = (size_t)width * height;
  pixels = (unsigned int*)malloc(count * sizeof(unsigned int));
  if (!pixels)
  {
    efl_unref(img);
    return NULL;
  }

  if (bpp == 8)
  {
    (void)colors_count;
    for (y = 0; y < height; y++)
    {
      unsigned char* line_data = imgdata + y * width;
      unsigned int* pix_line = pixels + y * width;
      for (x = 0; x < width; x++)
      {
        unsigned char index = line_data[x];
        iupColor* c = &colors[index];
        unsigned char pa = c->a;
        unsigned char pr = (c->r * pa) / 255;
        unsigned char pg = (c->g * pa) / 255;
        unsigned char pb = (c->b * pa) / 255;
        pix_line[x] = (pa << 24) | (pr << 16) | (pg << 8) | pb;
      }
    }
  }
  else if (bpp == 32)
  {
    for (y = 0; y < height; y++)
    {
      unsigned char* src = imgdata + y * width * 4;
      unsigned int* pix_line = pixels + y * width;
      for (x = 0; x < width; x++)
      {
        unsigned char r = src[x * 4];
        unsigned char g = src[x * 4 + 1];
        unsigned char b = src[x * 4 + 2];
        unsigned char a = src[x * 4 + 3];
        unsigned char pr = (r * a) / 255;
        unsigned char pg = (g * a) / 255;
        unsigned char pb = (b * a) / 255;
        pix_line[x] = (a << 24) | (pr << 16) | (pg << 8) | pb;
      }
    }
  }
  else
  {
    for (y = 0; y < height; y++)
    {
      unsigned char* src = imgdata + y * width * 3;
      unsigned int* pix_line = pixels + y * width;
      for (x = 0; x < width; x++)
        pix_line[x] = (0xFF << 24) | (src[x * 3] << 16) | (src[x * 3 + 1] << 8) | src[x * 3 + 2];
    }
  }

  eflImageSetPixels(img, pixels, width, height, (bpp == 32 || bpp == 8) ? EINA_TRUE : EINA_FALSE);
  free(pixels);

  return img;
}

IUP_SDK_API int iupdrvImageSave(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* filename, const char* format)
{
  Eo* img;
  Eina_Bool ret;
  Efl_File_Save_Info info = {0};

  if (!iEflImageGetExtension(format)) return 0;

  if (iupStrEqualNoCase(format, "BMP"))
  {
    int size;
    unsigned char* bmp_data = iupImageWriteBMP(imgdata, width, height, bpp, colors, colors_count, &size);
    if (!bmp_data) return 0;
    FILE* f = fopen(filename, "wb");
    if (!f) { free(bmp_data); return 0; }
    int written = (int)fwrite(bmp_data, 1, size, f);
    fclose(f);
    free(bmp_data);
    return (written == size) ? 1 : 0;
  }

  img = iEflImageCreateTemp(imgdata, width, height, bpp, colors, colors_count);
  if (!img) return 0;

  if (iupStrEqualNoCase(format, "JPEG"))
  {
    const char* q = IupGetGlobal("IMAGESAVEQUALITY");
    info.quality = q ? (unsigned int)atoi(q) : 85;
  }
  else if (iupStrEqualNoCase(format, "PNG"))
    info.compression = 6;

  ret = efl_file_save(img, filename, NULL, &info);
  efl_unref(img);
  return ret ? 1 : 0;
}

IUP_SDK_API unsigned char* iupdrvImageSaveToBuffer(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* format, int* size)
{
  Eo* img;
  char tmpname[] = "/tmp/iup_img_XXXXXX";
  const char* ext;
  char tmppath[256];
  int fd;
  FILE* f;
  long fsize;
  unsigned char* result;
  Eina_Bool ret;
  Efl_File_Save_Info info = {0};

  ext = iEflImageGetExtension(format);
  if (!ext) return NULL;

  if (iupStrEqualNoCase(format, "BMP"))
    return iupImageWriteBMP(imgdata, width, height, bpp, colors, colors_count, size);

  img = iEflImageCreateTemp(imgdata, width, height, bpp, colors, colors_count);
  if (!img) return NULL;

  fd = mkstemp(tmpname);
  if (fd < 0) { efl_unref(img); return NULL; }
  close(fd);

  snprintf(tmppath, sizeof(tmppath), "%s%s", tmpname, ext);
  rename(tmpname, tmppath);

  if (iupStrEqualNoCase(format, "JPEG"))
  {
    const char* q = IupGetGlobal("IMAGESAVEQUALITY");
    info.quality = q ? (unsigned int)atoi(q) : 85;
  }
  else if (iupStrEqualNoCase(format, "PNG"))
    info.compression = 6;

  ret = efl_file_save(img, tmppath, NULL, &info);
  efl_unref(img);

  if (!ret) { unlink(tmppath); return NULL; }

  f = fopen(tmppath, "rb");
  if (!f) { unlink(tmppath); return NULL; }

  fseek(f, 0, SEEK_END);
  fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  result = (unsigned char*)malloc(fsize);
  if (!result) { fclose(f); unlink(tmppath); return NULL; }

  if ((long)fread(result, 1, fsize, f) != fsize)
  {
    free(result);
    fclose(f);
    unlink(tmppath);
    return NULL;
  }

  fclose(f);
  unlink(tmppath);

  *size = (int)fsize;
  return result;
}

IUP_SDK_API int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
{
  Ihandle* image;
  unsigned char* imgdata;
  unsigned char* dst;
  int w, h, channels, bpp;
  int x, y;

  (void)ih;

  if (!value)
    return 0;

  image = IupGetHandle(value);
  if (!image)
    return 0;

  imgdata = (unsigned char*)iupAttribGet(image, "WID");
  if (!imgdata)
    return 0;

  w = image->currentwidth;
  h = image->currentheight;
  channels = iupAttribGetInt(image, "CHANNELS");
  bpp = iupAttribGetInt(image, "BPP");

  if (w <= 0 || h <= 0)
    return 0;

  dst = (unsigned char*)malloc((size_t)w * h * 4);
  if (!dst)
    return 0;

  if (bpp == 32 && channels == 4)
  {
    for (y = 0; y < h; y++)
    {
      for (x = 0; x < w; x++)
      {
        int src_offset = (y * w + x) * 4;
        int dst_offset = (y * w + x) * 4;
        unsigned char r = imgdata[src_offset + 0];
        unsigned char g = imgdata[src_offset + 1];
        unsigned char b = imgdata[src_offset + 2];
        unsigned char a = imgdata[src_offset + 3];

        dst[dst_offset + 0] = a;
        dst[dst_offset + 1] = r;
        dst[dst_offset + 2] = g;
        dst[dst_offset + 3] = b;
      }
    }
  }
  else if (bpp == 24 && channels == 3)
  {
    for (y = 0; y < h; y++)
    {
      for (x = 0; x < w; x++)
      {
        int src_offset = (y * w + x) * 3;
        int dst_offset = (y * w + x) * 4;
        unsigned char r = imgdata[src_offset + 0];
        unsigned char g = imgdata[src_offset + 1];
        unsigned char b = imgdata[src_offset + 2];

        dst[dst_offset + 0] = 255;
        dst[dst_offset + 1] = r;
        dst[dst_offset + 2] = g;
        dst[dst_offset + 3] = b;
      }
    }
  }
  else
  {
    free(dst);
    return 0;
  }

  *width = w;
  *height = h;
  *pixels = dst;

  return 1;
}

