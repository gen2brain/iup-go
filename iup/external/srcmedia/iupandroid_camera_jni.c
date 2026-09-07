/** \file
 * \brief Camera control, JNI entry points called from IupCameraHelper
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>
#include <stdint.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_camera.h"

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupCameraHelper_dispatchPermission(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jboolean granted)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  (void)jni_env; (void)cls;
  if (!ih || !iupObjectCheck(ih))
    return;
  iupandroidCameraPermission(ih, granted ? 1 : 0);
}
