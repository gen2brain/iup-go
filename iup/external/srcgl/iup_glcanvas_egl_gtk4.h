/** \file
 * \brief GTK4 backend for EGL GLCanvas.
 *        Included from iup_glcanvas_egl.c, not compiled standalone.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_GLCANVAS_EGL_GTK4_H
#define __IUP_GLCANVAS_EGL_GTK4_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "iup.h"
#include "iup_dialog.h"

#ifdef GDK_WINDOWING_WAYLAND
  #define IUP_EGL_HAS_WAYLAND
  #include <gdk/wayland/gdkwayland.h>
#endif

#ifdef GDK_WINDOWING_X11
  #include <gdk/x11/gdkx.h>
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  #ifdef IUPX11_USE_DLOPEN
    #include "iupunix_x11.h"
  #endif
#endif

#ifdef GDK_WINDOWING_X11
static int iupEGLGetX11VisualID(Display* x_display, unsigned long window_handle)
{
  int visual_id = 0;
  XWindowAttributes attrs;

#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return 0;
#endif

  if (XGetWindowAttributes(x_display, (Window)window_handle, &attrs) != 0)
    visual_id = (int)XVisualIDFromVisual(attrs.visual);

  return visual_id;
}
#endif

static void iupEGLBackendGetScaleAndSize(Ihandle* ih, IGlControlData* gldata, int* scale, int* realized_w, int* realized_h)
{
  GdkSurface* gdk_surface = (GdkSurface*)gldata->backend_handle;
  *scale = 1;
  *realized_w = 0;
  *realized_h = 0;

  if (gdk_surface && GDK_IS_SURFACE(gdk_surface))
  {
    *scale = gdk_surface_get_scale_factor(gdk_surface);
  }
  else if (ih->handle && GTK_IS_WIDGET(ih->handle))
  {
    *scale = gtk_widget_get_scale_factor((GtkWidget*)ih->handle);
  }
  if (*scale < 1) *scale = 1;

  if (gldata->subsurface || gldata->use_composite)
  {
    if (ih->handle && GTK_IS_WIDGET(ih->handle))
    {
      int w = gtk_widget_get_width((GtkWidget*)ih->handle);
      int h = gtk_widget_get_height((GtkWidget*)ih->handle);
      if (w > 0 && h > 0)
      {
        *realized_w = w;
        *realized_h = h;
      }
    }
  }
  else
  {
    if (gdk_surface && GDK_IS_SURFACE(gdk_surface))
    {
      *realized_w = gdk_surface_get_width(gdk_surface);
      *realized_h = gdk_surface_get_height(gdk_surface);
    }

    if (*realized_w <= 0 || *realized_h <= 0)
    {
      if (ih->handle && GTK_IS_WIDGET(ih->handle))
      {
        int w = gtk_widget_get_width((GtkWidget*)ih->handle);
        int h = gtk_widget_get_height((GtkWidget*)ih->handle);
        if (w > 0 && h > 0)
        {
          *realized_w = w;
          *realized_h = h;
        }
      }
    }
  }
}

static int iupEGLBackendMapInit(Ihandle* ih, IGlControlData* gldata)
{
  GdkSurface* gdk_surface;

  gdk_surface = (GdkSurface*)iupAttribGet(ih, "_IUPGL_GDKSURFACE");

  if (!gdk_surface && ih->handle && GTK_IS_WIDGET(ih->handle))
  {
    GtkWidget* widget = (GtkWidget*)ih->handle;

    if (!gtk_widget_get_realized(widget))
    {
      gtk_widget_realize(widget);
    }

    {
      GtkNative* native = gtk_widget_get_native(widget);
      if (native)
      {
        gdk_surface = gtk_native_get_surface(native);
      }
    }

    if (gdk_surface)
    {
      iupAttribSet(ih, "_IUPGL_GDKSURFACE", (char*)gdk_surface);
    }
  }

  if (!gdk_surface)
  {
    char* surface_ptr = IupGetAttribute(ih, "_IUPGL_GDKSURFACE");
    if (surface_ptr)
    {
      gdk_surface = (GdkSurface*)surface_ptr;
    }
  }

  if (!gdk_surface || !GDK_IS_SURFACE(gdk_surface))
  {
    iupAttribSet(ih, "ERROR", "Could not obtain valid GdkSurface handle.");
    gldata->backend_handle = NULL;
    return 0;
  }

  gldata->backend_handle = gdk_surface;
  gldata->use_composite = 1;
  iupAttribSet(ih, "_IUPGL_COMPOSITE", "1");
  return 1;
}

static EGLDisplay iupEGLBackendGetEGLDisplay(Ihandle* ih, IGlControlData* gldata, PFN_eglGetPlatformDisplay func, EGLNativeWindowType* native_window, int* visual_id)
{
  GdkSurface* gdk_surface = (GdkSurface*)gldata->backend_handle;
  GdkDisplay* gdk_display = gdk_surface_get_display(gdk_surface);
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

    if (GDK_IS_X11_SURFACE(gdk_surface)) {
      *native_window = (EGLNativeWindowType)gdk_x11_surface_get_xid(gdk_surface);
      *visual_id = iupEGLGetX11VisualID(x_display, (unsigned long)*native_window);
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
  GdkSurface* gdk_surface = (GdkSurface*)gldata->backend_handle;
  *skip_rest = 0;

  if (gldata->use_composite)
    return (EGLNativeWindowType)NULL;

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_SURFACE(gdk_surface)) {
    iupAttribSet(ih, "_IUP_EGL_LAZY_INIT", "1");
    return (EGLNativeWindowType)NULL;
  }
  else
#endif
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_SURFACE(gdk_surface))
  {
    /* GTK4 X11: Defer EGL surface creation until first GLMakeCurrent.
     * At MAP time, the GdkSurface is often 1x1 (not properly sized yet). */
    iupAttribSet(ih, "_IUP_EGL_LAZY_INIT", "1");
    return (EGLNativeWindowType)NULL;
  }
