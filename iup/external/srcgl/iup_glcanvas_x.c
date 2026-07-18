/** \file
 * \brief iupgl control for X11
 *
 * See Copyright Notice in "iup.h"
 */

#include <X11/Xlib.h>
#include <GL/glx.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "iup.h"
#include "iupgl.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_drv.h"

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif


typedef GLXContext (*glXCreateContextAttribsARB_PROC)(Display *dpy, GLXFBConfig config, GLXContext share_list, Bool direct, const int *attrib_list);

#ifndef GLX_CONTEXT_MAJOR_VERSION_ARB
#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092
#define GLX_CONTEXT_FLAGS_ARB 0x2094
#define GLX_CONTEXT_DEBUG_BIT_ARB 0x0001
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x0002
#endif

#ifndef GLX_CONTEXT_PROFILE_MASK_ARB
#define GLX_CONTEXT_PROFILE_MASK_ARB 0x9126
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#endif


/* Do NOT use _IcontrolData to make inheritance easy
   when parent class is glcanvas */
typedef struct _IGlControlData
{
  Display* display;
  Drawable window;
  Colormap colormap;
  XVisualInfo *vinfo;
  GLXContext context;

  int use_composite;
  GLuint fbo, color_tex, depth_rbo;
  int fbo_w, fbo_h;
  unsigned char* composite_pixels;   /* BGRA, top-left */
  size_t composite_cap;
} IGlControlData;

typedef void (*IUPglGenFramebuffers)(GLsizei, GLuint*);
typedef void (*IUPglBindFramebuffer)(GLenum, GLuint);
typedef void (*IUPglDeleteFramebuffers)(GLsizei, const GLuint*);
typedef void (*IUPglFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef GLenum (*IUPglCheckFramebufferStatus)(GLenum);
typedef void (*IUPglGenRenderbuffers)(GLsizei, GLuint*);
typedef void (*IUPglBindRenderbuffer)(GLenum, GLuint);
typedef void (*IUPglDeleteRenderbuffers)(GLsizei, const GLuint*);
typedef void (*IUPglRenderbufferStorage)(GLenum, GLenum, GLsizei, GLsizei);
typedef void (*IUPglFramebufferRenderbuffer)(GLenum, GLenum, GLenum, GLuint);

static IUPglGenFramebuffers        xGLGenFramebuffers;
static IUPglBindFramebuffer        xGLBindFramebuffer;
static IUPglDeleteFramebuffers     xGLDeleteFramebuffers;
static IUPglFramebufferTexture2D   xGLFramebufferTexture2D;
static IUPglCheckFramebufferStatus xGLCheckFramebufferStatus;
static IUPglGenRenderbuffers       xGLGenRenderbuffers;
static IUPglBindRenderbuffer       xGLBindRenderbuffer;
static IUPglDeleteRenderbuffers    xGLDeleteRenderbuffers;
static IUPglRenderbufferStorage    xGLRenderbufferStorage;
static IUPglFramebufferRenderbuffer xGLFramebufferRenderbuffer;

static int xGLLoadFBOProcs(void)
{
  if (xGLGenFramebuffers)
    return 1;
  xGLGenFramebuffers         = (IUPglGenFramebuffers)glXGetProcAddressARB((const GLubyte*)"glGenFramebuffers");
  xGLBindFramebuffer         = (IUPglBindFramebuffer)glXGetProcAddressARB((const GLubyte*)"glBindFramebuffer");
  xGLDeleteFramebuffers      = (IUPglDeleteFramebuffers)glXGetProcAddressARB((const GLubyte*)"glDeleteFramebuffers");
  xGLFramebufferTexture2D    = (IUPglFramebufferTexture2D)glXGetProcAddressARB((const GLubyte*)"glFramebufferTexture2D");
  xGLCheckFramebufferStatus  = (IUPglCheckFramebufferStatus)glXGetProcAddressARB((const GLubyte*)"glCheckFramebufferStatus");
  xGLGenRenderbuffers        = (IUPglGenRenderbuffers)glXGetProcAddressARB((const GLubyte*)"glGenRenderbuffers");
  xGLBindRenderbuffer        = (IUPglBindRenderbuffer)glXGetProcAddressARB((const GLubyte*)"glBindRenderbuffer");
  xGLDeleteRenderbuffers     = (IUPglDeleteRenderbuffers)glXGetProcAddressARB((const GLubyte*)"glDeleteRenderbuffers");
  xGLRenderbufferStorage     = (IUPglRenderbufferStorage)glXGetProcAddressARB((const GLubyte*)"glRenderbufferStorage");
  xGLFramebufferRenderbuffer = (IUPglFramebufferRenderbuffer)glXGetProcAddressARB((const GLubyte*)"glFramebufferRenderbuffer");
  return xGLGenFramebuffers && xGLBindFramebuffer && xGLFramebufferTexture2D && xGLCheckFramebufferStatus && xGLGenRenderbuffers && xGLRenderbufferStorage && xGLFramebufferRenderbuffer;
}

static int xGLCompositeEnsureFBO(IGlControlData* gldata, int w, int h)
{
  if (w < 1) w = 1;
  if (h < 1) h = 1;

  if (gldata->fbo && gldata->fbo_w == w && gldata->fbo_h == h)
    return 1;

  if (!xGLLoadFBOProcs())
    return 0;

  if (!gldata->fbo)       xGLGenFramebuffers(1, &gldata->fbo);
  if (!gldata->color_tex) glGenTextures(1, &gldata->color_tex);
  if (!gldata->depth_rbo) xGLGenRenderbuffers(1, &gldata->depth_rbo);

  glBindTexture(GL_TEXTURE_2D, gldata->color_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  xGLBindRenderbuffer(GL_RENDERBUFFER, gldata->depth_rbo);
  xGLRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
  xGLBindRenderbuffer(GL_RENDERBUFFER, 0);

  xGLBindFramebuffer(GL_FRAMEBUFFER, gldata->fbo);
  xGLFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gldata->color_tex, 0);
  xGLFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gldata->depth_rbo);

  if (xGLCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    xGLBindFramebuffer(GL_FRAMEBUFFER, 0);
    return 0;
  }

  gldata->fbo_w = w;
  gldata->fbo_h = h;
  return 1;
}

