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
#include <X11/Xatom.h>

#include <string.h>

#include "iup_export.h"

#include "iupgtk4_x11.h"

#ifdef IUPX11_USE_DLOPEN
#include "iupunix_x11.h"
#endif

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
  long supplied = 0;

  if (!surface || !GDK_IS_X11_SURFACE(surface))
    return 0;

  xdisplay = x11_get_xdisplay();
  if (!xdisplay)
    return 0;

#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return 0;
#endif

  xwindow = gdk_x11_surface_get_xid(surface);

  memset(&hints, 0, sizeof(XSizeHints));
  XGetWMNormalHints(xdisplay, xwindow, &hints, &supplied);
  hints.flags |= PPosition | USPosition;
  hints.x = x;
  hints.y = y;
  XSetWMNormalHints(xdisplay, xwindow, &hints);

  XMoveWindow(xdisplay, xwindow, x, y);

  XSync(xdisplay, 0);
  return 1;
}

IUP_DRV_API int iupgtk4X11ResizeWindow(GdkSurface* surface, int width, int height)
{
  Display* xdisplay;

  if (!surface || !GDK_IS_X11_SURFACE(surface) || width <= 0 || height <= 0)
    return 0;

  xdisplay = x11_get_xdisplay();
  if (!xdisplay)
    return 0;

#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return 0;
#endif

  XResizeWindow(xdisplay, gdk_x11_surface_get_xid(surface), (unsigned int)width, (unsigned int)height);
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

#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return 0;
#endif

  xwindow = gdk_x11_surface_get_xid(surface);
  screen = XDefaultScreen(xdisplay);
  XTranslateCoordinates(xdisplay, xwindow, XRootWindow(xdisplay, screen), 0, 0, x, y, &child);
  return 1;
}

IUP_DRV_API int iupgtk4X11GetFrameExtents(GdkSurface* surface, int* left, int* top, int* right, int* bottom)
{
  Display* xdisplay;
  Atom type;
  int format, found = 0;
  unsigned long nitems, bytes_after;
  unsigned char* data = NULL;

  if (!surface || !GDK_IS_X11_SURFACE(surface))
    return 0;

  xdisplay = x11_get_xdisplay();
  if (!xdisplay)
    return 0;

#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return 0;
#endif

  if (XGetWindowProperty(xdisplay, gdk_x11_surface_get_xid(surface), XInternAtom(xdisplay, "_NET_FRAME_EXTENTS", False),
                         0, 4, False, XA_CARDINAL, &type, &format, &nitems, &bytes_after, &data) == Success)
  {
    if (type == XA_CARDINAL && format == 32 && nitems == 4 && data)
    {
      long* extents = (long*)data;
      *left = (int)extents[0];
      *top = (int)extents[2];
      if (right) *right = (int)extents[1];
      if (bottom) *bottom = (int)extents[3];
      found = 1;
    }
    if (data)
      XFree(data);
  }

  return found;
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

#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return 0;
#endif

  xwindow = gdk_x11_surface_get_xid(surface);

  net_wm_window_type = XInternAtom(xdisplay, "_NET_WM_WINDOW_TYPE", 0);
  net_wm_window_type_popup_menu = XInternAtom(xdisplay, "_NET_WM_WINDOW_TYPE_POPUP_MENU", 0);
  XChangeProperty(xdisplay, xwindow, net_wm_window_type, 4, 32, 0, (unsigned char*)&net_wm_window_type_popup_menu, 1);

  net_wm_state = XInternAtom(xdisplay, "_NET_WM_STATE", 0);
  net_wm_state_skip_taskbar = XInternAtom(xdisplay, "_NET_WM_STATE_SKIP_TASKBAR", 0);
  net_wm_state_skip_pager = XInternAtom(xdisplay, "_NET_WM_STATE_SKIP_PAGER", 0);

  states[0] = net_wm_state_skip_taskbar;
  states[1] = net_wm_state_skip_pager;
  XChangeProperty(xdisplay, xwindow, net_wm_state, 4, 32, 0, (unsigned char*)states, 2);

  XSync(xdisplay, 0);
  return 1;
}

IUP_DRV_API int iupgtk4X11SetSkipTaskbar(GdkSurface* surface, int skip)
{
  Display* xdisplay;
  Atom net_wm_state, net_wm_state_skip_taskbar;
  XEvent evt;

  if (!surface || !GDK_IS_X11_SURFACE(surface))
    return 0;

  xdisplay = x11_get_xdisplay();
  if (!xdisplay)
    return 0;

#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return 0;
#endif

  net_wm_state = XInternAtom(xdisplay, "_NET_WM_STATE", 0);
  net_wm_state_skip_taskbar = XInternAtom(xdisplay, "_NET_WM_STATE_SKIP_TASKBAR", 0);

  memset(&evt, 0, sizeof(evt));
  evt.xclient.type = ClientMessage;
  evt.xclient.window = gdk_x11_surface_get_xid(surface);
  evt.xclient.message_type = net_wm_state;
  evt.xclient.format = 32;
  evt.xclient.data.l[0] = skip? 1: 0;
  evt.xclient.data.l[1] = (long)net_wm_state_skip_taskbar;
  evt.xclient.data.l[3] = 1;

  XSendEvent(xdisplay, XRootWindow(xdisplay, XDefaultScreen(xdisplay)), 0,
             SubstructureNotifyMask | SubstructureRedirectMask, &evt);
  return 1;
}

