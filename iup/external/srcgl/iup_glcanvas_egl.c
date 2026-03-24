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

/* Backend selection */
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
#ifndef EGL_CONTEXT_FLAGS_KHR
#define EGL_CONTEXT_FLAGS_KHR 0x30FC
#endif

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

/* Forward declarations for Wayland types (used in struct as opaque pointers) */
struct wl_egl_window;
struct wl_surface;
struct wl_subsurface;
struct wl_compositor;
struct wl_subcompositor;
struct wl_registry;
struct wl_event_queue;

/* GL control data, common across all backends.
 * Backend-specific native handles stored as void* in backend_handle/backend_handle2. */
typedef struct _IGlControlData
{
  EGLDisplay display;
  EGLSurface surface;
  EGLContext context;
  EGLConfig config;

  void* backend_handle;    /* Backend-specific: GdkWindow* / GdkSurface* / QWindow* / Evas_Object* */
  void* backend_handle2;   /* Backend-specific: Ecore_Evas* (EFL only) */

  int last_logical_width;
  int last_logical_height;

  /* Wayland support fields (always present, NULL/0 when unused) */
  struct wl_egl_window* egl_window;
  int egl_window_physical_width;
  int egl_window_physical_height;

  struct wl_surface* subsurface_wl;
  struct wl_subsurface* subsurface;
  struct wl_surface* parent_surface;
  struct wl_compositor* compositor;
  struct wl_compositor* registry_compositor;
  struct wl_subcompositor* subcompositor;
  struct wl_registry* registry;
  struct wl_event_queue* event_queue;
} IGlControlData;

/* Forward declaration of common function used by backends */
static void eGLCanvasGetActualSize(Ihandle* ih, IGlControlData* gldata, int* physical_width, int* physical_height);

/*
 * Include backend implementation.
 * Each backend defines IUP_EGL_HAS_WAYLAND if it supports Wayland,
 * and implements the following static functions:
 *
 *   iupEGLBackendGetScaleAndSize          - Get HiDPI scale factor and realized widget size
 *   iupEGLBackendMapInit                  - Initialize native handles from IUP widget
 *   iupEGLBackendGetEGLDisplay            - Get EGL display from native handles
 *   iupEGLBackendPostConfig               - Setup native window or lazy init after EGL config chosen
 *   iupEGLBackendCreateLazyNativeWindow   - Create native window during lazy init
 *   iupEGLBackendCheckSurfaceRecreation   - Check if EGL surface needs recreation (1x1 workaround)
 *   iupEGLBackendPostSurfaceCreation      - Post-surface-creation checks
 *   iupEGLBackendUpdateSubsurfacePosition - Update Wayland subsurface position
 *   iupEGLBackendCleanup                  - Clean up backend-specific resources
 *   iupEGLBackendGetVisual                - Get visual for VISUAL attribute
 *   iupEGLBackendPreSwapBuffers           - Pre-swap handling
 *   iupEGLBackendPostSwapBuffers          - Post-swap handling
 */
#if defined(IUP_EGL_USE_GTK3)
  #include "iup_glcanvas_egl_gtk.c"
#elif defined(IUP_EGL_USE_GTK4)
  #include "iup_glcanvas_egl_gtk4.c"
#elif defined(IUP_EGL_USE_QT)
  #include "iup_glcanvas_egl_qt.c"
#elif defined(IUP_EGL_USE_EFL)
  #include "iup_glcanvas_egl_efl.c"
#endif

/* ============================================================ */
/* Common EGL implementation                                     */
/* ============================================================ */

static void eGLCanvasGetActualSize(Ihandle* ih, IGlControlData* gldata, int* physical_width, int* physical_height)
{
  int logical_w = 0, logical_h = 0;
  int realized_w = 0, realized_h = 0;
  int requested_w = 0, requested_h = 0;
  int scale = 1;

  iupEGLBackendGetScaleAndSize(ih, gldata, &scale, &realized_w, &realized_h);

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

static int eGLCanvasDefaultResize(Ihandle *ih, int width, int height)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  int physical_width, physical_height;
  int scale = 1;
  int realized_w = 0, realized_h = 0;

  if (!gldata)
    return IUP_DEFAULT;

  gldata->last_logical_width = width;
  gldata->last_logical_height = height;

  iupEGLBackendGetScaleAndSize(ih, gldata, &scale, &realized_w, &realized_h);

  physical_width = width * scale;
  physical_height = height * scale;

#ifdef IUP_EGL_HAS_WAYLAND
  if (gldata->egl_window) {
      int resize_w = physical_width < 1 ? 1 : physical_width;
      int resize_h = physical_height < 1 ? 1 : physical_height;

      wl_egl_window_resize(gldata->egl_window, resize_w, resize_h, 0, 0);

      gldata->egl_window_physical_width = resize_w;
      gldata->egl_window_physical_height = resize_h;
  }
#endif

  iupEGLBackendUpdateSubsurfacePosition(ih, gldata);


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

  gldata->last_logical_width = 100;
  gldata->last_logical_height = 100;

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

  {
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
    iupAttribSet(ih, "STEREO", "NO");
  }

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

  if (visual_id != 0)
  {
      EGLConfig* configs = NULL;
      EGLint total_configs = 0;

      if (eglChooseConfig(gldata->display, alist, NULL, 0, &total_configs) && total_configs > 0)
      {
          configs = (EGLConfig*)malloc(total_configs * sizeof(EGLConfig));
          if (eglChooseConfig(gldata->display, alist, configs, total_configs, &num_config))
          {
              int i;
              for (i = 0; i < num_config; i++)
              {
                  EGLint native_vid = 0;
                  if (eglGetConfigAttrib(gldata->display, configs[i], EGL_NATIVE_VISUAL_ID, &native_vid))
                  {
                      if (native_vid == visual_id)
                      {
                          gldata->config = configs[i];
                          break;
                      }
                  }
              }
          }
          free(configs);
      }
  }

  iupAttribSet(ih, "ERROR", NULL);
  return 1;
}

