/** \file
 * \brief iupgl control for Windows
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <GL/gl.h>

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER                0x8D40
#define GL_RENDERBUFFER               0x8D41
#define GL_COLOR_ATTACHMENT0          0x8CE0
#define GL_DEPTH_STENCIL_ATTACHMENT   0x821A
#define GL_DEPTH24_STENCIL8           0x88F0
#define GL_FRAMEBUFFER_COMPLETE       0x8CD5
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupgl.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"

#include "iup_glcanvas_nativeinfo.h"


typedef HGLRC (WINAPI* wglCreateContextAttribsARB_PROC) (HDC hDC, HGLRC hShareContext, const int* attribList);

#ifndef WGL_CONTEXT_MAJOR_VERSION_ARB
#define WGL_CONTEXT_MAJOR_VERSION_ARB  0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB  0x2092
#define WGL_CONTEXT_FLAGS_ARB          0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB   0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB      0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#endif

#ifndef ERROR_INVALID_VERSION_ARB
#define ERROR_INVALID_VERSION_ARB   0x2095
#define ERROR_INVALID_PROFILE_ARB   0x2096
#endif

/* Do NOT use _IcontrolData to make inheritance easy
   when parent class is glcanvas */
typedef struct _IGlControlData
{
  HWND window;
  HDC device;
  HGLRC context;
  HPALETTE palette;
  int is_owned_dc;
  int owns_window;
  int lazy_init;

  GLuint fbo, color_tex, depth_rbo;
  int fbo_w, fbo_h;
  unsigned char* composite_pixels;
  size_t composite_cap;
} IGlControlData;

typedef void (*IFnComposite)(Ihandle* ih, const unsigned char* bgra, int w, int h);

typedef void (APIENTRY* wglGenFramebuffers_PROC)(GLsizei, GLuint*);
typedef void (APIENTRY* wglBindFramebuffer_PROC)(GLenum, GLuint);
typedef void (APIENTRY* wglDeleteFramebuffers_PROC)(GLsizei, const GLuint*);
typedef void (APIENTRY* wglFramebufferTexture2D_PROC)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef GLenum (APIENTRY* wglCheckFramebufferStatus_PROC)(GLenum);
typedef void (APIENTRY* wglGenRenderbuffers_PROC)(GLsizei, GLuint*);
typedef void (APIENTRY* wglBindRenderbuffer_PROC)(GLenum, GLuint);
typedef void (APIENTRY* wglDeleteRenderbuffers_PROC)(GLsizei, const GLuint*);
typedef void (APIENTRY* wglRenderbufferStorage_PROC)(GLenum, GLenum, GLsizei, GLsizei);
typedef void (APIENTRY* wglFramebufferRenderbuffer_PROC)(GLenum, GLenum, GLenum, GLuint);

static wglGenFramebuffers_PROC         wGLGenFramebuffers;
static wglBindFramebuffer_PROC         wGLBindFramebuffer;
static wglDeleteFramebuffers_PROC      wGLDeleteFramebuffers;
static wglFramebufferTexture2D_PROC    wGLFramebufferTexture2D;
static wglCheckFramebufferStatus_PROC  wGLCheckFramebufferStatus;
static wglGenRenderbuffers_PROC        wGLGenRenderbuffers;
static wglBindRenderbuffer_PROC        wGLBindRenderbuffer;
static wglDeleteRenderbuffers_PROC     wGLDeleteRenderbuffers;
static wglRenderbufferStorage_PROC     wGLRenderbufferStorage;
static wglFramebufferRenderbuffer_PROC wGLFramebufferRenderbuffer;

