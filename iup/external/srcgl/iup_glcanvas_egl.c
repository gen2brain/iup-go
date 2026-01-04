/** \file
 * \brief iupgl control for EGL
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include <GL/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#if defined(IUP_USE_GTK3)
  #define IUP_EGL_USE_GTK3
#elif defined(IUP_USE_GTK4)
  #define IUP_EGL_USE_GTK4
#elif defined(IUP_USE_QT)
  #define IUP_EGL_USE_QT
#elif defined(IUP_USE_EFL)
  #define IUP_EGL_USE_EFL
#else
  #error "No backend defined for EGL: must define IUP_USE_GTK3, IUP_USE_GTK4, IUP_USE_QT, or IUP_USE_EFL"
#endif

#if defined(IUP_EGL_USE_GTK3)
  #include <gtk/gtk.h>
  #include <gdk/gdk.h>

  #ifdef GDK_WINDOWING_WAYLAND
  #include <gdk/gdkwayland.h>
  #include <wayland-egl.h>
  #include <wayland-client.h>
  #endif

  #ifdef GDK_WINDOWING_X11
  #include <gdk/gdkx.h>
  #endif

#elif defined(IUP_EGL_USE_GTK4)
  #include <gtk/gtk.h>
  #include <gdk/gdk.h>

  #ifdef GDK_WINDOWING_WAYLAND
  #include <gdk/wayland/gdkwayland.h>
  #include <wayland-egl.h>
  #include <wayland-client.h>
  #endif

  #ifdef GDK_WINDOWING_X11
  #include <gdk/x11/gdkx.h>
  #include <dlfcn.h>
  /* X11 functions are deprecated in GTK4 but required for X11 backend */
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  #endif
#endif

#ifdef IUP_EGL_USE_QT
  #ifdef __cplusplus
  extern "C" {
  #endif

  /* Forward declarations */
  typedef struct _QWindow QWindow;
  typedef struct _QWidget QWidget;
  typedef struct _QTimer QTimer;

  /* Qt helper functions (implemented in iup_glcanvas_qt.cpp) */
  extern unsigned long iupQtGetNativeWindow(void* qwindow_ptr);
  extern void* iupQtGetWindowFromWidget(void* qwidget_ptr);
  extern double iupQtGetDevicePixelRatio(void* qwindow_ptr);
  extern void iupQtGetWidgetSize(void* qwidget_ptr, int* width, int* height);
  extern const char* iupQtGetPlatformName(void);
  extern int iupQtHasWaylandSupport(void);

  /* Qt6.7+ Wayland helper functions (for native Wayland support) */
  /* Only available when Qt >= 6.7 - check with iupQtHasWaylandSupport() */
  #if !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)
    /* Wayland client API */
    #include <wayland-client.h>
    #include <wayland-egl.h>

    /* Wayland client API forward declarations */
    struct wl_display;
    struct wl_compositor;
    struct wl_subcompositor;
    struct wl_surface;
    struct wl_subsurface;
    struct wl_egl_window;

    /* Functions to access Wayland objects from Qt6 native interfaces */
    /* These are only defined when Qt >= 6.7 (check at runtime with iupQtHasWaylandSupport) */
    extern struct wl_display* iupQtGetWaylandDisplay(void);
    extern struct wl_compositor* iupQtGetWaylandCompositor(void);
    extern struct wl_surface* iupQtGetWaylandSurface(void* qwindow_ptr);
    extern void* iupQtGetWaylandSurfaceIfReady(void* qwindow_ptr);
    extern int iupQtRegisterSurfaceReadyCallback(void* qwindow_ptr, void* ih, void (*callback)(void*, void*), void* user_data);
    extern void iupQtGetWidgetPosition(void* qwidget_ptr, int* x, int* y);
    extern void iupQtGetWindowFrameMargins(void* qwindow_ptr, int* left, int* top, int* right, int* bottom);
    extern void* iupQtGetTopLevelWindow(void* qwidget_ptr);
    extern void* iupQtGetWindowFromWidget(void* qwidget_ptr);
  #endif

  #ifdef __cplusplus
  }
  #endif
#endif

#ifdef IUP_EGL_USE_EFL
  #include <Ecore.h>
  #include <Ecore_Evas.h>
  #include <Evas.h>

  #ifdef HAVE_ECORE_WL2
    #include <Ecore_Wl2.h>
    #include <wayland-egl.h>
    #include <wayland-client.h>
  #endif

  #ifdef HAVE_ECORE_X
    #include <X11/Xlib.h>
    #include <Ecore_X.h>
  #endif

#endif

#include "iup.h"
#include "iupcbs.h"
#include "iupgl.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_assert.h"
#include "iup_register.h"
#include "iup_layout.h"
#include "iup_canvas.h"
#include "iup_dialog.h"

#ifdef IUP_EGL_USE_QT
  /* Qt canvas function from iupqt_canvas.cpp */
  #ifdef __cplusplus
  extern "C" {
  #endif
  extern void* iupqtCanvasGetContext(Ihandle* ih);
  #ifdef __cplusplus
  }
  #endif
#endif

/* Definitions for platform specific EGL display acquisition (EGL 1.5 or extensions) */
#ifndef EGL_PLATFORM_WAYLAND_KHR
#define EGL_PLATFORM_WAYLAND_KHR 0x31D8
#endif

#ifndef EGL_PLATFORM_X11_KHR
#define EGL_PLATFORM_X11_KHR 0x31D5
#endif

/* Function pointer type for eglGetPlatformDisplay/EXT. */
typedef PFNEGLGETPLATFORMDISPLAYPROC PFN_eglGetPlatformDisplay;

/* Definitions for ARB context creation if not present in system headers (EGL 1.5 or EGL_KHR_create_context) */
#ifndef EGL_CONTEXT_MAJOR_VERSION
#define EGL_CONTEXT_MAJOR_VERSION EGL_CONTEXT_CLIENT_VERSION
#endif
#ifndef EGL_CONTEXT_MINOR_VERSION
#define EGL_CONTEXT_MINOR_VERSION 0x30FB
#endif
/* Using EGL_CONTEXT_FLAGS_KHR as it's widely supported if EGL_KHR_create_context is available */
#ifndef EGL_CONTEXT_FLAGS_KHR
#define EGL_CONTEXT_FLAGS_KHR 0x30FC
#endif

/* If EGL 1.5 headers are present, use standard names, otherwise rely on KHR suffixes if available, else define standard values. */
#ifndef EGL_CONTEXT_OPENGL_PROFILE_MASK
#ifdef EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR
#define EGL_CONTEXT_OPENGL_PROFILE_MASK EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR
#else
#define EGL_CONTEXT_OPENGL_PROFILE_MASK 0x30FD
#endif
#endif

#ifndef EGL_CONTEXT_OPENGL_DEBUG_BIT
#ifdef EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR
#define EGL_CONTEXT_OPENGL_DEBUG_BIT EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR
#else
#define EGL_CONTEXT_OPENGL_DEBUG_BIT 0x00000001
#endif
#endif

#ifndef EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT
#ifdef EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR
#else
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT 0x00000002
#endif
#endif

#ifndef EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT
#ifdef EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR
#else
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT 0x00000001
#endif
#endif

#ifndef EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT
#ifdef EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR
#else
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT 0x00000002
#endif
#endif

#ifndef EGL_OPENGL_BIT
#define EGL_OPENGL_BIT 0x0008
#endif

#ifndef EGL_OPENGL_API
#define EGL_OPENGL_API 0x30A2
#endif

#ifdef IUP_EGL_USE_EFL
static int eGLCanvasEflIsWayland(Ecore_Evas* ee)
{
  const char* engine = ecore_evas_engine_name_get(ee);
  return engine && (strncmp(engine, "wayland", 7) == 0);
}

static int eGLCanvasEflIsX11(Ecore_Evas* ee)
{
  const char* engine = ecore_evas_engine_name_get(ee);
  return engine && (strstr(engine, "x11") != NULL);
}
#endif

typedef struct _IGlControlData
{
  EGLDisplay display;
  EGLSurface surface;
  EGLContext context;
  EGLConfig config;

#if defined(IUP_EGL_USE_GTK3)
  GdkWindow* window;
#elif defined(IUP_EGL_USE_GTK4)
  GdkSurface* gdk_surface;
#elif defined(IUP_EGL_USE_QT)
  QWindow* qwindow;
#elif defined(IUP_EGL_USE_EFL)
  Evas_Object* evas_obj;
  Ecore_Evas* ee;
#endif

  int last_logical_width;
  int last_logical_height;

/* Wayland EGL window support (GTK3/GTK4/Qt6/EFL) */
#if defined(GDK_WINDOWING_WAYLAND) || defined(IUP_EGL_USE_QT) || (defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_WL2))
  struct wl_egl_window* egl_window;
  int egl_window_physical_width;
  int egl_window_physical_height;
#endif

/* Wayland subsurface support (GTK3/GTK4/Qt6/EFL) */
#if (defined(IUP_EGL_USE_GTK3) && defined(GDK_WINDOWING_WAYLAND)) || (defined(IUP_EGL_USE_GTK4) && defined(GDK_WINDOWING_WAYLAND)) || (defined(IUP_EGL_USE_QT) && !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)) || (defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_WL2))
  struct wl_surface* subsurface_wl;      /* Wayland surface for the canvas subsurface */
  struct wl_subsurface* subsurface;       /* Wayland subsurface attached to parent */
  struct wl_surface* parent_surface;      /* Parent wl_surface (needed for commits after subsurface position changes) */
  struct wl_compositor* compositor;       /* Wayland compositor interface (from GTK/Qt/EFL, NOT owned by us) */
  struct wl_compositor* registry_compositor; /* Compositor from registry (on our queue, must be destroyed) */
  struct wl_subcompositor* subcompositor; /* Wayland subcompositor interface */
  struct wl_registry* registry;           /* Wayland registry (for cleanup) */
  struct wl_event_queue* event_queue;     /* Private event queue (kept alive while subcompositor is used) */
#endif

  /* For X11, the GdkWindow or QWindow encapsulates the XID, which is used directly with EGL. */
} IGlControlData;

/* -------------------------------------------------------------------------- */
/* Helper: Dynamic Loading of XLib (GTK4 X11 ONLY)                 */
/* -------------------------------------------------------------------------- */
#if defined(IUP_EGL_USE_GTK4) && defined(GDK_WINDOWING_X11)

/* Function pointer typedefs for XLib functions */
typedef int (*PFN_XGetWindowAttributes)(Display*, Window, XWindowAttributes*);
typedef VisualID (*PFN_XVisualIDFromVisual)(Visual*);

/**
 * \brief Dynamically loads XLib and retrieves the Visual ID for a window.
 * This avoids adding a link-time dependency on libX11.
 */
static int iupEGLGetX11VisualID(Display* x_display, unsigned long window_handle)
{
    int visual_id = 0;
    void* libx11 = NULL;
    PFN_XGetWindowAttributes ptr_XGetWindowAttributes = NULL;
    PFN_XVisualIDFromVisual ptr_XVisualIDFromVisual = NULL;

    /* Try to load libX11.so.6, or fallback to libX11.so */
    libx11 = dlopen("libX11.so.6", RTLD_LAZY);
    if (!libx11) {
        libx11 = dlopen("libX11.so", RTLD_LAZY);
    }

    if (libx11) {
        ptr_XGetWindowAttributes = (PFN_XGetWindowAttributes)dlsym(libx11, "XGetWindowAttributes");
        ptr_XVisualIDFromVisual = (PFN_XVisualIDFromVisual)dlsym(libx11, "XVisualIDFromVisual");

        if (ptr_XGetWindowAttributes && ptr_XVisualIDFromVisual) {
            XWindowAttributes attrs;
            /* XWindowAttributes struct is defined above to match X11 binary layout */
            if (ptr_XGetWindowAttributes(x_display, (Window)window_handle, &attrs) != 0) {
                visual_id = (int)ptr_XVisualIDFromVisual(attrs.visual);
            }
        }
        dlclose(libx11);
    }
    return visual_id;
}
#endif
/* -------------------------------------------------------------------------- */

