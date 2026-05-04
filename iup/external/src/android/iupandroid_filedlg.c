/** \file
 * \brief IupFileDlg pre-defined dialog
 *
 * SAF returns content:// URIs, not paths; we stage through getCacheDir()
 * so FILE stays fopen-able. See IupFileDlgHelper.java for details.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>

#include <jni.h>

#include "iup.h"

#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupFileDlgHelper);

/* Glob filter to MIME, first extension wins. Unknown falls to any. */
static const char* androidFileDlgFilterToMime(const char* filter)
{
  if (!filter || !*filter) return "*/*";

  const char* dot = strrchr(filter, '.');
  if (!dot) return "*/*";
  dot++;

  if (iupStrEqualNoCase(dot, "txt") || iupStrEqualNoCase(dot, "log") ||
      iupStrEqualNoCase(dot, "md")  || iupStrEqualNoCase(dot, "csv"))
    return "text/*";
  if (iupStrEqualNoCase(dot, "html") || iupStrEqualNoCase(dot, "htm"))
    return "text/html";
  if (iupStrEqualNoCase(dot, "xml"))
    return "text/xml";
  if (iupStrEqualNoCase(dot, "json"))
    return "application/json";
  if (iupStrEqualNoCase(dot, "pdf"))
    return "application/pdf";
  if (iupStrEqualNoCase(dot, "zip"))
    return "application/zip";
  if (iupStrEqualNoCase(dot, "png") || iupStrEqualNoCase(dot, "jpg") ||
      iupStrEqualNoCase(dot, "jpeg") || iupStrEqualNoCase(dot, "gif") ||
      iupStrEqualNoCase(dot, "bmp") || iupStrEqualNoCase(dot, "webp"))
    return "image/*";
  if (iupStrEqualNoCase(dot, "mp3") || iupStrEqualNoCase(dot, "wav") ||
      iupStrEqualNoCase(dot, "ogg") || iupStrEqualNoCase(dot, "flac"))
    return "audio/*";
  if (iupStrEqualNoCase(dot, "mp4") || iupStrEqualNoCase(dot, "webm") ||
      iupStrEqualNoCase(dot, "mkv") || iupStrEqualNoCase(dot, "mov"))
    return "video/*";

  return "*/*";
}

/* EXTFILTER = "Name|*.ext|Name2|*.ext"; pick first pattern (SAF can't show filter groups) */
static const char* androidFileDlgExtFilterToMime(const char* extfilter)
{
  if (!extfilter || !*extfilter) return NULL;
  const char* first_bar = strchr(extfilter, '|');
  if (!first_bar) return androidFileDlgFilterToMime(extfilter);
  const char* second_bar = strchr(first_bar + 1, '|');
  /* Name|Pattern[|Name|Pattern...]; pattern starts after first '|'. */
  size_t len = second_bar ? (size_t)(second_bar - first_bar - 1) : strlen(first_bar + 1);
  char buf[256];
  if (len >= sizeof(buf)) len = sizeof(buf) - 1;
  memcpy(buf, first_bar + 1, len);
  buf[len] = '\0';
  return androidFileDlgFilterToMime(buf);
}

