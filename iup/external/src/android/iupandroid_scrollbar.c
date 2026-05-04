/** \file
 * \brief Scrollbar Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>

#include <jni.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_scrollbar.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupScrollbarHelper);


static void androidScrollbarPushRange(Ihandle* ih)
{
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupScrollbarHelper, jni_env, "io/github/gen2brain/iupgo/IupScrollbarHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setRange", "(Landroid/view/View;DDDD)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, m, ih->handle,
    (jdouble)ih->data->vmin, (jdouble)ih->data->vmax,
    (jdouble)ih->data->val,  (jdouble)ih->data->pagesize);
  iupAndroid_CheckException(jni_env, "IupScrollbarHelper.setRange");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static void androidScrollbarPushSteps(Ihandle* ih)
{
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupScrollbarHelper, jni_env, "io/github/gen2brain/iupgo/IupScrollbarHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setSteps", "(Landroid/view/View;D)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, m, ih->handle, (jdouble)ih->data->pagestep);
  iupAndroid_CheckException(jni_env, "IupScrollbarHelper.setSteps");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static int androidScrollbarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    iupScrollbarCropValue(ih);
    androidScrollbarPushRange(ih);
  }
  return 0;
}

static int androidScrollbarSetLineStepAttrib(Ihandle* ih, const char* value)
{
  iupStrToDoubleDef(value, &(ih->data->linestep), 0.01);
  return 0;
}

static int androidScrollbarSetPageStepAttrib(Ihandle* ih, const char* value)
{
  iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1);
  androidScrollbarPushSteps(ih);
  return 0;
}

static int androidScrollbarSetPageSizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagesize), 0.1))
  {
    iupScrollbarCropValue(ih);
    androidScrollbarPushRange(ih);
  }
  return 0;
}

void iupAndroid_ScrollbarDispatch(Ihandle* ih, int op, double v)
{
  if (!ih) return;

  double old_val = ih->data->val;
  ih->data->val = v;
  iupScrollbarCropValue(ih);

  IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
  if (scroll_cb)
  {
    float posx = 0, posy = 0;
    if (ih->data->orientation == ISCROLLBAR_HORIZONTAL) posx = (float)ih->data->val;
    else                                                 posy = (float)ih->data->val;
    scroll_cb(ih, op, posx, posy);
  }

  IFn vc = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (vc && ih->data->val != old_val) vc(ih);
}

static int androidScrollbarMapMethod(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupScrollbarHelper, jni_env, "io/github/gen2brain/iupgo/IupScrollbarHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "createScrollbar", "(JZZ)Landroid/view/View;");
  jobject widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, m,
    (jlong)(intptr_t)ih,
    (jboolean)(ih->data->orientation == ISCROLLBAR_HORIZONTAL ? JNI_TRUE : JNI_FALSE),
    (jboolean)(ih->data->inverted ? JNI_TRUE : JNI_FALSE));
  iupAndroid_CheckException(jni_env, "IupScrollbarHelper.createScrollbar");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!widget) return IUP_ERROR;
  ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, widget));
  (*jni_env)->DeleteLocalRef(jni_env, widget);

  iupAndroid_AddWidgetToParent(jni_env, ih);
  androidScrollbarPushRange(ih);
  androidScrollbarPushSteps(ih);
  return IUP_NOERROR;
}

void iupdrvScrollbarGetMinSize(Ihandle* ih, int* w, int* h)
{
  /* Logical px; parser scales to HW. */
  int thumb = 20;
  int len = 100;
  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    if (w) *w = len;
    if (h) *h = thumb;
  }
  else
  {
    if (w) *w = thumb;
    if (h) *h = len;
  }
}

void iupdrvScrollbarInitClass(Iclass* ic)
{
  ic->Map = androidScrollbarMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", iupScrollbarGetValueAttrib, androidScrollbarSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINESTEP", iupScrollbarGetLineStepAttrib, androidScrollbarSetLineStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupScrollbarGetPageStepAttrib, androidScrollbarSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESIZE", iupScrollbarGetPageSizeAttrib, androidScrollbarSetPageSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
