/** \file
 * \brief Label Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>

#include <jni.h>
#include <android/log.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_label.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupLabelHelper);

/* Each IUP label subtype maps to a different Java widget class (TextView, ImageView, divider View). */
typedef enum
{
  IUPANDROIDLABELSUBTYPE_UNKNOWN = -1,
  IUPANDROIDLABELSUBTYPE_SEP_HORIZONTAL,
  IUPANDROIDLABELSUBTYPE_SEP_VERTICAL,
  IUPANDROIDLABELSUBTYPE_IMAGE,
  IUPANDROIDLABELSUBTYPE_TEXT
} IupAndroidLabelSubType;

static IupAndroidLabelSubType androidLabelGetSubType(Ihandle* ih)
{
  switch (ih->data->type)
  {
    case IUP_LABEL_SEP_HORIZ: return IUPANDROIDLABELSUBTYPE_SEP_HORIZONTAL;
    case IUP_LABEL_SEP_VERT:  return IUPANDROIDLABELSUBTYPE_SEP_VERTICAL;
    case IUP_LABEL_IMAGE:     return IUPANDROIDLABELSUBTYPE_IMAGE;
    case IUP_LABEL_TEXT:      return IUPANDROIDLABELSUBTYPE_TEXT;
    default:                  return IUPANDROIDLABELSUBTYPE_UNKNOWN;
  }
}

void iupdrvLabelAddExtraPadding(Ihandle* ih, int* x, int* y)
{
  (void)ih;
  (void)x;
  (void)y;
}

static int androidLabelSetTitleAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupLabelHelper_setText);

  if (value == NULL)
    value = "";

  if (androidLabelGetSubType(ih) != IUPANDROIDLABELSUBTYPE_TEXT)
    return 0;

  int markup = iupAttribGetBoolean(ih, "MARKUP");

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupLabelHelper, jni_env, "io/github/gen2brain/iupgo/IupLabelHelper");

  jmethodID method_id = IUPJNI_GetStaticMethodID(IupLabelHelper_setText, jni_env, java_class, "setText", "(JLandroid/widget/TextView;Ljava/lang/String;Z)V");

  /* MARKUP=YES: feed value as-is so '&' isn't mistaken for a mnemonic marker. */
  char* stripped = markup ? NULL : iupStrProcessMnemonic(value, NULL, 0);
  const char* effective = stripped ? stripped : value;
  jstring j_string = (*jni_env)->NewStringUTF(jni_env, effective);
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jobject)ih->handle, j_string, markup ? JNI_TRUE : JNI_FALSE);
  iupAndroid_CheckException(jni_env, "IupLabelHelper.setText");
  (*jni_env)->DeleteLocalRef(jni_env, j_string);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  if (stripped && stripped != value) free(stripped);

  return 1;
}

static int androidLabelSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupLabelHelper_setAlignment);
  char horiz[30] = "ALEFT";
  char vert[30] = "ACENTER";

  if (androidLabelGetSubType(ih) != IUPANDROIDLABELSUBTYPE_TEXT)
    return 0;

  iupStrToStrStr(value, horiz, sizeof(horiz), vert, sizeof(vert), ':');

  if (!ih->handle)
    return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupLabelHelper, jni_env, "io/github/gen2brain/iupgo/IupLabelHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupLabelHelper_setAlignment, jni_env, java_class, "setAlignment", "(Landroid/widget/TextView;Ljava/lang/String;Ljava/lang/String;)V");

  jstring j_horiz = (*jni_env)->NewStringUTF(jni_env, horiz);
  jstring j_vert = (*jni_env)->NewStringUTF(jni_env, vert);
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, j_horiz, j_vert);
  iupAndroid_CheckException(jni_env, "IupLabelHelper.setAlignment");
  (*jni_env)->DeleteLocalRef(jni_env, j_horiz);
  (*jni_env)->DeleteLocalRef(jni_env, j_vert);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  return 1;
}

