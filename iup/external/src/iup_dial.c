/** \file
 * \brief Dial Control.
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


#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

#define IUP_RAD2DEG  57.295779513   /* radians to degrees (deg = IUP_RAD2DEG * rad) */

#define IDIAL_SPACE 3   /* margin to the contents, includes the focus line */
#define IDIAL_MARK 4    /* circular mark */
#define IDIAL_NCOLORS 10
#define IDIAL_DEFAULT_DENSITY 0.2
#define IDIAL_DEFAULT_FGCOLOR "64 64 64"

enum{ IDIAL_VERTICAL, IDIAL_HORIZONTAL, IDIAL_CIRCULAR };

#define dialmin(a,b)  ((a)<(b)?(a):(b))

struct _IcontrolData
{
  iupCanvas canvas;  /* from IupCanvas (must reserve it) */

  /* attributes */
  double angle;
  int orientation;
  double unit;
  double density;

  /* mouse interaction control */
  int px;
  int py;
  int pressing;

  /* visual appearance control */
  void(*Draw)(Ihandle* ih);
  int w, h;
  int has_focus;
  int num_div;      /* number of sections in the dial wheel */
  double radius;    /* radius size of the dial wheel */
  long fgcolor[IDIAL_NCOLORS + 1];
  long bgcolor,
    light_shadow,
    mid_shadow,
    dark_shadow;
  int flat;
  long flatcolor;
};

static long iDialGetFgColor(Ihandle* ih, double a)
{
  double half = 0.5 * M_PI;
  double fr = fabs(a - half) / half;
  int i = (int)(IDIAL_NCOLORS * fr + 0.5);
  return ih->data->fgcolor[IDIAL_NCOLORS - i];
}

static void iDialDrawVerticalShading(Ihandle* ih, int *ymin, int *ymax)
{
  int border = ih->data->flat ? 1 : 2;
  double delta = (0.5 * M_PI - 0.0) / IDIAL_NCOLORS;
  double a, yc = ih->data->h / 2.0;
  *ymin = *ymax = ih->data->h / 2;
  for (a = 0.0; a < 0.5 * M_PI; a += delta)
  {
    int y0 = (int)(yc + ih->data->radius * cos(a + delta));
    int y1 = (int)(yc + ih->data->radius * cos(a));
    long fgcolor = iDialGetFgColor(ih, a);
    iupAttribSet(ih, "DRAWSTYLE", "FILL");
    iupDrawSetColor(ih, "DRAWCOLOR", fgcolor);
    IupDrawRectangle(ih, IDIAL_SPACE + 1, y0, ih->data->w - 1 - IDIAL_SPACE - border, y1);

    if (y1 > *ymax) *ymax = y1;

    if (abs(y1 - y0) < 2)
      continue;
  }
  for (a = 0.5 * M_PI; a < M_PI; a += delta)
  {
    int y0 = (int)(yc - ih->data->radius * fabs(cos(a + delta)));
    int y1 = (int)(yc - ih->data->radius * fabs(cos(a)));
    long fgcolor = iDialGetFgColor(ih, a);
    iupAttribSet(ih, "DRAWSTYLE", "FILL");
    iupDrawSetColor(ih, "DRAWCOLOR", fgcolor);
    IupDrawRectangle(ih, IDIAL_SPACE + 1, y0, ih->data->w - 1 - IDIAL_SPACE - border, y1);

    if (y0 < *ymin) *ymin = y0;

    if (abs(y1 - y0) < 2)
      continue;
  }
}

