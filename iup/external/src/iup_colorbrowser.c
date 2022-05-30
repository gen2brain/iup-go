/** \file
* \brief ColorBrowser Control.
*
* See Copyright Notice in "iup.h"
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iupdraw.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"
#include "iup_image.h"

#include "iup_colorhsi.h"

#ifndef min
#define min(a, b) ( (a < b) ? (a) : (b) )
#endif

#define IUP_RAD2DEG  57.295779513   /* radians to degrees (deg = IUP_RAD2DEG * rad) */

#define ICB_DEFAULTSIZE   181  /* default size */
#define ICB_SPACE  4           /* size of the spacing */
#define ICB_HUEWIDTH  20       /* width of the hue ring */
#define ICB_MARKSIZE  8        /* size of the cursor mark */

enum {ICB_INSIDE_NONE, ICB_INSIDE_HUE, ICB_INSIDE_SI};

#define iupInvertYAxis(_y, _h) ((_h) - (_y) - 1)

struct _IcontrolData
{
  iupCanvas canvas;  /* from IupCanvas (must reserve it) */

  /* mouse interaction state */ 
  int h_down,
      si_down;

  /* cursor positioning */
  int h_x, h_y,
      si_x, si_y;

  /* HSI-XY coordinate conversion */
  int xc, yc,  /* center */
      R,       /* maximum radius available inside the size of the control */
      Ix,      /* x coordinate where S is 0 */
      Iy1,     /* y coordinate where I is 0 */
      Iy2,     /* y coordinate where I is 1 */
      SxMax;   /* x coordinate where S is 1 and I = 0.5 */

  /* visual appearance control */
  int w, h;
  int has_focus; 
  long bgcolor;

  /* attributes */
  double hue,          /* 0<=H<=359 */
        saturation,   /* 0<=S<=1 */
        intensity;    /* 0<=I<=1 */
  unsigned char red, green, blue;  /* 0<=x<=255 */

  Ihandle *image;
};


static double iColorBrowserSXmax(Ihandle* ih, int y)
{
  if (y == ih->data->yc)
    return (double)(ih->data->SxMax - ih->data->Ix);
  else if (y < ih->data->yc)
  {
    double D2 = (ih->data->Iy2 - ih->data->Iy1)/2.0;
    return  ((double)(ih->data->SxMax - ih->data->Ix)*(y-ih->data->yc + D2))/D2;
  }
  else
  {
    double D2 = (ih->data->Iy2 - ih->data->Iy1)/2.0;
    return -((double)(ih->data->SxMax - ih->data->Ix)*(y-ih->data->yc - D2))/D2;
  }
}

static double iColorBrowserCalcIntensity(Ihandle* ih, int y)
{
  return (double)(y - ih->data->Iy1)/(double)(ih->data->Iy2 - ih->data->Iy1);
}

static double iColorBrowserCalcSaturation(Ihandle* ih, int x, double sx_max)
{
  if (sx_max == 0)
    return 0;
  else
    return (x - ih->data->Ix)/sx_max;
}

/* Rotate points of 60 degrees */
static void iColorBrowserRotatePoints(double *x1, double *y1, double *x2, double *y2, int xc, int yc)
{
  double xt, yt;
  double nxt, nyt;
  static const double s60 = 0.8660254;
  static const double c60 = 0.5;

  xt = *x1 - xc; 
  yt = *y1 - yc;
  nxt = xt * c60 - yt * s60;
  nyt = xt * s60 + yt * c60;
  *x1 = nxt + xc; 
  *y1 = nyt + yc;

  xt = *x2 - xc; 
  yt = *y2 - yc;
  nxt = xt * c60 - yt * s60;
  nyt = xt * s60 + yt * c60;
  *x2 = nxt + xc;
  *y2 = nyt + yc;
}