static int wGLLoadFBOProcs(void)
{
  if (wGLGenFramebuffers)
    return 1;
  wGLGenFramebuffers         = (wglGenFramebuffers_PROC)wglGetProcAddress("glGenFramebuffers");
  wGLBindFramebuffer         = (wglBindFramebuffer_PROC)wglGetProcAddress("glBindFramebuffer");
  wGLDeleteFramebuffers      = (wglDeleteFramebuffers_PROC)wglGetProcAddress("glDeleteFramebuffers");
  wGLFramebufferTexture2D    = (wglFramebufferTexture2D_PROC)wglGetProcAddress("glFramebufferTexture2D");
  wGLCheckFramebufferStatus  = (wglCheckFramebufferStatus_PROC)wglGetProcAddress("glCheckFramebufferStatus");
  wGLGenRenderbuffers        = (wglGenRenderbuffers_PROC)wglGetProcAddress("glGenRenderbuffers");
  wGLBindRenderbuffer        = (wglBindRenderbuffer_PROC)wglGetProcAddress("glBindRenderbuffer");
  wGLDeleteRenderbuffers     = (wglDeleteRenderbuffers_PROC)wglGetProcAddress("glDeleteRenderbuffers");
  wGLRenderbufferStorage     = (wglRenderbufferStorage_PROC)wglGetProcAddress("glRenderbufferStorage");
  wGLFramebufferRenderbuffer = (wglFramebufferRenderbuffer_PROC)wglGetProcAddress("glFramebufferRenderbuffer");
  return wGLGenFramebuffers && wGLBindFramebuffer && wGLFramebufferTexture2D && wGLCheckFramebufferStatus &&
         wGLGenRenderbuffers && wGLBindRenderbuffer && wGLRenderbufferStorage && wGLFramebufferRenderbuffer;
}

static int wGLCompositeEnsureFBO(IGlControlData* gldata, int w, int h)
{
  if (w < 1) w = 1;
  if (h < 1) h = 1;

  if (!wGLLoadFBOProcs())
    return 0;

  if (gldata->fbo && gldata->fbo_w == w && gldata->fbo_h == h)
  {
    wGLBindFramebuffer(GL_FRAMEBUFFER, gldata->fbo);
    return 1;
  }

  if (!gldata->fbo)       wGLGenFramebuffers(1, &gldata->fbo);
  if (!gldata->color_tex) glGenTextures(1, &gldata->color_tex);
  if (!gldata->depth_rbo) wGLGenRenderbuffers(1, &gldata->depth_rbo);

  glBindTexture(GL_TEXTURE_2D, gldata->color_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  wGLBindRenderbuffer(GL_RENDERBUFFER, gldata->depth_rbo);
  wGLRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
  wGLBindRenderbuffer(GL_RENDERBUFFER, 0);

  wGLBindFramebuffer(GL_FRAMEBUFFER, gldata->fbo);
  wGLFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gldata->color_tex, 0);
  wGLFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gldata->depth_rbo);

  if (wGLCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    wGLBindFramebuffer(GL_FRAMEBUFFER, 0);
    return 0;
  }

  gldata->fbo_w = w;
  gldata->fbo_h = h;
  return 1;
}

static void wGLCompositeRelease(IGlControlData* gldata)
{
  if (gldata->fbo && wGLDeleteFramebuffers) wGLDeleteFramebuffers(1, &gldata->fbo);
  if (gldata->depth_rbo && wGLDeleteRenderbuffers) wGLDeleteRenderbuffers(1, &gldata->depth_rbo);
  if (gldata->color_tex) glDeleteTextures(1, &gldata->color_tex);
  free(gldata->composite_pixels);
  gldata->fbo = gldata->depth_rbo = gldata->color_tex = 0;
  gldata->composite_pixels = NULL;
  gldata->composite_cap = 0;
}

