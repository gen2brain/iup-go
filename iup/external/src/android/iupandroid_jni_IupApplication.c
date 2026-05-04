/** \file
 * \brief JNI bridge: IupApplication (Java -> C application lifecycle).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>

#include "iup_loop.h"


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupApplication_IupExitCallback(JNIEnv* jni_env, jobject thiz)
{
  (void)jni_env;
  (void)thiz;

  iupLoopCallExitCb();
}
