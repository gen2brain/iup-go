/** \file
 * \brief GTK4 X11 Backend Support
 *
 * Centralizes all X11 functionality: dynamic loading of Xlib,
 * GDK-to-X11 type extraction, and all X11 helper operations.
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gdk/x11/gdkx.h>

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#include "iup_export.h"

#include "iupgtk4_x11.h"

/* Xlib function pointers (dynamically loaded) */
static void* x11_lib = NULL;
static int (*_XDefaultScreen)(Display*) = NULL;
static char* (*_XServerVendor)(Display*) = NULL;
static int (*_XVendorRelease)(Display*) = NULL;
static int (*_XMoveWindow)(Display*, Window, int, int) = NULL;
static int (*_XSync)(Display*, int) = NULL;
static Atom (*_XInternAtom)(Display*, const char*, int) = NULL;
static int (*_XSetWMNormalHints)(Display*, Window, XSizeHints*) = NULL;
static int (*_XChangeProperty)(Display*, Window, Atom, Atom, int, int, const unsigned char*, int) = NULL;
static int (*_XWarpPointer)(Display*, Window, Window, int, int, unsigned int, unsigned int, int, int) = NULL;
static Window (*_XRootWindow)(Display*, int) = NULL;
static int (*_XQueryPointer)(Display*, Window, Window*, Window*, int*, int*, int*, int*, unsigned int*) = NULL;
static int (*_XTranslateCoordinates)(Display*, Window, Window, int, int, int*, int*, Window*) = NULL;

static int x11_load_functions(void)
{
  if (x11_lib)
    return 1;

  x11_lib = dlopen("libX11.so.6", RTLD_LAZY);
  if (!x11_lib)
    x11_lib = dlopen("libX11.so", RTLD_LAZY);

  if (!x11_lib)
    return 0;

  _XDefaultScreen = (int (*)(Display*))dlsym(x11_lib, "XDefaultScreen");
  _XServerVendor = (char* (*)(Display*))dlsym(x11_lib, "XServerVendor");
  _XVendorRelease = (int (*)(Display*))dlsym(x11_lib, "XVendorRelease");
  _XMoveWindow = (int (*)(Display*, Window, int, int))dlsym(x11_lib, "XMoveWindow");
  _XSync = (int (*)(Display*, int))dlsym(x11_lib, "XSync");
  _XInternAtom = (Atom (*)(Display*, const char*, int))dlsym(x11_lib, "XInternAtom");
  _XSetWMNormalHints = (int (*)(Display*, Window, XSizeHints*))dlsym(x11_lib, "XSetWMNormalHints");
  _XChangeProperty = (int (*)(Display*, Window, Atom, Atom, int, int, const unsigned char*, int))dlsym(x11_lib, "XChangeProperty");
  _XWarpPointer = (int (*)(Display*, Window, Window, int, int, unsigned int, unsigned int, int, int))dlsym(x11_lib, "XWarpPointer");
  _XRootWindow = (Window (*)(Display*, int))dlsym(x11_lib, "XRootWindow");
  _XQueryPointer = (int (*)(Display*, Window, Window*, Window*, int*, int*, int*, int*, unsigned int*))dlsym(x11_lib, "XQueryPointer");
  _XTranslateCoordinates = (int (*)(Display*, Window, Window, int, int, int*, int*, Window*))dlsym(x11_lib, "XTranslateCoordinates");

  if (!_XDefaultScreen || !_XServerVendor || !_XVendorRelease || !_XMoveWindow ||
      !_XSync || !_XSetWMNormalHints || !_XChangeProperty || !_XWarpPointer ||
      !_XRootWindow || !_XQueryPointer || !_XTranslateCoordinates)
  {
    dlclose(x11_lib);
    x11_lib = NULL;
    return 0;
  }

  return 1;
}

static Display* x11_get_xdisplay(void)
{
  GdkDisplay* display = gdk_display_get_default();
  if (display && GDK_IS_X11_DISPLAY(display))
    return gdk_x11_display_get_xdisplay(display);
  return NULL;
}

/* Backend detection */

IUP_DRV_API int iupgtk4X11IsBackend(void)
{
  GdkDisplay* display = gdk_display_get_default();
  return (display && GDK_IS_X11_DISPLAY(display));
}

IUP_DRV_API void* iupgtk4X11GetDisplay(void)
{
  return x11_get_xdisplay();
}

IUP_DRV_API int iupgtk4X11IsSurface(GdkSurface* surface)
{
  return (surface && GDK_IS_X11_SURFACE(surface));
}

IUP_DRV_API unsigned long iupgtk4X11GetSurfaceXid(GdkSurface* surface)
{
  if (surface && GDK_IS_X11_SURFACE(surface))
    return (unsigned long)gdk_x11_surface_get_xid(surface);
  return 0;
}

/* Window operations */

