/** \file
 * \brief JNI bridge: IupValHelper (Slider -> IUP callbacks).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_val.h"


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupValHelper_dispatchValue(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_key, jfloat value)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !j_key) return;

  const char* key = (*jni_env)->GetStringUTFChars(jni_env, j_key, NULL);
  /* sync ih->data->val first so GetAttribute("VALUE") inside VALUECHANGED_CB returns the new value */
  if (ih->data && (strcmp(key, "MOUSEMOVE_CB") == 0 || strcmp(key, "BUTTON_RELEASE_CB") == 0))
    ih->data->val = (double)value;
  IFnd cb = (IFnd)IupGetCallback(ih, key);
  (*jni_env)->ReleaseStringUTFChars(jni_env, j_key, key);

  if (cb) cb(ih, (double)value);
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupValHelper_dispatchVoid(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jstring j_key)
{
  (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !j_key) return;

  const char* key = (*jni_env)->GetStringUTFChars(jni_env, j_key, NULL);
  Icallback cb = IupGetCallback(ih, key);
  (*jni_env)->ReleaseStringUTFChars(jni_env, j_key, key);

  if (cb) cb(ih);
}
