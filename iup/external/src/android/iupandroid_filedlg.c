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

static char* androidFileDlgGetPatterns(Ihandle* ih, int* index)
{
  const char* p = iupAttribGet(ih, "EXTFILTER");
  int bars;

  *index = 0;
  if (!p)
    return iupStrDup(iupAttribGet(ih, "FILTER"));

  *index = iupAttribGetInt(ih, "FILTERUSED");
  if (*index < 1)
    *index = 1;

  for (bars = 2 * *index - 1; bars > 0; bars--)
  {
    p = strchr(p, '|');
    if (!p)
      return NULL;
    p++;
  }

  return strchr(p, '|') ? iupStrDupUntil(&p, '|') : iupStrDup(p);
}

static void androidFileDlgStoreFiles(Ihandle* ih, char** paths, int count)
{
  char* dir = iupStrFileGetPath(paths[0]);
  int dir_len = dir ? (int)strlen(dir) : 0;
  int name_off = iupAttribGetBoolean(ih, "MULTIVALUEPATH") ? 0 : dir_len;
  int i;

  iupAttribSetStr(ih, "DIRECTORY", dir);

  if (iupAttribGetBoolean(ih, "MULTIPLEFILES"))
  {
    iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);
    for (i = 0; i < count; i++)
      iupAttribSetStrId(ih, "MULTIVALUE", i + 1, paths[i] + name_off);
    iupAttribSetInt(ih, "MULTIVALUECOUNT", count + 1);
  }

  if (count == 1 || !dir)
    iupAttribSetStr(ih, "VALUE", paths[0]);
  else
  {
    int size = dir_len + 1;
    char* value;
    char* v;

    for (i = 0; i < count; i++)
      size += (int)strlen(paths[i] + name_off) + 1;

    value = (char*)malloc(size);
    memcpy(value, dir, dir_len - 1);
    v = value + dir_len - 1;
    *v++ = '|';
    for (i = 0; i < count; i++)
    {
      int len = (int)strlen(paths[i] + name_off);
      memcpy(v, paths[i] + name_off, len);
      v += len;
      *v++ = '|';
    }
    *v = 0;

    iupAttribSetStr(ih, "VALUE", value);
    free(value);
  }

  free(dir);
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

  int filter_used;
  char* patterns = androidFileDlgGetPatterns(ih, &filter_used);
  int multi = iupAttribGetBoolean(ih, "MULTIPLEFILES");

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupFileDlgHelper, jni_env, "io/github/gen2brain/iupgo/IupFileDlgHelper");

  jstring j_patterns = patterns ? iupAndroid_NewStringUTF(jni_env, patterns) : NULL;
  free(patterns);
  jstring j_string = NULL;

  if (is_save)
  {
    char* default_name = iupStrFileGetTitle(iupAttribGet(ih, "FILE"));
    jstring j_name = default_name ? iupAndroid_NewStringUTF(jni_env, default_name) : NULL;
    free(default_name);
    jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "pickSave", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    j_string = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, m, j_patterns, j_name);
    iupAndroid_CheckException(jni_env, "IupFileDlgHelper.pickSave");
    if (j_name) (*jni_env)->DeleteLocalRef(jni_env, j_name);
  }
  else
  {
    jmethodID m = (*jni_env)->GetStaticMethodID(jni_env, java_class, "pickOpen", "(Ljava/lang/String;Z)Ljava/lang/String;");
    j_string = (jstring)(*jni_env)->CallStaticObjectMethod(jni_env, java_class, m, j_patterns, (jboolean)(multi ? JNI_TRUE : JNI_FALSE));
    iupAndroid_CheckException(jni_env, "IupFileDlgHelper.pickOpen");
  }

  if (j_patterns) (*jni_env)->DeleteLocalRef(jni_env, j_patterns);

  const char* result = NULL;
  if (j_string)
    result = (*jni_env)->GetStringUTFChars(jni_env, j_string, NULL);

  if (result && *result)
  {
    char* files = iupStrDup(result);
    int count = iupStrReplace(files, '\n', 0) + 1;
    char** paths = (char**)malloc(count * sizeof(char*));
    char* f = files;
    int i;

    for (i = 0; i < count; i++)
    {
      paths[i] = f;
      f += strlen(f) + 1;
    }
    androidFileDlgStoreFiles(ih, paths, count);
    free(paths);
    free(files);

    if (filter_used)
      iupAttribSetInt(ih, "FILTERUSED", filter_used);
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
    iupAttribSet(ih, "FILTERUSED", NULL);
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

IUP_SDK_API void iupdrvFileDlgInitClass(Iclass* ic)
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