#endif

  (void)ih;
  return (EGLNativeWindowType)NULL;
}

static void eGLCanvasGtk4GetSubsurfacePosition(Ihandle* ih, int* out_x, int* out_y)
{
  GtkWidget* widget = GTK_WIDGET(ih->handle);
  GtkWidget* root = widget ? GTK_WIDGET(gtk_widget_get_root(widget)) : NULL;
  graphene_rect_t bounds = { 0 };
  double surface_x = 0, surface_y = 0;

  *out_x = 0;
  *out_y = 0;

  if (!root)
    return;

  if (!gtk_widget_compute_bounds(widget, root, &bounds))
    return;

  gtk_native_get_surface_transform(GTK_NATIVE(root), &surface_x, &surface_y);

  *out_x = (int)bounds.origin.x + (int)surface_x;
  *out_y = (int)bounds.origin.y + (int)surface_y;
}

static int iupEGLBackendCreateLazyNativeWindow(Ihandle* ih, IGlControlData* gldata, EGLNativeWindowType* native_window, EGLint* context_attribs, int max_attribs)
{
  GdkSurface* gdk_surface = (GdkSurface*)gldata->backend_handle;

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_SURFACE(gdk_surface)) {
    GdkDisplay* gdk_display = gdk_surface_get_display(gdk_surface);
    struct wl_surface* parent_surface = gdk_wayland_surface_get_wl_surface(gdk_surface);
    struct wl_display* wl_display = gdk_wayland_display_get_wl_display(gdk_display);

    if (wl_display && parent_surface)
    {
      int x = 0, y = 0;
      int scale = gdk_surface_get_scale_factor(gdk_surface);
      struct IGlWaylandSubsurface ws;

      eGLCanvasGtk4GetSubsurfacePosition(ih, &x, &y);

      if (iupGLCreateWaylandSubsurface(wl_display, parent_surface,
            x, y, ih->currentwidth, ih->currentheight, scale, &ws))
      {
        eGLCopyWaylandSubsurface(&ws, gldata, parent_surface);
        *native_window = (EGLNativeWindowType)gldata->egl_window;

        if (ih->currentwidth <= 1 || ih->currentheight <= 1)
          iupAttribSet(ih, "_IUP_EGL_SURFACE_1x1", "1");

        context_attribs[0] = EGL_CONTEXT_CLIENT_VERSION;
        context_attribs[1] = 2;
        context_attribs[2] = EGL_NONE;
        return 1;
      }
    }

    iupAttribSet(ih, "ERROR", "Failed to create Wayland subsurface during lazy initialization");
    return -1;
  }
