/** \file
 * \brief Text Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <jni.h>
#include <android/log.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_mask.h"
#include "iup_array.h"
#include "iup_text.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupTextHelper);

void* iupAndroidTextGetMask(Ihandle* ih)
{
  if (!ih || !ih->data) return NULL;
  return ih->data->mask;
}

/* Each IUP text subtype maps to a different Java widget configuration. */
typedef enum
{
  IUPANDROIDTEXTSUBTYPE_UNKNOWN = -1,
  IUPANDROIDTEXTSUBTYPE_FIELD,
  IUPANDROIDTEXTSUBTYPE_VIEW,
  IUPANDROIDTEXTSUBTYPE_STEPPER
} IupAndroidTextSubType;

static IupAndroidTextSubType androidTextGetSubType(Ihandle* ih)
{
  if (ih->data->is_multiline)
    return IUPANDROIDTEXTSUBTYPE_VIEW;
  if (iupAttribGetBoolean(ih, "SPIN"))
    return IUPANDROIDTEXTSUBTYPE_STEPPER;
  return IUPANDROIDTEXTSUBTYPE_FIELD;
}

void iupdrvTextAddSpin(Ihandle* ih, int* w, int h)
{
  (void)ih;
  (void)h;
  if (w) *w += iupAndroid_DpToPx(72.0f);  /* 2x 36dp spin buttons */
}

void iupdrvTextAddBorders(Ihandle* ih, int* x, int* y)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getEditTextBorderH);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getEditTextBorderV);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getTextInputLayoutBorderH);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getTextInputLayoutBorderV);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");

  int has_til = ih && ih->data && !ih->data->is_multiline;

  if (has_til)
  {
    /* TIL probe already includes the inner TIET's compound padding. */
    if (x)
    {
      jmethodID mh = IUPJNI_GetStaticMethodID(IupTextHelper_getTextInputLayoutBorderH, jni_env, java_class, "getTextInputLayoutBorderH", "()I");
      *x += (int)(*jni_env)->CallStaticIntMethod(jni_env, java_class, mh);
    }
    if (y)
    {
      jmethodID mv = IUPJNI_GetStaticMethodID(IupTextHelper_getTextInputLayoutBorderV, jni_env, java_class, "getTextInputLayoutBorderV", "()I");
      *y += (int)(*jni_env)->CallStaticIntMethod(jni_env, java_class, mv);
    }
  }
  else
  {
    if (x)
    {
      jmethodID mh = IUPJNI_GetStaticMethodID(IupTextHelper_getEditTextBorderH, jni_env, java_class, "getEditTextBorderH", "()I");
      *x += (int)(*jni_env)->CallStaticIntMethod(jni_env, java_class, mh);
    }
    if (y)
    {
      jmethodID mv = IUPJNI_GetStaticMethodID(IupTextHelper_getEditTextBorderV, jni_env, java_class, "getEditTextBorderV", "()I");
      *y += (int)(*jni_env)->CallStaticIntMethod(jni_env, java_class, mv);
    }
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvTextAddExtraPadding(Ihandle* ih, int* w, int* h)
{
  (void)ih;
  (void)w;
  (void)h;
}

static int androidTextLinColToPos(Ihandle* ih, int lin, int col)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_linColToPos);
  if (!ih->handle) return 0;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_linColToPos, jni_env, cls,
      "linColToPos", "(Landroid/view/View;II)I");
  jint p = (*jni_env)->CallStaticIntMethod(jni_env, cls, m, (jobject)ih->handle, (jint)lin, (jint)col);
  iupAndroid_CheckException(jni_env, "IupTextHelper.linColToPos");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return (int)p;
}

static void androidTextPosToLinCol(Ihandle* ih, int pos, int* lin, int* col)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_posToLinCol);
  if (lin) *lin = 1;
  if (col) *col = 1;
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_posToLinCol, jni_env, cls,
      "posToLinCol", "(Landroid/view/View;I)J");
  jlong packed = (*jni_env)->CallStaticLongMethod(jni_env, cls, m, (jobject)ih->handle, (jint)pos);
  iupAndroid_CheckException(jni_env, "IupTextHelper.posToLinCol");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  if (lin) *lin = (int)((packed >> 32) & 0xFFFFFFFFLL);
  if (col) *col = (int)(packed & 0xFFFFFFFFLL);
}

void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int* pos)
{
  if (!pos) return;
  *pos = 0;
  if (!ih || !ih->handle) return;
  if (!ih->data->is_multiline)
  {
    *pos = (col >= 1) ? col - 1 : 0;
    return;
  }
  *pos = androidTextLinColToPos(ih, lin, col);
}

void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int* lin, int* col)
{
  if (lin) *lin = 1;
  if (col) *col = 1;
  if (!ih || !ih->handle) return;
  if (!ih->data->is_multiline)
  {
    if (col) *col = (pos >= 0) ? pos + 1 : 1;
    return;
  }
  androidTextPosToLinCol(ih, pos, lin, col);
}

/* Matches IupTextHelper.FMT_UNSET (Integer.MIN_VALUE). */
#define ANDROID_FMT_UNSET ((jint)0x80000000)

