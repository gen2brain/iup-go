/** \file
 * \brief Image Resource.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupgtk4_drv.h"


typedef struct _IupGtk4ScaledPaintable
{
  GObject parent;
  GdkPaintable* paintable;
  int width, height;
} IupGtk4ScaledPaintable;

typedef struct _IupGtk4ScaledPaintableClass
{
  GObjectClass parent_class;
} IupGtk4ScaledPaintableClass;

static void iup_gtk4_scaled_paintable_iface_init(GdkPaintableInterface* iface);

G_DEFINE_TYPE_WITH_CODE(IupGtk4ScaledPaintable, iup_gtk4_scaled_paintable, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(GDK_TYPE_PAINTABLE, iup_gtk4_scaled_paintable_iface_init))

static void iup_gtk4_scaled_paintable_snapshot(GdkPaintable* paintable, GdkSnapshot* snapshot, double width, double height)
{
  IupGtk4ScaledPaintable* self = (IupGtk4ScaledPaintable*)paintable;
  gdk_paintable_snapshot(self->paintable, snapshot, width, height);
}

static int iup_gtk4_scaled_paintable_get_intrinsic_width(GdkPaintable* paintable)
{
  return ((IupGtk4ScaledPaintable*)paintable)->width;
}

static int iup_gtk4_scaled_paintable_get_intrinsic_height(GdkPaintable* paintable)
{
  return ((IupGtk4ScaledPaintable*)paintable)->height;
}

static GdkPaintableFlags iup_gtk4_scaled_paintable_get_flags(GdkPaintable* paintable)
{
  (void)paintable;
  return GDK_PAINTABLE_STATIC_SIZE | GDK_PAINTABLE_STATIC_CONTENTS;
}

static void iup_gtk4_scaled_paintable_iface_init(GdkPaintableInterface* iface)
{
  iface->snapshot = iup_gtk4_scaled_paintable_snapshot;
  iface->get_intrinsic_width = iup_gtk4_scaled_paintable_get_intrinsic_width;
  iface->get_intrinsic_height = iup_gtk4_scaled_paintable_get_intrinsic_height;
  iface->get_flags = iup_gtk4_scaled_paintable_get_flags;
}

static void iup_gtk4_scaled_paintable_dispose(GObject* object)
{
  IupGtk4ScaledPaintable* self = (IupGtk4ScaledPaintable*)object;
  g_clear_object(&self->paintable);
  G_OBJECT_CLASS(iup_gtk4_scaled_paintable_parent_class)->dispose(object);
}

static void iup_gtk4_scaled_paintable_class_init(IupGtk4ScaledPaintableClass* klass)
{
  G_OBJECT_CLASS(klass)->dispose = iup_gtk4_scaled_paintable_dispose;
}

static void iup_gtk4_scaled_paintable_init(IupGtk4ScaledPaintable* self)
{
  (void)self;
}

IUP_DRV_API void iupgtk4TextureSetScale(GdkTexture* texture, double scale)
{
  if (texture && scale > 0 && scale != 1.0)
    g_object_set_data_full(G_OBJECT(texture), "_IUPGTK4_SCALE", g_memdup2(&scale, sizeof(double)), g_free);
}

IUP_DRV_API double iupgtk4ImageGetScale(void* image)
{
  double* scale = (image && GDK_IS_TEXTURE(image)) ? (double*)g_object_get_data(G_OBJECT(image), "_IUPGTK4_SCALE") : NULL;
  return scale ? *scale : 1.0;
}

IUP_DRV_API GdkPaintable* iupgtk4ImageNewPaintable(void* image)
{
  IupGtk4ScaledPaintable* wrapper;
  double scale = iupgtk4ImageGetScale(image);

  if (!image)
    return NULL;
  if (scale == 1.0)
    return GDK_PAINTABLE(g_object_ref(image));

  wrapper = g_object_new(iup_gtk4_scaled_paintable_get_type(), NULL);
  wrapper->paintable = GDK_PAINTABLE(g_object_ref(image));
  wrapper->width = iupRound(gdk_texture_get_width(GDK_TEXTURE(image)) / scale);
  wrapper->height = iupRound(gdk_texture_get_height(GDK_TEXTURE(image)) / scale);
  return GDK_PAINTABLE(wrapper);
}

IUP_DRV_API void iupgtk4ImageGetSize(void* image, int* w, int* h)
{
  double scale = iupgtk4ImageGetScale(image);
  GdkPaintable* paintable = image ? GDK_PAINTABLE(image) : NULL;
  if (w) *w = paintable ? iupRound(gdk_paintable_get_intrinsic_width(paintable) / scale) : 0;
  if (h) *h = paintable ? iupRound(gdk_paintable_get_intrinsic_height(paintable) / scale) : 0;
}

IUP_SDK_API void iupdrvImageGetData(void* handle, unsigned char* imgdata)
{
  GdkTexture* texture = (GdkTexture*)handle;
  int w, h, bpp;

  if (!iupdrvImageGetInfo(handle, &w, &h, &bpp))
    return;

  if (bpp == 8)
    return;

  {
    GdkTextureDownloader* downloader = gdk_texture_downloader_new(texture);
    gdk_texture_downloader_set_format(downloader, GDK_MEMORY_R8G8B8A8);
    gdk_texture_downloader_download_into(downloader, imgdata, (gsize)w * 4);
    gdk_texture_downloader_free(downloader);
  }
}


IUP_SDK_API void* iupdrvImageCreateImage(Ihandle* ih, const char* bgcolor, int make_inactive)
{
  GdkTexture* texture;
  guchar* pixdata;
  unsigned char* imgdata, *line_data, bg_r = 0, bg_g = 0, bg_b = 0;
  int x, y, bpp, colors_count = 0, has_alpha = 0;
  iupColor colors[256];
  GBytes* bytes;

  bpp = iupAttribGetInt(ih, "BPP");

  if (bpp == 8)
    has_alpha = iupImageInitColorTable(ih, colors, &colors_count);
  else if (bpp == 32)
    has_alpha = 1;

  pixdata = g_malloc((gsize)ih->currentwidth * ih->currentheight * 4);
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

  bytes = g_bytes_new_take(pixdata, (gsize)ih->currentwidth * ih->currentheight * 4);
  texture = gdk_memory_texture_new(ih->currentwidth, ih->currentheight, GDK_MEMORY_R8G8B8A8, bytes, (gsize)ih->currentwidth * 4);
  g_bytes_unref(bytes);

  iupgtk4TextureSetScale(texture, iupImageGetPixelScale(ih));

  return texture;
}

IUP_SDK_API void* iupdrvImageCreateIcon(Ihandle* ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

IUP_SDK_API void* iupdrvImageCreateCursor(Ihandle* ih)
{
  GdkCursor* cursor;
  GdkTexture* texture;
  int hx = 0, hy = 0;

  iupStrToIntInt(iupAttribGet(ih, "HOTSPOT"), &hx, &hy, ':');

  texture = iupdrvImageCreateImage(ih, NULL, 0);
  cursor = gdk_cursor_new_from_texture(texture, hx, hy, NULL);
  g_object_unref(texture);

  return cursor;
}

IUP_SDK_API void* iupdrvImageLoad(const char* name, int type)
{
  if (type == IUPIMAGE_CURSOR)
    return gdk_cursor_new_from_name(name, NULL);
  else /* IUPIMAGE_IMAGE or IUPIMAGE_ICON */
  {
    GdkTexture* texture = NULL;
    GtkIconTheme* icon_theme;
    GtkIconPaintable* paintable;
    int stock_size = iupImageStockGetSize();

    icon_theme = gtk_icon_theme_get_for_display(gdk_display_get_default());
    if (icon_theme && gtk_icon_theme_has_icon(icon_theme, name))
    {
      paintable = gtk_icon_theme_lookup_icon(icon_theme, name, NULL, stock_size, 1, GTK_TEXT_DIR_NONE, 0);
      if (paintable)
      {
        GFile* file = gtk_icon_paintable_get_file(paintable);
        if (file)
        {
          char* filename = g_file_get_path(file);
          if (filename)
          {
            texture = gdk_texture_new_from_filename(filename, NULL);
            g_free(filename);
          }
        }
        g_object_unref(paintable);
      }
    }

    if (!texture && iupStrEqualPartial(name, "gtk-"))
    {
      if (icon_theme && gtk_icon_theme_has_icon(icon_theme, name + 4))
      {
        paintable = gtk_icon_theme_lookup_icon(icon_theme, name + 4, NULL, stock_size, 1, GTK_TEXT_DIR_NONE, 0);
        if (paintable)
        {
          GFile* file = gtk_icon_paintable_get_file(paintable);
          if (file)
          {
            char* filename = g_file_get_path(file);
            if (filename)
            {
              texture = gdk_texture_new_from_filename(filename, NULL);
              g_free(filename);
            }
          }
          g_object_unref(paintable);
        }
      }

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
            if (icon_theme && gtk_icon_theme_has_icon(icon_theme, new_names[i]))
            {
              paintable = gtk_icon_theme_lookup_icon(icon_theme, new_names[i], NULL, stock_size, 1, GTK_TEXT_DIR_NONE, 0);
              if (paintable)
              {
                GFile* file = gtk_icon_paintable_get_file(paintable);
                if (file)
                {
                  char* filename = g_file_get_path(file);
                  if (filename)
                  {
                    texture = gdk_texture_new_from_filename(filename, NULL);
                    g_free(filename);
                  }
                }
                g_object_unref(paintable);
              }
            }
            break;
          }
        }
      }
    }

    if (!texture)
    {
      texture = gdk_texture_new_from_filename(iupgtk4StrConvertToSystem(name), NULL);
    }

    return texture;
  }
}

