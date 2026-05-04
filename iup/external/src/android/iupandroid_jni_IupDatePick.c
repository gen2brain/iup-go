/** \file
 * \brief JNI bridge: IupDatePickHelper (MaterialDatePicker -> IUP callbacks).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupDatePickHelper_dispatchValueChanged(JNIEnv* env, jclass cls, jlong ihandle_ptr, jint year, jint month, jint day)
{
  (void)env; (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;
  iupAttribSetInt(ih, "_IUPDATEPICK_YEAR",  (int)year);
  iupAttribSetInt(ih, "_IUPDATEPICK_MONTH", (int)month);
  iupAttribSetInt(ih, "_IUPDATEPICK_DAY",   (int)day);
  iupBaseCallValueChangedCb(ih);
}
