/** \file
 * \brief Android Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <unistd.h>

#include <jni.h>
#include <pthread.h>
#include <android/log.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_focus.h"
#include "iup_image.h"
#include "iup_drv.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"
#include "iupandroid_jnicacheglobals.h"


static JavaVM* s_javaVM = NULL;
static pthread_key_t s_attachThreadKey;

/* Pre-cached from JNI_OnLoad: background-thread FindClass can't reach app classes. */
IUPJNI_DECLARE_CLASS_STATIC(IupApplication);
IUPJNI_DECLARE_CLASS_STATIC(IupPostMessage);
/* Global so iupandroid_font.c's iupdrvGetSystemFont hits a populated cache. */
IUPJNI_DECLARE_CLASS_GLOBAL(IupFontHelper);

static void iupAndroid_ThreadDestroyed(void* user_data)
{
  /* fires when an attached pthread exits; detach + clear TLS to avoid double-detach */
  JNIEnv* jni_env = (JNIEnv*)user_data;
  if (jni_env != NULL)
  {
    (*s_javaVM)->DetachCurrentThread(s_javaVM);
    pthread_setspecific(s_attachThreadKey, NULL);
  }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* java_vm, void* reserved)
{
  (void)reserved;

  s_javaVM = java_vm;

  /* TLS JNIEnv per thread so we can detach on thread exit. */
  if (pthread_key_create(&s_attachThreadKey, iupAndroid_ThreadDestroyed) != 0)
  {
    __android_log_print(ANDROID_LOG_ERROR, "Iup", "Error initializing pthread key");
  }

  iupAndroid_OpenInit();

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();

  /* Seed the cache from the correct class loader; later lookups are hits. */
  jclass tmp;
  tmp = IUPJNI_FindClass(IupApplication, jni_env, "io/github/gen2brain/iupgo/IupApplication");
  (*jni_env)->DeleteLocalRef(jni_env, tmp);
  tmp = IUPJNI_FindClass(IupPostMessage, jni_env, "io/github/gen2brain/iupgo/IupPostMessage");
  (*jni_env)->DeleteLocalRef(jni_env, tmp);
  tmp = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  (*jni_env)->DeleteLocalRef(jni_env, tmp);
  tmp = IUPJNI_FindClass(IupFontHelper, jni_env, "io/github/gen2brain/iupgo/IupFontHelper");
  (*jni_env)->DeleteLocalRef(jni_env, tmp);

  return JNI_VERSION_1_6;
}

/* Attaches the calling thread if needed; TLS key detaches on thread exit. */
JNIEnv* iupAndroid_GetEnvThreadSafe(void)
{
  JNIEnv* jni_env = NULL;

  int get_env_stat = (*s_javaVM)->GetEnv(s_javaVM, (void**)&jni_env, JNI_VERSION_1_6);
  if (get_env_stat == JNI_EDETACHED)
  {
    jint attach_status = (*s_javaVM)->AttachCurrentThread(s_javaVM, &jni_env, NULL);
    if (attach_status != 0)
    {
      __android_log_print(ANDROID_LOG_ERROR, "Iup", "AttachCurrentThread failed");
    }

    /* destructor only fires when the TLS value is non-NULL */
    pthread_setspecific(s_attachThreadKey, (void*)jni_env);
  }
  else if (get_env_stat == JNI_EVERSION)
  {
    __android_log_print(ANDROID_LOG_ERROR, "Iup", "GetEnv version not supported");
  }

  return jni_env;
}

void iupAndroid_CheckException(JNIEnv* jni_env, const char* tag)
{
  if (!(*jni_env)->ExceptionCheck(jni_env))
    return;
  __android_log_print(ANDROID_LOG_ERROR, "Iup", "JNI exception in %s", tag ? tag : "<no tag>");
  (*jni_env)->ExceptionDescribe(jni_env);
  (*jni_env)->ExceptionClear(jni_env);
}

void iupAndroid_RetainIhandle(JNIEnv* jni_env, jobject native_widget, Ihandle* ih)
{
  if (ih)
  {
    ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, native_widget));
  }
}

void iupAndroid_ReleaseIhandle(JNIEnv* jni_env, Ihandle* ih)
{
  if (ih && ih->handle)
  {
    (*jni_env)->DeleteGlobalRef(jni_env, ih->handle);
    ih->handle = NULL;
  }
}

