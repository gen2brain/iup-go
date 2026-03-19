/** \file
 * \brief GTK3 backend for EGL GLCanvas.
 *        Included from iup_glcanvas_egl.c, not compiled standalone.
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#ifdef GDK_WINDOWING_WAYLAND
  #define IUP_EGL_HAS_WAYLAND
  #include <gdk/gdkwayland.h>
  #include <wayland-egl.h>
  #include <wayland-client.h>
  #include "iup_glcanvas_egl_wayland.c"
#endif

#ifdef GDK_WINDOWING_X11
  #include <gdk/gdkx.h>
#endif

static void iupEGLBackendGetScaleAndSize(Ihandle* ih, IGlControlData* gldata, int* scale, int* realized_w, int* realized_h)
{
  GdkWindow* window = (GdkWindow*)gldata->backend_handle;
  *scale = 1;
  *realized_w = 0;
  *realized_h = 0;

#if GTK_CHECK_VERSION(3, 10, 0)
  if (window && GDK_IS_WINDOW(window))
  {
    *scale = gdk_window_get_scale_factor(window);
  }
  else if (ih->handle && GTK_IS_WIDGET(ih->handle))
  {
    *scale = gtk_widget_get_scale_factor((GtkWidget*)ih->handle);
  }
#endif
  if (*scale < 1) *scale = 1;

#ifdef GDK_WINDOWING_WAYLAND
  if (gldata->subsurface)
  {
    if (ih->handle && GTK_IS_WIDGET(ih->handle))
    {
      GtkAllocation allocation;
      gtk_widget_get_allocation((GtkWidget*)ih->handle, &allocation);
      if (allocation.width > 0 && allocation.height > 0)
      {
        *realized_w = allocation.width;
        *realized_h = allocation.height;
      }
    }
  }
  else
#endif
  {
    if (window && GDK_IS_WINDOW(window))
    {
      *realized_w = gdk_window_get_width(window);
      *realized_h = gdk_window_get_height(window);
    }

    if (*realized_w <= 0 || *realized_h <= 0)
    {
      if (ih->handle && GTK_IS_WIDGET(ih->handle))
      {
        GtkAllocation allocation;
        gtk_widget_get_allocation((GtkWidget*)ih->handle, &allocation);
        if (allocation.width > 0 && allocation.height > 0)
        {
          *realized_w = allocation.width;
          *realized_h = allocation.height;
        }
      }
    }
  }
}

static int iupEGLBackendMapInit(Ihandle* ih, IGlControlData* gldata)
{
  GdkWindow* window;

  window = (GdkWindow*)iupAttribGet(ih, "GDKWINDOW");

  if (!window && ih->handle && GTK_IS_WIDGET(ih->handle))
  {
    GtkWidget* widget = (GtkWidget*)ih->handle;

    if (!gtk_widget_get_realized(widget))
    {
      gtk_widget_realize(widget);
    }

    window = gtk_widget_get_window(widget);

    if (window)
    {
      iupAttribSet(ih, "GDKWINDOW", (char*)window);

#ifdef GDK_WINDOWING_WAYLAND
      if (GDK_IS_WAYLAND_DISPLAY(gdk_window_get_display(window)))
      {
        GdkWindowType win_type = gdk_window_get_window_type(window);
        GdkWindow* parent = gdk_window_get_parent(window);
        GdkWindow* toplevel = gdk_window_get_toplevel(window);
        (void)win_type; (void)parent; (void)toplevel;
      }
#endif
    }
  }

  if (!window)
  {
    char* window_ptr = IupGetAttribute(ih, "GDKWINDOW");
    if (window_ptr)
    {
      window = (GdkWindow*)window_ptr;
    }
  }

  if (!window || !GDK_IS_WINDOW(window))
  {
    iupAttribSet(ih, "ERROR", "Could not obtain valid GdkWindow handle.");
    gldata->backend_handle = NULL;
    return 0;
  }

  gldata->backend_handle = window;
  return 1;
}

static EGLDisplay iupEGLBackendGetEGLDisplay(Ihandle* ih, IGlControlData* gldata, PFN_eglGetPlatformDisplay func, EGLNativeWindowType* native_window, int* visual_id)
{
  GdkWindow* window = (GdkWindow*)gldata->backend_handle;
  GdkDisplay* gdk_display = gdk_window_get_display(window);
  EGLDisplay display = EGL_NO_DISPLAY;

  (void)ih;
  *visual_id = 0;

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY(gdk_display)) {
    struct wl_display* wl_display = gdk_wayland_display_get_wl_display(gdk_display);

    if (func) {
      display = func(EGL_PLATFORM_WAYLAND_KHR, wl_display, NULL);
    }

    if (display == EGL_NO_DISPLAY) {
      display = eglGetDisplay((EGLNativeDisplayType)wl_display);
    }
  }
  else
#endif
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY(gdk_display)) {
    Display* x_display = gdk_x11_display_get_xdisplay(gdk_display);

    if (func) {
      display = func(EGL_PLATFORM_X11_KHR, x_display, NULL);
    }

    if (display == EGL_NO_DISPLAY) {
      display = eglGetDisplay((EGLNativeDisplayType)x_display);
    }
  }
#endif
  {
    (void)native_window;
  }

  return display;
}

static EGLNativeWindowType iupEGLBackendPostConfig(Ihandle* ih, IGlControlData* gldata, int* skip_rest)
{
  GdkWindow* window = (GdkWindow*)gldata->backend_handle;
  *skip_rest = 0;

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_WINDOW(window)) {
    iupAttribSet(ih, "_IUP_EGL_LAZY_INIT", "1");
    return (EGLNativeWindowType)NULL;
  }
  else
#endif
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_WINDOW(window)) {
    return (EGLNativeWindowType)gdk_x11_window_get_xid(window);
  }
#endif

  (void)ih;
  return (EGLNativeWindowType)NULL;
}

#ifdef GDK_WINDOWING_WAYLAND
static EGLNativeWindowType eGLCanvasCreateGtk3WaylandSubsurface(Ihandle* ih, IGlControlData* gldata)
{
  EGLNativeWindowType native_window = (EGLNativeWindowType)NULL;
  GdkWindow* window = (GdkWindow*)gldata->backend_handle;
  GdkWindow* toplevel_window;
  struct wl_surface* parent_surface;
  GdkDisplay* gdk_display;
  struct wl_display* wl_display;
  int physical_width = 0, physical_height = 0;

  if (!GDK_IS_WAYLAND_WINDOW(window)) {
    return native_window;
  }

  toplevel_window = gdk_window_get_toplevel(window);

  if (!toplevel_window || !GDK_IS_WAYLAND_WINDOW(toplevel_window)) {
    return native_window;
  }

  parent_surface = gdk_wayland_window_get_wl_surface(toplevel_window);

  if (!parent_surface) {
    return native_window;
  }

  gldata->parent_surface = parent_surface;

  gdk_display = gdk_window_get_display(window);
  wl_display = gdk_wayland_display_get_wl_display(gdk_display);

  if (!wl_display) {
    return native_window;
  }

  gldata->compositor = gdk_wayland_display_get_wl_compositor(gdk_display);

  if (!gldata->compositor) {
    return native_window;
  }

  gldata->subsurface_wl = wl_compositor_create_surface(gldata->compositor);

  if (!gldata->subsurface_wl) {
    return native_window;
  }

  if (!gldata->subcompositor) {
    gldata->subcompositor = eGLCanvasGetWaylandSubcompositor(wl_display, &gldata->event_queue, &gldata->registry_compositor, &gldata->registry);
  }

  if (!gldata->subcompositor) {
    wl_surface_destroy(gldata->subsurface_wl);
    gldata->subsurface_wl = NULL;
    return native_window;
  }

  gldata->subsurface = wl_subcompositor_get_subsurface(
      gldata->subcompositor,
      gldata->subsurface_wl,
      parent_surface);

  if (!gldata->subsurface) {
    wl_surface_destroy(gldata->subsurface_wl);
    gldata->subsurface_wl = NULL;
    return native_window;
  }

  wl_subsurface_set_desync(gldata->subsurface);

  {
    int subsurface_x = 0, subsurface_y = 0;

    GtkWidget* widget = GTK_WIDGET(ih->handle);
    if (widget && gtk_widget_get_realized(widget)) {
      GtkWidget* parent = gtk_widget_get_parent(widget);
      if (parent) {
        gtk_widget_translate_coordinates(widget, gtk_widget_get_toplevel(widget), 0, 0, &subsurface_x, &subsurface_y);

        if (subsurface_x < 0 || subsurface_y < 0) {
          subsurface_x = 0;
          subsurface_y = 0;
        }
      }
    }

    wl_subsurface_set_position(gldata->subsurface, subsurface_x, subsurface_y);
  }

  if (gldata->parent_surface) {
    wl_surface_commit(gldata->parent_surface);
  }

  eGLCanvasGetActualSize(ih, gldata, &physical_width, &physical_height);

  gldata->egl_window = wl_egl_window_create(gldata->subsurface_wl, physical_width, physical_height);

  if (gldata->egl_window) {
    native_window = (EGLNativeWindowType)gldata->egl_window;
    gldata->egl_window_physical_width = physical_width;
    gldata->egl_window_physical_height = physical_height;
  }

  return native_window;
}
#endif /* GDK_WINDOWING_WAYLAND */