static void iColorBrowserRenderImageHue(Ihandle* ih)
{
  int x, y, active = 1;
  unsigned char *colors;
  unsigned char bg_red, bg_green, bg_blue;

  if (!IupGetInt(ih, "ACTIVE"))
    active = 0;

  colors = (unsigned char*)iupAttribGet(ih->data->image, "WID");

  bg_red = iupDrawRed(ih->data->bgcolor);
  bg_green = iupDrawGreen(ih->data->bgcolor);
  bg_blue = iupDrawBlue(ih->data->bgcolor);

  for (y = 0; y < ih->data->h; y++)
  {
    double sx_max = iColorBrowserSXmax(ih, y);   

    for (x = 0; x < ih->data->w; x++)
    {
      int xl, yl;
      double radius, diff1, diff2;

      if (y > ih->data->Iy1 &&
          y < ih->data->Iy2 &&
          x > ih->data->Ix  &&
          x < ih->data->Ix + (int)sx_max)
        continue;

      xl = x - ih->data->xc;
      yl = y - ih->data->yc;
      radius = sqrt((double)xl*xl + (double)yl*yl);

      diff1 = radius - (ih->data->R-ICB_SPACE-ICB_HUEWIDTH);
      diff2 = (ih->data->R-ICB_SPACE) - radius;

      if (diff1>0 && diff2>0)
      {
        double h, s, i;
        int offset = 3 * ((ih->data->h-y-1)*ih->data->w + x);
        unsigned char* r = colors + offset;
        unsigned char* g = colors + offset + 1;
        unsigned char* b = colors + offset + 2;

        h = atan2((double)yl, (double)xl);
        h = h * IUP_RAD2DEG;
        s = 1.0;   /* maximum saturation */
        i = 0.5f;   /* choose I where S is maximum */

        iupColorHSI2RGB(h, s, i, r, g, b);

        if (diff1<1 || diff2<1)  /* anti-aliasing */
        {
          double diff = diff1<1? diff1: diff2;
          *r = (unsigned char)((*r)*diff + bg_red*(1.0-diff));
          *g = (unsigned char)((*g)*diff + bg_green*(1.0-diff));
          *b = (unsigned char)((*b)*diff + bg_blue*(1.0-diff));
        }

        if (!active)
          iupImageColorMakeInactive(r, g, b, bg_red, bg_green, bg_blue);
      }
      else
      {
        int offset = 3 * ((ih->data->h - y - 1)*ih->data->w + x);
        unsigned char* r = colors + offset;
        unsigned char* g = colors + offset + 1;
        unsigned char* b = colors + offset + 2;
        *r = bg_red;
        *g = bg_green;
        *b = bg_blue;
      }
    }
  }
}

static void iColorBrowserRenderImageSI(Ihandle* ih)
{
  int x, y, active = 1;
  unsigned char *colors;
  unsigned char bg_red, bg_green, bg_blue;

  if (!IupGetInt(ih, "ACTIVE"))
    active = 0;

  colors = (unsigned char*)iupAttribGet(ih->data->image, "WID");

  bg_red = iupDrawRed(ih->data->bgcolor);
  bg_green = iupDrawGreen(ih->data->bgcolor);
  bg_blue = iupDrawBlue(ih->data->bgcolor);

  for (y = 0; y < ih->data->h; y++)
  {
    double sx_max, i;

    if (y < ih->data->Iy1)
      continue;
    else if (y > ih->data->Iy2)
      continue;

    sx_max = iColorBrowserSXmax(ih, y);
    i = iColorBrowserCalcIntensity(ih, y);

    for (x = 0; x < ih->data->w; x++)
    {
      if (x < ih->data->Ix)
        continue;
      else if (x > ih->data->Ix+(int)sx_max)
        continue;

      {
        int offset = 3 * ((ih->data->h-y-1)*ih->data->w + x);
        unsigned char* r = colors + offset;
        unsigned char* g = colors + offset + 1;
        unsigned char* b = colors + offset + 2;
        double s, diff;

        s = iColorBrowserCalcSaturation(ih, x, sx_max);

        iupColorHSI2RGB(ih->data->hue, s, i, r, g, b);

        diff = sx_max - (double)(x - ih->data->Ix);
        if (diff<1.0)  /* anti-aliasing */
        {
          *r = (unsigned char)((*r)*diff + bg_red*(1.0-diff));
          *g = (unsigned char)((*g)*diff + bg_green*(1.0-diff));
          *b = (unsigned char)((*b)*diff + bg_blue*(1.0-diff));
        }

        if (!active)
          iupImageColorMakeInactive(r, g, b, bg_red, bg_green, bg_blue);
      }
    }
  }
}