static void wGLCompositeReadbackFBO(Ihandle* ih, IGlControlData* gldata, IFnComposite cb)
{
  int w = gldata->fbo_w, h = gldata->fbo_h, y;
  size_t rowbytes = (size_t)w * 4, need = rowbytes * h, i;
  unsigned char* px;

  if (!gldata->fbo || w < 1 || h < 1)
    return;

  if (gldata->composite_cap < need)
  {
    unsigned char* nb = (unsigned char*)realloc(gldata->composite_pixels, need);
    if (!nb)
      return;
    gldata->composite_pixels = nb;
    gldata->composite_cap = need;
  }
  px = gldata->composite_pixels;

  wGLBindFramebuffer(GL_FRAMEBUFFER, gldata->fbo);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, px);

  for (y = 0; y < h / 2; y++)
  {
    unsigned char* a = px + (size_t)y * rowbytes;
    unsigned char* b = px + (size_t)(h - 1 - y) * rowbytes;
    for (i = 0; i < rowbytes; i++)
    {
      unsigned char t = a[i];
      a[i] = b[i];
      b[i] = t;
    }
  }
  for (i = 0; i < (size_t)w * h; i++)
    px[i * 4 + 3] = 255;

  cb(ih, px, w, h);
}

static int wGLCreateContext(Ihandle* ih, IGlControlData* gldata);

static int wGLCanvasDefaultResize_CB(Ihandle* ih, int width, int height)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (gldata && gldata->lazy_init)
    return IUP_DEFAULT;

  IupGLMakeCurrent(ih);
  glViewport(0,0,width,height);
  return IUP_DEFAULT;
}

static int wGLCanvasCreateMethod(Ihandle* ih, void** params)
{
  IGlControlData* gldata;
  (void)params;

  gldata = (IGlControlData*)malloc(sizeof(IGlControlData));
  memset(gldata, 0, sizeof(IGlControlData));
  iupAttribSet(ih, "_IUP_GLCONTROLDATA", (char*)gldata);

  IupSetCallback(ih, "RESIZE_CB", (Icallback)wGLCanvasDefaultResize_CB);

  return IUP_NOERROR;
}

