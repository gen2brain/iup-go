/** \file
 * \brief Timer for the Android Driver.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdint.h>

#include <jni.h>
#include <android/log.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_timer.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupTimerHelper);

void iupdrvTimerRun(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTimer_createTimer);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTimer_startTimer);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTimerHelper, jni_env, "io/github/gen2brain/iupgo/IupTimerHelper");
  jmethodID method_id;
  jobject java_widget;

  if (ih->handle == NULL)
  {
    method_id = IUPJNI_GetStaticMethodID(IupTimer_createTimer, jni_env, java_class, "createTimer", "(J)Lio/github/gen2brain/iupgo/IupTimerHelper$IupTimer;");
    java_widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih);
    iupAndroid_CheckException(jni_env, "IupTimerHelper.createTimer");
    ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, java_widget));
  }
  else
  {
    java_widget = (*jni_env)->NewLocalRef(jni_env, ih->handle);
  }

  unsigned int time_ms = iupAttribGetInt(ih, "TIME");
  if (time_ms > 0)
  {
    method_id = IUPJNI_GetStaticMethodID(IupTimer_startTimer, jni_env, java_class, "startTimer", "(JLio/github/gen2brain/iupgo/IupTimerHelper$IupTimer;J)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, java_widget, (jlong)time_ms);
    iupAndroid_CheckException(jni_env, "IupTimerHelper.startTimer");
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_widget);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvTimerStop(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTimer_stopTimer);

  if (ih->handle != NULL)
  {
    JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
    jclass java_class = IUPJNI_FindClass(IupTimerHelper, jni_env, "io/github/gen2brain/iupgo/IupTimerHelper");
    jmethodID method_id = IUPJNI_GetStaticMethodID(IupTimer_stopTimer, jni_env, java_class, "stopTimer", "(JLio/github/gen2brain/iupgo/IupTimerHelper$IupTimer;)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jobject)ih->handle);
    iupAndroid_CheckException(jni_env, "IupTimerHelper.stopTimer");

    (*jni_env)->DeleteLocalRef(jni_env, java_class);
  }
}

static void androidTimerUnMapMethod(Ihandle* ih)
{
  iupdrvTimerStop(ih);

  if (ih->handle != NULL)
  {
    JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
    (*jni_env)->DeleteGlobalRef(jni_env, (jobject)ih->handle);
    ih->handle = NULL;
  }
}

void iupdrvTimerInitClass(Iclass* ic)
{
  /* UnMap, not Destroy: ih->handle is cleared before Destroy would run */
  ic->UnMap = androidTimerUnMapMethod;
}
