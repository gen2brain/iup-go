/** \file
 * \brief EFL System Tray Driver - Icon Conversion for Unified SNI
 *
 * This file provides IUP image to ARGB pixel conversion
 * for the unified SNI implementation (iupunix_sni.c).
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_image.h"


int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
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

  dst = (unsigned char*)malloc(w * h * 4);
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
