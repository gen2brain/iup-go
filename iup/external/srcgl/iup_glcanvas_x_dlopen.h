/** \file
 * \brief GLX and OpenGL Dynamic Loading Support
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_GLCANVAS_X_DLOPEN_H
#define __IUP_GLCANVAS_X_DLOPEN_H

#include <dlfcn.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifndef GLX_H

typedef struct __GLXcontextRec* GLXContext;
typedef struct __GLXFBConfigRec* GLXFBConfig;
typedef XID GLXDrawable;

#define GLX_BUFFER_SIZE 2
#define GLX_RGBA 4
#define GLX_DOUBLEBUFFER 5
#define GLX_STEREO 6
#define GLX_RED_SIZE 8
#define GLX_GREEN_SIZE 9
#define GLX_BLUE_SIZE 10
#define GLX_ALPHA_SIZE 11
#define GLX_DEPTH_SIZE 12
#define GLX_STENCIL_SIZE 13
#define GLX_ACCUM_RED_SIZE 14
#define GLX_ACCUM_GREEN_SIZE 15
#define GLX_ACCUM_BLUE_SIZE 16
#define GLX_ACCUM_ALPHA_SIZE 17
#define GLX_VISUAL_ID 0x800B
#define GLX_SAMPLE_BUFFERS 100000
#define GLX_SAMPLES 100001

#endif

#ifndef __gl_h_

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;

#define GL_TRUE 1
#define GL_TEXTURE_2D 0x0DE1
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_UNSIGNED_BYTE 0x1401
#define GL_RGBA 0x1908
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_NEAREST 0x2600
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_RGBA8 0x8058
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_DEPTH24_STENCIL8 0x88F0
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5

#endif

static Bool (*iupglx_QueryExtension)(Display*, int*, int*);
static XVisualInfo* (*iupglx_ChooseVisual)(Display*, int, int*);
static GLXContext (*iupglx_CreateContext)(Display*, XVisualInfo*, GLXContext, Bool);
static void (*iupglx_DestroyContext)(Display*, GLXContext);
static Bool (*iupglx_MakeCurrent)(Display*, GLXDrawable, GLXContext);
static GLXContext (*iupglx_GetCurrentContext)(void);
static Display* (*iupglx_GetCurrentDisplay)(void);
static GLXDrawable (*iupglx_GetCurrentDrawable)(void);
static GLXFBConfig* (*iupglx_GetFBConfigs)(Display*, int, int*);
static int (*iupglx_GetFBConfigAttrib)(Display*, GLXFBConfig, int, int*);
static void (*(*iupglx_GetProcAddressARB)(const GLubyte*))(void);
static void (*iupglx_SwapBuffers)(Display*, GLXDrawable);
static void (*iupglx_UseXFont)(Font, int, int, int);
static void (*iupglx_WaitGL)(void);
static void (*iupglx_WaitX)(void);

static void (*iupgl_BindTexture)(GLenum, GLuint);
static void (*iupgl_DeleteTextures)(GLsizei, const GLuint*);
static void (*iupgl_GenTextures)(GLsizei, GLuint*);
static const GLubyte* (*iupgl_GetString)(GLenum);
static void (*iupgl_PixelStorei)(GLenum, GLint);
static void (*iupgl_ReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*);
static void (*iupgl_TexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
static void (*iupgl_TexParameteri)(GLenum, GLenum, GLint);
static void (*iupgl_Viewport)(GLint, GLint, GLsizei, GLsizei);

#define glXQueryExtension iupglx_QueryExtension
#define glXChooseVisual iupglx_ChooseVisual
#define glXCreateContext iupglx_CreateContext
#define glXDestroyContext iupglx_DestroyContext
#define glXMakeCurrent iupglx_MakeCurrent
#define glXGetCurrentContext iupglx_GetCurrentContext
#define glXGetCurrentDisplay iupglx_GetCurrentDisplay
#define glXGetCurrentDrawable iupglx_GetCurrentDrawable
#define glXGetFBConfigs iupglx_GetFBConfigs
#define glXGetFBConfigAttrib iupglx_GetFBConfigAttrib
#define glXGetProcAddressARB iupglx_GetProcAddressARB
#define glXGetProcAddress iupglx_GetProcAddressARB
#define glXSwapBuffers iupglx_SwapBuffers
#define glXUseXFont iupglx_UseXFont
#define glXWaitGL iupglx_WaitGL
#define glXWaitX iupglx_WaitX

#define glBindTexture iupgl_BindTexture
#define glDeleteTextures iupgl_DeleteTextures
#define glGenTextures iupgl_GenTextures
#define glGetString iupgl_GetString
#define glPixelStorei iupgl_PixelStorei
#define glReadPixels iupgl_ReadPixels
#define glTexImage2D iupgl_TexImage2D
#define glTexParameteri iupgl_TexParameteri
#define glViewport iupgl_Viewport

static void* iupGLXSymbol(void* library, const char* name, int* missing)
{
  void* symbol = dlsym(library, name);
  if (!symbol)
    *missing = 1;
  return symbol;
}

static int iupGLXOpen(void)
{
  static int state = 0;
  void* gl;
  int missing = 0;

  if (state)
    return state > 0;
  state = -1;

  gl = dlopen("libGL.so.1", RTLD_NOW | RTLD_GLOBAL);
  if (!gl)
    return 0;

  *(void**)&iupglx_QueryExtension = iupGLXSymbol(gl, "glXQueryExtension", &missing);
  *(void**)&iupglx_ChooseVisual = iupGLXSymbol(gl, "glXChooseVisual", &missing);
  *(void**)&iupglx_CreateContext = iupGLXSymbol(gl, "glXCreateContext", &missing);
  *(void**)&iupglx_DestroyContext = iupGLXSymbol(gl, "glXDestroyContext", &missing);
  *(void**)&iupglx_MakeCurrent = iupGLXSymbol(gl, "glXMakeCurrent", &missing);
  *(void**)&iupglx_GetCurrentContext = iupGLXSymbol(gl, "glXGetCurrentContext", &missing);
  *(void**)&iupglx_GetCurrentDisplay = iupGLXSymbol(gl, "glXGetCurrentDisplay", &missing);
  *(void**)&iupglx_GetCurrentDrawable = iupGLXSymbol(gl, "glXGetCurrentDrawable", &missing);
  *(void**)&iupglx_GetFBConfigs = iupGLXSymbol(gl, "glXGetFBConfigs", &missing);
  *(void**)&iupglx_GetFBConfigAttrib = iupGLXSymbol(gl, "glXGetFBConfigAttrib", &missing);
  *(void**)&iupglx_GetProcAddressARB = iupGLXSymbol(gl, "glXGetProcAddressARB", &missing);
  *(void**)&iupglx_SwapBuffers = iupGLXSymbol(gl, "glXSwapBuffers", &missing);
  *(void**)&iupglx_UseXFont = iupGLXSymbol(gl, "glXUseXFont", &missing);
  *(void**)&iupglx_WaitGL = iupGLXSymbol(gl, "glXWaitGL", &missing);
  *(void**)&iupglx_WaitX = iupGLXSymbol(gl, "glXWaitX", &missing);

  *(void**)&iupgl_BindTexture = iupGLXSymbol(gl, "glBindTexture", &missing);
  *(void**)&iupgl_DeleteTextures = iupGLXSymbol(gl, "glDeleteTextures", &missing);
  *(void**)&iupgl_GenTextures = iupGLXSymbol(gl, "glGenTextures", &missing);
  *(void**)&iupgl_GetString = iupGLXSymbol(gl, "glGetString", &missing);
  *(void**)&iupgl_PixelStorei = iupGLXSymbol(gl, "glPixelStorei", &missing);
  *(void**)&iupgl_ReadPixels = iupGLXSymbol(gl, "glReadPixels", &missing);
  *(void**)&iupgl_TexImage2D = iupGLXSymbol(gl, "glTexImage2D", &missing);
  *(void**)&iupgl_TexParameteri = iupGLXSymbol(gl, "glTexParameteri", &missing);
  *(void**)&iupgl_Viewport = iupGLXSymbol(gl, "glViewport", &missing);

  if (missing)
    return 0;

  state = 1;
  return 1;
}

#endif
