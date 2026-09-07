/** \file
 * \brief Media controls, runtime permissions through IupPermissionHelper
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>
#include <stdint.h>
#include <string.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_media.h"
#include "iup_camera.h"
#include "iup_microphone.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"

IUPJNI_DECLARE_CLASS_STATIC(IupPermissionHelper);

static jclass androidMediaClass(JNIEnv* jni_env)
{
  return IUPJNI_FindClass(IupPermissionHelper, jni_env, "io/github/gen2brain/iupgo/IupPermissionHelper");
}

int iupandroidMediaPermissionState(const char* permission)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidMediaClass(jni_env);
  jstring name;
  jmethodID m;
  jint state;

  if (!cls)
    return -1;

  name = (*jni_env)->NewStringUTF(jni_env, permission);
  m = (*jni_env)->GetStaticMethodID(jni_env, cls, "permissionState", "(Ljava/lang/String;)I");
  state = (*jni_env)->CallStaticIntMethod(jni_env, cls, m, name);
  iupAndroid_CheckException(jni_env, "IupPermissionHelper.permissionState");
  (*jni_env)->DeleteLocalRef(jni_env, name);
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return (int)state;
}

void iupandroidMediaRequestPermission(const char* permission, Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidMediaClass(jni_env);
  jstring name;
  jmethodID m;

  if (!cls)
    return;

  name = (*jni_env)->NewStringUTF(jni_env, permission);
  m = (*jni_env)->GetStaticMethodID(jni_env, cls, "requestPermission", "(Ljava/lang/String;J)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, name, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(jni_env, "IupPermissionHelper.requestPermission");
  (*jni_env)->DeleteLocalRef(jni_env, name);
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupPermissionHelper_dispatchPermission(JNIEnv* jni_env, jclass cls, jstring permission, jlong ihandle_ptr, jboolean granted)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  const char* name;
  (void)cls;

  if (!ih || !iupObjectCheck(ih))
    return;

  name = (*jni_env)->GetStringUTFChars(jni_env, permission, NULL);
  if (name && strcmp(name, "android.permission.CAMERA") == 0)
    iupandroidCameraPermission(ih, granted ? 1 : 0);
  else if (name && strcmp(name, "android.permission.RECORD_AUDIO") == 0)
    iupandroidMicrophonePermission(ih, granted ? 1 : 0);
  if (name)
    (*jni_env)->ReleaseStringUTFChars(jni_env, permission, name);
}
