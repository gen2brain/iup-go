/** \file
 * \brief JNI bridge: IupNetworkHelper (NETWORK global attribute).
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdatomic.h>
#include <jni.h>

#include "iup.h"

#include "iupandroid_drv.h"

static _Atomic long long iupandroid_network_handle;

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupNetworkHelper_nativeSetHandle(JNIEnv* jni_env, jclass cls, jlong handle)
{
  (void)jni_env; (void)cls;
  atomic_store(&iupandroid_network_handle, (long long)handle);
}

IUP_SDK_API long long iupAndroid_GetNetworkHandle(void)
{
  return atomic_load(&iupandroid_network_handle);
}