static void xGLCompositeReadback(Ihandle* ih, IGlControlData* gldata)
{
  int w = gldata->fbo_w, h = gldata->fbo_h;
  int rowbytes = w * 4;
  size_t need = (size_t)rowbytes * h;
  unsigned char* px;
  int y;

  if (!gldata->fbo || w < 1 || h < 1)
    return;

  if (gldata->composite_cap < need)
  {
    unsigned char* nb = (unsigned char*)realloc(gldata->composite_pixels, need);
    if (!nb) return;
    gldata->composite_pixels = nb;
    gldata->composite_cap = need;
  }
  px = gldata->composite_pixels;

  xGLBindFramebuffer(GL_FRAMEBUFFER, gldata->fbo);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, px);

  {
    unsigned char* tmp = (unsigned char*)malloc(rowbytes);
    if (tmp)
    {
      for (y = 0; y < h/2; y++)
      {
        unsigned char* a = px + (size_t)y*rowbytes;
        unsigned char* b = px + (size_t)(h-1-y)*rowbytes;
        memcpy(tmp, a, rowbytes); memcpy(a, b, rowbytes); memcpy(b, tmp, rowbytes);
      }
      free(tmp);
    }
  }
  {
    size_t i, n = (size_t)w*h;
    for (i = 0; i < n; i++)
      px[i*4+3] = 255;
  }

  iupAttribSet(ih, "_IUPGL_COMPOSITE_PIXELS", (char*)px);
  iupAttribSetInt(ih, "_IUPGL_COMPOSITE_W", w);
  iupAttribSetInt(ih, "_IUPGL_COMPOSITE_H", h);
}

static void xGLCompositeSize(IGlControlData* gldata, int* w, int* h)
{
  Window root;
  int x, y;
  unsigned int ww, hh, bw, depth;
  if (gldata->window && XGetGeometry(gldata->display, gldata->window, &root, &x, &y, &ww, &hh, &bw, &depth))
  {
    *w = (int)ww;
    *h = (int)hh;
  }
  else
  {
    *w = 1;
    *h = 1;
  }
}


static int xGLCanvasDefaultResize(Ihandle *ih, int width, int height)
{
  IupGLMakeCurrent(ih);
  glViewport(0,0,width,height);
  return IUP_DEFAULT;
}

static int xGLCanvasCreateMethod(Ihandle* ih, void** params)
{
  IGlControlData* gldata;
  (void)params;

  gldata = (IGlControlData*)malloc(sizeof(IGlControlData));
  memset(gldata, 0, sizeof(IGlControlData));
  iupAttribSet(ih, "_IUP_GLCONTROLDATA", (char*)gldata);

  IupSetCallback(ih, "RESIZE_CB", (Icallback)xGLCanvasDefaultResize);

  return IUP_NOERROR;
}

