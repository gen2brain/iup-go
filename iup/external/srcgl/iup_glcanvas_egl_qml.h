/** \file
 * \brief Qt Quick backend for EGL GLCanvas.
 *        Included from iup_glcanvas_egl.c, not compiled standalone.
 *        Qt Quick items have no native window: every canvas renders into a
 *        pbuffer and is composited into the scene graph, like GTK4.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_GLCANVAS_EGL_QML_H
#define __IUP_GLCANVAS_EGL_QML_H

#include "iup.h"
#include "iup_drv.h"


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
  const char* windowing = IupGetGlobal("WINDOWING");
  if (!windowing)
    return 0;

  if (strcmp(windowing, "WAYLAND") == 0 ? !IupGetGlobal("WL_DISPLAY") : !IupGetGlobal("XDISPLAY"))
    return 0;

  gldata->use_composite = 1;
  iupAttribSet(ih, "_IUPGL_COMPOSITE", "1");
  return 1;
}

static EGLDisplay iupEGLBackendGetEGLDisplay(Ihandle* ih, IGlControlData* gldata, PFN_eglGetPlatformDisplay func, EGLNativeWindowType* native_window, int* visual_id)
{
  EGLDisplay display = EGL_NO_DISPLAY;
  const char* windowing = IupGetGlobal("WINDOWING");
  int wl = (windowing && strcmp(windowing, "WAYLAND") == 0);
  void* conn = wl ? IupGetGlobal("WL_DISPLAY") : IupGetGlobal("XDISPLAY");
  EGLenum platform = wl ? EGL_PLATFORM_WAYLAND_KHR : EGL_PLATFORM_X11_KHR;

  (void)ih; (void)gldata;
  *visual_id = 0;
  *native_window = (EGLNativeWindowType)0;

  if (conn)
  {
    if (func)
      display = func(platform, conn, NULL);
    if (display == EGL_NO_DISPLAY)
      display = eglGetDisplay((EGLNativeDisplayType)conn);
  }
  return display;
}

static EGLNativeWindowType iupEGLBackendPostConfig(Ihandle* ih, IGlControlData* gldata, int* skip_rest)
{
  (void)ih; (void)gldata;
  *skip_rest = 0;
  return (EGLNativeWindowType)NULL;
}

static int iupEGLBackendCreateLazyNativeWindow(Ihandle* ih, IGlControlData* gldata, EGLNativeWindowType* native_window, EGLint* context_attribs, int max_attribs)
{
  (void)ih; (void)gldata; (void)native_window; (void)context_attribs; (void)max_attribs;
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
  (void)ih; (void)gldata;
}

static void iupEGLBackendGetWaylandMaxPhysicalSize(Ihandle* ih, IGlControlData* gldata, int* max_pw, int* max_ph)
{
  (void)ih; (void)gldata;
  *max_pw = 0; *max_ph = 0;
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

static void iupEGLBackendQueueComposite(Ihandle* ih, IGlControlData* gldata)
{
  (void)gldata;
  if (!iupAttribGet(ih, "_IUPGL_IN_DRAW"))
    iupdrvPostRedraw(ih);
}

#endif