#endif

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_SURFACE(gdk_surface)) {
    int surf_w = 0, surf_h = 0;

    *native_window = (EGLNativeWindowType)gdk_x11_surface_get_xid(gdk_surface);
    surf_w = gdk_surface_get_width(gdk_surface);
    surf_h = gdk_surface_get_height(gdk_surface);

    if (*native_window == (EGLNativeWindowType)NULL) {
      iupAttribSet(ih, "ERROR", "Failed to get X11 window XID during lazy initialization");
      return -1;
    }

    if (surf_w <= 1 || surf_h <= 1) {
      iupAttribSet(ih, "_IUP_EGL_SURFACE_1x1", "1");
    }

    {
      int ctx_idx = 0;
      int requested_major = 0, requested_minor = 0;
      char* version_str = iupAttribGet(ih, "CONTEXTVERSION");
      char* requested_profile = iupAttribGet(ih, "CONTEXTPROFILE");

      if (version_str)
        iupStrToIntInt(version_str, &requested_major, &requested_minor, '.');

      if (requested_major > 0)
      {
        context_attribs[ctx_idx++] = EGL_CONTEXT_MAJOR_VERSION;
        context_attribs[ctx_idx++] = requested_major;
        if (requested_minor > 0)
        {
          context_attribs[ctx_idx++] = EGL_CONTEXT_MINOR_VERSION;
          context_attribs[ctx_idx++] = requested_minor;
        }
      }

      if (requested_profile)
      {
        if (iupStrEqualNoCase(requested_profile, "CORE"))
        {
          context_attribs[ctx_idx++] = EGL_CONTEXT_OPENGL_PROFILE_MASK;
          context_attribs[ctx_idx++] = EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT;
        }
        else if (iupStrEqualNoCase(requested_profile, "COMPATIBILITY"))
        {
          context_attribs[ctx_idx++] = EGL_CONTEXT_OPENGL_PROFILE_MASK;
          context_attribs[ctx_idx++] = EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT;
        }
      }

      context_attribs[ctx_idx] = EGL_NONE;
    }
    return 1;
  }
#endif

  (void)ih; (void)gldata; (void)native_window; (void)context_attribs; (void)max_attribs;
  return -1;
}

static EGLNativeWindowType iupEGLBackendCheckSurfaceRecreation(Ihandle* ih, IGlControlData* gldata)
{
  if (gldata->surface == EGL_NO_SURFACE || !iupAttribGet(ih, "_IUP_EGL_SURFACE_1x1"))
    return (EGLNativeWindowType)NULL;

#ifdef GDK_WINDOWING_WAYLAND
  if (gldata->egl_window && gldata->egl_window_physical_width > 1 && gldata->egl_window_physical_height > 1)
  {
    iupAttribSet(ih, "_IUP_EGL_SURFACE_1x1", NULL);
    return (EGLNativeWindowType)gldata->egl_window;
  }
#endif

#ifdef GDK_WINDOWING_X11
  {
    GdkSurface* gdk_surface = (GdkSurface*)gldata->backend_handle;
    int surf_w = 0, surf_h = 0;

    if (GDK_IS_X11_SURFACE(gdk_surface)) {
      surf_w = gdk_surface_get_width(gdk_surface);
      surf_h = gdk_surface_get_height(gdk_surface);
    }

    if (surf_w > 1 && surf_h > 1) {
      iupAttribSet(ih, "_IUP_EGL_SURFACE_1x1", NULL);
      return (EGLNativeWindowType)gdk_x11_surface_get_xid(gdk_surface);
    }
  }
#endif

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
    int subsurface_x = 0, subsurface_y = 0;
    eGLCanvasGtk4GetSubsurfacePosition(ih, &subsurface_x, &subsurface_y);
    iupwl_subsurface_set_position(gldata->subsurface, subsurface_x, subsurface_y);
  }
#else
  (void)ih; (void)gldata;
#endif
}

