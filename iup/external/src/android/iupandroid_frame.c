/** \file
 * \brief Frame Control
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
#include "iup_drvfont.h"
#include "iup_frame.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupFrameHelper);

/* GlobalRef to the inner IupAndroidFixed; cached for GetInnerNativeContainerHandle */
#define IUPANDROID_FRAME_INNER "_IUPANDROID_FRAME_INNER"

static int androidFrameCallIntGetter(Ihandle* ih, const char* method_name)
{
  if (!ih || !ih->handle)
    return 0;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupFrameHelper, jni_env, "io/github/gen2brain/iupgo/IupFrameHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, "(Lio/github/gen2brain/iupgo/IupAndroidFrame;)I");
  jint ret = (*jni_env)->CallStaticIntMethod(jni_env, java_class, method_id, ih->handle);
  iupAndroid_CheckException(jni_env, method_name);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return (int)ret;
}

static void androidFrameCallColorSetter(Ihandle* ih, const char* method_name, unsigned char r, unsigned char g, unsigned char b)
{
  if (!ih || !ih->handle)
    return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupFrameHelper, jni_env, "io/github/gen2brain/iupgo/IupFrameHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, method_name, "(Lio/github/gen2brain/iupgo/IupAndroidFrame;III)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, (jint)r, (jint)g, (jint)b);
  iupAndroid_CheckException(jni_env, method_name);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvFrameGetDecorOffset(Ihandle* ih, int* x, int* y)
{
  int title_h = 0;
  int stroke = 1;
  if (ih && ih->handle)
  {
    title_h = androidFrameCallIntGetter(ih, "getTitleHeight");
    stroke = androidFrameCallIntGetter(ih, "getStrokePx");
  }
  if (x) *x = stroke;
  if (y) *y = stroke + title_h;
}

int iupdrvFrameHasClientOffset(Ihandle* ih)
{
  /* inner IupAndroidFixed already offsets children past the decoration */
  (void)ih;
  return 0;
}

int iupdrvFrameGetTitleHeight(Ihandle* ih, int* h)
{
  const char* title = iupAttribGet(ih, "TITLE");
  if (!title || !*title)
  {
    if (h) *h = 0;
    return 1;
  }

  if (ih->handle)
  {
    if (h) *h = androidFrameCallIntGetter(ih, "getTitleHeight");
  }
  else
  {
    /* Pre-map fallback so natural size is close to final. */
    int char_h = 0;
    iupdrvFontGetCharSize(ih, NULL, &char_h);
    if (h) *h = char_h;
  }
  return 1;
}

int iupdrvFrameGetDecorSize(Ihandle* ih, int* w, int* h)
{
  int title_h;
  int stroke = ih->handle ? androidFrameCallIntGetter(ih, "getStrokePx") : iupAndroid_DpToPx(1.0f);
  iupdrvFrameGetTitleHeight(ih, &title_h);

  if (w) *w = 2 * stroke;
  if (h) *h = 2 * stroke + title_h;
  return 1;
}

static int androidFrameSetTitleAttrib(Ihandle* ih, const char* value)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupFrameHelper_setTitle);

  if (!ih->handle)
    return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupFrameHelper, jni_env, "io/github/gen2brain/iupgo/IupFrameHelper");
  jmethodID method_id = IUPJNI_GetStaticMethodID(IupFrameHelper_setTitle, jni_env, java_class, "setTitle", "(Lio/github/gen2brain/iupgo/IupAndroidFrame;Ljava/lang/String;)V");

  char* stripped = value ? iupStrProcessMnemonic(value, NULL, 0) : NULL;
  jstring j_string = stripped ? (*jni_env)->NewStringUTF(jni_env, stripped) : NULL;
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, ih->handle, j_string);
  iupAndroid_CheckException(jni_env, "IupFrameHelper.setTitle");
  if (j_string)
    (*jni_env)->DeleteLocalRef(jni_env, j_string);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  if (stripped && stripped != value) free(stripped);
  return 1;
}

static int androidFrameSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  androidFrameCallColorSetter(ih, "setFgColor", r, g, b);
  return 1;
}

static int androidFrameSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  androidFrameCallColorSetter(ih, "setBgColor", r, g, b);
  return 1;
}

static int androidFrameSetFrameColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  androidFrameCallColorSetter(ih, "setStrokeColor", r, g, b);
  return 1;
}

/* TypedValue: PX=0, SP=2. Positive IUP sizes are SP, negative are raw PX. */
#define IUPANDROID_FRAME_TYPEFACE_NORMAL      0
#define IUPANDROID_FRAME_TYPEFACE_BOLD        1
#define IUPANDROID_FRAME_TYPEFACE_ITALIC      2
#define IUPANDROID_FRAME_TYPEFACE_BOLD_ITALIC 3
#define IUPANDROID_FRAME_COMPLEX_UNIT_PX      0
#define IUPANDROID_FRAME_COMPLEX_UNIT_SP      2

