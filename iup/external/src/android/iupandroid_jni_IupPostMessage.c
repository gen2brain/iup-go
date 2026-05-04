/** \file
 * \brief JNI bridge: IupPostMessage (Java UI thread -> user POSTMESSAGE_CB).
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdint.h>
#include <jni.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupPostMessage_OnMainThreadCallback(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jlong message_data, jstring j_usr_str, jlong j_usr_int, jdouble j_usr_double)
{
  (void)cls;

  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;
  IFnsidv user_post_message_callback = (IFnsidv)IupGetCallback(ih, "POSTMESSAGE_CB");
  if (!user_post_message_callback)
    return;

  /* Producer side passes NULL when the payload is empty; surface "" to the callback. */
  const char* c_usr_str = j_usr_str ? (*jni_env)->GetStringUTFChars(jni_env, j_usr_str, NULL) : "";
  user_post_message_callback(ih, (char*)c_usr_str, (int)j_usr_int, j_usr_double, (void*)message_data);
  if (j_usr_str) (*jni_env)->ReleaseStringUTFChars(jni_env, j_usr_str, c_usr_str);
}
