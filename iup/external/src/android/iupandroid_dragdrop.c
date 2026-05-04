/** \file
 * \brief Android Drag&Drop Functions
 *
 * Long-press starts a drag; payload (source ih, types, wantMove) rides
 * in localState. DROPFILESTARGET accepts cross-app ClipData URIs via
 * cache staging. DRAGCURSORCOPY is not exposed by Android.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <jni.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_image.h"
#include "iup_key.h"

#include "iupandroid_drv.h"
#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_STATIC(IupDragDropHelper);

/* Comma-separated DRAGTYPES/DROPTYPES into a Java String[]. */
static jobjectArray androidDragDropParseTypes(JNIEnv* jni_env, const char* value)
{
  jclass string_cls = (*jni_env)->FindClass(jni_env, "java/lang/String");
  if (!value || !*value)
  {
    jobjectArray arr = (*jni_env)->NewObjectArray(jni_env, 0, string_cls, NULL);
    (*jni_env)->DeleteLocalRef(jni_env, string_cls);
    return arr;
  }

  int count = 1;
  for (const char* p = value; *p; p++) if (*p == ',') count++;

  jobjectArray arr = (*jni_env)->NewObjectArray(jni_env, count, string_cls, NULL);
  char buf[128];
  size_t bi = 0;
  int idx = 0;
  for (const char* p = value; ; p++)
  {
    if (*p == ',' || *p == '\0')
    {
      while (bi > 0 && buf[bi-1] == ' ') bi--;
      buf[bi] = '\0';
      const char* start = buf;
      while (*start == ' ') start++;
      jstring js = (*jni_env)->NewStringUTF(jni_env, start);
      (*jni_env)->SetObjectArrayElement(jni_env, arr, idx++, js);
      (*jni_env)->DeleteLocalRef(jni_env, js);
      bi = 0;
      if (*p == '\0') break;
    }
    else if (bi + 1 < sizeof(buf))
    {
      buf[bi++] = *p;
    }
  }

  (*jni_env)->DeleteLocalRef(jni_env, string_cls);
  return arr;
}

static int androidDragDropTypeMatches(const char* drop_types, const char* type)
{
  if (!drop_types || !*drop_types || !type) return 0;
  size_t tlen = strlen(type);
  const char* p = drop_types;
  while (*p)
  {
    while (*p == ' ' || *p == ',') p++;
    const char* start = p;
    while (*p && *p != ',') p++;
    size_t len = (size_t)(p - start);
    while (len > 0 && start[len-1] == ' ') len--;
    if (len == tlen && strncmp(start, type, len) == 0)
      return 1;
  }
  return 0;
}


static int androidDragDropCallSet(Ihandle* ih, const char* method, int enable)
{
  jobject widget = iupAndroid_RealNativeHandle(ih);
  if (!widget) return 1;

  JNIEnv* jni_env = iupAndroid_GetEnvThreadSafe();
  jclass java_class = IUPJNI_FindClass(IupDragDropHelper, jni_env, "io/github/gen2brain/iupgo/IupDragDropHelper");
  jmethodID method_id = (*jni_env)->GetStaticMethodID(jni_env, java_class, method, "(Ljava/lang/Object;JZ)V");
  (*jni_env)->CallStaticVoidMethod(jni_env, java_class, method_id, widget, (jlong)(intptr_t)ih, (jboolean)(enable ? JNI_TRUE : JNI_FALSE));
  iupAndroid_CheckException(jni_env, method);
  (*jni_env)->DeleteLocalRef(jni_env, java_class);
  return 1;
}

static int androidSetDragSourceAttrib(Ihandle* ih, const char* value)
{
  return androidDragDropCallSet(ih, "setDragSource", iupStrBoolean(value));
}

static int androidSetDropTargetAttrib(Ihandle* ih, const char* value)
{
  return androidDragDropCallSet(ih, "setDropTarget", iupStrBoolean(value));
}

static int androidSetDropFilesTargetAttrib(Ihandle* ih, const char* value)
{
  int enable = iupStrBoolean(value);
  iupAttribSetStr(ih, "DROPFILESTARGET", enable ? "YES" : "NO");
  return androidDragDropCallSet(ih, "setDropFilesTarget", enable);
}


JNIEXPORT jobject JNICALL Java_io_github_gen2brain_iupgo_IupDragDropHelper_requestDragPayload(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint x, jint y)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return NULL;

  IFnii cb_begin = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
  if (cb_begin && cb_begin(ih, (int)x, (int)y) == IUP_IGNORE)
    return NULL;

  const char* types_attr = iupAttribGet(ih, "DRAGTYPES");
  if (!types_attr || !*types_attr) types_attr = "TEXT";

  jobjectArray j_types = androidDragDropParseTypes(jni_env, types_attr);
  jboolean want_move = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE") ? JNI_TRUE : JNI_FALSE;

  jclass payload_class = (*jni_env)->FindClass(jni_env, "io/github/gen2brain/iupgo/IupDragDropHelper$Payload");
  jmethodID ctor = (*jni_env)->GetMethodID(jni_env, payload_class, "<init>", "(J[Ljava/lang/String;Z)V");
  jobject payload = (*jni_env)->NewObject(jni_env, payload_class, ctor, (jlong)(intptr_t)ih, j_types, want_move);

  (*jni_env)->DeleteLocalRef(jni_env, payload_class);
  (*jni_env)->DeleteLocalRef(jni_env, j_types);
  return payload;
}

