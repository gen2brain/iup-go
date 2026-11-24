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
  iupAttribSet(ih, "_IUPGL_COMPOSITE_DMABUF", NULL);
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
  {
    iupAttribSet(ih, "_IUPGL_COMPOSITE_FRESH", "1");
    iupdrvPostRedraw(ih);
  }
}

#define IUP_EGL_HAS_PRESENT_DMABUF

typedef void (*IUPglEGLImageTargetTexture2DOES)(GLenum, void*);

/* Called on the Qt Quick render thread with its context current. Returns a texture of that context, owned by the caller, 0 on failure. */
static unsigned int eGLCanvasQmlImport(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  IGlDmabuf* dmabuf = (IGlDmabuf*)iupAttribGet(ih, "_IUPGL_COMPOSITE_DMABUF");
  PFNEGLCREATEIMAGEKHRPROC create_image;
  PFNEGLDESTROYIMAGEKHRPROC destroy_image;
  IUPglEGLImageTargetTexture2DOES image_target;
  EGLDisplay display = eglGetCurrentDisplay();
  EGLImageKHR image;
  EGLint attribs[40];
  GLint previous = 0;
  int i, n = 0, index;
  static const EGLint plane_fd[3] = {EGL_DMA_BUF_PLANE0_FD_EXT, EGL_DMA_BUF_PLANE1_FD_EXT, EGL_DMA_BUF_PLANE2_FD_EXT};
  static const EGLint plane_offset[3] = {EGL_DMA_BUF_PLANE0_OFFSET_EXT, EGL_DMA_BUF_PLANE1_OFFSET_EXT, EGL_DMA_BUF_PLANE2_OFFSET_EXT};
  static const EGLint plane_pitch[3] = {EGL_DMA_BUF_PLANE0_PITCH_EXT, EGL_DMA_BUF_PLANE1_PITCH_EXT, EGL_DMA_BUF_PLANE2_PITCH_EXT};
  static const EGLint plane_lo[3] = {EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT};
  static const EGLint plane_hi[3] = {EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT, EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT};

  if (!gldata)
    return 0;

  if (!dmabuf || !dmabuf->planes || dmabuf->planes > 3)
    return 0;

  index = dmabuf == &gldata->dmabuf[1];
  if (gldata->import_tex[index] && gldata->import_serial[index] == dmabuf->serial)
    return gldata->import_tex[index];

  create_image = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
  destroy_image = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
  image_target = (IUPglEGLImageTargetTexture2DOES)eglGetProcAddress("glEGLImageTargetTexture2DOES");
  if (display == EGL_NO_DISPLAY || !create_image || !destroy_image || !image_target)
  {
    iupAttribSet(ih, "_IUPGL_COMPOSITE_DMABUF_FAILED", "1");
    return 0;
  }

  attribs[n++] = EGL_WIDTH;
  attribs[n++] = gldata->fbo_w;
  attribs[n++] = EGL_HEIGHT;
  attribs[n++] = gldata->fbo_h;
  attribs[n++] = EGL_LINUX_DRM_FOURCC_EXT;
  attribs[n++] = dmabuf->fourcc;
  for (i = 0; i < dmabuf->planes; i++)
  {
    attribs[n++] = plane_fd[i];
    attribs[n++] = dmabuf->fds[i];
    attribs[n++] = plane_offset[i];
    attribs[n++] = dmabuf->offsets[i];
    attribs[n++] = plane_pitch[i];
    attribs[n++] = dmabuf->strides[i];
    if (dmabuf->modifier != 0x00ffffffffffffffULL)
    {
      attribs[n++] = plane_lo[i];
      attribs[n++] = (EGLint)(dmabuf->modifier & 0xffffffffULL);
      attribs[n++] = plane_hi[i];
      attribs[n++] = (EGLint)(dmabuf->modifier >> 32);
    }
  }
  attribs[n] = EGL_NONE;

  image = create_image(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attribs);
  if (image == EGL_NO_IMAGE_KHR)
  {
    iupAttribSet(ih, "_IUPGL_COMPOSITE_DMABUF_FAILED", "1");
    return 0;
  }

  glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);
  glGenTextures(1, &gldata->import_tex[index]);
  glBindTexture(GL_TEXTURE_2D, gldata->import_tex[index]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  image_target(GL_TEXTURE_2D, image);
  glBindTexture(GL_TEXTURE_2D, (GLuint)previous);
  destroy_image(display, image);

  gldata->import_serial[index] = dmabuf->serial;
  return gldata->import_tex[index];
}

static int iupEGLBackendPresentDmabuf(Ihandle* ih, IGlControlData* gldata, const IGlDmabuf* dmabuf, int width, int height)
{
  if (!dmabuf || iupAttribGet(ih, "_IUPGL_COMPOSITE_DMABUF_FAILED"))
  {
    iupAttribSet(ih, "_IUPGL_COMPOSITE_DMABUF", NULL);
    return 0;
  }

  iupAttribSet(ih, "_IUPGL_COMPOSITE_DMABUF", (char*)dmabuf);
  iupAttribSetInt(ih, "_IUPGL_COMPOSITE_W", width);
  iupAttribSetInt(ih, "_IUPGL_COMPOSITE_H", height);
  iupAttribSet(ih, "_IUPGL_COMPOSITE_IMPORT_CB", (char*)eGLCanvasQmlImport);
  iupEGLBackendQueueComposite(ih, gldata);
  return 1;
}

static int iupEGLBackendCheckDmabuf(Ihandle* ih, const IGlDmabuf* dmabuf, int width, int height, const unsigned char* pixels)
{
  (void)dmabuf; (void)width; (void)height; (void)pixels;
  return iupAttribGet(ih, "_IUPQML_OPENGL") ? 1 : -1;
}

#endif
