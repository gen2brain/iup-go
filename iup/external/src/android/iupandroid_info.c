/** \file
 * \brief Android System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>

#include <jni.h>
#include <android/log.h>

#include "iup.h"
#include "iup_varg.h"

#include "iup_str.h"
#include "iup_drvinfo.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"
#include "iupandroid_jnicacheglobals.h"


/* First-query cache; IUP uses these for layout, not live measurement. */
static int s_screen_w = 0, s_screen_h = 0, s_screen_dpi = 0;
static int s_full_w = 0, s_full_h = 0;


static void androidQueryDisplayMetrics(void)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");

  jmethodID m_display = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getDisplayMetrics", "()[I");
  jintArray arr = (jintArray)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, m_display);
  iupAndroid_CheckException(jni_env, "IupCommon.getDisplayMetrics");
  if (arr)
  {
    jint vals[3];
    (*jni_env)->GetIntArrayRegion(jni_env, arr, 0, 3, vals);
    s_screen_w = vals[0];
    s_screen_h = vals[1];
    s_screen_dpi = vals[2];
    (*jni_env)->DeleteLocalRef(jni_env, arr);
  }

  jmethodID m_real = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getRealDisplayMetrics", "()[I");
  arr = (jintArray)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, m_real);
  iupAndroid_CheckException(jni_env, "IupCommon.getRealDisplayMetrics");
  if (arr)
  {
    jint vals[2];
    (*jni_env)->GetIntArrayRegion(jni_env, arr, 0, 2, vals);
    s_full_w = vals[0];
    s_full_h = vals[1];
    (*jni_env)->DeleteLocalRef(jni_env, arr);
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static char* androidCallStringStatic(const char* method_name)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, "()Ljava/lang/String;");
  jstring j_string = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id);
  iupAndroid_CheckException(jni_env, method_name);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return iupAndroid_JStringToReturnStr(jni_env, j_string);
}


void iupdrvAddScreenOffset(int* x, int* y, int add)
{
  /* Only X11 supports desktop origin offsets. */
  (void)x;
  (void)y;
  (void)add;
}

void iupdrvGetScreenSize(int* width, int* height)
{
  if (s_screen_w == 0) androidQueryDisplayMetrics();
  if (width) *width = s_screen_w;
  if (height) *height = s_screen_h;
}

void iupdrvGetFullSize(int* width, int* height)
{
  if (s_full_w == 0) androidQueryDisplayMetrics();
  if (width) *width = s_full_w;
  if (height) *height = s_full_h;
}

int iupdrvGetScreenDepth(void)
{
  /* Android window surfaces are effectively always 32-bit. */
  return 32;
}

double iupdrvGetScreenDpi(void)
{
  if (s_screen_dpi == 0) androidQueryDisplayMetrics();
  return (double)s_screen_dpi;
}

int iupdrvScaleNaturalPx(int px)
{
  return iupAndroid_DpToPx((float)px);
}

void iupdrvGetCursorPos(int* x, int* y)
{
  if (x) *x = 0;
  if (y) *y = 0;

  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = (*env)->FindClass(env, "io/github/gen2brain/iupgo/IupActivity");
  if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); return; }
  if (!cls) return;

  jmethodID mx = (*env)->GetStaticMethodID(env, cls, "getLastTouchX", "()I");
  jmethodID my = (*env)->GetStaticMethodID(env, cls, "getLastTouchY", "()I");
  /* Device px to match the IUP layout coord space (currentwidth/height, ih->data->w/h on Android are device px). */
  if (mx && x) *x = (int)(*env)->CallStaticIntMethod(env, cls, mx);
  if (my && y) *y = (int)(*env)->CallStaticIntMethod(env, cls, my);
  iupAndroid_CheckException(env, "IupActivity.getLastTouch");
  (*env)->DeleteLocalRef(env, cls);
}

void iupdrvGetKeyState(char* key)
{
  /* Modifier state is only meaningful during key events on Android. */
  key[0] = ' ';
  key[1] = ' ';
  key[2] = ' ';
  key[3] = ' ';
  key[4] = 0;
}

char* iupdrvGetSystemName(void)
{
  return "Android";
}

char* iupdrvGetSystemVersion(void)
{
  return androidCallStringStatic("getSystemVersion");
}

