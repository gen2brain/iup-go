/** \file
 * \brief Image Resource.
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <memory.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drvinfo.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


void iupdrvImageGetData(void* handle, unsigned char* imgdata)
{
  Pixmap pixmap = (Pixmap)handle;
  int w, h, bpp;
  XImage *xi;

  if (!iupdrvImageGetInfo(handle, &w, &h, &bpp))
    return;

  if (bpp == 8)
    return;

  xi = XGetImage(iupmot_display, pixmap, 0, 0, w, h, ULONG_MAX, ZPixmap);
  if (xi)
  {
    /* planes are packed and top-bottom in this imgdata */
    int planesize = w*h;
    unsigned char *line_data;
    int y, x;

    for (y = 0; y<h; y++)
    {
      line_data = imgdata + y * planesize;
      for (x = 0; x<w; x++)
      {
        iupmotColorGetRGB(XGetPixel(xi, x, y), line_data + x, line_data + x + 1, line_data + x + 2);
      }
    }

    XDestroyImage(xi);
  }
}

IUP_SDK_API void iupdrvImageGetRawData(void* handle, unsigned char* imgdata)
{
  Pixmap pixmap = (Pixmap)handle;
  int w, h, bpp;
  XImage *xi;

  if (!iupdrvImageGetInfo(handle, &w, &h, &bpp))
    return;

  if (bpp==8)
    return;

  xi = XGetImage(iupmot_display, pixmap, 0, 0, w, h, ULONG_MAX, ZPixmap);
  if (xi)
  {
    /* planes are separated in imgdata */
    int y, x;
    int planesize = w*h;
    unsigned char *r = imgdata,
                  *g = imgdata+planesize,
                  *b = imgdata+2*planesize;
    for (y=0; y<h; y++)
    {
      int lineoffset = (h-1 - y)*w;  /* imgdata is bottom up */
      for (x=0; x<w; x++)
      {
        iupmotColorGetRGB(XGetPixel(xi, x, y), r + lineoffset+x, g + lineoffset+x, b + lineoffset+x);
      }
    }
    
    XDestroyImage(xi);
  }
}

IUP_SDK_API void* iupdrvImageCreateImageRaw(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata)
{
  int y, x;
  Pixmap pixmap;
  GC gc;

  pixmap = XCreatePixmap(iupmot_display,
          RootWindow(iupmot_display,iupmot_screen),
          width, height, iupdrvGetScreenDepth());
  if (!pixmap)
    return NULL;

  gc = XCreateGC(iupmot_display,pixmap,0,NULL);

  /* Pixmap is top-bottom */
  /* imgdata is bottom up */

  if (bpp == 8)
  {
    Pixel color2pixel[256];
    int i;
    for (i=0;i<colors_count;i++)
      color2pixel[i] = iupmotColorGetPixel(colors[i].r, colors[i].g, colors[i].b);

    for (y=0;y<height;y++)
    {
      int lineoffset = (height-1 - y)*width;  /* imgdata is bottom up */
      for(x=0;x<width;x++)
      {
        unsigned long p = color2pixel[imgdata[lineoffset+x]];
        XSetForeground(iupmot_display,gc,p);
        XDrawPoint(iupmot_display,pixmap,gc,x,y);
      }
    }
  }
  else
  {
    /* planes are separated in imgdata */
    int planesize = width*height;
    unsigned char *r = imgdata,
                  *g = imgdata+planesize,
                  *b = imgdata+2*planesize;
    for (y=0;y<height;y++)
    {
      int lineoffset = (height-1 - y)*width;  /* imgdata is bottom up */
      for(x=0;x<width;x++)
      {
        unsigned long p = iupmotColorGetPixel(r[lineoffset+x], g[lineoffset+x], b[lineoffset+x]);
        XSetForeground(iupmot_display,gc,p);
        XDrawPoint(iupmot_display,pixmap,gc,x,y);
      }
    }
  }

  XFreeGC(iupmot_display,gc);

  return (void*)pixmap;
}

