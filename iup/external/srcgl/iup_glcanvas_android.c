/** \file
 * \brief GLCanvas Control for Android (SurfaceView + manual EGL14).
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include "iup.h"
#include "iupgl.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_drv.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupGLCanvasHelper);


#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x00000040
#endif

#ifndef EGL_CONTEXT_MAJOR_VERSION_KHR
#define EGL_CONTEXT_MAJOR_VERSION_KHR 0x3098
#endif

#ifndef EGL_CONTEXT_MINOR_VERSION_KHR
#define EGL_CONTEXT_MINOR_VERSION_KHR 0x30FB
#endif


typedef struct _IGlControlData
{
  EGLDisplay display;
  EGLSurface surface;
  EGLContext context;
  EGLConfig  config;

  ANativeWindow* window;

  int swap_interval;
} IGlControlData;


static int androidGLCanvasDefaultResize(Ihandle* ih, int width, int height)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || gldata->context == EGL_NO_CONTEXT || gldata->surface == EGL_NO_SURFACE)
    return IUP_DEFAULT;

  if (eglMakeCurrent(gldata->display, gldata->surface, gldata->surface, gldata->context) == EGL_TRUE)
    glViewport(0, 0, width, height);

  return IUP_DEFAULT;
}

static int androidGLCanvasCreateMethod(Ihandle* ih, void** params)
{
  IGlControlData* gldata;
  (void)params;

  gldata = (IGlControlData*)malloc(sizeof(IGlControlData));
  memset(gldata, 0, sizeof(IGlControlData));

  gldata->display = EGL_NO_DISPLAY;
  gldata->surface = EGL_NO_SURFACE;
  gldata->context = EGL_NO_CONTEXT;
  gldata->swap_interval = 1;

  iupAttribSet(ih, "_IUP_GLCONTROLDATA", (char*)gldata);

  IupSetCallback(ih, "RESIZE_CB", (Icallback)androidGLCanvasDefaultResize);

  return IUP_NOERROR;
}

static int androidGLCanvasChooseConfig(Ihandle* ih, IGlControlData* gldata)
{
  EGLint alist[32];
  EGLint num_config = 0;
  int n = 0;
  int red = 8, green = 8, blue = 8, alpha = 0;
  int depth = 16, stencil = 0, samples = 0;
  int value;
  int major = 3;

  if (iupAttribGetStr(ih, "CONTEXTVERSION"))
  {
    int m, mi;
    if (iupStrToIntInt(iupAttribGetStr(ih, "CONTEXTVERSION"), &m, &mi, '.') >= 1)
      major = m > 0 ? m : 3;
  }

  value = iupAttribGetInt(ih, "RED_SIZE");   if (value > 0) red = value;
  value = iupAttribGetInt(ih, "GREEN_SIZE"); if (value > 0) green = value;
  value = iupAttribGetInt(ih, "BLUE_SIZE");  if (value > 0) blue = value;
  value = iupAttribGetInt(ih, "ALPHA_SIZE"); if (value > 0) alpha = value;
  value = iupAttribGetInt(ih, "DEPTH_SIZE"); if (value > 0) depth = value;
  value = iupAttribGetInt(ih, "STENCIL_SIZE"); if (value > 0) stencil = value;
  value = iupAttribGetInt(ih, "SAMPLES");    if (value > 0) samples = value;

  alist[n++] = EGL_SURFACE_TYPE;     alist[n++] = EGL_WINDOW_BIT;
  alist[n++] = EGL_RENDERABLE_TYPE;  alist[n++] = (major >= 3) ? EGL_OPENGL_ES3_BIT_KHR : EGL_OPENGL_ES2_BIT;
  alist[n++] = EGL_RED_SIZE;         alist[n++] = red;
  alist[n++] = EGL_GREEN_SIZE;       alist[n++] = green;
  alist[n++] = EGL_BLUE_SIZE;        alist[n++] = blue;
  if (alpha > 0)   { alist[n++] = EGL_ALPHA_SIZE;   alist[n++] = alpha; }
  alist[n++] = EGL_DEPTH_SIZE;       alist[n++] = depth;
  if (stencil > 0) { alist[n++] = EGL_STENCIL_SIZE; alist[n++] = stencil; }
  if (samples > 0) { alist[n++] = EGL_SAMPLE_BUFFERS; alist[n++] = 1;
                     alist[n++] = EGL_SAMPLES;      alist[n++] = samples; }
  alist[n] = EGL_NONE;

  if (iupAttribGetBoolean(ih, "STEREO"))
    iupAttribSet(ih, "STEREO", "NO");

  if (!eglChooseConfig(gldata->display, alist, &gldata->config, 1, &num_config) || num_config == 0)
  {
    EGLint fb[] = {
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
      EGL_DEPTH_SIZE, 16,
      EGL_NONE
    };
    if (!eglChooseConfig(gldata->display, fb, &gldata->config, 1, &num_config) || num_config == 0)
    {
      iupAttribSetStrf(ih, "ERROR", "No appropriate EGL config found. Error: 0x%X", eglGetError());
      return 0;
    }
  }

  iupAttribSet(ih, "ERROR", NULL);
  return 1;
}

static EGLContext androidGLCanvasCreateContext(Ihandle* ih, IGlControlData* gldata)
{
  EGLint ctxAttr[7];
  int a = 0;
  int major = 3, minor = 0;
  EGLContext shared = EGL_NO_CONTEXT;
  Ihandle* ih_shared;

  if (iupAttribGetStr(ih, "CONTEXTVERSION"))
  {
    int m, mi;
    int got = iupStrToIntInt(iupAttribGetStr(ih, "CONTEXTVERSION"), &m, &mi, '.');
    if (got >= 1 && m > 0) major = m;
    if (got == 2 && mi >= 0) minor = mi;
  }

  ctxAttr[a++] = EGL_CONTEXT_MAJOR_VERSION_KHR;
  ctxAttr[a++] = major;
  ctxAttr[a++] = EGL_CONTEXT_MINOR_VERSION_KHR;
  ctxAttr[a++] = minor;
  ctxAttr[a] = EGL_NONE;

  ih_shared = IupGetAttributeHandle(ih, "SHAREDCONTEXT");
  if (ih_shared && IupClassMatch(ih_shared, "glcanvas"))
  {
    IGlControlData* shared_gldata = (IGlControlData*)iupAttribGet(ih_shared, "_IUP_GLCONTROLDATA");
    if (shared_gldata)
      shared = shared_gldata->context;
  }

  EGLContext ctx = eglCreateContext(gldata->display, gldata->config, shared, ctxAttr);
  if (ctx == EGL_NO_CONTEXT)
  {
    EGLint fallback[] = { EGL_CONTEXT_MAJOR_VERSION_KHR, 2, EGL_NONE };
    ctx = eglCreateContext(gldata->display, gldata->config, shared, fallback);
  }
  return ctx;
}

static EGLSurface androidGLCanvasCreateSurface(Ihandle* ih, IGlControlData* gldata)
{
  EGLint visual_id = 0;
  EGLint surfaceAttr[3] = { EGL_NONE, EGL_NONE, EGL_NONE };

  if (!gldata->window)
    return EGL_NO_SURFACE;

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "BUFFER"), "SINGLE"))
  {
    surfaceAttr[0] = EGL_RENDER_BUFFER;
    surfaceAttr[1] = EGL_SINGLE_BUFFER;
  }

  if (eglGetConfigAttrib(gldata->display, gldata->config, EGL_NATIVE_VISUAL_ID, &visual_id) && visual_id)
    ANativeWindow_setBuffersGeometry(gldata->window, 0, 0, visual_id);

  return eglCreateWindowSurface(gldata->display, gldata->config, gldata->window, surfaceAttr);
}

static int androidGLCanvasMapMethod(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupGLCanvasHelper_create);

  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupGLCanvasHelper, jni_env, "io/github/gen2brain/iupgo/IupGLCanvasHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupGLCanvasHelper_create, jni_env, java_class,
      "create", "(J)Lio/github/gen2brain/iupgo/IupGLCanvasHelper$IupGLSurfaceView;");

  jobject view = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(jni_env, "IupGLCanvasHelper.create");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!view)
    return IUP_ERROR;

  ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, view));
  (*jni_env)->DeleteLocalRef(jni_env, view);

  iupAndroid_AddWidgetToParent(jni_env, ih);

  gldata->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (gldata->display == EGL_NO_DISPLAY)
  {
    iupAttribSet(ih, "ERROR", "Could not get EGL display.");
    return IUP_NOERROR;
  }

  {
    EGLint maj, min;
    if (!eglInitialize(gldata->display, &maj, &min))
    {
      iupAttribSetStrf(ih, "ERROR", "Could not initialize EGL. Error: 0x%X", eglGetError());
      gldata->display = EGL_NO_DISPLAY;
      return IUP_NOERROR;
    }
  }

  if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE)
  {
    iupAttribSetStrf(ih, "ERROR", "Failed to bind EGL_OPENGL_ES_API. Error: 0x%X", eglGetError());
    return IUP_NOERROR;
  }

  if (!androidGLCanvasChooseConfig(ih, gldata))
    return IUP_NOERROR;

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "VSYNC"), "NO"))
    gldata->swap_interval = 0;

  return IUP_NOERROR;
}

static void androidGLCanvasUnMapMethod(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  JNIEnv* jni_env;

  if (gldata && gldata->display != EGL_NO_DISPLAY)
  {
    if (gldata->context != EGL_NO_CONTEXT)
    {
      if (eglGetCurrentContext() == gldata->context)
        eglMakeCurrent(gldata->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      eglDestroyContext(gldata->display, gldata->context);
      gldata->context = EGL_NO_CONTEXT;
    }
    if (gldata->surface != EGL_NO_SURFACE)
    {
      eglDestroySurface(gldata->display, gldata->surface);
      gldata->surface = EGL_NO_SURFACE;
    }
    gldata->display = EGL_NO_DISPLAY;
  }

  if (gldata && gldata->window)
  {
    ANativeWindow_release(gldata->window);
    gldata->window = NULL;
  }

  if (!ih || !ih->handle) return;
  jni_env = iupAndroid_GetEnvThreadSafe();
  iupAndroid_RemoveFromParent(jni_env, ih);
  iupAndroid_ReleaseIhandle(jni_env, ih);
}

static void androidGLCanvasDestroy(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (gldata)
    free(gldata);
  iupAttribSet(ih, "_IUP_GLCONTROLDATA", NULL);
}


void iupAndroidGLCanvas_handleSurfaceCreated(Ihandle* ih, ANativeWindow* window)
{
  IGlControlData* gldata;
  if (!ih) return;
  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || gldata->display == EGL_NO_DISPLAY)
  {
    if (window) ANativeWindow_release(window);
    return;
  }

  if (gldata->window)
  {
    ANativeWindow_release(gldata->window);
    gldata->window = NULL;
  }
  gldata->window = window;

  if (gldata->surface == EGL_NO_SURFACE && gldata->window)
  {
    gldata->surface = androidGLCanvasCreateSurface(ih, gldata);
    if (gldata->surface == EGL_NO_SURFACE)
    {
      iupAttribSetStrf(ih, "ERROR", "Could not create EGL surface. Error: 0x%X", eglGetError());
      return;
    }
  }

  if (gldata->context == EGL_NO_CONTEXT)
  {
    gldata->context = androidGLCanvasCreateContext(ih, gldata);
    if (gldata->context == EGL_NO_CONTEXT)
    {
      iupAttribSetStrf(ih, "ERROR", "Could not create EGL context. Error: 0x%X", eglGetError());
      return;
    }
    iupAttribSet(ih, "CONTEXT", (char*)gldata->context);
  }

  if (eglMakeCurrent(gldata->display, gldata->surface, gldata->surface, gldata->context) == EGL_TRUE)
    eglSwapInterval(gldata->display, gldata->swap_interval);
}

void iupAndroidGLCanvas_handleSurfaceChanged(Ihandle* ih, int width, int height)
{
  IGlControlData* gldata;
  IFnii cb;
  Icallback action_cb;
  EGLint cur_w = 0, cur_h = 0;
  if (!ih) return;

  /* eglCreateWindowSurface locks to the ANativeWindow's initial size; recreate so the EGL framebuffer tracks SurfaceView resizes. */
  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (gldata && gldata->display != EGL_NO_DISPLAY && gldata->surface != EGL_NO_SURFACE)
  {
    eglQuerySurface(gldata->display, gldata->surface, EGL_WIDTH,  &cur_w);
    eglQuerySurface(gldata->display, gldata->surface, EGL_HEIGHT, &cur_h);
    if (cur_w != width || cur_h != height)
    {
      if (eglGetCurrentSurface(EGL_DRAW) == gldata->surface ||
          eglGetCurrentSurface(EGL_READ) == gldata->surface)
        eglMakeCurrent(gldata->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      eglDestroySurface(gldata->display, gldata->surface);
      gldata->surface = androidGLCanvasCreateSurface(ih, gldata);
      if (gldata->surface != EGL_NO_SURFACE && gldata->context != EGL_NO_CONTEXT)
        eglMakeCurrent(gldata->display, gldata->surface, gldata->surface, gldata->context);
    }
  }

  cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (cb) cb(ih, width, height);

  /* SurfaceView has no implicit paint trigger; dispatch ACTION on every surface change. */
  action_cb = IupGetCallback(ih, "ACTION");
  if (action_cb) action_cb(ih);
}