static void iColorBrowserUpdateCursorSI(Ihandle* ih)
{
  int x;
  int y = (int)(ih->data->intensity*(ih->data->Iy2 - ih->data->Iy1)) + ih->data->Iy1;
  ih->data->si_y = iupROUND(y);
  x = (int)(ih->data->saturation*iColorBrowserSXmax(ih, ih->data->si_y)) + ih->data->Ix;
  ih->data->si_x = iupROUND(x);
}

static void iColorBrowserSetCursorSI(Ihandle* ih, int x, int y)
{
  double sx_max;

  if (y < ih->data->Iy1)
    ih->data->si_y = ih->data->Iy1;
  else if (y > ih->data->Iy2)
    ih->data->si_y = ih->data->Iy2;
  else
    ih->data->si_y = y;

  sx_max = iColorBrowserSXmax(ih, ih->data->si_y);

  if (x < ih->data->Ix)
    ih->data->si_x = ih->data->Ix;
  else if (x > ih->data->Ix+sx_max)
    ih->data->si_x = ih->data->Ix+(int)sx_max;
  else
    ih->data->si_x = x;

  ih->data->intensity = iColorBrowserCalcIntensity(ih, ih->data->si_y);
  ih->data->saturation = iColorBrowserCalcSaturation(ih, ih->data->si_x, sx_max);

  if (ih->data->saturation == -0.0)
    ih->data->saturation = 0;
  if (ih->data->intensity == -0.0)
    ih->data->intensity = 0;
}

static void iColorBrowserUpdateCursorHue(Ihandle* ih)
{
  int rc = ih->data->R-ICB_SPACE-ICB_HUEWIDTH/2;
  double angle = ih->data->hue / IUP_RAD2DEG;
  double cos_angle = cos(angle);
  double sin_angle = sin(angle);
  double x = rc*cos_angle + ih->data->xc;
  double y = rc*sin_angle + ih->data->yc;
  ih->data->h_x = iupROUND(x);
  ih->data->h_y = iupROUND(y);
}

static void iColorBrowserSetCursorHue(Ihandle* ih, int x, int y)
{
  int xl = x - ih->data->xc;
  int yl = y - ih->data->yc;
  ih->data->hue = atan2((double)yl, (double)xl) * IUP_RAD2DEG;
  ih->data->hue = fmod(ih->data->hue, 360.0);
  if (ih->data->hue < 0.0)
    ih->data->hue += 360.0;

  iColorBrowserUpdateCursorHue(ih);
}

static int iColorBrowserCheckInside(Ihandle* ih, int x, int y)
{
  int xl = x - ih->data->xc;
  int yl = y - ih->data->yc;
  double radius = sqrt((double)xl*xl + (double)yl*yl);

  if (radius < ih->data->R-ICB_SPACE-ICB_HUEWIDTH-ICB_SPACE)
    return ICB_INSIDE_SI;

  if (radius > ih->data->R-ICB_SPACE-ICB_HUEWIDTH &&
      radius < ih->data->R-ICB_SPACE)
    return ICB_INSIDE_HUE;

  return ICB_INSIDE_NONE;
}

static void iColorBrowserHSI2RGB(Ihandle* ih)
{
  iupColorHSI2RGB(ih->data->hue, ih->data->saturation, ih->data->intensity, 
                      &(ih->data->red), &(ih->data->green), &(ih->data->blue));
}