static void iupEGLBackendGetWaylandMaxPhysicalSize(Ihandle* ih, IGlControlData* gldata, int* max_pw, int* max_ph)
{
  *max_pw = 0; *max_ph = 0;
#ifdef GDK_WINDOWING_WAYLAND
  if (gldata->subsurface && ih->handle && GTK_IS_WIDGET(ih->handle)) {
    GtkWidget* widget = (GtkWidget*)ih->handle;
    GtkWidget* root = GTK_WIDGET(gtk_widget_get_root(widget));
    if (root) {
      GdkSurface* gdk_surf = gtk_native_get_surface(GTK_NATIVE(root));
      graphene_rect_t bounds;
      int w = 0, h = 0, x = 0, y = 0, scale = 1;
      if (gdk_surf)
        scale = gdk_surface_get_scale_factor(gdk_surf);
      if (scale < 1) scale = 1;
      /* Root widget size = window content (excludes GTK4's shadow area). */
      w = gtk_widget_get_width(root);
      h = gtk_widget_get_height(root);
      if (gtk_widget_compute_bounds(widget, root, &bounds)) {
        x = (int)bounds.origin.x;
        y = (int)bounds.origin.y;
      }
      if (x < 0) x = 0;
      if (y < 0) y = 0;
      if (w > x) *max_pw = (w - x) * scale;
      if (h > y) *max_ph = (h - y) * scale;
    }
  }
#else
  (void)ih; (void)gldata;
#endif
}

static void iupEGLBackendCleanup(Ihandle* ih, IGlControlData* gldata)
{
  GdkTexture* texture = (GdkTexture*)iupAttribGet(ih, "_IUPGL_COMPOSITE_TEXTURE");
  if (texture)
  {
    iupAttribSet(ih, "_IUPGL_COMPOSITE_TEXTURE", NULL);
    g_object_unref(texture);
  }
  gldata->backend_handle = NULL;
}

