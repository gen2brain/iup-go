/** \file
 * \brief iupgl control for Windows
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <GL/gl.h>

#include <stdlib.h>
#include <stdio.h>
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

typedef HGLRC (WINAPI *wglCreateContextAttribsARB_PROC) (HDC hDC, HGLRC hShareContext, const int *attribList);

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
#define ERROR_INVALID_VERSION_ARB		0x2095
#define ERROR_INVALID_PROFILE_ARB		0x2096
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
} IGlControlData;

static int wGLCanvasDefaultResize_CB(Ihandle *ih, int width, int height)
{
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
    gldata->is_owned_dc = (int) ((style & CS_OWNDC) || (style & CS_CLASSDC));
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

  /* get a device context */
  gldata->window = (HWND)iupAttribGet(ih, "HWND"); /* check first in the hash table, can be defined by the IupFileDlg */
  if (!gldata->window)
    gldata->window = (HWND)IupGetAttribute(ih, "HWND");  /* works for Win32 and GTK, only after mapping the IupCanvas */
  if (!gldata->window)
    return IUP_NOERROR;

  {
    LONG style = GetClassLong(gldata->window, GCL_STYLE);
    gldata->is_owned_dc = (int) ((style & CS_OWNDC) || (style & CS_CLASSDC));
  }

  return wGLCreateContext(ih, gldata);
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

  wGLReleaseContext(gldata);

  if (gldata->palette)
    DeleteObject((HGDIOBJ)gldata->palette);

  if (gldata->device)
    ReleaseDC(gldata->window, gldata->device);

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

void iupdrvGlCanvasInitClass(Iclass* ic)
{
  ic->Create = wGLCanvasCreateMethod;
  ic->Destroy = wGLCanvasDestroy;
  ic->Map = wGLCanvasMapMethod;
  ic->UnMap = wGLCanvasUnMapMethod;

  iupClassRegisterAttribute(ic, "VISUAL", NULL, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_STRING);

  iupClassRegisterAttribute(ic, "REFRESHCONTEXT", NULL, wGLCanvasSetRefreshContextAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
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

  if (gldata->context == wglGetCurrentContext())
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

  if (wglMakeCurrent(gldata->device, gldata->context)==FALSE)
  {
    iupAttribSet(ih, "ERROR", "Failed to set new current context.");
    iupAttribSetStr(ih, "LASTERROR", IupGetGlobal("LASTERROR"));
  }
  else
  {
    iupAttribSet(ih, "ERROR", NULL);
    iupAttribSet(ih, "LASTERROR", NULL);

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

  SwapBuffers(gldata->device);
}

void IupGLPalette(Ihandle* ih, int index, float r, float g, float b)
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

void IupGLUseFont(Ihandle* ih, int first, int count, int list_base)
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

void IupGLWait(int gl)
{
  if (gl)
    glFinish();
  else
    GdiFlush();
}