static char* eGLCanvasGetVisualAttrib(Ihandle *ih)
{
  return iupEGLBackendGetVisual(ih);
}

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

  /* Initialize backend-specific native handles */
  if (!iupEGLBackendMapInit(ih, gldata))
    return IUP_NOERROR;

  /* Try to load eglGetPlatformDisplay (EGL 1.5 core) or eglGetPlatformDisplayEXT (extension) */
  eglGetPlatformDisplay_func = (PFN_eglGetPlatformDisplay)eglGetProcAddress("eglGetPlatformDisplay");
  if (!eglGetPlatformDisplay_func)
      eglGetPlatformDisplay_func = (PFN_eglGetPlatformDisplay)eglGetProcAddress("eglGetPlatformDisplayEXT");

  /* Get EGL display from backend */
  gldata->display = iupEGLBackendGetEGLDisplay(ih, gldata, eglGetPlatformDisplay_func, &native_window, &target_visual_id);

  if (gldata->display == EGL_NO_DISPLAY) {
      gldata->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  }

  if (gldata->display == EGL_NO_DISPLAY) {
      iupAttribSet(ih, "ERROR", "Could not get EGL display (Platform specific, native handle, and default failed).");
      return IUP_NOERROR;
  }

  {
    EGLint major, minor;
    if (!eglInitialize(gldata->display, &major, &minor)) {
        EGLint error = eglGetError();
        iupAttribSetStrf(ih, "ERROR", "Could not initialize EGL. Error: 0x%X", error);
        gldata->display = EGL_NO_DISPLAY;
        return IUP_NOERROR;
    }
  }

  if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE)
  {
      EGLint error = eglGetError();
      iupAttribSetStrf(ih, "ERROR", "Failed to bind EGL_OPENGL_API (Desktop OpenGL). Check if OpenGL (not ES) is supported. Error: 0x%X", error);

      return IUP_NOERROR;
  }

  if (!eGLCanvasChooseConfig(ih, gldata, target_visual_id))
    return IUP_NOERROR;

  /* Post-config: get native window or setup lazy init */
  {
    int skip_rest = 0;
    EGLNativeWindowType post_native = iupEGLBackendPostConfig(ih, gldata, &skip_rest);
    if (skip_rest)
      return IUP_NOERROR;
    if (post_native)
      native_window = post_native;
  }

  if (native_window == (EGLNativeWindowType)NULL && !iupAttribGet(ih, "_IUP_EGL_LAZY_INIT")) {
      iupAttribSet(ih, "ERROR", "Could not create/obtain native window handle (Wayland EGL window or X11 Window ID). Check backend.");
      return IUP_NOERROR;
  }

  {
    EGLint surface_attribs[3] = { EGL_NONE, EGL_NONE, EGL_NONE };
    if (iupStrEqualNoCase(iupAttribGetStr(ih,"BUFFER"), "SINGLE"))
    {
        surface_attribs[0] = EGL_RENDER_BUFFER;
        surface_attribs[1] = EGL_SINGLE_BUFFER;
    }

    if (native_window != (EGLNativeWindowType)NULL && !iupAttribGet(ih, "_IUP_EGL_LAZY_INIT")) {
        gldata->surface = eglCreateWindowSurface(gldata->display, gldata->config, native_window, surface_attribs);
        if (gldata->surface == EGL_NO_SURFACE)
        {
            EGLint egl_error = eglGetError();
            iupAttribSetStrf(ih, "ERROR", "Could not create EGL surface. Error: 0x%X", egl_error);
#ifdef IUP_EGL_HAS_WAYLAND
            if (gldata->egl_window)
            {
                wl_egl_window_destroy(gldata->egl_window);
                gldata->egl_window = NULL;
            }
#endif
            return IUP_NOERROR;
        }

        iupEGLBackendPostSurfaceCreation(ih, gldata);
    }
  }

  /* Create context */
  {
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
          int profile = 0;
          requested_profile = value;
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
#ifdef IUP_EGL_HAS_WAYLAND
      if (gldata->egl_window) {
          wl_egl_window_destroy(gldata->egl_window);
          gldata->egl_window = NULL;
      }
#endif
      return IUP_NOERROR;
    }
  }

  iupAttribSet(ih, "CONTEXT", (char*)gldata->context);

  if (gldata->context != EGL_NO_CONTEXT)
  {
      int physical_width = 0, physical_height = 0;

      eGLCanvasGetActualSize(ih, gldata, &physical_width, &physical_height);

      eGLCanvasDefaultResize(ih, gldata->last_logical_width, gldata->last_logical_height);
  }

  iupAttribSet(ih, "ERROR", NULL);
  return IUP_NOERROR;
}