static void eGLCanvasGetActualSize(Ihandle* ih, IGlControlData* gldata, int* physical_width, int* physical_height)
{
  int logical_w = 0, logical_h = 0;
  int realized_w = 0, realized_h = 0;
  int requested_w = 0, requested_h = 0;
  int scale = 1;

#if defined(IUP_EGL_USE_GTK3)
  /* Determine the scale factor (HiDPI support). GTK 3.10+ required. */
#if GTK_CHECK_VERSION(3, 10, 0)
  if (gldata->window && GDK_IS_WINDOW(gldata->window))
  {
    scale = gdk_window_get_scale_factor(gldata->window);
  }
  else if (ih->handle && GTK_IS_WIDGET(ih->handle))
  {
    /* Fallback for when GdkWindow is not yet available during early initialization */
    scale = gtk_widget_get_scale_factor((GtkWidget*)ih->handle);
  }
#endif
  if (scale < 1) scale = 1;

  /* For Wayland subsurface, use widget allocation size, not GdkWindow size.
   * With has_window=FALSE, gldata->window is the parent/toplevel window, but we need the canvas widget's size. */
#ifdef GDK_WINDOWING_WAYLAND
  if (gldata->subsurface)
  {
    if (ih->handle && GTK_IS_WIDGET(ih->handle))
    {
      GtkAllocation allocation;
      gtk_widget_get_allocation((GtkWidget*)ih->handle, &allocation);
      if (allocation.width > 0 && allocation.height > 0)
      {
        realized_w = allocation.width;
        realized_h = allocation.height;
      }
    }
  }
  else
#endif
  {
    if (gldata->window && GDK_IS_WINDOW(gldata->window))
    {
      realized_w = gdk_window_get_width(gldata->window);
      realized_h = gdk_window_get_height(gldata->window);
    }

    if (realized_w <= 0 || realized_h <= 0)
    {
      if (ih->handle && GTK_IS_WIDGET(ih->handle))
      {
        GtkAllocation allocation;
        gtk_widget_get_allocation((GtkWidget*)ih->handle, &allocation);
        if (allocation.width > 0 && allocation.height > 0)
        {
          realized_w = allocation.width;
          realized_h = allocation.height;
        }
      }
    }
  }
#endif /* IUP_EGL_USE_GTK3 */

#if defined(IUP_EGL_USE_GTK4)
  /* Determine the scale factor (HiDPI support). */
  if (gldata->gdk_surface && GDK_IS_SURFACE(gldata->gdk_surface))
  {
    scale = gdk_surface_get_scale_factor(gldata->gdk_surface);
  }
  else if (ih->handle && GTK_IS_WIDGET(ih->handle))
  {
    /* Fallback for when GdkSurface is not yet available during early initialization */
    scale = gtk_widget_get_scale_factor((GtkWidget*)ih->handle);
  }
  if (scale < 1) scale = 1;

  /* For Wayland subsurface, use widget allocation size, not gdk_surface size.
   * The gdk_surface is the parent dialog window (includes decorations),
   * but the subsurface is sized to match the canvas widget. */
#ifdef GDK_WINDOWING_WAYLAND
  if (gldata->subsurface)
  {
    if (ih->handle && GTK_IS_WIDGET(ih->handle))
    {
      int w = gtk_widget_get_width((GtkWidget*)ih->handle);
      int h = gtk_widget_get_height((GtkWidget*)ih->handle);
      if (w > 0 && h > 0)
      {
        realized_w = w;
        realized_h = h;
      }
    }
  }
  else
#endif
  {
    if (gldata->gdk_surface && GDK_IS_SURFACE(gldata->gdk_surface))
    {
      realized_w = gdk_surface_get_width(gldata->gdk_surface);
      realized_h = gdk_surface_get_height(gldata->gdk_surface);
    }

    if (realized_w <= 0 || realized_h <= 0)
    {
      if (ih->handle && GTK_IS_WIDGET(ih->handle))
      {
        int w = gtk_widget_get_width((GtkWidget*)ih->handle);
        int h = gtk_widget_get_height((GtkWidget*)ih->handle);
        if (w > 0 && h > 0)
        {
          realized_w = w;
          realized_h = h;
        }
      }
    }
  }
#endif /* IUP_EGL_USE_GTK4 */

#ifdef IUP_EGL_USE_QT
  /* For Qt: get HiDPI scale and widget size */
  if (ih->handle)
  {
    /* Get size from the canvas widget (ih->handle) */
    iupQtGetWidgetSize(ih->handle, &realized_w, &realized_h);

    /* Get scale factor from top-level window */
    if (gldata->qwindow)
    {
      double dpr = iupQtGetDevicePixelRatio(gldata->qwindow);
      scale = (int)dpr;
      if (scale < 1) scale = 1;
    }
  }
#endif /* IUP_EGL_USE_QT */

#ifdef IUP_EGL_USE_EFL
  /* For EFL: get widget size from Evas object */
  if (gldata->evas_obj)
  {
    Eina_Rect geom = efl_gfx_entity_geometry_get(gldata->evas_obj);
    if (geom.w > 0 && geom.h > 0)
    {
      realized_w = geom.w;
      realized_h = geom.h;
    }
  }

  /* Get scale factor - only for X11 (EFL Wayland doesn't expose output scale) */
#ifdef HAVE_ECORE_X
  if (gldata->ee && eGLCanvasEflIsX11(gldata->ee))
  {
    int dpi_x = 0, dpi_y = 0;
    ecore_evas_screen_dpi_get(gldata->ee, &dpi_x, &dpi_y);
    if (dpi_x > 0)
    {
      scale = dpi_x / 96;
      if (scale < 1) scale = 1;
    }
  }
#endif
  /* For Wayland, leave scale=1 - EFL doesn't expose output scale and compositor handles HiDPI */
#endif /* IUP_EGL_USE_EFL */

  if (ih->currentwidth > 0 && ih->currentheight > 0)
  {
    requested_w = ih->currentwidth;
    requested_h = ih->currentheight;
  }

  if (requested_w <= 0 || requested_h <= 0)
  {
      if (ih->userwidth > 0 && ih->userheight > 0)
      {
          requested_w = ih->userwidth;
          requested_h = ih->userheight;
      }
  }

  if (requested_w <= 0 || requested_h <= 0)
  {
      requested_w = gldata->last_logical_width;
      requested_h = gldata->last_logical_height;
  }

  if (realized_w > 0 && realized_h > 0)
  {
      if (realized_w < 16 && realized_h < 16 && (requested_w > realized_w && requested_h > realized_h))
      {
          logical_w = requested_w;
          logical_h = requested_h;
      }
      else
      {
          logical_w = realized_w;
          logical_h = realized_h;
      }
  }
  else
  {
      logical_w = requested_w;
      logical_h = requested_h;
  }

  if (logical_w <= 0) logical_w = 100;
  if (logical_h <= 0) logical_h = 100;

  gldata->last_logical_width = logical_w;
  gldata->last_logical_height = logical_h;

  *physical_width = logical_w * scale;
  *physical_height = logical_h * scale;

  if (*physical_width < 1) *physical_width = 1;
  if (*physical_height < 1) *physical_height = 1;
}

#if defined(GDK_WINDOWING_WAYLAND) || defined(IUP_EGL_USE_QT) || (defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_WL2))
/* Forward declaration for Wayland compositor registry function */
static struct wl_subcompositor* eGLCanvasGetWaylandSubcompositor(struct wl_display* wl_display,
                                                                   struct wl_event_queue** out_queue,
                                                                   struct wl_compositor** out_compositor,
                                                                   struct wl_registry** out_registry);
#endif

static int eGLCanvasDefaultResize(Ihandle *ih, int width, int height)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  int physical_width, physical_height;
  int scale = 1;

  if (!gldata)
    return IUP_DEFAULT;

  gldata->last_logical_width = width;
  gldata->last_logical_height = height;

#if defined(IUP_EGL_USE_GTK3)
  /* Determine the scale factor for HiDPI support (GTK 3.10+) */
#if GTK_CHECK_VERSION(3, 10, 0)
  if (gldata->window && GDK_IS_WINDOW(gldata->window))
  {
    scale = gdk_window_get_scale_factor(gldata->window);
  }
  else if (ih->handle && GTK_IS_WIDGET(ih->handle))
  {
    /* Fallback if window is not yet associated or available */
    scale = gtk_widget_get_scale_factor((GtkWidget*)ih->handle);
  }
#endif
  if (scale < 1) scale = 1;
#endif /* IUP_EGL_USE_GTK3 */

#if defined(IUP_EGL_USE_GTK4)
  /* Determine the scale factor for HiDPI support */
  if (gldata->gdk_surface && GDK_IS_SURFACE(gldata->gdk_surface))
  {
    scale = gdk_surface_get_scale_factor(gldata->gdk_surface);
  }
  else if (ih->handle && GTK_IS_WIDGET(ih->handle))
  {
    /* Fallback if surface is not yet associated or available */
    scale = gtk_widget_get_scale_factor((GtkWidget*)ih->handle);
  }
  if (scale < 1) scale = 1;
#endif /* IUP_EGL_USE_GTK4 */

#ifdef IUP_EGL_USE_QT
  /* For Qt: get HiDPI scale factor */
  if (gldata->qwindow)
  {
    double dpr = iupQtGetDevicePixelRatio(gldata->qwindow);
    scale = (int)dpr;
    if (scale < 1) scale = 1;
  }
#endif /* IUP_EGL_USE_QT */

#ifdef IUP_EGL_USE_EFL
  /* For EFL: get HiDPI scale factor - only for X11 (EFL Wayland doesn't expose output scale) */
#ifdef HAVE_ECORE_X
  if (gldata->ee && eGLCanvasEflIsX11(gldata->ee))
  {
    int dpi_x = 0, dpi_y = 0;
    ecore_evas_screen_dpi_get(gldata->ee, &dpi_x, &dpi_y);
    if (dpi_x > 0)
    {
      scale = dpi_x / 96;
      if (scale < 1) scale = 1;
    }
  }
#endif
  /* For Wayland, leave scale=1 - EFL doesn't expose output scale and compositor handles HiDPI */
#endif /* IUP_EGL_USE_EFL */

  physical_width = width * scale;
  physical_height = height * scale;

/* Wayland EGL window resize (GTK3/GTK4/Qt6/EFL) */
#if defined(GDK_WINDOWING_WAYLAND) || (defined(IUP_EGL_USE_QT) && !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)) || (defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_WL2))
  if (gldata->egl_window) {
      int resize_w = physical_width < 1 ? 1 : physical_width;
      int resize_h = physical_height < 1 ? 1 : physical_height;

      wl_egl_window_resize(gldata->egl_window, resize_w, resize_h, 0, 0);

      gldata->egl_window_physical_width = resize_w;
      gldata->egl_window_physical_height = resize_h;

      /* Update subsurface position (GTK3/GTK4/Qt6) */
#if defined(IUP_EGL_USE_GTK3) && defined(GDK_WINDOWING_WAYLAND)
      if (gldata->subsurface && ih->handle && GTK_IS_WIDGET(ih->handle)) {
          GtkWidget* widget = GTK_WIDGET(ih->handle);
          GtkWidget* parent = gtk_widget_get_parent(widget);

          if (parent && gtk_widget_get_realized(widget)) {
              /* Translate widget coordinates to toplevel window coordinates */
              int subsurface_x = 0, subsurface_y = 0;
              gtk_widget_translate_coordinates(widget, gtk_widget_get_toplevel(widget),
                                                0, 0, &subsurface_x, &subsurface_y);

              wl_subsurface_set_position(gldata->subsurface, subsurface_x, subsurface_y);
          }
      }
#elif defined(IUP_EGL_USE_GTK4) && defined(GDK_WINDOWING_WAYLAND)
      if (gldata->subsurface && ih->handle && GTK_IS_WIDGET(ih->handle)) {
          GtkWidget* widget = (GtkWidget*)ih->handle;
          GtkWidget* parent = gtk_widget_get_parent(widget);
          graphene_rect_t bounds;
          int subsurface_x = 0, subsurface_y = 0;

          if (parent && gtk_widget_compute_bounds(widget, parent, &bounds)) {
              subsurface_x = (int)bounds.origin.x;
              subsurface_y = (int)bounds.origin.y;
          }

          Ihandle* dialog = IupGetDialog(ih);

          /* GTK4: Use bounds relative to parent container.
           * The subsurface is parented to the window's wl_surface.
           *
           * With Wayland + CSD, the wl_surface includes shadow borders, but GTK's
           * coordinates are relative to the Fixed container which is positioned accounting
           * for these shadows via xdg_surface_set_window_geometry(). */

          /* Check if window has CSD with invisible borders (not solid-csd) */
          GtkWidget* root = GTK_WIDGET(gtk_widget_get_root((GtkWidget*)ih->handle));
          gboolean has_csd = gtk_widget_has_css_class(root, "csd");
          gboolean has_solid_csd = gtk_widget_has_css_class(root, "solid-csd");

          if (has_csd && !has_solid_csd) {
              /* With CSD shadows, we can't query the exact shadow sizes, but we know:
               * - The wl_surface includes shadow borders
               * - The content area is offset within the surface
               * - Shadows may not be symmetric (bottom often larger than top)
               *
               * Calculate approximate shadow sizes from surface vs widget dimensions.
               * For Y: Don't assume symmetry - the bottom shadow is typically larger. */
              GdkSurface* gdk_surf = gtk_native_get_surface(GTK_NATIVE(root));
              if (gdk_surf) {
                  int surf_width = gdk_surface_get_width(gdk_surf);
                  int surf_height = gdk_surface_get_height(gdk_surf);
                  int root_width = gtk_widget_get_width(root);
                  int root_height = gtk_widget_get_height(root);

                  /* Assume left/right shadows are equal */
                  int shadow_left = (surf_width - root_width) / 2;

                  /* Calculate shadow_top: We know total_vertical shadow but not the split. */
                  int total_vertical = surf_height - root_height;

                  int caption_height = 37;  /* Default for GTK4 CSD titlebar */
                  if (dialog) {
                      int border, caption, menu;
                      iupdrvDialogGetDecoration(dialog, &border, &caption, &menu);
                      if (caption > 0) {
                          caption_height = caption;  /* Use actual value if available */
                      }
                  }

                  double surface_x, surface_y;
                  gtk_native_get_surface_transform(GTK_NATIVE(root), &surface_x, &surface_y);

                  subsurface_x += (int)surface_x;
                  subsurface_y += (int)surface_y + caption_height;
              }
          }

          wl_subsurface_set_position(gldata->subsurface, subsurface_x, subsurface_y);
      }
#elif defined(IUP_EGL_USE_QT) && !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)
      if (gldata->subsurface && ih->handle) {
          int x = 0, y = 0;
          iupQtGetWidgetPosition(ih->handle, &x, &y);

          /* Qt6 Wayland: Account for CSD shadow borders (like GTK4).
           * Get frame margins and add them to translate from content coords to surface coords. */
          void* toplevel_qwindow = iupQtGetTopLevelWindow(ih->handle);
          if (toplevel_qwindow) {
              int margin_left = 0, margin_top = 0, margin_right = 0, margin_bottom = 0;
              iupQtGetWindowFrameMargins(toplevel_qwindow, &margin_left, &margin_top, &margin_right, &margin_bottom);

              /* Only adjust if we have non-zero frame margins (CSD with shadows) */
              if (margin_left > 0 || margin_top > 0) {
                  x += margin_left;
                  y += margin_top;
              }
          }

          wl_subsurface_set_position(gldata->subsurface, x, y);
      }
#elif defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_WL2)
      if (gldata->subsurface && gldata->evas_obj) {
          Eina_Rect geom = efl_gfx_entity_geometry_get(gldata->evas_obj);
          int subsurface_x = geom.x;
          int subsurface_y = geom.y;

          /* Use framespace for position - this is where EFL renders content */
          Evas* evas = evas_object_evas_get(gldata->evas_obj);
          if (evas) {
              int fx = 0, fy = 0;
              evas_output_framespace_get(evas, &fx, &fy, NULL, NULL);
              subsurface_x += fx;
              subsurface_y += fy;
          }

          wl_subsurface_set_position(gldata->subsurface, subsurface_x, subsurface_y);
      }
#endif
  }
