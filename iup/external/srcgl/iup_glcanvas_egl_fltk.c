/** \file
 * \brief FLTK backend for EGL GLCanvas.
 *        Included from iup_glcanvas_egl.c, not compiled standalone.
 *
 * See Copyright Notice in "iup.h"
 */

#ifdef __cplusplus
extern "C" {
#endif

/* FLTK helper functions (implemented in iup_glcanvas_egl_fltk.cpp) */
extern void* iupFltkGLGetCanvasWindow(Ihandle* ih);
extern void* iupFltkGLCreateChildWindow(Ihandle* ih);
extern int iupFltkGLResizeChildWindow(void* child_window, Ihandle* ih);
extern void iupFltkGLDestroyChildWindow(void* child_window);
extern int iupFltkGLIsWayland(void);
extern void* iupFltkGLGetDisplay(void);
extern unsigned long iupFltkGLGetXid(void* fl_window);
extern void* iupFltkGLGetWlDisplay(void);
extern void* iupFltkGLGetWlSurface(void* fl_window);
extern void* iupFltkGLGetWlCompositor(void);
extern int iupFltkGLGetBufferScale(void* fl_window);
extern void iupFltkGLGetWidgetPosition(Ihandle* ih, int* x, int* y);

#ifdef __cplusplus
}
#endif

#ifndef IUP_FLTK_DISABLE_WAYLAND
  #define IUP_EGL_HAS_WAYLAND
  #include <wayland-client.h>
  #include <wayland-egl.h>
  #include "iup_glcanvas_egl_wayland.c"
#endif

static void iupEGLBackendGetScaleAndSize(Ihandle* ih, IGlControlData* gldata, int* scale, int* realized_w, int* realized_h)
{
  void* window = gldata->backend_handle;
  *scale = 1;
  *realized_w = 0;
  *realized_h = 0;

  if (window)
    *scale = iupFltkGLGetBufferScale(window);

  if (*scale < 1) *scale = 1;

  if (ih->currentwidth > 0 && ih->currentheight > 0)
  {
    *realized_w = ih->currentwidth;
    *realized_h = ih->currentheight;
  }
}

static int iupEGLBackendMapInit(Ihandle* ih, IGlControlData* gldata)
{
  void* window = iupFltkGLGetCanvasWindow(ih);

  if (!window)
  {
    iupAttribSet(ih, "ERROR", "Could not obtain FLTK window handle.");
    gldata->backend_handle = NULL;
    return 0;
  }

  gldata->backend_handle = window;

  if (!iupFltkGLIsWayland())
  {
    void* child = iupFltkGLCreateChildWindow(ih);
    if (child)
      iupAttribSet(ih, "_IUP_FLTK_GL_CHILD", (char*)child);
  }

  return 1;
}

static EGLDisplay iupEGLBackendGetEGLDisplay(Ihandle* ih, IGlControlData* gldata, PFN_eglGetPlatformDisplay func, EGLNativeWindowType* native_window, int* visual_id)
{
  EGLDisplay display = EGL_NO_DISPLAY;

  (void)ih;
  (void)gldata;
  *visual_id = 0;
  *native_window = (EGLNativeWindowType)0;

#ifndef IUP_FLTK_DISABLE_WAYLAND
  if (iupFltkGLIsWayland())
  {
    struct wl_display* wl_display = (struct wl_display*)iupFltkGLGetWlDisplay();
    if (wl_display)
    {
      if (func)
        display = func(EGL_PLATFORM_WAYLAND_KHR, wl_display, NULL);
      if (display == EGL_NO_DISPLAY)
        display = eglGetDisplay((EGLNativeDisplayType)wl_display);
    }
  }
  else
#endif
  {
    void* x_display = iupFltkGLGetDisplay();
    if (x_display)
    {
      if (func)
        display = func(EGL_PLATFORM_X11_KHR, x_display, NULL);
      if (display == EGL_NO_DISPLAY)
        display = eglGetDisplay((EGLNativeDisplayType)x_display);
    }
  }

  return display;
}

static EGLNativeWindowType iupEGLBackendPostConfig(Ihandle* ih, IGlControlData* gldata, int* skip_rest)
{
  *skip_rest = 0;

#ifndef IUP_FLTK_DISABLE_WAYLAND
  if (iupFltkGLIsWayland())
  {
    iupAttribSet(ih, "_IUP_EGL_LAZY_INIT", "1");
    *skip_rest = 1;
    return (EGLNativeWindowType)NULL;
  }
#endif

  {
    void* child = (void*)iupAttribGet(ih, "_IUP_FLTK_GL_CHILD");
    if (child)
    {
      unsigned long xid = iupFltkGLGetXid(child);
      if (xid != 0)
      {
        if (ih->currentwidth <= 1 || ih->currentheight <= 1)
          iupAttribSet(ih, "_IUP_EGL_SURFACE_1x1", "1");
        return (EGLNativeWindowType)xid;
      }
    }
  }

  (void)gldata;
  return (EGLNativeWindowType)NULL;
}