JNIEXPORT jbyteArray JNICALL Java_io_github_gen2brain_iupgo_IupDragDropHelper_requestDragData(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_type)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih) || !j_type) return NULL;

  IFns cb_size = (IFns)IupGetCallback(ih, "DRAGDATASIZE_CB");
  IFnsVi cb_data = (IFnsVi)IupGetCallback(ih, "DRAGDATA_CB");
  if (!cb_size || !cb_data) return NULL;

  const char* type = (*jni_env)->GetStringUTFChars(jni_env, j_type, NULL);
  int size = cb_size(ih, (char*)type);
  if (size <= 0)
  {
    (*jni_env)->ReleaseStringUTFChars(jni_env, j_type, type);
    return NULL;
  }

  void* buf = malloc((size_t)size);
  if (!buf)
  {
    (*jni_env)->ReleaseStringUTFChars(jni_env, j_type, type);
    return NULL;
  }
  cb_data(ih, (char*)type, buf, size);
  (*jni_env)->ReleaseStringUTFChars(jni_env, j_type, type);

  jbyteArray j_data = (*jni_env)->NewByteArray(jni_env, (jsize)size);
  (*jni_env)->SetByteArrayRegion(jni_env, j_data, 0, (jsize)size, (const jbyte*)buf);
  free(buf);
  return j_data;
}

JNIEXPORT jobject JNICALL Java_io_github_gen2brain_iupgo_IupDragDropHelper_requestDragCursor(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return NULL;
  const char* name = iupAttribGet(ih, "DRAGCURSOR");
  if (!name || !*name) return NULL;
  return (jobject)iupImageGetImage(name, ih, 0, NULL);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupDragDropHelper_dispatchDragEnd(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint action)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;
  IFni cb = (IFni)IupGetCallback(ih, "DRAGEND_CB");
  if (!cb) return;
  if (cb(ih, (int)action) == IUP_CLOSE) IupExitLoop();
}

/* True if `type` is in this widget's DROPTYPES. */
JNIEXPORT jboolean JNICALL Java_io_github_gen2brain_iupgo_IupDragDropHelper_acceptsType(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_type)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih) || !j_type) return JNI_FALSE;
  const char* drop_types = iupAttribGet(ih, "DROPTYPES");
  if (!drop_types || !*drop_types) return JNI_FALSE;
  const char* type = (*jni_env)->GetStringUTFChars(jni_env, j_type, NULL);
  int ok = androidDragDropTypeMatches(drop_types, type);
  (*jni_env)->ReleaseStringUTFChars(jni_env, j_type, type);
  return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupDragDropHelper_deliverDrop(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_type, jbyteArray j_data, jint x, jint y)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih) || !j_data) return;

  IFnsViii cb = (IFnsViii)IupGetCallback(ih, "DROPDATA_CB");
  if (!cb) return;

  const char* type = j_type ? (*jni_env)->GetStringUTFChars(jni_env, j_type, NULL) : "TEXT";
  jsize len = (*jni_env)->GetArrayLength(jni_env, j_data);
  jbyte* bytes = (*jni_env)->GetByteArrayElements(jni_env, j_data, NULL);

  cb(ih, (char*)type, (void*)bytes, (int)len, (int)x, (int)y);

  (*jni_env)->ReleaseByteArrayElements(jni_env, j_data, bytes, JNI_ABORT);
  if (j_type) (*jni_env)->ReleaseStringUTFChars(jni_env, j_type, type);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupDragDropHelper_deliverMotion(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint x, jint y)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;

  IFniis cb = (IFniis)IupGetCallback(ih, "DROPMOTION_CB");
  if (!cb) return;

  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  iupKEY_SETBUTTON1(status);
  cb(ih, (int)x, (int)y, status);
}

JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupDragDropHelper_dispatchDropFile(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_path, jint remaining, jint x, jint y)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih) || !j_path) return IUP_DEFAULT;

  Ihandle* cb_ih = ih;
  IFnsiii cb = (IFnsiii)IupGetCallback(ih, "DROPFILES_CB");
  if (!cb)
  {
    Ihandle* dlg = IupGetDialog(ih);
    if (dlg)
    {
      cb = (IFnsiii)IupGetCallback(dlg, "DROPFILES_CB");
      if (cb) cb_ih = dlg;
    }
  }
  if (!cb) return IUP_DEFAULT;

  const char* path = (*jni_env)->GetStringUTFChars(jni_env, j_path, NULL);
  int ret = cb(cb_ih, (char*)path, (int)remaining, (int)x, (int)y);
  (*jni_env)->ReleaseStringUTFChars(jni_env, j_path, path);
  return ret;
}


void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
  iupClassRegisterCallback(ic, "DRAGBEGIN_CB",    "ii");
  iupClassRegisterCallback(ic, "DRAGDATASIZE_CB", "s");
  iupClassRegisterCallback(ic, "DRAGDATA_CB",     "sVi");
  iupClassRegisterCallback(ic, "DRAGEND_CB",      "i");
  iupClassRegisterCallback(ic, "DROPDATA_CB",     "sViii");
  iupClassRegisterCallback(ic, "DROPMOTION_CB",   "iis");
  iupClassRegisterCallback(ic, "DROPFILES_CB",    "siii");

  iupClassRegisterAttribute(ic, "DRAGSOURCE",     NULL, androidSetDragSourceAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGET",     NULL, androidSetDropTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGTYPES",      NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTYPES",      NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGCURSOR",     NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSORCOPY", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);

  /* DROPFILESTARGET / DRAGDROP (alias) accept cross-app file URIs. */
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", NULL, androidSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGDROP",        NULL, androidSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
