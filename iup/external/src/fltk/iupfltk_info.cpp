/** \file
 * \brief FLTK System Information (toolkit-specific only)
 *
 * OS-level functions (system name, locale, directories, logging) are
 * provided by the shared iupunix_info.c / iupwindows_info.c files.
 * This file only has FLTK-specific screen/input functions.
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/platform.H>

#if defined(FLTK_USE_WAYLAND)
#include <FL/wayland.H>
#endif

#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_str.h"
#include "iup_object.h"
#include "iup_drvinfo.h"
}

#include "iupfltk_drv.h"


extern "C" IUP_SDK_API void iupdrvAddScreenOffset(int *x, int *y, int add)
{
  (void)x;
  (void)y;
  (void)add;
}

extern "C" IUP_SDK_API void iupdrvGetScreenSize(int *width, int *height)
{
  int x, y, w, h;
  Fl::screen_work_area(x, y, w, h);
  *width = w;
  *height = h;
}

extern "C" IUP_SDK_API void iupdrvGetFullSize(int *width, int *height)
{
  int x, y, w, h;
  Fl::screen_xywh(x, y, w, h);
  *width = w;
  *height = h;
}

extern "C" IUP_SDK_API int iupdrvGetScreenDepth(void)
{
  return 24;
}

extern "C" IUP_SDK_API double iupdrvGetScreenDpi(void)
{
  float h, v;
  Fl::screen_dpi(h, v);
  return (double)((h + v) / 2.0f);
}

extern "C" IUP_SDK_API void* iupdrvGetDisplay(void)
{
#if defined(FLTK_USE_WAYLAND)
  if (iupfltkIsWayland())
    return fl_wl_display();
#endif
#if defined(FLTK_USE_X11)
  if (iupfltkIsX11())
    return fl_display;
#endif
  return NULL;
}

extern "C" IUP_SDK_API void iupdrvGetCursorPos(int *x, int *y)
{
  Fl::get_mouse(*x, *y);
}

extern "C" IUP_SDK_API void iupdrvGetKeyState(char* key)
{
  int state = Fl::event_state();

  key[0] = (state & FL_SHIFT) ? 'S' : ' ';
  key[1] = (state & FL_CTRL) ? 'C' : ' ';
  key[2] = (state & FL_ALT) ? 'A' : ' ';
  key[3] = (state & FL_META) ? 'Y' : ' ';
  key[4] = 0;
}

extern "C" IUP_SDK_API void iupdrvSleep(int time)
{
  Fl_Timestamp start = Fl::now();

  while (Fl::seconds_since(start) * 1000.0 < (double)time)
  {
    double remaining = (time / 1000.0) - Fl::seconds_since(start);
    if (remaining <= 0) break;
    double wait = (remaining > 0.1) ? 0.1 : remaining;
    Fl::wait(wait);
  }
}
