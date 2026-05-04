/** \file
 * \brief Android Font Mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include <jni.h>
#include <android/log.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_object.h"
#include "iup_classbase.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_EXTERN(IupFontHelper);

/* Must match IupFontHelper.KIND_*. */
#define IUPANDROID_FONT_KIND_DEFAULT 0
#define IUPANDROID_FONT_KIND_BUTTON  1

static int androidFontKindForIhandle(Ihandle* ih)
{
  if (!ih || !ih->iclass || !ih->iclass->name) return IUPANDROID_FONT_KIND_DEFAULT;
  if (strcmp(ih->iclass->name, "button") == 0) return IUPANDROID_FONT_KIND_BUTTON;
  return IUPANDROID_FONT_KIND_DEFAULT;
}

/* android.graphics.Rect width()/height() accessors. Safe on NULL j_rect. */
static void androidFontRectGetSize(JNIEnv* jni_env, jobject j_rect, jint* w, jint* h)
{
  *w = 0;
  *h = 0;
  if (!j_rect) return;
  jclass rect_class = (*jni_env)->GetObjectClass(jni_env, j_rect);
  jmethodID width_id = (*jni_env)->GetMethodID(jni_env, rect_class, "width", "()I");
  jmethodID height_id = (*jni_env)->GetMethodID(jni_env, rect_class, "height", "()I");
  *w = (*jni_env)->CallIntMethod(jni_env, j_rect, width_id);
  *h = (*jni_env)->CallIntMethod(jni_env, j_rect, height_id);
  (*jni_env)->DeleteLocalRef(jni_env, rect_class);
}

char* iupdrvGetSystemFont(void)
{
  /* Static: iup_class stores this pointer as the FONT default. */
  static char systemfont[200] = "";
  if (systemfont[0] != 0)
    return systemfont;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupFontHelper, jni_env, "io/github/gen2brain/iupgo/IupFontHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getSystemFont", "()Ljava/lang/String;");
  jstring j_str = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id);
  iupAndroid_CheckException(jni_env, "IupFontHelper.getSystemFont");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (j_str != NULL)
  {
    const char* utf = (*jni_env)->GetStringUTFChars(jni_env, j_str, NULL);
    if (utf)
    {
      strncpy(systemfont, utf, sizeof(systemfont) - 1);
      systemfont[sizeof(systemfont) - 1] = 0;
      (*jni_env)->ReleaseStringUTFChars(jni_env, j_str, utf);
    }
    (*jni_env)->DeleteLocalRef(jni_env, j_str);
  }

  if (systemfont[0] == 0)
    strcpy(systemfont, "sans-serif, 14");
  return systemfont;
}

/* Android Typeface style flags (see android.graphics.Typeface). */
#define ANDROID_TYPEFACE_NORMAL      0
#define ANDROID_TYPEFACE_BOLD        1
#define ANDROID_TYPEFACE_ITALIC      2
#define ANDROID_TYPEFACE_BOLD_ITALIC 3

/* TypedValue.COMPLEX_UNIT_* selectors accepted by TextView.setTextSize. */
#define ANDROID_COMPLEX_UNIT_PX 0
#define ANDROID_COMPLEX_UNIT_SP 2
#define ANDROID_COMPLEX_UNIT_PT 3

/* Pack an IUP font string into Java parameters: family + style + sizeUnit + size. */
static void androidFontParse(const char* font, char family[1024], int* style, int* size_unit, float* size_value)
{
  int size = 0, is_bold = 0, is_italic = 0, is_underline = 0, is_strikeout = 0;
  family[0] = 0;
  if (!font || !*font || !iupGetFontInfo(font, family, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
  {
    *style = ANDROID_TYPEFACE_NORMAL;
    *size_unit = 0;
    *size_value = 0;
    return;
  }
  /* Typeface.create only knows monospace/serif/sans-serif; map cross-platform aliases. */
  const char* mapped = iupFontGetAndroidName(family);
  if (mapped) iupStrCopyN(family, 1024, mapped);
  *style = (is_bold && is_italic) ? ANDROID_TYPEFACE_BOLD_ITALIC
        : is_bold                ? ANDROID_TYPEFACE_BOLD
        : is_italic              ? ANDROID_TYPEFACE_ITALIC
        : ANDROID_TYPEFACE_NORMAL;
  if (size < 0) { *size_unit = ANDROID_COMPLEX_UNIT_PX; *size_value = (float)(-size); }
  else          { *size_unit = ANDROID_COMPLEX_UNIT_SP; *size_value = (float)size; }
}

static void androidFontApply(JNIEnv* jni_env, jobject widget, const char* typeface, int style, int size_unit, float size, int underline, int strikeout)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupFontHelper_applyFont);

  jclass java_class = IUPJNI_FindClass(IupFontHelper, jni_env, "io/github/gen2brain/iupgo/IupFontHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupFontHelper_applyFont, jni_env, java_class, "applyFont", "(Ljava/lang/Object;Ljava/lang/String;IIFZZ)V");

  jstring j_family = typeface ? (*jni_env)->NewStringUTF(jni_env, typeface) : NULL;
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, widget, j_family, (jint)style, (jint)size_unit, (jfloat)size,
                                   underline ? JNI_TRUE : JNI_FALSE, strikeout ? JNI_TRUE : JNI_FALSE);
  iupAndroid_CheckException(jni_env, "IupFontHelper.applyFont");
  if (j_family)
    (*jni_env)->DeleteLocalRef(jni_env, j_family);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

int iupdrvSetStandardFontAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int* w, int* h)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupFontHelper_getMultiLineStringSize);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupFontHelper, jni_env, "io/github/gen2brain/iupgo/IupFontHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupFontHelper_getMultiLineStringSize, jni_env, java_class, "getMultiLineStringSize", "(JLjava/lang/Object;ILjava/lang/String;)Landroid/graphics/Rect;");

  jstring java_string = (*jni_env)->NewStringUTF(jni_env, str);
  jobject j_rect = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, iupAndroid_RealNativeHandle(ih), (jint)androidFontKindForIhandle(ih), java_string);
  iupAndroid_CheckException(jni_env, "IupFontHelper.getMultiLineStringSize");
  (*jni_env)->DeleteLocalRef(jni_env, java_string);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  jint j_width = 0, j_height = 0;
  androidFontRectGetSize(jni_env, j_rect, &j_width, &j_height);
  if (j_rect) (*jni_env)->DeleteLocalRef(jni_env, j_rect);

  if (w) *w = (int)j_width;
  if (h) *h = (int)j_height;
}

