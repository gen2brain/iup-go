/** \file
 * \brief iupgl control for EGL (GTK3+/Wayland and X11)
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

typedef struct _IGlControlData
{
  EGLDisplay display;
  EGLSurface surface;
  EGLContext context;
  EGLConfig config;

  GdkWindow* window;
  int last_logical_width;
  int last_logical_height;

#ifdef GDK_WINDOWING_WAYLAND
  struct wl_egl_window* egl_window;
  int egl_window_physical_width;
  int egl_window_physical_height;
#endif
  /* For X11, the GdkWindow encapsulates the XID, which is used directly with EGL. */
} IGlControlData;


static void eGLCanvasGetActualSize(Ihandle* ih, IGlControlData* gldata, int* physical_width, int* physical_height)
{
  int logical_w = 0, logical_h = 0;
  int realized_w = 0, realized_h = 0;
  int requested_w = 0, requested_h = 0;
  int scale = 1;

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

  if (ih->currentwidth > 0 && ih->currentheight > 0)
  {
    requested_w = ih->currentwidth;
    requested_h = ih->currentheight;
  }

  if (requested_w <= 0 || requested_h <= 0)
  {
      char* size_str = IupGetAttribute(ih, "RASTERSIZE");
      if (size_str)
      {
          int rw=0, rh=0;
          if (sscanf(size_str, "%dx%d", &rw, &rh) == 2 && rw > 0 && rh > 0) {
              requested_w = rw;
              requested_h = rh;
          }
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

  if (logical_w <= 0) logical_w = 640;
  if (logical_h <= 0) logical_h = 480;

  gldata->last_logical_width = logical_w;
  gldata->last_logical_height = logical_h;

  *physical_width = logical_w * scale;
  *physical_height = logical_h * scale;

  if (*physical_width < 1) *physical_width = 1;
  if (*physical_height < 1) *physical_height = 1;
}


static int eGLCanvasDefaultResize(Ihandle *ih, int width, int height)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  int physical_width, physical_height;
  int scale = 1;

  if (!gldata)
    return IUP_DEFAULT;

  gldata->last_logical_width = width;
  gldata->last_logical_height = height;

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

  physical_width = width * scale;
  physical_height = height * scale;

#ifdef GDK_WINDOWING_WAYLAND
  /* In Wayland, the native EGL window must be resized explicitly to physical dimensions */
  if (gldata->egl_window) {
      int resize_w = physical_width < 1 ? 1 : physical_width;
      int resize_h = physical_height < 1 ? 1 : physical_height;
      wl_egl_window_resize(gldata->egl_window, resize_w, resize_h, 0, 0);

      gldata->egl_window_physical_width = resize_w;
      gldata->egl_window_physical_height = resize_h;
  }
#endif
  /* On X11/EGL, the surface typically tracks the window size automatically, but we still need the correct viewport. */
  if (gldata->context != EGL_NO_CONTEXT)
  {
    IupGLMakeCurrent(ih);
    glViewport(0, 0, physical_width, physical_height);
  }
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

  gldata->last_logical_width = 640;
  gldata->last_logical_height = 480;

#ifdef GDK_WINDOWING_WAYLAND
  gldata->egl_window_physical_width = 0;
  gldata->egl_window_physical_height = 0;
#endif

  iupAttribSet(ih, "_IUP_GLCONTROLDATA", (char*)gldata);

  IupSetCallback(ih, "RESIZE_CB", (Icallback)eGLCanvasDefaultResize);

  return IUP_NOERROR;
}

static int eGLCanvasChooseConfig(Ihandle* ih, IGlControlData* gldata)
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

  /* double buffer */
  /* In EGL, BUFFER=SINGLE/DOUBLE is handled during surface creation, not config selection. */

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

  iupAttribSet(ih, "ERROR", NULL);
  return 1;
}

static char* eGLCanvasGetVisualAttrib(Ihandle *ih)
{
  (void)ih;
  GdkScreen* screen = gdk_screen_get_default();
  if (screen) {
      GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
      if (!visual)
          visual = gdk_screen_get_system_visual(screen);
      return (char*)visual;
  }
  return NULL;
}

