/** \file
 * \brief Android Driver - Thread Support
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sched.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_thread.h"

#include "iupandroid_drv.h"


static void* androidThreadFunc(void* obj)
{
  Ihandle* ih = obj;
  Icallback cb = IupGetCallback(ih, "THREAD_CB");

  /* Attach the worker up front so THREAD_CB can call JNI. */
  iupAndroid_GetEnvThreadSafe();

  if (cb)
    cb(ih);

  return NULL;
}

IUP_SDK_API void* iupdrvThreadStart(Ihandle* ih)
{
  pthread_t* thread = malloc(sizeof(pthread_t));
  if (!thread)
    return NULL;

  if (pthread_create(thread, NULL, androidThreadFunc, ih) != 0)
  {
    free(thread);
    return NULL;
  }
  return thread;
}

IUP_SDK_API void iupdrvThreadJoin(void* handle)
{
  if (!handle)
    return;
  pthread_join(*(pthread_t*)handle, NULL);
}

IUP_SDK_API void iupdrvThreadYield(void)
{
  sched_yield();
}

IUP_SDK_API int iupdrvThreadIsCurrent(void* handle)
{
  if (!handle)
    return 0;
  return pthread_equal(*(pthread_t*)handle, pthread_self()) != 0;
}

IUP_SDK_API void iupdrvThreadExit(int code)
{
  pthread_exit((void*)(intptr_t)code);
}

IUP_SDK_API void iupdrvThreadDestroy(void* handle)
{
  if (handle)
    free(handle);
}

IUP_SDK_API void* iupdrvMutexCreate(void)
{
  pthread_mutex_t* mutex = malloc(sizeof(pthread_mutex_t));
  if (mutex)
    pthread_mutex_init(mutex, NULL);
  return mutex;
}

IUP_SDK_API void iupdrvMutexLock(void* handle)
{
  if (handle)
    pthread_mutex_lock(handle);
}

IUP_SDK_API void iupdrvMutexUnlock(void* handle)
{
  if (handle)
    pthread_mutex_unlock(handle);
}

IUP_SDK_API void iupdrvMutexDestroy(void* handle)
{
  if (!handle)
    return;
  pthread_mutex_destroy(handle);
  free(handle);
}
