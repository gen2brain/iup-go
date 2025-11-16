/** \file
 * \brief Image Resource.
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

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

#include "iupgtk4_drv.h"


void iupdrvImageGetData(void* handle, unsigned char* imgdata)
{
  GdkTexture* texture = (GdkTexture*)handle;
  int w, h, bpp;
  guchar *pixdata;
  int planesize;

  if (!iupdrvImageGetInfo(handle, &w, &h, &bpp))
    return;

  if (bpp == 8)
    return;

  /* Download texture data */
  planesize = w * h * 4;
  pixdata = g_malloc(planesize);
  gdk_texture_download(texture, pixdata, w * 4);

  /* Copy first plane (assuming RGBA format) */
  memcpy(imgdata, pixdata, w * h);

  g_free(pixdata);
}

IUP_SDK_API void iupdrvImageGetRawData(void* handle, unsigned char* imgdata)
{
  GdkTexture* texture = (GdkTexture*)handle;
  int w, h, y, x, bpp;
  guchar *pixdata;
  int planesize;
  unsigned char *r, *g, *b, *a;

  if (!iupdrvImageGetInfo(handle, &w, &h, &bpp))
    return;

  if (bpp == 8)
    return;

  /* Download texture data (always RGBA) */
  pixdata = g_malloc(w * h * 4);
  gdk_texture_download(texture, pixdata, w * 4);

  /* planes are separated in imgdata */
  planesize = w * h;
  r = imgdata;
  g = imgdata + planesize;
  b = imgdata + 2 * planesize;
  a = imgdata + 3 * planesize;

  for (y = 0; y < h; y++)
  {
    int lineoffset = (h - 1 - y) * w;  /* imgdata is bottom up */
    for (x = 0; x < w; x++)
    {
      int pos = (y * w + x) * 4;
      r[lineoffset + x] = pixdata[pos];
      g[lineoffset + x] = pixdata[pos + 1];
      b[lineoffset + x] = pixdata[pos + 2];

      if (bpp == 32)
        a[lineoffset + x] = pixdata[pos + 3];
    }
  }

  g_free(pixdata);
}

IUP_SDK_API void* iupdrvImageCreateImageRaw(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{
  GdkTexture* texture;
  guchar *pixdata;
  unsigned char *line_data;
  int x, y;
  GBytes *bytes;
  (void)colors_count;

  /* Create texture from memory */
  pixdata = g_malloc(width * height * 4);

  /* imgdata is bottom up, texture is top-bottom */

  if (bpp == 8)
  {
    for (y = 0; y < height; y++)
    {
      line_data = imgdata + (height - 1 - y) * width;  /* imgdata is bottom up */

      for (x = 0; x < width; x++)
      {
        unsigned char index = line_data[x];
        iupColor* c = &colors[index];
        int pos = (y * width + x) * 4;

        pixdata[pos] = c->r;
        pixdata[pos + 1] = c->g;
        pixdata[pos + 2] = c->b;
        pixdata[pos + 3] = c->a;
      }
    }
  }
  else /* bpp == 32 or bpp == 24 */
  {
    /* planes are separated in imgdata */
    int planesize = width * height;
    unsigned char *r = imgdata,
                  *g = imgdata + planesize,
                  *b = imgdata + 2 * planesize,
                  *a = imgdata + 3 * planesize;

    for (y = 0; y < height; y++)
    {
      int lineoffset = (height - 1 - y) * width;  /* imgdata is bottom up */
      for (x = 0; x < width; x++)
      {
        int pos = (y * width + x) * 4;
        pixdata[pos] = r[lineoffset + x];
        pixdata[pos + 1] = g[lineoffset + x];
        pixdata[pos + 2] = b[lineoffset + x];

        if (bpp == 32)
          pixdata[pos + 3] = a[lineoffset + x];
        else
          pixdata[pos + 3] = 255;
      }
    }
  }

  bytes = g_bytes_new_take(pixdata, width * height * 4);
  texture = gdk_memory_texture_new(width, height, GDK_MEMORY_R8G8B8A8, bytes, width * 4);
  g_bytes_unref(bytes);

  return texture;
}

void* iupdrvImageCreateImage(Ihandle *ih, const char* bgcolor, int make_inactive)
{
  GdkTexture* texture;
  guchar *pixdata;
  unsigned char *imgdata, *line_data, bg_r = 0, bg_g = 0, bg_b = 0;
  int x, y, bpp, colors_count = 0, has_alpha = 0;
  iupColor colors[256];
  GBytes *bytes;

  bpp = iupAttribGetInt(ih, "BPP");

  if (bpp == 8)
    has_alpha = iupImageInitColorTable(ih, colors, &colors_count);
  else if (bpp == 32)
    has_alpha = 1;

  pixdata = g_malloc(ih->currentwidth * ih->currentheight * 4);
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

        iupImageColorMakeInactive(&(colors[i].r), &(colors[i].g), &(colors[i].b), bg_r, bg_g, bg_b);
      }
    }

    for (y = 0; y < ih->currentheight; y++)
    {
      line_data = imgdata + y * ih->currentwidth;

      for (x = 0; x < ih->currentwidth; x++)
      {
        unsigned char index = line_data[x];
        iupColor* c = &colors[index];
        int pos = (y * ih->currentwidth + x) * 4;

        pixdata[pos] = c->r;
        pixdata[pos + 1] = c->g;
        pixdata[pos + 2] = c->b;
        pixdata[pos + 3] = has_alpha ? c->a : 255;
      }
    }
  }
  else /* bpp == 32 or bpp == 24 */
  {
    int channels = (bpp == 32) ? 4 : 3;

    for (y = 0; y < ih->currentheight; y++)
    {
      line_data = imgdata + y * ih->currentwidth * channels;

      for (x = 0; x < ih->currentwidth; x++)
      {
        int src_pos = x * channels;
        int dst_pos = (y * ih->currentwidth + x) * 4;
        guchar r = line_data[src_pos];
        guchar g = line_data[src_pos + 1];
        guchar b = line_data[src_pos + 2];
        guchar a = (bpp == 32) ? line_data[src_pos + 3] : 255;

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

        pixdata[dst_pos] = r;
        pixdata[dst_pos + 1] = g;
        pixdata[dst_pos + 2] = b;
        pixdata[dst_pos + 3] = a;
      }
    }
  }

  bytes = g_bytes_new_take(pixdata, ih->currentwidth * ih->currentheight * 4);
  texture = gdk_memory_texture_new(ih->currentwidth, ih->currentheight, GDK_MEMORY_R8G8B8A8, bytes, ih->currentwidth * 4);
  g_bytes_unref(bytes);

  return texture;
}