static int androidLabelSetFgColorAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupLabelHelper_setTextColor);
  unsigned char r, g, b;

  if (androidLabelGetSubType(ih) != IUPANDROIDLABELSUBTYPE_TEXT)
    return 0;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;
  if (!ih->handle)
    return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupLabelHelper, jni_env, "io/github/gen2brain/iupgo/IupLabelHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupLabelHelper_setTextColor, jni_env, java_class, "setTextColor", "(Landroid/widget/TextView;III)V");

  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jint)r, (jint)g, (jint)b);
  iupAndroid_CheckException(jni_env, "IupLabelHelper.setTextColor");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  return 1;
}

static int androidLabelSetBgColorAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupLabelHelper_setBgColor);
  unsigned char r, g, b;

  if (androidLabelGetSubType(ih) != IUPANDROIDLABELSUBTYPE_TEXT)
    return 0;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;
  if (!ih->handle)
    return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupLabelHelper, jni_env, "io/github/gen2brain/iupgo/IupLabelHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupLabelHelper_setBgColor, jni_env, java_class, "setBgColor", "(Landroid/widget/TextView;III)V");

  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jint)r, (jint)g, (jint)b);
  iupAndroid_CheckException(jni_env, "IupLabelHelper.setBgColor");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  return 1;
}

static int androidLabelSetImageAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupLabelHelper_setImage);

  if (androidLabelGetSubType(ih) != IUPANDROIDLABELSUBTYPE_IMAGE)
    return 0;

  int make_inactive = 0;
  if (!iupdrvIsActive(ih))
  {
    char* name = iupAttribGet(ih, "IMINACTIVE");
    if (!name)
      make_inactive = 1;
  }

  /* IUP image cache owns the Bitmap; do not release */
  jobject the_bitmap = value ? iupImageGetImage(value, ih, make_inactive, NULL) : NULL;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupLabelHelper, jni_env, "io/github/gen2brain/iupgo/IupLabelHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupLabelHelper_setImage, jni_env, java_class, "setImage", "(JLandroid/widget/ImageView;Landroid/graphics/Bitmap;)V");

  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jobject)ih->handle, the_bitmap);
  iupAndroid_CheckException(jni_env, "IupLabelHelper.setImage");

  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  return 1;
}

static int androidLabelSetEllipsisAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupLabelHelper_setEllipsis);
  if (androidLabelGetSubType(ih) != IUPANDROIDLABELSUBTYPE_TEXT) return 0;
  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupLabelHelper, jni_env, "io/github/gen2brain/iupgo/IupLabelHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupLabelHelper_setEllipsis, jni_env, cls, "setEllipsis", "(Landroid/widget/TextView;Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, iupStrBoolean(value) ? JNI_TRUE : JNI_FALSE);
  iupAndroid_CheckException(jni_env, "IupLabelHelper.setEllipsis");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 1;
}

static int androidLabelSetWordWrapAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupLabelHelper_setWordWrap);
  if (androidLabelGetSubType(ih) != IUPANDROIDLABELSUBTYPE_TEXT) return 0;
  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupLabelHelper, jni_env, "io/github/gen2brain/iupgo/IupLabelHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupLabelHelper_setWordWrap, jni_env, cls, "setWordWrap", "(Landroid/widget/TextView;Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, iupStrBoolean(value) ? JNI_TRUE : JNI_FALSE);
  iupAndroid_CheckException(jni_env, "IupLabelHelper.setWordWrap");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 1;
}

static int androidLabelSetSelectableAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupLabelHelper_setSelectable);
  if (androidLabelGetSubType(ih) != IUPANDROIDLABELSUBTYPE_TEXT) return 0;
  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupLabelHelper, jni_env, "io/github/gen2brain/iupgo/IupLabelHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupLabelHelper_setSelectable, jni_env, cls, "setSelectable", "(Landroid/widget/TextView;Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, iupStrBoolean(value) ? JNI_TRUE : JNI_FALSE);
  iupAndroid_CheckException(jni_env, "IupLabelHelper.setSelectable");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 1;
}

