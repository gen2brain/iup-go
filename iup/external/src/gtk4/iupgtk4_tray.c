/** \file
 * \brief GTK4 System Tray Driver - Icon Conversion for Unified SNI
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>
#include <stdlib.h>

#include "iup.h"
#include "iup_image.h"

int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
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

  /* gdk_texture_download always returns CAIRO_FORMAT_ARGB32
   * which is native-endian ARGB, meaning BGRA in memory on little-endian systems */
  src = (guchar*)g_malloc(w * h * 4);
  gdk_texture_download(texture, src, w * 4);

  dst = (unsigned char*)malloc(w * h * 4);
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