IUP_SDK_API int iupdrvImageGetInfo(void* handle, int* w, int* h, int* bpp)
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

IUP_SDK_API int iupdrvImageGetRawInfo(void* handle, int* w, int* h, int* bpp, iupColor* colors, int* colors_count)
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

static unsigned char* iGtk4ImageExpandPalette(unsigned char* imgdata, int width, int height, iupColor* colors, int colors_count)
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

static GdkPixbuf* iGtk4ImageCreatePixbuf(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count)
{
  unsigned char* data = imgdata;
  GdkPixbuf* pixbuf;
  gboolean has_alpha = FALSE;
  int channels = 3;

  if (bpp == 8)
  {
    data = iGtk4ImageExpandPalette(imgdata, width, height, colors, colors_count);
    if (!data) return NULL;
    has_alpha = TRUE;
    channels = 4;
  }
  else if (bpp == 32)
  {
    has_alpha = TRUE;
    channels = 4;
  }

  pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, has_alpha, 8, width, height);
  if (pixbuf)
  {
    guchar* pixdata = gdk_pixbuf_get_pixels(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int src_stride = width * channels;
    int y;

    for (y = 0; y < height; y++)
      memcpy(pixdata + y * rowstride, data + y * src_stride, src_stride);
  }

  if (bpp == 8)
    free(data);

  return pixbuf;
}

