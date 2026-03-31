/** \file
 * \brief X11 Dynamic Loading Support
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPX11_DLOPEN_H
#define __IUPX11_DLOPEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dlfcn.h>

/* Type definitions, only when real X11 headers are not included */
#ifndef _X11_XLIB_H_

typedef struct _XDisplay Display;
typedef struct _XGC* GC;
typedef unsigned long XID;
typedef unsigned long Window;
typedef unsigned long Atom;
typedef unsigned long Colormap;
typedef unsigned long VisualID;
typedef unsigned long Cursor;
typedef unsigned long Pixmap;
typedef int Bool;
typedef void* XPointer;

typedef struct {
  void* ext_data;
  VisualID visualid;
#if defined(__cplusplus) || defined(c_plusplus)
  int c_class;
#else
  int class;
#endif
  unsigned long red_mask, green_mask, blue_mask;
  int bits_per_rgb;
  int map_entries;
} Visual;

typedef struct {
  int depth;
  int nvisuals;
  Visual* visuals;
} Depth;

typedef struct {
  void* ext_data;
  struct _XDisplay* display;
  Window root;
  int width, height;
  int mwidth, mheight;
  int ndepths;
  Depth* depths;
  int root_depth;
  Visual* root_visual;
  GC default_gc;
  Colormap cmap;
  unsigned long white_pixel;
  unsigned long black_pixel;
  int max_maps, min_maps;
  int backing_store;
  Bool save_unders;
  long root_input_mask;
} Screen;

typedef struct {
  int x, y;
  int width, height;
  int border_width;
  int depth;
  Visual* visual;
  Window root;
#if defined(__cplusplus) || defined(c_plusplus)
  int c_class;
#else
  int class;
#endif
  int bit_gravity;
  int win_gravity;
  int backing_store;
  unsigned long backing_planes;
  unsigned long backing_pixel;
  Bool save_under;
  Colormap colormap;
  Bool map_installed;
  int map_state;
  long all_event_masks;
  long your_event_mask;
  long do_not_propagate_mask;
  Bool override_redirect;
  Screen* screen;
} XWindowAttributes;

typedef struct {
  long flags;
  int x, y;
  int width, height;
  int min_width, min_height;
  int max_width, max_height;
  int width_inc, height_inc;
  struct { int x; int y; } min_aspect, max_aspect;
  int base_width, base_height;
  int win_gravity;
} XSizeHints;

typedef struct {
  int key_click_percent;
  int bell_percent;
  int bell_pitch;
  int bell_duration;
  int led;
  int led_mode;
  int key;
  int auto_repeat_mode;
} XKeyboardControl;

typedef struct {
  int type;
  unsigned long serial;
  Bool send_event;
  Display* display;
  Window window;
  Atom message_type;
  int format;
  union {
    char b[20];
    short s[10];
    long l[5];
  } data;
} XClientMessageEvent;

typedef union _XEvent {
  int type;
  XClientMessageEvent xclient;
  long pad[24];
} XEvent;

#define None              0L
#define PPosition         (1L << 2)
#define USPosition        (1L << 0)

#define KBAutoRepeatMode  (1L << 7)
#define AutoRepeatModeOff 0
#define AutoRepeatModeOn  1

#define ClientMessage     33
#define PropModeReplace   0
#define SubstructureRedirectMask  (1L << 20)
#define SubstructureNotifyMask    (1L << 19)

#endif /* _X11_XLIB_H_ */

static void* iupx11_handle = NULL;

static GC (*iupx11_XCreateGC)(Display*, XID, unsigned long, void*) = NULL;
static int (*iupx11_XFreeGC)(Display*, GC) = NULL;
static int (*iupx11_XDefaultScreen)(Display*) = NULL;
static char* (*iupx11_XServerVendor)(Display*) = NULL;
static int (*iupx11_XVendorRelease)(Display*) = NULL;
static int (*iupx11_XMoveWindow)(Display*, Window, int, int) = NULL;
static int (*iupx11_XSync)(Display*, int) = NULL;
static Atom (*iupx11_XInternAtom)(Display*, const char*, int) = NULL;
static int (*iupx11_XSetWMNormalHints)(Display*, Window, XSizeHints*) = NULL;
static int (*iupx11_XChangeProperty)(Display*, Window, Atom, Atom, int, int, const unsigned char*, int) = NULL;
static int (*iupx11_XWarpPointer)(Display*, Window, Window, int, int, unsigned int, unsigned int, int, int) = NULL;
static Window (*iupx11_XRootWindow)(Display*, int) = NULL;
static int (*iupx11_XQueryPointer)(Display*, Window, Window*, Window*, int*, int*, int*, int*, unsigned int*) = NULL;
static int (*iupx11_XTranslateCoordinates)(Display*, Window, Window, int, int, int*, int*, Window*) = NULL;
static int (*iupx11_XChangeKeyboardControl)(Display*, unsigned long, XKeyboardControl*) = NULL;
static int (*iupx11_XDeleteProperty)(Display*, Window, Atom) = NULL;
static int (*iupx11_XSendEvent)(Display*, Window, int, long, XEvent*) = NULL;
static int (*iupx11_XGetWindowAttributes)(Display*, Window, XWindowAttributes*) = NULL;
static VisualID (*iupx11_XVisualIDFromVisual)(Visual*) = NULL;

