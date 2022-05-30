/** \file
 * \brief Motif Driver color management
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <Xm/Xm.h>
#include <X11/Xproto.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h" 
#include "iup_str.h" 
#include "iup_drvinfo.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"



/* local variables */

typedef struct _Icolor
{
  Colormap colormap;         /* colormap for all canvas, dialogs and menus */
  XColor color_table[256];   /* colormap colors */
  int num_colors;            /* number of colors in colormap  */

  int rshift;                /* red shift for truecolor */
  int gshift;                /* green shift for truecolor */
  int bshift;                /* blue shift for truecolor */

  int direct_conv[256];      /* used with directColor visuals */
} Icolor;

static Icolor mot_color;


/*******************************************
              NOT  TrueColor Functions
*******************************************/

static int motColor8BppErrorHandler(Display* dpy, XErrorEvent *err)
{
  char msg[80];

  /* BadAcess in XFreeColors is OK */
  if (err->request_code==X_FreeColors && err->error_code==BadAccess)
    return 0;

  XGetErrorText(dpy, err->error_code, msg, 80 );
  fprintf(stderr,"IUP Motif error handler: Xlib request %d: %s\n", err->request_code, msg);

  return 0;
}

static unsigned long motColorNearestRGB(XColor* xc1)
{
  static int nearest_try = 0;

  int pos = 0, i;
  unsigned long min_dist = ULONG_MAX, this_dist;
  int dr, dg, db;
  XColor* xc2;

  for (i=0; i<mot_color.num_colors; i++)
  {
    xc2 = &(mot_color.color_table[i]);

    dr = (xc1->red   - xc2->red) / 850;       /* 0.30 / 255 */
    dg = (xc1->green - xc2->green) / 432;     /* 0.59 / 255 */
    db = (xc1->blue  - xc2->blue) /  2318;    /* 0.11 / 255 */

    this_dist = dr*dr + dg*dg + db*db;

    if (this_dist < min_dist)
    {
      min_dist = this_dist;            
      pos = i;                          
    }
  }

  /* verifico se a cor ainda esta alocada */
  /* Try to allocate the closest match color.  This should only
     fail if the cell is read/write.  Otherwise, we're incrementing
     the cell's reference count. (comentario extraido da biblioteca Mesa) */
  if (!XAllocColor(iupmot_display, mot_color.colormap,
                   &(mot_color.color_table[pos])))
  {
    /* nao esta, preciso atualizar a tabela e procurar novamente */
    /* isto acontece porque a cor encontrada pode ter sido de uma aplicacao que nao existe mais */
    /* uma vez atualizada, o problema nao ocorrera' na nova procura */
    /* ou a celula e' read write */

    if (nearest_try == 1)
    {
      nearest_try = 0;
      return mot_color.color_table[pos].pixel;
    }

    XQueryColors(iupmot_display, mot_color.colormap, mot_color.color_table, mot_color.num_colors);

    nearest_try = 1; /* garante que so' vai tentar isso uma vez */
    return motColorNearestRGB(xc1);
  }

  return mot_color.color_table[pos].pixel;
}

static unsigned long motColorGetPixel_NotTrueColor(unsigned char cr, unsigned char cg, unsigned char cb)
{
  unsigned long pixel;
  XColor xc;
  xc.red = iupCOLOR8TO16(cr);
  xc.green = iupCOLOR8TO16(cg);
  xc.blue = iupCOLOR8TO16(cb);
  xc.flags = DoRed | DoGreen | DoBlue;

  /* verificamos se a nova cor ja' esta' disponivel */
  if (!XAllocColor(iupmot_display, mot_color.colormap, &xc))
  {
    /* nao estava disponivel, procuro pela mais proxima na tabela de cores */
    pixel = motColorNearestRGB(&xc); 
  }
  else
  {
    /* ja' estava disponivel */
    /* atualizo a tabela de cores */
    mot_color.color_table[xc.pixel] = xc;
    pixel = xc.pixel;
  }

  return pixel;
}

static void motColorGetRGB_NotTrueColor(unsigned long pixel, unsigned char* red, unsigned char* green, unsigned char* blue)
{
  XColor xc;
  xc.pixel = pixel;
  XQueryColor(iupmot_display, mot_color.colormap, &xc);
  *red = (unsigned char)xc.red;
  *green = (unsigned char)xc.green;
  *blue = (unsigned char)xc.blue;
}