char* iupdrvGetComputerName(void)
{
  /* Use Manufacturer + Model as the device identifier. */
  return androidCallStringStatic("getDeviceName");
}

char* iupdrvGetUserName(void)
{
  return androidCallStringStatic("getUserName");
}

int iupdrvGetPreferencePath(char* filename, const char* app_name, int use_system)
{
  JNIEnv* jni_env;
  jmethodID method_id;
  jclass java_class;
  jobject context_object;
  jobject file_object;
  jstring j_path_string;
  const char* c_path_string = NULL;

  filename[0] = '\0';

  if (!app_name || !app_name[0])
    return 0;

  jni_env = iupAndroid_GetEnvThreadSafe();

  /* Application object doubles as a Context. */
  context_object = iupAndroid_GetApplication(jni_env);

  java_class = (*jni_env)->GetObjectClass(jni_env, context_object);

  method_id = (*jni_env)->GetMethodID(jni_env, java_class, "getFilesDir", "()Ljava/io/File;");
  file_object = (*jni_env)->CallObjectMethod(jni_env, context_object, method_id);
  if (file_object == NULL)
  {
    __android_log_print(ANDROID_LOG_ERROR, "Iup", "iupdrvGetPreferencePath: context.getFilesDir() failed");
    (*jni_env)->DeleteLocalRef(jni_env, java_class);
    (*jni_env)->DeleteLocalRef(jni_env, context_object);
    return 0;
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  java_class = (*jni_env)->GetObjectClass(jni_env, file_object);

  method_id = (*jni_env)->GetMethodID(jni_env, java_class, "getAbsolutePath", "()Ljava/lang/String;");
  j_path_string = (jstring)(*jni_env)->CallObjectMethod(jni_env, file_object, method_id);

  c_path_string = (*jni_env)->GetStringUTFChars(jni_env, j_path_string, NULL);

  /* use_system: <files>/app_name/config ; legacy: <files>/.app_name */
  int ok = 1;
  if (use_system)
  {
    if (snprintf(filename, 10240, "%s/%s", c_path_string, app_name) >= 10240) ok = 0;
    if (ok) mkdir(filename, 0755);
    if (ok && snprintf(filename + strlen(filename), 10240 - strlen(filename), "/config") >= (int)(10240 - strlen(filename))) ok = 0;
  }
  else
  {
    if (snprintf(filename, 10240, "%s/.%s", c_path_string, app_name) >= 10240) ok = 0;
  }

  if (!ok) filename[0] = '\0';

  (*jni_env)->ReleaseStringUTFChars(jni_env, j_path_string, c_path_string);
  (*jni_env)->DeleteLocalRef(jni_env, j_path_string);
  (*jni_env)->DeleteLocalRef(jni_env, file_object);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  (*jni_env)->DeleteLocalRef(jni_env, context_object);

  return ok;
}

char* iupdrvLocaleInfo(void)
{
  return androidCallStringStatic("getLocaleTag");
}

char* iupdrvGetCurrentDirectory(void)
{
  char* cwd = getcwd(NULL, 0);
  if (!cwd) return NULL;
  char* ret = iupStrReturnStr(cwd);
  free(cwd);
  return ret;
}

int iupdrvSetCurrentDirectory(const char* dir)
{
  return chdir(dir) == 0;
}

void IupLogV(const char* type, const char* format, va_list arglist)
{
  int priority = ANDROID_LOG_DEFAULT;

  if (iupStrEqualNoCase(type, "DEBUG"))
    priority = ANDROID_LOG_DEBUG;
  else if (iupStrEqualNoCase(type, "ERROR"))
    priority = ANDROID_LOG_ERROR;
  else if (iupStrEqualNoCase(type, "WARNING"))
    priority = ANDROID_LOG_WARN;
  else if (iupStrEqualNoCase(type, "INFO"))
    priority = ANDROID_LOG_INFO;
  else if (iupStrEqualNoCase(type, "FATAL"))
    priority = ANDROID_LOG_FATAL;
  else if (iupStrEqualNoCase(type, "VERBOSE"))
    priority = ANDROID_LOG_VERBOSE;

  __android_log_vprint(priority, "IupLog", format, arglist);
}

void IupLog(const char* type, const char* format, ...)
{
  va_list arglist;
  va_start(arglist, format);
  IupLogV(type, format, arglist);
  va_end(arglist);
}
