/** \file
 * \brief JNI bridge: IupCalendarHelper (CalendarView -> IUP callbacks).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>

#include "iup.h"

#include "iup_object.h"


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupCalendarHelper_dispatchValueChanged(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih) return;
  iupBaseCallValueChangedCb(ih);
}