static const char* iGtk4ImageGetType(const char* format)
{
  if (iupStrEqualNoCase(format, "PNG"))  return "png";
  if (iupStrEqualNoCase(format, "JPEG")) return "jpeg";
  if (iupStrEqualNoCase(format, "BMP"))  return "bmp";
  return NULL;
}

IUP_SDK_API int iupdrvImageSave(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* filename, const char* format)
{
  GdkPixbuf* pixbuf;
  GError* error = NULL;
  gboolean ret;
  const char* type = iGtk4ImageGetType(format);
  if (!type) return 0;

  pixbuf = iGtk4ImageCreatePixbuf(imgdata, width, height, bpp, colors, colors_count);
  if (!pixbuf) return 0;

  if (iupStrEqualNoCase(format, "JPEG"))
  {
    char quality[16];
    const char* q = IupGetGlobal("IMAGESAVEQUALITY");
    if (!q) q = "85";
    iupStrCopyN(quality, sizeof(quality), q);
    ret = gdk_pixbuf_save(pixbuf, filename, type, &error, "quality", quality, NULL);
  }
  else
    ret = gdk_pixbuf_save(pixbuf, filename, type, &error, NULL);

  if (error) g_error_free(error);
  g_object_unref(pixbuf);
  return ret ? 1 : 0;
}

