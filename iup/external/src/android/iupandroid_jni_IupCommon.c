/** \file
 * \brief JNI bridge: IupCommon (Java -> C generic Ihandle/attrib/callback access).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_drvinfo.h"
#include "iup_key.h"

#include "iupandroid_drv.h"

/* AMETA_* mirrors android.view.KeyEvent meta flags. */
#define AMETA_SHIFT_ON  0x0001
#define AMETA_ALT_ON    0x0002
#define AMETA_CTRL_ON   0x1000
#define AMETA_META_ON   0x10000

static void androidGlobalFillStatus(char* status, int meta_state, int button_state)
{
  memcpy(status, IUPKEY_STATUS_INIT, IUPKEY_STATUS_SIZE);
  if (meta_state & AMETA_SHIFT_ON) iupKEY_SETSHIFT(status);
  if (meta_state & AMETA_CTRL_ON)  iupKEY_SETCONTROL(status);
  if (meta_state & AMETA_ALT_ON)   iupKEY_SETALT(status);
  if (meta_state & AMETA_META_ON)  iupKEY_SETSYS(status);
  if (button_state & 1)  iupKEY_SETBUTTON1(status);
  if (button_state & 2)  iupKEY_SETBUTTON2(status);
  if (button_state & 4)  iupKEY_SETBUTTON3(status);
  if (button_state & 8)  iupKEY_SETBUTTON4(status);
  if (button_state & 16) iupKEY_SETBUTTON5(status);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupCommon_dispatchGlobalButton(JNIEnv* env, jclass cls, jint button, jint pressed, jint x, jint y, jint meta_state)
{
  (void)env; (void)cls;
  IFiiiis cb = (IFiiiis)IupGetFunction("GLOBALBUTTON_CB");
  if (!cb) return;
  char status[IUPKEY_STATUS_SIZE];
  androidGlobalFillStatus(status, meta_state, pressed ? (1 << (button - IUP_BUTTON1)) : 0);
  cb(button, pressed, x, y, status);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupCommon_dispatchGlobalMotion(JNIEnv* env, jclass cls, jint x, jint y, jint meta_state, jint button_state)
{
  (void)env; (void)cls;
  IFiis cb = (IFiis)IupGetFunction("GLOBALMOTION_CB");
  if (!cb) return;
  char status[IUPKEY_STATUS_SIZE];
  androidGlobalFillStatus(status, meta_state, button_state);
  cb(x, y, status);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupCommon_dispatchGlobalKey(JNIEnv* env, jclass cls, jint keycode, jint meta_state, jint pressed)
{
  (void)env; (void)cls;
  IFii cb = (IFii)IupGetFunction("GLOBALKEYPRESS_CB");
  if (!cb) return;
  int code = iupandroidKeyDecode(keycode, meta_state);
  if (code != 0) cb(code, pressed);
}


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupCommon_RetainIhandle(JNIEnv* jni_env, jclass cls, jobject the_widget, jlong ihandle_ptr)
{
  (void)cls;
  iupAndroid_RetainIhandle(jni_env, the_widget, (Ihandle*)(intptr_t)ihandle_ptr);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupCommon_ReleaseIhandle(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)cls;
  iupAndroid_ReleaseIhandle(jni_env, (Ihandle*)(intptr_t)ihandle_ptr);
}

JNIEXPORT jobject JNICALL Java_io_github_gen2brain_iupgo_IupCommon_GetObjectFromIhandle(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return NULL;
  return iupAndroid_RealNativeHandle(ih);
}

JNIEXPORT jstring JNICALL Java_io_github_gen2brain_iupgo_IupCommon_nativeIupAttribGet(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_key_string)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || j_key_string == NULL)
    return NULL;

  const char* key_string = (*jni_env)->GetStringUTFChars(jni_env, j_key_string, NULL);
  char* value_string = iupAttribGet(ih, key_string);
  (*jni_env)->ReleaseStringUTFChars(jni_env, j_key_string, key_string);

  if (value_string != NULL && *value_string != 0)
    return (*jni_env)->NewStringUTF(jni_env, value_string);
  return NULL;
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupCommon_nativeIupAttribSet(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_key_string, jstring j_value_string)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || j_key_string == NULL)
    return;

  const char* key_string = (*jni_env)->GetStringUTFChars(jni_env, j_key_string, NULL);
  const char* value_string = j_value_string ? (*jni_env)->GetStringUTFChars(jni_env, j_value_string, NULL) : NULL;

  iupAttribSet(ih, key_string, value_string);

  if (value_string != NULL)
    (*jni_env)->ReleaseStringUTFChars(jni_env, j_value_string, value_string);
  (*jni_env)->ReleaseStringUTFChars(jni_env, j_key_string, key_string);
}

JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupCommon_nativeIupAttribGetInt(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_key_string)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || j_key_string == NULL)
    return 0;

  const char* key_string = (*jni_env)->GetStringUTFChars(jni_env, j_key_string, NULL);
  int value_int = iupAttribGetInt(ih, key_string);
  (*jni_env)->ReleaseStringUTFChars(jni_env, j_key_string, key_string);

  return (jint)value_int;
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupCommon_nativeIupAttribSetInt(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_key_string, jint j_value_int)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || j_key_string == NULL)
    return;

  const char* key_string = (*jni_env)->GetStringUTFChars(jni_env, j_key_string, NULL);
  iupAttribSetInt(ih, key_string, (int)j_value_int);
  (*jni_env)->ReleaseStringUTFChars(jni_env, j_key_string, key_string);
}

/* -15 signals "no callback registered"; IUP returns -4..0, user codes are >= 0. */
JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupCommon_HandleIupCallback(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_key_string)
{
  (void)cls;
  int ret_val = -15;

  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih) || j_key_string == NULL)
    return ret_val;

  const char* key_string = (*jni_env)->GetStringUTFChars(jni_env, j_key_string, NULL);
  Icallback callback_function = IupGetCallback(ih, key_string);
  (*jni_env)->ReleaseStringUTFChars(jni_env, j_key_string, key_string);

  if (callback_function)
  {
    ret_val = callback_function(ih);
    if (ret_val == IUP_CLOSE)
      IupExitLoop();
  }

  return ret_val;
}

/* int-argument variant for IFni callbacks (SHOW_CB state, FOCUS_CB 0/1, etc.) */
JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupCommon_HandleIupCallbackInt(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_key_string, jint arg)
{
  (void)cls;
  int ret_val = -15;

  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih) || j_key_string == NULL)
    return ret_val;

  const char* key_string = (*jni_env)->GetStringUTFChars(jni_env, j_key_string, NULL);
  IFni callback_function = (IFni)IupGetCallback(ih, key_string);
  (*jni_env)->ReleaseStringUTFChars(jni_env, j_key_string, key_string);

  if (callback_function)
  {
    ret_val = callback_function(ih, (int)arg);
    if (ret_val == IUP_CLOSE)
      IupExitLoop();
  }

  return ret_val;
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupCommon_DoResize(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint x, jint y, jint width, jint height)
{
  (void)jni_env;
  (void)cls;
  (void)x;
  (void)y;

  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;

  IFnii cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (cb)
  {
    /* RESIZE_CB sees logical px to match IupDraw's Canvas scale transform. */
    float d = iupAndroid_GetDisplayDensity(); if (d < 1.0f) d = 1.0f;
    (void)cb(ih, (int)ceilf((float)width / d), (int)ceilf((float)height / d));
  }
  /* SHRINK=YES would otherwise squeeze content into the viewport; let it overflow so NestedScrollView pans. */
  if (ih->naturalheight > height) height = ih->naturalheight;
  ih->currentwidth = width;
  ih->currentheight = height;
  IupRefresh(ih);
}