static void iColorBrowserRGB2HSI(Ihandle* ih)
{
  iupColorRGB2HSI(ih->data->red, ih->data->green, ih->data->blue,
                      &(ih->data->hue), &(ih->data->saturation), &(ih->data->intensity));
}

static void iColorBrowserCallChangeCb(Ihandle* ih)
{
  IFnccc change_cb = (IFnccc) IupGetCallback(ih, "CHANGE_CB");
  if (change_cb)
    change_cb(ih, ih->data->red, ih->data->green, ih->data->blue);
  
  iupBaseCallValueChangedCb(ih);
}
  
static void iColorBrowserCallDragCb(Ihandle* ih)
{
  IFnccc drag_cb = (IFnccc) IupGetCallback(ih, "DRAG_CB");
  if (drag_cb)
    drag_cb(ih, ih->data->red, ih->data->green, ih->data->blue);

  iupBaseCallValueChangedCb(ih);
}

static int iColorBrowserHmouse(Ihandle* ih, int x, int y, int drag)
{
  iColorBrowserSetCursorHue(ih, x, y);
  iColorBrowserHSI2RGB(ih);

  /* must update the Si area */
  iColorBrowserRenderImageSI(ih);
  IupSetAttribute(ih->data->image, "CLEARCACHE", "1");
  IupUpdate(ih);

  if (drag)
    iColorBrowserCallDragCb(ih);
  else
    iColorBrowserCallChangeCb(ih);

  return IUP_DEFAULT;
}

static int iColorBrowserSImouse(Ihandle* ih, int x, int y, int drag)
{
  iColorBrowserSetCursorSI(ih, x, y);
  iColorBrowserHSI2RGB(ih);
  IupUpdate(ih);

  if (drag)
    iColorBrowserCallDragCb(ih);
  else
    iColorBrowserCallChangeCb(ih);

  return IUP_DEFAULT;
}


/******************************************************************/


static int iColorBrowserButton_CB(Ihandle* ih, int b, int press, int x, int y)
{
  if (b != IUP_BUTTON1)
    return IUP_DEFAULT;

  y = iupInvertYAxis(y, ih->data->h);

  if (press)
  {
    int inside = iColorBrowserCheckInside(ih, x, y);

    if (!ih->data->h_down && inside==ICB_INSIDE_HUE)
    {
      iColorBrowserHmouse(ih, x, y, 1);
      ih->data->h_down = 1;
    } 

    if (!ih->data->si_down && inside==ICB_INSIDE_SI)
    {
      iColorBrowserSImouse(ih, x, y, 1);
      ih->data->si_down = 1;
    } 
  }
  else
  {
    if (ih->data->h_down)
    {
      iColorBrowserHmouse(ih, x, y, 0);
      ih->data->h_down = 0;
    }

    if (ih->data->si_down)
    {
      iColorBrowserSImouse(ih, x, y, 0);
      ih->data->si_down = 0;
    }
  }

  return IUP_DEFAULT;
}

/* Callback for the mouse motion in the canvas */
static int iColorBrowserMotion_CB(Ihandle* ih, int x, int y, char *status)
{
  if (!iup_isbutton1(status))
  {
    ih->data->h_down = 0;
    ih->data->si_down = 0;
    return IUP_DEFAULT;
  }

  if (ih->data->h_down)
  {
    y = iupInvertYAxis(y, ih->data->h);
    iColorBrowserHmouse(ih, x, y, 1);
  }
  else if (ih->data->si_down)
  {
    y = iupInvertYAxis(y, ih->data->h);
    iColorBrowserSImouse(ih, x, y, 1);
  }

  return IUP_DEFAULT;
}

static int iColorBrowserFocus_CB(Ihandle* ih, int focus)
{
  ih->data->has_focus = focus;
  IupUpdate(ih);
  return IUP_DEFAULT;
}

