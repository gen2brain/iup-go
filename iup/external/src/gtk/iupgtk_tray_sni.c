/** \file
 * \brief GTK System Tray Driver - Icon Conversion for Unified SNI
 *
 * This file provides the GdkPixbuf to ARGB pixel conversion
 * for the unified SNI implementation (iupunix_sni.c).
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>
#include <stdlib.h>

#include "iup.h"
#include "iup_image.h"

int iupdrvSNIGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
{
  GdkPixbuf* pixbuf;
  int w, h, rowstride, n_channels;
  guchar* src;
  gboolean has_alpha;
  int x, y;
  unsigned char* dst;

  (void)ih;

  if (!value)
    return 0;

  pixbuf = (GdkPixbuf*)iupImageGetIcon(value);
  if (!pixbuf)
    return 0;

  w = gdk_pixbuf_get_width(pixbuf);
  h = gdk_pixbuf_get_height(pixbuf);
  rowstride = gdk_pixbuf_get_rowstride(pixbuf);
  n_channels = gdk_pixbuf_get_n_channels(pixbuf);
  src = gdk_pixbuf_get_pixels(pixbuf);
  has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);

  dst = (unsigned char*)malloc(w * h * 4);
  if (!dst)
    return 0;

  for (y = 0; y < h; y++)
  {
    for (x = 0; x < w; x++)
    {
      guchar* pixel = src + y * rowstride + x * n_channels;
      int offset = (y * w + x) * 4;

      /* Convert to ARGB format */
      dst[offset + 0] = has_alpha ? pixel[3] : 0xff;  /* A */
      dst[offset + 1] = pixel[0];                      /* R */
      dst[offset + 2] = pixel[1];                      /* G */
      dst[offset + 3] = pixel[2];                      /* B */
    }
  }

  *width = w;
  *height = h;
  *pixels = dst;

  return 1;
}
