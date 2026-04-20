/** \file
 * \brief FLTK backend for EGL GLCanvas.
 *        Included from iup_glcanvas_egl.c, not compiled standalone.
 *        Uses the generic native window shim (iup_glcanvas_nativeinfo.h).
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_GLCANVAS_EGL_FLTK_H
#define __IUP_GLCANVAS_EGL_FLTK_H

#include "iup.h"


#define IUP_EGL_HAS_WAYLAND

static int iupEGLBackendGetScale(void)
{
  int dpi = IupGetInt(NULL, "SCREENDPI");
  if (dpi > 0)
    return dpi / 96;
  return 1;
}

static void iupEGLBackendGetScaleAndSize(Ihandle* ih, IGlControlData* gldata, int* scale, int* realized_w, int* realized_h)
{
  (void)gldata;
  *scale = iupEGLBackendGetScale();
  if (*scale < 1) *scale = 1;

  *realized_w = 0;
  *realized_h = 0;
  if (ih->currentwidth > 0 && ih->currentheight > 0)
  {
    *realized_w = ih->currentwidth;
    *realized_h = ih->currentheight;
  }
}

static int iupEGLBackendMapInit(Ihandle* ih, IGlControlData* gldata)
{
  IGlNativeInfo info;

  if (!iupGLGetNativeInfo(ih, &info))
  {
    iupAttribSet(ih, "ERROR", "Could not obtain native window handle.");
    return 0;
  }

  if (info.has_own_window)
    gldata->backend_handle = info.canvas_window;
  else
    gldata->backend_handle = info.parent_window;

  return gldata->backend_handle ? 1 : 0;
}

static EGLDisplay iupEGLBackendGetEGLDisplay(Ihandle* ih, IGlControlData* gldata, PFN_eglGetPlatformDisplay func, EGLNativeWindowType* native_window, int* visual_id)
{
  IGlNativeInfo info;
  EGLDisplay display = EGL_NO_DISPLAY;

  (void)gldata;
  *visual_id = 0;
  *native_window = (EGLNativeWindowType)0;

  if (!iupGLGetNativeInfo(ih, &info))
    return EGL_NO_DISPLAY;

  if (strcmp(info.windowing, "WAYLAND") == 0)
  {
    if (info.display)
    {
      if (func)
        display = func(EGL_PLATFORM_WAYLAND_KHR, info.display, NULL);
      if (display == EGL_NO_DISPLAY)
        display = eglGetDisplay((EGLNativeDisplayType)info.display);
    }
  }
  else
  {
    if (info.display)
    {
      if (func)
        display = func(EGL_PLATFORM_X11_KHR, info.display, NULL);
      if (display == EGL_NO_DISPLAY)
        display = eglGetDisplay((EGLNativeDisplayType)info.display);
    }
  }

  return display;
}

static EGLNativeWindowType iupEGLBackendPostConfig(Ihandle* ih, IGlControlData* gldata, int* skip_rest)
{
  IGlNativeInfo info;
  *skip_rest = 0;

  if (!iupGLGetNativeInfo(ih, &info))
    return (EGLNativeWindowType)NULL;

  if (strcmp(info.windowing, "WAYLAND") == 0)
  {
    iupAttribSet(ih, "_IUP_EGL_LAZY_INIT", "1");
    *skip_rest = 1;
    return (EGLNativeWindowType)NULL;
  }

  /* X11: use lazy init — at map time the surface may be 1x1 */
  iupAttribSet(ih, "_IUP_EGL_LAZY_INIT", "1");

  (void)gldata;
  return (EGLNativeWindowType)NULL;
}

static int iupEGLBackendCreateLazyNativeWindow(Ihandle* ih, IGlControlData* gldata, EGLNativeWindowType* native_window, EGLint* context_attribs, int max_attribs)
{
  IGlNativeInfo info;
  (void)max_attribs;

  if (!iupGLGetNativeInfo(ih, &info))
    return -1;

  if (strcmp(info.windowing, "WAYLAND") == 0)
  {
    if (info.display && info.parent_window)
    {
      struct IGlWaylandSubsurface ws;
      int scale = iupEGLBackendGetScale();
      if (iupGLCreateWaylandSubsurface(
            (struct wl_display*)info.display,
            (struct wl_surface*)info.parent_window,
            info.x, info.y, info.w, info.h, scale, &ws))
      {
        eGLCopyWaylandSubsurface(&ws, gldata, (struct wl_surface*)info.parent_window);
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

  /* X11: use the dialog/canvas XID directly */
  if (info.has_own_window)
    *native_window = (EGLNativeWindowType)(uintptr_t)info.canvas_window;
  else
    *native_window = (EGLNativeWindowType)(uintptr_t)info.parent_window;

  if (*native_window == (EGLNativeWindowType)NULL)
    return -1;

  if (ih->currentwidth <= 1 || ih->currentheight <= 1)
    iupAttribSet(ih, "_IUP_EGL_SURFACE_1x1", "1");

  context_attribs[0] = EGL_NONE;
  return 1;
}

static EGLNativeWindowType iupEGLBackendCheckSurfaceRecreation(Ihandle* ih, IGlControlData* gldata)
{
  if (gldata->surface != EGL_NO_SURFACE && iupAttribGet(ih, "_IUP_EGL_SURFACE_1x1"))
  {
    if (ih->currentwidth > 1 && ih->currentheight > 1)
    {
      IGlNativeInfo info;
      iupAttribSet(ih, "_IUP_EGL_SURFACE_1x1", NULL);
      if (iupGLGetNativeInfo(ih, &info))
      {
        if (info.has_own_window)
          return (EGLNativeWindowType)(uintptr_t)info.canvas_window;
        else
          return (EGLNativeWindowType)(uintptr_t)info.parent_window;
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
  if (gldata->subsurface)
    wl_subsurface_set_position(gldata->subsurface, ih->x, ih->y);
}

static void iupEGLBackendGetWaylandMaxPhysicalSize(Ihandle* ih, IGlControlData* gldata, int* max_pw, int* max_ph)
{
  Ihandle* dialog;
  int client_w, client_h, x, y, scale;
  int border = 0, caption = 0, menu = 0;
  (void)gldata;

  *max_pw = 0; *max_ph = 0;

  dialog = IupGetDialog(ih);
  if (!dialog)
    return;

  iupdrvDialogGetDecoration(dialog, &border, &caption, &menu);

  client_w = dialog->currentwidth  - 2 * border;
  client_h = dialog->currentheight - 2 * border - caption;
  x = ih->x;
  y = ih->y;
  scale = iupEGLBackendGetScale();
  if (scale < 1) scale = 1;

  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (client_w > x) *max_pw = (client_w - x) * scale;
  if (client_h > y) *max_ph = (client_h - y) * scale;
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

#endif
