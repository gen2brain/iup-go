/** \file
 * \brief JNI bridge: IupValHelper (Slider -> IUP callbacks).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_val.h"


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupValHelper_dispatchValueChanged(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jfloat value)
{
  (void)cls;
  (void)jni_env;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;

  if (ih->data)
    ih->data->val = (double)value;

  IFn cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb) cb(ih);
}
