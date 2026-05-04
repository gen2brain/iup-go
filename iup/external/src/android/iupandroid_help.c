/** \file
 * \brief Android Driver IupHelp/IupExecute
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <jni.h>

#include "iup.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupHelpHelper);

/* True if the string starts with an RFC-3986 scheme followed by ':'. */
static int androidHelpLooksLikeUrl(const char* s)
{
  for (const char* p = s; *p; p++)
  {
    if (*p == ':')
      return (p != s);
    if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
          (*p >= '0' && *p <= '9') || *p == '+' || *p == '-' || *p == '.'))
      return 0;
  }
  return 0;
}

static int androidHelpOpenUrl(const char* url)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupHelpHelper, jni_env, "io/github/gen2brain/iupgo/IupHelpHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "openUrl", "(Ljava/lang/String;)I");

  jstring j_url = (*jni_env)->NewStringUTF(jni_env, url);
  jint ret = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id, j_url);
  iupAndroid_CheckException(jni_env, "IupHelpHelper.openUrl");

  (*jni_env)->DeleteLocalRef(jni_env, j_url);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return (int)ret;
}

static int androidHelpLaunchPackage(const char* package_name)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupHelpHelper, jni_env, "io/github/gen2brain/iupgo/IupHelpHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "launchPackage", "(Ljava/lang/String;)I");

  jstring j_pkg = (*jni_env)->NewStringUTF(jni_env, package_name);
  jint ret = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id, j_pkg);
  iupAndroid_CheckException(jni_env, "IupHelpHelper.launchPackage");

  (*jni_env)->DeleteLocalRef(jni_env, j_pkg);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return (int)ret;
}

int IupHelp(const char* url)
{
  if (!url || !*url) return -1;
  return androidHelpOpenUrl(url);
}

int IupExecute(const char* filename, const char* parameters)
{
  if (!filename || !*filename) return -1;

  if (androidHelpLooksLikeUrl(filename))
  {
    /* Tack parameters on as a query suffix. */
    if (parameters && *parameters)
    {
      size_t len = strlen(filename) + strlen(parameters) + 2;
      char* full = (char*)malloc(len);
      if (!full) return -1;
      snprintf(full, len, "%s%s%s", filename,
        strchr(filename, '?') ? "&" : "?", parameters);
      int ret = androidHelpOpenUrl(full);
      free(full);
      return ret;
    }
    return androidHelpOpenUrl(filename);
  }

  /* Otherwise treat as a package name. */
  return androidHelpLaunchPackage(filename);
}

int IupExecuteWait(const char* filename, const char* parameters)
{
  /* Android Intents are fire-and-forget. */
  (void)filename;
  (void)parameters;
  return -1;
}