/* IupApplication singleton, cached as a global ref; returns a fresh LocalRef. */
jobject iupAndroid_GetApplication(JNIEnv* jni_env)
{
  static jobject s_applicationObject = NULL;
  jobject ret_object = NULL;

  if (s_applicationObject == NULL)
  {
    IUPJNI_DECLARE_METHOD_ID_STATIC(IupApplication_getIupApplication);
    jclass java_class;
    jmethodID method_id;

    java_class = IUPJNI_FindClass(IupApplication, jni_env, "io/github/gen2brain/iupgo/IupApplication");
    method_id = IUPJNI_GetStaticMethodID(IupApplication_getIupApplication, jni_env, java_class, "getIupApplication", "()Lio/github/gen2brain/iupgo/IupApplication;");
    ret_object = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id);
    iupAndroid_CheckException(jni_env, "IupApplication.getIupApplication");

    s_applicationObject = (*jni_env)->NewGlobalRef(jni_env, ret_object);
    (*jni_env)->DeleteLocalRef(jni_env, java_class);
  }
  else
  {
    ret_object = (*jni_env)->NewLocalRef(jni_env, s_applicationObject);
  }

  /* Caller owns the returned LocalRef and must DeleteLocalRef when done. */
  return ret_object;
}

/* not cached: current Activity changes over the app lifetime (back, new Intent, etc) */
jobject iupAndroid_GetCurrentActivity(JNIEnv* jni_env)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupApplication_getCurrentActivity);
  jclass java_class;
  jmethodID method_id;
  jobject ret_object = NULL;

  jobject application_object = iupAndroid_GetApplication(jni_env);

  java_class = IUPJNI_FindClass(IupApplication, jni_env, "io/github/gen2brain/iupgo/IupApplication");
  method_id = IUPJNI_GetMethodID(IupApplication_getCurrentActivity, jni_env, java_class, "getCurrentActivity", "()Landroid/app/Activity;");
  ret_object = (*jni_env)->CallObjectMethod(jni_env, application_object, method_id);
  iupAndroid_CheckException(jni_env, "IupApplication.getCurrentActivity");

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  (*jni_env)->DeleteLocalRef(jni_env, application_object);

  return ret_object;
}