#endif
  /* On X11/EGL, the surface typically tracks the window size automatically, but we still need the correct viewport. */
  if (gldata->context != EGL_NO_CONTEXT)
  {
    IupGLMakeCurrent(ih);
    glViewport(0, 0, physical_width, physical_height);
  }

  /* NOTE: We do NOT commit the parent surface here. Qt/GTK will handle committing
   * naturally as part of their rendering pipeline, after they attach new buffers.
   * The subsurface position changes we made will be applied when Qt/GTK commit. */

  return IUP_DEFAULT;
}

static int eGLCanvasCreateMethod(Ihandle* ih, void** params)
{
  IGlControlData* gldata;
  (void)params;

  gldata = (IGlControlData*)malloc(sizeof(IGlControlData));
  memset(gldata, 0, sizeof(IGlControlData));

  gldata->display = EGL_NO_DISPLAY;
  gldata->surface = EGL_NO_SURFACE;
  gldata->context = EGL_NO_CONTEXT;

  gldata->last_logical_width = 100;
  gldata->last_logical_height = 100;

/* Initialize Wayland EGL window fields (GTK3/GTK4/Qt6) */
#if defined(GDK_WINDOWING_WAYLAND) || defined(IUP_EGL_USE_QT)
  gldata->egl_window = NULL;
  gldata->egl_window_physical_width = 0;
  gldata->egl_window_physical_height = 0;
#endif

/* Initialize Wayland subsurface fields (GTK4/Qt6) */
#if (defined(IUP_EGL_USE_GTK4) && defined(GDK_WINDOWING_WAYLAND)) || (defined(IUP_EGL_USE_QT) && !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES))
  gldata->subsurface_wl = NULL;
  gldata->subsurface = NULL;
  gldata->compositor = NULL;
  gldata->subcompositor = NULL;
  gldata->event_queue = NULL;
#endif

  iupAttribSet(ih, "_IUP_GLCONTROLDATA", (char*)gldata);

  IupSetCallback(ih, "RESIZE_CB", (Icallback)eGLCanvasDefaultResize);

  return IUP_NOERROR;
}

/*
 * Accept an optional visual_id (from X11) to filter configs.
 * visual_id should be 0 if not relevant (e.g. GTK3, Wayland, Qt).
 */
static int eGLCanvasChooseConfig(Ihandle* ih, IGlControlData* gldata, int visual_id)
{
  int n = 0;
  EGLint alist[40];
  int number;
  EGLint num_config;

  if (gldata->display == EGL_NO_DISPLAY)
  {
    iupAttribSet(ih, "ERROR", "EGL display not initialized.");
    return 0;
  }

  alist[n++] = EGL_SURFACE_TYPE;
  alist[n++] = EGL_WINDOW_BIT;

  alist[n++] = EGL_RENDERABLE_TYPE;
  alist[n++] = EGL_OPENGL_BIT;


  if (iupStrEqualNoCase(iupAttribGetStr(ih,"COLOR"), "INDEX"))
  {
    /* EGL generally doesn't support INDEX mode for OpenGL. Proceed with RGBA. */
  }

  alist[n++] = EGL_COLOR_BUFFER_TYPE;
  alist[n++] = EGL_RGB_BUFFER;

  int red_size = 8, green_size = 8, blue_size = 8, alpha_size = 0;

  number = iupAttribGetInt(ih,"RED_SIZE");
  if (number > 0) red_size = number;

  number = iupAttribGetInt(ih,"GREEN_SIZE");
  if (number > 0) green_size = number;

  number = iupAttribGetInt(ih,"BLUE_SIZE");
  if (number > 0) blue_size = number;

  number = iupAttribGetInt(ih,"ALPHA_SIZE");
  if (number > 0) alpha_size = number;

  alist[n++] = EGL_RED_SIZE;
  alist[n++] = red_size;
  alist[n++] = EGL_GREEN_SIZE;
  alist[n++] = green_size;
  alist[n++] = EGL_BLUE_SIZE;
  alist[n++] = blue_size;

  if (alpha_size > 0)
  {
    alist[n++] = EGL_ALPHA_SIZE;
    alist[n++] = alpha_size;
  }

  number = iupAttribGetInt(ih,"DEPTH_SIZE");
  alist[n++] = EGL_DEPTH_SIZE;
  alist[n++] = (number > 0) ? number : 16;


  number = iupAttribGetInt(ih,"STENCIL_SIZE");
  if (number > 0)
  {
    alist[n++] = EGL_STENCIL_SIZE;
    alist[n++] = number;
  }

  if (iupAttribGetBoolean(ih,"STEREO"))
  {
    /* Stereo support is rare/non-standard in EGL. */
    iupAttribSet(ih, "STEREO", "NO");
  }

  /* TERMINATOR */
  alist[n++] = EGL_NONE;

  if (!eglChooseConfig(gldata->display, alist, &gldata->config, 1, &num_config) || num_config == 0)
  {
    EGLint fallback_alist[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };

    if (!eglChooseConfig(gldata->display, fallback_alist, &gldata->config, 1, &num_config) || num_config == 0)
    {
        iupAttribSetStrf(ih, "ERROR", "No appropriate EGL config found (including fallback). Error: 0x%X", eglGetError());
        return 0;
    }
  }

  /*
   * Special handling for GTK4 on X11:
   * If we detected a specific X11 Visual ID (32-bit ARGB), we must find an EGL config
   * that matches this visual exactly. Otherwise, the background may be transparent.
   */
  if (visual_id != 0)
  {
      EGLConfig* configs = NULL;
      EGLint total_configs = 0;

      /* Get ALL configs that match the base criteria */
      if (eglChooseConfig(gldata->display, alist, NULL, 0, &total_configs) && total_configs > 0)
      {
          configs = (EGLConfig*)malloc(total_configs * sizeof(EGLConfig));
          if (eglChooseConfig(gldata->display, alist, configs, total_configs, &num_config))
          {
              int found = 0;
              for (int i = 0; i < num_config; i++)
              {
                  EGLint native_vid = 0;
                  /* Check if this EGL config is compatible with our X11 Window's visual */
                  if (eglGetConfigAttrib(gldata->display, configs[i], EGL_NATIVE_VISUAL_ID, &native_vid))
                  {
                      if (native_vid == visual_id)
                      {
                          gldata->config = configs[i];
                          found = 1;
                          break;
                      }
                  }
              }
              /* If no exact visual match found, we stick with the default config selected above. */
          }
          free(configs);
      }
  }

  iupAttribSet(ih, "ERROR", NULL);
  return 1;
}

static char* eGLCanvasGetVisualAttrib(Ihandle *ih)
{
  (void)ih;
#if defined(IUP_EGL_USE_GTK3)
  GdkScreen* screen = gdk_screen_get_default();
  if (screen) {
      GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
      if (!visual)
          visual = gdk_screen_get_system_visual(screen);
      return (char*)visual;
  }
#endif /* IUP_EGL_USE_GTK3 */
#if defined(IUP_EGL_USE_GTK4)
  GdkDisplay* display = gdk_display_get_default();
  if (display) {
      /* GTK4 doesn't have GdkScreen */
      return (char*)display;
  }
#endif /* IUP_EGL_USE_GTK4 */
#ifdef IUP_EGL_USE_QT
  /* Qt doesn't expose visuals the same way */
#endif
  return NULL;
}

#if defined(GDK_WINDOWING_WAYLAND) || defined(IUP_EGL_USE_QT) || (defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_WL2))
/* Helper structure and function to retrieve wl_subcompositor from Wayland registry. */
struct wayland_registry_data {
  struct wl_compositor* compositor;
  struct wl_subcompositor* subcompositor;
};

static void wayland_registry_handle_global(void* data, struct wl_registry* registry,
                                           uint32_t name, const char* interface, uint32_t version)
{
  struct wayland_registry_data* reg_data = (struct wayland_registry_data*)data;

  if (strcmp(interface, "wl_compositor") == 0) {
    reg_data->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
  }
  else if (strcmp(interface, "wl_subcompositor") == 0) {
    reg_data->subcompositor = wl_registry_bind(registry, name, &wl_subcompositor_interface, 1);
  }
}

static void wayland_registry_handle_global_remove(void* data, struct wl_registry* registry, uint32_t name)
{
  /* We don't need to handle removals */
  (void)data;
  (void)registry;
  (void)name;
}

static const struct wl_registry_listener wayland_registry_listener = {
  wayland_registry_handle_global,
  wayland_registry_handle_global_remove
};

/* Retrieve wl_compositor and wl_subcompositor from the Wayland registry. */
static struct wl_subcompositor* eGLCanvasGetWaylandSubcompositor(struct wl_display* wl_display,
                                                                   struct wl_event_queue** out_queue,
                                                                   struct wl_compositor** out_compositor,
                                                                   struct wl_registry** out_registry)
{
  struct wayland_registry_data reg_data = { NULL };
  struct wl_registry* registry;
  struct wl_event_queue* queue = NULL;

  if (!wl_display) {
    return NULL;
  }

  queue = wl_display_create_queue(wl_display);
  if (!queue) {
    return NULL;
  }

  /* Get the Wayland registry */
  registry = wl_display_get_registry(wl_display);
  if (!registry) {
    wl_event_queue_destroy(queue);
    return NULL;
  }

  /* Assign our queue to the registry proxy so events go to our queue, not the default queue */
  wl_proxy_set_queue((struct wl_proxy*)registry, queue);

  /* Add listener to process registry events */
  wl_registry_add_listener(registry, &wayland_registry_listener, &reg_data);

  /* Roundtrip on our private queue to process registry events
   * This doesn't interfere with Qt/GTK's main event queue */
  if (wl_display_roundtrip_queue(wl_display, queue) < 0) {
    wl_registry_destroy(registry);
    wl_event_queue_destroy(queue);
    return NULL;
  }

  /*
   * IMPORTANT: DO NOT destroy the queue here!
   * The compositor and subcompositor proxies are attached to this queue, and destroying it
   * would make the proxies invalid. The caller must keep the queue alive for as long as compositor/subcompositor are used.
   */
  if (out_queue) {
    *out_queue = queue;
  }

  if (out_compositor) {
    *out_compositor = reg_data.compositor;
  }

  if (out_registry) {
    *out_registry = registry;
  }

  /*
   * Return the registry to the caller for cleanup.
   * The registry must be kept alive while compositor/subcompositor are used,
   * and destroyed AFTER compositor/subcompositor are destroyed.
   */

  return reg_data.subcompositor;
}
#endif /* GDK_WINDOWING_WAYLAND || IUP_EGL_USE_QT */