static int androidLabelSetPaddingAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupLabelHelper_setPadding);

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  ih->data->horiz_padding = iupdrvScaleNaturalPx(ih->data->horiz_padding);
  ih->data->vert_padding  = iupdrvScaleNaturalPx(ih->data->vert_padding);

  if (!ih->handle) return 1;
  IupAndroidLabelSubType st = androidLabelGetSubType(ih);
  if (st == IUPANDROIDLABELSUBTYPE_SEP_HORIZONTAL || st == IUPANDROIDLABELSUBTYPE_SEP_VERTICAL) return 0;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupLabelHelper, jni_env, "io/github/gen2brain/iupgo/IupLabelHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupLabelHelper_setPadding, jni_env, cls, "setPadding", "(Landroid/view/View;II)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jint)ih->data->horiz_padding, (jint)ih->data->vert_padding);
  iupAndroid_CheckException(jni_env, "IupLabelHelper.setPadding");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 0;
}

static int androidLabelSetMarkupAttrib(Ihandle* ih, const char* value)
{
  /* Update hash first so the title setter reads the new MARKUP value, then re-render. */
  iupAttribSetStr(ih, "MARKUP", value);
  if (ih->handle && androidLabelGetSubType(ih) == IUPANDROIDLABELSUBTYPE_TEXT)
  {
    char* title = iupAttribGet(ih, "TITLE");
    if (title) androidLabelSetTitleAttrib(ih, title);
  }
  return 0;
}

static int androidLabelMapMethod(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupLabelHelper_createLabelText);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupLabelHelper_createLabelImage);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupLabelHelper_createLabelSeparator);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupLabelHelper, jni_env, "io/github/gen2brain/iupgo/IupLabelHelper");
  jmethodID method_id = NULL;
  jobject java_widget = NULL;

  char* value = iupAttribGet(ih, "SEPARATOR");
  if (value)
  {
    int horizontal = iupStrEqualNoCase(value, "HORIZONTAL");
    ih->data->type = horizontal ? IUP_LABEL_SEP_HORIZ : IUP_LABEL_SEP_VERT;
    method_id = IUPJNI_GetStaticMethodID(IupLabelHelper_createLabelSeparator, jni_env, java_class, "createLabelSeparator", "(JZ)Landroid/view/View;");
    java_widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, horizontal ? JNI_TRUE : JNI_FALSE);
    iupAndroid_CheckException(jni_env, "IupLabelHelper.createLabelSeparator");
  }
  else if (iupAttribGet(ih, "IMAGE"))
  {
    ih->data->type = IUP_LABEL_IMAGE;
    method_id = IUPJNI_GetStaticMethodID(IupLabelHelper_createLabelImage, jni_env, java_class, "createLabelImage", "(J)Landroid/widget/ImageView;");
    java_widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih);
    iupAndroid_CheckException(jni_env, "IupLabelHelper.createLabelImage");
  }
  else
  {
    ih->data->type = IUP_LABEL_TEXT;
    method_id = IUPJNI_GetStaticMethodID(IupLabelHelper_createLabelText, jni_env, java_class, "createLabelText", "(J)Landroid/widget/TextView;");
    java_widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih);
    iupAndroid_CheckException(jni_env, "IupLabelHelper.createLabelText");
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!java_widget)
    return IUP_ERROR;

  ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, java_widget));
  (*jni_env)->DeleteLocalRef(jni_env, java_widget);

  iupAndroid_AddWidgetToParent(jni_env, ih);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  return IUP_NOERROR;
}

void iupdrvLabelInitClass(Iclass* ic)
{
  ic->Map = androidLabelMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "TITLE", NULL, androidLabelSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, androidLabelSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, androidLabelSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT:ACENTER", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, androidLabelSetFgColorAttrib, "DLGFGCOLOR", NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, androidLabelSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "WORDWRAP", NULL, androidLabelSetWordWrapAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SELECTABLE", NULL, androidLabelSetSelectableAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_DEFAULT|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ELLIPSIS", NULL, androidLabelSetEllipsisAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "PADDING", iupLabelGetPaddingAttrib, androidLabelSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, androidLabelSetMarkupAttrib, NULL, NULL, IUPAF_DEFAULT);
}
