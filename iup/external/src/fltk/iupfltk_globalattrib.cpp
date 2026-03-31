/** \file
 * \brief FLTK Driver iupdrvSetGlobal/iupdrvGetGlobal
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/platform.H>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef _WIN32
#include <climits>
#endif

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_key.h"
#include "iup_singleinstance.h"
}

#include "iupfltk_drv.h"


extern "C" IUP_SDK_API int iupdrvSetGlobal(const char* name, const char* value)
{
  if (iupStrEqual(name, "FLTKTHEME"))
  {
    Fl::scheme(value);
    return 1;
  }

  if (iupStrEqual(name, "SINGLEINSTANCE"))
  {
    if (iupdrvSingleInstanceSet(value))
      return 0;
    else
      return 1;
  }

  if (iupStrEqual(name, "AUTOREPEAT"))
  {
#if defined(FLTK_USE_X11)
    if (iupfltkIsX11() && iupfltkX11Load() && iupfltk_XChangeKeyboardControl)
    {
      struct { int auto_repeat_mode; } values;
      values.auto_repeat_mode = iupStrBoolean(value) ? 1 : 0;
      iupfltk_XChangeKeyboardControl(fl_display, (1L << 7), &values);
    }
#endif
    return 0;
  }

  if (iupStrEqual(name, "LANGUAGE"))
    return 1;

  if (iupStrEqual(name, "UTF8MODE"))
    return 1;

  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
    return 0;

  return 1;
}

extern "C" IUP_SDK_API char* iupdrvGetGlobal(const char* name)
{
  if (iupStrEqual(name, "FLTKTHEME"))
  {
    const char* s = Fl::scheme();
    if (s)
      return iupStrReturnStr(s);
    return (char*)"none";
  }

  if (iupStrEqual(name, "SHOWMENUIMAGES"))
    return (char*)"NO";

  if (iupStrEqual(name, "TRUECOLORCANVAS"))
    return iupStrReturnBoolean(1);

  if (iupStrEqual(name, "UTF8MODE"))
    return iupStrReturnBoolean(1);

  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
    return iupStrReturnBoolean(0);

  if (iupStrEqual(name, "VIRTUALSCREEN"))
  {
    int x, y, w, h;
    Fl::screen_xywh(x, y, w, h);
    return iupStrReturnStrf("%d %d %d %d", x, y, w, h);
  }

  if (iupStrEqual(name, "MONITORSCOUNT"))
    return iupStrReturnInt(Fl::screen_count());

  if (iupStrEqual(name, "MONITORSINFO"))
  {
    int count = Fl::screen_count();
    char* str = iupStrGetMemory(count * 50);
    char* pstr = str;

    for (int i = 0; i < count; i++)
    {
      int x, y, w, h;
      Fl::screen_xywh(x, y, w, h, i);
      int remaining = count * 50 - (int)(pstr - str);
      if (remaining <= 0) break;
      pstr += snprintf(pstr, remaining, "%d %d %d %d\n", x, y, w, h);
    }

    return str;
  }

  if (iupStrEqual(name, "SANDBOX"))
  {
    if (getenv("FLATPAK_ID"))
      return (char*)"FLATPAK";
    if (getenv("SNAP"))
      return (char*)"SNAP";
    if (getenv("APPIMAGE"))
      return (char*)"APPIMAGE";
    return NULL;
  }

  if (iupStrEqual(name, "DARKMODE"))
  {
    unsigned char r, g, b;
    Fl::get_color(FL_BACKGROUND_COLOR, r, g, b);
    int luminance = (int)(0.299 * r + 0.587 * g + 0.114 * b);
    return iupStrReturnBoolean(luminance < 128);
  }

#ifndef _WIN32
  if (iupStrEqual(name, "EXEFILENAME"))
  {
    char* argv0 = IupGetGlobal("ARGV0");
    if (argv0)
    {
      char* exefilename = realpath(argv0, NULL);
      if (exefilename)
      {
        char* str = iupStrReturnStr(exefilename);
        free(exefilename);
        return str;
      }
    }
  }
#endif

  return NULL;
}