/* Pointer operations */

/* GTK4 dropped the geometry hints, so the resize increments go straight to the window manager */
IUP_DRV_API int iupgtk4X11SetSizeHints(GdkSurface* surface, int base_w, int base_h, int inc_w, int inc_h, int max_w, int max_h)
{
  Display* xdisplay;
  Window xwindow;
  XSizeHints hints;
  long supplied = 0;

  if (!surface || !GDK_IS_X11_SURFACE(surface))
    return 0;

  xdisplay = x11_get_xdisplay();
  if (!xdisplay)
    return 0;

#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return 0;
#endif

  xwindow = gdk_x11_surface_get_xid(surface);

  memset(&hints, 0, sizeof(XSizeHints));
  XGetWMNormalHints(xdisplay, xwindow, &hints, &supplied);

  if (inc_w > 1 || inc_h > 1)
  {
    hints.flags |= PBaseSize | PResizeInc;
    hints.base_width = base_w > 0? base_w: 0;
    hints.base_height = base_h > 0? base_h: 0;
    hints.width_inc = inc_w > 1? inc_w: 1;
    hints.height_inc = inc_h > 1? inc_h: 1;
  }
  else
    hints.flags &= ~(PBaseSize | PResizeInc);

  if (max_w > 0 && max_h > 0)
  {
    hints.flags |= PMaxSize;
    hints.max_width = max_w;
    hints.max_height = max_h;
  }
  else
    hints.flags &= ~PMaxSize;

  XSetWMNormalHints(xdisplay, xwindow, &hints);
  return 1;
}

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

#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return 0;
#endif

  screen = XDefaultScreen(xdisplay);
  XQueryPointer(xdisplay, XRootWindow(xdisplay, screen), &root, &child, &root_x, &root_y, &win_x, &win_y, &mask);

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

#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return 0;
#endif

  screen = XDefaultScreen(xdisplay);
  XWarpPointer(xdisplay, None, XRootWindow(xdisplay, screen), 0, 0, 0, 0, x, y);

  return 1;
}

/* System info */

IUP_DRV_API int iupgtk4X11GetDefaultScreen(void)
{
  Display* xdisplay = x11_get_xdisplay();
  if (!xdisplay)
    return 0;
#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return 0;
#endif
  return XDefaultScreen(xdisplay);
}

IUP_DRV_API char* iupgtk4X11GetServerVendor(void)
{
  Display* xdisplay = x11_get_xdisplay();
  if (!xdisplay)
    return NULL;
#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return NULL;
#endif
  return XServerVendor(xdisplay);
}

IUP_DRV_API int iupgtk4X11GetVendorRelease(void)
{
  Display* xdisplay = x11_get_xdisplay();
  if (!xdisplay)
    return 0;
#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return 0;
#endif
  return XVendorRelease(xdisplay);
}

/* Events */

static void (*x11_configure_cb)(unsigned long xid) = NULL;
static gulong x11_xevent_handler = 0;

static gboolean x11_xevent(GdkX11Display* display, gpointer xevent, gpointer user_data)
{
  XEvent* ev = (XEvent*)xevent;
  (void)display;
  (void)user_data;

  if (ev->type == ConfigureNotify && x11_configure_cb)
    x11_configure_cb((unsigned long)ev->xconfigure.window);

  return FALSE;
}

IUP_DRV_API void iupgtk4X11WatchConfigure(void (*cb)(unsigned long xid))
{
  GdkDisplay* display = gdk_display_get_default();

  if (x11_xevent_handler || !display || !GDK_IS_X11_DISPLAY(display))
    return;

  x11_configure_cb = cb;
  x11_xevent_handler = g_signal_connect(display, "xevent", G_CALLBACK(x11_xevent), NULL);
}

/* Lifecycle */

IUP_DRV_API int iupgtk4X11Sync(void)
{
  Display* xdisplay = x11_get_xdisplay();
  if (!xdisplay)
    return 0;
#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return 0;
#endif

  XSync(xdisplay, 0);
  return 1;
}

IUP_DRV_API void iupgtk4X11Cleanup(void)
{
  GdkDisplay* display = gdk_display_get_default();

  if (x11_xevent_handler && display)
    g_signal_handler_disconnect(display, x11_xevent_handler);
  x11_xevent_handler = 0;
  x11_configure_cb = NULL;

#ifdef IUPX11_USE_DLOPEN
  iupX11Close();
#endif
}

#endif