#if defined(IUP_EGL_USE_QT) && !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)
/* Qt6 Wayland: Create subsurface lazily when first needed (on GLMakeCurrent). */
static EGLNativeWindowType eGLCanvasCreateQt6WaylandSubsurface(Ihandle* ih, IGlControlData* gldata)
{
  EGLNativeWindowType native_window = (EGLNativeWindowType)NULL;

  struct wl_display* wl_display = iupQtGetWaylandDisplay();

  /* Get top-level window */
  void* toplevel_qwindow = iupQtGetTopLevelWindow(ih->handle);
  if (!toplevel_qwindow) {
      /* Window not ready yet - will retry on next GLMakeCurrent */
      return native_window;
  }

  /* Get Wayland surface if ready (non-blocking). On Wayland, the surface is created asynchronously.
     If not ready yet, return NULL and let the caller retry on next GLMakeCurrent. */
  struct wl_surface* parent_surface = (struct wl_surface*)iupQtGetWaylandSurfaceIfReady(toplevel_qwindow);

  if (!wl_display || !parent_surface) {
      /* Surface not ready yet, will retry on next GLMakeCurrent */
      return native_window;
  }

  gldata->parent_surface = parent_surface;
  gldata->qwindow = toplevel_qwindow;

  gldata->compositor = iupQtGetWaylandCompositor();

  if (gldata->compositor) {
      if (!gldata->subcompositor) {
          gldata->subcompositor = eGLCanvasGetWaylandSubcompositor(wl_display, &gldata->event_queue, &gldata->registry_compositor, &gldata->registry);
      }
  }

  if (gldata->compositor) {
      /* Create a new Wayland surface for the canvas subsurface */
      gldata->subsurface_wl = wl_compositor_create_surface(gldata->compositor);

      if (gldata->subsurface_wl && gldata->subcompositor) {
          /* Create subsurface attached to parent window's surface */
          gldata->subsurface = wl_subcompositor_get_subsurface(
              gldata->subcompositor,
              gldata->subsurface_wl,
              parent_surface);

          if (gldata->subsurface) {
              /* Set desynchronized mode for independent rendering */
              wl_subsurface_set_desync(gldata->subsurface);

              /* Position the subsurface within the parent window */
              int x = 0, y = 0;
              iupQtGetWidgetPosition(ih->handle, &x, &y);

              /* Account for CSD shadow borders */
              int margin_left = 0, margin_top = 0, margin_right = 0, margin_bottom = 0;
              iupQtGetWindowFrameMargins(toplevel_qwindow, &margin_left, &margin_top, &margin_right, &margin_bottom);

              if (margin_left > 0 || margin_top > 0) {
                  x += margin_left;
                  y += margin_top;
              }

              wl_subsurface_set_position(gldata->subsurface, x, y);

              /* Commit parent surface on initial subsurface creation.
                 Per Wayland protocol, subsurface position changes are buffered and require parent surface commit to take effect. */
              if (gldata->parent_surface) {
                  wl_surface_commit(gldata->parent_surface);
              }

              /* Create the EGL window using the subsurface */
              int physical_width = 0, physical_height = 0;
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
#endif /* IUP_EGL_USE_QT && !IUP_QT_DISABLE_WAYLAND_INCLUDES */

#if defined(IUP_EGL_USE_GTK3) && defined(GDK_WINDOWING_WAYLAND)
/* GTK3 Wayland: Create subsurface lazily when first needed (on GLMakeCurrent). */
static EGLNativeWindowType eGLCanvasCreateGtk3WaylandSubsurface(Ihandle* ih, IGlControlData* gldata)
{
  EGLNativeWindowType native_window = (EGLNativeWindowType)NULL;

  if (!GDK_IS_WAYLAND_WINDOW(gldata->window)) {
    return native_window;
  }

  /* In GTK3, we need the TOPLEVEL window's wl_surface for the subsurface parent.
   * The canvas has its own GdkWindow, but on Wayland, subsurfaces must be parented to the
   * toplevel window's wl_surface, not to the canvas's own surface. */
  GdkWindow* toplevel_window = gdk_window_get_toplevel(gldata->window);

  if (!toplevel_window || !GDK_IS_WAYLAND_WINDOW(toplevel_window)) {
    return native_window;
  }

  struct wl_surface* parent_surface = gdk_wayland_window_get_wl_surface(toplevel_window);

  if (!parent_surface) {
    return native_window;
  }

  gldata->parent_surface = parent_surface;

  GdkDisplay* gdk_display = gdk_window_get_display(gldata->window);
  struct wl_display* wl_display = gdk_wayland_display_get_wl_display(gdk_display);

  if (!wl_display) {
    return native_window;
  }

  gldata->compositor = gdk_wayland_display_get_wl_compositor(gdk_display);

  if (!gldata->compositor) {
    return native_window;
  }

  /* Create a new Wayland surface for the canvas subsurface */
  gldata->subsurface_wl = wl_compositor_create_surface(gldata->compositor);

  if (!gldata->subsurface_wl) {
    return native_window;
  }

  /* Get wl_subcompositor from the Wayland registry */
  if (!gldata->subcompositor) {
    gldata->subcompositor = eGLCanvasGetWaylandSubcompositor(wl_display,
                                                               &gldata->event_queue,
                                                               &gldata->registry_compositor,
                                                               &gldata->registry);
  }

  if (!gldata->subcompositor) {
    wl_surface_destroy(gldata->subsurface_wl);
    gldata->subsurface_wl = NULL;
    return native_window;
  }

  /* Create subsurface attached to parent window's surface */
  gldata->subsurface = wl_subcompositor_get_subsurface(
      gldata->subcompositor,
      gldata->subsurface_wl,
      parent_surface);

  if (!gldata->subsurface) {
    wl_surface_destroy(gldata->subsurface_wl);
    gldata->subsurface_wl = NULL;
    return native_window;
  }

  /* Set subsurface to desynchronized mode for independent rendering */
  wl_subsurface_set_desync(gldata->subsurface);

  /* Position the subsurface within the parent window.
   * Since canvas has has_window=FALSE on Wayland, we need to calculate position from widget allocation. */
  int subsurface_x = 0, subsurface_y = 0;

  GtkWidget* widget = GTK_WIDGET(ih->handle);
  if (widget && gtk_widget_get_realized(widget)) {
    /* Get widget allocation to find position */
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    /* The allocation x,y are relative to the parent container.
     * We need to translate this to toplevel window coordinates. */
    GtkWidget* parent = gtk_widget_get_parent(widget);
    if (parent) {
      gtk_widget_translate_coordinates(widget, gtk_widget_get_toplevel(widget),
                                        0, 0, &subsurface_x, &subsurface_y);

      /* If position is invalid (-1, -1), default to (0, 0) and rely on resize to fix it */
      if (subsurface_x < 0 || subsurface_y < 0) {
        subsurface_x = 0;
        subsurface_y = 0;
      }
    }
  }

  wl_subsurface_set_position(gldata->subsurface, subsurface_x, subsurface_y);

  /* Commit parent surface on initial subsurface creation.
   * Per Wayland protocol, subsurface position changes are buffered and require parent surface commit to take effect.
   * After this, GTK will handle parent commits during rendering. */
  if (gldata->parent_surface) {
      wl_surface_commit(gldata->parent_surface);
  }

  /* Create the EGL window using the subsurface */
  int physical_width = 0, physical_height = 0;
  eGLCanvasGetActualSize(ih, gldata, &physical_width, &physical_height);

  gldata->egl_window = wl_egl_window_create(gldata->subsurface_wl, physical_width, physical_height);

  if (gldata->egl_window) {
    native_window = (EGLNativeWindowType)gldata->egl_window;
    gldata->egl_window_physical_width = physical_width;
    gldata->egl_window_physical_height = physical_height;
  }

  return native_window;
}
#endif /* IUP_EGL_USE_GTK3 && GDK_WINDOWING_WAYLAND */

#if defined(IUP_EGL_USE_GTK4) && defined(GDK_WINDOWING_WAYLAND)
/* GTK4 Wayland: Create subsurface lazily when first needed (on GLMakeCurrent). */
static EGLNativeWindowType eGLCanvasCreateGtk4WaylandSubsurface(Ihandle* ih, IGlControlData* gldata)
{
  EGLNativeWindowType native_window = (EGLNativeWindowType)NULL;

  if (!GDK_IS_WAYLAND_SURFACE(gldata->gdk_surface)) {
    return native_window;
  }

  struct wl_surface* parent_surface = gdk_wayland_surface_get_wl_surface(gldata->gdk_surface);

  if (!parent_surface) {
    return native_window;
  }

  gldata->parent_surface = parent_surface;

  GdkDisplay* gdk_display = gdk_surface_get_display(gldata->gdk_surface);
  struct wl_display* wl_display = gdk_wayland_display_get_wl_display(gdk_display);

  if (!wl_display) {
    return native_window;
  }

  gldata->compositor = gdk_wayland_display_get_wl_compositor(gdk_display);

  if (!gldata->compositor) {
    return native_window;
  }

  /* Create a new Wayland surface for the canvas subsurface */
  gldata->subsurface_wl = wl_compositor_create_surface(gldata->compositor);

  if (!gldata->subsurface_wl) {
    return native_window;
  }

  /* Get wl_subcompositor from the Wayland registry */
  if (!gldata->subcompositor) {
    gldata->subcompositor = eGLCanvasGetWaylandSubcompositor(wl_display,
                                                               &gldata->event_queue,
                                                               &gldata->registry_compositor,
                                                               &gldata->registry);
  }

  if (!gldata->subcompositor) {
    wl_surface_destroy(gldata->subsurface_wl);
    gldata->subsurface_wl = NULL;
    return native_window;
  }

  /* Create subsurface attached to parent window's surface */
  gldata->subsurface = wl_subcompositor_get_subsurface(
      gldata->subcompositor,
      gldata->subsurface_wl,
      parent_surface);

  if (!gldata->subsurface) {
    wl_surface_destroy(gldata->subsurface_wl);
    gldata->subsurface_wl = NULL;
    return native_window;
  }

  /* Set subsurface to desynchronized mode for independent rendering */
  wl_subsurface_set_desync(gldata->subsurface);

  /* Position the subsurface within the parent window */
  GtkWidget* widget = GTK_WIDGET(ih->handle);
  if (widget) {
    GtkWidget* parent = gtk_widget_get_parent(widget);
    graphene_rect_t bounds;
    int subsurface_x = 0, subsurface_y = 0;

    if (parent && gtk_widget_compute_bounds(widget, parent, &bounds)) {
      subsurface_x = (int)bounds.origin.x;
      subsurface_y = (int)bounds.origin.y;
    }

    /* Check if window has CSD with invisible borders */
    GtkWidget* root = GTK_WIDGET(gtk_widget_get_root(widget));
    gboolean has_csd = gtk_widget_has_css_class(root, "csd");
    gboolean has_solid_csd = gtk_widget_has_css_class(root, "solid-csd");

    if (has_csd && !has_solid_csd) {
      /* Calculate shadow offsets */
      GdkSurface* gdk_surf = gtk_native_get_surface(GTK_NATIVE(root));
      if (gdk_surf) {
        Ihandle* dialog = IupGetDialog(ih);
        int caption_height = 37;  /* Default for GTK4 CSD titlebar */
        if (dialog) {
          int border, caption, menu;
          iupdrvDialogGetDecoration(dialog, &border, &caption, &menu);
          if (caption > 0) {
            caption_height = caption;
          }
        }

        double surface_x, surface_y;
        gtk_native_get_surface_transform(GTK_NATIVE(root), &surface_x, &surface_y);

        subsurface_x += (int)surface_x;
        subsurface_y += (int)surface_y + caption_height;
      }
    }

    wl_subsurface_set_position(gldata->subsurface, subsurface_x, subsurface_y);

    /* Commit parent surface on initial subsurface creation.
     * Per Wayland protocol, subsurface position changes are buffered and require parent surface commit to take effect.
     * After this, GTK will handle parent commits during rendering. */
    if (gldata->parent_surface) {
        wl_surface_commit(gldata->parent_surface);
    }
  }

  /* Create the EGL window using the subsurface */
  int physical_width = 0, physical_height = 0;
  eGLCanvasGetActualSize(ih, gldata, &physical_width, &physical_height);

  gldata->egl_window = wl_egl_window_create(gldata->subsurface_wl, physical_width, physical_height);

  if (gldata->egl_window) {
    native_window = (EGLNativeWindowType)gldata->egl_window;
    gldata->egl_window_physical_width = physical_width;
    gldata->egl_window_physical_height = physical_height;
  }

  return native_window;
}
#endif /* IUP_EGL_USE_GTK4 && GDK_WINDOWING_WAYLAND */

#if defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_WL2)
/* EFL Wayland: Create subsurface lazily when first needed (on GLMakeCurrent). */
static EGLNativeWindowType eGLCanvasCreateEflWaylandSubsurface(Ihandle* ih, IGlControlData* gldata)
{
  EGLNativeWindowType native_window = (EGLNativeWindowType)NULL;
  Ecore_Wl2_Display* wl2_display;
  Ecore_Wl2_Window* wl2_win;
  struct wl_display* wl_display;
  struct wl_surface* parent_surface;

  if (!eGLCanvasEflIsWayland(gldata->ee))
    return native_window;

  wl2_win = ecore_evas_wayland2_window_get(gldata->ee);
  if (!wl2_win)
    return native_window;

  wl2_display = ecore_wl2_window_display_get(wl2_win);
  if (!wl2_display)
    return native_window;

  wl_display = ecore_wl2_display_get(wl2_display);
  if (!wl_display)
    return native_window;

  parent_surface = ecore_wl2_window_surface_get(wl2_win);
  if (!parent_surface)
    return native_window;

  gldata->parent_surface = parent_surface;

  gldata->compositor = ecore_wl2_display_compositor_get(wl2_display);
  if (!gldata->compositor)
    return native_window;

  /* Create a new Wayland surface for the canvas subsurface */
  gldata->subsurface_wl = wl_compositor_create_surface(gldata->compositor);
  if (!gldata->subsurface_wl)
    return native_window;

  /* Get wl_subcompositor from the Wayland registry */
  if (!gldata->subcompositor) {
    gldata->subcompositor = eGLCanvasGetWaylandSubcompositor(wl_display, &gldata->event_queue, &gldata->registry_compositor, &gldata->registry);
  }

  if (!gldata->subcompositor) {
    wl_surface_destroy(gldata->subsurface_wl);
    gldata->subsurface_wl = NULL;
    return native_window;
  }

  /* Create subsurface attached to parent window's surface */
  gldata->subsurface = wl_subcompositor_get_subsurface(
      gldata->subcompositor,
      gldata->subsurface_wl,
      parent_surface);

  if (!gldata->subsurface) {
    wl_surface_destroy(gldata->subsurface_wl);
    gldata->subsurface_wl = NULL;
    return native_window;
  }

  /* Set subsurface to desynchronized mode for independent rendering */
  wl_subsurface_set_desync(gldata->subsurface);

  /* Position the subsurface within the parent window */
  int subsurface_x = 0, subsurface_y = 0;
  if (gldata->evas_obj)
  {
    Eina_Rect geom = efl_gfx_entity_geometry_get(gldata->evas_obj);
    subsurface_x = geom.x;
    subsurface_y = geom.y;

    /* Use framespace for position - this is where EFL renders content */
    Evas* evas = evas_object_evas_get(gldata->evas_obj);
    if (evas) {
        int fx = 0, fy = 0;
        evas_output_framespace_get(evas, &fx, &fy, NULL, NULL);
        subsurface_x += fx;
        subsurface_y += fy;
    }
  }

  wl_subsurface_set_position(gldata->subsurface, subsurface_x, subsurface_y);

  /* Create the EGL window using the subsurface.
   * eGLCanvasGetActualSize returns the content area size (already excludes decorations). */
  int physical_width = 0, physical_height = 0;
  eGLCanvasGetActualSize(ih, gldata, &physical_width, &physical_height);
  if (physical_width < 1) physical_width = 1;
  if (physical_height < 1) physical_height = 1;

  gldata->egl_window = wl_egl_window_create(gldata->subsurface_wl, physical_width, physical_height);

  if (gldata->egl_window) {
    native_window = (EGLNativeWindowType)gldata->egl_window;
    gldata->egl_window_physical_width = physical_width;
    gldata->egl_window_physical_height = physical_height;
  }

  return native_window;
}
#endif /* IUP_EGL_USE_EFL && HAVE_ECORE_WL2 */

static int eGLCanvasMapMethod(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  EGLContext shared_context = EGL_NO_CONTEXT;
  Ihandle* ih_shared;
  EGLNativeWindowType native_window = (EGLNativeWindowType)NULL;
  PFN_eglGetPlatformDisplay eglGetPlatformDisplay_func = NULL;
  int requested_major = 0, requested_minor = 0;
  char* requested_profile = NULL;
  int target_visual_id = 0;

#if defined(IUP_EGL_USE_GTK3)
  GdkDisplay* gdk_display;

  /* Try to get the GdkWindow from IUP's attributes first */
  gldata->window = (GdkWindow*)iupAttribGet(ih, "GDKWINDOW");

  /* If not found, try to get it from the GTK widget if available */
  if (!gldata->window && ih->handle && GTK_IS_WIDGET(ih->handle))
  {
    GtkWidget* widget = (GtkWidget*)ih->handle;

    if (!gtk_widget_get_realized(widget))
    {
      gtk_widget_realize(widget);
    }

    /* Get the GdkWindow from the widget */
    gldata->window = gtk_widget_get_window(widget);

    /* Store it back in IUP's attributes for future use */
    if (gldata->window)
    {
      iupAttribSet(ih, "GDKWINDOW", (char*)gldata->window);

#ifdef GDK_WINDOWING_WAYLAND
      GdkDisplay* display = gdk_window_get_display(gldata->window);
      if (display && GDK_IS_WAYLAND_DISPLAY(display))
      {
        GdkWindowType win_type = gdk_window_get_window_type(gldata->window);
        GdkWindow* parent = gdk_window_get_parent(gldata->window);
        GdkWindow* toplevel = gdk_window_get_toplevel(gldata->window);
      }
#endif
    }
  }

  if (!gldata->window)
  {
    /* Try getting it via IupGetAttribute which works after mapping */
    char* window_ptr = IupGetAttribute(ih, "GDKWINDOW");
    if (window_ptr)
    {
      gldata->window = (GdkWindow*)window_ptr;
    }
  }

  if (!gldata->window || !GDK_IS_WINDOW(gldata->window))
  {
    iupAttribSet(ih, "ERROR", "Could not obtain valid GdkWindow handle.");
    gldata->window = NULL;
    return IUP_NOERROR;
  }

  gdk_display = gdk_window_get_display(gldata->window);
#endif /* IUP_EGL_USE_GTK3 */

#if defined(IUP_EGL_USE_GTK4)
  GdkDisplay* gdk_display;

  /* Try to get the GdkSurface from IUP's attributes first */
  gldata->gdk_surface = (GdkSurface*)iupAttribGet(ih, "GDKSURFACE");

  /* If not found, try to get it from the GTK widget if available */
  if (!gldata->gdk_surface && ih->handle && GTK_IS_WIDGET(ih->handle))
  {
    GtkWidget* widget = (GtkWidget*)ih->handle;

    if (!gtk_widget_get_realized(widget))
    {
      gtk_widget_realize(widget);
    }

    /* Get the GdkSurface from the widget using GtkNative */
    GtkNative* native = gtk_widget_get_native(widget);
    if (native)
    {
      gldata->gdk_surface = gtk_native_get_surface(native);
    }

    /* Store it back in IUP's attributes for future use */
    if (gldata->gdk_surface)
    {
      iupAttribSet(ih, "GDKSURFACE", (char*)gldata->gdk_surface);
    }
  }

  if (!gldata->gdk_surface)
  {
    /* Try getting it via IupGetAttribute which works after mapping */
    char* surface_ptr = IupGetAttribute(ih, "GDKSURFACE");
    if (surface_ptr)
    {
      gldata->gdk_surface = (GdkSurface*)surface_ptr;
    }
  }

  if (!gldata->gdk_surface || !GDK_IS_SURFACE(gldata->gdk_surface))
  {
    iupAttribSet(ih, "ERROR", "Could not obtain valid GdkSurface handle.");
    gldata->gdk_surface = NULL;
    return IUP_NOERROR;
  }

  gdk_display = gdk_surface_get_display(gldata->gdk_surface);
#endif /* IUP_EGL_USE_GTK4 */

#ifdef IUP_EGL_USE_QT
  void* canvas_widget = iupqtCanvasGetContext(ih);

  if (!canvas_widget)
  {
    iupAttribSet(ih, "ERROR", "Could not obtain canvas widget context.");
    return IUP_NOERROR;
  }

  gldata->qwindow = (QWindow*)iupAttribGet(ih, "QWINDOW");

  if (!gldata->qwindow)
  {
    gldata->qwindow = (QWindow*)iupQtGetWindowFromWidget(canvas_widget);

    if (gldata->qwindow)
    {
      iupAttribSet(ih, "QWINDOW", (char*)gldata->qwindow);
    }
  }

#endif /* IUP_EGL_USE_QT */

#ifdef IUP_EGL_USE_EFL
  /* Get the Evas object and Ecore_Evas from the canvas */
  gldata->evas_obj = (Evas_Object*)ih->handle;

  if (!gldata->evas_obj)
  {
    iupAttribSet(ih, "ERROR", "Could not obtain Evas_Object handle.");
    return IUP_NOERROR;
  }

  {
    Evas* evas = evas_object_evas_get(gldata->evas_obj);
    if (evas)
      gldata->ee = ecore_evas_ecore_evas_get(evas);
  }

  if (!gldata->ee)
  {
    iupAttribSet(ih, "ERROR", "Could not obtain Ecore_Evas handle.");
    return IUP_NOERROR;
  }
#endif /* IUP_EGL_USE_EFL */

  /* Try to load eglGetPlatformDisplay (EGL 1.5 core) or eglGetPlatformDisplayEXT (extension) */
  eglGetPlatformDisplay_func = (PFN_eglGetPlatformDisplay)eglGetProcAddress("eglGetPlatformDisplay");
  if (!eglGetPlatformDisplay_func)
      eglGetPlatformDisplay_func = (PFN_eglGetPlatformDisplay)eglGetProcAddress("eglGetPlatformDisplayEXT");

#if defined(IUP_EGL_USE_GTK3)
#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY(gdk_display)) {
      struct wl_display* wl_display = gdk_wayland_display_get_wl_display(gdk_display);

      if (eglGetPlatformDisplay_func) {
          gldata->display = eglGetPlatformDisplay_func(EGL_PLATFORM_WAYLAND_KHR, wl_display, NULL);
      }

      if (gldata->display == EGL_NO_DISPLAY) {
          gldata->display = eglGetDisplay((EGLNativeDisplayType)wl_display);
      }
  }
  else
#endif
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY(gdk_display)) {
      Display* x_display = gdk_x11_display_get_xdisplay(gdk_display);

      if (eglGetPlatformDisplay_func) {
          gldata->display = eglGetPlatformDisplay_func(EGL_PLATFORM_X11_KHR, x_display, NULL);
      }

      if (gldata->display == EGL_NO_DISPLAY) {
         gldata->display = eglGetDisplay((EGLNativeDisplayType)x_display);
      }
  }
#endif
#endif /* IUP_EGL_USE_GTK3 */

#if defined(IUP_EGL_USE_GTK4)
#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY(gdk_display)) {
      struct wl_display* wl_display = gdk_wayland_display_get_wl_display(gdk_display);

      if (eglGetPlatformDisplay_func) {
          gldata->display = eglGetPlatformDisplay_func(EGL_PLATFORM_WAYLAND_KHR, wl_display, NULL);
      }

      if (gldata->display == EGL_NO_DISPLAY) {
          gldata->display = eglGetDisplay((EGLNativeDisplayType)wl_display);
      }
  }
  else
#endif
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY(gdk_display)) {
      Display* x_display = gdk_x11_display_get_xdisplay(gdk_display);

      if (eglGetPlatformDisplay_func) {
          gldata->display = eglGetPlatformDisplay_func(EGL_PLATFORM_X11_KHR, x_display, NULL);
      }

      if (gldata->display == EGL_NO_DISPLAY) {
         gldata->display = eglGetDisplay((EGLNativeDisplayType)x_display);
      }

      /*
       * FOR GTK4 X11 ONLY:
       * Retrieve the native Visual ID to force EGL to pick a compatible config.
       */
      if (GDK_IS_X11_SURFACE(gldata->gdk_surface)) {
          native_window = (EGLNativeWindowType)gdk_x11_surface_get_xid(gldata->gdk_surface);
          target_visual_id = iupEGLGetX11VisualID(x_display, (unsigned long)native_window);
      }
  }
#endif
#endif /* IUP_EGL_USE_GTK4 */

#ifdef IUP_EGL_USE_QT
  /* For Qt: Get native Display (X11 or Wayland) */
  const char* qt_platform = iupQtGetPlatformName();
  int has_wayland_support = iupQtHasWaylandSupport();

  if (has_wayland_support && strcmp(qt_platform, "wayland") == 0) {
      /* Qt6.7+ on Wayland - use Wayland APIs */
      #if !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)
      struct wl_display* wl_display = iupQtGetWaylandDisplay();
      if (wl_display) {
          if (eglGetPlatformDisplay_func) {
              gldata->display = eglGetPlatformDisplay_func(EGL_PLATFORM_WAYLAND_KHR, wl_display, NULL);
          }
          if (gldata->display == EGL_NO_DISPLAY) {
              gldata->display = eglGetDisplay((EGLNativeDisplayType)wl_display);
          }
      }
      #endif
  }
  else if (strcmp(qt_platform, "xcb") == 0 || !has_wayland_support) {
      /* X11 (including XWayland) or Qt5/Qt<6.7 fallback */
      if (eglGetPlatformDisplay_func) {
          /* Passing NULL as native_display lets EGL open its own X connection */
          gldata->display = eglGetPlatformDisplay_func(EGL_PLATFORM_X11_KHR, EGL_DEFAULT_DISPLAY, NULL);
      }

      /* For Qt X11, try to get XID early */
      if (gldata->qwindow) {
          unsigned long xid = iupQtGetNativeWindow(gldata->qwindow);
          if (xid != 0) {
              native_window = (EGLNativeWindowType)xid;
          }
      }
  }
#endif /* IUP_EGL_USE_QT */

#ifdef IUP_EGL_USE_EFL
  /* EFL: Get native Display (X11 or Wayland) */
#ifdef HAVE_ECORE_WL2
  if (eGLCanvasEflIsWayland(gldata->ee)) {
      Ecore_Wl2_Window* wl2_win = ecore_evas_wayland2_window_get(gldata->ee);
      if (wl2_win) {
          Ecore_Wl2_Display* wl2_display = ecore_wl2_window_display_get(wl2_win);
          if (wl2_display) {
              struct wl_display* wl_display = ecore_wl2_display_get(wl2_display);
              if (wl_display) {
                  if (eglGetPlatformDisplay_func) {
                      gldata->display = eglGetPlatformDisplay_func(EGL_PLATFORM_WAYLAND_KHR, wl_display, NULL);
                  }
                  if (gldata->display == EGL_NO_DISPLAY) {
                      gldata->display = eglGetDisplay((EGLNativeDisplayType)wl_display);
                  }
              }
          }
      }
  }
  else
#endif
#ifdef HAVE_ECORE_X
  if (eGLCanvasEflIsX11(gldata->ee)) {
      Display* x_display = ecore_x_display_get();
      if (x_display) {
          if (eglGetPlatformDisplay_func) {
              gldata->display = eglGetPlatformDisplay_func(EGL_PLATFORM_X11_KHR, x_display, NULL);
          }
          if (gldata->display == EGL_NO_DISPLAY) {
              gldata->display = eglGetDisplay((EGLNativeDisplayType)x_display);
          }

          /* Get X11 window handle for native window */
          Ecore_X_Window xwin = ecore_evas_window_get(gldata->ee);
          if (xwin)
              native_window = (EGLNativeWindowType)xwin;
      }
  }
#endif
#endif /* IUP_EGL_USE_EFL */

  if (gldata->display == EGL_NO_DISPLAY) {
      gldata->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  }

  if (gldata->display == EGL_NO_DISPLAY) {
      iupAttribSet(ih, "ERROR", "Could not get EGL display (Platform specific, native handle, and default failed).");
      return IUP_NOERROR;
  }

  EGLint major, minor;
  if (!eglInitialize(gldata->display, &major, &minor)) {
      EGLint error = eglGetError();
      iupAttribSetStrf(ih, "ERROR", "Could not initialize EGL. Error: 0x%X", error);
      gldata->display = EGL_NO_DISPLAY;
      return IUP_NOERROR;
  }

  if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE)
  {
      EGLint error = eglGetError();
      iupAttribSetStrf(ih, "ERROR", "Failed to bind EGL_OPENGL_API (Desktop OpenGL). Check if OpenGL (not ES) is supported. Error: 0x%X", error);

      return IUP_NOERROR;
  }

  /* Pass the detected target_visual_id to the config chooser.
     It will be 0 for GTK3, Qt, and Wayland, maintaining existing behavior. */
  if (!eGLCanvasChooseConfig(ih, gldata, target_visual_id))
    return IUP_NOERROR;

