/** \file
 * \brief Android Sensor (SensorManager)
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
#include "iup_sensor.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"
#include "iupandroid_jnicacheglobals.h"

IUPJNI_DECLARE_CLASS_STATIC(IupSensorHelper);

static jclass androidSensorClass(JNIEnv* jni_env)
{
  return IUPJNI_FindClass(IupSensorHelper, jni_env, "io/github/gen2brain/iupgo/IupSensorHelper");
}

IUP_SDK_API int iupdrvSensorIsAvailable(int type)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidSensorClass(jni_env);
  jmethodID m;
  jboolean ret;
  if (!cls) return 0;
  m = (*jni_env)->GetStaticMethodID(jni_env, cls, "isAvailable", "(I)Z");
  ret = (*jni_env)->CallStaticBooleanMethod(jni_env, cls, m, (jint)type);
  iupAndroid_CheckException(jni_env, "IupSensorHelper.isAvailable");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return ret? 1: 0;
}

IUP_SDK_API int iupdrvSensorStart(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidSensorClass(jni_env);
  jmethodID m;
  jboolean ret;
  if (!cls) return 0;
  m = (*jni_env)->GetStaticMethodID(jni_env, cls, "start", "(JII)Z");
  ret = (*jni_env)->CallStaticBooleanMethod(jni_env, cls, m, (jlong)(intptr_t)ih, (jint)iupSensorGetType(ih), (jint)iupAttribGetInt(ih, "INTERVAL"));
  iupAndroid_CheckException(jni_env, "IupSensorHelper.start");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return ret? 1: 0;
}

IUP_SDK_API void iupdrvSensorStop(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidSensorClass(jni_env);
  jmethodID m;
  if (!cls) return;
  m = (*jni_env)->GetStaticMethodID(jni_env, cls, "stop", "(J)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(jni_env, "IupSensorHelper.stop");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

IUP_SDK_API char* iupdrvSensorGetPermission(Ihandle* ih)
{
  return iupdrvSensorIsAvailable(iupSensorGetType(ih))? "GRANTED": "UNAVAILABLE";
}

IUP_SDK_API void iupdrvSensorDestroy(Ihandle* ih)
{
  iupdrvSensorStop(ih);
}

IUP_SDK_API void iupdrvSensorInitClass(Iclass* ic)
{
  (void)ic;
}
