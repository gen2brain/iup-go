/** \file
 * \brief Motif System Tray Driver - Icon Conversion for Unified SNI
 *
 * See Copyright Notice in "iup.h"
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>

#include "iup.h"
#include "iup_image.h"

/* X11 globals from Motif driver */
extern Display* iupmot_display;
extern Visual* iupmot_visual;
extern int iupmot_screen;

/* Get mask pixmap for an image */
extern Pixmap iupmotImageGetMask(const char *name);

/* Helper to find highest set bit position */
static int sniHighBit(unsigned long ul)
{
  int i;
  unsigned long hb;
  hb = 0x80;
  hb = hb << 24;
  for (i = 31; ((ul & hb) == 0) && i >= 0; i--, ul <<= 1);
  return i;
}

/* Convert X11 pixel to RGB using visual's masks - works for TrueColor */
static void sniPixelToRGB(unsigned long pixel, unsigned char* r, unsigned char* g, unsigned char* b)
{
  int rshift, gshift, bshift;
  unsigned long rv, gv, bv;

  rshift = 15 - sniHighBit(iupmot_visual->red_mask);
  gshift = 15 - sniHighBit(iupmot_visual->green_mask);
  bshift = 15 - sniHighBit(iupmot_visual->blue_mask);

  rv = pixel & iupmot_visual->red_mask;
  gv = pixel & iupmot_visual->green_mask;
  bv = pixel & iupmot_visual->blue_mask;

  if (rshift < 0) rv = rv >> (-rshift);
  else rv = rv << rshift;
  if (gshift < 0) gv = gv >> (-gshift);
  else gv = gv << gshift;
  if (bshift < 0) bv = bv >> (-bshift);
  else bv = bv << bshift;

  *r = (unsigned char)(rv >> 8);
  *g = (unsigned char)(gv >> 8);
  *b = (unsigned char)(bv >> 8);
}

int iupdrvSNIGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
{
  Pixmap icon, mask;
  Window root;
  int x, y;
  unsigned int w, h, border, depth;
  XImage* image;
  XImage* mask_image = NULL;
  unsigned char* dst;
  int px, py;

  (void)ih;

  if (!value)
    return 0;

  icon = (Pixmap)iupImageGetIcon(value);
  if (!icon)
    return 0;

  mask = iupmotImageGetMask(value);

  if (!XGetGeometry(iupmot_display, icon, &root, &x, &y, &w, &h, &border, &depth))
    return 0;

  image = XGetImage(iupmot_display, icon, 0, 0, w, h, AllPlanes, ZPixmap);
  if (!image)
    return 0;

  if (mask)
    mask_image = XGetImage(iupmot_display, mask, 0, 0, w, h, AllPlanes, ZPixmap);

  dst = (unsigned char*)malloc(w * h * 4);
  if (!dst)
  {
    XDestroyImage(image);
    if (mask_image)
      XDestroyImage(mask_image);
    return 0;
  }

  for (py = 0; py < (int)h; py++)
  {
    for (px = 0; px < (int)w; px++)
    {
      unsigned long pixel = XGetPixel(image, px, py);
      unsigned char r, g, b, a;
      int offset = (py * (int)w + px) * 4;

      sniPixelToRGB(pixel, &r, &g, &b);

      if (mask_image)
      {
        unsigned long mask_pixel = XGetPixel(mask_image, px, py);
        a = mask_pixel ? 0xff : 0x00;
      }
      else
      {
        a = 0xff;
      }

      /* Output in ARGB order for SNI */
      dst[offset + 0] = a;
      dst[offset + 1] = r;
      dst[offset + 2] = g;
      dst[offset + 3] = b;
    }
  }

  XDestroyImage(image);
  if (mask_image)
    XDestroyImage(mask_image);

  *width = (int)w;
  *height = (int)h;
  *pixels = dst;

  return 1;
}
