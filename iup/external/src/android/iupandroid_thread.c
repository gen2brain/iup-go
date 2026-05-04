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

void* iupdrvThreadStart(Ihandle* ih)
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

void iupdrvThreadJoin(void* handle)
{
  if (!handle)
    return;
  pthread_join(*(pthread_t*)handle, NULL);
}

void iupdrvThreadYield(void)
{
  sched_yield();
}

int iupdrvThreadIsCurrent(void* handle)
{
  if (!handle)
    return 0;
  return pthread_equal(*(pthread_t*)handle, pthread_self()) != 0;
}

void iupdrvThreadExit(int code)
{
  pthread_exit((void*)(intptr_t)code);
}

void iupdrvThreadDestroy(void* handle)
{
  if (handle)
    free(handle);
}

void* iupdrvMutexCreate(void)
{
  pthread_mutex_t* mutex = malloc(sizeof(pthread_mutex_t));
  if (mutex)
    pthread_mutex_init(mutex, NULL);
  return mutex;
}

void iupdrvMutexLock(void* handle)
{
  if (handle)
    pthread_mutex_lock(handle);
}

void iupdrvMutexUnlock(void* handle)
{
  if (handle)
    pthread_mutex_unlock(handle);
}

void iupdrvMutexDestroy(void* handle)
{
  if (!handle)
    return;
  pthread_mutex_destroy(handle);
  free(handle);
}
