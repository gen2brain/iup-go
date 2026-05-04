/** \file
 * \brief JNI bridge: IupScrollbarHelper -> IUP callbacks.
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>

#include "iup.h"

#include "iup_object.h"

#include "iupandroid_drv.h"


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupScrollbarHelper_dispatchScroll(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint op, jdouble value)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (ih) iupAndroid_ScrollbarDispatch(ih, (int)op, (double)value);
}