static void xGLCanvasGetVisual(Ihandle* ih, IGlControlData* gldata)
{
  int erb, evb, number;
  int n = 0;
  int alist[40];

  if (!gldata->display)
    gldata->display = (Display*)IupGetGlobal("XDISPLAY");  /* works for Motif and GTK, can be called before mapped */
  if (!gldata->display)
    return;

  /* double or single buffer */
  if (iupStrEqualNoCase(iupAttribGetStr(ih,"BUFFER"), "DOUBLE"))
  {
    alist[n++] = GLX_DOUBLEBUFFER;
  }

  /* rgba or index */
  if (iupStrEqualNoCase(iupAttribGetStr(ih,"COLOR"), "INDEX"))
  {
    /* buffer size (for index mode) */
    number = iupAttribGetInt(ih,"BUFFER_SIZE");
    if (number > 0)
    {
      alist[n++] = GLX_BUFFER_SIZE;
      alist[n++] = number;
    }
  }
  else
  {
    alist[n++] = GLX_RGBA;      /* assume rgba as default */
  }

  /* red, green, blue bits */
  number = iupAttribGetInt(ih,"RED_SIZE");
  if (number > 0)
  {
    alist[n++] = GLX_RED_SIZE;
    alist[n++] = number;
  }

  number = iupAttribGetInt(ih,"GREEN_SIZE");
  if (number > 0)
  {
    alist[n++] = GLX_GREEN_SIZE;
    alist[n++] = number;
  }

  number = iupAttribGetInt(ih,"BLUE_SIZE");
  if (number > 0)
  {
    alist[n++] = GLX_BLUE_SIZE;
    alist[n++] = number;
  }

  number = iupAttribGetInt(ih,"ALPHA_SIZE");
  if (number > 0)
  {
    alist[n++] = GLX_ALPHA_SIZE;
    alist[n++] = number;
  }

  /* depth and stencil size */
  number = iupAttribGetInt(ih,"DEPTH_SIZE");
  alist[n++] = GLX_DEPTH_SIZE;
  alist[n++] = (number > 0) ? number : 16; /* Default to 16-bit depth */

  number = iupAttribGetInt(ih,"STENCIL_SIZE");
  if (number > 0)
  {
    alist[n++] = GLX_STENCIL_SIZE;
    alist[n++] = number;
  }

  /* red, green, blue accumulation bits */
  number = iupAttribGetInt(ih,"ACCUM_RED_SIZE");
  if (number > 0)
  {
    alist[n++] = GLX_ACCUM_RED_SIZE;
    alist[n++] = number;
  }

  number = iupAttribGetInt(ih,"ACCUM_GREEN_SIZE");
  if (number > 0)
  {
    alist[n++] = GLX_ACCUM_GREEN_SIZE;
    alist[n++] = number;
  }

  number = iupAttribGetInt(ih,"ACCUM_BLUE_SIZE");
  if (number > 0)
  {
    alist[n++] = GLX_ACCUM_BLUE_SIZE;
    alist[n++] = number;
  }

  number = iupAttribGetInt(ih,"ACCUM_ALPHA_SIZE");
  if (number > 0)
  {
    alist[n++] = GLX_ACCUM_ALPHA_SIZE;
    alist[n++] = number;
  }

  /* stereo */
  if (iupAttribGetBoolean(ih,"STEREO"))
  {
    alist[n++] = GLX_STEREO;
  }

  /* TERMINATOR */
  alist[n++] = None;

  /* check out X extension */
  if (!glXQueryExtension(gldata->display, &erb, &evb))
  {
    iupAttribSet(ih, "ERROR", "X server has no OpenGL GLX extension.");
    return;
  }

  /* choose visual */
  gldata->vinfo = glXChooseVisual(gldata->display, DefaultScreen(gldata->display), alist);
  if (!gldata->vinfo)
  {
    if (iupAttribGetBoolean(ih,"STEREO"))
    {
      int i;
      /* remove the stereo flag and try again */
      for (i = 0; i < n; i++)
      {
        if (alist[i] == GLX_STEREO)
        {
          int j;
          for (j = i; j < n - 1; j++) /* n-1 is the index of None */
            alist[j] = alist[j + 1];
          n--;
          break;
        }
      }

      iupAttribSet(ih, "STEREO", "NO");
      gldata->vinfo = glXChooseVisual(gldata->display, DefaultScreen(gldata->display), alist);
    }
  }

  if (!gldata->vinfo)
  {
    iupAttribSet(ih, "ERROR", "No appropriate visual.");
    return;
  }

  iupAttribSet(ih, "ERROR", NULL);
}

