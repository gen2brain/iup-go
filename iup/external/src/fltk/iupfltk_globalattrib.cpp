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

static void fltkGlobalEventHandler(int event)
{
  switch (event)
  {
  case FL_PUSH:
  case FL_RELEASE:
    {
      auto cb = reinterpret_cast<IFiiiis>(IupGetFunction("GLOBALBUTTON_CB"));
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
      auto cb = reinterpret_cast<IFiis>(IupGetFunction("GLOBALMOTION_CB"));
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
      auto cb = reinterpret_cast<IFfiis>(IupGetFunction("GLOBALWHEEL_CB"));
      if (cb)
      {
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
        iupfltkButtonKeySetStatus(Fl::event_state(), 0, status, 0);
        cb(static_cast<float>(-Fl::event_dy()), Fl::event_x_root(), Fl::event_y_root(), status);
      }
      break;
    }
  case FL_KEYDOWN:
  case FL_KEYUP:
    {
      IFii cb = reinterpret_cast<IFii>(IupGetFunction("GLOBALKEYPRESS_CB"));
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
}

static int fltk_input_callbacks = 0;

static int fltkEventDispatch(int event, Fl_Window* window)
{
  if (fltk_input_callbacks)
    fltkGlobalEventHandler(event);

  int ret = Fl::handle_(event, window);
  iupfltkTipsEvent(event);
  return ret;
}

IUP_DRV_API void iupfltkEventDispatchInstall(void)
{
  if (Fl::event_dispatch() != fltkEventDispatch)
    Fl::event_dispatch(fltkEventDispatch);
}

extern "C" IUP_SDK_API int iupdrvSetGlobal(const char* name, const char* value)
{
  if (iupStrEqual(name, "FLTKTHEME"))
  {
    Fl::scheme(value);
    return 1;
  }

  if (iupStrEqual(name, "INPUTCALLBACKS"))
  {
    fltk_input_callbacks = iupStrBoolean(value);
    if (fltk_input_callbacks)
      iupfltkEventDispatchInstall();
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
    return const_cast<char*>("none");
  }

  if (iupStrEqual(name, "SHOWMENUIMAGES"))
    return const_cast<char*>("NO");

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
      int remaining = count * 50 - static_cast<int>(pstr - str);
      if (remaining <= 0) break;
      pstr += snprintf(pstr, remaining, "%d %d %d %d\n", x, y, w, h);
    }

    return str;
  }

  if (iupStrEqual(name, "SANDBOX"))
  {
    if (getenv("FLATPAK_ID"))
      return const_cast<char*>("FLATPAK");
    if (getenv("SNAP"))
      return const_cast<char*>("SNAP");
    if (getenv("APPIMAGE"))
      return const_cast<char*>("APPIMAGE");
    return nullptr;
  }

  return nullptr;
}
