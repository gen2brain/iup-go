/** \file
 * \brief DatePick Control
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
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_register.h"
#include "iup_drv.h"
#include "iup_drvfont.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupDatePickHelper);

static jclass androidDatePickFindHelper(JNIEnv* env)
{
  return IUPJNI_FindClass(IupDatePickHelper, env, "io/github/gen2brain/iupgo/IupDatePickHelper");
}

static void androidDatePickGetToday(int* y, int* m, int* d)
{
  time_t t = time(NULL);
  struct tm* tm = localtime(&t);
  if (tm)
  {
    *y = tm->tm_year + 1900;
    *m = tm->tm_mon + 1;
    *d = tm->tm_mday;
  }
  else
  {
    *y = 2000; *m = 1; *d = 1;
  }
}

static void androidDatePickStoreDate(Ihandle* ih, int y, int m, int d)
{
  iupAttribSetInt(ih, "_IUPDATEPICK_YEAR",  y);
  iupAttribSetInt(ih, "_IUPDATEPICK_MONTH", m);
  iupAttribSetInt(ih, "_IUPDATEPICK_DAY",   d);
}

static void androidDatePickPushValue(Ihandle* ih, int y, int m, int d)
{
  if (!ih->handle) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidDatePickFindHelper(env);
  jmethodID mid = (*env)->GetStaticMethodID(env, cls, "setValue", "(Lcom/google/android/material/button/MaterialButton;III)V");
  (*env)->CallStaticVoidMethod(env, cls, mid, (jobject)ih->handle, (jint)y, (jint)m, (jint)d);
  iupAndroid_CheckException(env, "IupDatePickHelper.setValue");
  (*env)->DeleteLocalRef(env, cls);
}

/* Builds a java.text.SimpleDateFormat pattern from ORDER/SEPARATOR/ZEROPRECED/MONTHSHORTNAMES. */
static void androidDatePickBuildPattern(Ihandle* ih, char* out, size_t out_size)
{
  const char* order = iupAttribGetStr(ih, "ORDER");
  const char* sep = iupAttribGetStr(ih, "SEPARATOR");
  int zero = iupAttribGetBoolean(ih, "ZEROPRECED");
  int shortn = iupAttribGetBoolean(ih, "MONTHSHORTNAMES");

  if (!order || strlen(order) != 3) order = "DMY";
  if (!sep) sep = "/";

  out[0] = '\0';
  for (int i = 0; i < 3; i++)
  {
    if (i > 0) strncat(out, sep, out_size - strlen(out) - 1);
    char c = order[i];
    if (c == 'D' || c == 'd')
      strncat(out, zero ? "dd" : "d", out_size - strlen(out) - 1);
    else if (c == 'M' || c == 'm')
      strncat(out, shortn ? "MMM" : (zero ? "MM" : "M"), out_size - strlen(out) - 1);
    else if (c == 'Y' || c == 'y')
      strncat(out, "yyyy", out_size - strlen(out) - 1);
  }
}

static void androidDatePickPushPattern(Ihandle* ih, const char* pattern)
{
  if (!ih->handle || !pattern) return;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidDatePickFindHelper(env);
  jmethodID mid = (*env)->GetStaticMethodID(env, cls, "setPattern", "(Lcom/google/android/material/button/MaterialButton;Ljava/lang/String;)V");
  jstring js = (*env)->NewStringUTF(env, pattern);
  (*env)->CallStaticVoidMethod(env, cls, mid, (jobject)ih->handle, js);
  iupAndroid_CheckException(env, "IupDatePickHelper.setPattern");
  (*env)->DeleteLocalRef(env, js);
  (*env)->DeleteLocalRef(env, cls);
}

static void androidDatePickRefreshPattern(Ihandle* ih)
{
  const char* user = iupAttribGet(ih, "FORMAT");
  if (user)
  {
    androidDatePickPushPattern(ih, user);
    return;
  }
  char buf[64];
  androidDatePickBuildPattern(ih, buf, sizeof(buf));
  androidDatePickPushPattern(ih, buf);
}


static int androidDatePickSetValueAttrib(Ihandle* ih, const char* value)
{
  int y = 2000, m = 1, d = 1;
  if (iupStrEqualNoCase(value, "TODAY") || !value)
  {
    androidDatePickGetToday(&y, &m, &d);
  }
  else if (sscanf(value, "%d/%d/%d", &y, &m, &d) != 3)
  {
    return 0;
  }
  if (m < 1) m = 1; else if (m > 12) m = 12;
  if (d < 1) d = 1; else if (d > 31) d = 31;
  androidDatePickStoreDate(ih, y, m, d);
  androidDatePickPushValue(ih, y, m, d);
  return 0;
}

