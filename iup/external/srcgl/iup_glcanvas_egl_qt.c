/** \file
 * \brief Qt backend for EGL GLCanvas.
 *        Included from iup_glcanvas_egl.c, not compiled standalone.
 *
 * See Copyright Notice in "iup.h"
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations for Qt types */
typedef struct _QWindow QWindow;
typedef struct _QWidget QWidget;
typedef struct _QTimer QTimer;

/* Qt helper functions (implemented in iup_glcanvas_egl_qt.cpp) */
extern unsigned long iupQtGetNativeWindow(void* qwindow_ptr);
extern void* iupQtGetWindowFromWidget(void* qwidget_ptr);
extern double iupQtGetDevicePixelRatio(void* qwindow_ptr);
extern void iupQtGetWidgetSize(void* qwidget_ptr, int* width, int* height);
extern const char* iupQtGetPlatformName(void);
extern int iupQtHasWaylandSupport(void);

#if !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)
  #define IUP_EGL_HAS_WAYLAND
  #include <wayland-client.h>
  #include <wayland-egl.h>

  struct wl_display;
  struct wl_compositor;
  struct wl_subcompositor;
  struct wl_surface;
  struct wl_subsurface;
  struct wl_egl_window;

  extern struct wl_display* iupQtGetWaylandDisplay(void);
  extern struct wl_compositor* iupQtGetWaylandCompositor(void);
  extern struct wl_surface* iupQtGetWaylandSurface(void* qwindow_ptr);
  extern void* iupQtGetWaylandSurfaceIfReady(void* qwindow_ptr);
  extern int iupQtRegisterSurfaceReadyCallback(void* qwindow_ptr, void* ih, void (*callback)(void*, void*), void* user_data);
  extern void iupQtGetWidgetPosition(void* qwidget_ptr, int* x, int* y);
  extern void iupQtGetWindowFrameMargins(void* qwindow_ptr, int* left, int* top, int* right, int* bottom);
  extern void* iupQtGetTopLevelWindow(void* qwidget_ptr);
  extern void* iupQtGetWindowFromWidget(void* qwidget_ptr);

  #include "iup_glcanvas_egl_wayland.c"
#endif

#ifdef __cplusplus
}
#endif

static void iupEGLBackendGetScaleAndSize(Ihandle* ih, IGlControlData* gldata, int* scale, int* realized_w, int* realized_h)
{
  QWindow* qwindow = (QWindow*)gldata->backend_handle;
  *scale = 1;
  *realized_w = 0;
  *realized_h = 0;

  if (ih->handle)
  {
    iupQtGetWidgetSize(ih->handle, realized_w, realized_h);

    if (qwindow)
    {
      double dpr = iupQtGetDevicePixelRatio(qwindow);
      *scale = (int)dpr;
      if (*scale < 1) *scale = 1;
    }
  }

}

static int iupEGLBackendMapInit(Ihandle* ih, IGlControlData* gldata)
{
  QWindow* qwindow;
  void* canvas_widget = iupAttribGet(ih, "_IUPQT_CANVAS_WIDGET");

  if (!canvas_widget)
  {
    iupAttribSet(ih, "ERROR", "Could not obtain canvas widget context.");
    return 0;
  }

  qwindow = (QWindow*)iupAttribGet(ih, "QWINDOW");

  if (!qwindow)
  {
    qwindow = (QWindow*)iupQtGetWindowFromWidget(canvas_widget);

    if (qwindow)
    {
      iupAttribSet(ih, "QWINDOW", (char*)qwindow);
    }
  }

  gldata->backend_handle = qwindow;

  return 1;
}

static EGLDisplay iupEGLBackendGetEGLDisplay(Ihandle* ih, IGlControlData* gldata, PFN_eglGetPlatformDisplay func, EGLNativeWindowType* native_window, int* visual_id)
{
  QWindow* qwindow = (QWindow*)gldata->backend_handle;
  EGLDisplay display = EGL_NO_DISPLAY;
  const char* qt_platform = iupQtGetPlatformName();
  int has_wayland_support = iupQtHasWaylandSupport();

  (void)ih;
  *visual_id = 0;

  if (has_wayland_support && strcmp(qt_platform, "wayland") == 0) {
#if !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)
    struct wl_display* wl_display = iupQtGetWaylandDisplay();
    if (wl_display) {
      if (func) {
        display = func(EGL_PLATFORM_WAYLAND_KHR, wl_display, NULL);
      }
      if (display == EGL_NO_DISPLAY) {
        display = eglGetDisplay((EGLNativeDisplayType)wl_display);
      }
    }
#endif
  }
  else if (strcmp(qt_platform, "xcb") == 0 || !has_wayland_support) {
    if (func) {
      display = func(EGL_PLATFORM_X11_KHR, EGL_DEFAULT_DISPLAY, NULL);
    }

    if (qwindow) {
      unsigned long xid = iupQtGetNativeWindow(qwindow);

      if (xid != 0) {
        *native_window = (EGLNativeWindowType)xid;
      }
    }
  }

  return display;
}