static void motColorReserveColors(void)
{
  unsigned char r, g, b;
  unsigned long background, foreground, shadow, highlight, selection;
  XtVaGetValues(iupmot_appshell, XmNbackground, &background, NULL);
  XmGetColors(DefaultScreenOfDisplay(iupmot_display), mot_color.colormap,
              background, &foreground, &highlight, &shadow, &selection);
  motColorGetRGB_NotTrueColor(background, &r, &g, &b);
  motColorGetPixel_NotTrueColor(r, g, b);
  motColorGetRGB_NotTrueColor(foreground, &r, &g, &b);
  motColorGetPixel_NotTrueColor(r, g, b);
  motColorGetRGB_NotTrueColor(shadow, &r, &g, &b);
  motColorGetPixel_NotTrueColor(r, g, b);
  motColorGetRGB_NotTrueColor(highlight, &r, &g, &b);
  motColorGetPixel_NotTrueColor(r, g, b);
  motColorGetRGB_NotTrueColor(selection, &r, &g, &b);
  motColorGetPixel_NotTrueColor(r, g, b);
}


/*******************************************
              TrueColor Functions
*******************************************/


static void motColorGetRGB_TrueColor(unsigned long pixel, unsigned char* red, unsigned char* green, unsigned char* blue)
{
  unsigned long r = pixel & iupmot_visual->red_mask;
  unsigned long g = pixel & iupmot_visual->green_mask;
  unsigned long b = pixel & iupmot_visual->blue_mask;
  if (mot_color.rshift<0) r = r >> (-mot_color.rshift);
  else r = r << mot_color.rshift;
  if (mot_color.gshift<0) g = g >> (-mot_color.gshift);
  else g = g << mot_color.gshift;
  if (mot_color.bshift<0) b = b >> (-mot_color.bshift);
  else b = b << mot_color.bshift;
  *red = iupCOLOR16TO8(r);
  *green = iupCOLOR16TO8(g);
  *blue = iupCOLOR16TO8(b);
}

static unsigned long motColorGetPixel_TrueColor(unsigned char cr, unsigned char cg, unsigned char cb)
{
  unsigned long r = iupCOLOR8TO16(cr);
  unsigned long g = iupCOLOR8TO16(cg);
  unsigned long b = iupCOLOR8TO16(cb);

  if (mot_color.rshift<0) 
    r = r << (-mot_color.rshift);
  else 
    r = r >> mot_color.rshift;

  if (mot_color.gshift<0) 
    g = g << (-mot_color.gshift);
  else 
    g = g >> mot_color.gshift;

  if (mot_color.bshift<0) 
    b = b << (-mot_color.bshift);
  else 
    b = b >> mot_color.bshift;

  r = r & iupmot_visual->red_mask;
  g = g & iupmot_visual->green_mask;
  b = b & iupmot_visual->blue_mask;

  return r | g | b;
}

static int motColorHighBit(unsigned long ul)
{
/* returns position of highest set bit in 'ul' as an integer (0-31),
  or -1 if none */
  int i;  unsigned long hb;

  hb = 0x80;  hb = hb << 24;   /* hb = 0x80000000UL */
  for (i=31; ((ul & hb) == 0) && i>=0;  i--, ul<<=1);
  return i;
}

