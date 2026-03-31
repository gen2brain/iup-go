/** \file
 * \brief FLTK Driver Core - Initialization and Setup
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <clocale>
#include <cstdint>

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/platform.H>

#if defined(FLTK_USE_WAYLAND)
#include <FL/wayland.H>
#endif

extern "C" {
#include "iup.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_object.h"
#include "iup_globalattrib.h"
}

#include "iupfltk_drv.h"

#if !defined(_WIN32) && !defined(__APPLE__)
#include <dlfcn.h>

static void* x11_lib = NULL;

char* (*iupfltk_XServerVendor)(void*) = NULL;
int (*iupfltk_XVendorRelease)(void*) = NULL;
int (*iupfltk_XWarpPointer)(void*, unsigned long, unsigned long, int, int, unsigned int, unsigned int, int, int) = NULL;
int (*iupfltk_XChangeKeyboardControl)(void*, unsigned long, void*) = NULL;
unsigned long (*iupfltk_XRootWindow)(void*, int) = NULL;
unsigned long (*iupfltk_XInternAtom)(void*, const char*, int) = NULL;
int (*iupfltk_XDeleteProperty)(void*, unsigned long, unsigned long) = NULL;
int (*iupfltk_XSendEvent)(void*, unsigned long, int, long, void*) = NULL;

IUP_DRV_API int iupfltkX11Load(void)
{
  if (x11_lib)
    return 1;

  x11_lib = dlopen("libX11.so.6", RTLD_LAZY);
  if (!x11_lib)
    x11_lib = dlopen("libX11.so", RTLD_LAZY);
  if (!x11_lib)
    return 0;

  iupfltk_XServerVendor = (char* (*)(void*))dlsym(x11_lib, "XServerVendor");
  iupfltk_XVendorRelease = (int (*)(void*))dlsym(x11_lib, "XVendorRelease");
  iupfltk_XWarpPointer = (int (*)(void*, unsigned long, unsigned long, int, int, unsigned int, unsigned int, int, int))dlsym(x11_lib, "XWarpPointer");
  iupfltk_XChangeKeyboardControl = (int (*)(void*, unsigned long, void*))dlsym(x11_lib, "XChangeKeyboardControl");
  iupfltk_XRootWindow = (unsigned long (*)(void*, int))dlsym(x11_lib, "XRootWindow");
  iupfltk_XInternAtom = (unsigned long (*)(void*, const char*, int))dlsym(x11_lib, "XInternAtom");
  iupfltk_XDeleteProperty = (int (*)(void*, unsigned long, unsigned long))dlsym(x11_lib, "XDeleteProperty");
  iupfltk_XSendEvent = (int (*)(void*, unsigned long, int, long, void*))dlsym(x11_lib, "XSendEvent");

  return 1;
}

IUP_DRV_API void iupfltkX11Release(void)
{
  if (x11_lib)
  {
    dlclose(x11_lib);
    x11_lib = NULL;
  }
  iupfltk_XServerVendor = NULL;
  iupfltk_XVendorRelease = NULL;
  iupfltk_XWarpPointer = NULL;
  iupfltk_XChangeKeyboardControl = NULL;
  iupfltk_XRootWindow = NULL;
  iupfltk_XInternAtom = NULL;
  iupfltk_XDeleteProperty = NULL;
  iupfltk_XSendEvent = NULL;
}
#else
IUP_DRV_API int iupfltkX11Load(void) { return 0; }
IUP_DRV_API void iupfltkX11Release(void) {}
#endif

IUP_DRV_API int iupfltkIsX11(void)
{
#if defined(FLTK_USE_WAYLAND)
  if (fl_wl_display())
    return 0;
#endif
#if defined(FLTK_USE_X11)
  return fl_display != NULL;
#else
  return 0;
#endif
}

IUP_DRV_API int iupfltkIsWayland(void)
{
#if defined(FLTK_USE_WAYLAND)
  return fl_wl_display() != NULL;
#else
  return 0;
#endif
}

/****************************************************************************
 * Native Handle Access
 ****************************************************************************/

IUP_DRV_API char* iupfltkGetNativeWindowHandle(Fl_Window* window)
{
  if (!window || !window->shown())
    return NULL;

#if defined(FLTK_USE_WAYLAND)
  if (iupfltkIsWayland())
  {
    struct wld_window* wld = fl_wl_xid(window);
    if (wld)
      return (char*)fl_wl_surface(wld);
    return NULL;
  }
#endif

#if defined(FLTK_USE_X11) || defined(_WIN32) || defined(__APPLE__)
  return (char*)(uintptr_t)fl_xid(window);
#else
  return NULL;
#endif
}

IUP_DRV_API char* iupfltkGetNativeWindowHandleAttrib(Ihandle* ih)
{
  if (!ih->handle)
    return NULL;

  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  Fl_Window* window = widget->as_window();
  if (!window)
    window = widget->window();

  return iupfltkGetNativeWindowHandle(window);
}

IUP_DRV_API const char* iupfltkGetNativeWindowHandleName(void)
{
#if defined(_WIN32)
  return "HWND";
#elif defined(__APPLE__)
  return "NSVIEW";
#elif defined(FLTK_USE_WAYLAND)
  if (iupfltkIsWayland())
    return "WL_SURFACE";
  return "XWINDOW";
#else
  return "XWINDOW";
#endif
}