#define XCreateGC iupx11_XCreateGC
#define XFreeGC iupx11_XFreeGC
#define XDefaultScreen iupx11_XDefaultScreen
#define XServerVendor iupx11_XServerVendor
#define XVendorRelease iupx11_XVendorRelease
#define XMoveWindow iupx11_XMoveWindow
#define XSync iupx11_XSync
#define XInternAtom iupx11_XInternAtom
#define XSetWMNormalHints iupx11_XSetWMNormalHints
#define XChangeProperty iupx11_XChangeProperty
#define XWarpPointer iupx11_XWarpPointer
#define XRootWindow iupx11_XRootWindow
#define XQueryPointer iupx11_XQueryPointer
#define XTranslateCoordinates iupx11_XTranslateCoordinates
#define XChangeKeyboardControl iupx11_XChangeKeyboardControl
#define XDeleteProperty iupx11_XDeleteProperty
#define XSendEvent iupx11_XSendEvent
#define XGetWindowAttributes iupx11_XGetWindowAttributes
#define XVisualIDFromVisual iupx11_XVisualIDFromVisual

static int iupX11Open(void)
{
  if (iupx11_handle)
    return 1;

  iupx11_handle = dlopen("libX11.so.6", RTLD_LAZY | RTLD_LOCAL);
  if (!iupx11_handle)
    iupx11_handle = dlopen("libX11.so", RTLD_LAZY | RTLD_LOCAL);

  if (!iupx11_handle)
    return 0;

  iupx11_XCreateGC = (GC (*)(Display*, XID, unsigned long, void*))dlsym(iupx11_handle, "XCreateGC");
  iupx11_XFreeGC = (int (*)(Display*, GC))dlsym(iupx11_handle, "XFreeGC");
  iupx11_XDefaultScreen = (int (*)(Display*))dlsym(iupx11_handle, "XDefaultScreen");
  iupx11_XServerVendor = (char* (*)(Display*))dlsym(iupx11_handle, "XServerVendor");
  iupx11_XVendorRelease = (int (*)(Display*))dlsym(iupx11_handle, "XVendorRelease");
  iupx11_XMoveWindow = (int (*)(Display*, Window, int, int))dlsym(iupx11_handle, "XMoveWindow");
  iupx11_XSync = (int (*)(Display*, int))dlsym(iupx11_handle, "XSync");
  iupx11_XInternAtom = (Atom (*)(Display*, const char*, int))dlsym(iupx11_handle, "XInternAtom");
  iupx11_XSetWMNormalHints = (int (*)(Display*, Window, XSizeHints*))dlsym(iupx11_handle, "XSetWMNormalHints");
  iupx11_XChangeProperty = (int (*)(Display*, Window, Atom, Atom, int, int, const unsigned char*, int))dlsym(iupx11_handle, "XChangeProperty");
  iupx11_XWarpPointer = (int (*)(Display*, Window, Window, int, int, unsigned int, unsigned int, int, int))dlsym(iupx11_handle, "XWarpPointer");
  iupx11_XRootWindow = (Window (*)(Display*, int))dlsym(iupx11_handle, "XRootWindow");
  iupx11_XQueryPointer = (int (*)(Display*, Window, Window*, Window*, int*, int*, int*, int*, unsigned int*))dlsym(iupx11_handle, "XQueryPointer");
  iupx11_XTranslateCoordinates = (int (*)(Display*, Window, Window, int, int, int*, int*, Window*))dlsym(iupx11_handle, "XTranslateCoordinates");
  iupx11_XChangeKeyboardControl = (int (*)(Display*, unsigned long, XKeyboardControl*))dlsym(iupx11_handle, "XChangeKeyboardControl");
  iupx11_XDeleteProperty = (int (*)(Display*, Window, Atom))dlsym(iupx11_handle, "XDeleteProperty");
  iupx11_XSendEvent = (int (*)(Display*, Window, int, long, XEvent*))dlsym(iupx11_handle, "XSendEvent");
  iupx11_XGetWindowAttributes = (int (*)(Display*, Window, XWindowAttributes*))dlsym(iupx11_handle, "XGetWindowAttributes");
  iupx11_XVisualIDFromVisual = (VisualID (*)(Visual*))dlsym(iupx11_handle, "XVisualIDFromVisual");

  if (!iupx11_XDefaultScreen || !iupx11_XServerVendor || !iupx11_XVendorRelease)
  {
    dlclose(iupx11_handle);
    iupx11_handle = NULL;
    return 0;
  }

  return 1;
}

static void iupX11Close(void)
{
  if (iupx11_handle)
  {
    dlclose(iupx11_handle);
    iupx11_handle = NULL;
  }
}

#ifdef __cplusplus
}
#endif

#endif