static void iDialDrawVertical(Ihandle* ih)
{
  int border = ih->data->flat ? 1 : 2;
  double delta = 2.0 * M_PI / ih->data->num_div;
  double a;
  int ymin, ymax;

  ih->data->radius = (ih->data->h - 2 * IDIAL_SPACE - 1 - border) / 2.0;

  if (ih->data->angle < 0.0)
  {
    for (a = ih->data->angle; a < 0.0; a += delta)
      ;
  }
  else
  {
    for (a = ih->data->angle; a > 0.0; a -= delta)
      ;
    a += delta;
  }

  iDialDrawVerticalShading(ih, &ymin, &ymax);

  if (ih->data->flat)
  {
    iupAttribSet(ih, "DRAWSTYLE", "STROKE");
    iupDrawSetColor(ih, "DRAWCOLOR", ih->data->flatcolor);
    IupDrawRectangle(ih, IDIAL_SPACE, ymin, ih->data->w - 1 - IDIAL_SPACE, ymax);
  }
  else
    iupDrawRaiseRect(ih, IDIAL_SPACE, ymin, ih->data->w - 1 - IDIAL_SPACE, ymax,
                     ih->data->light_shadow, ih->data->mid_shadow, ih->data->dark_shadow);

  for (; a < M_PI; a += delta)    /* graduation */
  {
    int y;
    if (a < 0.5 * M_PI) y = (int)(ih->data->h / 2.0 + ih->data->radius * cos(a));
    else                y = (int)(ih->data->h / 2.0 - ih->data->radius * fabs(cos(a)));

    if (abs(y - ymin) < 3 || abs(ymax - y) < 3)
      continue;

    iupDrawHorizSunkenMark(ih, IDIAL_SPACE + 1, ih->data->w - 1 - IDIAL_SPACE - border, y,
                           ih->data->light_shadow, ih->data->dark_shadow);
  }
}

static void iDialDrawHorizontalShading(Ihandle* ih, int *xmin, int *xmax)
{
  long fgcolor;
  int border = ih->data->flat ? 1 : 2;
  double delta = (0.5 * M_PI - 0.0) / IDIAL_NCOLORS;
  double a, xc = ih->data->w / 2.0;
  *xmin = *xmax = ih->data->w / 2;
  for (a = 0.0; a < 0.5 * M_PI; a += delta)
  {

    int x0 = (int)(xc - ih->data->radius * cos(a));
    int x1 = (int)(xc - ih->data->radius * cos(a + delta));  /* x1 is always bigger than x0 here (cos is decreasing) */
    iupAttribSet(ih, "DRAWSTYLE", "FILL");
    fgcolor = iDialGetFgColor(ih, a);
    iupDrawSetColor(ih, "DRAWCOLOR", fgcolor);
    IupDrawRectangle(ih, x0, IDIAL_SPACE + 1, x1, ih->data->h - 1 - IDIAL_SPACE - border);

    if (x0 < *xmin) *xmin = x0;

    if (abs(x1 - x0) < 2)
      continue;
  }
  for (a = 0.5 * M_PI; a < M_PI; a += delta)
  {
    int x0 = (int)(xc + ih->data->radius * fabs(cos(a)));
    int x1 = (int)(xc + ih->data->radius * fabs(cos(a + delta)));  /* x1 is always bigger than x0 here (abs(cos) is increasing) */
    iupAttribSet(ih, "DRAWSTYLE", "FILL");
    fgcolor = iDialGetFgColor(ih, a);
    iupDrawSetColor(ih, "DRAWCOLOR", fgcolor);
    IupDrawRectangle(ih, x0, IDIAL_SPACE + 1, x1, ih->data->h - 1 - IDIAL_SPACE - border);

    if (x1 > *xmax) *xmax = x1;

    if (abs(x1 - x0) < 2)
      continue;
  }
}

static void iDialDrawHorizontal(Ihandle* ih)
{
  int border = ih->data->flat ? 1 : 2;
  double delta = 2.0 * M_PI / ih->data->num_div;
  double a;
  int xmin, xmax;

  ih->data->radius = (ih->data->w - 2 * IDIAL_SPACE - 1 - border) / 2.0;

  if (ih->data->angle < 0.0)
  {
    for (a = ih->data->angle; a < 0.0; a += delta)
      ;
  }
  else
  {
    for (a = ih->data->angle; a > 0.0; a -= delta)
      ;
    a += delta;
  }

  iDialDrawHorizontalShading(ih, &xmin, &xmax);

  if (ih->data->flat)
  {
    iupAttribSet(ih, "DRAWSTYLE", "STROKE");
    iupDrawSetColor(ih, "DRAWCOLOR", ih->data->flatcolor);
    IupDrawRectangle(ih, xmin, IDIAL_SPACE, xmax, ih->data->h - 1 - IDIAL_SPACE);
  }
  else
    iupDrawRaiseRect(ih, xmin, IDIAL_SPACE, xmax, ih->data->h - 1 - IDIAL_SPACE,
                     ih->data->light_shadow, ih->data->mid_shadow, ih->data->dark_shadow);

  for (; a < M_PI; a += delta)
  {
    int x;
    if (a < 0.5 * M_PI) x = (int)(ih->data->w / 2.0 - ih->data->radius * cos(a));
    else                x = (int)(ih->data->w / 2.0 + ih->data->radius * fabs(cos(a)));

    if (abs(x - xmin) < 3 || abs(xmax - x) < 3)
      continue;

    iupDrawVertSunkenMark(ih, x, IDIAL_SPACE + 1, ih->data->h - 1 - IDIAL_SPACE - border,
                          ih->data->light_shadow, ih->data->dark_shadow);
  }
}