static int wGLCreateContext(Ihandle* ih, IGlControlData* gldata)
{
  Ihandle* ih_shared;
  HGLRC shared_context = NULL;
  int number;
  int isIndex = 0;
  int pixelFormat;
  PIXELFORMATDESCRIPTOR test_pfd;
  PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR),  /*  size of this pfd   */
      1,                     /* version number             */
      PFD_DRAW_TO_WINDOW |   /* support window             */
      PFD_SUPPORT_OPENGL,    /* support OpenGL             */
      PFD_TYPE_RGBA,         /* RGBA type                  */
      24,                    /* 24-bit color depth         */
      0, 0, 0, 0, 0, 0,      /* color bits ignored         */
      0,                     /* no alpha buffer            */
      0,                     /* shift bit ignored          */
      0,                     /* no accumulation buffer     */
      0, 0, 0, 0,            /* accum bits ignored         */
      16,                    /* 32-bit z-buffer             */
      0,                     /* no stencil buffer          */
      0,                     /* no auxiliary buffer        */
      PFD_MAIN_PLANE,        /* main layer                 */
      0,                     /* reserved                   */
      0, 0, 0                /* layer masks ignored        */
  };

  /* the IupCanvas is already mapped, just initialize the OpenGL context */

  /* double or single buffer */
  if (iupStrEqualNoCase(iupAttribGetStr(ih,"BUFFER"), "DOUBLE"))
    pfd.dwFlags |= PFD_DOUBLEBUFFER;

  /* stereo */
  if (iupAttribGetBoolean(ih,"STEREO"))
    pfd.dwFlags |= PFD_STEREO;

  /* rgba or index */
  if (iupStrEqualNoCase(iupAttribGetStr(ih,"COLOR"), "INDEX"))
  {
    isIndex = 1;
    pfd.iPixelType = PFD_TYPE_COLORINDEX;
    pfd.cColorBits = 8;  /* assume 8 bits when indexed */
    number = iupAttribGetInt(ih,"BUFFER_SIZE");
    if (number > 0) pfd.cColorBits = (BYTE)number;
  }

  /* red, green, blue bits */
  number = iupAttribGetInt(ih,"RED_SIZE");
  if (number > 0) pfd.cRedBits = (BYTE)number;
  pfd.cRedShift = 0;

  number = iupAttribGetInt(ih,"GREEN_SIZE");
  if (number > 0) pfd.cGreenBits = (BYTE)number;
  pfd.cGreenShift = pfd.cRedBits;

  number = iupAttribGetInt(ih,"BLUE_SIZE");
  if (number > 0) pfd.cBlueBits = (BYTE)number;
  pfd.cBlueShift = pfd.cRedBits + pfd.cGreenBits;

  number = iupAttribGetInt(ih,"ALPHA_SIZE");
  if (number > 0) pfd.cAlphaBits = (BYTE)number;
  pfd.cAlphaShift = pfd.cRedBits + pfd.cGreenBits + pfd.cBlueBits;

  /* depth and stencil size */
  number = iupAttribGetInt(ih,"DEPTH_SIZE");
  if (number > 0) pfd.cDepthBits = (BYTE)number;

  /* stencil */
  number = iupAttribGetInt(ih,"STENCIL_SIZE");
  if (number > 0) pfd.cStencilBits = (BYTE)number;

  /* red, green, blue accumulation bits */
  number = iupAttribGetInt(ih,"ACCUM_RED_SIZE");
  if (number > 0) pfd.cAccumRedBits = (BYTE)number;

  number = iupAttribGetInt(ih,"ACCUM_GREEN_SIZE");
  if (number > 0) pfd.cAccumGreenBits = (BYTE)number;

  number = iupAttribGetInt(ih,"ACCUM_BLUE_SIZE");
  if (number > 0) pfd.cAccumBlueBits = (BYTE)number;

  number = iupAttribGetInt(ih,"ACCUM_ALPHA_SIZE");
  if (number > 0) pfd.cAccumAlphaBits = (BYTE)number;

  pfd.cAccumBits = pfd.cAccumRedBits + pfd.cAccumGreenBits + pfd.cAccumBlueBits + pfd.cAccumAlphaBits;

  /* get a device context */
  {
    LONG style = GetClassLong(gldata->window, GCL_STYLE);
    gldata->is_owned_dc = (style & CS_OWNDC) || (style & CS_CLASSDC);
  }

  gldata->device = GetDC(gldata->window);
  iupAttribSet(ih, "VISUAL", (char*)gldata->device);

  /* choose pixel format */
  pixelFormat = ChoosePixelFormat(gldata->device, &pfd);
  if (pixelFormat == 0)
  {
    iupAttribSet(ih, "ERROR", "No appropriate pixel format.");
    iupAttribSetStr(ih, "LASTERROR", IupGetGlobal("LASTERROR"));
    return IUP_NOERROR;
  }
  SetPixelFormat(gldata->device,pixelFormat,&pfd);

  ih_shared = IupGetAttributeHandle(ih, "SHAREDCONTEXT");
  if (ih_shared && IupClassMatch(ih_shared, "glcanvas"))  /* must be an IupGLCanvas */
  {
    IGlControlData* shared_gldata = (IGlControlData*)iupAttribGet(ih_shared, "_IUP_GLCONTROLDATA");
    shared_context = shared_gldata->context;
  }

  /* create rendering context */
  if (iupAttribGetBoolean(ih, "ARBCONTEXT"))
  {
    wglCreateContextAttribsARB_PROC CreateContextAttribsARB;
    HGLRC tempContext = wglCreateContext(gldata->device);
    HGLRC oldContext = wglGetCurrentContext();
    HDC oldDC = wglGetCurrentDC();
    wglMakeCurrent(gldata->device, tempContext);   /* wglGetProcAddress only works with an active context */

    CreateContextAttribsARB = (wglCreateContextAttribsARB_PROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if (CreateContextAttribsARB)
    {
      int attribs[9], a = 0;
      char* value;

      value = iupAttribGetStr(ih, "CONTEXTVERSION");
      if (value)
      {
        int major, minor;
        if (iupStrToIntInt(value, &major, &minor, '.') == 2)
        {
          attribs[a++] = WGL_CONTEXT_MAJOR_VERSION_ARB;
          attribs[a++] = major;
          attribs[a++] = WGL_CONTEXT_MINOR_VERSION_ARB;
          attribs[a++] = minor;
        }
      }

      value = iupAttribGetStr(ih, "CONTEXTFLAGS");
      if (value)
      {
        int flags = 0;
        if (iupStrEqualNoCase(value, "DEBUG"))
          flags = WGL_CONTEXT_DEBUG_BIT_ARB;
        else if (iupStrEqualNoCase(value, "FORWARDCOMPATIBLE"))
          flags = WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
        else if (iupStrEqualNoCase(value, "DEBUGFORWARDCOMPATIBLE"))
          flags = WGL_CONTEXT_DEBUG_BIT_ARB|WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
        if (flags)
        {
          attribs[a++] = WGL_CONTEXT_FLAGS_ARB;
          attribs[a++] = flags;
        }
      }

      value = iupAttribGetStr(ih, "CONTEXTPROFILE");
      if (value)
      {
        int profile = 0;
        if (iupStrEqualNoCase(value, "CORE"))
          profile = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
        else if (iupStrEqualNoCase(value, "COMPATIBILITY"))
          profile = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
        else if (iupStrEqualNoCase(value, "CORECOMPATIBILITY"))
          profile = WGL_CONTEXT_CORE_PROFILE_BIT_ARB|WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
        if (profile)
        {
          attribs[a++] = WGL_CONTEXT_PROFILE_MASK_ARB;
          attribs[a++] = profile;
        }
      }

      attribs[a] = 0; /* terminator */

      gldata->context = CreateContextAttribsARB(gldata->device, shared_context, attribs);
      if (!gldata->context)
      {
        DWORD error = GetLastError();
        if (error == ERROR_INVALID_VERSION_ARB)
          iupAttribSetStr(ih, "LASTERROR", "Invalid ARB Version");
        else if (error == ERROR_INVALID_PROFILE_ARB)
          iupAttribSetStr(ih, "LASTERROR", "Invalid ARGB Profile");
        else
          iupAttribSetStr(ih, "LASTERROR", IupGetGlobal("LASTERROR"));

        iupAttribSet(ih, "ERROR", "Could not create a rendering context.");

        wglMakeCurrent(oldDC, oldContext);
        wglDeleteContext(tempContext);

        return IUP_NOERROR;
      }
    }

    wglMakeCurrent(oldDC, oldContext);
    wglDeleteContext(tempContext);

    if (!CreateContextAttribsARB)
    {
      gldata->context = wglCreateContext(gldata->device);
      iupAttribSet(ih, "ARBCONTEXT", "NO");
    }
  }
  else
    gldata->context = wglCreateContext(gldata->device);

  if (!gldata->context)
  {
    iupAttribSet(ih, "ERROR", "Could not create a rendering context.");
    iupAttribSetStr(ih, "LASTERROR", IupGetGlobal("LASTERROR"));
    return IUP_NOERROR;
  }

  iupAttribSet(ih, "CONTEXT", (char*)gldata->context);

  if (shared_context)
    wglShareLists(shared_context, gldata->context);

  /* create colormap for index mode */
  if (isIndex)
  {
    if (!gldata->palette)
    {
      LOGPALETTE lp = {0x300,1,{255,255,255,PC_NOCOLLAPSE}};  /* set first color as white */
      gldata->palette = CreatePalette(&lp);
      ResizePalette(gldata->palette,1<<pfd.cColorBits);
      iupAttribSet(ih, "COLORMAP", (char*)gldata->palette);
    }

    SelectPalette(gldata->device,gldata->palette,FALSE);
    RealizePalette(gldata->device);
  }

  DescribePixelFormat(gldata->device, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &test_pfd);
  if ((pfd.dwFlags & PFD_STEREO) && !(test_pfd.dwFlags & PFD_STEREO))
  {
    iupAttribSet(ih, "STEREO", "NO");
    return IUP_NOERROR;
  }

  iupAttribSet(ih, "ERROR", NULL);
  return IUP_NOERROR;
}

