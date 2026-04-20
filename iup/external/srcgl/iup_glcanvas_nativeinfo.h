/** \file
 * \brief GLCanvas native window management.
 *        Provides a generic shim to query native handles using IUP's attribute
 *        system, and platform-specific child window creation for GL rendering
 *        when the canvas widget lacks its own native window.
 *
 *        Included as a static-function header by iup_glcanvas_win.c,
 *        iup_glcanvas_cocoa.m, and the EGL backend files.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef IUP_GLCANVAS_NATIVEINFO_H
#define IUP_GLCANVAS_NATIVEINFO_H

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"

typedef struct _IGlNativeInfo
{
  const char* windowing;

  int has_own_window;
  void* canvas_window;
  void* parent_window;
  void* display;

  int x, y, w, h;

  int surface_offset_x;
  int surface_offset_y;
} IGlNativeInfo;

static int iupGLGetNativeInfo(Ihandle* ih, IGlNativeInfo* info)
{
  const char* windowing;
  const char* handle_attr = NULL;
  void* canvas_handle = NULL;
  void* dialog_handle = NULL;
  Ihandle* dialog;

  memset(info, 0, sizeof(IGlNativeInfo));

  windowing = IupGetGlobal("WINDOWING");
  if (!windowing)
    return 0;

  info->windowing = windowing;

  if (strcmp(windowing, "DWM") == 0)
    handle_attr = "HWND";
  else if (strcmp(windowing, "QUARTZ") == 0)
    handle_attr = "NSVIEW";
  else if (strcmp(windowing, "X11") == 0)
    handle_attr = "XWINDOW";
  else if (strcmp(windowing, "WAYLAND") == 0)
    handle_attr = "WL_SURFACE";
  else
    return 0;

  dialog = IupGetDialog(ih);
  if (!dialog)
    return 0;

  dialog_handle = IupGetAttribute(dialog, handle_attr);
  if (!dialog_handle)
    return 0;

  canvas_handle = IupGetAttribute(ih, handle_attr);

  if (canvas_handle && canvas_handle != dialog_handle)
  {
    info->has_own_window = 1;
    info->canvas_window = canvas_handle;
  }
  else
  {
    info->has_own_window = 0;
    info->parent_window = dialog_handle;
  }

  info->x = ih->x;
  info->y = ih->y;
  info->w = ih->currentwidth;
  info->h = ih->currentheight;
  if (info->w <= 0) info->w = 1;
  if (info->h <= 0) info->h = 1;

  if (strcmp(windowing, "X11") == 0)
    info->display = IupGetGlobal("XDISPLAY");
  else if (strcmp(windowing, "WAYLAND") == 0)
    info->display = IupGetGlobal("WL_DISPLAY");

  if (strcmp(windowing, "WAYLAND") == 0)
  {
    char* offset = iupAttribGet(dialog, "_IUP_GL_SURFACE_OFFSET");
    if (offset)
      iupStrToIntInt(offset, &info->surface_offset_x, &info->surface_offset_y, 'x');
  }

  return 1;
}

/* ======================================================================
 * Win32: Child HWND creation for GL rendering
 * ====================================================================== */

#ifdef _WIN32
#include <windows.h>

static const char* iupGL_WndClassName = "IupGLCanvasChild";
static int iupGL_WndClassRegistered = 0;

static LRESULT CALLBACK iupGLChildWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  return DefWindowProc(hwnd, msg, wp, lp);
}

static void iupGLRegisterChildWndClass(void)
{
  if (!iupGL_WndClassRegistered)
  {
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(WNDCLASSA));
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = iupGLChildWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = iupGL_WndClassName;
    RegisterClassA(&wc);
    iupGL_WndClassRegistered = 1;
  }
}