static char* iupEGLBackendGetVisual(Ihandle* ih)
{
  GdkDisplay* display;
  (void)ih;
  display = gdk_display_get_default();
  if (display) {
    return (char*)display;
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

static void iupEGLBackendQueueComposite(Ihandle* ih, IGlControlData* gldata)
{
  (void)gldata;
  if (!iupAttribGet(ih, "_IUPGL_IN_DRAW") && ih->handle && GTK_IS_WIDGET(ih->handle))
  {
    iupAttribSet(ih, "_IUPGL_COMPOSITE_FRESH", "1");
    gtk_widget_queue_draw((GtkWidget*)ih->handle);
  }
}

#if GTK_CHECK_VERSION(4, 14, 0)
#define IUP_EGL_HAS_PRESENT_DMABUF

#define IUP_DRM_FORMAT_ABGR8888 0x34324241
#define IUP_DRM_FORMAT_XBGR8888 0x34324258
#define IUP_DRM_FORMAT_ARGB8888 0x34325241
#define IUP_DRM_FORMAT_XRGB8888 0x34325258

typedef struct _IGtk4DmabufFds
{
  int count;
  int fds[4];
} IGtk4DmabufFds;

static void eGLCanvasGtk4ReleaseDmabuf(gpointer data)
{
  IGtk4DmabufFds* fds = (IGtk4DmabufFds*)data;
  int i;
  for (i = 0; i < fds->count; i++)
    close(fds->fds[i]);
  g_free(fds);
}

static GdkTexture* eGLCanvasGtk4DmabufTexture(Ihandle* ih, const IGlDmabuf* dmabuf, int width, int height)
{
  GdkDisplay* display;
  GdkDmabufFormats* formats;
  GdkDmabufTextureBuilder* builder;
  GdkTexture* texture;
  IGtk4DmabufFds* fds;
  guint32 fourcc = (guint32)dmabuf->fourcc;
  int i;

  if (!ih->handle || !GTK_IS_WIDGET(ih->handle))
    return NULL;

  display = gtk_widget_get_display((GtkWidget*)ih->handle);
  formats = gdk_display_get_dmabuf_formats(display);

  if (fourcc == IUP_DRM_FORMAT_ABGR8888 && gdk_dmabuf_formats_contains(formats, IUP_DRM_FORMAT_XBGR8888, dmabuf->modifier))
    fourcc = IUP_DRM_FORMAT_XBGR8888;
  else if (fourcc == IUP_DRM_FORMAT_ARGB8888 && gdk_dmabuf_formats_contains(formats, IUP_DRM_FORMAT_XRGB8888, dmabuf->modifier))
    fourcc = IUP_DRM_FORMAT_XRGB8888;
  if (!gdk_dmabuf_formats_contains(formats, fourcc, dmabuf->modifier))
    return NULL;

  fds = g_new0(IGtk4DmabufFds, 1);
  builder = gdk_dmabuf_texture_builder_new();
  gdk_dmabuf_texture_builder_set_display(builder, display);
  gdk_dmabuf_texture_builder_set_width(builder, (unsigned)width);
  gdk_dmabuf_texture_builder_set_height(builder, (unsigned)height);
  gdk_dmabuf_texture_builder_set_fourcc(builder, fourcc);
  gdk_dmabuf_texture_builder_set_modifier(builder, dmabuf->modifier);
  gdk_dmabuf_texture_builder_set_n_planes(builder, (unsigned)dmabuf->planes);
  for (i = 0; i < dmabuf->planes; i++)
  {
    int fd = dup(dmabuf->fds[i]);
    if (fd < 0)
      break;
    fds->fds[fds->count++] = fd;
    gdk_dmabuf_texture_builder_set_fd(builder, (unsigned)i, fd);
    gdk_dmabuf_texture_builder_set_stride(builder, (unsigned)i, (unsigned)dmabuf->strides[i]);
    gdk_dmabuf_texture_builder_set_offset(builder, (unsigned)i, (unsigned)dmabuf->offsets[i]);
  }
  if (fds->count != dmabuf->planes)
  {
    g_object_unref(builder);
    eGLCanvasGtk4ReleaseDmabuf(fds);
    return NULL;
  }

  texture = gdk_dmabuf_texture_builder_build(builder, eGLCanvasGtk4ReleaseDmabuf, fds, NULL);
  g_object_unref(builder);
  if (!texture)
    eGLCanvasGtk4ReleaseDmabuf(fds);
  return texture;
}

static int iupEGLBackendPresentDmabuf(Ihandle* ih, IGlControlData* gldata, const IGlDmabuf* dmabuf, int width, int height)
{
  GdkTexture* texture = dmabuf ? eGLCanvasGtk4DmabufTexture(ih, dmabuf, width, height) : NULL;
  GdkTexture* old = (GdkTexture*)iupAttribGet(ih, "_IUPGL_COMPOSITE_TEXTURE");
  iupAttribSet(ih, "_IUPGL_COMPOSITE_TEXTURE", (char*)texture);
  if (old)
    g_object_unref(old);
  if (!texture)
    return 0;

  iupEGLBackendQueueComposite(ih, gldata);
  return 1;
}

/* pixels: the same frame read back, BGRA, top row first. Returns 0 when the frame is one flat color and cannot tell. */
static int iupEGLBackendCheckDmabuf(Ihandle* ih, const IGlDmabuf* dmabuf, int width, int height, const unsigned char* pixels)
{
  GdkTexture* texture;
  unsigned char* data;
  size_t stride = (size_t)width * 4;
  int x, y, c, flat = 1, same = 1;
  int step_x = width / 64 + 1, step_y = height / 64 + 1;

  for (y = 0; y < height && flat; y += step_y)
  {
    for (x = 0; x < width && flat; x += step_x)
      flat = memcmp(pixels + (size_t)y * stride + (size_t)x * 4, pixels, 3) == 0;
  }
  if (flat)
    return 0;

  texture = eGLCanvasGtk4DmabufTexture(ih, dmabuf, width, height);
  if (!texture)
    return -1;

  data = (unsigned char*)calloc((size_t)height, stride);
  if (!data)
  {
    g_object_unref(texture);
    return -1;
  }
  gdk_texture_download(texture, data, stride);
  g_object_unref(texture);

  for (y = 0; y < height && same; y += step_y)
  {
    const unsigned char* row = data + (size_t)(height - 1 - y) * stride;
    for (x = 0; x < width && same; x += step_x)
    {
      for (c = 0; c < 3; c++)
      {
        int d = row[x * 4 + c] - pixels[(size_t)y * stride + (size_t)x * 4 + c];
        if (d < -2 || d > 2)
          same = 0;
      }
    }
  }
  free(data);
  return same ? 1 : -1;
}
#endif

#endif