static void iColorBrowserUpdateSize(Ihandle* ih, int width, int height)
{
  int T, D;

  ih->data->w = width;
  ih->data->h = height;

  ih->data->R = min(ih->data->w, ih->data->h)/2;
  ih->data->xc = ih->data->w/2;
  ih->data->yc = ih->data->h/2;
  T = ih->data->R-ICB_SPACE-ICB_HUEWIDTH-ICB_SPACE;
  ih->data->Ix = ih->data->xc - T/2; /* cos(60)=0.5 */
  D = (int)(2*T*0.866);              /* sin(60)=0.866 */
  ih->data->Iy1 = ih->data->yc - D/2;
  ih->data->Iy2 = ih->data->Iy1 + D;
  ih->data->SxMax = ih->data->xc + T;
}

static int iColorBrowserResize_CB(Ihandle* ih, int width, int height)
{
  int old_width = IupGetInt(ih->data->image, "WIDTH");
  int old_height = IupGetInt(ih->data->image, "HEIGHT");

  if (width != old_width || height != old_height)
  {
    IupSetStrf(ih->data->image, "RESHAPE", "%dx%d", width, height);

    /* update size */
    iColorBrowserUpdateSize(ih, width, height);

    iColorBrowserUpdateCursorHue(ih);
    iColorBrowserUpdateCursorSI(ih);

    /* update render */
    iColorBrowserRenderImageHue(ih);
    iColorBrowserRenderImageSI(ih);
    IupSetAttribute(ih->data->image, "CLEARCACHE", "1");
    /* no need to call IupUpdate, a redraw event is already on the way */
  }

  return IUP_DEFAULT;
}