static int eGLCanvasMapMethod(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  EGLContext shared_context = EGL_NO_CONTEXT;
  Ihandle* ih_shared;
  EGLNativeWindowType native_window = (EGLNativeWindowType)NULL;
  GdkDisplay* gdk_display;
  PFN_eglGetPlatformDisplay eglGetPlatformDisplay_func = NULL;
  int requested_major = 0, requested_minor = 0;
  char* requested_profile = NULL;

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

  /* Try to load eglGetPlatformDisplay (EGL 1.5 core) or eglGetPlatformDisplayEXT (extension) */
  eglGetPlatformDisplay_func = (PFN_eglGetPlatformDisplay)eglGetProcAddress("eglGetPlatformDisplay");
  if (!eglGetPlatformDisplay_func)
      eglGetPlatformDisplay_func = (PFN_eglGetPlatformDisplay)eglGetProcAddress("eglGetPlatformDisplayEXT");


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
  else
#endif
  {
      /* Unsupported GDK backend for this EGL implementation (e.g. macOS Quartz, Windows GDK) */
  }

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

  if (!eGLCanvasChooseConfig(ih, gldata))
    return IUP_NOERROR;

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_WINDOW(gldata->window)) {
      struct wl_surface* surface = gdk_wayland_window_get_wl_surface(gldata->window);
      if (surface) {
          int physical_width = 0, physical_height = 0;

          eGLCanvasGetActualSize(ih, gldata, &physical_width, &physical_height);

          gldata->egl_window = wl_egl_window_create(surface, physical_width, physical_height);
          if (gldata->egl_window) {
              native_window = (EGLNativeWindowType)gldata->egl_window;

              gldata->egl_window_physical_width = physical_width;
              gldata->egl_window_physical_height = physical_height;
          }
      }
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

  if (native_window == (EGLNativeWindowType)NULL) {
      /* This might happen if the backend is neither Wayland nor X11, or if getting the native handle failed. */
      iupAttribSet(ih, "ERROR", "Could not create/obtain native window handle (Wayland EGL window or X11 Window ID). Check GDK backend.");
      return IUP_NOERROR;
  }

  EGLint surface_attribs[3] = { EGL_NONE, EGL_NONE, EGL_NONE };
  if (iupStrEqualNoCase(iupAttribGetStr(ih,"BUFFER"), "SINGLE"))
  {
      surface_attribs[0] = EGL_RENDER_BUFFER;
      surface_attribs[1] = EGL_SINGLE_BUFFER;
  }


  gldata->surface = eglCreateWindowSurface(gldata->display, gldata->config, native_window, surface_attribs);
  if (gldata->surface == EGL_NO_SURFACE) {
      iupAttribSetStrf(ih, "ERROR", "Could not create EGL surface. Error: 0x%X", eglGetError());
#ifdef GDK_WINDOWING_WAYLAND
      if (gldata->egl_window) {
          wl_egl_window_destroy(gldata->egl_window);
          gldata->egl_window = NULL;
      }
#endif
      return IUP_NOERROR;
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
    eglDestroySurface(gldata->display, gldata->surface);
    gldata->surface = EGL_NO_SURFACE;
#ifdef GDK_WINDOWING_WAYLAND
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

#ifdef GDK_WINDOWING_WAYLAND
  if (gldata->egl_window)
  {
    wl_egl_window_destroy(gldata->egl_window);
    gldata->egl_window = NULL;
  }
#endif
  /* On X11, the native window is managed by GTK/GDK, so we don't destroy it. */

  /* We do NOT call eglTerminate(gldata->display).
     eglInitialize increments a reference count. Since the display connection
     might be shared by other components (like GTK itself if using EGL),
     we rely on the application or environment to manage the EGL connection lifetime.
  */

  gldata->window = NULL;
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
  if (!gldata || gldata->display == EGL_NO_DISPLAY ||
      gldata->context == EGL_NO_CONTEXT || gldata->surface == EGL_NO_SURFACE)
    return;

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

  eglSwapBuffers(gldata->display, gldata->surface);
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
    glFinish(); /* Wait for GL commands to finish (implies eglWaitClient). */
  else
  {
#ifdef EGL_CORE_NATIVE_ENGINE
       eglWaitNative(EGL_CORE_NATIVE_ENGINE);
#else
       /* If EGL_CORE_NATIVE_ENGINE is not defined (older EGL versions), this synchronization might not be possible or necessary. */
#endif
  }
}