static char* xGLCanvasGetVisualAttrib(Ihandle *ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");

  /* This must be available before mapping, because IupCanvas uses it during map in GTK and Motif. */
  if (gldata->vinfo)
    return (char*)gldata->vinfo->visual;

  xGLCanvasGetVisual(ih, gldata);

  if (gldata->vinfo)
    return (char*)gldata->vinfo->visual;

  return NULL;
}

static int xGLCanvasMapMethod(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  GLXContext shared_context = NULL;
  Ihandle* ih_shared;

  /* the IupCanvas is already mapped, just initialize the OpenGL context */

  if (!xGLCanvasGetVisualAttrib(ih))
    return IUP_NOERROR; /* do not abort mapping */

  gldata->window = (XID)iupAttribGet(ih, "XWINDOW"); /* check first in the hash table, can be defined by the IupFileDlg */
  if (!gldata->window)
    gldata->window = (XID)IupGetAttribute(ih, "XWINDOW");  /* works for Motif and GTK, only after mapping the IupCanvas */
  if (!gldata->window)
    return IUP_NOERROR;

  if (IupClassMatch(ih, "glbackgroundbox"))
  {
    gldata->use_composite = 1;
    iupAttribSet(ih, "_IUPGL_COMPOSITE", "1");
  }

  ih_shared = IupGetAttributeHandle(ih, "SHAREDCONTEXT");
  if (ih_shared && IupClassMatch(ih_shared, "glcanvas"))  /* must be an IupGLCanvas */
  {
    IGlControlData* shared_gldata = (IGlControlData*)iupAttribGet(ih_shared, "_IUP_GLCONTROLDATA");
    shared_context = shared_gldata->context;
  }

  /* create rendering context */
  if (iupAttribGetBoolean(ih, "ARBCONTEXT"))
  {
    glXCreateContextAttribsARB_PROC CreateContextAttribsARB = NULL;

    GLXContext tempContext = glXCreateContext(gldata->display, gldata->vinfo, NULL, GL_TRUE);
    GLXContext oldContext = glXGetCurrentContext();
    Display* oldDisplay = glXGetCurrentDisplay();
    GLXDrawable oldDrawable = glXGetCurrentDrawable();
    glXMakeCurrent(gldata->display, gldata->window, tempContext);   /* glXGetProcAddress only works with an active context */

    CreateContextAttribsARB = (glXCreateContextAttribsARB_PROC)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
    if (CreateContextAttribsARB)
    {
      int attribs[9], a = 0;
      char* value;
      GLXFBConfig matching_config = NULL;
      int nelements = 0, i;
      GLXFBConfig *configs = glXGetFBConfigs(gldata->display, DefaultScreen(gldata->display), &nelements);

      if (configs && nelements > 0)
      {
        for (i = 0; i < nelements; i++)
        {
          int visual_id = 0;
          /* Find the FBConfig that matches the VisualID of our chosen XVisualInfo */
          glXGetFBConfigAttrib(gldata->display, configs[i], GLX_VISUAL_ID, &visual_id);
          if (visual_id == gldata->vinfo->visualid)
          {
            matching_config = configs[i];
            break;
          }
        }
        XFree(configs);
      }

      if (!matching_config)
      {
        iupAttribSet(ih, "ERROR", "Could not find a GLXFBConfig matching the chosen XVisualInfo.");
        glXMakeCurrent(oldDisplay, oldDrawable, oldContext);
        glXDestroyContext(gldata->display, tempContext);
        return IUP_NOERROR;
      }

      value = iupAttribGetStr(ih, "CONTEXTVERSION");
      if (value)
      {
        int major, minor;
        if (iupStrToIntInt(value, &major, &minor, '.') == 2)
        {
          attribs[a++] = GLX_CONTEXT_MAJOR_VERSION_ARB;
          attribs[a++] = major;
          attribs[a++] = GLX_CONTEXT_MINOR_VERSION_ARB;
          attribs[a++] = minor;
        }
      }

      value = iupAttribGetStr(ih, "CONTEXTFLAGS");
      if (value)
      {
        int flags = 0;
        if (iupStrEqualNoCase(value, "DEBUG"))
          flags = GLX_CONTEXT_DEBUG_BIT_ARB;
        else if (iupStrEqualNoCase(value, "FORWARDCOMPATIBLE"))
          flags = GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
        else if (iupStrEqualNoCase(value, "DEBUGFORWARDCOMPATIBLE"))
          flags = GLX_CONTEXT_DEBUG_BIT_ARB|GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
        if (flags)
        {
          attribs[a++] = GLX_CONTEXT_FLAGS_ARB;
          attribs[a++] = flags;
        }
      }

      value = iupAttribGetStr(ih, "CONTEXTPROFILE");
      if (value)
      {
        int profile = 0;
        if (iupStrEqualNoCase(value, "CORE"))
          profile = GLX_CONTEXT_CORE_PROFILE_BIT_ARB;
        else if (iupStrEqualNoCase(value, "COMPATIBILITY"))
          profile = GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
        else if (iupStrEqualNoCase(value, "CORECOMPATIBILITY"))
          profile = GLX_CONTEXT_CORE_PROFILE_BIT_ARB|GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
        if (profile)
        {
          attribs[a++] = GLX_CONTEXT_PROFILE_MASK_ARB;
          attribs[a++] = profile;
        }
      }

      attribs[a] = 0; /* terminator */

      gldata->context = CreateContextAttribsARB(gldata->display, matching_config, shared_context, GL_TRUE, attribs);
    }

    glXMakeCurrent(oldDisplay, oldDrawable, oldContext);
    glXDestroyContext(gldata->display, tempContext);

    if (!CreateContextAttribsARB)
    {
      gldata->context = glXCreateContext(gldata->display, gldata->vinfo, shared_context, GL_TRUE);
      iupAttribSet(ih, "ARBCONTEXT", "NO");
    }
  }
  else
    gldata->context = glXCreateContext(gldata->display, gldata->vinfo, shared_context, GL_TRUE);

  if (!gldata->context)
  {
    iupAttribSet(ih, "ERROR", "Could not create a rendering context.");
    return IUP_NOERROR;
  }

  iupAttribSet(ih, "CONTEXT", (char*)gldata->context);

  /* create colormap for index mode */
  if (iupStrEqualNoCase(iupAttribGetStr(ih,"COLOR"), "INDEX") &&
      gldata->vinfo->class != StaticColor && gldata->vinfo->class != StaticGray)
  {
    gldata->colormap = XCreateColormap(gldata->display, RootWindow(gldata->display, DefaultScreen(gldata->display)), gldata->vinfo->visual, AllocAll);
    iupAttribSet(ih, "COLORMAP", (char*)gldata->colormap);
  }

  if (gldata->colormap != None)
    IupGLPalette(ih,0,1,1,1);  /* set first color as white */

  iupAttribSet(ih, "ERROR", NULL);
  return IUP_NOERROR;
}

