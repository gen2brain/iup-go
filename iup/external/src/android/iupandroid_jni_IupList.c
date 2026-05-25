/** \file
 * \brief JNI bridge: IupListHelper (List/Spinner selection -> ACTION callback).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_image.h"
#include "iup_key.h"

#include "iupandroid_drv.h"

#ifndef AMETA_SHIFT_ON
#define AMETA_SHIFT_ON  0x0001
#define AMETA_ALT_ON    0x0002
#define AMETA_CTRL_ON   0x1000
#define AMETA_META_ON   0x10000
#endif

static void androidListFillStatus(char* status, int pressed, int meta_state)
{
  memcpy(status, IUPKEY_STATUS_INIT, IUPKEY_STATUS_SIZE);
  if (meta_state & AMETA_SHIFT_ON) iupKEY_SETSHIFT(status);
  if (meta_state & AMETA_CTRL_ON)  iupKEY_SETCONTROL(status);
  if (meta_state & AMETA_ALT_ON)   iupKEY_SETALT(status);
  if (meta_state & AMETA_META_ON)  iupKEY_SETSYS(status);
  if (pressed) iupKEY_SETBUTTON1(status);
}


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupListHelper_dispatchAction(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_text, jint item, jint state)
{
  (void)cls; (void)jni_env; (void)j_text; (void)state;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;
  iupAndroidListDispatchSelection(ih, item);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupListHelper_dispatchListDoubleClick(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint item)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;
  iupAndroidListDispatchDoubleClick(ih, (int)item);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupListHelper_dispatchListTouch(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint is_button, jint pressed, jint x, jint y, jint meta_state)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;

  float d = iupAndroid_GetDisplayDensity(); if (d < 1.0f) d = 1.0f;
  int lx = (int)((float)x / d), ly = (int)((float)y / d);
  char status[IUPKEY_STATUS_SIZE];

  if (is_button)
  {
    IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
    if (!cb) return;
    androidListFillStatus(status, pressed, meta_state);
    if (cb(ih, IUP_BUTTON1, pressed, lx, ly, status) == IUP_CLOSE) IupExitLoop();
  }
  else
  {
    IFniis cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
    if (!cb) return;
    androidListFillStatus(status, 1, meta_state);
    if (cb(ih, lx, ly, status) == IUP_CLOSE) IupExitLoop();
  }
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupListHelper_dispatchListMultiSelect(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jintArray positions)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;
  jint n = positions ? (*jni_env)->GetArrayLength(jni_env, positions) : 0;
  jint* elems = (n > 0) ? (*jni_env)->GetIntArrayElements(jni_env, positions, NULL) : NULL;
  iupAndroidListDispatchMultiSelection(ih, (int*)elems, (int)n);
  if (elems) (*jni_env)->ReleaseIntArrayElements(jni_env, positions, elems, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupListHelper_dispatchListDragDrop(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint drag_id, jint drop_id)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;
  iupAndroidListDispatchDragDrop(ih, (int)drag_id, (int)drop_id);
}

/* VIRTUALMODE adapter asks for a visible row's text. pos is 0-based; IUP VALUE_CB is 1-based. */
JNIEXPORT jstring JNICALL Java_io_github_gen2brain_iupgo_IupListHelper_dispatchListValueCb(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint pos)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return NULL;
  sIFni cb = (sIFni)IupGetCallback(ih, "VALUE_CB");
  if (!cb) return NULL;
  char* value = cb(ih, (int)pos + 1);
  return value ? (*jni_env)->NewStringUTF(jni_env, value) : NULL;
}

/* VIRTUALMODE per-row image: IMAGE_CB returns an image name; resolve to a Bitmap. */
JNIEXPORT jobject JNICALL Java_io_github_gen2brain_iupgo_IupListHelper_dispatchListImageCb(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint pos)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return NULL;
  sIFni cb = (sIFni)IupGetCallback(ih, "IMAGE_CB");
  if (!cb) return NULL;
  char* name = cb(ih, (int)pos + 1);
  if (!name || !*name) return NULL;
  return (jobject)iupImageGetImage(name, ih, 0, NULL);
}