static void iDialDrawCircularMark(Ihandle* ih, int xc, int yc)
{
  int x1 = xc - IDIAL_MARK / 2;
  int y1 = yc - IDIAL_MARK / 2;
  int x2 = xc + IDIAL_MARK / 2;
  int y2 = yc + IDIAL_MARK / 2;

  iupDrawSetColor(ih, "DRAWCOLOR", ih->data->fgcolor[0]);
  IupDrawRectangle(ih, x1 + 1, y1 + 1, x2 - 1, y2 - 1);

  iupDrawSetColor(ih, "DRAWCOLOR", ih->data->light_shadow);
  IupDrawLine(ih, x1, y1 + 1, x1, y2 - 1);
  IupDrawLine(ih, x1 + 1, y1, x2 - 1, y1);

  iupDrawSetColor(ih, "DRAWCOLOR", ih->data->mid_shadow);
  IupDrawLine(ih, x1 + 1, y2, x2, y2);
  IupDrawLine(ih, x2, y1 + 1, x2, y2);

  iupDrawSetColor(ih, "DRAWCOLOR", ih->data->dark_shadow);
  IupDrawLine(ih, x1 + 2, y2, x2 - 1, y2);
  IupDrawLine(ih, x2, y1 + 2, x2, y2 - 1);
}

static void iDialDrawCircular(Ihandle* ih)
{
  int border = ih->data->flat ? 1 : 2;
  double delta = 2.0 * M_PI / ih->data->num_div,
         a = ih->data->angle;
  int i, xc = ih->data->w / 2, 
         yc = ih->data->h / 2,
         r;
  int x1, y1, x2, y2;

  ih->data->radius = (dialmin(ih->data->w, ih->data->h) - 2 * IDIAL_SPACE) / 2.0;
  r = (int)ih->data->radius;

  x1 = xc - r;
  y1 = yc - r;
  x2 = xc + r;
  y2 = yc + r;

  iupAttribSet(ih, "DRAWSTYLE", "FILL");
  iupDrawSetColor(ih, "DRAWCOLOR", ih->data->bgcolor);
  IupDrawArc(ih, x1, y1, x2, y2, 0.0, 360.0);

  iupAttribSet(ih, "DRAWSTYLE", "STROKE");

  if (ih->data->flat)
  {
    iupAttribSet(ih, "DRAWLINEWIDTH", "1");
    iupDrawSetColor(ih, "DRAWCOLOR", ih->data->flatcolor);
    IupDrawArc(ih, x1, y1, x2, y2, 0.0, 360.0);
  }
  else
  {
    iupAttribSet(ih, "DRAWLINEWIDTH", "2");
    iupDrawSetColor(ih, "DRAWCOLOR", ih->data->mid_shadow);
    IupDrawArc(ih, x1, y1, x2, y2, -135, 45.0);
    iupDrawSetColor(ih, "DRAWCOLOR", ih->data->light_shadow);
    IupDrawArc(ih, x1, y1, x2, y2, 45, 225);
    iupAttribSet(ih, "DRAWLINEWIDTH", "1");
    iupDrawSetColor(ih, "DRAWCOLOR", ih->data->dark_shadow);
    IupDrawArc(ih, x1, y1, x2, y2, -135, 45);
  }

  ih->data->radius -= border + IDIAL_SPACE + IDIAL_MARK/2;

  iupAttribSet(ih, "DRAWSTYLE", "FILL");

  for (i = 0; i < ih->data->num_div; ++i)
  {
    x2 = (int)(xc + ih->data->radius * cos(a));
    y2 = (int)(yc - ih->data->radius * sin(a));

    if (i == 0)
    {
      iupDrawSetColor(ih, "DRAWCOLOR", ih->data->fgcolor[0]);
      IupDrawLine(ih, xc, yc, x2, y2);
    }

    iDialDrawCircularMark(ih, x2, y2);
    a += delta;
  }

  iDialDrawCircularMark(ih, xc, yc);
}