static int androidFileDlgPopup(Ihandle* ih, int x, int y)
{
  (void)x; (void)y;

  const char* type = iupAttribGetStr(ih, "DIALOGTYPE");
  int is_save = iupStrEqualNoCase(type, "SAVE");
  int is_dir  = iupStrEqualNoCase(type, "DIR");

  if (is_dir)
  {
    /* SAF directory picks return tree URIs, not fopen paths; bail */
    iupAttribSet(ih, "STATUS", "-1");
    iupAttribSet(ih, "VALUE", NULL);
    return IUP_NOERROR;
  }

  const char* mime = NULL;
  const char* extfilter = iupAttribGet(ih, "EXTFILTER");
  if (extfilter)
    mime = androidFileDlgExtFilterToMime(extfilter);
  else
    mime = androidFileDlgFilterToMime(iupAttribGet(ih, "FILTER"));

  int multi = iupAttribGetBoolean(ih, "MULTIPLEFILES");

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupFileDlgHelper, jni_env, "io/github/gen2brain/iupgo/IupFileDlgHelper");

  jstring j_mime = mime ? (*jni_env)->NewStringUTF(jni_env, mime) : NULL;
  jstring j_string = NULL;

  if (is_save)
  {
    const char* default_name = iupAttribGet(ih, "FILE");
    jstring j_name = default_name ? (*jni_env)->NewStringUTF(jni_env, default_name) : NULL;
    jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "pickSave", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    j_string = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, m, j_mime, j_name);
    iupAndroid_CheckException(jni_env, "IupFileDlgHelper.pickSave");
    if (j_name) (*jni_env)->DeleteLocalRef(jni_env, j_name);
  }
  else
  {
    jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "pickOpen", "(Ljava/lang/String;Z)Ljava/lang/String;");
    j_string = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, m, j_mime, (jboolean)(multi ? JNI_TRUE : JNI_FALSE));
    iupAndroid_CheckException(jni_env, "IupFileDlgHelper.pickOpen");
  }

  if (j_mime) (*jni_env)->DeleteLocalRef(jni_env, j_mime);

  const char* result = NULL;
  if (j_string)
    result = (*jni_env)->GetStringUTFChars(jni_env, j_string, NULL);

  if (result && *result)
  {
    iupAttribSetStr(ih, "VALUE", result);
    iupAttribSet(ih, "STATUS", is_save ? "1" : "0");
    iupAttribSet(ih, "FILEEXIST", is_save ? "NO" : "YES");

    /* VALUE_URI = stable SAF URI for recent-files; VALUE stays a cache path for fopen. */
    jmethodID m_uri = (*jni_env)->GetStaticMethodID(jni_env, java_class, "getLastUri", "()Ljava/lang/String;");
    jstring j_uri = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, m_uri);
    iupAndroid_CheckException(jni_env, "IupFileDlgHelper.getLastUri");
    if (j_uri)
    {
      const char* c_uri = (*jni_env)->GetStringUTFChars(jni_env, j_uri, NULL);
      iupAttribSetStr(ih, "VALUE_URI", c_uri);
      (*jni_env)->ReleaseStringUTFChars(jni_env, j_uri, c_uri);
      (*jni_env)->DeleteLocalRef(jni_env, j_uri);
    }

    /* SAVE flushes on destroy */
    if (is_save)
      iupAttribSet(ih, "_IUP_ANDROID_FILEDLG_PENDING_SAVE", "1");
  }
  else
  {
    iupAttribSet(ih, "STATUS", "-1");
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "VALUE_URI", NULL);
  }

  (*jni_env)->DeleteLocalRef(jni_env, java_class);

  if (result) (*jni_env)->ReleaseStringUTFChars(jni_env, j_string, result);
  if (j_string) (*jni_env)->DeleteLocalRef(jni_env, j_string);

  return IUP_NOERROR;
}

static void androidFileDlgDestroyMethod(Ihandle* ih)
{
  if (!iupAttribGet(ih, "_IUP_ANDROID_FILEDLG_PENDING_SAVE")) return;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupFileDlgHelper, jni_env, "io/github/gen2brain/iupgo/IupFileDlgHelper");
  jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "commitSave", "()V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, m);
  iupAndroid_CheckException(jni_env, "IupFileDlgHelper.commitSave");
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
}

void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = androidFileDlgPopup;
  ic->Destroy  = androidFileDlgDestroyMethod;

  iupClassRegisterAttribute(ic, "EXTFILTER",     NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERINFO",    NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERUSED",    NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MULTIPLEFILES", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  /* Android-only; SAF content:// URI for the last successful pick. */
  iupClassRegisterAttribute(ic, "VALUE_URI",     NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
