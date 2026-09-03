/** \file
 * \brief Android Location (LocationManager)
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <jni.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_location.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"
#include "iupandroid_jnicacheglobals.h"

IUPJNI_DECLARE_CLASS_STATIC(IupLocationHelper);

static jclass androidLocationClass(JNIEnv* jni_env)
{
  return IUPJNI_FindClass(IupLocationHelper, jni_env, "io/github/gen2brain/iupgo/IupLocationHelper");
}

IUP_SDK_API int iupdrvLocationIsAvailable(void)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidLocationClass(jni_env);
  jmethodID m;
  jboolean ret;
  if (!cls) return 0;
  m = (*jni_env)->GetStaticMethodID(jni_env, cls, "isAvailable", "()Z");
  ret = (*jni_env)->CallStaticBooleanMethod(jni_env, cls, m);
  iupAndroid_CheckException(jni_env, "IupLocationHelper.isAvailable");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return ret? 1: 0;
}

IUP_SDK_API int iupdrvLocationStart(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidLocationClass(jni_env);
  jmethodID m;
  jboolean fine = iupStrEqualNoCase(iupAttribGetStr(ih, "ACCURACY"), "FINE")? JNI_TRUE: JNI_FALSE;
  jboolean ret;
  if (!cls) return 0;
  m = (*jni_env)->GetStaticMethodID(jni_env, cls, "start", "(JZJF)Z");
  ret = (*jni_env)->CallStaticBooleanMethod(jni_env, cls, m, (jlong)(intptr_t)ih, fine,
    (jlong)iupAttribGetInt(ih, "INTERVAL"), (jfloat)iupAttribGetDouble(ih, "DISTANCE"));
  iupAndroid_CheckException(jni_env, "IupLocationHelper.start");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return ret? 1: 0;
}

IUP_SDK_API void iupdrvLocationStop(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidLocationClass(jni_env);
  jmethodID m;
  if (!cls) return;
  m = (*jni_env)->GetStaticMethodID(jni_env, cls, "stop", "(J)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(jni_env, "IupLocationHelper.stop");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

IUP_SDK_API char* iupdrvLocationGetPermission(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidLocationClass(jni_env);
  jmethodID m;
  jint state;
  (void)ih;
  if (!cls) return "UNAVAILABLE";
  m = (*jni_env)->GetStaticMethodID(jni_env, cls, "permissionState", "()I");
  state = (*jni_env)->CallStaticIntMethod(jni_env, cls, m);
  iupAndroid_CheckException(jni_env, "IupLocationHelper.permissionState");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  if (state == 1) return "GRANTED";
  if (state == 2) return "DENIED";
  if (state == 3) return "UNAVAILABLE";
  return "PROMPT";
}

IUP_SDK_API void iupdrvLocationDestroy(Ihandle* ih)
{
  iupdrvLocationStop(ih);
}

IUP_SDK_API void iupdrvLocationInitClass(Iclass* ic)
{
  (void)ic;
}
