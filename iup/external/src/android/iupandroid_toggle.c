/** \file
 * \brief Toggle Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>

#include <jni.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_toggle.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupToggleHelper);

/* radio exclusivity tracked via an attribute; toggles live in IupAndroidFixed, not RadioGroup */
static void androidToggleDeactivatePrevious(Ihandle* ih)
{
  Ihandle* radio = iupRadioFindToggleParent(ih);
  if (!radio) return;

  Ihandle* previous = (Ihandle*)iupAttribGet(radio, "_IUPANDROID_RADIO_ACTIVE");
  if (previous && previous != ih && iupObjectCheck(previous))
  {
    IupSetAttribute(previous, "VALUE", "OFF");
  }
  iupAttribSet(radio, "_IUPANDROID_RADIO_ACTIVE", (char*)ih);
}

void iupAndroid_ToggleActionFromJava(Ihandle* ih, int state)
{
  if (!ih) return;

  if (state && ih->data->is_radio)
    androidToggleDeactivatePrevious(ih);

  IFni cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int ret = cb(ih, state);
    if (ret == IUP_CLOSE) IupExitLoop();
  }

  Icallback vc = IupGetCallback(ih, "VALUECHANGED_CB");
  if (vc) vc(ih);
}

void iupdrvToggleAddBorders(Ihandle* ih, int* x, int* y)
{
  /* IMAGE-only. Override IUP's raw-pixel size with our 32dp icon + 16dp pad. */
  (void)ih;
  int icon_box = iupAndroid_DpToPx(32.0f);
  int padding = iupAndroid_DpToPx(16.0f);

  if (x)
  {
    int img_w = *x;
    if (img_w < icon_box) img_w = icon_box;
    *x = img_w + padding;
  }
  if (y)
  {
    int img_h = *y;
    if (img_h < icon_box) img_h = icon_box;
    *y = img_h + padding;
  }
}

void iupdrvToggleAddCheckBox(Ihandle* ih, int* x, int* y, const char* str)
{
  /* Indicator (~24dp) plus drawable padding (~16dp) plus a breathing margin. */
  (void)ih;
  (void)str;
  if (x) *x += iupAndroid_DpToPx(48.0f);
  if (y) *y += iupAndroid_DpToPx(8.0f);
}

void iupdrvToggleAddSwitch(Ihandle* ih, int* x, int* y, const char* str)
{
  /* MaterialSwitch track is ~52dp wide + ~8dp padding. */
  (void)ih;
  (void)str;
  if (x) *x += iupAndroid_DpToPx(60.0f);
  if (y) *y += iupAndroid_DpToPx(8.0f);
}

static int androidToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;

  /* Image toggles: block TITLE; the core replays it via iupAttribUpdate after Map. */
  if (ih->data->type == IUP_TOGGLE_IMAGE)
    value = "";

  /* MaterialSwitch is TextView-based; suppress its built-in label */
  if (iupAttribGetBoolean(ih, "SWITCH"))
    value = "";

  int markup = iupAttribGetBoolean(ih, "MARKUP");

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupToggleHelper, jni_env, "io/github/gen2brain/iupgo/IupToggleHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setTitle", "(Landroid/view/View;Ljava/lang/String;Z)V");
  /* MARKUP=YES: feed value as-is so '&' isn't mistaken for a mnemonic marker. */
  char* stripped = (value && !markup) ? iupStrProcessMnemonic(value, NULL, 0) : NULL;
  const char* effective = stripped ? stripped : value;
  jstring j_string = effective ? (*jni_env)->NewStringUTF(jni_env, effective) : NULL;
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, j_string, markup ? JNI_TRUE : JNI_FALSE);
  iupAndroid_CheckException(jni_env, "IupToggleHelper.setTitle");
  if (j_string) (*jni_env)->DeleteLocalRef(jni_env, j_string);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  if (stripped && stripped != value) free(stripped);
  return 1;
}

static int androidToggleSetMarkupAttrib(Ihandle* ih, const char* value)
{
  /* Update hash first so the title setter reads the new MARKUP value, then re-render. */
  iupAttribSetStr(ih, "MARKUP", value);
  if (ih->handle && ih->data->type != IUP_TOGGLE_IMAGE)
  {
    char* title = iupAttribGet(ih, "TITLE");
    if (title) androidToggleSetTitleAttrib(ih, title);
  }
  return 0;
}

/* Returns 0 unchecked, 1 checked, -1 indeterminate (NOTDEF). */
static int androidToggleGetState(Ihandle* ih)
{
  if (!ih->handle) return 0;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupToggleHelper, jni_env, "io/github/gen2brain/iupgo/IupToggleHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "getValueState", "(Landroid/view/View;)I");
  jint s = (*jni_env)->CallStaticIntMethod(jni_env, cls, m, ih->handle);
  iupAndroid_CheckException(jni_env, "IupToggleHelper.getValueState");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return (int)s;
}

static void androidTogglePushState(Ihandle* ih, int state)
{
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupToggleHelper, jni_env, "io/github/gen2brain/iupgo/IupToggleHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "setValueState", "(Landroid/view/View;I)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, ih->handle, (jint)state);
  iupAndroid_CheckException(jni_env, "IupToggleHelper.setValueState");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

