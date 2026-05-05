/** \file
 * \brief FLTK Driver iupdrvSetGlobal/iupdrvGetGlobal
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/platform.H>

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_key.h"
#include "iup_singleinstance.h"
}

#include "iupfltk_drv.h"

#ifdef IUPX11_USE_DLOPEN
#include "iupunix_x11.h"
#endif


static int fltkGlobalButton(int b)
{
  switch (b)
  {
    case FL_LEFT_MOUSE:   return IUP_BUTTON1;
    case FL_MIDDLE_MOUSE: return IUP_BUTTON2;
    case FL_RIGHT_MOUSE:  return IUP_BUTTON3;
    default:              return 0;
  }
}

/* Returning 0 lets FLTK continue its normal widget-tree dispatch. */
static int fltkGlobalEventHandler(int event)
{
  switch (event)
  {
  case FL_PUSH:
  case FL_RELEASE:
    {
      IFiiiis cb = (IFiiiis)IupGetFunction("GLOBALBUTTON_CB");
      if (cb)
      {
        int button = fltkGlobalButton(Fl::event_button());
        if (!button) break;
        int pressed = (event == FL_PUSH) ? 1 : 0;
        int doubleclick = (event == FL_PUSH && Fl::event_clicks() > 0) ? 1 : 0;
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
        iupfltkButtonKeySetStatus(Fl::event_state(), button, status, doubleclick);
        cb(button, pressed, Fl::event_x_root(), Fl::event_y_root(), status);
      }
      break;
    }
  case FL_MOVE:
  case FL_DRAG:
    {
      IFiis cb = (IFiis)IupGetFunction("GLOBALMOTION_CB");
      if (cb)
      {
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
        iupfltkButtonKeySetStatus(Fl::event_state(), 0, status, 0);
        cb(Fl::event_x_root(), Fl::event_y_root(), status);
      }
      break;
    }
  case FL_MOUSEWHEEL:
    {
      IFfiis cb = (IFfiis)IupGetFunction("GLOBALWHEEL_CB");
      if (cb)
      {
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
        iupfltkButtonKeySetStatus(Fl::event_state(), 0, status, 0);
        cb((float)-Fl::event_dy(), Fl::event_x_root(), Fl::event_y_root(), status);
      }
      break;
    }
  case FL_KEYDOWN:
  case FL_KEYUP:
    {
      IFii cb = (IFii)IupGetFunction("GLOBALKEYPRESS_CB");
      if (cb)
      {
        int code = iupfltkKeyDecode();
        if (code != 0)
          cb(code, (event == FL_KEYDOWN) ? 1 : 0);
      }
      break;
    }
  default:
    break;
  }
  return 0;
}

static int fltk_global_handler_installed = 0;

extern "C" IUP_SDK_API int iupdrvSetGlobal(const char* name, const char* value)
{
  if (iupStrEqual(name, "FLTKTHEME"))
  {
    Fl::scheme(value);
    return 1;
  }

  if (iupStrEqual(name, "INPUTCALLBACKS"))
  {
    if (iupStrBoolean(value))
    {
      if (!fltk_global_handler_installed)
      {
        Fl::add_handler(fltkGlobalEventHandler);
        fltk_global_handler_installed = 1;
      }
    }
    else
    {
      if (fltk_global_handler_installed)
      {
        Fl::remove_handler(fltkGlobalEventHandler);
        fltk_global_handler_installed = 0;
      }
    }
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
    if (iupfltkIsX11()
#ifdef IUPX11_USE_DLOPEN
        && iupX11Open()
#endif
        )
    {
      XKeyboardControl values;
      memset(&values, 0, sizeof(values));
      values.auto_repeat_mode = iupStrBoolean(value) ? AutoRepeatModeOn : AutoRepeatModeOff;
      XChangeKeyboardControl(fl_display, KBAutoRepeatMode, &values);
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