static void motColorMakeDirectCmap(Colormap cmap)
{
  int    i, cmaplen, numgot;
  unsigned char   origgot[256];
  XColor c;
  unsigned long rmask, gmask, bmask;
  int    rshift, gshift, bshift;

  rmask = iupmot_visual->red_mask;
  gmask = iupmot_visual->green_mask;
  bmask = iupmot_visual->blue_mask;

  rshift = motColorHighBit(rmask) - 15;
  gshift = motColorHighBit(gmask) - 15;
  bshift = motColorHighBit(bmask) - 15;

  if (rshift<0) rmask = rmask << (-rshift);
  else rmask = rmask >> rshift;

  if (gshift<0) gmask = gmask << (-gshift);
  else gmask = gmask >> gshift;

  if (bshift<0) bmask = bmask << (-bshift);
  else bmask = bmask >> bshift;

  cmaplen = iupmot_visual->map_entries;
  if (cmaplen>256) cmaplen=256;

  /* try to alloc a 'cmaplen' long grayscale colormap.  May not get all
  entries for whatever reason.  Build table 'mot_color.direct_conv[]' that
  maps range [0..(cmaplen-1)] into set of colors we did get */

  for (i=0; i<256; i++) {  origgot[i] = 0;  mot_color.direct_conv[i] = i; }

  for (i=numgot=0; i<cmaplen; i++) 
  {
    c.red = c.green = c.blue = (unsigned short)((i * 0xffff) / (cmaplen - 1));
    c.red   = (unsigned short)(c.red   & rmask);
    c.green = (unsigned short)(c.green & gmask);
    c.blue  = (unsigned short)(c.blue  & bmask);
    c.flags = DoRed | DoGreen | DoBlue;

    if (XAllocColor(iupmot_display, cmap, &c)) 
    {
      origgot[i] = 1;
      numgot++;
    }
  }

  if (numgot == 0) 
    return;

  /* mot_color.direct_conv may or may not have holes in it. */
  for (i=0; i<cmaplen; i++) 
  {
    if (!origgot[i]) 
    {
      int numbak, numfwd;
      numbak = numfwd = 0;
      while ((i - numbak) >= 0       && !origgot[i-numbak]) numbak++;
      while ((i + numfwd) <  cmaplen && !origgot[i+numfwd]) numfwd++;

      if (i-numbak<0        || !origgot[i-numbak]) numbak = 999;
      if (i+numfwd>=cmaplen || !origgot[i+numfwd]) numfwd = 999;

      if      (numbak<numfwd) mot_color.direct_conv[i] = mot_color.direct_conv[i-numbak];
      else if (numfwd<999)    mot_color.direct_conv[i] = mot_color.direct_conv[i+numfwd];
    }
  }
}


/*******************************************
              Exported Functions
*******************************************/

unsigned long (* iupmotColorGetPixel)(unsigned char r, unsigned char g, unsigned char b) = 0; 
void (* iupmotColorGetRGB)(unsigned long pixel, unsigned char* red, unsigned char* green, unsigned char* blue) = 0;

void iupmotColorInit(void)
{
  int depth = iupdrvGetScreenDepth();

  if (depth > 8)
  {
    iupmotColorGetRGB = motColorGetRGB_TrueColor;
    iupmotColorGetPixel = motColorGetPixel_TrueColor;

     /* make linear colormap for DirectColor visual */
    if (iupmot_visual->class == DirectColor)
      motColorMakeDirectCmap(DefaultColormap(iupmot_display, iupmot_screen));

    mot_color.rshift = 15 - motColorHighBit(iupmot_visual->red_mask);
    mot_color.gshift = 15 - motColorHighBit(iupmot_visual->green_mask);
    mot_color.bshift = 15 - motColorHighBit(iupmot_visual->blue_mask);

    mot_color.num_colors = 0;
    mot_color.colormap = 0;
  }
  else
  {
    int i;
    iupmotColorGetRGB = motColorGetRGB_NotTrueColor;
    iupmotColorGetPixel = motColorGetPixel_NotTrueColor;

    mot_color.colormap = DefaultColormap(iupmot_display, iupmot_screen);
    mot_color.num_colors = 1L << depth;

    for (i=0; i<mot_color.num_colors; i++)
      mot_color.color_table[i].pixel = i;

    XQueryColors(iupmot_display, mot_color.colormap, mot_color.color_table, mot_color.num_colors);
    XSetErrorHandler(motColor8BppErrorHandler);

    motColorReserveColors();
  }
}

void iupmotColorFinish(void)
{
  if (mot_color.colormap)
  {
    unsigned long pixels[256];
    int i;

    /* release colors */
    for (i = 0; i < mot_color.num_colors; i++)
      pixels[i] = mot_color.color_table[i].pixel;

    XFreeColors(iupmot_display, mot_color.colormap, pixels, mot_color.num_colors, 0);

    /* release palette */
    if (mot_color.colormap != DefaultColormap(iupmot_display, iupmot_screen))
      XFreeColormap(iupmot_display, mot_color.colormap);
  }
}

Colormap iupmotColorMap(void)
{
  return mot_color.colormap;
}

unsigned long iupmotColorGetPixelStr(const char* color)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(color, &r, &g, &b))
    return (unsigned long)-1;

  return iupmotColorGetPixel(r, g, b);
}