static int wGLCanvasMapMethod(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  IGlNativeInfo info;

  if (iupGLGetNativeInfo(ih, &info))
  {
    if (info.has_own_window)
    {
      gldata->window = (HWND)info.canvas_window;
      gldata->owns_window = 0;
    }
    else if (info.parent_window)
    {
      const char* driver = IupGetGlobal("DRIVER");

      if (driver && strcmp(driver, "GTK4") == 0)
      {
        gldata->window = (HWND)info.parent_window;
        gldata->owns_window = 0;
      }
      else
      {
        gldata->window = iupGLCreateChildWindow(info.parent_window, info.x, info.y, info.w, info.h);
        gldata->owns_window = 1;
      }
    }
    else
    {
      gldata->lazy_init = 1;
      return IUP_NOERROR;
    }
  }
  else
  {
    gldata->window = (HWND)iupAttribGet(ih, "HWND");
    if (!gldata->window)
      gldata->window = (HWND)IupGetAttribute(ih, "HWND");
  }

  if (!gldata->window)
  {
    gldata->lazy_init = 1;
    return IUP_NOERROR;
  }

  {
    LONG style = GetClassLong(gldata->window, GCL_STYLE);
    gldata->is_owned_dc = (style & CS_OWNDC) || (style & CS_CLASSDC);
  }

  return wGLCreateContext(ih, gldata);
}

