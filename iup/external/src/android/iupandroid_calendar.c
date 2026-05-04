/** \file
 * \brief Calendar Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include <jni.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_str.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupCalendarHelper);

static int androidCalendarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 0;

  int year, month, day;
  if (iupStrEqualNoCase(value, "TODAY"))
  {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    if (!tm) return 0;
    year = tm->tm_year + 1900;
    month = tm->tm_mon + 1;
    day = tm->tm_mday;
  }
  else if (sscanf(value, "%d/%d/%d", &year, &month, &day) != 3
        && sscanf(value, "%d-%d-%d", &year, &month, &day) != 3)
    return 0;

  if (month < 1) month = 1; else if (month > 12) month = 12;
  if (day < 1) day = 1; else if (day > 31) day = 31;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCalendarHelper, jni_env, "io/github/gen2brain/iupgo/IupCalendarHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setDate", "(Landroid/widget/CalendarView;III)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jint)year, (jint)month, (jint)day);
  iupAndroid_CheckException(jni_env, "IupCalendarHelper.setDate");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 0;
}

static char* androidCalendarGetValueAttrib(Ihandle* ih)
{
  if (!ih->handle) return NULL;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCalendarHelper, jni_env, "io/github/gen2brain/iupgo/IupCalendarHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getDate", "(Landroid/widget/CalendarView;)Ljava/lang/String;");
  jstring j_string = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, ih->handle);
  iupAndroid_CheckException(jni_env, "IupCalendarHelper.getDate");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  return iupAndroid_JStringToReturnStr(jni_env, j_string);
}

static char* androidCalendarGetTodayAttrib(Ihandle* ih)
{
  (void)ih;
  time_t t = time(NULL);
  struct tm* tm = localtime(&t);
  if (!tm) return NULL;
  return iupStrReturnStrf("%d/%d/%d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
}

static int androidCalendarMapMethod(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupCalendarHelper, jni_env, "io/github/gen2brain/iupgo/IupCalendarHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "createCalendar", "(J)Landroid/widget/CalendarView;");
  jobject widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(jni_env, "IupCalendarHelper.createCalendar");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!widget) return IUP_ERROR;

  ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, widget));
  (*jni_env)->DeleteLocalRef(jni_env, widget);

  iupAndroid_AddWidgetToParent(jni_env, ih);
  return IUP_NOERROR;
}

static void androidCalendarComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  (void)children_expand;
  (void)ih;
  /* Android's CalendarView is large by default; IUP SIZE or EXPAND overrides. */
  *w = iupAndroid_DpToPx(280.0f);
  *h = iupAndroid_DpToPx(260.0f);
}

Iclass* iupCalendarNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "calendar";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  ic->New = iupCalendarNewClass;
  ic->Map = androidCalendarMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->ComputeNaturalSize = androidCalendarComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");
  iupBaseRegisterCommonCallbacks(ic);
  iupBaseRegisterCommonAttrib(ic);
  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "VALUE", androidCalendarGetValueAttrib, androidCalendarSetValueAttrib, NULL, "TODAY", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", androidCalendarGetTodayAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  /* CalendarView has no public API to expose ISO week numbers */
  iupClassRegisterAttribute(ic, "WEEKNUMBERS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);

  return ic;
}

Ihandle* IupCalendar(void)
{
  return IupCreate("calendar");
}