static int androidToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 0;

  int three_state = iupAttribGetBoolean(ih, "3STATE");
  int target;
  if (iupStrEqualNoCase(value, "NOTDEF"))
    target = three_state ? -1 : 0;
  else if (iupStrEqualNoCase(value, "TOGGLE"))
  {
    int cur = androidToggleGetState(ih);
    if (three_state)
      target = (cur == 0) ? 1 : (cur == 1) ? -1 : 0;  /* OFF -> ON -> NOTDEF -> OFF */
    else
      target = (cur > 0) ? 0 : 1;
  }
  else
    target = iupStrBoolean(value) ? 1 : 0;

  if (target == 1 && ih->data->is_radio)
    androidToggleDeactivatePrevious(ih);

  androidTogglePushState(ih, target);
  return 0;
}

static char* androidToggleGetValueAttrib(Ihandle* ih)
{
  if (!ih->handle) return NULL;
  return iupStrReturnChecked(androidToggleGetState(ih));
}

static int androidToggleSetRightButtonAttrib(Ihandle* ih, const char* value)
{
  /* spec marks RIGHTBUTTON creation-only; iupAttribUpdate replays the setter post-map with handle valid */
  if (!ih->handle) return 1;
  if (ih->data->type != IUP_TOGGLE_TEXT) return 0;
  if (iupAttribGetBoolean(ih, "SWITCH")) return 0;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupToggleHelper, jni_env, "io/github/gen2brain/iupgo/IupToggleHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "setRightButton", "(Landroid/view/View;Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, ih->handle, (jboolean)iupStrBoolean(value));
  iupAndroid_CheckException(jni_env, "IupToggleHelper.setRightButton");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 1;
}

static int androidToggleSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupToggleHelper, jni_env, "io/github/gen2brain/iupgo/IupToggleHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setFgColor", "(Landroid/view/View;III)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jint)r, (jint)g, (jint)b);
  iupAndroid_CheckException(jni_env, "IupToggleHelper.setFgColor");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}

static int androidToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  void* bmp = iupImageGetImage(value, ih, 0, NULL);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupToggleHelper, jni_env, "io/github/gen2brain/iupgo/IupToggleHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setImage", "(Landroid/view/View;Landroid/graphics/Bitmap;)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jobject)bmp);
  iupAndroid_CheckException(jni_env, "IupToggleHelper.setImage");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}

static int androidToggleMapMethod(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupToggleHelper_createCheckbox);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupToggleHelper_createRadio);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupToggleHelper_createSwitch);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupToggleHelper_createImageToggle);

  if (iupAttribGet(ih, "IMAGE"))
    ih->data->type = IUP_TOGGLE_IMAGE;
  else
    ih->data->type = IUP_TOGGLE_TEXT;

  /* core doesn't track radio parentage; detect the ancestor before widget creation */
  Ihandle* radio = iupRadioFindToggleParent(ih);
  if (radio)
  {
    ih->data->is_radio = 1;
    /* First radio in the group starts checked. */
    if (!iupAttribGet(radio, "_IUPANDROID_RADIO_FIRST_SEEN"))
    {
      iupAttribSet(radio, "_IUPANDROID_RADIO_FIRST_SEEN", "1");
      if (!iupAttribGet(ih, "VALUE"))
      {
        iupAttribSet(ih, "VALUE", "ON");
        iupAttribSet(radio, "_IUPANDROID_RADIO_ACTIVE", (char*)ih);
      }
    }
  }

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupToggleHelper, jni_env, "io/github/gen2brain/iupgo/IupToggleHelper");
  jobject widget;

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    /* Radio exclusivity for image toggles is enforced in iupandroid_jni_IupToggle.c. */
    jmethodID m = IUPJNI_GetStaticMethodID(IupToggleHelper_createImageToggle, jni_env, java_class, "createImageToggle", "(J)Landroid/view/View;");
    widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, m, (jlong)(intptr_t)ih);
  }
  else if (ih->data->is_radio)
  {
    jmethodID m = IUPJNI_GetStaticMethodID(IupToggleHelper_createRadio, jni_env, java_class, "createRadio", "(J)Landroid/view/View;");
    widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, m, (jlong)(intptr_t)ih);
  }
  else if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    jmethodID m = IUPJNI_GetStaticMethodID(IupToggleHelper_createSwitch, jni_env, java_class, "createSwitch", "(J)Landroid/view/View;");
    widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, m, (jlong)(intptr_t)ih);
  }
  else
  {
    jmethodID m = IUPJNI_GetStaticMethodID(IupToggleHelper_createCheckbox, jni_env, java_class, "createCheckbox", "(J)Landroid/view/View;");
    widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, m, (jlong)(intptr_t)ih);
  }
  iupAndroid_CheckException(jni_env, "IupToggleHelper.createX");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!widget) return IUP_ERROR;

  ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, widget));
  (*jni_env)->DeleteLocalRef(jni_env, widget);

  iupAndroid_AddWidgetToParent(jni_env, ih);
  return IUP_NOERROR;
}

void iupdrvToggleInitClass(Iclass* ic)
{
  ic->Map = androidToggleMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "TITLE", NULL, androidToggleSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, androidToggleSetMarkupAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "VALUE", androidToggleGetValueAttrib, androidToggleSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, androidToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, androidToggleSetFgColorAttrib, "DLGFGCOLOR", NULL, IUPAF_DEFAULT);

  /* 3STATE: creation-only flag read by VALUE setter; MaterialCheckBox renders the indeterminate state. */
  iupClassRegisterAttribute(ic, "3STATE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  /* RIGHTBUTTON: spec marks Windows-only (creation-only); supported on Android via layout-direction flip. */
  iupClassRegisterAttribute(ic, "RIGHTBUTTON", NULL, androidToggleSetRightButtonAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* Not yet implemented on Android: impress, flat styling. */
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FLAT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
