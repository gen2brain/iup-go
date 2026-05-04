/** \file
 * \brief JNI bridge: IupIdleHelper (main-looper IdleHandler -> user IDLE_ACTION).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>

#include "iup.h"

#include "iupandroid_drv.h"


JNIEXPORT jint JNICALL Java_io_github_gen2brain_iupgo_IupIdleHelper_dispatchIdle(JNIEnv* jni_env, jclass cls)
{
  (void)jni_env; (void)cls;
  return (jint)iupAndroid_DispatchIdle();
}