void iupAndroidGLCanvas_handleSurfaceDestroyed(Ihandle* ih)
{
  IGlControlData* gldata;
  if (!ih) return;
  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || gldata->display == EGL_NO_DISPLAY) return;

  if (eglGetCurrentSurface(EGL_DRAW) == gldata->surface ||
      eglGetCurrentSurface(EGL_READ) == gldata->surface)
    eglMakeCurrent(gldata->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

  if (gldata->surface != EGL_NO_SURFACE)
  {
    eglDestroySurface(gldata->display, gldata->surface);
    gldata->surface = EGL_NO_SURFACE;
  }

  if (gldata->window)
  {
    ANativeWindow_release(gldata->window);
    gldata->window = NULL;
  }
}


IUPGL_API int IupGLIsCurrent(Ihandle* ih)
{
  IGlControlData* gldata;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih)) return 0;
  if (ih->iclass->nativetype != IUP_TYPECANVAS || !IupClassMatch(ih, "glcanvas")) return 0;

  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || gldata->context == EGL_NO_CONTEXT) return 0;
  return (gldata->context == eglGetCurrentContext()) ? 1 : 0;
}

IUPGL_API void IupGLMakeCurrent(Ihandle* ih)
{
  IGlControlData* gldata;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih)) return;
  if (ih->iclass->nativetype != IUP_TYPECANVAS || !IupClassMatch(ih, "glcanvas")) return;

  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || gldata->display == EGL_NO_DISPLAY) return;
  if (gldata->surface == EGL_NO_SURFACE || gldata->context == EGL_NO_CONTEXT) return;

  if (eglMakeCurrent(gldata->display, gldata->surface, gldata->surface, gldata->context) == EGL_FALSE)
  {
    iupAttribSetStrf(ih, "ERROR", "Failed to make current. Error: 0x%X", eglGetError());
    return;
  }

  iupAttribSet(ih, "ERROR", NULL);

  if (!IupGetGlobal("GL_VERSION"))
  {
    IupSetStrGlobal("GL_VENDOR", (char*)glGetString(GL_VENDOR));
    IupSetStrGlobal("GL_RENDERER", (char*)glGetString(GL_RENDERER));
    IupSetStrGlobal("GL_VERSION", (char*)glGetString(GL_VERSION));
  }
}