IUP_DRV_API const char* iupfltkGetNativeFontIdName(void)
{
#if defined(_WIN32)
  return "HFONT";
#elif defined(FLTK_USE_X11)
  return "XFONTID";
#else
  return NULL;
#endif
}

/****************************************************************************
 * Global Colors
 ****************************************************************************/

IUP_DRV_API void iupfltkSetGlobalColors(void)
{
  unsigned char r, g, b;

  Fl::get_color(FL_BACKGROUND_COLOR, r, g, b);
  iupGlobalSetDefaultColorAttrib("DLGBGCOLOR", r, g, b);

  Fl::get_color(FL_FOREGROUND_COLOR, r, g, b);
  iupGlobalSetDefaultColorAttrib("DLGFGCOLOR", r, g, b);

  Fl::get_color(FL_BACKGROUND2_COLOR, r, g, b);
  iupGlobalSetDefaultColorAttrib("TXTBGCOLOR", r, g, b);

  Fl::get_color(FL_FOREGROUND_COLOR, r, g, b);
  iupGlobalSetDefaultColorAttrib("TXTFGCOLOR", r, g, b);

  Fl::get_color(FL_SELECTION_COLOR, r, g, b);
  iupGlobalSetDefaultColorAttrib("TXTHLCOLOR", r, g, b);

  Fl::get_color(FL_BACKGROUND_COLOR, r, g, b);
  iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", r, g, b);

  Fl::get_color(FL_FOREGROUND_COLOR, r, g, b);
  iupGlobalSetDefaultColorAttrib("MENUFGCOLOR", r, g, b);

  iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", 0, 0, 238);
}

/****************************************************************************
 * System Theme Detection
 ****************************************************************************/

IUP_DRV_API int iupfltkIsSystemDarkMode(void)
{
  unsigned char r, g, b;
  Fl::get_color(FL_BACKGROUND_COLOR, r, g, b);
  int luminance = (int)(0.299 * r + 0.587 * g + 0.114 * b);
  return (luminance < 128) ? 1 : 0;
}

/****************************************************************************
 * Driver Initialization
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvOpen(int *argc, char ***argv)
{
  Fl::visual(FL_DOUBLE | FL_RGB);

  if (argc && argv)
    Fl::args(*argc, *argv);

  fl_open_display();

  Fl::lock();

  setlocale(LC_NUMERIC, "C");

  IupSetGlobal("DRIVER", "FLTK");

  IupSetfAttribute(NULL, "FLTKVERSION", "%d.%d.%d", FL_MAJOR_VERSION, FL_MINOR_VERSION, FL_PATCH_VERSION);

#if defined(__APPLE__)
  IupSetGlobal("WINDOWING", "QUARTZ");
#elif defined(_WIN32)
  IupSetGlobal("WINDOWING", "DWM");
#elif defined(FLTK_USE_WAYLAND)
  if (iupfltkIsWayland())
    IupSetGlobal("WINDOWING", "WAYLAND");
  else
    IupSetGlobal("WINDOWING", "X11");
#else
  IupSetGlobal("WINDOWING", "X11");
#endif

  if (argc && argv && *argv && (*argv)[0] && (*argv)[0][0] != 0)
    IupStoreGlobal("ARGV0", (*argv)[0]);

#if defined(FLTK_USE_X11)
  if (iupfltkIsX11())
  {
    IupSetGlobal("XDISPLAY", (char*)fl_display);
    IupSetGlobal("XSCREEN", (char*)(long)fl_screen);

    if (iupfltkX11Load() && iupfltk_XServerVendor && iupfltk_XVendorRelease)
    {
      IupSetGlobal("XSERVERVENDOR", iupfltk_XServerVendor(fl_display));
      IupSetInt(NULL, "XVENDORRELEASE", iupfltk_XVendorRelease(fl_display));
    }
  }
#endif

#if defined(FLTK_USE_WAYLAND)
  if (iupfltkIsWayland())
    IupSetGlobal("WL_DISPLAY", (char*)fl_wl_display());
#endif

  {
    const char* locale = setlocale(LC_ALL, NULL);
    if (locale)
      IupStoreGlobal("SYSTEMLANGUAGE", locale);
  }

  {
    const char* scheme = getenv("IUP_FLTKTHEME");
    if (scheme && scheme[0])
      Fl::scheme(scheme);
  }

  iupfltkSetGlobalColors();

  if (iupfltkIsSystemDarkMode())
    IupSetGlobal("DARKMODE", "YES");

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API int iupdrvSetGlobalAppIDAttrib(const char* value)
{
  static int appid_set = 0;
  if (appid_set || !value || !value[0])
    return 0;

  Fl_Window::default_xclass(value);
  IupStoreGlobal("_IUP_APPID_INTERNAL", value);
  appid_set = 1;
  return 1;
}

extern "C" IUP_SDK_API int iupdrvSetGlobalAppNameAttrib(const char* value)
{
  static int appname_set = 0;
  if (appname_set || !value || !value[0])
    return 0;

  appname_set = 1;
  return 1;
}

extern "C" IUP_SDK_API void iupdrvClose(void)
{
  iupfltkLoopCleanup();
  iupfltkX11Release();
}