static HWND iupGLCreateChildWindow(void* parent, int x, int y, int w, int h)
{
  HWND parent_hwnd = (HWND)parent;
  HWND child;

  /* Ensure parent clips children so toolkit painting doesn't cover the child HWND */
  SetWindowLongPtr(parent_hwnd, GWL_STYLE,
    GetWindowLongPtr(parent_hwnd, GWL_STYLE) | WS_CLIPCHILDREN);

  iupGLRegisterChildWndClass();
  child = CreateWindowExA(0, iupGL_WndClassName, NULL,
    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
    x, y, w, h, parent_hwnd, NULL, GetModuleHandle(NULL), NULL);

  return child;
}

static void iupGLMoveChildWindow(HWND child, int x, int y, int w, int h)
{
  if (w <= 0 || h <= 0)
    return;
  MoveWindow(child, x, y, w, h, TRUE);
}

static void iupGLDestroyChildWindow(HWND child)
{
  DestroyWindow(child);
}

#endif /* _WIN32 */

/* ======================================================================
 * macOS/Cocoa: Child NSView creation for GL rendering
 * ====================================================================== */

#if defined(__APPLE__) && defined(__OBJC__)
#import <Cocoa/Cocoa.h>

static NSView* iupGLCreateChildView(NSView* parent, int x, int y, int w, int h)
{
  NSRect parent_frame = [parent frame];
  NSRect frame = NSMakeRect(x, parent_frame.size.height - y - h, w, h);
  NSView* child = [[NSView alloc] initWithFrame:frame];
  [child setAutoresizesSubviews:NO];
  [child setWantsLayer:NO];
  [parent addSubview:child positioned:NSWindowAbove relativeTo:nil];
  return child;
}

static void iupGLMoveChildView(NSView* child, int x, int y, int w, int h)
{
  NSView* parent = [child superview];
  if (parent)
  {
    NSRect parent_frame = [parent frame];
    NSRect frame = NSMakeRect(x, parent_frame.size.height - y - h, w, h);
    [child setFrame:frame];
  }
}

static void iupGLDestroyChildView(NSView* child)
{
  [child removeFromSuperview];
  [child release];
}

#endif /* __APPLE__ && __OBJC__ */

/* ======================================================================
 * Wayland: Subsurface creation for GL rendering
 *
 * Uses the Wayland registry protocol to obtain wl_compositor and
 * wl_subcompositor. This is toolkit-independent, works for any driver
 * that sets the WL_DISPLAY and WL_SURFACE globals/attributes.
 * ====================================================================== */

#if !defined(_WIN32) && !defined(__APPLE__) && !defined(IUP_GL_NO_WAYLAND)
#define IUP_GL_HAS_WAYLAND
#include <wayland-client.h>
#include <wayland-egl.h>

struct IGlWaylandSubsurface
{
  struct wl_surface* surface;
  struct wl_subsurface* subsurface;
  struct wl_egl_window* egl_window;

  struct wl_compositor* compositor;
  struct wl_subcompositor* subcompositor;
  struct wl_registry* registry;
  struct wl_event_queue* event_queue;

  int physical_width;
  int physical_height;
};

struct iupgl_wl_registry_data
{
  struct wl_compositor* compositor;
  struct wl_subcompositor* subcompositor;
};

static void iupgl_wl_registry_global(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
  struct iupgl_wl_registry_data* d = (struct iupgl_wl_registry_data*)data;
  (void)version;

  if (strcmp(interface, "wl_compositor") == 0)
    d->compositor = (struct wl_compositor*)wl_registry_bind(registry, name, &wl_compositor_interface, 1);
  else if (strcmp(interface, "wl_subcompositor") == 0)
    d->subcompositor = (struct wl_subcompositor*)wl_registry_bind(registry, name, &wl_subcompositor_interface, 1);
}

static void iupgl_wl_registry_global_remove(void* data, struct wl_registry* registry, uint32_t name)
{
  (void)data; (void)registry; (void)name;
}

static const struct wl_registry_listener iupgl_wl_registry_listener = {
  iupgl_wl_registry_global,
  iupgl_wl_registry_global_remove
};