static EGLNativeWindowType iupEGLBackendPostConfig(Ihandle* ih, IGlControlData* gldata, int* skip_rest)
{
  QWindow* qwindow = (QWindow*)gldata->backend_handle;
  const char* platform_name = iupQtGetPlatformName();
  int has_qt_wayland_support = iupQtHasWaylandSupport();

  *skip_rest = 0;

  if (has_qt_wayland_support && platform_name && strcmp(platform_name, "wayland") == 0) {
#if !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)
    iupAttribSet(ih, "_IUP_EGL_LAZY_INIT", "1");
    *skip_rest = 1;
    return (EGLNativeWindowType)NULL;
#endif
  }

  /* X11 or XWayland path */
  if (qwindow) {
    unsigned long xid = iupQtGetNativeWindow(qwindow);

    if (xid != 0)
      return (EGLNativeWindowType)xid;
  }

  (void)ih;

  return (EGLNativeWindowType)NULL;
}

#if !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)
static EGLNativeWindowType eGLCanvasCreateQt6WaylandSubsurface(Ihandle* ih, IGlControlData* gldata)
{
  EGLNativeWindowType native_window = (EGLNativeWindowType)NULL;
  struct wl_display* wl_display;
  void* toplevel_qwindow;
  struct wl_surface* parent_surface;
  int physical_width = 0, physical_height = 0;

  wl_display = iupQtGetWaylandDisplay();

  toplevel_qwindow = iupQtGetTopLevelWindow(ih->handle);
  if (!toplevel_qwindow) {

    return native_window;
  }

  parent_surface = (struct wl_surface*)iupQtGetWaylandSurfaceIfReady(toplevel_qwindow);

  if (!wl_display || !parent_surface) {
    return native_window;
  }

  gldata->parent_surface = parent_surface;
  gldata->backend_handle = toplevel_qwindow;

  gldata->compositor = iupQtGetWaylandCompositor();

  if (gldata->compositor) {
    if (!gldata->subcompositor) {
      gldata->subcompositor = eGLCanvasGetWaylandSubcompositor(wl_display, &gldata->event_queue, &gldata->registry_compositor, &gldata->registry);
    }
  }

  if (gldata->compositor) {
    gldata->subsurface_wl = wl_compositor_create_surface(gldata->compositor);

    if (gldata->subsurface_wl && gldata->subcompositor) {
      gldata->subsurface = wl_subcompositor_get_subsurface(
          gldata->subcompositor,
          gldata->subsurface_wl,
          parent_surface);

      if (gldata->subsurface) {
        wl_subsurface_set_desync(gldata->subsurface);

        {
          int x = 0, y = 0;
          iupQtGetWidgetPosition(ih->handle, &x, &y);

          {
            int margin_left = 0, margin_top = 0, margin_right = 0, margin_bottom = 0;
            iupQtGetWindowFrameMargins(toplevel_qwindow, &margin_left, &margin_top, &margin_right, &margin_bottom);

            if (margin_left > 0 || margin_top > 0) {
              x += margin_left;
              y += margin_top;
            }
          }

          wl_subsurface_set_position(gldata->subsurface, x, y);
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
      }
    }
  }

  return native_window;
}
#endif /* !IUP_QT_DISABLE_WAYLAND_INCLUDES */

static int iupEGLBackendCreateLazyNativeWindow(Ihandle* ih, IGlControlData* gldata, EGLNativeWindowType* native_window, EGLint* context_attribs, int max_attribs)
{
  (void)max_attribs;

#if !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)
  *native_window = eGLCanvasCreateQt6WaylandSubsurface(ih, gldata);

  if (*native_window == (EGLNativeWindowType)NULL) {
    /* Window not ready yet. Return 0 to retry silently on next GLMakeCurrent. */
    return 0;
  }

  context_attribs[0] = EGL_NONE;
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
#if !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)
  if (gldata->subsurface && ih->handle) {
    int x = 0, y = 0;
    iupQtGetWidgetPosition(ih->handle, &x, &y);

    {
      void* toplevel_qwindow = iupQtGetTopLevelWindow(ih->handle);
      if (toplevel_qwindow) {
        int margin_left = 0, margin_top = 0, margin_right = 0, margin_bottom = 0;
        iupQtGetWindowFrameMargins(toplevel_qwindow, &margin_left, &margin_top, &margin_right, &margin_bottom);

        if (margin_left > 0 || margin_top > 0) {
          x += margin_left;
          y += margin_top;
        }
      }
    }

    wl_subsurface_set_position(gldata->subsurface, x, y);
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
  (void)ih;
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