static int iDialRedraw_CB(Ihandle* ih)
{
  IupDrawBegin(ih);

  IupDrawParentBackground(ih);

  ih->data->Draw(ih);

  if (ih->data->has_focus)
    IupDrawFocusRect(ih, 0, 0, ih->data->w - 1, ih->data->h - 1);

  IupDrawEnd(ih);

  return IUP_DEFAULT;
}

static int iDialUpdateFgColors(Ihandle* ih, const char* fgcolor)
{
  int i, max, deltar, deltag, deltab, active = 1;
  unsigned char r, g, b;

  if (!iupStrToRGB(fgcolor, &r, &g, &b))
    return 0;

  if (ih->handle && !iupdrvIsActive(ih))
    active = 0;

  /* this function is also called before mapping */
  max = active? 255: 192;
  deltar = (max - r) / IDIAL_NCOLORS;
  deltag = (max - g) / IDIAL_NCOLORS;
  deltab = (max - b) / IDIAL_NCOLORS;

  for (i = 0; i <= IDIAL_NCOLORS; i++)
  {
    ih->data->fgcolor[i] = iupDrawColor(r, g, b, 255);
    r = (unsigned char)(r + deltar);
    g = (unsigned char)(g + deltag);
    b = (unsigned char)(b + deltab);
  }

  return 1;
}

static int iDialButtonPress(Ihandle* ih, int button, int x, int y)
{
  IFn cb;

  if (button != IUP_BUTTON1)
    return IUP_DEFAULT;

  ih->data->px = x;
  ih->data->py = y;

  if (ih->data->orientation != IDIAL_CIRCULAR)
    ih->data->angle = 0;

  cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
    cb(ih);
  else
  {
    IFnd cb_old = (IFnd)IupGetCallback(ih, "BUTTON_PRESS_CB");
    if (cb_old)
      cb_old(ih, ih->data->angle * ih->data->unit);
  }

  return IUP_DEFAULT;
}

static int iDialButtonRelease(Ihandle* ih, int button)
{
  IFn cb;

  if (button != IUP_BUTTON1)
    return IUP_DEFAULT;

  IupUpdate(ih);

  cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
    cb(ih);
  else
  {
    IFnd cb_old = (IFnd)IupGetCallback(ih, "BUTTON_RELEASE_CB");
    if (cb_old)
      cb_old(ih, ih->data->angle * ih->data->unit);
  }

  return IUP_DEFAULT;
}


/******************************************************************/


static int iDialMotionVertical_CB(Ihandle* ih, int x, int y, char *status)
{
  IFn cb;
  (void)x; /* not used */

  if (!iup_isbutton1(status))
    return IUP_DEFAULT;

  ih->data->angle += (double)(ih->data->py - y) / ih->data->radius;
  ih->data->py = y;

  IupUpdate(ih);

  cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
    cb(ih);
  else
  {
    IFnd cb_old = (IFnd)IupGetCallback(ih, "MOUSEMOVE_CB");
    if (cb_old)
      cb_old(ih, ih->data->angle * ih->data->unit);
  }

  return IUP_DEFAULT;
}

static int iDialMotionHorizontal_CB(Ihandle* ih, int x, int y, char *status)
{
  IFn cb;
  (void)y;

  if (!iup_isbutton1(status))
    return IUP_DEFAULT;

  ih->data->angle += (double)(x - ih->data->px) / ih->data->radius;
  ih->data->px = x;

  IupUpdate(ih);

  cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
    cb(ih);
  else
  {
    IFnd cb_old = (IFnd)IupGetCallback(ih, "MOUSEMOVE_CB");
    if (cb_old)
      cb_old(ih, ih->data->angle * ih->data->unit);
  }

  return IUP_DEFAULT;
}