void* iupdrvImageCreateImage(Ihandle *ih, const char* bgcolor, int make_inactive)
{
  int y, x, bpp, bgcolor_depend = 0,
      width = ih->currentwidth,
      height = ih->currentheight;
  unsigned char *imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");
  Pixmap pixmap;
  unsigned char bg_r=0, bg_g=0, bg_b=0;
  GC gc;
  Pixel color2pixel[256];

  bpp = iupAttribGetInt(ih, "BPP");

  iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

  if (bpp == 8)
  {
    int i, colors_count = 0;
    iupColor colors[256];

    iupImageInitColorTable(ih, colors, &colors_count);

    for (i=0;i<colors_count;i++)
    {
      if (colors[i].a == 0)
      {
        colors[i].r = bg_r;
        colors[i].g = bg_g;
        colors[i].b = bg_b;
        colors[i].a = 255;
        bgcolor_depend = 1;
      }

      if (make_inactive)
        iupImageColorMakeInactive(&(colors[i].r), &(colors[i].g), &(colors[i].b), bg_r, bg_g, bg_b);

      color2pixel[i] = iupmotColorGetPixel(colors[i].r, colors[i].g, colors[i].b);
    }
  }

  pixmap = XCreatePixmap(iupmot_display,
          RootWindow(iupmot_display,iupmot_screen),
          width, height, iupdrvGetScreenDepth());
  if (!pixmap)
    return NULL;

  gc = XCreateGC(iupmot_display,pixmap,0,NULL);
  for (y=0;y<height;y++)
  {
    for(x=0;x<width;x++)
    {
      unsigned long p;
      if (bpp == 8)
        p = color2pixel[imgdata[y*width+x]];
      else
      {
        int channels = (bpp==24)? 3: 4;
        unsigned char *pixel_data = imgdata + y*width*channels + x*channels;
        unsigned char r = *(pixel_data),
                      g = *(pixel_data+1),
                      b = *(pixel_data+2);

        if (bpp == 32)
        {
          unsigned char a = *(pixel_data+3);
          if (a != 255)
          {
            /* flat alpha */
            r = iupALPHABLEND(r, bg_r, a);
            g = iupALPHABLEND(g, bg_g, a);
            b = iupALPHABLEND(b, bg_b, a);
            bgcolor_depend = 1;
          }
        }

        if (make_inactive)
          iupImageColorMakeInactive(&r, &g, &b, bg_r, bg_g, bg_b);

        p = iupmotColorGetPixel(r, g, b);
      }

      XSetForeground(iupmot_display,gc,p);
      XDrawPoint(iupmot_display,pixmap,gc,x,y);
    }
  }
  XFreeGC(iupmot_display,gc);

  if (bgcolor_depend || make_inactive)
    iupAttribSet(ih, "_IUP_BGCOLOR_DEPEND", "1");

  return (void*)pixmap;
}

void* iupdrvImageCreateIcon(Ihandle *ih)
{
  return iupdrvImageCreateImage(ih, NULL, 0);
}

void* iupdrvImageCreateCursor(Ihandle *ih)
{
  int bpp,y,x,hx,hy,
      width = ih->currentwidth,
      height = ih->currentheight,
      line_size = (width+7)/8,
      size_bytes = line_size*height;
  unsigned char *imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");
  char *sbits, *mbits, *sb, *mb;
  Pixmap source, mask;
  XColor fg, bg;
  unsigned char r, g, b;
  Cursor cursor;

  bpp = iupAttribGetInt(ih, "BPP");
  if (bpp > 8)
    return NULL;

  sbits = (char*)malloc(2*size_bytes);
  if (!sbits) return NULL;
  memset(sbits, 0, 2*size_bytes);
  mbits = sbits + size_bytes;

  sb = sbits;
  mb = mbits;
  for (y=0; y<height; y++)
  {
    for (x=0; x<width; x++)
    {
      int byte = x/8;
      int bit = x%8;
      int index = (int)imgdata[y*width+x];
      /* index==0 is transparent */
      if (index == 1)
        sb[byte] = (char)(sb[byte] | (1<<bit));
      if (index != 0)
        mb[byte] = (char)(mb[byte] | (1<<bit));
    }

    sb += line_size;
    mb += line_size;
  }

  r = 255; g = 255; b = 255;
  iupStrToRGB(iupAttribGet(ih, "1"), &r, &g, &b );
  fg.red   = iupCOLOR8TO16(r);
  fg.green = iupCOLOR8TO16(g);
  fg.blue  = iupCOLOR8TO16(b);
  fg.flags = DoRed | DoGreen | DoBlue;

  r = 0; g = 0; b = 0;
  iupStrToRGB(iupAttribGet(ih, "2"), &r, &g, &b );
  bg.red   = iupCOLOR8TO16(r);
  bg.green = iupCOLOR8TO16(g);
  bg.blue  = iupCOLOR8TO16(b);
  bg.flags = DoRed | DoGreen | DoBlue;

  hx=0; hy=0;
  iupStrToIntInt(iupAttribGet(ih, "HOTSPOT"), &hx, &hy, ':');

  source = XCreateBitmapFromData(iupmot_display, 
             RootWindow(iupmot_display,iupmot_screen),
             sbits, width, height);
  mask   = XCreateBitmapFromData(iupmot_display, 
             RootWindow(iupmot_display,iupmot_screen),
             mbits, width, height);

  cursor = XCreatePixmapCursor(iupmot_display, source, mask, &fg, &bg, hx, hy);

  free(sbits);
  return (void*)cursor;
}