static void eGLCanvasUnMapMethod(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");

  if (!gldata || gldata->display == EGL_NO_DISPLAY)
    return;

  /* Check if backend owns the context (e.g., Qt may own it) */
  if (iupAttribGet(ih, "_IUP_GLCANVAS_QT_CONTEXT"))
  {
    gldata->context = EGL_NO_CONTEXT;
    gldata->surface = EGL_NO_SURFACE;
    gldata->display = EGL_NO_DISPLAY;
    iupEGLBackendCleanup(ih, gldata);
    return;
  }

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

#ifdef IUP_EGL_HAS_WAYLAND
  if (gldata->egl_window)
  {
    wl_egl_window_destroy(gldata->egl_window);
    gldata->egl_window = NULL;
  }

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

  if (gldata->event_queue)
  {
    wl_event_queue_destroy(gldata->event_queue);
    gldata->event_queue = NULL;
  }

  gldata->compositor = NULL;
#endif

  iupEGLBackendCleanup(ih, gldata);
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

  /* Lazy initialization: create native window + EGL surface + context on first call */
  if (gldata->surface == EGL_NO_SURFACE && iupAttribGet(ih, "_IUP_EGL_LAZY_INIT"))
  {
    EGLNativeWindowType native_window = (EGLNativeWindowType)NULL;
    EGLint context_attribs[20];

    int result = iupEGLBackendCreateLazyNativeWindow(ih, gldata, &native_window, context_attribs, 20);
    if (result == 0)
      return;
    if (result < 0)
      return;

    {
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
#ifdef IUP_EGL_HAS_WAYLAND
        if (gldata->egl_window)
        {
          wl_egl_window_destroy(gldata->egl_window);
          gldata->egl_window = NULL;
        }
#endif
        return;
      }
    }

    /* Create context */
    {
      EGLContext shared_ctx = EGL_NO_CONTEXT;
      Ihandle* ih_shared = IupGetAttributeHandle(ih, "SHAREDCONTEXT");
      if (ih_shared && IupClassMatch(ih_shared, "glcanvas"))
      {
        IGlControlData* shared_gldata = (IGlControlData*)iupAttribGet(ih_shared, "_IUP_GLCONTROLDATA");
        if (shared_gldata)
          shared_ctx = shared_gldata->context;
      }

      gldata->context = eglCreateContext(gldata->display, gldata->config, shared_ctx, context_attribs);
      if (gldata->context == EGL_NO_CONTEXT)
      {
        EGLint egl_error = eglGetError();
        iupAttribSetStrf(ih, "ERROR", "Could not create EGL context during lazy init. Error: 0x%X", egl_error);
        return;
      }
    }

    iupAttribSet(ih, "_IUP_EGL_LAZY_INIT", NULL);
  }

  /* Check if surface needs recreation (1x1 workaround) */
  {
    EGLNativeWindowType recreate_window = iupEGLBackendCheckSurfaceRecreation(ih, gldata);

    if (recreate_window)
    {
      EGLint surface_attribs[3] = { EGL_NONE, EGL_NONE, EGL_NONE };
      if (iupStrEqualNoCase(iupAttribGetStr(ih,"BUFFER"), "SINGLE"))
      {
        surface_attribs[0] = EGL_RENDER_BUFFER;
        surface_attribs[1] = EGL_SINGLE_BUFFER;
      }

      eglDestroySurface(gldata->display, gldata->surface);

      gldata->surface = eglCreateWindowSurface(gldata->display, gldata->config, recreate_window, surface_attribs);
      if (gldata->surface == EGL_NO_SURFACE)
      {
        EGLint egl_error = eglGetError();
        iupAttribSetStrf(ih, "ERROR", "Could not recreate EGL surface. Error: 0x%X", egl_error);
        return;
      }
    }
  }

  if (gldata->context == EGL_NO_CONTEXT || gldata->surface == EGL_NO_SURFACE)
  {
    return;
  }

  /* Check if Wayland EGL window needs resize */
#ifdef IUP_EGL_HAS_WAYLAND
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
    if (performed_resize)
    {
      glViewport(0, 0, physical_width, physical_height);
    }

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


  iupEGLBackendPreSwapBuffers(ih, gldata);
  eglSwapBuffers(gldata->display, gldata->surface);
  iupEGLBackendPostSwapBuffers(ih, gldata);
}

void IupGLPalette(Ihandle* ih, int index, float r, float g, float b)
{
  (void)ih;
  (void)index;
  (void)r;
  (void)g;
  (void)b;
}

void IupGLUseFont(Ihandle* ih, int first, int count, int list_base)
{
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
