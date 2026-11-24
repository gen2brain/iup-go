/** \file
 * \brief EGL, OpenGL and Wayland Dynamic Loading Support
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_GLCANVAS_EGL_DLOPEN_H
#define __IUP_GLCANVAS_EGL_DLOPEN_H

#include <stddef.h>
#include <stdint.h>
#include <dlfcn.h>

#ifndef __egl_h_

typedef void* EGLDisplay;
typedef void* EGLSurface;
typedef void* EGLContext;
typedef void* EGLConfig;
typedef void* EGLClientBuffer;
typedef void* EGLNativeDisplayType;
typedef unsigned long EGLNativeWindowType;
typedef unsigned int EGLBoolean;
typedef unsigned int EGLenum;
typedef int32_t EGLint;
typedef intptr_t EGLAttrib;

#define EGL_FALSE 0
#define EGL_TRUE 1
#define EGL_DEFAULT_DISPLAY ((EGLNativeDisplayType)0)
#define EGL_NO_CONTEXT ((EGLContext)0)
#define EGL_NO_DISPLAY ((EGLDisplay)0)
#define EGL_NO_SURFACE ((EGLSurface)0)

#define EGL_ALPHA_SIZE 0x3021
#define EGL_BLUE_SIZE 0x3022
#define EGL_GREEN_SIZE 0x3023
#define EGL_RED_SIZE 0x3024
#define EGL_DEPTH_SIZE 0x3025
#define EGL_STENCIL_SIZE 0x3026
#define EGL_NATIVE_VISUAL_ID 0x302E
#define EGL_SAMPLES 0x3031
#define EGL_SAMPLE_BUFFERS 0x3032
#define EGL_SURFACE_TYPE 0x3033
#define EGL_NONE 0x3038
#define EGL_COLOR_BUFFER_TYPE 0x303F
#define EGL_RENDERABLE_TYPE 0x3040
#define EGL_EXTENSIONS 0x3055
#define EGL_HEIGHT 0x3056
#define EGL_WIDTH 0x3057
#define EGL_DRAW 0x3059
#define EGL_READ 0x305A
#define EGL_CORE_NATIVE_ENGINE 0x305B
#define EGL_SINGLE_BUFFER 0x3085
#define EGL_RENDER_BUFFER 0x3086
#define EGL_RGB_BUFFER 0x308E
#define EGL_CONTEXT_CLIENT_VERSION 0x3098
#define EGL_OPENGL_API 0x30A2
#define EGL_PBUFFER_BIT 0x0001
#define EGL_WINDOW_BIT 0x0004
#define EGL_OPENGL_BIT 0x0008

#endif

#ifndef __eglext_h_

typedef void* EGLImageKHR;
typedef uint64_t EGLuint64KHR;

#define EGL_NO_IMAGE_KHR ((EGLImageKHR)0)
#define EGL_GL_TEXTURE_2D_KHR 0x30B1
#define EGL_LINUX_DMA_BUF_EXT 0x3270
#define EGL_LINUX_DRM_FOURCC_EXT 0x3271
#define EGL_DMA_BUF_PLANE0_FD_EXT 0x3272
#define EGL_DMA_BUF_PLANE0_OFFSET_EXT 0x3273
#define EGL_DMA_BUF_PLANE0_PITCH_EXT 0x3274
#define EGL_DMA_BUF_PLANE1_FD_EXT 0x3275
#define EGL_DMA_BUF_PLANE1_OFFSET_EXT 0x3276
#define EGL_DMA_BUF_PLANE1_PITCH_EXT 0x3277
#define EGL_DMA_BUF_PLANE2_FD_EXT 0x3278
#define EGL_DMA_BUF_PLANE2_OFFSET_EXT 0x3279
#define EGL_DMA_BUF_PLANE2_PITCH_EXT 0x327A
#define EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT 0x3443
#define EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT 0x3444
#define EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT 0x3445
#define EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT 0x3446
#define EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT 0x3447
#define EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT 0x3448

typedef EGLDisplay (*PFNEGLGETPLATFORMDISPLAYPROC)(EGLenum platform, void* native_display, const EGLAttrib* attrib_list);
typedef EGLImageKHR (*PFNEGLCREATEIMAGEKHRPROC)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint* attrib_list);
typedef EGLBoolean (*PFNEGLDESTROYIMAGEKHRPROC)(EGLDisplay dpy, EGLImageKHR image);
typedef EGLBoolean (*PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC)(EGLDisplay dpy, EGLImageKHR image, int* fourcc, int* num_planes, EGLuint64KHR* modifiers);
typedef EGLBoolean (*PFNEGLEXPORTDMABUFIMAGEMESAPROC)(EGLDisplay dpy, EGLImageKHR image, int* fds, EGLint* strides, EGLint* offsets);

#endif

#ifndef __gl_h_

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;

#define GL_TEXTURE_2D 0x0DE1
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_UNSIGNED_BYTE 0x1401
#define GL_RGBA 0x1908
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803

#endif

static EGLDisplay (*iupegl_GetDisplay)(EGLNativeDisplayType);
static EGLBoolean (*iupegl_Initialize)(EGLDisplay, EGLint*, EGLint*);
static const char* (*iupegl_QueryString)(EGLDisplay, EGLint);
static EGLint (*iupegl_GetError)(void);
static EGLBoolean (*iupegl_BindAPI)(EGLenum);
static EGLBoolean (*iupegl_ChooseConfig)(EGLDisplay, const EGLint*, EGLConfig*, EGLint, EGLint*);
static EGLBoolean (*iupegl_GetConfigAttrib)(EGLDisplay, EGLConfig, EGLint, EGLint*);
static EGLContext (*iupegl_CreateContext)(EGLDisplay, EGLConfig, EGLContext, const EGLint*);
static EGLBoolean (*iupegl_DestroyContext)(EGLDisplay, EGLContext);
static EGLSurface (*iupegl_CreateWindowSurface)(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint*);
static EGLSurface (*iupegl_CreatePbufferSurface)(EGLDisplay, EGLConfig, const EGLint*);
static EGLBoolean (*iupegl_DestroySurface)(EGLDisplay, EGLSurface);
static EGLBoolean (*iupegl_MakeCurrent)(EGLDisplay, EGLSurface, EGLSurface, EGLContext);
static EGLContext (*iupegl_GetCurrentContext)(void);
static EGLSurface (*iupegl_GetCurrentSurface)(EGLint);
static EGLDisplay (*iupegl_GetCurrentDisplay)(void);
static EGLBoolean (*iupegl_SwapBuffers)(EGLDisplay, EGLSurface);
static EGLBoolean (*iupegl_SwapInterval)(EGLDisplay, EGLint);
static EGLBoolean (*iupegl_WaitClient)(void);
static EGLBoolean (*iupegl_WaitNative)(EGLint);
static void (*(*iupegl_GetProcAddress)(const char*))(void);

static void (*iupgl_BindTexture)(GLenum, GLuint);
static void (*iupgl_CopyTexSubImage2D)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
static void (*iupgl_DeleteTextures)(GLsizei, const GLuint*);
static void (*iupgl_Flush)(void);
static void (*iupgl_GenTextures)(GLsizei, GLuint*);
static void (*iupgl_GetIntegerv)(GLenum, GLint*);
static const GLubyte* (*iupgl_GetString)(GLenum);
static void (*iupgl_PixelStorei)(GLenum, GLint);
static void (*iupgl_ReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*);
static void (*iupgl_TexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
static void (*iupgl_TexParameteri)(GLenum, GLenum, GLint);
static void (*iupgl_Viewport)(GLint, GLint, GLsizei, GLsizei);

#define eglGetDisplay iupegl_GetDisplay
#define eglInitialize iupegl_Initialize
#define eglQueryString iupegl_QueryString
#define eglGetError iupegl_GetError
#define eglBindAPI iupegl_BindAPI
#define eglChooseConfig iupegl_ChooseConfig
#define eglGetConfigAttrib iupegl_GetConfigAttrib
#define eglCreateContext iupegl_CreateContext
#define eglDestroyContext iupegl_DestroyContext
#define eglCreateWindowSurface iupegl_CreateWindowSurface
#define eglCreatePbufferSurface iupegl_CreatePbufferSurface
#define eglDestroySurface iupegl_DestroySurface
#define eglMakeCurrent iupegl_MakeCurrent
#define eglGetCurrentContext iupegl_GetCurrentContext
#define eglGetCurrentSurface iupegl_GetCurrentSurface
#define eglGetCurrentDisplay iupegl_GetCurrentDisplay
#define eglSwapBuffers iupegl_SwapBuffers
#define eglSwapInterval iupegl_SwapInterval
#define eglWaitClient iupegl_WaitClient
#define eglWaitNative iupegl_WaitNative
#define eglGetProcAddress iupegl_GetProcAddress

#define glBindTexture iupgl_BindTexture
#define glCopyTexSubImage2D iupgl_CopyTexSubImage2D
#define glDeleteTextures iupgl_DeleteTextures
#define glFlush iupgl_Flush
#define glGenTextures iupgl_GenTextures
#define glGetIntegerv iupgl_GetIntegerv
#define glGetString iupgl_GetString
#define glPixelStorei iupgl_PixelStorei
#define glReadPixels iupgl_ReadPixels
#define glTexImage2D iupgl_TexImage2D
#define glTexParameteri iupgl_TexParameteri
#define glViewport iupgl_Viewport

static void* iupEGLSymbol(void* library, const char* name, int* missing)
{
  void* symbol = dlsym(library, name);
  if (!symbol)
    *missing = 1;
  return symbol;
}

static int iupEGLOpen(void)
{
  static int state = 0;
  void* egl;
  void* gl;
  int missing = 0;

  if (state)
    return state > 0;
  state = -1;

  egl = dlopen("libEGL.so.1", RTLD_NOW | RTLD_GLOBAL);
  if (!egl)
    return 0;
  gl = dlopen("libGL.so.1", RTLD_NOW | RTLD_GLOBAL);
  if (!gl)
    gl = dlopen("libOpenGL.so.0", RTLD_NOW | RTLD_GLOBAL);
  if (!gl)
    return 0;

  *(void**)&iupegl_GetDisplay = iupEGLSymbol(egl, "eglGetDisplay", &missing);
  *(void**)&iupegl_Initialize = iupEGLSymbol(egl, "eglInitialize", &missing);
  *(void**)&iupegl_QueryString = iupEGLSymbol(egl, "eglQueryString", &missing);
  *(void**)&iupegl_GetError = iupEGLSymbol(egl, "eglGetError", &missing);
  *(void**)&iupegl_BindAPI = iupEGLSymbol(egl, "eglBindAPI", &missing);
  *(void**)&iupegl_ChooseConfig = iupEGLSymbol(egl, "eglChooseConfig", &missing);
  *(void**)&iupegl_GetConfigAttrib = iupEGLSymbol(egl, "eglGetConfigAttrib", &missing);
  *(void**)&iupegl_CreateContext = iupEGLSymbol(egl, "eglCreateContext", &missing);
  *(void**)&iupegl_DestroyContext = iupEGLSymbol(egl, "eglDestroyContext", &missing);
  *(void**)&iupegl_CreateWindowSurface = iupEGLSymbol(egl, "eglCreateWindowSurface", &missing);
  *(void**)&iupegl_CreatePbufferSurface = iupEGLSymbol(egl, "eglCreatePbufferSurface", &missing);
  *(void**)&iupegl_DestroySurface = iupEGLSymbol(egl, "eglDestroySurface", &missing);
  *(void**)&iupegl_MakeCurrent = iupEGLSymbol(egl, "eglMakeCurrent", &missing);
  *(void**)&iupegl_GetCurrentContext = iupEGLSymbol(egl, "eglGetCurrentContext", &missing);
  *(void**)&iupegl_GetCurrentSurface = iupEGLSymbol(egl, "eglGetCurrentSurface", &missing);
  *(void**)&iupegl_GetCurrentDisplay = iupEGLSymbol(egl, "eglGetCurrentDisplay", &missing);
  *(void**)&iupegl_SwapBuffers = iupEGLSymbol(egl, "eglSwapBuffers", &missing);
  *(void**)&iupegl_SwapInterval = iupEGLSymbol(egl, "eglSwapInterval", &missing);
  *(void**)&iupegl_WaitClient = iupEGLSymbol(egl, "eglWaitClient", &missing);
  *(void**)&iupegl_WaitNative = iupEGLSymbol(egl, "eglWaitNative", &missing);
  *(void**)&iupegl_GetProcAddress = iupEGLSymbol(egl, "eglGetProcAddress", &missing);

  *(void**)&iupgl_BindTexture = iupEGLSymbol(gl, "glBindTexture", &missing);
  *(void**)&iupgl_CopyTexSubImage2D = iupEGLSymbol(gl, "glCopyTexSubImage2D", &missing);
  *(void**)&iupgl_DeleteTextures = iupEGLSymbol(gl, "glDeleteTextures", &missing);
  *(void**)&iupgl_Flush = iupEGLSymbol(gl, "glFlush", &missing);
  *(void**)&iupgl_GenTextures = iupEGLSymbol(gl, "glGenTextures", &missing);
  *(void**)&iupgl_GetIntegerv = iupEGLSymbol(gl, "glGetIntegerv", &missing);
  *(void**)&iupgl_GetString = iupEGLSymbol(gl, "glGetString", &missing);
  *(void**)&iupgl_PixelStorei = iupEGLSymbol(gl, "glPixelStorei", &missing);
  *(void**)&iupgl_ReadPixels = iupEGLSymbol(gl, "glReadPixels", &missing);
  *(void**)&iupgl_TexImage2D = iupEGLSymbol(gl, "glTexImage2D", &missing);
  *(void**)&iupgl_TexParameteri = iupEGLSymbol(gl, "glTexParameteri", &missing);
  *(void**)&iupgl_Viewport = iupEGLSymbol(gl, "glViewport", &missing);

  if (missing)
    return 0;

  state = 1;
  return 1;
}

/* Wayland client and wayland-egl, only for the subsurface of a canvas without its own window */