static int iDialMotionCircular_CB(Ihandle* ih, int x, int y, char *status)
{
  int cx = ih->data->w / 2;
  int cy = ih->data->h / 2;
  double vet, xa, ya, xb, yb, ma, mb, ab;
  IFn cb;

  if (!iup_isbutton1(status))
    return IUP_DEFAULT;

  xa = ih->data->px - cx;
  ya = ih->data->py - cy;
  ma = sqrt(xa * xa + ya * ya);

  xb = x - cx;
  yb = y - cy;
  mb = sqrt(xb * xb + yb * yb);

  ab = xa * xb + ya * yb;
  vet = xa * yb - xb * ya;

  ab = ab / (ma * mb);

  /* if the mouse is in the center of the dial, ignore it */
  if (ma == 0 || mb == 0 || ab < -1 || ab > 1)
    return IUP_DEFAULT;

  if (vet > 0) ih->data->angle -= acos(ab);
  else         ih->data->angle += acos(ab);

  IupUpdate(ih);

  ih->data->px = x;
  ih->data->py = y;

  cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
    cb(ih);
  else
  {
    IFnd cb_old = (IFnd)IupGetCallback(ih, "MOUSEMOVE_CB");
    if (cb_old)
      cb_old(ih, ih->data->angle * ih->data->unit);
  }

  return IUP_DEFAULT;
}

static int iDialButton_CB(Ihandle* ih, int button, int pressed, int x, int y)
{
  if (pressed)
    return iDialButtonPress(ih, button, x, y);
  else
    return iDialButtonRelease(ih, button);
}

static int iDialResize_CB(Ihandle* ih, int w, int h)
{
  int border = ih->data->flat ? 1 : 2;
  int total_size = 0;

  ih->data->w = w;
  ih->data->h = h;

  /* update number of divisions */
  switch (ih->data->orientation)
  {
    case IDIAL_VERTICAL:
      total_size = ih->data->h;
      break;
    case IDIAL_HORIZONTAL:
      total_size = ih->data->w;
      break;
    case IDIAL_CIRCULAR:
      total_size = dialmin(ih->data->w, ih->data->h);
      break;
  }

  ih->data->num_div = (int)((total_size - 2 * IDIAL_SPACE - 1 - border) * ih->data->density);

  if (ih->data->num_div < 3) 
    ih->data->num_div = 3;

  return IUP_DEFAULT;
}

static int iDialFocus_CB(Ihandle* ih, int focus)
{
  ih->data->has_focus = focus;
  IupUpdate(ih);
  return IUP_DEFAULT;
}

static int iDialKeyPress_CB(Ihandle* ih, int c, int press)
{
  IFn cb;
  char* cb_name;

  if (c != K_LEFT   && c != K_UP   &&
      c != K_sLEFT  && c != K_sUP  &&
      c != K_RIGHT  && c != K_DOWN &&
      c != K_sRIGHT && c != K_sDOWN &&
      c != K_HOME)
      return IUP_DEFAULT;

  if (press && ih->data->pressing)
  {
    switch (c)
    {
      case K_sLEFT:
      case K_sDOWN:
        ih->data->angle -= M_PI / 100.0;
        break;
      case K_LEFT:
      case K_DOWN:
        ih->data->angle -= M_PI / 10.0;
        break;
      case K_sRIGHT:
      case K_sUP:
        ih->data->angle += M_PI / 100.0;
        break;
      case K_RIGHT:
      case K_UP:
        ih->data->angle += M_PI / 10.0;
        break;
    }
  }

  if (c == K_HOME)
    ih->data->angle = 0;

  if (press)
  {
    if (ih->data->pressing)
    {
      cb_name = "MOUSEMOVE_CB";
    }
    else
    {
      ih->data->pressing = 1;
      if (ih->data->orientation != IDIAL_CIRCULAR)
        ih->data->angle = 0;
      cb_name = "BUTTON_PRESS_CB";
    }
  }
  else
  {
    ih->data->pressing = 0;
    cb_name = "RELEASE_CB";
  }

  IupUpdate(ih);

  cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
    cb(ih);
  else
  {
    IFnd cb_old = (IFnd)IupGetCallback(ih, cb_name);
    if (cb_old)
      cb_old(ih, ih->data->angle * ih->data->unit);
  }

  return IUP_IGNORE;  /* to avoid arrow keys being processed by the system */
}

