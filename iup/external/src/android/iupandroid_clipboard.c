/** \file
 * \brief Clipboard for the Android Driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <string.h>

#include <jni.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_attrib.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupClipboardHelper);

static int androidClipboardSetTextAttrib(Ihandle *ih, const char *value)
{
  (void)ih;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupClipboardHelper, jni_env, "io/github/gen2brain/iupgo/IupClipboardHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setText", "(Ljava/lang/String;)V");

  jstring j_text = value ? (*jni_env)->NewStringUTF(jni_env, value) : NULL;
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, j_text);
  iupAndroid_CheckException(jni_env, "IupClipboardHelper.setText");
  if (j_text) (*jni_env)->DeleteLocalRef(jni_env, j_text);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 0;
}

static char* androidClipboardGetTextAttrib(Ihandle *ih)
{
  (void)ih;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupClipboardHelper, jni_env, "io/github/gen2brain/iupgo/IupClipboardHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getText", "()Ljava/lang/String;");

  jstring j_text = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id);
  iupAndroid_CheckException(jni_env, "IupClipboardHelper.getText");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!j_text) return NULL;

  const char* chars = (*jni_env)->GetStringUTFChars(jni_env, j_text, NULL);
  char* value = iupStrReturnStr(chars);
  (*jni_env)->ReleaseStringUTFChars(jni_env, j_text, chars);
  (*jni_env)->DeleteLocalRef(jni_env, j_text);
  return value;
}

static char* androidClipboardGetTextAvailableAttrib(Ihandle *ih)
{
  (void)ih;
  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupClipboardHelper, jni_env, "io/github/gen2brain/iupgo/IupClipboardHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "isTextAvailable", "()Z");

  jboolean available = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, method_id);
  iupAndroid_CheckException(jni_env, "IupClipboardHelper.isTextAvailable");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return iupStrReturnBoolean((int)available);
}

static int androidClipboardSetAddFormatAttrib(Ihandle *ih, const char *value)
{
  (void)ih; (void)value;
  return 0;
}

static char* androidClipboardGetFormatAvailableAttrib(Ihandle *ih)
{
  char* format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
    return NULL;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupClipboardHelper, jni_env, "io/github/gen2brain/iupgo/IupClipboardHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "isFormatAvailable", "(Ljava/lang/String;)Z");

  jstring j_mime = (*jni_env)->NewStringUTF(jni_env, format);
  jboolean available = (*jni_env)->CallStaticBooleanMethod(jni_env, java_class, method_id, j_mime);
  iupAndroid_CheckException(jni_env, "IupClipboardHelper.isFormatAvailable");
  (*jni_env)->DeleteLocalRef(jni_env, j_mime);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return iupStrReturnBoolean((int)available);
}

static int androidClipboardSetFormatDataStringAttrib(Ihandle *ih, const char *value)
{
  char* format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
    return 0;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupClipboardHelper, jni_env, "io/github/gen2brain/iupgo/IupClipboardHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "setFormatData", "(Ljava/lang/String;Ljava/lang/String;)V");

  jstring j_mime = (*jni_env)->NewStringUTF(jni_env, format);
  jstring j_text = value ? (*jni_env)->NewStringUTF(jni_env, value) : NULL;
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, j_mime, j_text);
  iupAndroid_CheckException(jni_env, "IupClipboardHelper.setFormatData");
  if (j_text) (*jni_env)->DeleteLocalRef(jni_env, j_text);
  (*jni_env)->DeleteLocalRef(jni_env, j_mime);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (value)
    iupAttribSetInt(ih, "FORMATDATASIZE", (int)strlen(value) + 1);
  return 0;
}

static char* androidClipboardGetFormatDataStringAttrib(Ihandle *ih)
{
  char* format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
    return NULL;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupClipboardHelper, jni_env, "io/github/gen2brain/iupgo/IupClipboardHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getFormatData", "(Ljava/lang/String;)Ljava/lang/String;");

  jstring j_mime = (*jni_env)->NewStringUTF(jni_env, format);
  jstring j_text = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, method_id, j_mime);
  iupAndroid_CheckException(jni_env, "IupClipboardHelper.getFormatData");
  (*jni_env)->DeleteLocalRef(jni_env, j_mime);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (!j_text)
  {
    iupAttribSetInt(ih, "FORMATDATASIZE", 0);
    return NULL;
  }

  const char* chars = (*jni_env)->GetStringUTFChars(jni_env, j_text, NULL);
  char* value = iupStrReturnStr(chars);
  iupAttribSetInt(ih, "FORMATDATASIZE", (int)strlen(chars) + 1);
  (*jni_env)->ReleaseStringUTFChars(jni_env, j_text, chars);
  (*jni_env)->DeleteLocalRef(jni_env, j_text);
  return value;
}

Ihandle* IupClipboard(void)
{
  return IupCreate("clipboard");
}

Iclass* iupClipboardNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "clipboard";
  ic->format = NULL;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupClipboardNewClass;

  iupClassRegisterAttribute(ic, "TEXT", androidClipboardGetTextAttrib, androidClipboardSetTextAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", androidClipboardGetTextAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* custom string formats ride a ClipData with a custom MIME (FORMAT) on a text item; images and
     binary FORMATDATA would need a content-URI/ContentProvider, so they stay unsupported */
  iupClassRegisterAttribute(ic, "NATIVEIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMAT", NULL, androidClipboardSetAddFormatAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATAVAILABLE", androidClipboardGetFormatAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATA", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASTRING", androidClipboardGetFormatDataStringAttrib, androidClipboardSetFormatDataStringAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  return ic;
}