static Pixmap motImageCreateMask(Ihandle *ih)
{
  int bpp,y,x,
      width = ih->currentwidth,
      height = ih->currentheight,
      line_size = (width+7)/8,
      size_bytes = line_size*height;
  unsigned char *imgdata = (unsigned char*)iupAttribGetStr(ih, "WID");
  char *bits, *sb;
  Pixmap mask;
  unsigned char colors[256];

  bpp = iupAttribGetInt(ih, "BPP");
  if (bpp > 8)
    return 0;

  bits = (char*)malloc(size_bytes);
  if (!bits) return 0;
  memset(bits, 0, size_bytes);

  iupImageInitNonBgColors(ih, colors);

  sb = bits;
  for (y=0; y<height; y++)
  {
    for (x=0; x<width; x++)
    {
      int byte = x/8;
      int bit = x%8;
      int index = (int)imgdata[y*width+x];
      if (colors[index])
        sb[byte] = (char)(sb[byte] | (1<<bit));
    }

    sb += line_size;
  }

  mask = XCreateBitmapFromData(iupmot_display, 
                               RootWindow(iupmot_display,iupmot_screen),
                               bits, width, height);

  free(bits);
  return mask;
}

Pixmap iupmotImageGetMask(const char* name)
{
  Pixmap mask;
  Ihandle *ih;

  if (!name)
    return 0;

  ih = iupImageGetImageFromName(name);
  if (!ih)
    return 0;

  /* Check for an already created icon */
  mask = (Pixmap)iupAttribGet(ih, "_IUPIMAGE_MASK");
  if (mask)
    return mask;

  /* Not created, tries to create the mask */
  mask = motImageCreateMask(ih);

  /* save the mask */
  iupAttribSet(ih, "_IUPIMAGE_MASK", (char*)mask);

  return mask;
}

void* iupdrvImageLoad(const char* name, int type)
{
  if (type == IUPIMAGE_CURSOR)
  {
    Cursor cursor = 0;
    int id;
    if (iupStrToInt(name, &id))
      cursor = XCreateFontCursor(iupmot_display, id);
    return (void*)cursor;
  }
  else /* IUPIMAGE_IMAGE or IUPIMAGE_ICON */
  {
    Screen* screen = ScreenOfDisplay(iupmot_display, iupmot_screen);
  	Pixmap pixmap = XmGetPixmap(screen, (char*)name, BlackPixelOfScreen(screen), WhitePixelOfScreen(screen));
    if (pixmap == XmUNSPECIFIED_PIXMAP)
    {
	    unsigned int width, height;
	    int hotx, hoty;
      pixmap = 0;
    	XReadBitmapFile(iupmot_display, RootWindow(iupmot_display,iupmot_screen), name, &width, &height, &pixmap, &hotx, &hoty);
    }
  	return (void*)pixmap;
  }
}

int iupdrvImageGetInfo(void* handle, int *w, int *h, int *bpp)
{
  Pixmap pixmap = (Pixmap)handle;
  Window root;
  int x, y;
  unsigned int width, height, b, depth;
  if (!XGetGeometry(iupmot_display, pixmap, &root, &x, &y, &width, &height, &b, &depth))
  {
    if (w) *w = 0;
    if (h) *h = 0;
    if (bpp) *bpp = 0;
    return 0;
  }
  if (w) *w = width;
  if (h) *h = height;
  if (bpp) *bpp = iupImageNormBpp(depth);
  return 1;
}

IUP_SDK_API int iupdrvImageGetRawInfo(void* handle, int *w, int *h, int *bpp, iupColor* colors, int *colors_count)
{
  /* How to get the pallete? */
  (void)colors;
  (void)colors_count;
  return iupdrvImageGetInfo(handle, w, h, bpp);
}

IUP_SDK_API void iupdrvImageDestroy(void* handle, int type)
{
  if (type == IUPIMAGE_CURSOR)
    XFreeCursor(iupmot_display, (Cursor)handle);
  else
  {
    Screen* screen = ScreenOfDisplay(iupmot_display, iupmot_screen);
    if (!XmDestroyPixmap(screen, (Pixmap)handle))
      XFreePixmap(iupmot_display, (Pixmap)handle);
  }
}