static char* androidDatePickGetValueAttrib(Ihandle* ih)
{
  int y = iupAttribGetInt(ih, "_IUPDATEPICK_YEAR");
  int m = iupAttribGetInt(ih, "_IUPDATEPICK_MONTH");
  int d = iupAttribGetInt(ih, "_IUPDATEPICK_DAY");
  if (y == 0)
  {
    androidDatePickGetToday(&y, &m, &d);
    androidDatePickStoreDate(ih, y, m, d);
  }
  return iupStrReturnStrf("%d/%d/%d", y, m, d);
}

static char* androidDatePickGetTodayAttrib(Ihandle* ih)
{
  int y, m, d;
  (void)ih;
  androidDatePickGetToday(&y, &m, &d);
  return iupStrReturnStrf("%d/%d/%d", y, m, d);
}

static int androidDatePickSetOrderAttrib(Ihandle* ih, const char* value)
{
  if (!value || strlen(value) != 3) return 0;
  for (int i = 0; i < 3; i++)
  {
    char c = value[i];
    if (c != 'D' && c != 'd' && c != 'M' && c != 'm' && c != 'Y' && c != 'y') return 0;
  }
  iupAttribSetStr(ih, "ORDER", value);
  androidDatePickRefreshPattern(ih);
  return 0;
}

static int androidDatePickSetSeparatorAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "SEPARATOR", value);
  androidDatePickRefreshPattern(ih);
  return 0;
}

static int androidDatePickSetZeroprecedAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "ZEROPRECED", value);
  androidDatePickRefreshPattern(ih);
  return 0;
}

static int androidDatePickSetMonthshortnamesAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "MONTHSHORTNAMES", value);
  androidDatePickRefreshPattern(ih);
  return 0;
}

static int androidDatePickSetFormatAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "FORMAT", value);
  androidDatePickRefreshPattern(ih);
  return 0;
}

static int androidDatePickSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 0;
  int show = iupStrBoolean(value);
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidDatePickFindHelper(env);
  jmethodID mid = (*env)->GetStaticMethodID(env, cls, "showDropdown", "(Lcom/google/android/material/button/MaterialButton;Z)V");
  (*env)->CallStaticVoidMethod(env, cls, mid, (jobject)ih->handle, show ? JNI_TRUE : JNI_FALSE);
  iupAndroid_CheckException(env, "IupDatePickHelper.showDropdown");
  (*env)->DeleteLocalRef(env, cls);
  return 0;
}


static void androidDatePickComputeNaturalSizeMethod(Ihandle* ih, int* w, int* h, int* children_expand)
{
  (void)children_expand;
  iupdrvFontGetMultiLineStringSize(ih, "WW/MMM/WWWW", w, h);
  /* Outlined MaterialButton padding: 24dp horiz + 1dp stroke; 8dp vert. */
  *w += iupAndroid_DpToPx(50.0f);
  *h += iupAndroid_DpToPx(16.0f);
}

static int androidDatePickMapMethod(Ihandle* ih)
{
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidDatePickFindHelper(env);
  jmethodID mid = (*env)->GetStaticMethodID(env, cls, "createDatePick", "(J)Lcom/google/android/material/button/MaterialButton;");
  jobject widget = (*env)->CallStaticObjectMethod(env, cls, mid, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(env, "IupDatePickHelper.createDatePick");
  (*env)->DeleteLocalRef(env, cls);
  if (!widget) return IUP_ERROR;

  ih->handle = (jobject)((*env)->NewGlobalRef(env, widget));
  (*env)->DeleteLocalRef(env, widget);

  int y = iupAttribGetInt(ih, "_IUPDATEPICK_YEAR");
  int m = iupAttribGetInt(ih, "_IUPDATEPICK_MONTH");
  int d = iupAttribGetInt(ih, "_IUPDATEPICK_DAY");
  if (y == 0)
  {
    androidDatePickGetToday(&y, &m, &d);
    androidDatePickStoreDate(ih, y, m, d);
  }
  androidDatePickRefreshPattern(ih);
  androidDatePickPushValue(ih, y, m, d);

  iupAndroid_AddWidgetToParent(env, ih);
  return IUP_NOERROR;
}

Iclass* iupDatePickNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "datepick";
  ic->cons = "DatePick";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  ic->New = iupDatePickNewClass;
  ic->Map = androidDatePickMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->ComputeNaturalSize = androidDatePickComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");
  iupBaseRegisterCommonCallbacks(ic);
  iupBaseRegisterCommonAttrib(ic);
  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "VALUE", androidDatePickGetValueAttrib, androidDatePickSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", androidDatePickGetTodayAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SEPARATOR", NULL, androidDatePickSetSeparatorAttrib, IUPAF_SAMEASSYSTEM, "/", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZEROPRECED", NULL, androidDatePickSetZeroprecedAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORDER", NULL, androidDatePickSetOrderAttrib, IUPAF_SAMEASSYSTEM, "DMY", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MONTHSHORTNAMES", NULL, androidDatePickSetMonthshortnamesAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", NULL, androidDatePickSetFormatAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CALENDARWEEKNUMBERS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, androidDatePickSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupDatePick(void)
{
  return IupCreate("datepick");
}
