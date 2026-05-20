/** \file
 * \brief JNI bridge: IupTextHelper (ACTION, VALUECHANGED_CB, K_ANY).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_key.h"
#include "iup_mask.h"

#include "iupandroid_drv.h"

#include <string.h>


/* AKEYCODE -> IUP key. Printable chars route through ACTION, not K_ANY. */
static int androidTextAndroidKeyToIup(jint keyCode)
{
  switch (keyCode)
  {
    /* AKEYCODE_BACK (4) unmapped: belongs to Activity, not text field. */
    case 61:  return K_TAB;         /* AKEYCODE_TAB */
    case 62:  return K_SP;          /* AKEYCODE_SPACE */
    case 66:  return K_CR;          /* AKEYCODE_ENTER */
    case 67:  return K_BS;          /* AKEYCODE_DEL (backspace) */
    case 111: return K_ESC;         /* AKEYCODE_ESCAPE */
    case 112: return K_DEL;         /* AKEYCODE_FORWARD_DEL */
    case 122: return K_HOME;        /* AKEYCODE_MOVE_HOME */
    case 123: return K_END;         /* AKEYCODE_MOVE_END */
    case 124: return K_INS;         /* AKEYCODE_INSERT */
    case 92:  return K_PGUP;        /* AKEYCODE_PAGE_UP */
    case 93:  return K_PGDN;        /* AKEYCODE_PAGE_DOWN */
    case 19:  return K_UP;          /* AKEYCODE_DPAD_UP */
    case 20:  return K_DOWN;        /* AKEYCODE_DPAD_DOWN */
    case 21:  return K_LEFT;        /* AKEYCODE_DPAD_LEFT */
    case 22:  return K_RIGHT;       /* AKEYCODE_DPAD_RIGHT */
    default:  return 0;
  }
}

JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupTextHelper_dispatchAction(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint key, jstring new_value)
{
  (void)cls;

  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return -1;

  /* same pipeline drives Text and List EDITBOX; switch callback + mask owner by class */
  int is_list = ih->iclass && ih->iclass->name && strcmp(ih->iclass->name, "list") == 0;
  const char* cb_name = is_list ? "EDIT_CB" : "ACTION";
  IFnis cb = (IFnis)IupGetCallback(ih, cb_name);
  Imask* mask = (Imask*)(is_list ? iupAndroidListGetMask(ih) : iupAndroidTextGetMask(ih));

  if (!cb && !mask) return -1;

  const char* str = new_value ? (*jni_env)->GetStringUTFChars(jni_env, new_value, NULL) : NULL;
  const char* effective = str ? str : "";

  if (mask && iupMaskCheck(mask, effective) == 0)
  {
    IFns fail_cb = (IFns)IupGetCallback(ih, "MASKFAIL_CB");
    if (fail_cb) fail_cb(ih, (char*)effective);
    if (str) (*jni_env)->ReleaseStringUTFChars(jni_env, new_value, str);
    return 0;  /* abort: filter rejects the change */
  }

  int ret = cb ? cb(ih, (int)key, (char*)effective) : IUP_DEFAULT;
  if (str) (*jni_env)->ReleaseStringUTFChars(jni_env, new_value, str);

  if (ret == IUP_IGNORE) return 0;
  if (ret == IUP_CLOSE)
  {
    IupExitLoop();
    return 0;
  }
  if (ret != 0 && key != 0 && ret != IUP_DEFAULT && ret != IUP_CONTINUE)
    return ret;
  return -1;
}


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupTextHelper_dispatchValueChanged(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env;
  (void)cls;

  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;

  Icallback cb = IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb) cb(ih);
}


JNIEXPORT jboolean JNICALL Java_io_github_gen2brain_iupgo_IupTextHelper_dispatchKAny(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint android_key_code, jint meta_state)
{
  (void)jni_env;
  (void)cls;

  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return JNI_FALSE;

  int iup_key = androidTextAndroidKeyToIup(android_key_code);
  if (iup_key == 0) return JNI_FALSE;

  /* Android meta_state bit 1 == META_SHIFT_ON. */
  int shift = (meta_state & 0x01) ? 1 : 0;

  int kany_consumed = 0;
  /* Walk up the parent chain so a K_ANY on the dialog sees the event. */
  int ret = iupKeyCallKeyCb(ih, iup_key);
  if (ret == IUP_CLOSE) IupExitLoop();
  if (ret == IUP_IGNORE) kany_consumed = 1;

  if (!kany_consumed && iupKeyProcessNavigation(ih, iup_key, shift))
    return JNI_TRUE;

  return kany_consumed ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupTextHelper_dispatchSpin(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint current, jint direction)
{
  (void)jni_env;
  (void)cls;

  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return 0x80000000;

  int inc = iupAttribGet(ih, "SPININC") ? iupAttribGetInt(ih, "SPININC") : 1;
  if (inc <= 0) inc = 1;
  int min = iupAttribGet(ih, "SPINMIN") ? iupAttribGetInt(ih, "SPINMIN") : 0;
  int max = iupAttribGet(ih, "SPINMAX") ? iupAttribGetInt(ih, "SPINMAX") : 100;
  int wrap = iupAttribGetBoolean(ih, "SPINWRAP");

  int next = current + direction * inc;
  if (next < min) next = wrap ? max : min;
  if (next > max) next = wrap ? min : max;

  IFni cb = (IFni)IupGetCallback(ih, "SPIN_CB");
  if (cb)
  {
    int ret = cb(ih, next);
    if (ret == IUP_IGNORE) return 0x80000000;
    if (ret == IUP_CLOSE) { IupExitLoop(); return 0x80000000; }
  }

  /* SPINAUTO=NO: SPIN_CB owns the text; suppress auto-write. */
  if (!iupAttribGetBoolean(ih, "SPINAUTO")) return 0x80000000;

  iupAttribSetInt(ih, "_IUPANDROID_SPIN_VALUE", next);
  return next;
}


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupTextHelper_dispatchLinkClick(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring url)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih) || !url) return;

  const char* utf = (*jni_env)->GetStringUTFChars(jni_env, url, NULL);
  IFns cb = (IFns)IupGetCallback(ih, "LINK_CB");
  if (cb)
  {
    int ret = cb(ih, (char*)utf);
    if (ret == IUP_CLOSE) IupExitLoop();
  }
  (*jni_env)->ReleaseStringUTFChars(jni_env, url, utf);
}