static int wGLCanvasLazyInit(Ihandle* ih, IGlControlData* gldata)
{
  IGlNativeInfo info;

  if (!iupGLGetNativeInfo(ih, &info))
    return 0;

  if (info.has_own_window)
  {
    gldata->window = (HWND)info.canvas_window;
    gldata->owns_window = 0;
  }
  else
  {
    const char* driver = IupGetGlobal("DRIVER");
    int use_child = 1;

    if (driver && strcmp(driver, "GTK4") == 0)
      use_child = 0;

    if (use_child)
    {
      gldata->window = iupGLCreateChildWindow(info.parent_window, info.x, info.y, info.w, info.h);
      gldata->owns_window = 1;
    }
    else
    {
      gldata->window = (HWND)info.parent_window;
      gldata->owns_window = 0;
    }
  }

  if (!gldata->window)
    return 0;

  {
    LONG style = GetClassLong(gldata->window, GCL_STYLE);
    gldata->is_owned_dc = (style & CS_OWNDC) || (style & CS_CLASSDC);
  }

  gldata->lazy_init = 0;
  return wGLCreateContext(ih, gldata) == IUP_NOERROR && gldata->context != NULL;
}

static void wGLReleaseContext(IGlControlData* gldata)
{
  if (gldata->context)
  {
    if (gldata->context == wglGetCurrentContext())
      wglMakeCurrent(NULL, NULL);

    wglDeleteContext(gldata->context);
  }
}

static void wGLCanvasUnMapMethod(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");

  if (gldata->context && gldata->fbo && wglMakeCurrent(gldata->device, gldata->context))
    wGLCompositeRelease(gldata);
  else
    free(gldata->composite_pixels);

  wGLReleaseContext(gldata);

  if (gldata->palette)
    DeleteObject((HGDIOBJ)gldata->palette);

  if (gldata->device)
    ReleaseDC(gldata->window, gldata->device);

  if (gldata->owns_window && gldata->window)
    iupGLDestroyChildWindow(gldata->window);

  {
    HBRUSH br = (HBRUSH)iupAttribGet(ih, "_IUPWIN_BGBRUSH");
    HBITMAP bmp = (HBITMAP)iupAttribGet(ih, "_IUPWIN_BGBMP");
    if (br) { DeleteObject(br); iupAttribSet(ih, "_IUPWIN_BGBRUSH", NULL); }
    if (bmp) { DeleteObject(bmp); iupAttribSet(ih, "_IUPWIN_BGBMP", NULL); }
  }

  memset(gldata, 0, sizeof(IGlControlData));
}