static int iColorBrowserRedraw_CB(Ihandle* ih)
{
  int xc, yc;
  int active = 1;
  char* name;

  if (!iupdrvIsActive(ih))
    active = 0;

  IupDrawBegin(ih);

  IupDrawParentBackground(ih);

  name = IupGetAttribute(ih, "ICB_IMAGE");
  IupDrawImage(ih, name, 0, 0, -1, -1);

  if (active)
  {
    double x1, x2, y1, y2;
    unsigned char shade_lr, shade_lg, shade_lb,
      shade_dr, shade_dg, shade_db;
    unsigned char bg_red, bg_green, bg_blue;

    bg_red = iupDrawRed(ih->data->bgcolor);
    bg_green = iupDrawGreen(ih->data->bgcolor);
    bg_blue = iupDrawBlue(ih->data->bgcolor);

    shade_dr = (unsigned char)((2 * bg_red) / 3);
    shade_dg = (unsigned char)((2 * bg_green) / 3);
    shade_db = (unsigned char)((2 * bg_blue) / 3);
    shade_lr = (unsigned char)((255 + bg_red) / 2);
    shade_lg = (unsigned char)((255 + bg_green) / 2);
    shade_lb = (unsigned char)((255 + bg_blue) / 2);

    iupDrawSetColor(ih, "DRAWCOLOR", iupDrawColor(shade_dr, shade_dg, shade_db, 255));

    x1 = (double)(ih->data->xc - ih->data->R + ICB_SPACE);
    y1 = (double)ih->data->yc;
    x2 = (double)(x1 + ICB_HUEWIDTH / 2);
    y2 = (double)ih->data->yc;

    IupDrawLine(ih, (int)x1, (int)y1, (int)x2, (int)y2);
    iColorBrowserRotatePoints(&x1, &y1, &x2, &y2, ih->data->xc, ih->data->yc);
    iupDrawSetColor(ih, "DRAWCOLOR", iupDrawColor(shade_lr, shade_lg, shade_lb, 255));
    IupDrawLine(ih, (int)x1, (int)y1, (int)x2, (int)y2);
    iColorBrowserRotatePoints(&x1, &y1, &x2, &y2, ih->data->xc, ih->data->yc);
    iupDrawSetColor(ih, "DRAWCOLOR", iupDrawColor(shade_dr, shade_dg, shade_db, 255));
    IupDrawLine(ih, (int)x1, (int)y1, (int)x2, (int)y2);
    iColorBrowserRotatePoints(&x1, &y1, &x2, &y2, ih->data->xc, ih->data->yc);
    iupDrawSetColor(ih, "DRAWCOLOR", iupDrawColor(shade_lr, shade_lg, shade_lb, 255));
    IupDrawLine(ih, (int)x1, (int)y1, (int)x2, (int)y2);
    IupDrawLine(ih, (int)x1, (int)y1, (int)x2, (int)y2);
    iupDrawSetColor(ih, "DRAWCOLOR", iupDrawColor(shade_dr, shade_dg, shade_db, 255));
    IupDrawLine(ih, (int)x1, (int)y1, (int)x2, (int)y2);
    iColorBrowserRotatePoints(&x1, &y1, &x2, &y2, ih->data->xc, ih->data->yc);
    IupDrawLine(ih, (int)x1, (int)y1, (int)x2, (int)y2);

    iupDrawSetColor(ih, "DRAWCOLOR", iupDrawColor(192, 192, 192, 255));  /* draw markers shade as light gray always */

    xc = ih->data->h_x + 1;
    yc = ih->data->h - ih->data->h_y;
    IupDrawArc(ih, xc - ICB_MARKSIZE/2, yc - ICB_MARKSIZE/2, xc + ICB_MARKSIZE/2, yc + ICB_MARKSIZE/2, 0, 360);

    xc = ih->data->si_x + 1;
    yc = ih->data->h - ih->data->si_y;
    IupDrawArc(ih, xc - ICB_MARKSIZE / 2, yc - ICB_MARKSIZE / 2, xc + ICB_MARKSIZE / 2, yc + ICB_MARKSIZE / 2, 0, 360);

    iupDrawSetColor(ih, "DRAWCOLOR", iupDrawColor(255, 255, 255, 255));  /* draw markers as white always */

    xc = ih->data->h_x;
    yc = ih->data->h - ih->data->h_y;
    IupDrawArc(ih, xc - ICB_MARKSIZE / 2, yc - ICB_MARKSIZE / 2, xc + ICB_MARKSIZE / 2, yc + ICB_MARKSIZE / 2, 0, 360);

    xc = ih->data->si_x;
    yc = ih->data->h - ih->data->si_y;
    IupDrawArc(ih, xc - ICB_MARKSIZE / 2, yc - ICB_MARKSIZE / 2, xc + ICB_MARKSIZE / 2, yc + ICB_MARKSIZE / 2, 0, 360);
  }
  else
  {
    iupAttribSet(ih, "DRAWSTYLE", "FILL");

    iupDrawSetColor(ih, "DRAWCOLOR", iupDrawColor(128, 128, 128, 255));  /* draw inactive markers as gray always */

    xc = ih->data->h_x + 1;
    yc = ih->data->h - ih->data->h_y;
    IupDrawArc(ih, xc - ICB_MARKSIZE / 2, yc - ICB_MARKSIZE / 2, xc + ICB_MARKSIZE / 2, yc + ICB_MARKSIZE / 2, 0, 360);

    xc = ih->data->si_x + 1;
    yc = ih->data->h - ih->data->si_y;
    IupDrawArc(ih, xc - ICB_MARKSIZE / 2, yc - ICB_MARKSIZE / 2, xc + ICB_MARKSIZE / 2, yc + ICB_MARKSIZE / 2, 0, 360);
  }

  if (ih->data->has_focus)
    IupDrawFocusRect(ih, 0, 0, ih->data->w - 1, ih->data->h - 1);

  IupDrawEnd(ih);

  return IUP_DEFAULT;
}