void iupdrvFontGetTextSize(const char* font, const char* str, int len, int* w, int* h)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupFontHelper_getTextSize);
  (void)len;

  char family[1024]; int style, size_unit; float size_value;
  androidFontParse(font, family, &style, &size_unit, &size_value);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupFontHelper, jni_env, "io/github/gen2brain/iupgo/IupFontHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupFontHelper_getTextSize, jni_env, java_class, "getTextSize", "(Ljava/lang/String;IIFLjava/lang/String;)Landroid/graphics/Rect;");

  jstring j_family = family[0] ? (*jni_env)->NewStringUTF(jni_env, family) : NULL;
  jstring j_str = (*jni_env)->NewStringUTF(jni_env, str);
  jobject j_rect = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id,
      j_family, (jint)style, (jint)size_unit, (jfloat)size_value, j_str);
  iupAndroid_CheckException(jni_env, "IupFontHelper.getTextSize");
  if (j_family) (*jni_env)->DeleteLocalRef(jni_env, j_family);
  (*jni_env)->DeleteLocalRef(jni_env, j_str);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  jint j_width = 0, j_height = 0;
  androidFontRectGetSize(jni_env, j_rect, &j_width, &j_height);
  (*jni_env)->DeleteLocalRef(jni_env, j_rect);

  /* HW px -> canvas-coord; float density + ceil preserves exact px round-trip with iupdrvScaleNaturalPx */
  float d = iupAndroid_GetDisplayDensity(); if (d < 1.0f) d = 1.0f;
  if (w) *w = (int)ceilf((float)j_width  / d);
  if (h) *h = (int)ceilf((float)j_height / d);
}

int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupFontHelper_getStringWidth);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupFontHelper, jni_env, "io/github/gen2brain/iupgo/IupFontHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupFontHelper_getStringWidth, jni_env, java_class, "getStringWidth", "(JLjava/lang/Object;ILjava/lang/String;)I");

  jstring java_string = (*jni_env)->NewStringUTF(jni_env, str);
  jint j_width = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, iupAndroid_RealNativeHandle(ih), (jint)androidFontKindForIhandle(ih), java_string);
  iupAndroid_CheckException(jni_env, "IupFontHelper.getStringWidth");
  (*jni_env)->DeleteLocalRef(jni_env, java_string);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  return (int)j_width;
}

void iupdrvFontGetCharSize(Ihandle* ih, int* charwidth, int* charheight)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupFontHelper_getCharSize);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupFontHelper, jni_env, "io/github/gen2brain/iupgo/IupFontHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupFontHelper_getCharSize, jni_env, java_class, "getCharSize", "(JLjava/lang/Object;I)Landroid/graphics/Rect;");

  jobject j_rect = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, (jlong)(intptr_t)ih, iupAndroid_RealNativeHandle(ih), (jint)androidFontKindForIhandle(ih));
  iupAndroid_CheckException(jni_env, "IupFontHelper.getCharSize");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  jint j_width = 0, j_height = 0;
  androidFontRectGetSize(jni_env, j_rect, &j_width, &j_height);
  (*jni_env)->DeleteLocalRef(jni_env, j_rect);

  if (charwidth) *charwidth = (int)j_width;
  if (charheight) *charheight = (int)j_height;
}