struct wl_display;
struct wl_proxy;
struct wl_event_queue;
struct wl_interface;
struct wl_registry;
struct wl_compositor;
struct wl_subcompositor;
struct wl_surface;
struct wl_subsurface;
struct wl_egl_window;

struct iupwl_registry_listener
{
  void (*global)(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
  void (*global_remove)(void* data, struct wl_registry* registry, uint32_t name);
};

static struct wl_event_queue* (*iupwl_display_create_queue)(struct wl_display*);
static int (*iupwl_display_roundtrip_queue)(struct wl_display*, struct wl_event_queue*);
static int (*iupwl_display_dispatch_pending)(struct wl_display*);
static void (*iupwl_event_queue_destroy)(struct wl_event_queue*);
static void (*iupwl_proxy_set_queue)(struct wl_proxy*, struct wl_event_queue*);
static void (*iupwl_proxy_marshal)(struct wl_proxy*, uint32_t, ...);
static struct wl_proxy* (*iupwl_proxy_marshal_constructor)(struct wl_proxy*, uint32_t, const struct wl_interface*, ...);
static struct wl_proxy* (*iupwl_proxy_marshal_constructor_versioned)(struct wl_proxy*, uint32_t, const struct wl_interface*, uint32_t, ...);
static int (*iupwl_proxy_add_listener)(struct wl_proxy*, void (**)(void), void*);
static void (*iupwl_proxy_destroy)(struct wl_proxy*);
static const struct wl_interface* iupwl_registry_interface;
static const struct wl_interface* iupwl_compositor_interface;
static const struct wl_interface* iupwl_subcompositor_interface;
static const struct wl_interface* iupwl_surface_interface;
static const struct wl_interface* iupwl_subsurface_interface;

static struct wl_egl_window* (*iupwl_egl_window_create)(struct wl_surface*, int, int);
static void (*iupwl_egl_window_destroy)(struct wl_egl_window*);
static void (*iupwl_egl_window_resize)(struct wl_egl_window*, int, int, int, int);

static int iupWaylandOpen(void)
{
  static int state = 0;
  void* client;
  void* egl;
  int missing = 0;

  if (state)
    return state > 0;
  state = -1;

  client = dlopen("libwayland-client.so.0", RTLD_NOW | RTLD_GLOBAL);
  egl = dlopen("libwayland-egl.so.1", RTLD_NOW | RTLD_GLOBAL);
  if (!client || !egl)
    return 0;

  *(void**)&iupwl_display_create_queue = iupEGLSymbol(client, "wl_display_create_queue", &missing);
  *(void**)&iupwl_display_roundtrip_queue = iupEGLSymbol(client, "wl_display_roundtrip_queue", &missing);
  *(void**)&iupwl_display_dispatch_pending = iupEGLSymbol(client, "wl_display_dispatch_pending", &missing);
  *(void**)&iupwl_event_queue_destroy = iupEGLSymbol(client, "wl_event_queue_destroy", &missing);
  *(void**)&iupwl_proxy_set_queue = iupEGLSymbol(client, "wl_proxy_set_queue", &missing);
  *(void**)&iupwl_proxy_marshal = iupEGLSymbol(client, "wl_proxy_marshal", &missing);
  *(void**)&iupwl_proxy_marshal_constructor = iupEGLSymbol(client, "wl_proxy_marshal_constructor", &missing);
  *(void**)&iupwl_proxy_marshal_constructor_versioned = iupEGLSymbol(client, "wl_proxy_marshal_constructor_versioned", &missing);
  *(void**)&iupwl_proxy_add_listener = iupEGLSymbol(client, "wl_proxy_add_listener", &missing);
  *(void**)&iupwl_proxy_destroy = iupEGLSymbol(client, "wl_proxy_destroy", &missing);
  iupwl_registry_interface = (const struct wl_interface*)iupEGLSymbol(client, "wl_registry_interface", &missing);
  iupwl_compositor_interface = (const struct wl_interface*)iupEGLSymbol(client, "wl_compositor_interface", &missing);
  iupwl_subcompositor_interface = (const struct wl_interface*)iupEGLSymbol(client, "wl_subcompositor_interface", &missing);
  iupwl_surface_interface = (const struct wl_interface*)iupEGLSymbol(client, "wl_surface_interface", &missing);
  iupwl_subsurface_interface = (const struct wl_interface*)iupEGLSymbol(client, "wl_subsurface_interface", &missing);

  *(void**)&iupwl_egl_window_create = iupEGLSymbol(egl, "wl_egl_window_create", &missing);
  *(void**)&iupwl_egl_window_destroy = iupEGLSymbol(egl, "wl_egl_window_destroy", &missing);
  *(void**)&iupwl_egl_window_resize = iupEGLSymbol(egl, "wl_egl_window_resize", &missing);

  if (missing)
    return 0;

  state = 1;
  return 1;
}

static struct wl_registry* iupwl_display_get_registry(struct wl_display* display)
{
  return (struct wl_registry*)iupwl_proxy_marshal_constructor((struct wl_proxy*)display, 1, iupwl_registry_interface, NULL);
}

static int iupwl_registry_add_listener(struct wl_registry* registry, const struct iupwl_registry_listener* listener, void* data)
{
  return iupwl_proxy_add_listener((struct wl_proxy*)registry, (void (**)(void))listener, data);
}

static void* iupwl_registry_bind(struct wl_registry* registry, uint32_t name, const struct wl_interface* interface, uint32_t version)
{
  const char* interface_name = *(const char* const*)interface;
  return iupwl_proxy_marshal_constructor_versioned((struct wl_proxy*)registry, 0, interface, version, name, interface_name, version, NULL);
}

static void iupwl_registry_destroy(struct wl_registry* registry)
{
  iupwl_proxy_destroy((struct wl_proxy*)registry);
}

static struct wl_surface* iupwl_compositor_create_surface(struct wl_compositor* compositor)
{
  return (struct wl_surface*)iupwl_proxy_marshal_constructor((struct wl_proxy*)compositor, 0, iupwl_surface_interface, NULL);
}

static void iupwl_compositor_destroy(struct wl_compositor* compositor)
{
  iupwl_proxy_destroy((struct wl_proxy*)compositor);
}

static struct wl_subsurface* iupwl_subcompositor_get_subsurface(struct wl_subcompositor* subcompositor, struct wl_surface* surface, struct wl_surface* parent)
{
  return (struct wl_subsurface*)iupwl_proxy_marshal_constructor((struct wl_proxy*)subcompositor, 1, iupwl_subsurface_interface, NULL, surface, parent);
}

static void iupwl_subcompositor_destroy(struct wl_subcompositor* subcompositor)
{
  iupwl_proxy_marshal((struct wl_proxy*)subcompositor, 0);
  iupwl_proxy_destroy((struct wl_proxy*)subcompositor);
}

static void iupwl_surface_destroy(struct wl_surface* surface)
{
  iupwl_proxy_marshal((struct wl_proxy*)surface, 0);
  iupwl_proxy_destroy((struct wl_proxy*)surface);
}

static void iupwl_subsurface_destroy(struct wl_subsurface* subsurface)
{
  iupwl_proxy_marshal((struct wl_proxy*)subsurface, 0);
  iupwl_proxy_destroy((struct wl_proxy*)subsurface);
}

static void iupwl_subsurface_set_position(struct wl_subsurface* subsurface, int32_t x, int32_t y)
{
  iupwl_proxy_marshal((struct wl_proxy*)subsurface, 1, x, y);
}

static void iupwl_subsurface_set_desync(struct wl_subsurface* subsurface)
{
  iupwl_proxy_marshal((struct wl_proxy*)subsurface, 5);
}

#endif
