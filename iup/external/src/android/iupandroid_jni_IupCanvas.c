/** \file
 * \brief JNI bridge: IupCanvasHelper (Java -> C touch/motion dispatch).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>
#include <stdint.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_key.h"
#include "iup_drvinfo.h"

#include "iupandroid_drv.h"


/* Subset of android.view.KeyEvent META_* used for modifier mapping. */
#define AMETA_SHIFT_ON  0x0001
#define AMETA_ALT_ON    0x0002
#define AMETA_CTRL_ON   0x1000
#define AMETA_META_ON   0x10000

static void androidCanvasFillStatus(char* status, int button, int pressed, int meta_state)
{
  memcpy(status, IUPKEY_STATUS_INIT, IUPKEY_STATUS_SIZE);

  if (meta_state & AMETA_SHIFT_ON) iupKEY_SETSHIFT(status);
  if (meta_state & AMETA_CTRL_ON)  iupKEY_SETCONTROL(status);
  if (meta_state & AMETA_ALT_ON)   iupKEY_SETALT(status);
  if (meta_state & AMETA_META_ON)  iupKEY_SETSYS(status);

  if (pressed)
  {
    if (button == 1) iupKEY_SETBUTTON1(status);
    else if (button == 2) iupKEY_SETBUTTON2(status);
    else if (button == 3) iupKEY_SETBUTTON3(status);
    else if (button == 4) iupKEY_SETBUTTON4(status);
    else if (button == 5) iupKEY_SETBUTTON5(status);
  }
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupCanvasHelper_dispatchButton(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint button, jint pressed, jint x, jint y, jint meta_state)
{
  (void)jni_env;
  (void)cls;

  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih) return;

  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (!cb) return;

  char status[IUPKEY_STATUS_SIZE];
  androidCanvasFillStatus(status, button, pressed, meta_state);

  /* Touch in HW px, canvas/draw in logical px (float density for accuracy). */
  float d = iupAndroid_GetDisplayDensity(); if (d < 1.0f) d = 1.0f;

  /* IUP button codes are the ASCII digits '1'..'5', not integer indices. */
  int iup_button = '0' + button;
  cb(ih, iup_button, pressed, (int)((float)x / d), (int)((float)y / d), status);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupCanvasHelper_dispatchMotion(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint x, jint y, jint meta_state, jint button_state)
{
  (void)jni_env;
  (void)cls;

  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih) return;

  IFniis cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
  if (!cb) return;

  char status[IUPKEY_STATUS_SIZE];
  androidCanvasFillStatus(status, 0, 0, meta_state);

  if (button_state & 1) iupKEY_SETBUTTON1(status);
  if (button_state & 2) iupKEY_SETBUTTON2(status);
  if (button_state & 4) iupKEY_SETBUTTON3(status);
  if (button_state & 8) iupKEY_SETBUTTON4(status);
  if (button_state & 16) iupKEY_SETBUTTON5(status);

  float d = iupAndroid_GetDisplayDensity(); if (d < 1.0f) d = 1.0f;
  cb(ih, (int)((float)x / d), (int)((float)y / d), status);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupCanvasHelper_dispatchAction(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint x1, jint y1, jint x2, jint y2)
{
  (void)jni_env;
  (void)cls;

  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih) return;

  IFn cb = (IFn)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    iupAttribSetStrf(ih, "CLIPRECT", "%d %d %d %d", (int)x1, (int)y1, (int)x2, (int)y2);
    cb(ih);
    iupAttribSet(ih, "CLIPRECT", NULL);
  }
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupCanvasHelper_dispatchLeaveWindow(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih) return;
  IFn cb = (IFn)IupGetCallback(ih, "LEAVEWINDOW_CB");
  if (cb) cb(ih);
}

/* canvas drives its own drag when BUTTON_CB+MOTION_CB are set; Java pauses ancestor scroll then */
JNIEXPORT jboolean JNICALL Java_io_github_gen2brain_iupgo_IupCanvasHelper_isDragInteractive(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih) return JNI_FALSE;
  return (IupGetCallback(ih, "BUTTON_CB") && IupGetCallback(ih, "MOTION_CB")) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_io_github_gen2brain_iupgo_IupCanvasHelper_isTouchEnabled(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih) return JNI_FALSE;
  return (IupGetCallback(ih, "TOUCH_CB") || IupGetCallback(ih, "MULTITOUCH_CB")) ? JNI_TRUE : JNI_FALSE;
}

/* fires TOUCH_CB per touch point + one MULTITOUCH_CB for the batch; states are 'D'/'M'/'U' */
JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupCanvasHelper_dispatchTouch(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint count, jintArray jids, jintArray jxs, jintArray jys, jintArray jstates, jint primary_id)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || count <= 0) return;

  IFniiis single_cb = (IFniiis)IupGetCallback(ih, "TOUCH_CB");
  IFniIIII multi_cb = (IFniIIII)IupGetCallback(ih, "MULTITOUCH_CB");
  if (!single_cb && !multi_cb) return;

  jint* ids = (*jni_env)->GetIntArrayElements(jni_env, jids, NULL);
  jint* xs = (*jni_env)->GetIntArrayElements(jni_env, jxs, NULL);
  jint* ys = (*jni_env)->GetIntArrayElements(jni_env, jys, NULL);
  jint* states = (*jni_env)->GetIntArrayElements(jni_env, jstates, NULL);

  if (single_cb)
  {
    int i;
    for (i = 0; i < count; i++)
    {
      char st = (char)states[i];
      const char* str;
      if (st == 'D')      str = (ids[i] == primary_id) ? "DOWN-PRIMARY" : "DOWN";
      else if (st == 'U') str = (ids[i] == primary_id) ? "UP-PRIMARY" : "UP";
      else                str = (ids[i] == primary_id) ? "MOVE-PRIMARY" : "MOVE";

      if (single_cb(ih, ids[i], xs[i], ys[i], (char*)str) == IUP_CLOSE)
      {
        IupExitLoop();
        break;
      }
    }
  }

  if (multi_cb)
  {
    if (multi_cb(ih, count, (int*)ids, (int*)xs, (int*)ys, (int*)states) == IUP_CLOSE)
      IupExitLoop();
  }

  (*jni_env)->ReleaseIntArrayElements(jni_env, jids, ids, JNI_ABORT);
  (*jni_env)->ReleaseIntArrayElements(jni_env, jxs, xs, JNI_ABORT);
  (*jni_env)->ReleaseIntArrayElements(jni_env, jys, ys, JNI_ABORT);
  (*jni_env)->ReleaseIntArrayElements(jni_env, jstates, states, JNI_ABORT);
}
