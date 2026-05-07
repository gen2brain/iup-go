/** \file
 * \brief Valuator Control
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
#include "iup_val.h"
#include "iup_drvinfo.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupValHelper);

void iupdrvValGetMinSize(Ihandle* ih, int* w, int* h)
{
  /* Logical px; parser scales to HW. 48dp thumb halo (cross axis), 200dp touch-friendly major. */
  if (ih->data->orientation == IVAL_HORIZONTAL)
  {
    if (w) *w = 200;
    if (h) *h = 48;
  }
  else
  {
    if (w) *w = 48;
    if (h) *h = 200;
  }
}

static int androidValSetValueAttrib(Ihandle* ih, const char* value)
{
  double d;
  if (!iupStrToDouble(value, &d)) return 0;
  if (d < ih->data->vmin) d = ih->data->vmin;
  else if (d > ih->data->vmax) d = ih->data->vmax;
  ih->data->val = d;

  if (!ih->handle) return 0;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupValHelper, jni_env, "io/github/gen2brain/iupgo/IupValHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setValue", "(Landroid/view/View;F)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jfloat)d);
  iupAndroid_CheckException(jni_env, "IupValHelper.setValue");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 0;
}

static void androidValPushStepSize(Ihandle* ih, float step_size)
{
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupValHelper, jni_env, "io/github/gen2brain/iupgo/IupValHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "setStep", "(Landroid/view/View;F)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, ih->handle, (jfloat)step_size);
  iupAndroid_CheckException(jni_env, "IupValHelper.setStep");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

/* recompute stepSize from SHOWTICKS + current range so MIN/MAX/SHOWTICKS can come in any order */
static void androidValSyncDiscrete(Ihandle* ih)
{
  if (!ih->handle) return;
  int ticks = iupAttribGetInt(ih, "SHOWTICKS");
  float step_size = 0.0f;
  if (ticks >= 2)
    step_size = (float)((ih->data->vmax - ih->data->vmin) / (double)(ticks - 1));
  androidValPushStepSize(ih, step_size);
}

static void androidValSyncRange(Ihandle* ih)
{
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupValHelper, jni_env, "io/github/gen2brain/iupgo/IupValHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setRange", "(Landroid/view/View;FF)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jfloat)ih->data->vmin, (jfloat)ih->data->vmax);
  iupAndroid_CheckException(jni_env, "IupValHelper.setRange");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  androidValSyncDiscrete(ih);
}

static int androidValSetMinAttrib(Ihandle* ih, const char* value)
{
  double d;
  if (!iupStrToDouble(value, &d)) return 0;
  ih->data->vmin = d;
  androidValSyncRange(ih);
  return 1;
}

static int androidValSetMaxAttrib(Ihandle* ih, const char* value)
{
  double d;
  if (!iupStrToDouble(value, &d)) return 0;
  ih->data->vmax = d;
  androidValSyncRange(ih);
  return 1;
}

static int androidValSetStepAttrib(Ihandle* ih, const char* value)
{
  /* IUP STEP is keyboard/wheel increment; discreteness is driven by SHOWTICKS via stepSize */
  double d;
  if (!iupStrToDouble(value, &d)) return 0;
  ih->data->step = d;
  return 1;
}

static int androidValSetShowTicksAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  /* iupAttribGetInt below reads the attribute fresh; class register stores via return 1. */
  iupAttribSetStr(ih, "SHOWTICKS", value);
  androidValSyncDiscrete(ih);
  return 0;  /* already stored */
}

static int androidValMapMethod(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupValHelper_createHorizontal);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupValHelper_createVertical);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupValHelper, jni_env, "io/github/gen2brain/iupgo/IupValHelper");
  jmethodID method_id;
  jobject widget;

  if (ih->data->orientation == IVAL_VERTICAL)
  {
    method_id = IUPJNI_GetStaticMethodID(IupValHelper_createVertical, jni_env, java_class, "createVertical", "(J)Lio/github/gen2brain/iupgo/IupAndroidVerticalSlider;");
  }
  else
  {
    method_id = IUPJNI_GetStaticMethodID(IupValHelper_createHorizontal, jni_env, java_class, "createHorizontal", "(J)Lcom/google/android/material/slider/Slider;");
  }

  widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(jni_env, "IupValHelper.createX");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!widget) return IUP_ERROR;

  ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, widget));
  (*jni_env)->DeleteLocalRef(jni_env, widget);

  /* Apply range first so the subsequent VALUE set has a valid domain. */
  androidValSyncRange(ih);

  iupAndroid_AddWidgetToParent(jni_env, ih);
  return IUP_NOERROR;
}

void iupdrvValInitClass(Iclass* ic)
{
  ic->Map = androidValMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", iupValGetValueAttrib, androidValSetValueAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterReplaceAttribFunc(ic, "MIN", NULL, androidValSetMinAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "MAX", NULL, androidValSetMaxAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "STEP", NULL, androidValSetStepAttrib);

  iupClassRegisterAttribute(ic, "SHOWTICKS", NULL, androidValSetShowTicksAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  /* Material Slider draws ticks centered along the track; tick position is not separately controllable. */
  iupClassRegisterAttribute(ic, "TICKSPOS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
