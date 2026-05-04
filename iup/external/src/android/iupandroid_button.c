/** \file
 * \brief Button Control
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
#include "iup_image.h"
#include "iup_button.h"
#include "iup_drvinfo.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupButtonHelper);

static jclass androidButtonFindHelper(JNIEnv* jni_env)
{
  return IUPJNI_FindClass(IupButtonHelper, jni_env, "io/github/gen2brain/iupgo/IupButtonHelper");
}


void iupdrvButtonAddBorders(Ihandle* ih, int* x, int* y)
{
  int bw = 0, bh = 0;

  /* No user PADDING: widget keeps theme padding, reserve it as border. */
  if (!iupAttribGet(ih, "PADDING"))
    iupAndroid_GetButtonBorderSize(&bw, &bh);

  /* rebuildIcon floors to 24dp; compensate so natural size matches. */
  if (ih && ih->data && (ih->data->type & IUP_BUTTON_IMAGE))
  {
    const char* image = iupAttribGet(ih, "IMAGE");
    int img_w = 0, img_h = 0;
    if (image) iupImageGetInfo(image, &img_w, &img_h, NULL);
    int min_icon = iupAndroid_DpToPx(24.0f);
    if (min_icon > img_w) bw += (min_icon - img_w);
    if (min_icon > img_h) bh += (min_icon - img_h);

    /* Icon-text gap (matches setIconPadding(8dp) in IupButtonHelper.rebuildIcon). */
    if (ih->data->type & IUP_BUTTON_TEXT)
      bw += iupAndroid_DpToPx(8.0f);
  }

  if (x) *x += bw;
  if (y) *y += bh;
}

static int androidButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  int markup = iupAttribGetBoolean(ih, "MARKUP");

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidButtonFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setTitle", "(Landroid/widget/Button;Ljava/lang/String;Z)V");

  /* MARKUP=YES: feed value as-is so '&' isn't mistaken for a mnemonic marker. */
  char* stripped = (value && !markup) ? iupStrProcessMnemonic(value, NULL, 0) : NULL;
  const char* effective = stripped ? stripped : value;
  jstring j_string = effective ? (*jni_env)->NewStringUTF(jni_env, effective) : NULL;
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, j_string, markup ? JNI_TRUE : JNI_FALSE);
  iupAndroid_CheckException(jni_env, "IupButtonHelper.setTitle");
  if (j_string) (*jni_env)->DeleteLocalRef(jni_env, j_string);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  if (stripped && stripped != value) free(stripped);
  return 1;
}

static int androidButtonSetMarkupAttrib(Ihandle* ih, const char* value)
{
  /* Update hash first so the title setter reads the new MARKUP value, then re-render. */
  iupAttribSetStr(ih, "MARKUP", value);
  if (ih->handle)
  {
    char* title = iupAttribGet(ih, "TITLE");
    if (title) androidButtonSetTitleAttrib(ih, title);
  }
  return 0;
}

static int androidButtonSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
  {
    ih->data->horiz_padding = iupAndroid_DpToPx(16.0f);
    ih->data->vert_padding  = iupAndroid_DpToPx(8.0f);
  }
  else
  {
    iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
    /* Logical dp → HW. */
    ih->data->horiz_padding = iupdrvScaleNaturalPx(ih->data->horiz_padding);
    ih->data->vert_padding  = iupdrvScaleNaturalPx(ih->data->vert_padding);
  }

  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidButtonFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setPadding", "(Landroid/widget/Button;II)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jint)ih->data->horiz_padding, (jint)ih->data->vert_padding);
  iupAndroid_CheckException(jni_env, "IupButtonHelper.setPadding");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 0;
}

static int androidButtonSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  char horiz[30] = "ACENTER";
  char vert[30] = "ACENTER";
  iupStrToStrStr(value, horiz, sizeof(horiz), vert, sizeof(vert), ':');

  if (!ih->handle) return 1;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidButtonFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setAlignment", "(Landroid/widget/Button;Ljava/lang/String;Ljava/lang/String;)V");

  jstring j_horiz = (*jni_env)->NewStringUTF(jni_env, horiz);
  jstring j_vert = (*jni_env)->NewStringUTF(jni_env, vert);
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, j_horiz, j_vert);
  iupAndroid_CheckException(jni_env, "IupButtonHelper.setAlignment");
  (*jni_env)->DeleteLocalRef(jni_env, j_horiz);
  (*jni_env)->DeleteLocalRef(jni_env, j_vert);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}

static int androidButtonSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidButtonFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setTextColor", "(Landroid/widget/Button;III)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jint)r, (jint)g, (jint)b);
  iupAndroid_CheckException(jni_env, "IupButtonHelper.setTextColor");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}

static int androidButtonSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidButtonFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setBgColor", "(Landroid/widget/Button;III)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jint)r, (jint)g, (jint)b);
  iupAndroid_CheckException(jni_env, "IupButtonHelper.setBgColor");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}


static int androidButtonSetImageGeneric(Ihandle* ih, const char* image_name, const char* java_method)
{
  if (!ih->handle) return 1;
  void* bmp = image_name ? iupImageGetImage(image_name, ih, 0, NULL) : NULL;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidButtonFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, java_method, "(Landroid/widget/Button;Landroid/graphics/Bitmap;)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jobject)bmp);
  iupAndroid_CheckException(jni_env, java_method);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}