#if defined(IUP_EGL_USE_GTK3)
#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_WINDOW(gldata->window)) {
      /* GTK3 Wayland: Defer subsurface creation until first GLMakeCurrent call. */
      iupAttribSet(ih, "_IUP_GTK3_WAYLAND_LAZY_INIT", "1");
  }
  else
#endif
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_WINDOW(gldata->window)) {
      /* For X11, the native window is the X Window ID (XID). Use the GDK wrapper function. */
      /* HiDPI scaling for the window itself is handled by GDK/X11, EGL surface tracks it. Viewport is handled later. */
      native_window = (EGLNativeWindowType)gdk_x11_window_get_xid(gldata->window);
  }
#endif
#endif /* IUP_EGL_USE_GTK3 */

#if defined(IUP_EGL_USE_GTK4)
#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_SURFACE(gldata->gdk_surface)) {
      /* GTK4 Wayland: Defer subsurface creation until first GLMakeCurrent call. */
      iupAttribSet(ih, "_IUP_GTK4_WAYLAND_LAZY_INIT", "1");
  }
  else
#endif
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_SURFACE(gldata->gdk_surface))
  {
      /* GTK4 X11: Defer EGL surface creation until first GLMakeCurrent call.
       * At MAP time, the GdkSurface is often 1x1 (not properly sized yet).
       * Creating the EGL surface before the X11 window is properly sized causes the
       * GL content not to appear until after a resize. */
      iupAttribSet(ih, "_IUP_GTK4_X11_LAZY_INIT", "1");
  }