static int iupEGLBackendCreateLazyNativeWindow(Ihandle* ih, IGlControlData* gldata, EGLNativeWindowType* native_window, EGLint* context_attribs, int max_attribs)
{
  (void)max_attribs;

#ifdef GDK_WINDOWING_WAYLAND
  *native_window = eGLCanvasCreateGtk3WaylandSubsurface(ih, gldata);
  if (*native_window == (EGLNativeWindowType)NULL)
  {
    iupAttribSet(ih, "ERROR", "Failed to create Wayland subsurface during lazy initialization");
    return -1;
  }

  context_attribs[0] = EGL_CONTEXT_CLIENT_VERSION;
  context_attribs[1] = 2;
  context_attribs[2] = EGL_NONE;
  return 1;
#else
  (void)ih; (void)gldata; (void)native_window; (void)context_attribs;
  return -1;
#endif
}

static EGLNativeWindowType iupEGLBackendCheckSurfaceRecreation(Ihandle* ih, IGlControlData* gldata)
{
  (void)ih; (void)gldata;
  return (EGLNativeWindowType)NULL;
}

static void iupEGLBackendPostSurfaceCreation(Ihandle* ih, IGlControlData* gldata)
{
  (void)ih; (void)gldata;
}

static void iupEGLBackendUpdateSubsurfacePosition(Ihandle* ih, IGlControlData* gldata)
{
#ifdef GDK_WINDOWING_WAYLAND
  if (gldata->subsurface && ih->handle && GTK_IS_WIDGET(ih->handle)) {
    GtkWidget* widget = GTK_WIDGET(ih->handle);
    GtkWidget* parent = gtk_widget_get_parent(widget);

    if (parent && gtk_widget_get_realized(widget)) {
      int subsurface_x = 0, subsurface_y = 0;
      gtk_widget_translate_coordinates(widget, gtk_widget_get_toplevel(widget), 0, 0, &subsurface_x, &subsurface_y);

      wl_subsurface_set_position(gldata->subsurface, subsurface_x, subsurface_y);
    }
  }
#else
  (void)ih; (void)gldata;
#endif
}

static void iupEGLBackendCleanup(Ihandle* ih, IGlControlData* gldata)
{
  (void)ih;
  gldata->backend_handle = NULL;
}

static char* iupEGLBackendGetVisual(Ihandle* ih)
{
  GdkScreen* screen;
  (void)ih;
  screen = gdk_screen_get_default();
  if (screen) {
    GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
    if (!visual)
      visual = gdk_screen_get_system_visual(screen);
    return (char*)visual;
  }
  return NULL;
}

static void iupEGLBackendPreSwapBuffers(Ihandle* ih, IGlControlData* gldata)
{
  (void)ih; (void)gldata;
}

static void iupEGLBackendPostSwapBuffers(Ihandle* ih, IGlControlData* gldata)
{
  (void)ih; (void)gldata;
}
