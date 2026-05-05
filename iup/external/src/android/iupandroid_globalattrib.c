/** \file
 * \brief Android Driver iupdrvSetGlobal/iupdrvGetGlobal
 *
 * Most IUP globals (SYSTEMVERSION, SCREENSIZE, DPI, etc.) flow through the
 * iup_drvinfo.h interface in iupandroid_info.c. Only attributes that need
 * platform-specific SET semantics live here.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>

#include <jni.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_strmessage.h"
#include "iup_singleinstance.h"

#include "iupandroid_drv.h"


static char* androidGetStringStatic(const char* method_name)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = (*jni_env)->FindClass(jni_env, "io/github/gen2brain/iupgo/IupCommon");
  if (!java_class) return NULL;
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, "()Ljava/lang/String;");
  jstring j_string = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id);
  iupAndroid_CheckException(jni_env, method_name);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return iupAndroid_JStringToReturnStr(jni_env, j_string);
}

static int androidGetIntStatic(const char* method_name)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = (*jni_env)->FindClass(jni_env, "io/github/gen2brain/iupgo/IupCommon");
  if (!java_class) return 0;
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, "()I");
  jint val = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id);
  iupAndroid_CheckException(jni_env, method_name);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return (int)val;
}

static int androidGetBoolStatic(const char* method_name)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = (*jni_env)->FindClass(jni_env, "io/github/gen2brain/iupgo/IupCommon");
  if (!java_class) return 0;
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, "()Z");
  jboolean val = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, method_id);
  iupAndroid_CheckException(jni_env, method_name);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return val ? 1 : 0;
}

static char* androidGetVirtualScreen(void)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = (*jni_env)->FindClass(jni_env, "io/github/gen2brain/iupgo/IupCommon");
  if (!java_class) return NULL;
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getVirtualScreen", "()[I");
  jintArray arr = (jintArray)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id);
  iupAndroid_CheckException(jni_env, "getVirtualScreen");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  if (!arr) return NULL;
  jint vals[4] = {0, 0, 0, 0};
  (*jni_env)->GetIntArrayRegion(jni_env, arr, 0, 4, vals);
  (*jni_env)->DeleteLocalRef(jni_env, arr);
  return iupStrReturnStrf("%d %d %d %d", (int)vals[0], (int)vals[1], (int)vals[2], (int)vals[3]);
}


static void androidSetInputCallbacks(int enabled)
{
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = (*env)->FindClass(env, "io/github/gen2brain/iupgo/IupCommon");
  if (!cls) return;
  jfieldID fld = (*env)->GetStaticFieldID(env, cls, "inputCallbacksEnabled", "Z");
  if (fld) (*env)->SetStaticBooleanField(env, cls, fld, enabled ? JNI_TRUE : JNI_FALSE);
  iupAndroid_CheckException(env, "IupCommon.inputCallbacksEnabled");
  (*env)->DeleteLocalRef(env, cls);
}

int iupdrvSetGlobal(const char* name, const char* value)
{
  if (iupStrEqual(name, "LANGUAGE"))
  {
    iupStrMessageUpdateLanguage(value);
    return 1;
  }
  if (iupStrEqual(name, "SINGLEINSTANCE"))
  {
    if (iupdrvSingleInstanceSet(value))
      return 0;
    else
      return 1;
  }
  if (iupStrEqual(name, "INPUTCALLBACKS"))
  {
    androidSetInputCallbacks(iupStrBoolean(value));
    return 1;
  }
  return 1;
}

char* iupdrvGetGlobal(const char* name)
{
  if (iupStrEqual(name, "APPID"))          return androidGetStringStatic("getAppId");
  if (iupStrEqual(name, "APPNAME"))        return androidGetStringStatic("getAppName");
  if (iupStrEqual(name, "VIRTUALSCREEN"))  return androidGetVirtualScreen();
  if (iupStrEqual(name, "MONITORSCOUNT"))  return iupStrReturnInt(androidGetIntStatic("getMonitorsCount"));
  if (iupStrEqual(name, "MONITORSINFO"))   return androidGetStringStatic("getMonitorsInfo");
  if (iupStrEqual(name, "TRUECOLORCANVAS")) return iupStrReturnBoolean(iupdrvGetScreenDepth() > 8);
  if (iupStrEqual(name, "UTF8MODE"))       return iupStrReturnBoolean(1);
  if (iupStrEqual(name, "DARKMODE"))       return iupStrReturnBoolean(androidGetBoolStatic("isDarkMode"));
  return NULL;
}