void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_beginFormatBulk);
  if (!ih->handle) return NULL;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupTextHelper_beginFormatBulk, jni_env, java_class,
      "beginFormatBulk", "(Landroid/view/View;)Ljava/lang/Object;");
  jobject state = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jobject)ih->handle);
  iupAndroid_CheckException(jni_env, "IupTextHelper.beginFormatBulk");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return state ? (void*)((*jni_env)->NewGlobalRef(jni_env, state)) : NULL;
}

void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_endFormatBulk);
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupTextHelper_endFormatBulk, jni_env, java_class,
      "endFormatBulk", "(Landroid/view/View;Ljava/lang/Object;)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jobject)ih->handle, (jobject)state);
  iupAndroid_CheckException(jni_env, "IupTextHelper.endFormatBulk");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  if (state) (*jni_env)->DeleteGlobalRef(jni_env, (jobject)state);
}

static jint androidTextParseBold(const char* s)
{
  if (iupStrEqualNoCase(s, "BOLD") || iupStrEqualNoCase(s, "YES") ||
      iupStrEqualNoCase(s, "SEMIBOLD") || iupStrEqualNoCase(s, "EXTRABOLD") ||
      iupStrEqualNoCase(s, "HEAVY") || iupStrEqualNoCase(s, "ULTRABOLD")) return 1;
  return 0;
}

static jint androidTextParseColor(const char* s)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(s, &r, &g, &b)) return ANDROID_FMT_UNSET;
  return (jint)(0xFF000000u | ((unsigned)r << 16) | ((unsigned)g << 8) | (unsigned)b);
}

static jint androidTextParseFontSize(const char* s)
{
  if (iupStrEqualNoCase(s, "XX-SMALL")) return 8;
  if (iupStrEqualNoCase(s, "X-SMALL"))  return 9;
  if (iupStrEqualNoCase(s, "SMALL"))    return 10;
  if (iupStrEqualNoCase(s, "MEDIUM"))   return 12;
  if (iupStrEqualNoCase(s, "LARGE"))    return 14;
  if (iupStrEqualNoCase(s, "X-LARGE"))  return 18;
  if (iupStrEqualNoCase(s, "XX-LARGE")) return 24;
  int v = 0;
  iupStrToInt(s, &v);
  return (jint)v;
}

