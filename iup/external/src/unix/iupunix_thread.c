/** \file
 * \brief Unix/POSIX Driver - Thread Support
 *
 * Used by Motif, Cocoa, EFL, and Qt (on macOS) drivers.
 *
 * See Copyright Notice in "iup.h"
 */

#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_thread.h"

static void* iupunix_ThreadFunc(void* obj)
{
  Ihandle* ih = (Ihandle*)obj;
  Icallback cb = IupGetCallback(ih, "THREAD_CB");
  if (cb)
    cb(ih);
  return NULL;
}

IUP_SDK_API void* iupdrvThreadStart(Ihandle* ih)
{
  pthread_t* thread = (pthread_t*)malloc(sizeof(pthread_t));
  if (thread)
  {
    if (pthread_create(thread, NULL, iupunix_ThreadFunc, ih) == 0)
    {
      pthread_detach(*thread);
    }
    else
    {
      free(thread);
      return NULL;
    }
  }
  return (void*)thread;
}

IUP_SDK_API void iupdrvThreadJoin(void* handle)
{
  pthread_join(*(pthread_t*)handle, NULL);
}

IUP_SDK_API void iupdrvThreadYield(void)
{
  sched_yield();
}

IUP_SDK_API int iupdrvThreadIsCurrent(void* handle)
{
  return pthread_equal(*(pthread_t*)handle, pthread_self());
}

IUP_SDK_API void iupdrvThreadExit(int code)
{
  pthread_exit((void*)(intptr_t)code);
}

IUP_SDK_API void iupdrvThreadDestroy(void* handle)
{
  free(handle);
}

IUP_SDK_API void* iupdrvMutexCreate(void)
{
  pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  if (mutex)
    pthread_mutex_init(mutex, NULL);
  return (void*)mutex;
}

IUP_SDK_API void iupdrvMutexLock(void* handle)
{
  pthread_mutex_lock((pthread_mutex_t*)handle);
}

IUP_SDK_API void iupdrvMutexUnlock(void* handle)
{
  pthread_mutex_unlock((pthread_mutex_t*)handle);
}

IUP_SDK_API void iupdrvMutexDestroy(void* handle)
{
  if (!handle)
    return;

  pthread_mutex_destroy((pthread_mutex_t*)handle);
  free(handle);
}