IUP_SDK_API unsigned char* iupdrvImageSaveToBuffer(unsigned char* imgdata, int width, int height, int bpp, iupColor* colors, int colors_count, const char* format, int* size)
{
  GdkPixbuf* pixbuf;
  GError* error = NULL;
  gboolean ret;
  gchar* buffer = NULL;
  gsize buffer_size = 0;
  unsigned char* result;
  const char* type = iGtk4ImageGetType(format);
  if (!type) return NULL;

  pixbuf = iGtk4ImageCreatePixbuf(imgdata, width, height, bpp, colors, colors_count);
  if (!pixbuf) return NULL;

  if (iupStrEqualNoCase(format, "JPEG"))
  {
    char quality[16];
    const char* q = IupGetGlobal("IMAGESAVEQUALITY");
    if (!q) q = "85";
    iupStrCopyN(quality, sizeof(quality), q);
    ret = gdk_pixbuf_save_to_buffer(pixbuf, &buffer, &buffer_size, type, &error, "quality", quality, NULL);
  }
  else
    ret = gdk_pixbuf_save_to_buffer(pixbuf, &buffer, &buffer_size, type, &error, NULL);

  g_object_unref(pixbuf);

  if (!ret || !buffer)
  {
    if (error) g_error_free(error);
    return NULL;
  }

  if (error) g_error_free(error);

  result = (unsigned char*)malloc(buffer_size);
  if (!result)
  {
    g_free(buffer);
    return NULL;
  }

  memcpy(result, buffer, buffer_size);
  g_free(buffer);

  *size = (int)buffer_size;
  return result;
}

IUP_SDK_API int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
{
  GdkTexture* texture;
  int w, h;
  guchar* src;
  unsigned char* dst;
  int x, y;

  (void)ih;

  if (!value)
    return 0;

  texture = (GdkTexture*)iupImageGetIcon(value);
  if (!texture)
    return 0;

  w = gdk_texture_get_width(texture);
  h = gdk_texture_get_height(texture);

  /* gdk_texture_download returns CAIRO_FORMAT_ARGB32, which is BGRA in memory on little-endian */
  src = (guchar*)g_malloc((gsize)w * h * 4);
  gdk_texture_download(texture, src, (gsize)w * 4);

  dst = (unsigned char*)malloc((size_t)w * h * 4);
  if (!dst)
  {
    g_free(src);
    return 0;
  }

  for (y = 0; y < h; y++)
  {
    for (x = 0; x < w; x++)
    {
      guchar* pixel = src + (y * w + x) * 4;
      int offset = (y * w + x) * 4;
      guchar r, g, b, a;

      /* Cairo ARGB32 format on little-endian = BGRA in memory */
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
      b = pixel[0];
      g = pixel[1];
      r = pixel[2];
      a = pixel[3];
#else
      /* Big-endian: ARGB in memory */
      a = pixel[0];
      r = pixel[1];
      g = pixel[2];
      b = pixel[3];
#endif

      /* Output in ARGB order for SNI */
      dst[offset + 0] = a;
      dst[offset + 1] = r;
      dst[offset + 2] = g;
      dst[offset + 3] = b;
    }
  }

  g_free(src);

  *width = w;
  *height = h;
  *pixels = dst;

  return 1;
}