void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_applyFormatTag);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_applyImageSpan);
  (void)bulk;

  if (!ih->data->is_multiline || !ih->handle) return;

  /* IMAGE replaces the selection with an inline bitmap; mutually exclusive with char/para formatting. */
  char* image_name = iupAttribGet(formattag, "IMAGE");
  if (image_name)
  {
    jobject bitmap = (jobject)iupImageGetImage(image_name, ih, 0, NULL);
    if (bitmap)
    {
      int img_w = 0, img_h = 0;
      char* wattr = iupAttribGet(formattag, "WIDTH");
      char* hattr = iupAttribGet(formattag, "HEIGHT");
      if (wattr) iupStrToInt(wattr, &img_w);
      if (hattr) iupStrToInt(hattr, &img_h);

      char* sel = iupAttribGet(formattag, "SELECTION");
      char* selpos = iupAttribGet(formattag, "SELECTIONPOS");

      JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
      jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
      jmethodID method_id = IUPJNI_GetStaticMethodID(IupTextHelper_applyImageSpan, jni_env, java_class,
        "applyImageSpan", "(Landroid/view/View;Ljava/lang/String;Ljava/lang/String;Landroid/graphics/Bitmap;II)V");

      jstring jsel = sel ? (*jni_env)->NewStringUTF(jni_env, sel) : NULL;
      jstring jselpos = selpos ? (*jni_env)->NewStringUTF(jni_env, selpos) : NULL;
      (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id,
        (jobject)ih->handle, jsel, jselpos, bitmap, (jint)img_w, (jint)img_h);
      iupAndroid_CheckException(jni_env, "IupTextHelper.applyImageSpan");
      if (jsel) (*jni_env)->DeleteLocalRef(jni_env, jsel);
      if (jselpos) (*jni_env)->DeleteLocalRef(jni_env, jselpos);
      (*jni_env)->DeleteLocalRef(jni_env, java_class);
    }
    return;
  }

  char* v;
  jint bold = ANDROID_FMT_UNSET, italic = ANDROID_FMT_UNSET;
  jint underline = ANDROID_FMT_UNSET, strike = ANDROID_FMT_UNSET;
  jint superscript = ANDROID_FMT_UNSET, subscript = ANDROID_FMT_UNSET;
  jint fg = ANDROID_FMT_UNSET, bg = ANDROID_FMT_UNSET;
  jint fontSize = 0;
  jfloat fontScale = 0.0f;
  jint alignment = ANDROID_FMT_UNSET, indent = ANDROID_FMT_UNSET, lineSpacing = ANDROID_FMT_UNSET;

  if (((v = iupAttribGet(formattag, "WEIGHT"))) || ((v = iupAttribGet(formattag, "BOLD"))))
    bold = androidTextParseBold(v);
  if ((v = iupAttribGet(formattag, "ITALIC")))
    italic = (iupStrEqualNoCase(v, "ITALIC") || iupStrEqualNoCase(v, "YES") || iupStrEqualNoCase(v, "OBLIQUE")) ? 1 : 0;
  if ((v = iupAttribGet(formattag, "UNDERLINE")))
    underline = iupStrEqualNoCase(v, "NONE") ? 0 : 1;
  if (((v = iupAttribGet(formattag, "STRIKEOUT"))) || ((v = iupAttribGet(formattag, "STRIKETHROUGH"))))
    strike = iupStrBoolean(v) ? 1 : 0;
  if ((v = iupAttribGet(formattag, "RISE")))
  {
    if (iupStrEqualNoCase(v, "SUPERSCRIPT")) { superscript = 1; subscript = 0; }
    else if (iupStrEqualNoCase(v, "SUBSCRIPT")) { superscript = 0; subscript = 1; }
    else { superscript = 0; subscript = 0; }
  }
  if ((v = iupAttribGet(formattag, "FGCOLOR"))) fg = androidTextParseColor(v);
  if ((v = iupAttribGet(formattag, "BGCOLOR"))) bg = androidTextParseColor(v);
  if ((v = iupAttribGet(formattag, "FONTSIZE"))) fontSize = androidTextParseFontSize(v);
  if ((v = iupAttribGet(formattag, "FONTSCALE")))
  {
    if      (iupStrEqualNoCase(v, "XX-SMALL")) fontScale = 0.58f;
    else if (iupStrEqualNoCase(v, "X-SMALL"))  fontScale = 0.69f;
    else if (iupStrEqualNoCase(v, "SMALL"))    fontScale = 0.83f;
    else if (iupStrEqualNoCase(v, "MEDIUM"))   fontScale = 1.0f;
    else if (iupStrEqualNoCase(v, "LARGE"))    fontScale = 1.2f;
    else if (iupStrEqualNoCase(v, "X-LARGE"))  fontScale = 1.44f;
    else if (iupStrEqualNoCase(v, "XX-LARGE")) fontScale = 1.73f;
    else { double d = 1.0; iupStrToDouble(v, &d); fontScale = (jfloat)d; }
  }

  if ((v = iupAttribGet(formattag, "ALIGNMENT")))
  {
    if      (iupStrEqualNoCase(v, "LEFT"))    alignment = 0;
    else if (iupStrEqualNoCase(v, "CENTER"))  alignment = 1;
    else if (iupStrEqualNoCase(v, "RIGHT"))   alignment = 2;
    else if (iupStrEqualNoCase(v, "JUSTIFY")) alignment = 3;
  }
  if ((v = iupAttribGet(formattag, "INDENT")))
  {
    int iv = 0; iupStrToInt(v, &iv); indent = (jint)iv;
  }
  if ((v = iupAttribGet(formattag, "LINESPACING")))
  {
    int iv = 0; iupStrToInt(v, &iv); lineSpacing = (jint)iv;
  }

  char* selection = iupAttribGet(formattag, "SELECTION");
  char* selectionpos = iupAttribGet(formattag, "SELECTIONPOS");
  char* fontFamily = iupAttribGet(formattag, "FONTFAMILY");
  if (!fontFamily) fontFamily = iupAttribGet(formattag, "FONTFACE");
  char* link = iupAttribGet(formattag, "LINK");
  char* numbering = iupAttribGet(formattag, "NUMBERING");
  char* numberingStyle = iupAttribGet(formattag, "NUMBERINGSTYLE");

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupTextHelper_applyFormatTag, jni_env, java_class,
    "applyFormatTag",
    "(Landroid/view/View;Ljava/lang/String;Ljava/lang/String;IIIIIIIIIFLjava/lang/String;IIIJLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

  jstring jsel = selection ? (*jni_env)->NewStringUTF(jni_env, selection) : NULL;
  jstring jselpos = selectionpos ? (*jni_env)->NewStringUTF(jni_env, selectionpos) : NULL;
  jstring jfontfam = fontFamily ? (*jni_env)->NewStringUTF(jni_env, fontFamily) : NULL;
  jstring jlink = link ? (*jni_env)->NewStringUTF(jni_env, link) : NULL;
  jstring jnumbering = numbering ? (*jni_env)->NewStringUTF(jni_env, numbering) : NULL;
  jstring jnumstyle = numberingStyle ? (*jni_env)->NewStringUTF(jni_env, numberingStyle) : NULL;

  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id,
    (jobject)ih->handle, jsel, jselpos,
    bold, italic, underline, strike, superscript, subscript,
    fg, bg, fontSize, fontScale, jfontfam,
    alignment, indent, lineSpacing,
    (jlong)(intptr_t)ih, jlink, jnumbering, jnumstyle);
  iupAndroid_CheckException(jni_env, "IupTextHelper.applyFormatTag");

  if (jsel) (*jni_env)->DeleteLocalRef(jni_env, jsel);
  if (jselpos) (*jni_env)->DeleteLocalRef(jni_env, jselpos);
  if (jfontfam) (*jni_env)->DeleteLocalRef(jni_env, jfontfam);
  if (jlink) (*jni_env)->DeleteLocalRef(jni_env, jlink);
  if (jnumbering) (*jni_env)->DeleteLocalRef(jni_env, jnumbering);
  if (jnumstyle) (*jni_env)->DeleteLocalRef(jni_env, jnumstyle);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