static int iDialWheel_CB(Ihandle* ih, float delta)
{
  IFn cb;

  ih->data->angle += ((double)delta) * (M_PI / 10.0);

  if (fabs(ih->data->angle) < M_PI / 10.1)
    ih->data->angle = 0;

  IupUpdate(ih);

  cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
    cb(ih);
  else
  {
    IFnd cb_old = (IFnd)IupGetCallback(ih, "MOUSEMOVE_CB");
    if (cb_old)
      cb_old(ih, ih->data->angle * ih->data->unit);
  }

  return IUP_DEFAULT;
}


/*********************************************************************************/


static char* iDialGetValueAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->angle);
}

static int iDialSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->angle), 0.0))
    IupUpdate(ih);
  return 0; /* do not store value in hash table */
}

static int iDialSetDensityAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->density), 0.2))
    IupUpdate(ih);
  return 0;   /* do not store value in hash table */
}

static char* iDialGetDensityAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->density);
}

static int iDialSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    value = iupBaseNativeParentGetBgColorAttrib(ih);

  ih->data->bgcolor = iupDrawStrToColor(value, ih->data->bgcolor);

  iupDrawCalcShadows(ih->data->bgcolor, &ih->data->light_shadow, &ih->data->mid_shadow, &ih->data->dark_shadow);

  if (!iupdrvIsActive(ih))
    ih->data->light_shadow = ih->data->mid_shadow;

  IupUpdate(ih);
  return 1;
}

static int iDialSetFgColorAttrib(Ihandle* ih, const char* value)
{
  if (!iDialUpdateFgColors(ih, value))
    return 0;

  IupUpdate(ih);
  return 1;
}

static int iDialSetActiveAttrib(Ihandle* ih, const char* value)
{
  iupBaseSetActiveAttrib(ih, value);

  if (!iDialUpdateFgColors(ih, iupAttribGetStr(ih, "FGCOLOR")))
    return 0;

  iupDrawCalcShadows(ih->data->bgcolor, &ih->data->light_shadow, &ih->data->mid_shadow, &ih->data->dark_shadow);

  if (!iupdrvIsActive(ih))
    ih->data->light_shadow = ih->data->mid_shadow;

  IupUpdate(ih);
  return 0;   /* do not store value in hash table */
}

static int iDialSetUnitAttrib(Ihandle* ih, const char* value)
{
  ih->data->unit = iupStrEqualNoCase(value, "DEGREES") ? IUP_RAD2DEG : 1.0;
  return 1;
}

static int iDialSetOrientationAttrib(Ihandle* ih, const char* value)
{
  /* valid only before map */
  if (ih->handle)
    return 0;

  if (iupStrEqualNoCase(value, "VERTICAL"))
  {
    ih->data->Draw = iDialDrawVertical;
    ih->data->orientation = IDIAL_VERTICAL;
    IupSetCallback(ih, "MOTION_CB", (Icallback)iDialMotionVertical_CB);
    IupSetAttribute(ih, "SIZE", "16x80");
  }
  else if (iupStrEqualNoCase(value, "CIRCULAR"))
  {
    ih->data->Draw = iDialDrawCircular;
    ih->data->orientation = IDIAL_CIRCULAR;
    IupSetCallback(ih, "MOTION_CB", (Icallback)iDialMotionCircular_CB);
    IupSetAttribute(ih, "SIZE", "40x36");
  }
  else /* "HORIZONTAL" */
  {
    ih->data->Draw = iDialDrawHorizontal;
    ih->data->orientation = IDIAL_HORIZONTAL;
    IupSetCallback(ih, "MOTION_CB", (Icallback)iDialMotionHorizontal_CB);
    IupSetAttribute(ih, "SIZE", "80x16");
  }
  return 0; /* do not store value in hash table */
}