IUP_DRV_API int iupgtk4X11MoveWindow(GdkSurface* surface, int x, int y)
{
  Display* xdisplay;
  Window xwindow;
  XSizeHints hints;

  if (!surface || !GDK_IS_X11_SURFACE(surface))
    return 0;

  xdisplay = x11_get_xdisplay();
  if (!xdisplay)
    return 0;

  if (!x11_load_functions())
    return 0;

  xwindow = gdk_x11_surface_get_xid(surface);

  memset(&hints, 0, sizeof(XSizeHints));
  hints.flags = PPosition | USPosition;
  hints.x = x;
  hints.y = y;
  _XSetWMNormalHints(xdisplay, xwindow, &hints);

  _XMoveWindow(xdisplay, xwindow, x, y);

  _XSync(xdisplay, 0);
  return 1;
}

IUP_DRV_API int iupgtk4X11GetWindowPosition(GdkSurface* surface, int* x, int* y)
{
  Display* xdisplay;
  Window xwindow, child;
  int screen;

  if (!surface || !GDK_IS_X11_SURFACE(surface))
    return 0;

  xdisplay = x11_get_xdisplay();
  if (!xdisplay)
    return 0;

  if (!x11_load_functions())
    return 0;

  xwindow = gdk_x11_surface_get_xid(surface);
  screen = _XDefaultScreen(xdisplay);
  _XTranslateCoordinates(xdisplay, xwindow, _XRootWindow(xdisplay, screen), 0, 0, x, y, &child);
  return 1;
}

IUP_DRV_API int iupgtk4X11HideFromTaskbar(GdkSurface* surface)
{
  Display* xdisplay;
  Window xwindow;
  Atom net_wm_window_type, net_wm_window_type_popup_menu;
  Atom net_wm_state, net_wm_state_skip_taskbar, net_wm_state_skip_pager;
  Atom states[2];

  if (!surface || !GDK_IS_X11_SURFACE(surface))
    return 0;

  xdisplay = x11_get_xdisplay();
  if (!xdisplay)
    return 0;

  if (!x11_load_functions())
    return 0;

  xwindow = gdk_x11_surface_get_xid(surface);

  net_wm_window_type = _XInternAtom(xdisplay, "_NET_WM_WINDOW_TYPE", 0);
  net_wm_window_type_popup_menu = _XInternAtom(xdisplay, "_NET_WM_WINDOW_TYPE_POPUP_MENU", 0);
  _XChangeProperty(xdisplay, xwindow, net_wm_window_type, 4, 32, 0, (unsigned char*)&net_wm_window_type_popup_menu, 1);

  net_wm_state = _XInternAtom(xdisplay, "_NET_WM_STATE", 0);
  net_wm_state_skip_taskbar = _XInternAtom(xdisplay, "_NET_WM_STATE_SKIP_TASKBAR", 0);
  net_wm_state_skip_pager = _XInternAtom(xdisplay, "_NET_WM_STATE_SKIP_PAGER", 0);

  states[0] = net_wm_state_skip_taskbar;
  states[1] = net_wm_state_skip_pager;
  _XChangeProperty(xdisplay, xwindow, net_wm_state, 4, 32, 0, (unsigned char*)states, 2);

  _XSync(xdisplay, 0);
  return 1;
}

/* Pointer operations */

IUP_DRV_API int iupgtk4X11QueryPointer(int* x, int* y)
{
  Display* xdisplay;
  Window root, child;
  int root_x, root_y, win_x, win_y;
  unsigned int mask;
  int screen;

  xdisplay = x11_get_xdisplay();
  if (!xdisplay)
    return 0;

  if (!x11_load_functions())
    return 0;

  screen = _XDefaultScreen(xdisplay);
  _XQueryPointer(xdisplay, _XRootWindow(xdisplay, screen), &root, &child, &root_x, &root_y, &win_x, &win_y, &mask);

  *x = root_x;
  *y = root_y;
  return 1;
}

IUP_DRV_API int iupgtk4X11WarpPointer(int x, int y)
{
  Display* xdisplay;
  int screen;

  xdisplay = x11_get_xdisplay();
  if (!xdisplay)
    return 0;

  if (!x11_load_functions())
    return 0;

  screen = _XDefaultScreen(xdisplay);
  _XWarpPointer(xdisplay, None, _XRootWindow(xdisplay, screen), 0, 0, 0, 0, x, y);

  return 1;
}

/* System info */

IUP_DRV_API int iupgtk4X11GetDefaultScreen(void)
{
  Display* xdisplay = x11_get_xdisplay();
  if (!xdisplay || !x11_load_functions())
    return 0;
  return _XDefaultScreen(xdisplay);
}

IUP_DRV_API char* iupgtk4X11GetServerVendor(void)
{
  Display* xdisplay = x11_get_xdisplay();
  if (!xdisplay || !x11_load_functions())
    return NULL;
  return _XServerVendor(xdisplay);
}

IUP_DRV_API int iupgtk4X11GetVendorRelease(void)
{
  Display* xdisplay = x11_get_xdisplay();
  if (!xdisplay || !x11_load_functions())
    return 0;
  return _XVendorRelease(xdisplay);
}

/* Lifecycle */

IUP_DRV_API int iupgtk4X11Sync(void)
{
  Display* xdisplay = x11_get_xdisplay();
  if (!xdisplay || !x11_load_functions())
    return 0;

  _XSync(xdisplay, 0);
  return 1;
}

IUP_DRV_API void iupgtk4X11Cleanup(void)
{
  if (x11_lib)
  {
    dlclose(x11_lib);
    x11_lib = NULL;
  }
}

#endif