IUPGL_API void IupGLSwapBuffers(Ihandle* ih)
{
  IGlControlData* gldata;
  Icallback cb;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih)) return;
  if (ih->iclass->nativetype != IUP_TYPECANVAS || !IupClassMatch(ih, "glcanvas")) return;

  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || gldata->display == EGL_NO_DISPLAY || gldata->surface == EGL_NO_SURFACE) return;

  cb = IupGetCallback(ih, "SWAPBUFFERS_CB");
  if (cb) cb(ih);

  eglSwapBuffers(gldata->display, gldata->surface);
}

IUPGL_API void* IupGLGetProcAddress(const char* name)
{
  return (void*)eglGetProcAddress(name);
}

IUPGL_API void IupGLPalette(Ihandle* ih, int index, float r, float g, float b)
{
  (void)ih; (void)index; (void)r; (void)g; (void)b;
}

IUPGL_API void IupGLUseFont(Ihandle* ih, int first, int count, int list_base)
{
  (void)ih; (void)first; (void)count; (void)list_base;
}

IUPGL_API void IupGLWait(int gl)
{
  if (gl) eglWaitClient();
  else    eglWaitNative(EGL_CORE_NATIVE_ENGINE);
}


void iupGlCanvasInitClass(Iclass* ic)
{
  ic->Create  = androidGLCanvasCreateMethod;
  ic->Destroy = androidGLCanvasDestroy;
  ic->Map     = androidGLCanvasMapMethod;
  ic->UnMap   = androidGLCanvasUnMapMethod;
}