static void xGLCanvasDestroy(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  free(gldata);
  iupAttribSet(ih, "_IUP_GLCONTROLDATA", NULL);
}

static void xGLCanvasUnMapMethod(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");

  if (gldata->context)
  {
    if (gldata->fbo || gldata->color_tex || gldata->depth_rbo)
    {
      glXMakeCurrent(gldata->display, gldata->window, gldata->context);
      if (gldata->fbo)       xGLDeleteFramebuffers(1, &gldata->fbo);
      if (gldata->color_tex) glDeleteTextures(1, &gldata->color_tex);
      if (gldata->depth_rbo) xGLDeleteRenderbuffers(1, &gldata->depth_rbo);
    }

    if (gldata->context == glXGetCurrentContext())
      glXMakeCurrent(gldata->display, None, NULL);

    glXDestroyContext(gldata->display, gldata->context);
  }

  if (gldata->composite_pixels)
    free(gldata->composite_pixels);
  iupAttribSet(ih, "_IUPGL_COMPOSITE_PIXELS", NULL);

#if defined(IUP_USE_MOTIF)
  {
    Pixmap pm = (Pixmap)iupAttribGet(ih, "_IUPMOT_GLPIXMAP");
    GC gc = (GC)iupAttribGet(ih, "_IUPMOT_GLGC");
    if (pm)
    {
      XFreePixmap(gldata->display, pm);
      iupAttribSet(ih, "_IUPMOT_GLPIXMAP", NULL);
    }
    if (gc)
    {
      XFreeGC(gldata->display, gc);
      iupAttribSet(ih, "_IUPMOT_GLGC", NULL);
    }
  }
#endif

  if (gldata->colormap != None)
    XFreeColormap(gldata->display, gldata->colormap);

  if (gldata->vinfo)
    XFree(gldata->vinfo);

  memset(gldata, 0, sizeof(IGlControlData));
}