static int androidFrameSetFontAttrib(Ihandle* ih, const char* value)
{
  /* Inheritance to children is handled by the core path. */
  if (!iupdrvSetFontAttrib(ih, value)) return 0;
  if (!ih->handle) return 1;

  char typeface[1024] = "";
  int size = 0, is_bold = 0, is_italic = 0, is_underline = 0, is_strikeout = 0;
  if (!value || !value[0]) return 1;
  if (!iupGetFontInfo(value, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return 1;

  int style = IUPANDROID_FRAME_TYPEFACE_NORMAL;
  if (is_bold && is_italic) style = IUPANDROID_FRAME_TYPEFACE_BOLD_ITALIC;
  else if (is_bold)         style = IUPANDROID_FRAME_TYPEFACE_BOLD;
  else if (is_italic)       style = IUPANDROID_FRAME_TYPEFACE_ITALIC;

  int size_unit = (size < 0) ? IUPANDROID_FRAME_COMPLEX_UNIT_PX : IUPANDROID_FRAME_COMPLEX_UNIT_SP;
  float size_value = (size < 0) ? (float)(-size) : (float)size;

  /* Default-font frames render the title a touch smaller than body text. */
  const char* defaults = IupGetGlobal("DEFAULTFONT");
  if (defaults && iupStrEqual(value, defaults))
  {
    float min_size = (size_unit == IUPANDROID_FRAME_COMPLEX_UNIT_PX) ? 8.0f : 11.0f;
    size_value -= 3.0f;
    if (size_value < min_size) size_value = min_size;
  }

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupFrameHelper, jni_env, "io/github/gen2brain/iupgo/IupFrameHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "setTitleFont",
      "(Lio/github/gen2brain/iupgo/IupAndroidFrame;Ljava/lang/String;IIF)V");
  jstring j_family = typeface[0] ? (*jni_env)->NewStringUTF(jni_env, typeface) : NULL;
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, ih->handle, j_family, (jint)style, (jint)size_unit, (jfloat)size_value);
  iupAndroid_CheckException(jni_env, "IupFrameHelper.setTitleFont");
  if (j_family) (*jni_env)->DeleteLocalRef(jni_env, j_family);
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 1;
}

static int androidFrameSetSunkenAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 1;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass cls = IUPJNI_FindClass(IupFrameHelper, jni_env, "io/github/gen2brain/iupgo/IupFrameHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, cls, "setSunken", "(Lio/github/gen2brain/iupgo/IupAndroidFrame;Z)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, cls, m, ih->handle, (jboolean)iupStrBoolean(value));
  iupAndroid_CheckException(jni_env, "IupFrameHelper.setSunken");
  (*jni_env)->DeleteLocalRef(jni_env, cls);
  return 1;
}

static void* androidFrameGetInnerNativeContainerHandle(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return iupAttribGet(ih, IUPANDROID_FRAME_INNER);
}

static int androidFrameMapMethod(Ihandle* ih)
{
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupFrameHelper_createFrame);
  IUPJNI_DECLARE_METHOD_ID_STATIC(IupFrameHelper_getInner);

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupFrameHelper, jni_env, "io/github/gen2brain/iupgo/IupFrameHelper");

  jmethodID create_id = IUPJNI_GetStaticMethodID(IupFrameHelper_createFrame, jni_env, java_class, "createFrame", "(J)Lio/github/gen2brain/iupgo/IupAndroidFrame;");
  jobject frame = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, create_id, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(jni_env, "IupFrameHelper.createFrame");

  if (!frame)
  {
    (*jni_env)->DeleteLocalRef(jni_env, java_class);
    return IUP_ERROR;
  }

  ih->handle = (jobject)((*jni_env)->NewGlobalRef(jni_env, frame));

  /* GlobalRef the inner Fixed once so GetInnerNativeContainerHandle skips JNI on repeats */
  jmethodID inner_id = IUPJNI_GetStaticMethodID(IupFrameHelper_getInner, jni_env, java_class, "getInner", "(Lio/github/gen2brain/iupgo/IupAndroidFrame;)Lio/github/gen2brain/iupgo/IupAndroidFixed;");
  jobject inner = (*jni_env)->CallStaticObjectMethod(jni_env, java_class, inner_id, ih->handle);
  iupAndroid_CheckException(jni_env, "IupFrameHelper.getInner");
  iupAttribSet(ih, IUPANDROID_FRAME_INNER, (char*)((*jni_env)->NewGlobalRef(jni_env, inner)));

  (*jni_env)->DeleteLocalRef(jni_env, inner);
  (*jni_env)->DeleteLocalRef(jni_env, frame);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  iupAndroid_AddWidgetToParent(jni_env, ih);

  return IUP_NOERROR;
}

static void androidFrameUnMapMethod(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();

  jobject inner = (jobject)iupAttribGet(ih, IUPANDROID_FRAME_INNER);
  if (inner)
  {
    (*jni_env)->DeleteGlobalRef(jni_env, inner);
    iupAttribSet(ih, IUPANDROID_FRAME_INNER, NULL);
  }

  iupAndroid_RemoveFromParent(jni_env, ih);
  iupAndroid_ReleaseIhandle(jni_env, ih);
}

void iupdrvFrameInitClass(Iclass* ic)
{
  ic->Map = androidFrameMapMethod;
  ic->UnMap = androidFrameUnMapMethod;
  ic->GetInnerNativeContainerHandle = androidFrameGetInnerNativeContainerHandle;

  iupClassRegisterAttribute(ic, "TITLE", NULL, androidFrameSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, androidFrameSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", iupFrameGetBgColorAttrib, androidFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FRAMECOLOR", NULL, androidFrameSetFrameColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* FONT inherits to children (core handles this); driver pushes it to the title TextPaint too. */
  iupClassRegisterAttribute(ic, "FONT", NULL, androidFrameSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SUNKEN", NULL, androidFrameSetSunkenAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