static int androidTextSetValueAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setText);

  if (value == NULL)
    value = "";

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupTextHelper_setText, jni_env, java_class, "setText", "(JLandroid/view/View;Ljava/lang/String;)V");

  jstring j_string = (*jni_env)->NewStringUTF(jni_env, value);
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jobject)ih->handle, j_string);
  iupAndroid_CheckException(jni_env, "IupTextHelper.setText");
  (*jni_env)->DeleteLocalRef(jni_env, j_string);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  return 0;
}

static char* androidTextGetValueAttrib(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getText);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupTextHelper_getText, jni_env, java_class, "getText", "(JLandroid/view/View;)Ljava/lang/String;");
  jstring j_string = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jobject)ih->handle);
  iupAndroid_CheckException(jni_env, "IupTextHelper.getText");

  char* value = iupAndroid_JStringToReturnStr(jni_env, j_string);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  return value;
}

static int androidTextSetCueBannerAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setCueBanner);

  if (value == NULL)
    value = "";

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupTextHelper_setCueBanner, jni_env, java_class, "setCueBanner", "(JLandroid/view/View;Ljava/lang/String;)V");

  jstring j_string = (*jni_env)->NewStringUTF(jni_env, value);
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jobject)ih->handle, j_string);
  iupAndroid_CheckException(jni_env, "IupTextHelper.setCueBanner");
  (*jni_env)->DeleteLocalRef(jni_env, j_string);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  return 0;
}

static int androidTextSetAppendAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_appendText);

  if (!ih->handle || !value) return 0;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupTextHelper_appendText, jni_env, java_class, "appendText", "(JLandroid/view/View;Ljava/lang/String;Z)V");

  int multi_newline = ih->data->is_multiline && ih->data->append_newline;
  jstring j_string = (*jni_env)->NewStringUTF(jni_env, value);
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jobject)ih->handle, j_string, multi_newline ? JNI_TRUE : JNI_FALSE);
  iupAndroid_CheckException(jni_env, "IupTextHelper.appendText");
  (*jni_env)->DeleteLocalRef(jni_env, j_string);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  return 0;
}

static int androidTextSetPasswordAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setPassword);

  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupTextHelper_setPassword, jni_env, java_class, "setPassword", "(JLandroid/view/View;Z)V");

  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jobject)ih->handle, (jboolean)iupStrBoolean(value));
  iupAndroid_CheckException(jni_env, "IupTextHelper.setPassword");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}

static int androidTextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setSpinValue);
  if (!ih->handle || !value) return 0;
  if (androidTextGetSubType(ih) != IUPANDROIDTEXTSUBTYPE_STEPPER) return 0;

  int v = 0;
  iupStrToInt(value, &v);
  iupAttribSetInt(ih, "_IUPANDROID_SPIN_VALUE", v);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupTextHelper_setSpinValue, jni_env, java_class, "setSpinValue", "(JLandroid/view/View;I)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jobject)ih->handle, (jint)v);
  iupAndroid_CheckException(jni_env, "IupTextHelper.setSpinValue");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 0;
}

static char* androidTextGetSpinValueAttrib(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getSpinValue);
  if (!ih->handle) return NULL;
  if (androidTextGetSubType(ih) != IUPANDROIDTEXTSUBTYPE_STEPPER) return NULL;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupTextHelper_getSpinValue, jni_env, java_class, "getSpinValue", "(JLandroid/view/View;)I");
  jint v = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jobject)ih->handle);
  iupAndroid_CheckException(jni_env, "IupTextHelper.getSpinValue");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return iupStrReturnInt((int)v);
}

static int androidTextSetNCAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setMaxChars);

  int nc = 0;
  if (value) iupStrToInt(value, &nc);
  if (nc < 0) nc = 0;
  ih->data->nc = nc;

  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupTextHelper_setMaxChars, jni_env, java_class, "setMaxChars", "(JLandroid/view/View;I)V");

  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jobject)ih->handle, (jint)nc);
  iupAndroid_CheckException(jni_env, "IupTextHelper.setMaxChars");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}