void iupGlCanvasInitClass(Iclass* ic)
{
  ic->Create = xGLCanvasCreateMethod;
  ic->Destroy = xGLCanvasDestroy;
  ic->Map = xGLCanvasMapMethod;
  ic->UnMap = xGLCanvasUnMapMethod;

  iupClassRegisterAttribute(ic, "VISUAL", xGLCanvasGetVisualAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_STRING|IUPAF_NOT_MAPPED);
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

  if (gldata->context == glXGetCurrentContext())
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
  if (!gldata->window)
    return;

  if (glXMakeCurrent(gldata->display, gldata->window, gldata->context)==False)
    iupAttribSet(ih, "ERROR", "Failed to set new current context.");
  else
  {
    iupAttribSet(ih, "ERROR", NULL);
    glXWaitX();

    if (gldata->use_composite)
    {
      int pw, ph;
      xGLCompositeSize(gldata, &pw, &ph);
      if (xGLCompositeEnsureFBO(gldata, pw, ph))
        xGLBindFramebuffer(GL_FRAMEBUFFER, gldata->fbo);
    }

    if (!IupGetGlobal("GL_VERSION"))
    {
      IupSetStrGlobal("GL_VENDOR", (char*)glGetString(GL_VENDOR));
      IupSetStrGlobal("GL_RENDERER", (char*)glGetString(GL_RENDERER));
      IupSetStrGlobal("GL_VERSION", (char*)glGetString(GL_VERSION));
    }
  }
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

  if (gldata->use_composite)
  {
    xGLCompositeReadback(ih, gldata);
    if (!iupAttribGet(ih, "_IUPGL_IN_DRAW"))
      iupdrvPostRedraw(ih);
    return;
  }

  glXSwapBuffers(gldata->display, gldata->window);
}

IUPGL_API void* IupGLGetProcAddress(const char* name)
{
  return (void*)glXGetProcAddressARB((const GLubyte*)name);
}

static int xGLCanvasIgnoreError(Display *param1, XErrorEvent *param2)
{
  (void)param1;
  (void)param2;
  return 0;
}

#define iglxColorScale(c)  (unsigned short)(c * 65535.0 + 0.5)

IUPGL_API void IupGLPalette(Ihandle* ih, int index, float r, float g, float b)
{
  IGlControlData* gldata;
  XColor color;
  int rShift, gShift, bShift;
  XVisualInfo *vinfo;
  XErrorHandler old_handler;

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

  /* must have a colormap */
  if (gldata->colormap == None)
    return;

  /* code fragment based on the toolkit library provided with OpenGL */
  old_handler = XSetErrorHandler(xGLCanvasIgnoreError);

  vinfo = gldata->vinfo;
  switch (vinfo->class)
  {
  case DirectColor:
    rShift = ffs((unsigned int)vinfo->red_mask) - 1;
    gShift = ffs((unsigned int)vinfo->green_mask) - 1;
    bShift = ffs((unsigned int)vinfo->blue_mask) - 1;
    color.pixel = ((index << rShift) & vinfo->red_mask) |
                  ((index << gShift) & vinfo->green_mask) |
                  ((index << bShift) & vinfo->blue_mask);
    color.red = iglxColorScale(r);
    color.green = iglxColorScale(g);
    color.blue = iglxColorScale(b);
    color.flags = DoRed | DoGreen | DoBlue;
    XStoreColor(gldata->display, gldata->colormap, &color);
    break;
  case GrayScale:
  case PseudoColor:
    if (index < vinfo->colormap_size)
    {
      color.pixel = index;
      color.red = iglxColorScale(r);
      color.green = iglxColorScale(g);
      color.blue = iglxColorScale(b);
      color.flags = DoRed | DoGreen | DoBlue;
      XStoreColor(gldata->display, gldata->colormap, &color);
    }
    break;
  }

  XSync(gldata->display, 0);
  XSetErrorHandler(old_handler);
}

IUPGL_API void IupGLUseFont(Ihandle* ih, int first, int count, int list_base)
{
  IGlControlData* gldata;
  Font font;

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

  font = (Font)IupGetAttribute(ih, "XFONTID");
  if (font)
    glXUseXFont(font, first, count, list_base);
}

IUPGL_API void IupGLWait(int gl)
{
  if (gl)
    glXWaitGL();
  else
    glXWaitX();
}