#ifndef IUP_FLTK_DISABLE_WAYLAND
static EGLNativeWindowType eGLCanvasCreateFltkWaylandSubsurface(Ihandle* ih, IGlControlData* gldata)
{
  EGLNativeWindowType native_window = (EGLNativeWindowType)NULL;
  void* window = gldata->backend_handle;
  struct wl_surface* parent_surface;
  struct wl_display* wl_display;
  int physical_width = 0, physical_height = 0;

  if (!window || !iupFltkGLIsWayland())
    return native_window;

  parent_surface = (struct wl_surface*)iupFltkGLGetWlSurface(window);
  if (!parent_surface)
    return native_window;

  gldata->parent_surface = parent_surface;

  wl_display = (struct wl_display*)iupFltkGLGetWlDisplay();
  if (!wl_display)
    return native_window;

  gldata->compositor = (struct wl_compositor*)iupFltkGLGetWlCompositor();
  if (!gldata->compositor)
    return native_window;

  gldata->subsurface_wl = wl_compositor_create_surface(gldata->compositor);
  if (!gldata->subsurface_wl)
    return native_window;

  if (!gldata->subcompositor)
  {
    gldata->subcompositor = eGLCanvasGetWaylandSubcompositor(wl_display, &gldata->event_queue, &gldata->registry_compositor, &gldata->registry);
  }

  if (!gldata->subcompositor)
  {
    wl_surface_destroy(gldata->subsurface_wl);
    gldata->subsurface_wl = NULL;
    return native_window;
  }

  gldata->subsurface = wl_subcompositor_get_subsurface(
      gldata->subcompositor,
      gldata->subsurface_wl,
      parent_surface);

  if (!gldata->subsurface)
  {
    wl_surface_destroy(gldata->subsurface_wl);
    gldata->subsurface_wl = NULL;
    return native_window;
  }

  wl_subsurface_set_desync(gldata->subsurface);

  {
    int subsurface_x = 0, subsurface_y = 0;
    iupFltkGLGetWidgetPosition(ih, &subsurface_x, &subsurface_y);
    wl_subsurface_set_position(gldata->subsurface, subsurface_x, subsurface_y);
  }

  if (gldata->parent_surface)
    wl_surface_commit(gldata->parent_surface);

  eGLCanvasGetActualSize(ih, gldata, &physical_width, &physical_height);

  gldata->egl_window = wl_egl_window_create(gldata->subsurface_wl, physical_width, physical_height);

  if (gldata->egl_window)
  {
    native_window = (EGLNativeWindowType)gldata->egl_window;
    gldata->egl_window_physical_width = physical_width;
    gldata->egl_window_physical_height = physical_height;
  }

  return native_window;
}
#endif

static int iupEGLBackendCreateLazyNativeWindow(Ihandle* ih, IGlControlData* gldata, EGLNativeWindowType* native_window, EGLint* context_attribs, int max_attribs)
{
  (void)max_attribs;

#ifndef IUP_FLTK_DISABLE_WAYLAND
  if (iupFltkGLIsWayland())
  {
    *native_window = eGLCanvasCreateFltkWaylandSubsurface(ih, gldata);
    if (*native_window == (EGLNativeWindowType)NULL)
    {
      iupAttribSet(ih, "ERROR", "Failed to create Wayland subsurface during lazy initialization");
      return -1;
    }

    context_attribs[0] = EGL_CONTEXT_CLIENT_VERSION;
    context_attribs[1] = 2;
    context_attribs[2] = EGL_NONE;
    return 1;
  }
#endif

  (void)ih; (void)gldata; (void)native_window; (void)context_attribs;
  return -1;
}

static EGLNativeWindowType iupEGLBackendCheckSurfaceRecreation(Ihandle* ih, IGlControlData* gldata)
{
  void* child = (void*)iupAttribGet(ih, "_IUP_FLTK_GL_CHILD");
  if (child)
  {
    int changed = iupFltkGLResizeChildWindow(child, ih);

    if (gldata->surface != EGL_NO_SURFACE && iupAttribGet(ih, "_IUP_EGL_SURFACE_1x1"))
    {
      if (ih->currentwidth > 1 && ih->currentheight > 1)
      {
        iupAttribSet(ih, "_IUP_EGL_SURFACE_1x1", NULL);
        return (EGLNativeWindowType)iupFltkGLGetXid(child);
      }
    }
  }

  return (EGLNativeWindowType)NULL;
}

static void iupEGLBackendPostSurfaceCreation(Ihandle* ih, IGlControlData* gldata)
{
  (void)ih; (void)gldata;
}

static void iupEGLBackendUpdateSubsurfacePosition(Ihandle* ih, IGlControlData* gldata)
{
#ifndef IUP_FLTK_DISABLE_WAYLAND
  if (gldata->subsurface)
  {
    int x = 0, y = 0;
    iupFltkGLGetWidgetPosition(ih, &x, &y);
    wl_subsurface_set_position(gldata->subsurface, x, y);
  }
#else
  (void)ih; (void)gldata;
#endif
}

static void iupEGLBackendCleanup(Ihandle* ih, IGlControlData* gldata)
{
  void* child = (void*)iupAttribGet(ih, "_IUP_FLTK_GL_CHILD");
  if (child)
  {
    iupFltkGLDestroyChildWindow(child);
    iupAttribSet(ih, "_IUP_FLTK_GL_CHILD", NULL);
  }
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