static int androidTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setReadOnlyMultiLine);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setReadOnlySingleLine);

  bool is_read_only = (bool)iupStrBoolean(value);
  IupAndroidTextSubType sub_type = androidTextGetSubType(ih);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID method_id = NULL;

  if (sub_type == IUPANDROIDTEXTSUBTYPE_VIEW)
  {
    method_id = IUPJNI_GetStaticMethodID(IupTextHelper_setReadOnlyMultiLine, jni_env, java_class, "setReadOnlyMultiLine", "(JLandroid/view/View;Z)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jobject)ih->handle, is_read_only);
    iupAndroid_CheckException(jni_env, "IupTextHelper.setReadOnlyMultiLine");
  }
  else if (sub_type == IUPANDROIDTEXTSUBTYPE_FIELD)
  {
    method_id = IUPJNI_GetStaticMethodID(IupTextHelper_setReadOnlySingleLine, jni_env, java_class, "setReadOnlySingleLine", "(JLandroid/view/View;Z)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jobject)ih->handle, is_read_only);
    iupAndroid_CheckException(jni_env, "IupTextHelper.setReadOnlySingleLine");
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 0;
}

static char* androidTextGetReadOnlyAttrib(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getReadOnlyMultiLine);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getReadOnlySingleLine);

  bool is_read_only = false;
  IupAndroidTextSubType sub_type = androidTextGetSubType(ih);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID method_id = NULL;

  if (sub_type == IUPANDROIDTEXTSUBTYPE_VIEW)
  {
    method_id = IUPJNI_GetStaticMethodID(IupTextHelper_getReadOnlyMultiLine, jni_env, java_class, "getReadOnlyMultiLine", "(JLandroid/view/View;)Z");
    is_read_only = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jobject)ih->handle);
    iupAndroid_CheckException(jni_env, "IupTextHelper.getReadOnlyMultiLine");
  }
  else if (sub_type == IUPANDROIDTEXTSUBTYPE_FIELD)
  {
    method_id = IUPJNI_GetStaticMethodID(IupTextHelper_getReadOnlySingleLine, jni_env, java_class, "getReadOnlySingleLine", "(JLandroid/view/View;)Z");
    is_read_only = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, (jobject)ih->handle);
    iupAndroid_CheckException(jni_env, "IupTextHelper.getReadOnlySingleLine");
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return iupStrReturnBoolean(is_read_only);
}

static int androidTextGetCaretPosImpl(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getCaretPos);
  if (!ih->handle) return 0;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_getCaretPos, jni_env, cls,
      "getCaretPos", "(Landroid/view/View;)I");
  jint p = (*jni_env)->CallStaticIntMethod(jni_env, cls, m, (jobject)ih->handle);
  iupAndroid_CheckException(jni_env, "IupTextHelper.getCaretPos");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return (int)p;
}

static void androidTextSetCaretPosImpl(Ihandle* ih, int pos)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setCaretPos);
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_setCaretPos, jni_env, cls,
      "setCaretPos", "(Landroid/view/View;I)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jint)pos);
  iupAndroid_CheckException(jni_env, "IupTextHelper.setCaretPos");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

/* Returns 1 if there's a selection, 0 otherwise. start/end are character offsets, 0-based. */
static int androidTextGetSelectionImpl(Ihandle* ih, int* start, int* end)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getSelectionRange);
  if (!ih->handle) return 0;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_getSelectionRange, jni_env, cls,
      "getSelectionRange", "(Landroid/view/View;)J");
  jlong packed = (*jni_env)->CallStaticLongMethod(jni_env, cls, m, (jobject)ih->handle);
  iupAndroid_CheckException(jni_env, "IupTextHelper.getSelectionRange");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  if (packed == -1LL) return 0;
  if (start) *start = (int)((packed >> 32) & 0xFFFFFFFFLL);
  if (end)   *end   = (int)(packed & 0xFFFFFFFFLL);
  return 1;
}

static void androidTextSetSelectionImpl(Ihandle* ih, int start, int end)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setSelectionRange);
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_setSelectionRange, jni_env, cls,
      "setSelectionRange", "(Landroid/view/View;II)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jint)start, (jint)end);
  iupAndroid_CheckException(jni_env, "IupTextHelper.setSelectionRange");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

static char* androidTextGetSelectedTextImpl(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getSelectedText);
  if (!ih->handle) return NULL;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_getSelectedText, jni_env, cls,
      "getSelectedText", "(Landroid/view/View;)Ljava/lang/String;");
  jstring js = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, cls, m, (jobject)ih->handle);
  iupAndroid_CheckException(jni_env, "IupTextHelper.getSelectedText");
  char* value = js ? iupAndroid_JStringToReturnStr(jni_env, js) : NULL;
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return value;
}

static void androidTextReplaceSelectionImpl(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_replaceSelection);
  if (!ih->handle || !value) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_replaceSelection, jni_env, cls,
      "replaceSelection", "(Landroid/view/View;Ljava/lang/String;)V");
  jstring js = (*jni_env)->NewStringUTF(jni_env, value);
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, js);
  iupAndroid_CheckException(jni_env, "IupTextHelper.replaceSelection");
  (*jni_env)->DeleteLocalRef(jni_env, js);
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

static void androidTextInsertAtCaretImpl(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_insertAtCaret);
  if (!ih->handle || !value) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_insertAtCaret, jni_env, cls,
      "insertAtCaret", "(Landroid/view/View;Ljava/lang/String;)V");
  jstring js = (*jni_env)->NewStringUTF(jni_env, value);
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, js);
  iupAndroid_CheckException(jni_env, "IupTextHelper.insertAtCaret");
  (*jni_env)->DeleteLocalRef(jni_env, js);
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

static void androidTextClipboardImpl(Ihandle* ih, int op)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_clipboardOp);
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_clipboardOp, jni_env, cls,
      "clipboardOp", "(Landroid/view/View;I)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jint)op);
  iupAndroid_CheckException(jni_env, "IupTextHelper.clipboardOp");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

static int androidTextSetCaretAttrib(Ihandle* ih, const char* value)
{
  if (!value || !ih->handle) return 0;

  int pos = 0;
  if (ih->data->is_multiline)
  {
    int lin = 1, col = 1;
    iupStrToIntInt(value, &lin, &col, ',');
    pos = androidTextLinColToPos(ih, lin, col);
  }
  else
  {
    iupStrToInt(value, &pos);
    if (pos < 1) pos = 1;
    pos--;
  }

  androidTextSetCaretPosImpl(ih, pos);
  return 0;
}