#endif
#endif /* IUP_EGL_USE_GTK4 */


#ifdef IUP_EGL_USE_QT
  /* Check if running on native Wayland (requires Qt 6.7+) */
  const char* platform_name = iupQtGetPlatformName();

  int has_qt_wayland_support = iupQtHasWaylandSupport();
  if (has_qt_wayland_support && platform_name && strcmp(platform_name, "wayland") == 0) {
      #if !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)
          /* Qt6.7+ Wayland: Defer subsurface creation until first GLMakeCurrent call. */
          iupAttribSet(ih, "_IUP_QT6_WAYLAND_LAZY_INIT", "1");
          return IUP_NOERROR;
      #endif /* !IUP_QT_DISABLE_WAYLAND_INCLUDES */
  }

  /* X11 or XWayland path (also fallback if Wayland subsurface fails or Qt<6.7) */
  if (native_window == (EGLNativeWindowType)NULL && gldata->qwindow) {
      unsigned long xid = iupQtGetNativeWindow(gldata->qwindow);
      if (xid != 0)
          native_window = (EGLNativeWindowType)xid;
  }
#endif /* IUP_EGL_USE_QT */

#ifdef IUP_EGL_USE_EFL
#ifdef HAVE_ECORE_WL2
  if (eGLCanvasEflIsWayland(gldata->ee)) {
      /* EFL Wayland: Defer subsurface creation until first GLMakeCurrent call. */
      iupAttribSet(ih, "_IUP_EFL_WAYLAND_LAZY_INIT", "1");
  }
#endif
#endif /* IUP_EGL_USE_EFL */

  /* If we are not in lazy-init mode and still have no window, it's an error */
  if (native_window == (EGLNativeWindowType)NULL &&
      !iupAttribGet(ih, "_IUP_GTK3_WAYLAND_LAZY_INIT") &&
      !iupAttribGet(ih, "_IUP_GTK4_WAYLAND_LAZY_INIT") &&
      !iupAttribGet(ih, "_IUP_GTK4_X11_LAZY_INIT") &&
      !iupAttribGet(ih, "_IUP_QT6_WAYLAND_LAZY_INIT") &&
      !iupAttribGet(ih, "_IUP_EFL_WAYLAND_LAZY_INIT")) {
      /* This might happen if the backend is neither Wayland nor X11, or if getting the native handle failed. */
      iupAttribSet(ih, "ERROR", "Could not create/obtain native window handle (Wayland EGL window or X11 Window ID). Check backend.");
      return IUP_NOERROR;
  }

  EGLint surface_attribs[3] = { EGL_NONE, EGL_NONE, EGL_NONE };
  if (iupStrEqualNoCase(iupAttribGetStr(ih,"BUFFER"), "SINGLE"))
  {
      surface_attribs[0] = EGL_RENDER_BUFFER;
      surface_attribs[1] = EGL_SINGLE_BUFFER;
  }

  /* Only create surface immediately if not in lazy init mode */
  if (native_window != (EGLNativeWindowType)NULL &&
      !iupAttribGet(ih, "_IUP_GTK3_WAYLAND_LAZY_INIT") &&
      !iupAttribGet(ih, "_IUP_GTK4_WAYLAND_LAZY_INIT") &&
      !iupAttribGet(ih, "_IUP_GTK4_X11_LAZY_INIT") &&
      !iupAttribGet(ih, "_IUP_QT6_WAYLAND_LAZY_INIT") &&
      !iupAttribGet(ih, "_IUP_EFL_WAYLAND_LAZY_INIT")) {
      gldata->surface = eglCreateWindowSurface(gldata->display, gldata->config, native_window, surface_attribs);
      if (gldata->surface == EGL_NO_SURFACE)
      {
          EGLint egl_error = eglGetError();
          iupAttribSetStrf(ih, "ERROR", "Could not create EGL surface. Error: 0x%X", egl_error);
#if defined(GDK_WINDOWING_WAYLAND) || (defined(IUP_EGL_USE_QT) && !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)) || (defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_WL2))
          if (gldata->egl_window)
          {
              wl_egl_window_destroy(gldata->egl_window);
              gldata->egl_window = NULL;
          }
#endif
          return IUP_NOERROR;
      }

#if defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_X)
      /* EFL X11: Mark if surface was created when window was 1x1 */
      if (gldata->ee && eGLCanvasEflIsX11(gldata->ee))
      {
          Ecore_X_Window xwin = ecore_evas_window_get(gldata->ee);
          if (xwin)
          {
              int x, y, w, h;
              ecore_x_window_geometry_get(xwin, &x, &y, &w, &h);
              if (w <= 1 || h <= 1)
                  iupAttribSet(ih, "_IUP_EFL_X11_SURFACE_1x1", "1");
          }
      }
#endif
  }

  EGLint context_attribs[15];
  int a = 0;
  char* value;
  int use_legacy_context = 0;

  int arbcontext = iupAttribGetBoolean(ih, "ARBCONTEXT");
  int has_version = (iupAttribGetStr(ih, "CONTEXTVERSION") != NULL);
  int has_profile = (iupAttribGetStr(ih, "CONTEXTPROFILE") != NULL);
  int has_flags = (iupAttribGetStr(ih, "CONTEXTFLAGS") != NULL);

  if (arbcontext || has_version || has_profile || has_flags)
  {
    value = iupAttribGetStr(ih, "CONTEXTVERSION");
    if (value)
    {
        int major_v, minor_v;
        if (iupStrToIntInt(value, &major_v, &minor_v, '.') == 2)
        {
            requested_major = major_v;
            requested_minor = minor_v;

            context_attribs[a++] = EGL_CONTEXT_MAJOR_VERSION;
            context_attribs[a++] = major_v;
            context_attribs[a++] = EGL_CONTEXT_MINOR_VERSION;
            context_attribs[a++] = minor_v;
        }
    }

    value = iupAttribGetStr(ih, "CONTEXTFLAGS");
    if (value)
    {
        int flags = 0;
        if (iupStrEqualNoCase(value, "DEBUG"))
          flags = EGL_CONTEXT_OPENGL_DEBUG_BIT;
        else if (iupStrEqualNoCase(value, "FORWARDCOMPATIBLE"))
          flags = EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT;
        else if (iupStrEqualNoCase(value, "DEBUGFORWARDCOMPATIBLE"))
          flags = EGL_CONTEXT_OPENGL_DEBUG_BIT | EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT;

        if (flags)
        {
            context_attribs[a++] = EGL_CONTEXT_FLAGS_KHR;
            context_attribs[a++] = flags;
        }
    }

    value = iupAttribGetStr(ih, "CONTEXTPROFILE");
    if (value)
    {
        requested_profile = value;
        int profile = 0;
        if (iupStrEqualNoCase(value, "CORE"))
            profile = EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT;
        else if (iupStrEqualNoCase(value, "COMPATIBILITY"))
            profile = EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT;

        if (profile)
        {
            context_attribs[a++] = EGL_CONTEXT_OPENGL_PROFILE_MASK;
            context_attribs[a++] = profile;
        }
    }

    /* If requesting core profile without explicit version, default to 3.2 (minimum for core) */
    if (requested_profile && iupStrEqualNoCase(requested_profile, "CORE") && requested_major == 0)
    {
        if (a == 0 || requested_major < 3 || (requested_major == 3 && requested_minor < 2))
        {
            context_attribs[a++] = EGL_CONTEXT_MAJOR_VERSION;
            context_attribs[a++] = 3;
            context_attribs[a++] = EGL_CONTEXT_MINOR_VERSION;
            context_attribs[a++] = 2;
            requested_major = 3;
            requested_minor = 2;
        }
    }

    context_attribs[a] = EGL_NONE;
  }
  else
  {
    use_legacy_context = 1;
    context_attribs[0] = EGL_NONE;
  }

  ih_shared = IupGetAttributeHandle(ih, "SHAREDCONTEXT");
  if (ih_shared && IupClassMatch(ih_shared, "glcanvas"))
  {
    IGlControlData* shared_gldata = (IGlControlData*)iupAttribGet(ih_shared, "_IUP_GLCONTROLDATA");
    if (shared_gldata)
        shared_context = shared_gldata->context;
  }

  gldata->context = eglCreateContext(gldata->display, gldata->config, shared_context, context_attribs);

  if (gldata->context == EGL_NO_CONTEXT && !use_legacy_context)
  {
    if (requested_profile && a > 0)
    {
        /* Remove profile specification and try again */
        int new_a = 0;
        value = iupAttribGetStr(ih, "CONTEXTVERSION");
        if (value && requested_major > 0 && requested_minor >= 0)
        {
            context_attribs[new_a++] = EGL_CONTEXT_MAJOR_VERSION;
            context_attribs[new_a++] = requested_major;
            context_attribs[new_a++] = EGL_CONTEXT_MINOR_VERSION;
            context_attribs[new_a++] = requested_minor;
        }
        context_attribs[new_a] = EGL_NONE;

        gldata->context = eglCreateContext(gldata->display, gldata->config, shared_context, context_attribs);
    }

    if (gldata->context == EGL_NO_CONTEXT)
    {
        /* Try fallback to legacy context without any specific attributes */
        EGLint legacy_context_attribs[] = { EGL_NONE };
        gldata->context = eglCreateContext(gldata->display, gldata->config, shared_context, legacy_context_attribs);

        if (gldata->context == EGL_NO_CONTEXT)
        {
            iupAttribSetStrf(ih, "ERROR", "Could not create EGL context (including fallback). Error: 0x%X", eglGetError());
        }
    }
  }

  if (gldata->context == EGL_NO_CONTEXT)
  {
    if (gldata->surface != EGL_NO_SURFACE) {
        eglDestroySurface(gldata->display, gldata->surface);
        gldata->surface = EGL_NO_SURFACE;
    }
#if defined(GDK_WINDOWING_WAYLAND) || (defined(IUP_EGL_USE_QT) && !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES))
    if (gldata->egl_window) {
        wl_egl_window_destroy(gldata->egl_window);
        gldata->egl_window = NULL;
    }
#endif
    return IUP_NOERROR;
  }

  iupAttribSet(ih, "CONTEXT", (char*)gldata->context);

  if (gldata->context != EGL_NO_CONTEXT)
  {
      int physical_width = 0, physical_height = 0;

      /* Get the current size (physical). This also updates gldata->last_logical_width/height. */
      eGLCanvasGetActualSize(ih, gldata, &physical_width, &physical_height);

      /* Call the default resize handler with the determined logical size.
         It handles Wayland synchronization, HiDPI scaling, and setting the viewport. */
      eGLCanvasDefaultResize(ih, gldata->last_logical_width, gldata->last_logical_height);
  }

  // INDEX color mode is not supported in EGL/Modern OpenGL, so no Colormap creation needed.

  iupAttribSet(ih, "ERROR", NULL);
  return IUP_NOERROR;
}

