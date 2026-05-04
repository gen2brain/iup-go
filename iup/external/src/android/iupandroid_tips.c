/** \file
 * \brief Android Driver TIPS management
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>

#include <jni.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_str.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupTipHelper);

int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTipHelper, jni_env, "io/github/gen2brain/iupgo/IupTipHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setTip", "(Ljava/lang/Object;Ljava/lang/String;)V");
  jstring j_text = value ? (*jni_env)->NewStringUTF(jni_env, value) : NULL;
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jobject)ih->handle, j_text);
  iupAndroid_CheckException(jni_env, "IupTipHelper.setTip");
  if (j_text) (*jni_env)->DeleteLocalRef(jni_env, j_text);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}

int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle) return 0;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTipHelper, jni_env, "io/github/gen2brain/iupgo/IupTipHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setTipVisible", "(Ljava/lang/Object;Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jobject)ih->handle, (jboolean)(iupStrBoolean(value) ? JNI_TRUE : JNI_FALSE));
  iupAndroid_CheckException(jni_env, "IupTipHelper.setTipVisible");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 0;
}

char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  /* Android has no public API to query tooltip visibility. */
  (void)ih;
  return NULL;
}