static char* androidTextGetCaretAttrib(Ihandle* ih)
{
  if (!ih->handle) return NULL;
  int pos = androidTextGetCaretPosImpl(ih);
  if (ih->data->is_multiline)
  {
    int lin = 1, col = 1;
    androidTextPosToLinCol(ih, pos, &lin, &col);
    return iupStrReturnIntInt(lin, col, ',');
  }
  return iupStrReturnInt(pos + 1);
}

static int androidTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  if (!value || !ih->handle) return 0;
  int pos = 0;
  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;
  androidTextSetCaretPosImpl(ih, pos);
  return 0;
}

static char* androidTextGetCaretPosAttrib(Ihandle* ih)
{
  if (!ih->handle) return NULL;
  return iupStrReturnInt(androidTextGetCaretPosImpl(ih));
}

static int androidTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 0;

  int start, end;
  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    start = 0; end = 0;
  }
  else if (iupStrEqualNoCase(value, "ALL"))
  {
    start = 0; end = -1;
  }
  else if (ih->data->is_multiline)
  {
    int l1 = 1, c1 = 1, l2 = 1, c2 = 1;
    if (sscanf(value, "%d,%d:%d,%d", &l1, &c1, &l2, &c2) != 4) return 0;
    if (l1 < 1 || c1 < 1 || l2 < 1 || c2 < 1) return 0;
    start = androidTextLinColToPos(ih, l1, c1);
    end   = androidTextLinColToPos(ih, l2, c2);
  }
  else
  {
    start = 1; end = 1;
    if (iupStrToIntInt(value, &start, &end, ':') != 2) return 0;
    if (start < 1 || end < 1) return 0;
    start--; end--;
  }

  androidTextSetSelectionImpl(ih, start, end);
  return 0;
}

static char* androidTextGetSelectionAttrib(Ihandle* ih)
{
  if (!ih->handle) return NULL;
  int start = 0, end = 0;
  if (!androidTextGetSelectionImpl(ih, &start, &end)) return NULL;
  if (ih->data->is_multiline)
  {
    int l1, c1, l2, c2;
    androidTextPosToLinCol(ih, start, &l1, &c1);
    androidTextPosToLinCol(ih, end,   &l2, &c2);
    return iupStrReturnStrf("%d,%d:%d,%d", l1, c1, l2, c2);
  }
  return iupStrReturnIntInt(start + 1, end + 1, ':');
}

static int androidTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 0;
  int start, end;
  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    start = 0; end = 0;
  }
  else if (iupStrEqualNoCase(value, "ALL"))
  {
    start = 0; end = -1;
  }
  else
  {
    start = 0; end = 0;
    if (iupStrToIntInt(value, &start, &end, ':') != 2) return 0;
    if (start < 0 || end < 0) return 0;
  }
  androidTextSetSelectionImpl(ih, start, end);
  return 0;
}

static char* androidTextGetSelectionPosAttrib(Ihandle* ih)
{
  if (!ih->handle) return NULL;
  int start = 0, end = 0;
  if (!androidTextGetSelectionImpl(ih, &start, &end)) return NULL;
  return iupStrReturnIntInt(start, end, ':');
}

static char* androidTextGetSelectedTextAttrib(Ihandle* ih)
{
  if (!ih->handle) return NULL;
  return androidTextGetSelectedTextImpl(ih);
}

static int androidTextSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  if (!value || !ih->handle) return 0;
  androidTextReplaceSelectionImpl(ih, value);
  return 0;
}

static int androidTextSetInsertAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value) return 0;
  androidTextInsertAtCaretImpl(ih, value);
  return 0;
}

static int androidTextGetCharCountImpl(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getCharCount);
  if (!ih->handle) return 0;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_getCharCount, jni_env, cls,
      "getCharCount", "(Landroid/view/View;)I");
  jint n = (*jni_env)->CallStaticIntMethod(jni_env, cls, m, (jobject)ih->handle);
  iupAndroid_CheckException(jni_env, "IupTextHelper.getCharCount");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return (int)n;
}

static int androidTextGetLineCountImpl(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getLineCount);
  if (!ih->handle) return 1;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_getLineCount, jni_env, cls,
      "getLineCount", "(Landroid/view/View;)I");
  jint n = (*jni_env)->CallStaticIntMethod(jni_env, cls, m, (jobject)ih->handle);
  iupAndroid_CheckException(jni_env, "IupTextHelper.getLineCount");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return (int)n;
}

static char* androidTextGetLineValueImpl(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getLineValue);
  if (!ih->handle) return NULL;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_getLineValue, jni_env, cls,
      "getLineValue", "(Landroid/view/View;)Ljava/lang/String;");
  jstring js = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, cls, m, (jobject)ih->handle);
  iupAndroid_CheckException(jni_env, "IupTextHelper.getLineValue");
  char* value = js ? iupAndroid_JStringToReturnStr(jni_env, js) : NULL;
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return value;
}