static void eGLCanvasUnMapMethod(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");

  if (!gldata || gldata->display == EGL_NO_DISPLAY)
    return;

#ifdef IUP_EGL_USE_QT
  /* If using Qt's EGL context, DON'T destroy it - Qt owns it */
  if (iupAttribGet(ih, "_IUP_GLCANVAS_QT_CONTEXT"))
  {
    /* Just clear our pointers, but don't destroy Qt's resources */
    gldata->context = EGL_NO_CONTEXT;
    gldata->surface = EGL_NO_SURFACE;
    gldata->display = EGL_NO_DISPLAY;
    gldata->qwindow = NULL;
    return;
  }
#endif

  if (gldata->context != EGL_NO_CONTEXT)
  {
    if (eglGetCurrentContext() == gldata->context)
      eglMakeCurrent(gldata->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    eglDestroyContext(gldata->display, gldata->context);
    gldata->context = EGL_NO_CONTEXT;
  }

  if (gldata->surface != EGL_NO_SURFACE)
  {
    eglDestroySurface(gldata->display, gldata->surface);
    gldata->surface = EGL_NO_SURFACE;
  }

/* Clean up Wayland EGL window (GTK3/GTK4/Qt6/EFL) */
#if defined(GDK_WINDOWING_WAYLAND) || (defined(IUP_EGL_USE_QT) && !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)) || (defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_WL2))
  if (gldata->egl_window)
  {
    wl_egl_window_destroy(gldata->egl_window);
    gldata->egl_window = NULL;
  }
#endif

/* Clean up Wayland subsurface resources (GTK4/Qt6/EFL) */
#if (defined(IUP_EGL_USE_GTK4) && defined(GDK_WINDOWING_WAYLAND)) || (defined(IUP_EGL_USE_QT) && !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)) || (defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_WL2))
  if (gldata->subsurface)
  {
    wl_subsurface_destroy(gldata->subsurface);
    gldata->subsurface = NULL;
  }

  if (gldata->subsurface_wl)
  {
    wl_surface_destroy(gldata->subsurface_wl);
    gldata->subsurface_wl = NULL;
  }

  /* Destroy Wayland proxies attached to our event queue BEFORE destroying the queue.
   * Order matters: destroy proxies in reverse order of creation.
   */
  if (gldata->registry_compositor)
  {
    wl_compositor_destroy(gldata->registry_compositor);
    gldata->registry_compositor = NULL;
  }

  if (gldata->subcompositor)
  {
    wl_subcompositor_destroy(gldata->subcompositor);
    gldata->subcompositor = NULL;
  }

  if (gldata->registry)
  {
    wl_registry_destroy(gldata->registry);
    gldata->registry = NULL;
  }

  /* Now safe to destroy the event queue (all attached proxies have been destroyed) */
  if (gldata->event_queue)
  {
    wl_event_queue_destroy(gldata->event_queue);
    gldata->event_queue = NULL;
  }

  /* Clear compositor pointer (but don't destroy - owned by GTK/Qt/EFL) */
  gldata->compositor = NULL;
#endif

  /* On X11, the native window is managed by GTK/GDK/Qt/EFL, so we don't destroy it. */

#if defined(IUP_EGL_USE_GTK3)
  gldata->window = NULL;
#endif /* IUP_EGL_USE_GTK3 */
#if defined(IUP_EGL_USE_GTK4)
  gldata->gdk_surface = NULL;
#endif /* IUP_EGL_USE_GTK4 */
#ifdef IUP_EGL_USE_QT
  gldata->qwindow = NULL;
#endif
#ifdef IUP_EGL_USE_EFL
  gldata->evas_obj = NULL;
  gldata->ee = NULL;
#endif
  gldata->display = EGL_NO_DISPLAY;
}

static void eGLCanvasDestroy(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (gldata)
  {
    eGLCanvasUnMapMethod(ih);
    free(gldata);
  }
  iupAttribSet(ih, "_IUP_GLCONTROLDATA", NULL);
}

void iupdrvGlCanvasInitClass(Iclass* ic)
{
  ic->Create = eGLCanvasCreateMethod;
  ic->Destroy = eGLCanvasDestroy;
  ic->Map = eGLCanvasMapMethod;
  ic->UnMap = eGLCanvasUnMapMethod;

  iupClassRegisterAttribute(ic, "VISUAL", eGLCanvasGetVisualAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_STRING|IUPAF_NOT_MAPPED);
}

int IupGLIsCurrent(Ihandle* ih)
{
  IGlControlData* gldata;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return 0;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "glcanvas"))
    return 0;

  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || gldata->display == EGL_NO_DISPLAY || gldata->context == EGL_NO_CONTEXT)
    return 0;

  if (gldata->context == eglGetCurrentContext())
    return 1;

  return 0;
}

void IupGLMakeCurrent(Ihandle* ih)
{
  IGlControlData* gldata;
  int performed_resize = 0;
  int physical_width = 0, physical_height = 0;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "glcanvas"))
    return;

  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata)
    return;

  if (!gldata || gldata->display == EGL_NO_DISPLAY)
    return;

  /* Qt6 Wayland lazy initialization: Create subsurface+surface+context on first GLMakeCurrent */
#if defined(IUP_EGL_USE_QT) && !defined(IUP_QT_DISABLE_WAYLAND_INCLUDES)
  if (gldata->surface == EGL_NO_SURFACE && iupAttribGet(ih, "_IUP_QT6_WAYLAND_LAZY_INIT"))
  {
    /* Create the Wayland subsurface */
    EGLNativeWindowType native_window = eGLCanvasCreateQt6WaylandSubsurface(ih, gldata);

    if (native_window == (EGLNativeWindowType)NULL) {
      /* Window not ready yet. Keep the lazy init flag and return silently.
         The next GLMakeCurrent call will retry when the window is exposed. */
      return;
    }

    /* Now create EGL surface */
    EGLint surface_attribs[3] = { EGL_NONE, EGL_NONE, EGL_NONE };
    if (iupStrEqualNoCase(iupAttribGetStr(ih,"BUFFER"), "SINGLE"))
    {
      surface_attribs[0] = EGL_RENDER_BUFFER;
      surface_attribs[1] = EGL_SINGLE_BUFFER;
    }

    gldata->surface = eglCreateWindowSurface(gldata->display, gldata->config, native_window, surface_attribs);

    if (gldata->surface == EGL_NO_SURFACE)
    {
      EGLint egl_error = eglGetError();
      iupAttribSetStrf(ih, "ERROR", "Could not create EGL surface during lazy init. Error: 0x%X", egl_error);
      if (gldata->egl_window)
      {
        wl_egl_window_destroy(gldata->egl_window);
        gldata->egl_window = NULL;
      }
      return;
    }

    /* Create EGL context (simplified - using defaults) */
    EGLContext shared_context = EGL_NO_CONTEXT;
    Ihandle* ih_shared = (Ihandle*)iupAttribGet(ih, "SHAREDCONTEXT");
    if (ih_shared && iupObjectCheck(ih_shared) && IupClassMatch(ih_shared, "glcanvas"))
    {
      IGlControlData* shared_gldata = (IGlControlData*)iupAttribGet(ih_shared, "_IUP_GLCONTROLDATA");
      if (shared_gldata)
        shared_context = shared_gldata->context;
    }

    EGLint context_attribs[] = { EGL_NONE };
    gldata->context = eglCreateContext(gldata->display, gldata->config, shared_context, context_attribs);

    if (gldata->context == EGL_NO_CONTEXT)
    {
      iupAttribSet(ih, "ERROR", "Failed to create EGL context during lazy initialization");
      return;
    }

    /* Clear the lazy init flag */
    iupAttribSet(ih, "_IUP_QT6_WAYLAND_LAZY_INIT", NULL);
  }
#endif

  /* GTK3 Wayland lazy initialization: Create subsurface+surface+context on first GLMakeCurrent */
#if defined(IUP_EGL_USE_GTK3) && defined(GDK_WINDOWING_WAYLAND)
  if (gldata->surface == EGL_NO_SURFACE && iupAttribGet(ih, "_IUP_GTK3_WAYLAND_LAZY_INIT"))
  {
    /* Create the Wayland subsurface */
    EGLNativeWindowType native_window = eGLCanvasCreateGtk3WaylandSubsurface(ih, gldata);

    if (native_window == (EGLNativeWindowType)NULL) {
      iupAttribSet(ih, "ERROR", "Failed to create Wayland subsurface during lazy initialization");
      return;
    }

    /* Now create EGL surface */
    EGLint surface_attribs[3] = { EGL_NONE, EGL_NONE, EGL_NONE };
    if (iupStrEqualNoCase(iupAttribGetStr(ih,"BUFFER"), "SINGLE"))
    {
      surface_attribs[0] = EGL_RENDER_BUFFER;
      surface_attribs[1] = EGL_SINGLE_BUFFER;
    }

    gldata->surface = eglCreateWindowSurface(gldata->display, gldata->config, native_window, surface_attribs);

    if (gldata->surface == EGL_NO_SURFACE)
    {
      EGLint egl_error = eglGetError();
      iupAttribSetStrf(ih, "ERROR", "Could not create EGL surface during lazy init. Error: 0x%X", egl_error);
      if (gldata->egl_window)
      {
        wl_egl_window_destroy(gldata->egl_window);
        gldata->egl_window = NULL;
      }
      return;
    }

    /* Create EGL context (same code as in MapMethod) */
    Ihandle* ih_shared = (Ihandle*)iupAttribGet(ih, "SHAREDCONTEXT");
    EGLContext shared_context = EGL_NO_CONTEXT;
    if (ih_shared && iupObjectCheck(ih_shared))
    {
      IGlControlData* shared_gldata = (IGlControlData*)iupAttribGet(ih_shared, "_IUP_GLCONTROLDATA");
      if (shared_gldata)
        shared_context = shared_gldata->context;
    }

    EGLint context_attribs[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
    };
    gldata->context = eglCreateContext(gldata->display, gldata->config, shared_context, context_attribs);

    if (gldata->context == EGL_NO_CONTEXT)
    {
      EGLint egl_error = eglGetError();
      iupAttribSetStrf(ih, "ERROR", "Could not create EGL context during lazy init. Error: 0x%X", egl_error);
      return;
    }

    /* Clear the lazy init flag */
    iupAttribSet(ih, "_IUP_GTK3_WAYLAND_LAZY_INIT", NULL);
  }
#endif

  /* GTK4 Wayland lazy initialization: Create subsurface+surface+context on first GLMakeCurrent */
#if defined(IUP_EGL_USE_GTK4) && defined(GDK_WINDOWING_WAYLAND)
  if (gldata->surface == EGL_NO_SURFACE && iupAttribGet(ih, "_IUP_GTK4_WAYLAND_LAZY_INIT"))
  {
    /* Create the Wayland subsurface */
    EGLNativeWindowType native_window = eGLCanvasCreateGtk4WaylandSubsurface(ih, gldata);

    if (native_window == (EGLNativeWindowType)NULL) {
      iupAttribSet(ih, "ERROR", "Failed to create Wayland subsurface during lazy initialization");
      return;
    }

    /* Now create EGL surface */
    EGLint surface_attribs[3] = { EGL_NONE, EGL_NONE, EGL_NONE };
    if (iupStrEqualNoCase(iupAttribGetStr(ih,"BUFFER"), "SINGLE"))
    {
      surface_attribs[0] = EGL_RENDER_BUFFER;
      surface_attribs[1] = EGL_SINGLE_BUFFER;
    }

    gldata->surface = eglCreateWindowSurface(gldata->display, gldata->config, native_window, surface_attribs);

    if (gldata->surface == EGL_NO_SURFACE)
    {
      EGLint egl_error = eglGetError();
      iupAttribSetStrf(ih, "ERROR", "Could not create EGL surface during lazy init. Error: 0x%X", egl_error);
      if (gldata->egl_window)
      {
        wl_egl_window_destroy(gldata->egl_window);
        gldata->egl_window = NULL;
      }
      return;
    }

    /* Create EGL context (same code as in MapMethod) */
    Ihandle* ih_shared = (Ihandle*)iupAttribGet(ih, "SHAREDCONTEXT");
    EGLContext shared_context = EGL_NO_CONTEXT;
    if (ih_shared && iupObjectCheck(ih_shared))
    {
      IGlControlData* shared_gldata = (IGlControlData*)iupAttribGet(ih_shared, "_IUP_GLCONTROLDATA");
      if (shared_gldata)
        shared_context = shared_gldata->context;
    }

    EGLint context_attribs[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
    };
    gldata->context = eglCreateContext(gldata->display, gldata->config, shared_context, context_attribs);

    if (gldata->context == EGL_NO_CONTEXT)
    {
      iupAttribSet(ih, "ERROR", "Failed to create EGL context during lazy initialization");
      return;
    }

    /* Clear the lazy init flag */
    iupAttribSet(ih, "_IUP_GTK4_WAYLAND_LAZY_INIT", NULL);
  }
#endif

  /* GTK4 X11 lazy initialization: Create EGL surface+context on first GLMakeCurrent */
