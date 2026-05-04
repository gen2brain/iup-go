/** \file
 * \brief Android Focus
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>

#include <jni.h>

#include "iup.h"
#include "iup_drv.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"
#include "iupandroid_jnicacheglobals.h"


void iupdrvSetFocus(Ihandle* ih)
{
  jobject widget = iupAndroid_RealNativeHandle(ih);
  if (!widget) return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setFocus", "(Ljava/lang/Object;)V");

  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, widget);
  iupAndroid_CheckException(jni_env, "IupCommon.setFocus");

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}