static int iColorBrowserWheel_CB(Ihandle* ih, float delta)
{
  ih->data->hue += (double)delta;

  iColorBrowserUpdateCursorHue(ih);
  iColorBrowserHSI2RGB(ih);

  /* must update the Si area */
  iColorBrowserRenderImageSI(ih);
  IupSetAttribute(ih->data->image, "CLEARCACHE", "1");
  IupUpdate(ih);

  iColorBrowserCallChangeCb(ih);

  return IUP_DEFAULT;
}

static int iColorBrowserKeyPress_CB(Ihandle* ih, int c, int press)
{
  int x, y, changing_hue = 0;

  if (!press)
    return IUP_DEFAULT;

  x = ih->data->si_x; 
  y = ih->data->si_y; 

  switch (c)
  {
    case K_UP:
      y++; 
      break;
    case K_DOWN:
      y--;
      break;
    case K_RIGHT:
      x++;
      break;
    case K_LEFT:
      x--;
      break;
    case K_PGUP:
      ih->data->hue += 1.0; 
      changing_hue = 1; 
      break;
    case K_PGDN:
      ih->data->hue -= 1.0; 
      changing_hue = 1; break;
    case K_HOME:
      ih->data->hue = 0.0; 
      changing_hue = 1; 
      break;
    case K_END:
      ih->data->hue = 180.0; 
      changing_hue = 1; 
      break;
    default:
      return IUP_DEFAULT;
  }

  if (changing_hue)
  {
    iColorBrowserUpdateCursorHue(ih);

    /* must update the Si area */
    iColorBrowserRenderImageSI(ih);
    IupSetAttribute(ih->data->image, "CLEARCACHE", "1");
  }
  else
    iColorBrowserSetCursorSI(ih, x, y);

  iColorBrowserHSI2RGB(ih);

  IupUpdate(ih);

  iColorBrowserCallChangeCb(ih);

  return IUP_IGNORE;  /* to avoid arrow keys being processed by the system */
}


/*********************************************************************************/


static char* iColorBrowserGetHSIAttrib(Ihandle* ih)
{
  return iupStrReturnStrf(IUP_DOUBLE2STR" "IUP_DOUBLE2STR" "IUP_DOUBLE2STR, ih->data->hue, ih->data->saturation, ih->data->intensity);
}

static int iColorBrowserSetHSIAttrib(Ihandle* ih, const char* value)
{
  double old_hue = ih->data->hue,
        old_saturation = ih->data->saturation,
        old_intensity = ih->data->intensity;

  if (!iupStrToHSI(value, &ih->data->hue, &ih->data->saturation, &ih->data->intensity))
    return 0;
  
  if (old_hue != ih->data->hue) 
    iColorBrowserUpdateCursorHue(ih);
  if (old_saturation != ih->data->saturation || old_intensity != ih->data->intensity) 
    iColorBrowserUpdateCursorSI(ih);
  iColorBrowserHSI2RGB(ih);

  iColorBrowserRenderImageSI(ih);
  IupSetAttribute(ih->data->image, "CLEARCACHE", "1");
  IupUpdate(ih);

  return 0;
}

static char* iColorBrowserGetRGBAttrib(Ihandle* ih)
{
  return iupStrReturnRGB(ih->data->red, ih->data->green, ih->data->blue);
}

static int iColorBrowserSetRGBAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;
  
  ih->data->red = r;
  ih->data->green = g;
  ih->data->blue = b;
  iColorBrowserRGB2HSI(ih);

  iColorBrowserUpdateCursorHue(ih);
  iColorBrowserUpdateCursorSI(ih);

  iColorBrowserRenderImageSI(ih);
  IupSetAttribute(ih->data->image, "CLEARCACHE", "1");
  IupUpdate(ih);

  return 0;
}

static int iColorBrowserSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    value = iupBaseNativeParentGetBgColorAttrib(ih);

  ih->data->bgcolor = iupDrawStrToColor(value, ih->data->bgcolor);

  iColorBrowserRenderImageHue(ih);
  iColorBrowserRenderImageSI(ih);
  IupSetAttribute(ih->data->image, "CLEARCACHE", "1");
  IupUpdate(ih);

  return 1;
}