static int androidButtonSetImageAttrib(Ihandle* ih, const char* value)
{
  return androidButtonSetImageGeneric(ih, value, "setImage");
}

static void androidButtonUpdateChrome(Ihandle* ih)
{
  if (!ih->handle) return;
  int borderless = iupAttribGet(ih, "IMPRESS") != NULL
                && !iupAttribGetBoolean(ih, "IMPRESSBORDER");

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidButtonFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setBorderless", "(Landroid/widget/Button;Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jboolean)borderless);
  iupAndroid_CheckException(jni_env, "IupButtonHelper.setBorderless");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static int androidButtonSetImpressBorderAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  androidButtonUpdateChrome(ih);
  return 1;
}

static int androidButtonSetShowAsDefaultAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidButtonFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setShowAsDefault", "(Landroid/widget/Button;Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jboolean)iupStrBoolean(value));
  iupAndroid_CheckException(jni_env, "IupButtonHelper.setShowAsDefault");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}

static int androidButtonSetButtonStyleAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidButtonFindHelper(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setButtonStyle", "(Landroid/widget/Button;Ljava/lang/String;)V");
  jstring j_str = value ? (*env)->NewStringUTF(env, value) : NULL;
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, j_str);
  iupAndroid_CheckException(env, "IupButtonHelper.setButtonStyle");
  if (j_str) (*env)->DeleteLocalRef(env, j_str);
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

static int androidButtonSetCornerStyleAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidButtonFindHelper(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setCornerStyle", "(Landroid/widget/Button;Ljava/lang/String;)V");
  jstring j_str = value ? (*env)->NewStringUTF(env, value) : NULL;
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, j_str);
  iupAndroid_CheckException(env, "IupButtonHelper.setCornerStyle");
  if (j_str) (*env)->DeleteLocalRef(env, j_str);
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

static int androidButtonSetImPressAttrib(Ihandle* ih, const char* value)
{
  int ret = androidButtonSetImageGeneric(ih, value, "setPressedImage");
  androidButtonUpdateChrome(ih);
  return ret;
}

static int androidButtonSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  return androidButtonSetImageGeneric(ih, value, "setInactiveImage");
}

static int androidButtonSetImagePositionAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidButtonFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setImagePosition", "(Landroid/widget/Button;Ljava/lang/String;)V");
  jstring j_pos = value ? (*jni_env)->NewStringUTF(jni_env, value) : NULL;
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, j_pos);
  iupAndroid_CheckException(jni_env, "IupButtonHelper.setImagePosition");
  if (j_pos) (*jni_env)->DeleteLocalRef(jni_env, j_pos);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}


static int androidButtonMapMethod(Ihandle* ih)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = androidButtonFindHelper(jni_env);
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "createButton", "(J)Landroid/widget/Button;");
  jobject widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(jni_env, "IupButtonHelper.createButton");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!widget) return IUP_ERROR;

  ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, widget));
  (*jni_env)->DeleteLocalRef(jni_env, widget);

  /* Record the final button type so the core's natural-size calc matches. */
  const char* image = iupAttribGet(ih, "IMAGE");
  const char* title = iupAttribGet(ih, "TITLE");
  if (image && *image)
  {
    ih->data->type = IUP_BUTTON_IMAGE;
    if (title && *title) ih->data->type |= IUP_BUTTON_TEXT;
  }
  else
    ih->data->type = IUP_BUTTON_TEXT;

  iupAndroid_AddWidgetToParent(jni_env, ih);
  androidButtonUpdateChrome(ih);

  /* Replay PADDING only if user set it; default keeps Material3 theme padding. */
  if (iupAttribGet(ih, "PADDING"))
  {
    jclass jc = androidButtonFindHelper(jni_env);
    jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, jc, "setPadding", "(Landroid/widget/Button;II)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, jc, m, ih->handle, (jint)ih->data->horiz_padding, (jint)ih->data->vert_padding);
    iupAndroid_CheckException(jni_env, "IupButtonHelper.setPadding");
    (*jni_env)->DeleteLocalRef(jni_env, jc);
  }

  /* Borderless image buttons: core skips AddBorders, floor to 48dp touch target. */
  int borderless = iupAttribGet(ih, "IMPRESS") != NULL
                && !iupAttribGetBoolean(ih, "IMPRESSBORDER");
  if (borderless && !iupAttribGet(ih, "MINSIZE"))
    iupAttribSetStr(ih, "MINSIZE", "48x48");

  return IUP_NOERROR;
}

void iupdrvButtonInitClass(Iclass* ic)
{
  ic->Map = androidButtonMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "TITLE", NULL, androidButtonSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, androidButtonSetMarkupAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, androidButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, androidButtonSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ACENTER:ACENTER", IUPAF_NO_INHERIT);

  /* Material3's primary tint wins unless explicitly overridden. */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, androidButtonSetFgColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, androidButtonSetBgColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGE", NULL, androidButtonSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, androidButtonSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, androidButtonSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEPOSITION", NULL, androidButtonSetImagePositionAttrib, IUPAF_SAMEASSYSTEM, "LEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESSBORDER", NULL, androidButtonSetImpressBorderAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWASDEFAULT", NULL, androidButtonSetShowAsDefaultAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BUTTONSTYLE", NULL, androidButtonSetButtonStyleAttrib, IUPAF_SAMEASSYSTEM, "FILLED", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CORNERSTYLE", NULL, androidButtonSetCornerStyleAttrib, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_NO_DEFAULTVALUE);
}
