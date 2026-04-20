/** \file
 * \brief EFL backend for EGL GLCanvas.
 *        Included from iup_glcanvas_egl.c, not compiled standalone.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_GLCANVAS_EGL_EFL_H
#define __IUP_GLCANVAS_EGL_EFL_H

#include "iupcbs.h"

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>

#ifdef HAVE_ECORE_WL2
  #define IUP_EGL_HAS_WAYLAND
  #include <Ecore_Wl2.h>
#endif

#ifdef HAVE_ECORE_X
  #include <Ecore_X.h>
#endif

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

static int eGLCanvasEflIsGLEngine(Ecore_Evas* ee)
{
  const char* engine = ecore_evas_engine_name_get(ee);
  if (!engine)
    return 0;
  return (strstr(engine, "opengl") != NULL) || (strstr(engine, "egl") != NULL);
}

#ifdef HAVE_ECORE_X
static void eGLCanvasEflGetCanvasGeometry(IGlControlData* gldata, int* x, int* y, int* w, int* h)
{
  Evas_Object* evas_obj = (Evas_Object*)gldata->backend_handle;
  *x = 0;
  *y = 0;
  *w = 1;
  *h = 1;

  if (evas_obj)
  {
    Eina_Rect geom = efl_gfx_entity_geometry_get(evas_obj);
    if (geom.w > 0 && geom.h > 0)
    {
      *x = geom.x;
      *y = geom.y;
      *w = geom.w;
      *h = geom.h;
    }
  }
}

static Ecore_X_Window eGLCanvasEflCreateChildWindow(IGlControlData* gldata)
{
  Ecore_Evas* ee = (Ecore_Evas*)gldata->backend_handle2;
  Ecore_X_Window parent_xwin = ecore_evas_window_get(ee);
  Ecore_X_Window child;
  int x, y, w, h;

  if (!parent_xwin)
    return 0;

  eGLCanvasEflGetCanvasGeometry(gldata, &x, &y, &w, &h);

  child = ecore_x_window_new(parent_xwin, x, y, w, h);
  if (child)
    ecore_x_window_show(child);

  return child;
}
#endif

static void iupEGLBackendGetScaleAndSize(Ihandle* ih, IGlControlData* gldata, int* scale, int* realized_w, int* realized_h)
{
  Evas_Object* evas_obj = (Evas_Object*)gldata->backend_handle;
  Ecore_Evas* ee = (Ecore_Evas*)gldata->backend_handle2;
  *scale = 1;
  *realized_w = 0;
  *realized_h = 0;

  (void)ih;

  if (evas_obj)
  {
    Eina_Rect geom = efl_gfx_entity_geometry_get(evas_obj);
    if (geom.w > 0 && geom.h > 0)
    {
      *realized_w = geom.w;
      *realized_h = geom.h;
    }
  }

#ifdef HAVE_ECORE_X
  if (ee && eGLCanvasEflIsX11(ee))
  {
    int dpi_x = 0, dpi_y = 0;
    ecore_evas_screen_dpi_get(ee, &dpi_x, &dpi_y);
    if (dpi_x > 0)
    {
      *scale = dpi_x / 96;
      if (*scale < 1) *scale = 1;
    }
  }
#endif
}

static int iupEGLBackendMapInit(Ihandle* ih, IGlControlData* gldata)
{
  Evas_Object* evas_obj = (Evas_Object*)ih->handle;

  if (!evas_obj)
  {
    iupAttribSet(ih, "ERROR", "Could not obtain Evas_Object handle.");
    return 0;
  }

  gldata->backend_handle = evas_obj;

  {
    Evas* evas = evas_object_evas_get(evas_obj);
    if (evas)
      gldata->backend_handle2 = ecore_evas_ecore_evas_get(evas);
  }

  if (!gldata->backend_handle2)
  {
    iupAttribSet(ih, "ERROR", "Could not obtain Ecore_Evas handle.");
    return 0;
  }

  return 1;
}

static EGLDisplay iupEGLBackendGetEGLDisplay(Ihandle* ih, IGlControlData* gldata, PFN_eglGetPlatformDisplay func, EGLNativeWindowType* native_window, int* visual_id)
{
  Ecore_Evas* ee = (Ecore_Evas*)gldata->backend_handle2;
  EGLDisplay display = EGL_NO_DISPLAY;

  (void)ih;
  *visual_id = 0;

#ifdef HAVE_ECORE_WL2
  if (eGLCanvasEflIsWayland(ee)) {
    Ecore_Wl2_Window* wl2_win = ecore_evas_wayland2_window_get(ee);
    if (wl2_win) {
      Ecore_Wl2_Display* wl2_display = ecore_wl2_window_display_get(wl2_win);
      if (wl2_display) {
        struct wl_display* wl_display = ecore_wl2_display_get(wl2_display);
        if (wl_display) {
          if (func) {
            display = func(EGL_PLATFORM_WAYLAND_KHR, wl_display, NULL);
          }
          if (display == EGL_NO_DISPLAY) {
            display = eglGetDisplay((EGLNativeDisplayType)wl_display);
          }
        }
      }
    }
  }
  else
#endif
#ifdef HAVE_ECORE_X
  if (eGLCanvasEflIsX11(ee)) {
    Ecore_X_Display* x_display = ecore_x_display_get();
    if (x_display) {
      if (func) {
        display = func(EGL_PLATFORM_X11_KHR, x_display, NULL);
      }
      if (display == EGL_NO_DISPLAY) {
        display = eglGetDisplay((EGLNativeDisplayType)x_display);
      }
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
  Ecore_Evas* ee = (Ecore_Evas*)gldata->backend_handle2;
  *skip_rest = 0;

#ifdef HAVE_ECORE_WL2
  if (eGLCanvasEflIsWayland(ee)) {
    iupAttribSet(ih, "_IUP_EGL_LAZY_INIT", "1");
    return (EGLNativeWindowType)NULL;
  }
#endif

#ifdef HAVE_ECORE_X
  if (eGLCanvasEflIsX11(ee)) {
    Ecore_X_Window child = eGLCanvasEflCreateChildWindow(gldata);
    if (child)
    {
      iupAttribSet(ih, "_IUP_EGL_X11_CHILD", (char*)(uintptr_t)child);
      return (EGLNativeWindowType)child;
    }
  }
#endif

  (void)ih; (void)gldata;
  return (EGLNativeWindowType)NULL;
}

static void eGLCanvasEflGetSubsurfacePosition(IGlControlData* gldata, int* x, int* y)
{
  Evas_Object* evas_obj = (Evas_Object*)gldata->backend_handle;
  *x = 0;
  *y = 0;
  if (evas_obj)
  {
    Eina_Rect geom = efl_gfx_entity_geometry_get(evas_obj);
    *x = geom.x;
    *y = geom.y;

    {
      Evas* evas = evas_object_evas_get(evas_obj);
      if (evas) {
        int fx = 0, fy = 0;
        evas_output_framespace_get(evas, &fx, &fy, NULL, NULL);
        *x += fx;
        *y += fy;
      }
    }
  }
}

static int iupEGLBackendCreateLazyNativeWindow(Ihandle* ih, IGlControlData* gldata, EGLNativeWindowType* native_window, EGLint* context_attribs, int max_attribs)
{
  (void)max_attribs;

#ifdef HAVE_ECORE_WL2
  {
    Ecore_Evas* ee = (Ecore_Evas*)gldata->backend_handle2;
    if (eGLCanvasEflIsWayland(ee))
    {
      Ecore_Wl2_Window* wl2_win = ecore_evas_wayland2_window_get(ee);
      Ecore_Wl2_Display* wl2_display = wl2_win ? ecore_wl2_window_display_get(wl2_win) : NULL;
      struct wl_display* wl_display = wl2_display ? ecore_wl2_display_get(wl2_display) : NULL;
      struct wl_surface* parent_surface = wl2_win ? ecore_wl2_window_surface_get(wl2_win) : NULL;

      if (wl2_win)
        ecore_wl2_window_show(wl2_win);

      if (wl_display)
        wl_display_dispatch_pending(wl_display);

      if (wl_display && parent_surface)
      {
        int x = 0, y = 0;
        struct IGlWaylandSubsurface ws;

        eGLCanvasEflGetSubsurfacePosition(gldata, &x, &y);

        if (iupGLCreateWaylandSubsurface(wl_display, parent_surface,
              x, y, ih->currentwidth, ih->currentheight, 1, &ws))
        {
          eGLCopyWaylandSubsurface(&ws, gldata, parent_surface);
          *native_window = (EGLNativeWindowType)gldata->egl_window;

          context_attribs[0] = EGL_NONE;
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
#ifdef HAVE_ECORE_X
  if (gldata->surface != EGL_NO_SURFACE && iupAttribGet(ih, "_IUP_EGL_SURFACE_1x1"))
  {
    Ecore_X_Window child = (Ecore_X_Window)(uintptr_t)iupAttribGet(ih, "_IUP_EGL_X11_CHILD");
    if (child)
    {
      int x, y, w, h;
      ecore_x_window_geometry_get(child, &x, &y, &w, &h);
      if (w > 1 && h > 1)
      {
        iupAttribSet(ih, "_IUP_EGL_SURFACE_1x1", NULL);
        return (EGLNativeWindowType)child;
      }
    }
  }
#else
  (void)ih; (void)gldata;
#endif
  return (EGLNativeWindowType)NULL;
}

#ifdef HAVE_ECORE_X
static void eflGLCanvasPostRenderCallback(Ecore_Evas* ee)
{
  Ihandle* ih = (Ihandle*)ecore_evas_data_get(ee, "_IUP_EGL_CANVAS");
  if (!ih || !iupObjectCheck(ih))
    return;

  {
    IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
    if (!gldata || gldata->context == EGL_NO_CONTEXT || gldata->surface == EGL_NO_SURFACE)
      return;
  }

  {
    IFn cb = (IFn)IupGetCallback(ih, "ACTION");
    if (cb)
      cb(ih);
  }
}
#endif

static void iupEGLBackendPostSurfaceCreation(Ihandle* ih, IGlControlData* gldata)
{
#ifdef HAVE_ECORE_X
  {
    Ecore_Evas* ee = (Ecore_Evas*)gldata->backend_handle2;
    if (ee && eGLCanvasEflIsX11(ee))
    {
      Ecore_X_Window child = (Ecore_X_Window)(uintptr_t)iupAttribGet(ih, "_IUP_EGL_X11_CHILD");
      if (child)
      {
        int x, y, w, h;
        ecore_x_window_geometry_get(child, &x, &y, &w, &h);
        if (w <= 1 || h <= 1)
          iupAttribSet(ih, "_IUP_EGL_SURFACE_1x1", "1");
      }

      ecore_evas_data_set(ee, "_IUP_EGL_CANVAS", ih);
      ecore_evas_callback_post_render_set(ee, eflGLCanvasPostRenderCallback);
    }
  }
#else
  (void)ih;
#endif
  (void)gldata;
}

static void iupEGLBackendUpdateSubsurfacePosition(Ihandle* ih, IGlControlData* gldata)
{
#ifdef HAVE_ECORE_X
  {
    Ecore_Evas* ee = (Ecore_Evas*)gldata->backend_handle2;
    if (ee && eGLCanvasEflIsX11(ee))
    {
      Ecore_X_Window child = (Ecore_X_Window)(uintptr_t)iupAttribGet(ih, "_IUP_EGL_X11_CHILD");
      if (child)
      {
        int x, y, w, h;
        eGLCanvasEflGetCanvasGeometry(gldata, &x, &y, &w, &h);
        ecore_x_window_move_resize(child, x, y, w, h);
      }
    }
  }
#endif

#ifdef HAVE_ECORE_WL2
  if (gldata->subsurface)
  {
    int x = 0, y = 0;
    eGLCanvasEflGetSubsurfacePosition(gldata, &x, &y);
    wl_subsurface_set_position(gldata->subsurface, x, y);
  }
#endif

  (void)ih; (void)gldata;
}

static void iupEGLBackendGetWaylandMaxPhysicalSize(Ihandle* ih, IGlControlData* gldata, int* max_pw, int* max_ph)
{
  (void)ih; (void)gldata;
  *max_pw = 0; *max_ph = 0;
}

static void iupEGLBackendCleanup(Ihandle* ih, IGlControlData* gldata)
{
#ifdef HAVE_ECORE_X
  {
    Ecore_Evas* ee = (Ecore_Evas*)gldata->backend_handle2;
    if (ee && eGLCanvasEflIsX11(ee))
    {
      ecore_evas_callback_post_render_set(ee, NULL);
      ecore_evas_data_set(ee, "_IUP_EGL_CANVAS", NULL);
    }
  }

  {
    Ecore_X_Window child = (Ecore_X_Window)(uintptr_t)iupAttribGet(ih, "_IUP_EGL_X11_CHILD");
    if (child)
    {
      ecore_x_window_free(child);
      iupAttribSet(ih, "_IUP_EGL_X11_CHILD", NULL);
    }
  }
#endif
  (void)ih;
  gldata->backend_handle = NULL;
  gldata->backend_handle2 = NULL;
}

static char* iupEGLBackendGetVisual(Ihandle* ih)
{
  (void)ih;
  return NULL;
}

static void iupEGLBackendPreSwapBuffers(Ihandle* ih, IGlControlData* gldata)
{
#ifdef HAVE_ECORE_WL2
  {
    Ecore_Evas* ee = (Ecore_Evas*)gldata->backend_handle2;
    if (ee && eGLCanvasEflIsWayland(ee))
    {
      eglSwapInterval(gldata->display, 0);
    }
  }
#endif
  (void)ih; (void)gldata;
}

static void iupEGLBackendPostSwapBuffers(Ihandle* ih, IGlControlData* gldata)
{
  Ecore_Evas* ee = (Ecore_Evas*)gldata->backend_handle2;

#ifdef HAVE_ECORE_WL2
  if (ee && eGLCanvasEflIsWayland(ee))
  {
    Ecore_Wl2_Window* wl2_win = ecore_evas_wayland2_window_get(ee);
    if (wl2_win)
    {
      Ecore_Wl2_Display* wl2_display = ecore_wl2_window_display_get(wl2_win);
      if (wl2_display)
        ecore_wl2_display_flush(wl2_display);
    }
  }
#endif

  if (ee && eGLCanvasEflIsGLEngine(ee))
    eglMakeCurrent(gldata->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

  (void)ih;
}

#endif