void iupAndroid_AddWidgetToParent(JNIEnv* jni_env, Ihandle* ih)
{
  jclass java_class;
  jmethodID method_id;

  jobject parent_native_handle = iupChildTreeGetNativeParentHandle(ih);
  jobject child_handle = ih->handle;

  java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  method_id = IUPJNI_GetStaticMethodID(IupCommon_addWidgetToParent, jni_env, java_class, "addWidgetToParent", "(Ljava/lang/Object;Ljava/lang/Object;)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, parent_native_handle, child_handle);
  iupAndroid_CheckException(jni_env, "IupCommon.addWidgetToParent");

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupAndroid_RemoveFromParent(JNIEnv* jni_env, Ihandle* ih)
{
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupCommon_removeWidgetFromParent, jni_env, java_class, "removeWidgetFromParent", "(J)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(jni_env, "IupCommon.removeWidgetFromParent");

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

float iupAndroid_GetDisplayDensity(void)
{
  static float cached = 0.0f;
  if (cached > 0.0f)
    return cached;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getDisplayDensity", "()F");
  jfloat density = (*jni_env)->CallStaticFloatMethod(jni_env, java_class, method_id);
  iupAndroid_CheckException(jni_env, "IupCommon.getDisplayDensity");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  cached = (float)density;
  if (cached <= 0.0f) cached = 1.0f;
  return cached;
}

int iupAndroid_DpToPx(float dp)
{
  return (int)(dp * iupAndroid_GetDisplayDensity() + 0.5f);
}

float iupAndroid_PxToDp(int px)
{
  return (float)px / iupAndroid_GetDisplayDensity();
}

/* Cached empty-MaterialButton size, pairs with IupFontHelper.measureButtonText. */
void iupAndroid_GetButtonBorderSize(int* w, int* h)
{
  static int cached_w = -1;
  static int cached_h = -1;

  if (cached_w < 0)
  {
    JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
    jclass java_class = IUPJNI_FindClass(IupFontHelper, jni_env, "io/github/gen2brain/iupgo/IupFontHelper");
    jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getButtonBorderSize", "()Landroid/graphics/Rect;");
    jobject j_rect = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id);
    iupAndroid_CheckException(jni_env, "IupFontHelper.getButtonBorderSize");

    jclass rect_class = (*jni_env)->GetObjectClass(jni_env, j_rect);
    jmethodID width_id = (*jni_env)->GetMethodID(jni_env, rect_class, "width", "()I");
    jmethodID height_id = (*jni_env)->GetMethodID(jni_env, rect_class, "height", "()I");
    cached_w = (int)(*jni_env)->CallIntMethod(jni_env, j_rect, width_id);
    cached_h = (int)(*jni_env)->CallIntMethod(jni_env, j_rect, height_id);

    (*jni_env)->DeleteLocalRef(jni_env, rect_class);
    (*jni_env)->DeleteLocalRef(jni_env, j_rect);
    (*jni_env)->DeleteLocalRef(jni_env, java_class);
  }

  if (w) *w = cached_w;
  if (h) *h = cached_h;
}

char* iupAndroid_JStringToReturnStr(JNIEnv* jni_env, jstring j_string)
{
  if (j_string == NULL)
    return NULL;

  char* value = NULL;
  const char* c_string = (*jni_env)->GetStringUTFChars(jni_env, j_string, NULL);
  if (c_string != NULL)
  {
    value = iupStrReturnStr(c_string);
    (*jni_env)->ReleaseStringUTFChars(jni_env, j_string, c_string);
  }
  (*jni_env)->DeleteLocalRef(jni_env, j_string);
  return value;
}

void iupdrvActivate(Ihandle* ih)
{
  if (!ih || !ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "performClick", "(Ljava/lang/Object;)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jobject)ih->handle);
  iupAndroid_CheckException(jni_env, "IupCommon.performClick");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvReparent(Ihandle* ih)
{
  if (!ih || !ih->handle) return;
  /* native-parent resolution skips TYPE_VOID and uses per-child inner containers (Tabs) */
  jobject parent_handle = (jobject)iupChildTreeGetNativeParentHandle(ih);
  if (!parent_handle || (intptr_t)parent_handle == -1) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "reparentTo", "(JLjava/lang/Object;)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, parent_handle);
  iupAndroid_CheckException(jni_env, "IupCommon.reparentTo");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvBaseLayoutUpdateMethod(Ihandle* ih)
{
  jobject child_handle = ih->handle;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupCommon_setWidgetPosition, jni_env, java_class, "setWidgetPosition", "(Ljava/lang/Object;IIII)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, child_handle, ih->x, ih->y, ih->currentwidth, ih->currentheight);
  iupAndroid_CheckException(jni_env, "IupCommon.setWidgetPosition");

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  if (!ih || !ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  iupAndroid_RemoveFromParent(jni_env, ih);
  iupAndroid_ReleaseIhandle(jni_env, ih);
}

static void androidCommonGetViewScreenLocation(Ihandle* ih, int* sx, int* sy)
{
  *sx = 0;
  *sy = 0;
  jobject widget = iupAndroid_RealNativeHandle(ih);
  if (!widget) return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getScreenLocation", "(Ljava/lang/Object;)[I");
  jintArray arr = (jintArray)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, widget);
  iupAndroid_CheckException(jni_env, "IupCommon.getScreenLocation");
  if (arr)
  {
    jint vals[2];
    (*jni_env)->GetIntArrayRegion(jni_env, arr, 0, 2, vals);
    *sx = (int)vals[0];
    *sy = (int)vals[1];
    (*jni_env)->DeleteLocalRef(jni_env, arr);
  }
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvScreenToClient(Ihandle* ih, int* x, int* y)
{
  int sx, sy;
  androidCommonGetViewScreenLocation(ih, &sx, &sy);
  if (x) *x -= sx;
  if (y) *y -= sy;
}

void iupdrvClientToScreen(Ihandle* ih, int* x, int* y)
{
  int sx, sy;
  androidCommonGetViewScreenLocation(ih, &sx, &sy);
  if (x) *x += sx;
  if (y) *y += sy;
}

int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

void iupdrvSetVisible(Ihandle* ih, int visible)
{
  jobject widget = iupAndroid_RealNativeHandle(ih);
  if (!widget) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setViewVisible", "(Ljava/lang/Object;Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, widget, (jboolean)(visible ? JNI_TRUE : JNI_FALSE));
  iupAndroid_CheckException(jni_env, "IupCommon.setViewVisible");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

int iupdrvIsVisible(Ihandle* ih)
{
  jobject widget = iupAndroid_RealNativeHandle(ih);
  if (!widget) return 0;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "isViewVisible", "(Ljava/lang/Object;)Z");
  jboolean vis = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, method_id, widget);
  iupAndroid_CheckException(jni_env, "IupCommon.isViewVisible");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return vis ? 1 : 0;
}

jobject iupAndroid_RealNativeHandle(Ihandle* ih)
{
  if (!ih || !ih->handle) return NULL;
  if ((intptr_t)ih->handle == -1) return NULL;
  return (jobject)ih->handle;
}

int iupdrvIsActive(Ihandle* ih)
{
  jobject widget = iupAndroid_RealNativeHandle(ih);
  if (!widget) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupCommon_isActive, jni_env, java_class, "isActive", "(Ljava/lang/Object;)Z");
  jboolean ret_val = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, method_id, widget);
  iupAndroid_CheckException(jni_env, "IupCommon.isActive");

  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  return ret_val;
}

void iupdrvSetActive(Ihandle* ih, int enable)
{
  jobject widget = iupAndroid_RealNativeHandle(ih);
  if (!widget) return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupCommon_setActive, jni_env, java_class, "setActive", "(Ljava/lang/Object;Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, widget, (jboolean)enable);
  iupAndroid_CheckException(jni_env, "IupCommon.setActive");

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

int iupdrvGetScrollbarSize(void)
{
  return 0;
}

void iupdrvSetAccessibleTitle(Ihandle* ih, const char* title)
{
  jobject widget = iupAndroid_RealNativeHandle(ih);
  if (!widget) return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setAccessibleTitle", "(Ljava/lang/Object;Ljava/lang/String;)V");
  jstring j_title = title ? (*jni_env)->NewStringUTF(jni_env, title) : NULL;
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, widget, j_title);
  iupAndroid_CheckException(jni_env, "IupCommon.setAccessibleTitle");
  if (j_title) (*jni_env)->DeleteLocalRef(jni_env, j_title);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
  (void)ic;
}

void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
  (void)ic;
}

void iupdrvPostRedraw(Ihandle* ih)
{
  if (!ih) return;
  jobject widget = iupAndroid_RealNativeHandle(ih);
  if (!widget) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "postInvalidate", "(Ljava/lang/Object;)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, widget);
  iupAndroid_CheckException(jni_env, "IupCommon.postInvalidate");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvRedrawNow(Ihandle* ih)
{
  if (!ih) return;
  jobject widget = iupAndroid_RealNativeHandle(ih);
  if (!widget) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "invalidateNow", "(Ljava/lang/Object;)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, widget);
  iupAndroid_CheckException(jni_env, "IupCommon.invalidateNow");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvSendKey(int key, int press)
{
  unsigned int keyval = 0, state = 0;
  iupdrvKeyEncode(key, &keyval, &state);
  if (!keyval) return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "sendKey", "(III)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jint)keyval, (jint)state, (jint)press);
  iupAndroid_CheckException(jni_env, "IupCommon.sendKey");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvSendMouse(int x, int y, int bt, int status)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCommon, jni_env, "io/github/gen2brain/iupgo/IupCommon");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "sendMouse", "(IIII)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jint)x, (jint)y, (jint)bt, (jint)status);
  iupAndroid_CheckException(jni_env, "IupCommon.sendMouse");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvSleep(int time)
{
  if (time > 0) usleep((useconds_t)time * 1000);
}

void iupdrvWarpPointer(int x, int y)
{
  (void)x;
  (void)y;
}

/* Lives here (not iupandroid_loop.c) so the class pre-cache in JNI_OnLoad is unconditional. */
void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupPostMessage_postMessage);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jobject app_context = iupAndroid_GetApplication(jni_env);
  jclass java_class = IUPJNI_FindClass(IupPostMessage, jni_env, "io/github/gen2brain/iupgo/IupPostMessage");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupPostMessage_postMessage, jni_env, java_class, "postMessage", "(Landroid/content/Context;JJLjava/lang/String;JD)V");

  /* Skip the Java String allocation in the common empty-payload case. */
  jstring j_string = (s && *s) ? (*jni_env)->NewStringUTF(jni_env, s) : NULL;
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, app_context, (jlong)(intptr_t)ih, (jlong)(intptr_t)p, j_string, (jlong)i, (jdouble)d);
  iupAndroid_CheckException(jni_env, "IupPostMessage.postMessage");

  if (j_string) (*jni_env)->DeleteLocalRef(jni_env, j_string);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  (*jni_env)->DeleteLocalRef(jni_env, app_context);
}