static void wGLCanvasDestroy(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  free(gldata);
  iupAttribSet(ih, "_IUP_GLCONTROLDATA", NULL);
}

static int wGLCanvasSetRefreshContextAttrib(Ihandle* ih, const char* value)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");

  if (!gldata->is_owned_dc)
  {
    wGLReleaseContext(gldata);

    if (gldata->device)
      ReleaseDC(gldata->window, gldata->device);

    wGLCreateContext(ih, gldata);
  }

  (void)value;
  return 0;
}

void iupGlCanvasInitClass(Iclass* ic)
{
  ic->Create = wGLCanvasCreateMethod;
  ic->Destroy = wGLCanvasDestroy;
  ic->Map = wGLCanvasMapMethod;
  ic->UnMap = wGLCanvasUnMapMethod;

  iupClassRegisterAttribute(ic, "VISUAL", NULL, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_STRING);

  iupClassRegisterAttribute(ic, "REFRESHCONTEXT", NULL, wGLCanvasSetRefreshContextAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
}

/******************************************* Exported functions */

IUPGL_API int IupGLIsCurrent(Ihandle* ih)
{
  IGlControlData* gldata;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return 0;

  /* must be an IupGLCanvas */
  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "glcanvas"))
    return 0;

  /* must be mapped */
  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata->window)
    return 0;

  if (gldata->context == wglGetCurrentContext())
    return 1;

  return 0;
}

IUPGL_API void IupGLMakeCurrent(Ihandle* ih)
{
  IGlControlData* gldata;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  /* must be an IupGLCanvas */
  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "glcanvas"))
    return;

  /* must be mapped */
  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");

  if (gldata->lazy_init)
  {
    if (!wGLCanvasLazyInit(ih, gldata))
      return;
  }

  if (!gldata->window)
    return;

  if (gldata->owns_window)
    iupGLMoveChildWindow(gldata->window, ih->x, ih->y, ih->currentwidth, ih->currentheight);

  if (wglMakeCurrent(gldata->device, gldata->context)==FALSE)
  {
    iupAttribSet(ih, "ERROR", "Failed to set new current context.");
    iupAttribSetStr(ih, "LASTERROR", IupGetGlobal("LASTERROR"));
  }
  else
  {
    iupAttribSet(ih, "ERROR", NULL);
    iupAttribSet(ih, "LASTERROR", NULL);

    if (iupAttribGet(ih, "_IUPGL_COMPOSITE_CB"))
      wGLCompositeEnsureFBO(gldata, ih->currentwidth, ih->currentheight);

    if (!IupGetGlobal("GL_VERSION"))
    {
      IupSetStrGlobal("GL_VENDOR", (char*)glGetString(GL_VENDOR));
      IupSetStrGlobal("GL_RENDERER", (char*)glGetString(GL_RENDERER));
      IupSetStrGlobal("GL_VERSION", (char*)glGetString(GL_VERSION));
    }
  }
}

