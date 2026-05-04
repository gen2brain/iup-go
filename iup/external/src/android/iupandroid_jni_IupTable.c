/** \file
 * \brief JNI bridge: IupTableHelper (CLICK_CB / ENTERITEM_CB dispatch).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_key.h"

#include "iupandroid_drv.h"


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupTableHelper_dispatchClick(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr,
    jint lin, jint col, jint focus_changed)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;

  if (focus_changed)
  {
    IFnii enter_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
    if (enter_cb)
    {
      int ret = enter_cb(ih, (int)lin, (int)col);
      if (ret == IUP_CLOSE) IupExitLoop();
    }
  }

  IFniis click_cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
  if (click_cb)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupKEY_SETBUTTON1(status);
    int ret = click_cb(ih, (int)lin, (int)col, status);
    if (ret == IUP_CLOSE) IupExitLoop();
  }
}


JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupTableHelper_dispatchEditBegin(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint lin, jint col)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return 1;

  IFnii cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (!cb) return 0;
  int ret = cb(ih, (int)lin, (int)col);
  if (ret == IUP_CLOSE) IupExitLoop();
  return (ret == IUP_IGNORE) ? 1 : 0;
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupTableHelper_dispatchEdition(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint lin, jint col, jstring j_text)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;

  IFniis cb = (IFniis)IupGetCallback(ih, "EDITION_CB");
  if (!cb) return;

  const char* text = j_text ? (*jni_env)->GetStringUTFChars(jni_env, j_text, NULL) : "";
  int ret = cb(ih, (int)lin, (int)col, (char*)text);
  if (j_text) (*jni_env)->ReleaseStringUTFChars(jni_env, j_text, text);
  if (ret == IUP_CLOSE) IupExitLoop();
}

JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupTableHelper_dispatchEditEnd(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint lin, jint col, jstring j_text, jint apply)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return 0;

  IFniisi cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
  if (!cb) return 0;

  const char* text = j_text ? (*jni_env)->GetStringUTFChars(jni_env, j_text, NULL) : "";
  int ret = cb(ih, (int)lin, (int)col, (char*)text, (int)apply);
  if (j_text) (*jni_env)->ReleaseStringUTFChars(jni_env, j_text, text);
  if (ret == IUP_CLOSE) IupExitLoop();
  return (ret == IUP_IGNORE) ? 1 : 0;
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupTableHelper_dispatchValueChanged(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint lin, jint col)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;

  IFnii cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
  if (!cb) return;
  int ret = cb(ih, (int)lin, (int)col);
  if (ret == IUP_CLOSE) IupExitLoop();
}


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupTableHelper_dispatchSort(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint col, jint asc)
{
  (void)jni_env;
  (void)cls;
  (void)asc;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;

  IFni cb = (IFni)IupGetCallback(ih, "SORT_CB");
  if (!cb) return;
  int ret = cb(ih, (int)col);
  if (ret == IUP_CLOSE) IupExitLoop();
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupTableHelper_dispatchReorder(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint from_col, jint to_col)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;

  IFnii cb = (IFnii)IupGetCallback(ih, "REORDER_CB");
  if (!cb) return;
  int ret = cb(ih, (int)from_col, (int)to_col);
  if (ret == IUP_CLOSE) IupExitLoop();
}

JNIEXPORT jstring JNICALL Java_io_github_gen2brain_iupgo_IupTableHelper_dispatchValueRequest(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint lin, jint col)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return NULL;

  sIFnii cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
  if (!cb) return NULL;
  char* v = cb(ih, (int)lin, (int)col);
  return v ? (*jni_env)->NewStringUTF(jni_env, v) : NULL;
}