static int iupGLCreateWaylandSubsurface(struct wl_display* wl_display, struct wl_surface* parent_surface,
                                         int x, int y, int w, int h, int scale, struct IGlWaylandSubsurface* out)
{
  struct iupgl_wl_registry_data reg_data = { NULL, NULL };

  memset(out, 0, sizeof(struct IGlWaylandSubsurface));

  if (!wl_display || !parent_surface)
    return 0;

  out->event_queue = wl_display_create_queue(wl_display);
  if (!out->event_queue)
    return 0;

  out->registry = wl_display_get_registry(wl_display);
  if (!out->registry)
  {
    wl_event_queue_destroy(out->event_queue);
    out->event_queue = NULL;
    return 0;
  }

  wl_proxy_set_queue((struct wl_proxy*)out->registry, out->event_queue);
  wl_registry_add_listener(out->registry, &iupgl_wl_registry_listener, &reg_data);

  if (wl_display_roundtrip_queue(wl_display, out->event_queue) < 0)
  {
    wl_registry_destroy(out->registry);
    wl_event_queue_destroy(out->event_queue);
    out->registry = NULL;
    out->event_queue = NULL;
    return 0;
  }

  out->compositor = reg_data.compositor;
  out->subcompositor = reg_data.subcompositor;

  if (!out->compositor || !out->subcompositor)
  {
    wl_registry_destroy(out->registry);
    wl_event_queue_destroy(out->event_queue);
    out->registry = NULL;
    out->event_queue = NULL;
    return 0;
  }

  out->surface = wl_compositor_create_surface(out->compositor);
  if (!out->surface)
    return 0;

  out->subsurface = wl_subcompositor_get_subsurface(out->subcompositor, out->surface, parent_surface);
  if (!out->subsurface)
  {
    wl_surface_destroy(out->surface);
    out->surface = NULL;
    return 0;
  }

  wl_subsurface_set_desync(out->subsurface);
  wl_subsurface_set_position(out->subsurface, x, y);

  if (scale < 1) scale = 1;
  out->physical_width = w * scale;
  out->physical_height = h * scale;
  if (out->physical_width < 1) out->physical_width = 1;
  if (out->physical_height < 1) out->physical_height = 1;

  out->egl_window = wl_egl_window_create(out->surface, out->physical_width, out->physical_height);
  if (!out->egl_window)
  {
    wl_subsurface_destroy(out->subsurface);
    wl_surface_destroy(out->surface);
    out->subsurface = NULL;
    out->surface = NULL;
    return 0;
  }

  return 1;
}

static void iupGLMoveWaylandSubsurface(struct IGlWaylandSubsurface* ws, int x, int y, int w, int h, int scale)
{
  if (ws->subsurface)
    wl_subsurface_set_position(ws->subsurface, x, y);

  if (scale < 1) scale = 1;

  if (ws->egl_window)
  {
    int pw = w * scale;
    int ph = h * scale;
    if (pw < 1) pw = 1;
    if (ph < 1) ph = 1;

    if (pw != ws->physical_width || ph != ws->physical_height)
    {
      wl_egl_window_resize(ws->egl_window, pw, ph, 0, 0);
      ws->physical_width = pw;
      ws->physical_height = ph;
    }
  }
}

static void iupGLDestroyWaylandSubsurface(struct IGlWaylandSubsurface* ws)
{
  if (ws->egl_window)
  {
    wl_egl_window_destroy(ws->egl_window);
    ws->egl_window = NULL;
  }
  if (ws->subsurface)
  {
    wl_subsurface_destroy(ws->subsurface);
    ws->subsurface = NULL;
  }
  if (ws->surface)
  {
    wl_surface_destroy(ws->surface);
    ws->surface = NULL;
  }
  if (ws->registry)
  {
    wl_registry_destroy(ws->registry);
    ws->registry = NULL;
  }
  if (ws->event_queue)
  {
    wl_event_queue_destroy(ws->event_queue);
    ws->event_queue = NULL;
  }
}

#endif /* IUP_GL_HAS_WAYLAND */

#endif /* IUP_GLCANVAS_NATIVEINFO_H */