static void wGLCompositeReadback(Ihandle* ih)
{
  int w = ih->currentwidth, h = ih->currentheight;
  int oldw = iupAttribGetInt(ih, "_IUPWIN_BGBMPW");
  int oldh = iupAttribGetInt(ih, "_IUPWIN_BGBMPH");
  HBITMAP hbmp = (HBITMAP)iupAttribGet(ih, "_IUPWIN_BGBMP");
  void* bits = iupAttribGet(ih, "_IUPWIN_BGBMPBITS");
  HBRUSH oldbr, br;
  int first = 0;

  if (w < 1 || h < 1)
    return;

  if (!hbmp || oldw != w || oldh != h)
  {
    BITMAPINFO bi;
    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = h;    /* bottom-up */
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    if (hbmp) DeleteObject(hbmp);
    hbmp = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!hbmp) return;

    iupAttribSet(ih, "_IUPWIN_BGBMP", (char*)hbmp);
    iupAttribSet(ih, "_IUPWIN_BGBMPBITS", (char*)bits);
    iupAttribSetInt(ih, "_IUPWIN_BGBMPW", w);
    iupAttribSetInt(ih, "_IUPWIN_BGBMPH", h);
    first = 1;
  }

  glPixelStorei(GL_PACK_ALIGNMENT, 4);
  glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, bits);
  GdiFlush();

  oldbr = (HBRUSH)iupAttribGet(ih, "_IUPWIN_BGBRUSH");
  br = CreatePatternBrush(hbmp);
  iupAttribSet(ih, "_IUPWIN_BGBRUSH", (char*)br);
  if (oldbr) DeleteObject(oldbr);

  if (first)
    RedrawWindow(ih->handle, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
}

IUPGL_API void IupGLSwapBuffers(Ihandle* ih)
{
  IGlControlData* gldata;
  Icallback cb;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  /* must be an IupGLCanvas */
  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "glcanvas"))
    return;

  /* must be mapped */
  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata->window)
    return;

  cb = IupGetCallback(ih, "SWAPBUFFERS_CB");
  if (cb)
    cb(ih);

  {
    IFnComposite composite_cb = (IFnComposite)iupAttribGet(ih, "_IUPGL_COMPOSITE_CB");
    if (composite_cb)
    {
      wGLCompositeReadbackFBO(ih, gldata, composite_cb);
      return;
    }
  }

  if (IupClassMatch(ih, "glbackgroundbox"))
    wGLCompositeReadback(ih);

  SwapBuffers(gldata->device);
}

IUPGL_API void* IupGLGetProcAddress(const char* name)
{
  PROC proc = wglGetProcAddress(name);
  if (proc == NULL || proc == (PROC)1 || proc == (PROC)2 || proc == (PROC)3 || proc == (PROC)-1)
  {
    HMODULE lib = GetModuleHandleA("opengl32.dll");
    if (lib)
      proc = (PROC)GetProcAddress(lib, name);
  }
  return (void*)proc;
}

IUPGL_API void IupGLPalette(Ihandle* ih, int index, float r, float g, float b)
{
  IGlControlData* gldata;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  /* must be an IupGLCanvas */
  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "glcanvas"))
    return;

  /* must be mapped */
  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata->window)
    return;

  /* must have a palette */
  if (gldata->palette)
  {
    PALETTEENTRY entry;
    entry.peRed    = (BYTE)(r*255);
    entry.peGreen  = (BYTE)(g*255);
    entry.peBlue   = (BYTE)(b*255);
    entry.peFlags  = PC_NOCOLLAPSE;
    SetPaletteEntries(gldata->palette,index,1,&entry);
    UnrealizeObject(gldata->device);
    SelectPalette(gldata->device,gldata->palette,FALSE);
    RealizePalette(gldata->device);
  }
}

IUPGL_API void IupGLUseFont(Ihandle* ih, int first, int count, int list_base)
{
  HFONT font;
  IGlControlData* gldata;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  /* must be an IupGLCanvas */
  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "glcanvas"))
    return;

  /* must be mapped */
  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata->window)
    return;

  font = (HFONT)IupGetAttribute(ih, "HFONT");
  if (font)
  {
    HFONT old_font = SelectObject(gldata->device, font);
    wglUseFontBitmaps(gldata->device, first, count, list_base);
    SelectObject(gldata->device, old_font);
  }
}

IUPGL_API void IupGLWait(int gl)
{
  if (gl)
    glFinish();
  else
    GdiFlush();
}