static int iColorBrowserSetActiveAttrib(Ihandle* ih, const char* value)
{
  iupBaseSetActiveAttrib(ih, value);

  iColorBrowserRenderImageHue(ih);
  iColorBrowserRenderImageSI(ih);
  IupSetAttribute(ih->data->image, "CLEARCACHE", "1");
  IupUpdate(ih);

  return 0;  /* do not store value in hash table */
}


/****************************************************************************/


static void iColorBrowserDestroyMethod(Ihandle* ih)
{
  IupDestroy(ih->data->image);
}

static int iColorBrowserCreateMethod(Ihandle* ih, void **params)
{
  (void)params;

  /* free the data allocated by IupCanvas */
  free(ih->data);
  ih->data = iupALLOCCTRLDATA();

  /* change the IupCanvas default values */
  IupSetfAttribute(ih, "RASTERSIZE", "%dx%d", ICB_DEFAULTSIZE, ICB_DEFAULTSIZE);
  IupSetAttribute(ih, "EXPAND", "NO");

  ih->data->w = ICB_DEFAULTSIZE;
  ih->data->h = ICB_DEFAULTSIZE;

  ih->data->image = IupImageRGB(ICB_DEFAULTSIZE, ICB_DEFAULTSIZE, NULL);
  IupSetAttributeHandle(ih, "ICB_IMAGE", ih->data->image);

  iColorBrowserUpdateSize(ih, ICB_DEFAULTSIZE, ICB_DEFAULTSIZE);

  /* IupCanvas callbacks */
  IupSetCallback(ih, "ACTION",      (Icallback)iColorBrowserRedraw_CB);
  IupSetCallback(ih, "RESIZE_CB",   (Icallback)iColorBrowserResize_CB);
  IupSetCallback(ih, "BUTTON_CB",   (Icallback)iColorBrowserButton_CB);
  IupSetCallback(ih, "MOTION_CB",   (Icallback)iColorBrowserMotion_CB);
  IupSetCallback(ih, "FOCUS_CB",    (Icallback)iColorBrowserFocus_CB);
  IupSetCallback(ih, "KEYPRESS_CB", (Icallback)iColorBrowserKeyPress_CB);
  IupSetCallback(ih, "WHEEL_CB",    (Icallback)iColorBrowserWheel_CB);

  return IUP_NOERROR;
}

Iclass* iupColorBrowserNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name = "colorbrowser";
  ic->cons = "ColorBrowser";
  ic->format = NULL; /* no parameters */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupColorBrowserNewClass;
  ic->Create  = iColorBrowserCreateMethod;
  ic->Destroy = iColorBrowserDestroyMethod;

  /* IupColorBrowser Callbacks */
  iupClassRegisterCallback(ic, "DRAG_CB",   "ccc");
  iupClassRegisterCallback(ic, "CHANGE_CB", "ccc");
  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  /* replace IupCanvas behavior */
  iupClassRegisterReplaceAttribDef(ic, "BORDER", "NO", NULL);
  iupClassRegisterReplaceAttribFlags(ic, "BORDER", IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* IupColorBrowser only */
  iupClassRegisterAttribute(ic, "RGB", iColorBrowserGetRGBAttrib, iColorBrowserSetRGBAttrib, "255 0 0", NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);    /* force new default value */
  iupClassRegisterAttribute(ic, "HSI", iColorBrowserGetHSIAttrib, iColorBrowserSetHSIAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* Overwrite IupCanvas Attributes */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iColorBrowserSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iColorBrowserSetBgColorAttrib, NULL, "255 255 255", IUPAF_NO_INHERIT);    /* overwrite canvas implementation, set a system default to force a new default */

  return ic;
}

IUP_API Ihandle* IupColorBrowser(void)
{
  return IupCreate("colorbrowser");
}
