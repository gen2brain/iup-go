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
#include "iupcbs.h"
#include "iupgl.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_assert.h"
#include "iup_register.h"


typedef GLXContext (*glXCreateContextAttribsARB_PROC)(Display *dpy, GLXFBConfig config,
					       GLXContext share_list, Bool direct,
					       const int *attrib_list);

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
} IGlControlData;


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
  if (number > 0) 
  {
    alist[n++] = GLX_DEPTH_SIZE;
    alist[n++] = number;
  }

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
      /* remove the stereo flag and try again */
      n--;
      alist[n] = None;
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

	    int nelements;
	    GLXFBConfig *config = glXChooseFBConfig(gldata->display, DefaultScreen(gldata->display), 0, &nelements);

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
      
      gldata->context = CreateContextAttribsARB(gldata->display, *config, shared_context, GL_TRUE, attribs);
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
    if (gldata->context == glXGetCurrentContext())
      glXMakeCurrent(gldata->display, None, NULL);

    glXDestroyContext(gldata->display, gldata->context);
  }

  if (gldata->colormap != None)
    XFreeColormap(gldata->display, gldata->colormap);

  if (gldata->vinfo)
    XFree(gldata->vinfo); 

  memset(gldata, 0, sizeof(IGlControlData));
}

void iupdrvGlCanvasInitClass(Iclass* ic)
{
  ic->Create = xGLCanvasCreateMethod;
  ic->Destroy = xGLCanvasDestroy;
  ic->Map = xGLCanvasMapMethod;
  ic->UnMap = xGLCanvasUnMapMethod;

  iupClassRegisterAttribute(ic, "VISUAL", xGLCanvasGetVisualAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_STRING|IUPAF_NOT_MAPPED);
}


/******************************************* Exported functions */


int IupGLIsCurrent(Ihandle* ih)
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

void IupGLMakeCurrent(Ihandle* ih)
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

    if (!IupGetGlobal("GL_VERSION"))
    {
      IupSetStrGlobal("GL_VENDOR", (char*)glGetString(GL_VENDOR));
      IupSetStrGlobal("GL_RENDERER", (char*)glGetString(GL_RENDERER));
      IupSetStrGlobal("GL_VERSION", (char*)glGetString(GL_VERSION));
    }
  }
}

void IupGLSwapBuffers(Ihandle* ih)
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

  glXSwapBuffers(gldata->display, gldata->window);
}

static int xGLCanvasIgnoreError(Display *param1, XErrorEvent *param2)
{
  (void)param1;
  (void)param2;
  return 0;
}

#define iglxColorScale(c)  (unsigned short)(c * 65535.0 + 0.5)

void IupGLPalette(Ihandle* ih, int index, float r, float g, float b)
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

void IupGLUseFont(Ihandle* ih, int first, int count, int list_base)
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

void IupGLWait(int gl)
{
  if (gl)
    glXWaitGL();
  else
    glXWaitX();
}