static void androidTextScrollToOffsetImpl(Ihandle* ih, int pos)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_scrollToOffset);
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_scrollToOffset, jni_env, cls,
      "scrollToOffset", "(Landroid/view/View;I)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jint)pos);
  iupAndroid_CheckException(jni_env, "IupTextHelper.scrollToOffset");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

static void androidTextSetAlignmentImpl(Ihandle* ih, int al)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setAlignment);
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_setAlignment, jni_env, cls,
      "setAlignment", "(Landroid/view/View;I)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jint)al);
  iupAndroid_CheckException(jni_env, "IupTextHelper.setAlignment");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

static void androidTextSetFilterModeImpl(Ihandle* ih, int mode)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setFilterMode);
  if (!ih->handle) return;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_setFilterMode, jni_env, cls,
      "setFilterMode", "(Landroid/view/View;I)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jint)mode);
  iupAndroid_CheckException(jni_env, "IupTextHelper.setFilterMode");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
}

static int androidTextSetOverwriteAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setOverwrite);
  if (!ih->handle) return 1;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_setOverwrite, jni_env, cls,
      "setOverwrite", "(Landroid/view/View;Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle, (jboolean)iupStrBoolean(value));
  iupAndroid_CheckException(jni_env, "IupTextHelper.setOverwrite");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 1;
}

static int androidTextSetPaddingAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setPadding);

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  ih->data->horiz_padding = iupdrvScaleNaturalPx(ih->data->horiz_padding);
  ih->data->vert_padding  = iupdrvScaleNaturalPx(ih->data->vert_padding);

  if (!ih->handle) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_setPadding, jni_env, cls,
      "setPadding", "(Landroid/view/View;II)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, (jobject)ih->handle,
      (jint)ih->data->horiz_padding, (jint)ih->data->vert_padding);
  iupAndroid_CheckException(jni_env, "IupTextHelper.setPadding");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 0;
}

static char* androidTextGetScrollVisibleAttrib(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getScrollVisibleBits);
  if (!ih->data->is_multiline || !ih->handle) return (char*)"NO";
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_getScrollVisibleBits, jni_env, cls,
      "getScrollVisibleBits", "(Landroid/view/View;)I");
  jint bits = (*jni_env)->CallStaticIntMethod(jni_env, cls, m, (jobject)ih->handle);
  iupAndroid_CheckException(jni_env, "IupTextHelper.getScrollVisibleBits");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  int sb_h = (bits & 1) ? 1 : 0;
  int sb_v = (bits & 2) ? 1 : 0;
  if (sb_h && sb_v) return (char*)"YES";
  if (sb_h) return (char*)"HORIZONTAL";
  if (sb_v) return (char*)"VERTICAL";
  return (char*)"NO";
}

static char* androidTextGetOverwriteAttrib(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_getOverwrite);
  if (!ih->handle) return NULL;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = IUPJNI_GetStaticMethodID(IupTextHelper_getOverwrite, jni_env, cls,
      "getOverwrite", "(Landroid/view/View;)Z");
  jboolean r = (*jni_env)->CallStaticBooleanMethod(jni_env, cls, m, (jobject)ih->handle);
  iupAndroid_CheckException(jni_env, "IupTextHelper.getOverwrite");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return iupStrReturnBoolean(r ? 1 : 0);
}

static char* androidTextGetCountAttrib(Ihandle* ih)
{
  if (!ih->handle) return NULL;
  return iupStrReturnInt(androidTextGetCharCountImpl(ih));
}

static char* androidTextGetLineCountAttrib(Ihandle* ih)
{
  if (!ih->data->is_multiline) return "1";
  if (!ih->handle) return NULL;
  return iupStrReturnInt(androidTextGetLineCountImpl(ih));
}

static char* androidTextGetLineValueAttrib(Ihandle* ih)
{
  if (!ih->handle) return NULL;
  if (!ih->data->is_multiline) return androidTextGetValueAttrib(ih);
  return androidTextGetLineValueImpl(ih);
}

static int androidTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
  if (!value || !ih->handle) return 0;
  int pos;
  if (ih->data->is_multiline)
  {
    int lin = 1, col = 1;
    iupStrToIntInt(value, &lin, &col, ',');
    if (lin < 1) lin = 1;
    if (col < 1) col = 1;
    pos = androidTextLinColToPos(ih, lin, col);
  }
  else
  {
    pos = 1;
    iupStrToInt(value, &pos);
    if (pos < 1) pos = 1;
    pos--;
  }
  androidTextScrollToOffsetImpl(ih, pos);
  return 0;
}

static int androidTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  if (!value || !ih->handle) return 0;
  int pos = 0;
  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;
  androidTextScrollToOffsetImpl(ih, pos);
  return 0;
}

static int androidTextSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  int al = 0;
  if      (iupStrEqualNoCase(value, "ARIGHT"))  al = 2;
  else if (iupStrEqualNoCase(value, "ACENTER")) al = 1;
  androidTextSetAlignmentImpl(ih, al);
  return 1;
}

static int androidTextSetFilterAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  int mode = 0;
  if      (iupStrEqualNoCase(value, "LOWERCASE")) mode = 1;
  else if (iupStrEqualNoCase(value, "UPPERCASE")) mode = 2;
  else if (iupStrEqualNoCase(value, "NUMBER"))    mode = 3;
  androidTextSetFilterModeImpl(ih, mode);
  return 1;
}

