/** \file
 * \brief JNI bridge: IupGLCanvasHelper (SurfaceHolder lifecycle -> EGL surface/context).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>
#include <stdint.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "iup.h"
#include "iup_object.h"

#include "iupandroid_drv.h"


/* Implemented in iup_glcanvas_android.c. */
extern void iupAndroidGLCanvas_handleSurfaceCreated(Ihandle* ih, ANativeWindow* window);
extern void iupAndroidGLCanvas_handleSurfaceChanged(Ihandle* ih, int width, int height);
extern void iupAndroidGLCanvas_handleSurfaceDestroyed(Ihandle* ih);


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupGLCanvasHelper_dispatchSurfaceCreated(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jobject surface)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  ANativeWindow* window;
  if (!ih || !surface) return;

  window = ANativeWindow_fromSurface(jni_env, surface);
  iupAndroidGLCanvas_handleSurfaceCreated(ih, window);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupGLCanvasHelper_dispatchSurfaceChanged(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint width, jint height)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih) return;
  iupAndroidGLCanvas_handleSurfaceChanged(ih, (int)width, (int)height);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupGLCanvasHelper_dispatchSurfaceDestroyed(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih) return;
  iupAndroidGLCanvas_handleSurfaceDestroyed(ih);
}
