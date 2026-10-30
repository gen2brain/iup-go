/** \file
 * \brief WebKitGTK Web Browser Control (X11 host shim for other toolkits).
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "iupwebgtk_dlopen.h"
#include "iupunix_x11.h"

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"

#include "iupweb_host.h"


#ifndef _X11_XLIB_H_
#define KeyPress       2
#define KeyRelease     3
#define FocusIn        9
#define FocusOut       10
#define NotifyGrab     1
#define NotifyUngrab   2
#define NotifyInferior 2
#define NotifyPointer  5
#endif

enum
{
  XEMBED_WINDOW_ACTIVATE = 1,
  XEMBED_WINDOW_DEACTIVATE = 2,
  XEMBED_REQUEST_FOCUS = 3,
  XEMBED_FOCUS_IN = 4,
  XEMBED_FOCUS_OUT = 5,
  XEMBED_FOCUS_NEXT = 6,
  XEMBED_FOCUS_PREV = 7
};

typedef struct _IgtkWebHost
{
  Ihandle* ih;
  GtkWidget* view;
  GtkWidget* plug;
  Window holder;
  GdkWindow* holder_window;
  Window parent;
  Window toplevel;
  GdkWindow* toplevel_window;
  int rect[4];
  int clip[4];
  int focus;
} IgtkWebHost;

typedef struct _IgtkWebHostWatch
{
  Ihandle* watch;
  int fd;
  int mode;
} IgtkWebHostWatch;

static int s_hostOpened = 0;
static GPollFD* s_glibFds = NULL;
static int s_glibFdsMax = 0;
static IgtkWebHostWatch* s_glibWatches = NULL;
static int s_glibWatchesCount = 0;
static Ihandle* s_glibTimer = NULL;
static int s_glibRefresh = 0;
static char* s_hostLastConvert = NULL;
static Atom s_xembedAtom = 0;

static IgtkWebHost* gtkWebHostGet(Ihandle* ih)
{
  return (IgtkWebHost*)iupAttribGet(ih, "_IUPWEB_GTKHOST");
}

static void gtkWebHostGlibUpdate(void);

static int gtkWebHostGlibAction(Ihandle* ih)
{
  (void)ih;
  g_main_context_iteration(NULL, FALSE);
  gtkWebHostGlibUpdate();
  return IUP_DEFAULT;
}

static void gtkWebHostGlibSetTimer(int timeout)
{
  IupSetAttribute(s_glibTimer, "RUN", "NO");
  if (timeout < 0)
    return;

  IupSetInt(s_glibTimer, "TIME", timeout > 0 ? timeout : 1);
  IupSetAttribute(s_glibTimer, "RUN", "YES");
}

static int gtkWebHostGlibMode(int fd, int count)
{
  int i, mode = 0;

  for (i = 0; i < count; i++)
  {
    if (s_glibFds[i].fd != fd)
      continue;
    if (s_glibFds[i].events & (G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_ERR))
      mode |= 1;
    if (s_glibFds[i].events & G_IO_OUT)
      mode |= 2;
  }

  return mode;
}

static void gtkWebHostGlibStart(int fd, int mode)
{
  IgtkWebHostWatch* watch = NULL;
  int i;

  for (i = 0; i < s_glibWatchesCount; i++)
  {
    if (s_glibWatches[i].fd == fd)
      return;
    if (!watch && s_glibWatches[i].fd < 0)
      watch = s_glibWatches + i;
  }

  if (!watch)
  {
    s_glibWatches = (IgtkWebHostWatch*)realloc(s_glibWatches, (s_glibWatchesCount + 1) * sizeof(IgtkWebHostWatch));
    watch = s_glibWatches + s_glibWatchesCount;
    watch->watch = IupFdWatch();
    IupSetCallback(watch->watch, "ACTION_CB", gtkWebHostGlibAction);
    s_glibWatchesCount++;
  }

  watch->fd = fd;
  watch->mode = mode;
  IupSetInt(watch->watch, "FD", fd);
  IupSetAttribute(watch->watch, "MODE", mode == 3 ? "READWRITE" : (mode == 2 ? "WRITE" : "READ"));
  IupSetAttribute(watch->watch, "RUN", "YES");
}

static void gtkWebHostGlibUpdate(void)
{
  GMainContext* context = g_main_context_default();
  int priority, timeout, count, ready, i;

  s_glibRefresh = 0;

  ready = g_main_context_prepare(context, &priority);
  count = g_main_context_query(context, priority, &timeout, s_glibFds, s_glibFdsMax);
  if (count > s_glibFdsMax)
  {
    s_glibFdsMax = count;
    s_glibFds = (GPollFD*)realloc(s_glibFds, s_glibFdsMax * sizeof(GPollFD));
    count = g_main_context_query(context, priority, &timeout, s_glibFds, s_glibFdsMax);
  }

  for (i = 0; i < s_glibWatchesCount; i++)
  {
    IgtkWebHostWatch* watch = s_glibWatches + i;
    if (watch->fd >= 0 && gtkWebHostGlibMode(watch->fd, count) != watch->mode)
    {
      IupSetAttribute(watch->watch, "RUN", "NO");
      watch->fd = -1;
      watch->mode = 0;
    }
  }

  for (i = 0; i < count; i++)
  {
    int mode = gtkWebHostGlibMode(s_glibFds[i].fd, count);
    if (mode)
      gtkWebHostGlibStart(s_glibFds[i].fd, mode);
  }

  gtkWebHostGlibSetTimer(ready ? 0 : timeout);
}

static void gtkWebHostRefresh(void)
{
  if (!s_glibTimer || s_glibRefresh)
    return;

  s_glibRefresh = 1;
  gtkWebHostGlibSetTimer(0);
}

static int gtkWebHostOpen(void)
{
  GdkDisplay* display;

  if (s_hostOpened)
    return s_hostOpened > 0;

  s_hostOpened = -1;

  if (!iupX11Open() || !XSendEvent || !XInternAtom || !XRootWindow || !XDefaultScreen || !XSync ||
      !XCreateSimpleWindow || !XSetWindowBackgroundPixmap || !XReparentWindow || !XMoveResizeWindow ||
      !XMapWindow || !XUnmapWindow || !XDestroyWindow || !XQueryTree || !XGetWindowProperty || !XFree)
    return 0;

  gdk_set_allowed_backends("x11");
  if (!gtk_init_check(NULL, NULL))
    return 0;

  display = gdk_display_get_default();
  if (!display || !g_type_check_instance_is_a((GTypeInstance*)display, gdk_x11_display_get_type()))
    return 0;

  if (!g_main_context_acquire(g_main_context_default()))
    return 0;

  s_glibTimer = IupTimer();
  IupSetCallback(s_glibTimer, "ACTION_CB", gtkWebHostGlibAction);

  s_hostOpened = 1;
  gtkWebHostGlibUpdate();
  return 1;
}

static char* gtkWebHostConvert(const char* str, const char* to_charset, const char* from_charset)
{
  if (s_hostLastConvert)
    g_free(s_hostLastConvert);

  s_hostLastConvert = g_convert(str, -1, to_charset, from_charset, NULL, NULL, NULL);
  if (!s_hostLastConvert)
    return (char*)str;
  return s_hostLastConvert;
}

static char* iupgtkStrConvertToSystem(const char* str)
{
  const char* charset = NULL;

  if (!str || *str == 0 || IupGetInt(NULL, "UTF8MODE"))
    return (char*)str;

  if (g_get_charset(&charset))
  {
    if (g_utf8_validate(str, -1, NULL))
      return (char*)str;
    charset = "ISO8859-1";
  }
  else if (!charset || iupStrIsAscii(str))
    return (char*)str;

  return gtkWebHostConvert(str, "UTF-8", charset);
}

static char* iupgtkStrConvertToFilename(const char* str)
{
  const gchar** charsets = NULL;

  if (!str || *str == 0 || !IupGetInt(NULL, "UTF8MODE"))
    return (char*)str;

  if (g_get_filename_charsets(&charsets) || !charsets || iupStrIsAscii(str))
    return (char*)str;

  return gtkWebHostConvert(str, charsets[0], "UTF-8");
}

static gboolean iupgtkShowHelp(GtkWidget* widget, GtkWidgetHelpType* arg1, Ihandle* ih)
{
  Icallback cb;
  (void)widget;
  (void)arg1;

  cb = IupGetCallback(ih, "HELP_CB");
  if (cb && cb(ih) == IUP_CLOSE)
    IupExitLoop();

  return FALSE;
}

static gboolean iupgtkEnterLeaveEvent(GtkWidget* widget, GdkEventCrossing* evt, Ihandle* ih)
{
  Icallback cb = NULL;
  (void)widget;

  if (evt->type == GDK_ENTER_NOTIFY)
    cb = IupGetCallback(ih, "ENTERWINDOW_CB");
  else if (evt->type == GDK_LEAVE_NOTIFY)
    cb = IupGetCallback(ih, "LEAVEWINDOW_CB");

  if (cb)
    cb(ih);

  return FALSE;
}

static Display* gtkWebHostXDisplay(void)
{
  return (Display*)gdk_x11_display_get_xdisplay(gdk_display_get_default());
}

static Window gtkWebHostPlugWindow(IgtkWebHost* host)
{
  GdkWindow* window = host->plug ? gtk_widget_get_window(host->plug) : NULL;
  return window ? (Window)gdk_x11_window_get_xid(window) : 0;
}

static void gtkWebHostXEmbed(IgtkWebHost* host, long message)
{
  GdkDisplay* display = gdk_display_get_default();
  Display* xdisplay = gtkWebHostXDisplay();
  Window window = gtkWebHostPlugWindow(host);
  XEvent event;

  if (!window)
    return;

  memset(&event, 0, sizeof(event));
  event.xclient.type = ClientMessage;
  event.xclient.window = window;
  event.xclient.message_type = s_xembedAtom;
  event.xclient.format = 32;
  event.xclient.data.l[0] = CurrentTime;
  event.xclient.data.l[1] = message;

  gdk_x11_display_error_trap_push(display);
  XSendEvent(xdisplay, window, 0, NoEventMask, &event);
  gdk_x11_display_error_trap_pop_ignored(display);
}

static void gtkWebHostFocusIn(IgtkWebHost* host)
{
  gtkWebHostXEmbed(host, XEMBED_WINDOW_ACTIVATE);
  gtkWebHostXEmbed(host, XEMBED_FOCUS_IN);
}

static GdkFilterReturn gtkWebHostParentFilter(GdkXEvent* gdk_xevent, GdkEvent* event, gpointer data)
{
  IgtkWebHost* host = (IgtkWebHost*)data;
  XEvent* xevent = (XEvent*)gdk_xevent;
  (void)event;

  if ((xevent->type == FocusIn || xevent->type == FocusOut) && xevent->xfocus.window == host->toplevel &&
      xevent->xfocus.mode != NotifyGrab && xevent->xfocus.mode != NotifyUngrab &&
      xevent->xfocus.detail != NotifyInferior && xevent->xfocus.detail != NotifyPointer)
  {
    if (xevent->type == FocusIn)
    {
      gtkWebHostXEmbed(host, XEMBED_WINDOW_ACTIVATE);
      if (host->focus)
        gtkWebHostXEmbed(host, XEMBED_FOCUS_IN);
    }
    else
    {
      if (host->focus)
        gtkWebHostXEmbed(host, XEMBED_FOCUS_OUT);
      gtkWebHostXEmbed(host, XEMBED_WINDOW_DEACTIVATE);
    }
    gtkWebHostRefresh();
  }

  return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn gtkWebHostHolderFilter(GdkXEvent* gdk_xevent, GdkEvent* event, gpointer data)
{
  IgtkWebHost* host = (IgtkWebHost*)data;
  XEvent* xevent = (XEvent*)gdk_xevent;
  (void)event;

  if (xevent->type != ClientMessage || xevent->xclient.window != host->holder || xevent->xclient.message_type != s_xembedAtom)
    return GDK_FILTER_CONTINUE;

  switch (xevent->xclient.data.l[1])
  {
  case XEMBED_REQUEST_FOCUS:
    if (IupGetFocus() != host->ih)
      IupSetFocus(host->ih);
    break;
  case XEMBED_FOCUS_NEXT:
    IupNextField(host->ih);
    break;
  case XEMBED_FOCUS_PREV:
    IupPreviousField(host->ih);
    break;
  }

  gtkWebHostRefresh();
  return GDK_FILTER_REMOVE;
}

static void gtkWebHostPlace(IgtkWebHost* host)
{
  Display* xdisplay = gtkWebHostXDisplay();
  GdkWindow* window = gtk_widget_get_window(host->plug);
  int scale;

  if (!host->parent || host->clip[2] <= 0 || host->clip[3] <= 0 || !window)
  {
    XUnmapWindow(xdisplay, host->holder);
    return;
  }

  scale = gdk_window_get_scale_factor(window);
  if (scale < 1)
    scale = 1;

  XMoveResizeWindow(xdisplay, host->holder, host->clip[0], host->clip[1], host->clip[2], host->clip[3]);
  gdk_window_move_resize(window, (host->rect[0] - host->clip[0]) / scale, (host->rect[1] - host->clip[1]) / scale, host->rect[2] / scale, host->rect[3] / scale);
  XMapWindow(xdisplay, host->holder);
}

static Window gtkWebHostTopLevel(Display* xdisplay, Window window)
{
  Atom wm_state = XInternAtom(xdisplay, "WM_STATE", 0);

  for (;;)
  {
    Window root = 0, parent = 0, *children = NULL;
    unsigned int count = 0;
    Atom type = None;
    int format;
    unsigned long items, after;
    unsigned char* data = NULL;

    if (XGetWindowProperty(xdisplay, window, wm_state, 0, 0, 0, AnyPropertyType, &type, &format, &items, &after, &data) == Success && data)
      XFree(data);
    if (type != None)
      return window;

    if (!XQueryTree(xdisplay, window, &root, &parent, &children, &count))
      return window;
    if (children)
      XFree(children);
    if (!parent || parent == root)
      return window;

    window = parent;
  }
}

static void gtkWebHostReparent(IgtkWebHost* host, Window parent)
{
  GdkDisplay* display = gdk_display_get_default();
  Display* xdisplay = gtkWebHostXDisplay();
  Window root = XRootWindow(xdisplay, XDefaultScreen(xdisplay));

  if (host->toplevel_window)
  {
    gdk_window_remove_filter(host->toplevel_window, gtkWebHostParentFilter, host);
    g_object_unref(host->toplevel_window);
    host->toplevel_window = NULL;
  }
  host->toplevel = 0;

  if (parent == root)
    parent = 0;

  gdk_x11_display_error_trap_push(display);
  XUnmapWindow(xdisplay, host->holder);
  XReparentWindow(xdisplay, host->holder, parent ? parent : root, host->clip[0], host->clip[1]);
  XSync(xdisplay, 0);
  if (gdk_x11_display_error_trap_pop(display) && parent)
  {
    gdk_x11_display_error_trap_push(display);
    XReparentWindow(xdisplay, host->holder, root, 0, 0);
    XSync(xdisplay, 0);
    gdk_x11_display_error_trap_pop_ignored(display);
    parent = 0;
  }

  host->parent = parent;
  if (!parent)
    return;

  gdk_x11_display_error_trap_push(display);
  host->toplevel = gtkWebHostTopLevel(xdisplay, parent);
  host->toplevel_window = gdk_x11_window_foreign_new_for_display(display, host->toplevel);
  if (host->toplevel_window)
  {
    gdk_window_set_events(host->toplevel_window, (GdkEventMask)(gdk_window_get_events(host->toplevel_window) | GDK_FOCUS_CHANGE_MASK));
    gdk_window_add_filter(host->toplevel_window, gtkWebHostParentFilter, host);
  }
  gtkWebHostPlace(host);
  XSync(xdisplay, 0);
  gdk_x11_display_error_trap_pop_ignored(display);
}

static GtkWindow* gtkWebHostWindow(Ihandle* ih)
{
  IgtkWebHost* host = gtkWebHostGet(ih);
  return host ? (GtkWindow*)host->plug : NULL;
}

static void gtkWebHostDestroy(IgtkWebHost* host)
{
  GdkDisplay* display = gdk_display_get_default();
  Display* xdisplay = gtkWebHostXDisplay();

  gdk_x11_display_error_trap_push(display);

  if (host->toplevel_window)
  {
    gdk_window_remove_filter(host->toplevel_window, gtkWebHostParentFilter, host);
    g_object_unref(host->toplevel_window);
  }

  if (host->plug)
    gtk_widget_destroy(host->plug);

  if (host->holder_window)
  {
    gdk_window_remove_filter(host->holder_window, gtkWebHostHolderFilter, host);
    g_object_unref(host->holder_window);
  }

  XDestroyWindow(xdisplay, host->holder);
  XSync(xdisplay, 0);
  gdk_x11_display_error_trap_pop_ignored(display);

  free(host);
  gtkWebHostRefresh();
}

static void gtkWebHostRemoveFromParent(Ihandle* ih)
{
  IgtkWebHost* host = gtkWebHostGet(ih);
  if (!host)
    return;

  iupAttribSet(ih, "_IUPWEB_GTKHOST", NULL);
  gtkWebHostDestroy(host);
  iupwebHostUnMap(ih);
}

static int gtkWebHostAddToParent(Ihandle* ih, GtkWidget* view)
{
  GdkDisplay* display = gdk_display_get_default();
  Display* xdisplay = gtkWebHostXDisplay();
  IgtkWebHost* host = (IgtkWebHost*)calloc(1, sizeof(IgtkWebHost));
  GdkWindow* window;
  void* parent;

  if (!s_xembedAtom)
    s_xembedAtom = XInternAtom(xdisplay, "_XEMBED", 0);

  host->ih = ih;
  host->view = view;
  host->holder = XCreateSimpleWindow(xdisplay, XRootWindow(xdisplay, XDefaultScreen(xdisplay)), 0, 0, 1, 1, 0, 0, 0);
  XSetWindowBackgroundPixmap(xdisplay, host->holder, None);
  XSync(xdisplay, 0);

  host->holder_window = gdk_x11_window_foreign_new_for_display(display, host->holder);
  if (host->holder_window)
    gdk_window_add_filter(host->holder_window, gtkWebHostHolderFilter, host);

  host->plug = gtk_plug_new(host->holder);
  gtk_container_add((GtkContainer*)host->plug, view);
  gtk_widget_show(view);
  gtk_widget_show(host->plug);

  window = gtk_widget_get_window(host->plug);
  if (window)
  {
    gdk_window_set_events(window, (GdkEventMask)(gdk_window_get_events(window) & ~(GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK)));
    gdk_window_show(window);
  }

  iupAttribSet(ih, "_IUPWEB_GTKHOST", (char*)host);

  parent = iupwebHostMap(ih);
  if (!parent && !ih->handle)
  {
    iupAttribSet(ih, "_IUPWEB_GTKHOST", NULL);
    gtkWebHostDestroy(host);
    return 0;
  }

  if (parent)
    iupwebHostSetParent(ih, parent);

  gtkWebHostRefresh();
  return 1;
}


#include "iupgtk_webbrowser.c"


void iupwebHostSetBounds(Ihandle* ih, int x, int y, int width, int height, int clip_x, int clip_y, int clip_width, int clip_height)
{
  IgtkWebHost* host = gtkWebHostGet(ih);
  if (!host)
    return;

  host->rect[0] = x; host->rect[1] = y; host->rect[2] = width; host->rect[3] = height;
  host->clip[0] = clip_x; host->clip[1] = clip_y; host->clip[2] = clip_width; host->clip[3] = clip_height;

  gtkWebHostPlace(host);
  gtkWebHostRefresh();
}

void iupwebHostSetParent(Ihandle* ih, void* parent)
{
  IgtkWebHost* host = gtkWebHostGet(ih);
  Window window = (Window)(uintptr_t)parent;

  if (!host || host->parent == window)
    return;

  gtkWebHostReparent(host, window);
  if (window && host->focus)
    gtkWebHostFocusIn(host);

  gtkWebHostRefresh();
}

void iupwebHostSetFocus(Ihandle* ih, int focus)
{
  IgtkWebHost* host = gtkWebHostGet(ih);
  if (!host || host->focus == focus)
    return;

  host->focus = focus;
  if (focus)
  {
    gtk_widget_grab_focus(host->view);
    gtkWebHostFocusIn(host);
  }
  else
    gtkWebHostXEmbed(host, XEMBED_FOCUS_OUT);

  gtkWebHostRefresh();
}

void iupwebHostKeyEvent(Ihandle* ih, int press, unsigned int keycode, unsigned int state, unsigned long time)
{
  IgtkWebHost* host = gtkWebHostGet(ih);
  GdkDisplay* display = gdk_display_get_default();
  Display* xdisplay = gtkWebHostXDisplay();
  Window window;
  XEvent event;

  if (!host)
    return;

  window = gtkWebHostPlugWindow(host);
  if (!window)
    return;

  memset(&event, 0, sizeof(event));
  event.xkey.type = press ? KeyPress : KeyRelease;
  event.xkey.window = window;
  event.xkey.root = XRootWindow(xdisplay, XDefaultScreen(xdisplay));
  event.xkey.time = time;
  event.xkey.state = state;
  event.xkey.keycode = keycode;
  event.xkey.same_screen = 1;

  gdk_x11_display_error_trap_push(display);
  XSendEvent(xdisplay, window, 0, NoEventMask, &event);
  gdk_x11_display_error_trap_pop_ignored(display);

  gtkWebHostRefresh();
}
