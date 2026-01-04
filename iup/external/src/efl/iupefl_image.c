/** \file
 * \brief Image Resource
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drvinfo.h"

#include "iupefl_drv.h"


void iupdrvImageGetData(void* handle, unsigned char* imgdata)
{
  Evas_Object* elm_img = (Evas_Object*)handle;
  Evas_Object* evas_img;
  int w, h, y;
  unsigned int* pixels;
  int stride;

  if (!iupdrvImageGetInfo(handle, &w, &h, NULL))
    return;

  evas_img = elm_image_object_get(elm_img);
  if (!evas_img)
    return;

  pixels = evas_object_image_data_get(evas_img, EINA_FALSE);
  if (!pixels)
    return;

  stride = evas_object_image_stride_get(evas_img);

  for (y = 0; y < h; y++)
  {
    unsigned char* line_data = imgdata + y * w * 4;
    unsigned int* pix_line = (unsigned int*)((unsigned char*)pixels + y * stride);

    memcpy(line_data, pix_line, w * 4);
  }

  evas_object_image_data_set(evas_img, pixels);
}

IUP_SDK_API void iupdrvImageGetRawData(void* handle, unsigned char* imgdata)
{
  Evas_Object* elm_img = (Evas_Object*)handle;
  Evas_Object* evas_img;
  int w, h, x, y, bpp;
  unsigned int* pixels;
  int stride, planesize;
  unsigned char *r, *g, *b, *a;

  if (!iupdrvImageGetInfo(handle, &w, &h, &bpp))
    return;

  if (bpp == 8)
    return;

  evas_img = elm_image_object_get(elm_img);
  if (!evas_img)
    return;

  pixels = evas_object_image_data_get(evas_img, EINA_FALSE);
  if (!pixels)
    return;

  stride = evas_object_image_stride_get(evas_img);

  planesize = w * h;
  r = imgdata;
  g = imgdata + planesize;
  b = imgdata + 2 * planesize;
  a = imgdata + 3 * planesize;

  for (y = 0; y < h; y++)
  {
    int lineoffset = (h - 1 - y) * w;
    unsigned int* pix_line = (unsigned int*)((unsigned char*)pixels + y * stride);

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

  evas_object_image_data_set(evas_img, pixels);
}

IUP_SDK_API void* iupdrvImageCreateImageRaw(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char* imgdata)
{
  Evas_Object* elm_img;
  Evas_Object* evas_img;
  unsigned int* pixels;
  int x, y;

  (void)colors_count;

  Evas_Object* win = iupeflGetMainWindow();
  if (!win)
    return NULL;

  elm_img = elm_image_add(win);
  if (!elm_img)
    return NULL;

  elm_image_aspect_fixed_set(elm_img, EINA_FALSE);
  elm_image_no_scale_set(elm_img, EINA_TRUE);

  evas_img = elm_image_object_get(elm_img);
  if (!evas_img)
  {
    evas_object_del(elm_img);
    return NULL;
  }

  evas_object_image_size_set(evas_img, width, height);
  evas_object_image_alpha_set(evas_img, (bpp == 32) ? EINA_TRUE : EINA_FALSE);

  pixels = evas_object_image_data_get(evas_img, EINA_TRUE);
  if (!pixels)
  {
    evas_object_del(elm_img);
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

  evas_object_image_data_set(evas_img, pixels);
  evas_object_image_data_update_add(evas_img, 0, 0, width, height);

  evas_object_size_hint_min_set(elm_img, width, height);
  evas_object_resize(elm_img, width, height);

  return elm_img;
}

void* iupdrvImageCreateImage(Ihandle* ih, const char* bgcolor, int make_inactive)
{
  Evas_Object* elm_img;
  Evas_Object* evas_img;
  unsigned int* pixels;
  unsigned char* imgdata;
  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;
  int x, y, bpp, colors_count = 0, has_alpha = 0;
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

  elm_img = elm_image_add(win);
  if (!elm_img)
    return NULL;

  elm_image_aspect_fixed_set(elm_img, EINA_FALSE);
  elm_image_no_scale_set(elm_img, EINA_TRUE);

  evas_img = elm_image_object_get(elm_img);
  if (!evas_img)
  {
    evas_object_del(elm_img);
    return NULL;
  }

  evas_object_image_size_set(evas_img, width, height);
  evas_object_image_alpha_set(evas_img, has_alpha ? EINA_TRUE : EINA_FALSE);

  pixels = evas_object_image_data_get(evas_img, EINA_TRUE);
  if (!pixels)
  {
    evas_object_del(elm_img);
    return NULL;
  }

  imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");

  if (make_inactive)
    iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

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

  evas_object_image_data_set(evas_img, pixels);
  evas_object_image_data_update_add(evas_img, 0, 0, width, height);

  evas_object_size_hint_min_set(elm_img, width, height);
  evas_object_resize(elm_img, width, height);

  return elm_img;
}

void* iupdrvImageCreateIcon(Ihandle* ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

void* iupdrvImageCreateCursor(Ihandle* ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

void iupdrvImageDestroy(void* handle, int type)
{
  Evas_Object* image = (Evas_Object*)handle;

  (void)type;

  if (image)
    evas_object_del(image);
}

int iupdrvImageGetInfo(void* handle, int* w, int* h, int* bpp)
{
  Evas_Object* elm_img = (Evas_Object*)handle;
  Evas_Object* evas_img;
  int width, height;
  Eina_Bool has_alpha;

  if (!elm_img)
    return 0;

  evas_img = elm_image_object_get(elm_img);
  if (!evas_img)
    return 0;

  evas_object_image_size_get(evas_img, &width, &height);
  has_alpha = evas_object_image_alpha_get(evas_img);

  if (w) *w = width;
  if (h) *h = height;
  if (bpp) *bpp = has_alpha ? 32 : 24;

  return 1;
}

IUP_SDK_API int iupdrvImageGetRawInfo(void* handle, int* w, int* h, int* bpp, iupColor* colors, int* colors_count)
{
  (void)colors;
  (void)colors_count;

  return iupdrvImageGetInfo(handle, w, h, bpp);
}

void* iupdrvImageLoad(const char* name, int type)
{
  Evas_Object* elm_img;
  Evas_Object* win;

  (void)type;

  win = iupeflGetMainWindow();
  if (!win)
    return NULL;

  elm_img = elm_image_add(win);
  if (!elm_img)
    return NULL;

  elm_image_aspect_fixed_set(elm_img, EINA_FALSE);
  elm_image_no_scale_set(elm_img, EINA_TRUE);

  if (!elm_image_file_set(elm_img, name, NULL))
  {
    evas_object_del(elm_img);
    return NULL;
  }

  return elm_img;
}

static Evas_Object* eflImageCreateFromIhandle(Ihandle* img_ih, Evas_Object* parent, int make_inactive, const char* bgcolor)
{
  Evas_Object* elm_img;
  Evas_Object* evas_img;
  unsigned int* pixels;
  unsigned char* imgdata;
  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;
  int x, y, bpp, colors_count = 0, has_alpha = 0;
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

  elm_img = elm_image_add(parent);
  if (!elm_img)
    return NULL;

  elm_image_preload_disabled_set(elm_img, EINA_TRUE);
  elm_image_no_scale_set(elm_img, EINA_TRUE);
  elm_image_resizable_set(elm_img, EINA_FALSE, EINA_FALSE);

  evas_img = elm_image_object_get(elm_img);
  if (!evas_img)
  {
    evas_object_del(elm_img);
    return NULL;
  }

  evas_object_image_size_set(evas_img, width, height);
  evas_object_image_alpha_set(evas_img, has_alpha ? EINA_TRUE : EINA_FALSE);

  pixels = evas_object_image_data_get(evas_img, EINA_TRUE);
  if (!pixels)
  {
    evas_object_del(elm_img);
    return NULL;
  }

  imgdata = (unsigned char*)iupAttribGetStr(img_ih, "WID");

  if (make_inactive)
    iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

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

  evas_object_image_data_set(evas_img, pixels);
  evas_object_image_data_update_add(evas_img, 0, 0, width, height);
  evas_object_image_filled_set(evas_img, EINA_TRUE);

  evas_object_size_hint_min_set(elm_img, width, height);
  evas_object_resize(elm_img, width, height);

  return elm_img;
}

static int eflImageUpdateFromIhandle(Evas_Object* elm_img, Ihandle* img_ih, int make_inactive, const char* bgcolor)
{
  Evas_Object* evas_img;
  unsigned int* pixels;
  unsigned char* imgdata;
  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;
  int x, y, bpp, colors_count = 0, has_alpha = 0;
  iupColor colors[256];
  int width = img_ih->currentwidth;
  int height = img_ih->currentheight;

  if (!elm_img)
    return 0;

  bpp = iupAttribGetInt(img_ih, "BPP");

  if (bpp == 8)
    has_alpha = iupImageInitColorTable(img_ih, colors, &colors_count);
  else if (bpp == 32)
    has_alpha = 1;

  evas_img = elm_image_object_get(elm_img);
  if (!evas_img)
    return 0;

  evas_object_image_size_set(evas_img, width, height);
  evas_object_image_alpha_set(evas_img, has_alpha ? EINA_TRUE : EINA_FALSE);

  pixels = evas_object_image_data_get(evas_img, EINA_TRUE);
  if (!pixels)
    return 0;

  imgdata = (unsigned char*)iupAttribGetStr(img_ih, "WID");

  if (make_inactive)
    iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

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

  evas_object_image_data_set(evas_img, pixels);
  evas_object_image_data_update_add(evas_img, 0, 0, width, height);
  evas_object_image_filled_set(evas_img, EINA_TRUE);

  evas_object_size_hint_min_set(elm_img, width, height);
  evas_object_resize(elm_img, width, height);

  return 1;
}

int iupeflImageUpdateImage(Evas_Object* elm_img, const char* name, Ihandle* ih_parent, int make_inactive)
{
  Ihandle* img_ih;
  const char* bgcolor = NULL;

  if (!elm_img || !name || !name[0])
    return 0;

  img_ih = iupImageGetImageFromName(name);
  if (!img_ih)
    return 0;

  if (ih_parent)
    bgcolor = IupGetAttribute(ih_parent, "BGCOLOR");

  return eflImageUpdateFromIhandle(elm_img, img_ih, make_inactive, bgcolor);
}

Evas_Object* iupeflImageGetImage(const char* name, Ihandle* ih_parent, int make_inactive)
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

Evas_Object* iupeflImageGetImageForParent(const char* name, Ihandle* ih_parent, int make_inactive, Evas_Object* parent)
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

void iupeflImageDestroy(Evas_Object* image)
{
  if (image)
    evas_object_del(image);
}