static int androidTextSetClipboardAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value) return 0;
  int op;
  if      (iupStrEqualNoCase(value, "COPY"))  op = 0;
  else if (iupStrEqualNoCase(value, "CUT"))   op = 1;
  else if (iupStrEqualNoCase(value, "PASTE")) op = 2;
  else if (iupStrEqualNoCase(value, "CLEAR")) op = 3;
  else if (iupStrEqualNoCase(value, "UNDO"))  op = 4;
  else if (iupStrEqualNoCase(value, "REDO"))  op = 5;
  else return 0;
  androidTextClipboardImpl(ih, op);
  return 0;
}

static int androidTextSetRemoveFormattingAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_removeFormattingAll);
  (void)value;
  if (!ih->data->is_multiline || !ih->handle) return 0;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupTextHelper_removeFormattingAll, jni_env, java_class,
    "removeFormattingAll", "(Landroid/view/View;)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, (jobject)ih->handle);
  iupAndroid_CheckException(jni_env, "IupTextHelper.removeFormattingAll");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 0;
}

static int androidTextMapMethod(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_createMultiLineText);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_createSpinnerText);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_createSingleLineText);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID method_id;
  jobject java_widget;

  if (ih->data->is_multiline)
  {
    method_id = IUPJNI_GetStaticMethodID(IupTextHelper_createMultiLineText, jni_env, java_class, "createMultiLineText", "(J)Landroid/view/View;");
  }
  else if (iupAttribGetBoolean(ih, "SPIN"))
  {
    method_id = IUPJNI_GetStaticMethodID(IupTextHelper_createSpinnerText, jni_env, java_class, "createSpinnerText", "(J)Landroid/view/View;");
  }
  else
  {
    method_id = IUPJNI_GetStaticMethodID(IupTextHelper_createSingleLineText, jni_env, java_class, "createSingleLineText", "(J)Landroid/view/View;");
  }

  java_widget = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(jni_env, "IupTextHelper.createText");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!java_widget)
    return IUP_ERROR;

  ih->handle = (*jni_env)->NewGlobalRef(jni_env, java_widget);
  (*jni_env)->DeleteLocalRef(jni_env, java_widget);

  iupAndroid_AddWidgetToParent(jni_env, ih);

  if (!iupAttribGetBoolean(ih, "BORDER"))
  {
    IUPJNI_DECLARE_METHOD_ID_STATIC(IupTextHelper_setBorderless);
    jclass cls2 = IUPJNI_FindClass(IupTextHelper, jni_env, "io/github/gen2brain/iupgo/IupTextHelper");
    jmethodID mid = IUPJNI_GetStaticMethodID(IupTextHelper_setBorderless, jni_env, cls2,
      "setBorderless", "(Landroid/view/View;)V");
    (*jni_env)->CallStaticVoidMethod(jni_env, cls2, mid, (jobject)ih->handle);
    iupAndroid_CheckException(jni_env, "IupTextHelper.setBorderless");
    (*jni_env)->DeleteLocalRef(jni_env, cls2);
  }

  if (ih->data->formattags)
    iupTextUpdateFormatTags(ih);

  return IUP_NOERROR;
}

static int androidTextSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  if (!ih->handle) return 1;

  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupTextHelper, env, "io/github/gen2brain/iupgo/IupTextHelper");
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setBgColor", "(Landroid/view/View;III)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)r, (jint)g, (jint)b);
  iupAndroid_CheckException(env, "IupTextHelper.setBgColor");
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

void iupdrvTextInitClass(Iclass* ic)
{
  ic->Map = androidTextMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, androidTextSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "VALUE", androidTextGetValueAttrib, androidTextSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", androidTextGetReadOnlyAttrib, androidTextSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "CUEBANNER", NULL, androidTextSetCueBannerAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, androidTextSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASSWORD", NULL, androidTextSetPasswordAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, androidTextSetNCAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", androidTextGetSpinValueAttrib, androidTextSetSpinValueAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMIN", NULL, NULL, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, NULL, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINWRAP", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINALIGN", NULL, NULL, IUPAF_SAMEASSYSTEM, "RIGHT", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CARET", androidTextGetCaretAttrib, androidTextSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", androidTextGetCaretPosAttrib, androidTextSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", androidTextGetSelectionAttrib, androidTextSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", androidTextGetSelectionPosAttrib, androidTextSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", androidTextGetSelectedTextAttrib, androidTextSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, androidTextSetInsertAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, androidTextSetClipboardAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", androidTextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINECOUNT", androidTextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEVALUE", androidTextGetLineValueAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, androidTextSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, androidTextSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, androidTextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTER", NULL, androidTextSetFilterAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", androidTextGetOverwriteAttrib, androidTextSetOverwriteAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, androidTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", androidTextGetScrollVisibleAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FORMATTING", iupTextGetFormattingAttrib, iupTextSetFormattingAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iupTextSetAddFormatTagAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iupTextSetAddFormatTagHandleAttrib, NULL, NULL, IUPAF_IHANDLE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REMOVEFORMATTING", NULL, androidTextSetRemoveFormattingAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
}