void iupdrvFontGetFontDim(const char* font, int* max_width, int* line_height, int* ascent, int* descent)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupFontHelper_getFontDim);

  if (max_width) *max_width = 0;
  if (line_height) *line_height = 0;
  if (ascent) *ascent = 0;
  if (descent) *descent = 0;

  char family[1024]; int style, size_unit; float size_value;
  androidFontParse(font, family, &style, &size_unit, &size_value);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupFontHelper, jni_env, "io/github/gen2brain/iupgo/IupFontHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupFontHelper_getFontDim, jni_env, java_class, "getFontDim", "(Ljava/lang/String;IIF)[I");

  jstring j_family = family[0] ? (*jni_env)->NewStringUTF(jni_env, family) : NULL;
  jintArray j_arr = (jintArray)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id,
      j_family, (jint)style, (jint)size_unit, (jfloat)size_value);
  iupAndroid_CheckException(jni_env, "IupFontHelper.getFontDim");
  if (j_family) (*jni_env)->DeleteLocalRef(jni_env, j_family);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!j_arr) return;
  jint vals[4] = {0, 0, 0, 0};
  (*jni_env)->GetIntArrayRegion(jni_env, j_arr, 0, 4, vals);
  (*jni_env)->DeleteLocalRef(jni_env, j_arr);

  /* HW px -> canvas-coord (matches iupdrvFontGetTextSize and Canvas density transform). */
  float d = iupAndroid_GetDisplayDensity(); if (d < 1.0f) d = 1.0f;
  if (max_width)   *max_width   = (int)ceilf((float)vals[0] / d);
  if (line_height) *line_height = (int)ceilf((float)vals[1] / d);
  if (ascent)      *ascent      = (int)ceilf((float)vals[2] / d);
  if (descent)     *descent     = (int)ceilf((float)vals[3] / d);
}

int iupdrvFontGetFamilyList(char*** list)
{
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  if (!jni_env)
  {
    *list = NULL;
    return 0;
  }

  IUPJNI_DECLARE_METHOD_ID_STATIC(IupFontHelper_getSystemFamilyList);

  jclass java_class = IUPJNI_FindClass(IupFontHelper, jni_env, "io/github/gen2brain/iupgo/IupFontHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupFontHelper_getSystemFamilyList, jni_env, java_class, "getSystemFamilyList", "()[Ljava/lang/String;");

  jobjectArray j_array = (jobjectArray)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id);
  iupAndroid_CheckException(jni_env, "IupFontHelper.getSystemFamilyList");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!j_array)
  {
    *list = NULL;
    return 0;
  }

  jsize count = (*jni_env)->GetArrayLength(jni_env, j_array);
  if (count <= 0)
  {
    (*jni_env)->DeleteLocalRef(jni_env, j_array);
    *list = NULL;
    return 0;
  }

  char** temp = (char**)malloc(count * sizeof(char*));
  if (!temp)
  {
    (*jni_env)->DeleteLocalRef(jni_env, j_array);
    *list = NULL;
    return 0;
  }

  int out = 0;
  for (jsize i = 0; i < count; i++)
  {
    jstring j_str = (jstring)(*jni_env)->GetObjectArrayElement(jni_env, j_array, i);
    if (!j_str) continue;
    const char* c_str = (*jni_env)->GetStringUTFChars(jni_env, j_str, NULL);
    if (c_str)
    {
      temp[out++] = iupStrDup(c_str);
      (*jni_env)->ReleaseStringUTFChars(jni_env, j_str, c_str);
    }
    (*jni_env)->DeleteLocalRef(jni_env, j_str);
  }
  (*jni_env)->DeleteLocalRef(jni_env, j_array);

  if (out == 0)
  {
    free(temp);
    *list = NULL;
    return 0;
  }

  /* realloc-NULL keeps the original buffer; oversize is harmless. */
  char** shrunk = (char**)realloc(temp, out * sizeof(char*));
  *list = shrunk ? shrunk : temp;
  return out;
}

void iupdrvFontInit(void)
{
}

void iupdrvFontFinish(void)
{
}

int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
  char typeface[1024] = "";
  int size = 0, is_bold = 0, is_italic = 0, is_underline = 0, is_strikeout = 0;

  if (!iupGetFontInfo(value, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return 0;

  /* Typeface.create only knows monospace/serif/sans-serif; map cross-platform aliases. */
  const char* mapped = iupFontGetAndroidName(typeface);
  if (mapped) iupStrCopyN(typeface, sizeof(typeface), mapped);

  if (ih->handle && ih->iclass->nativetype != IUP_TYPEVOID)
  {
    int style = ANDROID_TYPEFACE_NORMAL;
    if (is_bold && is_italic) style = ANDROID_TYPEFACE_BOLD_ITALIC;
    else if (is_bold)         style = ANDROID_TYPEFACE_BOLD;
    else if (is_italic)       style = ANDROID_TYPEFACE_ITALIC;

    /* size>0 maps to SP (mobile body scale); size<0 stays absolute pixels. */
    int size_unit;
    float size_value;
    if (size < 0)
    {
      size_unit = ANDROID_COMPLEX_UNIT_PX;
      size_value = (float)(-size);
    }
    else
    {
      size_unit = ANDROID_COMPLEX_UNIT_SP;
      size_value = (float)size;
    }

    JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
    androidFontApply(jni_env, (jobject)ih->handle, typeface[0] ? typeface : NULL, style, size_unit, size_value, is_underline, is_strikeout);
  }

  iupBaseUpdateAttribFromFont(ih);
  return 1;
}
