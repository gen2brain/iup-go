/** \file
 * \brief GTK3 backend for EGL GLCanvas.
 *        Included from iup_glcanvas_egl.c, not compiled standalone.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_GLCANVAS_EGL_GTK_H
#define __IUP_GLCANVAS_EGL_GTK_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#ifdef GDK_WINDOWING_WAYLAND
  #define IUP_EGL_HAS_WAYLAND
  #include <gdk/gdkwayland.h>
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

static void eGLCanvasGtk3GetSubsurfacePosition(Ihandle* ih, int* x, int* y)
{
  GtkWidget* widget = GTK_WIDGET(ih->handle);
  *x = 0;
  *y = 0;
  if (widget && gtk_widget_get_realized(widget)) {
    GtkWidget* parent = gtk_widget_get_parent(widget);
    if (parent) {
      gtk_widget_translate_coordinates(widget, gtk_widget_get_toplevel(widget), 0, 0, x, y);
      if (*x < 0) *x = 0;
      if (*y < 0) *y = 0;
    }
  }
}

static int iupEGLBackendCreateLazyNativeWindow(Ihandle* ih, IGlControlData* gldata, EGLNativeWindowType* native_window, EGLint* context_attribs, int max_attribs)
{
  (void)max_attribs;

#ifdef GDK_WINDOWING_WAYLAND
  {
    GdkWindow* window = (GdkWindow*)gldata->backend_handle;
    if (GDK_IS_WAYLAND_WINDOW(window))
    {
      GdkWindow* toplevel_window = gdk_window_get_toplevel(window);
      GdkDisplay* gdk_display = gdk_window_get_display(window);
      struct wl_surface* parent_surface = toplevel_window ? gdk_wayland_window_get_wl_surface(toplevel_window) : NULL;
      struct wl_display* wl_display = gdk_wayland_display_get_wl_display(gdk_display);

      if (wl_display && parent_surface)
      {
        int x = 0, y = 0;
        int scale = gdk_window_get_scale_factor(window);
        struct IGlWaylandSubsurface ws;

        eGLCanvasGtk3GetSubsurfacePosition(ih, &x, &y);

        if (iupGLCreateWaylandSubsurface(wl_display, parent_surface,
              x, y, ih->currentwidth, ih->currentheight, scale, &ws))
        {
          eGLCopyWaylandSubsurface(&ws, gldata, parent_surface);
          *native_window = (EGLNativeWindowType)gldata->egl_window;

          context_attribs[0] = EGL_CONTEXT_CLIENT_VERSION;
          context_attribs[1] = 2;
          context_attribs[2] = EGL_NONE;
          return 1;
        }
      }

      iupAttribSet(ih, "ERROR", "Failed to create Wayland subsurface during lazy initialization");
      return -1;
    }
  }
#endif

  (void)ih; (void)gldata; (void)native_window; (void)context_attribs;
  return -1;
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

#endif
