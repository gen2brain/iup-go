/** \file
 * \brief Progress bar Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>

#include <jni.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_progressbar.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupProgressBarHelper);

static int androidProgressBarSetValueAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupProgressBarHelper_setProgressBarValues);

  if (!value)
    ih->data->value = 0;
  else
    iupStrToDouble(value, &(ih->data->value));

  iProgressBarCropValue(ih);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupProgressBarHelper, jni_env, "io/github/gen2brain/iupgo/IupProgressBarHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupProgressBarHelper_setProgressBarValues, jni_env, java_class, "setProgressBarValues", "(JLandroid/widget/ProgressBar;DDD)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id,
    (jlong)(intptr_t)ih,
    (jobject)ih->handle,
    ih->data->vmin,
    ih->data->vmax,
    ih->data->value);
  iupAndroid_CheckException(jni_env, "IupProgressBarHelper.setProgressBarValues");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  return 0;
}

static int androidProgressBarSetFgColorAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupProgressBarHelper_setIndicatorColor);
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupProgressBarHelper, jni_env, "io/github/gen2brain/iupgo/IupProgressBarHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupProgressBarHelper_setIndicatorColor, jni_env, cls,
      "setIndicatorColor", "(Landroid/widget/ProgressBar;III)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jint)r, (jint)g, (jint)b);
  iupAndroid_CheckException(jni_env, "IupProgressBarHelper.setIndicatorColor");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 1;
}

static int androidProgressBarSetBgColorAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupProgressBarHelper_setTrackColor);
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupProgressBarHelper, jni_env, "io/github/gen2brain/iupgo/IupProgressBarHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupProgressBarHelper_setTrackColor, jni_env, cls,
      "setTrackColor", "(Landroid/widget/ProgressBar;III)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jint)r, (jint)g, (jint)b);
  iupAndroid_CheckException(jni_env, "IupProgressBarHelper.setTrackColor");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 1;
}

static int androidProgressBarSetMarqueeAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupProgressBarHelper_setIndeterminate);
  if (!ih->handle) return 1;  /* Map replays the attribute post-map. */
  jboolean enable_marquee = iupStrBoolean(value);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupProgressBarHelper, jni_env, "io/github/gen2brain/iupgo/IupProgressBarHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupProgressBarHelper_setIndeterminate, jni_env, java_class, "setIndeterminate", "(JLandroid/widget/ProgressBar;Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id,
    (jlong)(intptr_t)ih,
    (jobject)ih->handle,
    enable_marquee);
  iupAndroid_CheckException(jni_env, "IupProgressBarHelper.setIndeterminate");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  return 1;
}

static int androidProgressBarMapMethod(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupProgressBarHelper_createProgressBar);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupProgressBarHelper, jni_env, "io/github/gen2brain/iupgo/IupProgressBarHelper");

  int initial_width = 200;
  int initial_height = 30;
  IupGetIntInt(ih, "RASTERSIZE", &initial_width, &initial_height);
  if (initial_width == 0)  initial_width = 200;
  if (initial_height == 0) initial_height = 30;

  jboolean is_vertical = iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL") ? JNI_TRUE : JNI_FALSE;
  jboolean is_marquee  = iupStrBoolean(iupAttribGetStr(ih, "MARQUEE")) ? JNI_TRUE : JNI_FALSE;
  jboolean is_circular = iupStrBoolean(iupAttribGetStr(ih, "CIRCULAR")) ? JNI_TRUE : JNI_FALSE;

  jmethodID method_id = IUPJNI_GetStaticMethodID(IupProgressBarHelper_createProgressBar, jni_env, java_class, "createProgressBar", "(JIIZZZ)Landroid/widget/ProgressBar;");
  jobject java_widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id,
    (jlong)(intptr_t)ih,
    initial_width,
    initial_height,
    is_vertical,
    is_marquee,
    is_circular);
  iupAndroid_CheckException(jni_env, "IupProgressBarHelper.createProgressBar");

  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!java_widget)
    return IUP_ERROR;

  ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, java_widget));
  (*jni_env)->DeleteLocalRef(jni_env, java_widget);

  iupAndroid_AddWidgetToParent(jni_env, ih);

  return IUP_NOERROR;
}

void iupdrvProgressBarGetMinSize(Ihandle* ih, int* w, int* h)
{
  (void)ih;
  /* Logical px; 8dp = Material3 track thickness. */
  if (w) *w = 8;
  if (h) *h = 8;
}

void iupdrvProgressBarInitClass(Iclass* ic)
{
  ic->Map = androidProgressBarMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", iProgressBarGetValueAttrib, androidProgressBarSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARQUEE", NULL, androidProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DASHED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, androidProgressBarSetFgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, androidProgressBarSetBgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
}
