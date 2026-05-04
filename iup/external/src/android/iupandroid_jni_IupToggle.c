/** \file
 * \brief JNI bridge: IupToggleHelper (checked-state -> ACTION callback).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>

#include "iup.h"

#include "iup_object.h"

#include "iupandroid_drv.h"


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupToggleHelper_dispatchAction(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint state)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (ih) iupAndroid_ToggleActionFromJava(ih, state);
}
