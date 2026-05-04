/** \file
 * \brief JNI bridge: IupNotifyHelper -> IUP callbacks.
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"

#include "iupandroid_drv.h"


JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupNotifyHelper_dispatchAction(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint action_index)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;
  IFni cb = (IFni)IupGetCallback(ih, "NOTIFY_CB");
  if (cb)
  {
    int ret = cb(ih, (int)action_index);
    if (ret == IUP_CLOSE) IupExitLoop();
  }
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupNotifyHelper_dispatchClose(JNIEnv* jni_env, jclass cls, jlong ihandle_ptr, jint reason)
{
  (void)jni_env; (void)cls;
  Ihandle* ih = (Ihandle*)ihandle_ptr;
  if (!ih || !iupObjectCheck(ih)) return;
  IFni cb = (IFni)IupGetCallback(ih, "CLOSE_CB");
  if (cb)
  {
    int ret = cb(ih, (int)reason);
    if (ret == IUP_CLOSE) IupExitLoop();
  }
}