static char* iDialGetOrientationAttrib(Ihandle* ih)
{
  if (ih->data->orientation == IDIAL_HORIZONTAL)
    return "HORIZONTAL";
  else if (ih->data->orientation == IDIAL_VERTICAL)
    return "VERTICAL";
  else /* (ih->data->orientation == IDIAL_CIRCULAR) */
    return "CIRCULAR";
}

static int iDialSetFlatAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->flat = 1;
  else
    ih->data->flat = 0;

  IupUpdate(ih);
  return 0; /* do not store value in hash table */
}

static char* iDialGetFlatAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->flat);
}

static int iDialSetFlatColorAttrib(Ihandle* ih, const char* value)
{
  ih->data->flatcolor = iupDrawStrToColor(value, ih->data->flatcolor);
  IupUpdate(ih);
  return 1;
}


/****************************************************************************/


static int iDialCreateMethod(Ihandle* ih, void **params)
{
  char* orientation = "HORIZONTAL";
  if (params && params[0])
    orientation = params[0];

  /* free the data allocated by IupCanvas */
  free(ih->data);
  ih->data = iupALLOCCTRLDATA();

  /* change the IupCanvas default values */
  IupSetAttribute(ih, "EXPAND", "NO");

  /* default values */
  iDialSetOrientationAttrib(ih, orientation);
  ih->data->density = IDIAL_DEFAULT_DENSITY;
  ih->data->unit = 1.0;  /* RADIANS */
  ih->data->num_div = 3;
  iDialUpdateFgColors(ih, IDIAL_DEFAULT_FGCOLOR);
  ih->data->flatcolor = iupDrawColor(160, 160, 160, 255);

  /* IupCanvas callbacks */
  IupSetCallback(ih, "ACTION", (Icallback)iDialRedraw_CB);
  IupSetCallback(ih, "RESIZE_CB", (Icallback)iDialResize_CB);
  IupSetCallback(ih, "BUTTON_CB", (Icallback)iDialButton_CB);
  IupSetCallback(ih, "FOCUS_CB", (Icallback)iDialFocus_CB);
  IupSetCallback(ih, "KEYPRESS_CB", (Icallback)iDialKeyPress_CB);
  IupSetCallback(ih, "WHEEL_CB", (Icallback)iDialWheel_CB);

  return IUP_NOERROR;
}

Iclass* iupDialNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name = "dial";
  ic->format = "s"; /* one string */
  ic->format_attr = "ORIENTATION";
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupDialNewClass;
  ic->Create = iDialCreateMethod;

  /* Do not need to set base attributes because they are inherited from IupCanvas */

  /* IupDial Callbacks */
  iupClassRegisterCallback(ic, "MOUSEMOVE_CB", "d");
  iupClassRegisterCallback(ic, "BUTTON_PRESS_CB", "d");
  iupClassRegisterCallback(ic, "BUTTON_RELEASE_CB", "d");
  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  /* replace IupCanvas behavior */
  iupClassRegisterReplaceAttribDef(ic, "BORDER", "NO", NULL);
  iupClassRegisterReplaceAttribFlags(ic, "BORDER", IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* IupDial only */
  iupClassRegisterAttribute(ic, "VALUE", iDialGetValueAttrib, iDialSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TYPE", iDialGetOrientationAttrib, iDialSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", iDialGetOrientationAttrib, iDialSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DENSITY", iDialGetDensityAttrib, iDialSetDensityAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iDialSetFgColorAttrib, IDIAL_DEFAULT_FGCOLOR, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "UNIT", NULL, iDialSetUnitAttrib, IUPAF_SAMEASSYSTEM, "RADIANS", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FLAT", iDialGetFlatAttrib, iDialSetFlatAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FLATCOLOR", NULL, iDialSetFlatColorAttrib, IUPAF_SAMEASSYSTEM, "160 160 160", IUPAF_NOT_MAPPED);

  /* Overwrite IupCanvas Attributes */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iDialSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iDialSetBgColorAttrib, NULL, "255 255 255", IUPAF_NO_INHERIT);    /* overwrite canvas implementation, set a system default to force a new default */

  return ic;
}

IUP_API Ihandle* IupDial(const char* orientation)
{
  void *params[2];
  params[0] = (void*)orientation;
  params[1] = NULL;
  return IupCreatev("dial", params);
}