#if defined(IUP_EGL_USE_GTK4) && defined(GDK_WINDOWING_X11)
  if (gldata->surface == EGL_NO_SURFACE && iupAttribGet(ih, "_IUP_GTK4_X11_LAZY_INIT"))
  {
    /* Get the native X11 window */
    EGLNativeWindowType native_window = (EGLNativeWindowType)NULL;
    int surf_w = 0, surf_h = 0;

    if (GDK_IS_X11_SURFACE(gldata->gdk_surface)) {
        native_window = (EGLNativeWindowType)gdk_x11_surface_get_xid(gldata->gdk_surface);
        surf_w = gdk_surface_get_width(gldata->gdk_surface);
        surf_h = gdk_surface_get_height(gldata->gdk_surface);
    }

    if (native_window == (EGLNativeWindowType)NULL) {
      iupAttribSet(ih, "ERROR", "Failed to get X11 window XID during lazy initialization");
      return;
    }

    /* Create the EGL surface (even at 1x1, for gl.Init() to work) */
    EGLint surface_attribs[3] = { EGL_NONE, EGL_NONE, EGL_NONE };
    if (iupStrEqualNoCase(iupAttribGetStr(ih,"BUFFER"), "SINGLE"))
    {
        surface_attribs[0] = EGL_RENDER_BUFFER;
        surface_attribs[1] = EGL_SINGLE_BUFFER;
    }

    gldata->surface = eglCreateWindowSurface(gldata->display, gldata->config, native_window, surface_attribs);
    if (gldata->surface == EGL_NO_SURFACE)
    {
      EGLint egl_error = eglGetError();
      iupAttribSetStrf(ih, "ERROR", "Could not create EGL surface during lazy init. Error: 0x%X", egl_error);
      return;
    }

    /* If created at 1x1, mark for recreation when properly sized */
    if (surf_w <= 1 || surf_h <= 1) {
        iupAttribSet(ih, "_IUP_GTK4_X11_SURFACE_1x1", "1");
    }

    /* Create the EGL context (same code as in MapMethod) */
    Ihandle* ih_shared = (Ihandle*)iupAttribGet(ih, "SHAREDCONTEXT");
    EGLContext shared_context = EGL_NO_CONTEXT;
    if (ih_shared && iupStrEqualNoCase(IupGetClassName(ih_shared), "glcanvas"))
    {
      IGlControlData* shared_gldata = (IGlControlData*)iupAttribGet(ih_shared, "_IUP_GLCONTROLDATA");
      if (shared_gldata)
        shared_context = shared_gldata->context;
    }

    int requested_major = 0, requested_minor = 0;
    char* version_str = iupAttribGet(ih, "CONTEXTVERSION");
    if (version_str)
      iupStrToIntInt(version_str, &requested_major, &requested_minor, '.');

    char* requested_profile = iupAttribGet(ih, "CONTEXTPROFILE");

    EGLint ctx_attribs[20];
    int ctx_idx = 0;

    if (requested_major > 0)
    {
      ctx_attribs[ctx_idx++] = EGL_CONTEXT_MAJOR_VERSION;
      ctx_attribs[ctx_idx++] = requested_major;
      if (requested_minor > 0)
      {
        ctx_attribs[ctx_idx++] = EGL_CONTEXT_MINOR_VERSION;
        ctx_attribs[ctx_idx++] = requested_minor;
      }
    }

    if (requested_profile)
    {
      if (iupStrEqualNoCase(requested_profile, "CORE"))
      {
        ctx_attribs[ctx_idx++] = EGL_CONTEXT_OPENGL_PROFILE_MASK;
        ctx_attribs[ctx_idx++] = EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT;
      }
      else if (iupStrEqualNoCase(requested_profile, "COMPATIBILITY"))
      {
        ctx_attribs[ctx_idx++] = EGL_CONTEXT_OPENGL_PROFILE_MASK;
        ctx_attribs[ctx_idx++] = EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT;
      }
    }

    ctx_attribs[ctx_idx] = EGL_NONE;

    gldata->context = eglCreateContext(gldata->display, gldata->config, shared_context, ctx_attribs);
    if (gldata->context == EGL_NO_CONTEXT)
    {
      EGLint egl_error = eglGetError();
      iupAttribSetStrf(ih, "ERROR", "Could not create EGL context during lazy init. Error: 0x%X", egl_error);
      return;
    }

    /* Clear the lazy init flag */
    iupAttribSet(ih, "_IUP_GTK4_X11_LAZY_INIT", NULL);
  }
#endif

  /* EFL Wayland lazy initialization: Create subsurface+surface+context on first GLMakeCurrent */
#if defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_WL2)
  if (gldata->surface == EGL_NO_SURFACE && iupAttribGet(ih, "_IUP_EFL_WAYLAND_LAZY_INIT"))
  {
    /* Create the Wayland subsurface */
    EGLNativeWindowType native_window = eGLCanvasCreateEflWaylandSubsurface(ih, gldata);

    if (native_window == (EGLNativeWindowType)NULL) {
      iupAttribSet(ih, "ERROR", "Failed to create Wayland subsurface during lazy initialization");
      return;
    }

    /* Now create EGL surface */
    EGLint surface_attribs[3] = { EGL_NONE, EGL_NONE, EGL_NONE };
    if (iupStrEqualNoCase(iupAttribGetStr(ih,"BUFFER"), "SINGLE"))
    {
      surface_attribs[0] = EGL_RENDER_BUFFER;
      surface_attribs[1] = EGL_SINGLE_BUFFER;
    }

    gldata->surface = eglCreateWindowSurface(gldata->display, gldata->config, native_window, surface_attribs);

    if (gldata->surface == EGL_NO_SURFACE)
    {
      EGLint egl_error = eglGetError();
      iupAttribSetStrf(ih, "ERROR", "Could not create EGL surface during lazy init. Error: 0x%X", egl_error);
      if (gldata->egl_window)
      {
        wl_egl_window_destroy(gldata->egl_window);
        gldata->egl_window = NULL;
      }
      return;
    }

    /* Create EGL context */
    Ihandle* ih_shared = (Ihandle*)iupAttribGet(ih, "SHAREDCONTEXT");
    EGLContext shared_context = EGL_NO_CONTEXT;
    if (ih_shared && iupObjectCheck(ih_shared))
    {
      IGlControlData* shared_gldata = (IGlControlData*)iupAttribGet(ih_shared, "_IUP_GLCONTROLDATA");
      if (shared_gldata)
        shared_context = shared_gldata->context;
    }

    EGLint context_attribs[] = { EGL_NONE };
    gldata->context = eglCreateContext(gldata->display, gldata->config, shared_context, context_attribs);

    if (gldata->context == EGL_NO_CONTEXT)
    {
      EGLint egl_error = eglGetError();
      iupAttribSetStrf(ih, "ERROR", "Could not create EGL context during lazy init. Error: 0x%X", egl_error);
      return;
    }

    /* Clear the lazy init flag */
    iupAttribSet(ih, "_IUP_EFL_WAYLAND_LAZY_INIT", NULL);
  }
#endif

  /* GTK4 X11: Check if surface needs recreation (was created at 1x1, now properly sized) */
#if defined(IUP_EGL_USE_GTK4) && defined(GDK_WINDOWING_X11)
  if (gldata->surface != EGL_NO_SURFACE && iupAttribGet(ih, "_IUP_GTK4_X11_SURFACE_1x1"))
  {
    int surf_w = 0, surf_h = 0;
    if (GDK_IS_X11_SURFACE(gldata->gdk_surface)) {
        surf_w = gdk_surface_get_width(gldata->gdk_surface);
        surf_h = gdk_surface_get_height(gldata->gdk_surface);
    }

    /* If window is now properly sized, recreate the surface */
    if (surf_w > 1 && surf_h > 1) {
        EGLNativeWindowType native_window = (EGLNativeWindowType)gdk_x11_surface_get_xid(gldata->gdk_surface);

        /* Destroy old 1x1 surface */
        eglDestroySurface(gldata->display, gldata->surface);

        /* Create new surface at proper size */
        EGLint surface_attribs[3] = { EGL_NONE, EGL_NONE, EGL_NONE };
        if (iupStrEqualNoCase(iupAttribGetStr(ih,"BUFFER"), "SINGLE"))
        {
            surface_attribs[0] = EGL_RENDER_BUFFER;
            surface_attribs[1] = EGL_SINGLE_BUFFER;
        }

        gldata->surface = eglCreateWindowSurface(gldata->display, gldata->config, native_window, surface_attribs);
        if (gldata->surface == EGL_NO_SURFACE)
        {
          EGLint egl_error = eglGetError();
          iupAttribSetStrf(ih, "ERROR", "Could not recreate EGL surface. Error: 0x%X", egl_error);
          return;
        }

        /* Clear the 1x1 flag */
        iupAttribSet(ih, "_IUP_GTK4_X11_SURFACE_1x1", NULL);
    }
  }
#endif

  /* EFL X11: Check if surface needs recreation */
#if defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_X)
  if (gldata->surface != EGL_NO_SURFACE && iupAttribGet(ih, "_IUP_EFL_X11_SURFACE_1x1"))
  {
    if (gldata->ee && eGLCanvasEflIsX11(gldata->ee))
    {
      Ecore_X_Window xwin = ecore_evas_window_get(gldata->ee);
      if (xwin)
      {
        int x, y, w, h;
        ecore_x_window_geometry_get(xwin, &x, &y, &w, &h);

        /* If window is now properly sized, recreate the surface */
        if (w > 1 && h > 1)
        {
          /* Destroy old 1x1 surface */
          eglDestroySurface(gldata->display, gldata->surface);

          /* Create new surface at proper size */
          EGLint surface_attribs[3] = { EGL_NONE, EGL_NONE, EGL_NONE };
          if (iupStrEqualNoCase(iupAttribGetStr(ih,"BUFFER"), "SINGLE"))
          {
            surface_attribs[0] = EGL_RENDER_BUFFER;
            surface_attribs[1] = EGL_SINGLE_BUFFER;
          }

          gldata->surface = eglCreateWindowSurface(gldata->display, gldata->config, (EGLNativeWindowType)xwin, surface_attribs);
          if (gldata->surface == EGL_NO_SURFACE)
          {
            EGLint egl_error = eglGetError();
            iupAttribSetStrf(ih, "ERROR", "Could not recreate EGL surface. Error: 0x%X", egl_error);
            return;
          }

          /* Clear the 1x1 flag */
          iupAttribSet(ih, "_IUP_EFL_X11_SURFACE_1x1", NULL);
        }
      }
    }
  }
#endif

  if (gldata->context == EGL_NO_CONTEXT || gldata->surface == EGL_NO_SURFACE)
  {
    return;
  }

#ifdef GDK_WINDOWING_WAYLAND
  if (gldata->egl_window) {

    eGLCanvasGetActualSize(ih, gldata, &physical_width, &physical_height);

    if (physical_width != gldata->egl_window_physical_width ||
        physical_height != gldata->egl_window_physical_height)
    {
      wl_egl_window_resize(gldata->egl_window, physical_width, physical_height, 0, 0);
      gldata->egl_window_physical_width = physical_width;
      gldata->egl_window_physical_height = physical_height;
      performed_resize = 1;
    }
  }
#endif

#if defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_WL2)
  if (gldata->egl_window) {
    eGLCanvasGetActualSize(ih, gldata, &physical_width, &physical_height);

    if (physical_width != gldata->egl_window_physical_width ||
        physical_height != gldata->egl_window_physical_height)
    {
      wl_egl_window_resize(gldata->egl_window, physical_width, physical_height, 0, 0);
      gldata->egl_window_physical_width = physical_width;
      gldata->egl_window_physical_height = physical_height;
      performed_resize = 1;
    }
  }
#endif

  if (eglMakeCurrent(gldata->display, gldata->surface, gldata->surface, gldata->context) == EGL_FALSE)
  {
    iupAttribSetStrf(ih, "ERROR", "Failed to set new current context. Error: 0x%X", eglGetError());
  }
  else
  {
#ifdef GDK_WINDOWING_WAYLAND
    if (performed_resize)
    {
      glViewport(0, 0, physical_width, physical_height);
    }
#endif

#if defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_WL2)
    if (performed_resize)
    {
      glViewport(0, 0, physical_width, physical_height);
    }
#endif

    iupAttribSet(ih, "ERROR", NULL);

    if (!IupGetGlobal("GL_VERSION"))
    {
      IupSetStrGlobal("GL_VENDOR", (char*)glGetString(GL_VENDOR));
      IupSetStrGlobal("GL_RENDERER", (char*)glGetString(GL_RENDERER));
      IupSetStrGlobal("GL_VERSION", (char*)glGetString(GL_VERSION));
    }
  }
}

void IupGLSwapBuffers(Ihandle* ih)
{
  IGlControlData* gldata;
  Icallback cb;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "glcanvas"))
    return;

  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || gldata->display == EGL_NO_DISPLAY || gldata->surface == EGL_NO_SURFACE)
    return;

  cb = IupGetCallback(ih, "SWAPBUFFERS_CB");
  if (cb)
    cb(ih);

  /* eglSwapBuffers commits the SUBSURFACE with the new rendered buffer.
   * Qt/GTK will commit the parent surface as part of their rendering pipeline. */
#if defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_WL2)
  if (gldata->ee && eGLCanvasEflIsWayland(gldata->ee))
  {
    /* EFL Wayland: Set swap interval 0 to avoid blocking on frame callback. EFL's own wayland_egl engine does this.
     * Without this, eglSwapBuffers blocks on second call waiting for frame callback. */
    eglSwapInterval(gldata->display, 0);
  }
#endif
  eglSwapBuffers(gldata->display, gldata->surface);
#if defined(IUP_EGL_USE_EFL) && defined(HAVE_ECORE_WL2)
  if (gldata->ee && eGLCanvasEflIsWayland(gldata->ee))
  {
    /* Flush display to ensure events are sent to compositor */
    Ecore_Wl2_Window* wl2_win = ecore_evas_wayland2_window_get(gldata->ee);
    if (wl2_win)
    {
      Ecore_Wl2_Display* wl2_display = ecore_wl2_window_display_get(wl2_win);
      if (wl2_display)
        ecore_wl2_display_flush(wl2_display);
    }
  }
#endif
}

void IupGLPalette(Ihandle* ih, int index, float r, float g, float b)
{
  /* EGL/modern OpenGL doesn't support indexed color mode and palettes. */
  (void)ih;
  (void)index;
  (void)r;
  (void)g;
  (void)b;
}

void IupGLUseFont(Ihandle* ih, int first, int count, int list_base)
{
  /* OpenGL font rendering using display lists (like wglUseFontBitmaps or glXUseXFont) is deprecated and not available in EGL/Core Profile. */
  (void)ih;
  (void)first;
  (void)count;
  (void)list_base;
}

void IupGLWait(int gl)
{
  if (gl)
    eglWaitClient();
  else
    eglWaitNative(EGL_CORE_NATIVE_ENGINE);
}
