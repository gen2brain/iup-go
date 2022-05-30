/** \file
 * \brief UNIX System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <limits.h>
#include <sys/stat.h>

/* This module should depend only on IUP core headers 
   and UNIX system headers. NO Motif headers allowed. */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "iup_export.h"
#include "iup_str.h"
#include "iup_drvinfo.h"


static int xGetWorkAreaSize(Display* display, int screen, int *width, int *height)
{
  /* _NET_WORKAREA, x, y, width, height CARDINAL[][4]/32 */
  static Atom workarea = 0;
  Atom type;
  long *data;
  int format;
  unsigned long after, ndata;

  if (!workarea)
    workarea = XInternAtom(display, "_NET_WORKAREA", False);

  XGetWindowProperty(display, RootWindow(display, screen),
                     workarea, 0, LONG_MAX, False, XA_CARDINAL, &type, &format, &ndata,
                     &after, (unsigned char **)&data);
  if (type != XA_CARDINAL || data == NULL)
  {
    if (data) XFree(data);
    return 0;
  }

  *width = data[2];    /* get only for the first desktop */
  *height = data[3];

  XFree(data);
  return 1;
}

IUP_SDK_API void iupdrvAddScreenOffset(int *x, int *y, int add)
{
  (void)x;
  (void)y;
  (void)add;
}

IUP_SDK_API void iupdrvGetScreenSize(int *width, int *height)
{
  Display* display = (Display*)iupdrvGetDisplay();
  int screen = XDefaultScreen(display);
  if (!xGetWorkAreaSize(display, screen, width, height))
  {
    *width = DisplayWidth(display, screen);
    *height = DisplayHeight(display, screen);
  }
}

IUP_SDK_API void iupdrvGetFullSize(int *width, int *height)
{
  Display* display = (Display*)iupdrvGetDisplay();
  int screen = XDefaultScreen(display);
  Window root = RootWindow(display, screen);
  XWindowAttributes wa;
  XGetWindowAttributes(display, root, &wa);
  *width = wa.width;
  *height = wa.height;
}

static int xCheckVisualInfo(Display* drv_display, int bpp)
{
  int nitems;
  XVisualInfo info, *ret_info;

  info.depth = bpp;
  ret_info = XGetVisualInfo(drv_display, VisualDepthMask, &info, &nitems);
  if (ret_info != NULL)
  {
    XFree(ret_info);
    return 1;
  }
  return 0;
}

IUP_SDK_API int iupdrvGetScreenDepth(void)
{
  static int first = 1;
  static int bpp;

  if (first)
  {
    Display* drv_display = (Display*)iupdrvGetDisplay();

    if (xCheckVisualInfo(drv_display, 24))
    {
      bpp = 24;
      return bpp;
    }

    if (xCheckVisualInfo(drv_display, 16))
    {
      bpp = 16;
      return bpp;
    }

    if (xCheckVisualInfo(drv_display, 8))
    {
      bpp = 8;
      return bpp;
    }

    if (xCheckVisualInfo(drv_display, 4))
    {
      bpp = 4;
      return bpp;
    }

    bpp = 2;

    first = 0;
  }

  return bpp;
}

IUP_SDK_API double iupdrvGetScreenDpi(void)
{
  double dpi;
  Display* drv_display = (Display*)iupdrvGetDisplay();
  int screen = XDefaultScreen(drv_display);
  dpi = ((double)DisplayHeight(drv_display, screen) / (double)DisplayHeightMM(drv_display, screen));
  return dpi * 25.4;  /* mm to inch */
}

IUP_SDK_API void iupdrvGetCursorPos(int *x, int *y)
{
  Window root, child;
  int cx, cy;
  unsigned int keys;
  Display* display = (Display*)iupdrvGetDisplay();
  int screen = XDefaultScreen(display);

  XQueryPointer(display, RootWindow(display, screen),
                &root, &child, x, y, &cx, &cy, &keys);
}

static int xCheckModifier(KeyCode* modifiermap, int max_keypermod, int index, const char* keys)
{
  int i;
  for (i = 0; i < max_keypermod; i++)
  {
    KeyCode key = modifiermap[max_keypermod * index + i];
    if (key)
    {
      int KeyIndex = key / 8;
      int KeyMask = 1 << (key % 8);
      if (keys[KeyIndex] & KeyMask)
        return 1;
    }
  }
  return 0;
}

IUP_SDK_API void iupdrvGetKeyState(char* key)
{
  char keys[32];
  Display* display = (Display*)iupdrvGetDisplay();
  XModifierKeymap *modMap = XGetModifierMapping(display);
  XQueryKeymap(display, keys);

  if (xCheckModifier(modMap->modifiermap, modMap->max_keypermod, ShiftMapIndex, keys))
    key[0] = 'S';
  else
    key[0] = ' ';
  if (xCheckModifier(modMap->modifiermap, modMap->max_keypermod, ControlMapIndex, keys))
    key[1] = 'C';
  else
    key[1] = ' ';
  if (xCheckModifier(modMap->modifiermap, modMap->max_keypermod, Mod1MapIndex, keys) ||
      xCheckModifier(modMap->modifiermap, modMap->max_keypermod, Mod5MapIndex, keys))
    key[2] = 'A';
  else
    key[2] = ' ';
  if (xCheckModifier(modMap->modifiermap, modMap->max_keypermod, Mod4MapIndex, keys))
    key[3] = 'Y';
  else
    key[3] = ' ';

  key[4] = 0;

  XFreeModifiermap(modMap);
}


/******************************************************************************/


#include <unistd.h>

IUP_SDK_API char *iupdrvGetComputerName(void)
{
  char* str = iupStrGetMemory(50);
  gethostname(str, 50);
  return str;
}

IUP_SDK_API char *iupdrvGetUserName(void)
{
  return (char*)getlogin();
}