void* iupdrvImageCreateIcon(Ihandle *ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

void* iupdrvImageCreateCursor(Ihandle *ih)
{
  GdkCursor *cursor;
  GdkTexture* texture;
  int hx = 0, hy = 0;

  iupStrToIntInt(iupAttribGet(ih, "HOTSPOT"), &hx, &hy, ':');

  /* Always use texture-based cursor */
  texture = iupdrvImageCreateImage(ih, NULL, 0);
  cursor = gdk_cursor_new_from_texture(texture, hx, hy, NULL);
  g_object_unref(texture);

  return cursor;
}

void* iupdrvImageLoad(const char* name, int type)
{
  if (type == IUPIMAGE_CURSOR)
    return gdk_cursor_new_from_name(name, NULL);
  else /* IUPIMAGE_IMAGE or IUPIMAGE_ICON */
  {
    GdkTexture *texture = NULL;
    GtkIconTheme* icon_theme;
    GtkIconPaintable *paintable;
    int stock_size = iupImageStockGetSize();

    /* Use GtkIconTheme to load icon */
    icon_theme = gtk_icon_theme_get_for_display(gdk_display_get_default());
    if (icon_theme)
    {
      paintable = gtk_icon_theme_lookup_icon(icon_theme, name, NULL, stock_size, 1, GTK_TEXT_DIR_NONE, 0);
      if (paintable)
      {
        GFile *file = gtk_icon_paintable_get_file(paintable);
        if (file)
        {
          char *filename = g_file_get_path(file);
          if (filename)
          {
            texture = gdk_texture_new_from_filename(filename, NULL);
            g_free(filename);
          }
        }
        g_object_unref(paintable);
      }
    }

    /* Try removing "gtk-" prefix for stock icons */
    if (!texture && iupStrEqualPartial(name, "gtk-"))
    {
      paintable = gtk_icon_theme_lookup_icon(icon_theme, name + 4, NULL, stock_size, 1,
                                             GTK_TEXT_DIR_NONE, 0);
      if (paintable)
      {
        GFile *file = gtk_icon_paintable_get_file(paintable);
        if (file)
        {
          char *filename = g_file_get_path(file);
          if (filename)
          {
            texture = gdk_texture_new_from_filename(filename, NULL);
            g_free(filename);
          }
        }
        g_object_unref(paintable);
      }

      /* Try new names for old stock icons */
      if (!texture)
      {
#define IUP_GTK_STOCK_NAMES 6
        int i;
        const char* old_names[IUP_GTK_STOCK_NAMES] = { "find", "sort-ascending", "sort-descending", "zoom-fit", "zoom-100", "media-play-rtl" };
        const char* new_names[IUP_GTK_STOCK_NAMES] = { "edit-find", "view-sort-ascending", "view-sort-descending", "zoom-fit-best", "zoom-original", "media-playback-start-rtl" };

        for (i = 0; i < IUP_GTK_STOCK_NAMES; i++)
        {
          if (iupStrEqual(name + 4, old_names[i]))
          {
            paintable = gtk_icon_theme_lookup_icon(icon_theme, new_names[i], NULL, stock_size, 1,
                                                   GTK_TEXT_DIR_NONE, 0);
            if (paintable)
            {
              GFile *file = gtk_icon_paintable_get_file(paintable);
              if (file)
              {
                char *filename = g_file_get_path(file);
                if (filename)
                {
                  texture = gdk_texture_new_from_filename(filename, NULL);
                  g_free(filename);
                }
              }
              g_object_unref(paintable);
            }
            break;
          }
        }
      }
    }

    /* Try loading from file */
    if (!texture)
    {
      texture = gdk_texture_new_from_filename(iupgtk4StrConvertToSystem(name), NULL);
    }

    return texture;
  }
}

int iupdrvImageGetInfo(void* handle, int *w, int *h, int *bpp)
{
  GdkTexture* texture = (GdkTexture*)handle;
  if (!GDK_IS_TEXTURE(texture))
  {
    if (w) *w = 0;
    if (h) *h = 0;
    if (bpp) *bpp = 0;
    return 0;
  }
  if (w) *w = gdk_texture_get_width(texture);
  if (h) *h = gdk_texture_get_height(texture);
  if (bpp) *bpp = 32;
  return 1;
}

IUP_SDK_API int iupdrvImageGetRawInfo(void* handle, int *w, int *h, int *bpp, iupColor* colors, int *colors_count)
{
  /* GTK4 textures are always 32 bpp RGBA */
  (void)colors;
  (void)colors_count;
  return iupdrvImageGetInfo(handle, w, h, bpp);
}

IUP_SDK_API void iupdrvImageDestroy(void* handle, int type)
{
  g_object_unref(handle);
  (void)type;
}
