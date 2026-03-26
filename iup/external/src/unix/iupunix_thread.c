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

void* iupdrvThreadStart(Ihandle* ih)
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

void iupdrvThreadJoin(void* handle)
{
  pthread_join(*(pthread_t*)handle, NULL);
}

void iupdrvThreadYield(void)
{
  sched_yield();
}

int iupdrvThreadIsCurrent(void* handle)
{
  return pthread_equal(*(pthread_t*)handle, pthread_self());
}

void iupdrvThreadExit(int code)
{
  pthread_exit((void*)(intptr_t)code);
}

void iupdrvThreadDestroy(void* handle)
{
  free(handle);
}

void* iupdrvMutexCreate(void)
{
  pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  if (mutex)
    pthread_mutex_init(mutex, NULL);
  return (void*)mutex;
}

void iupdrvMutexLock(void* handle)
{
  pthread_mutex_lock((pthread_mutex_t*)handle);
}

void iupdrvMutexUnlock(void* handle)
{
  pthread_mutex_unlock((pthread_mutex_t*)handle);
}

void iupdrvMutexDestroy(void* handle)
{
  if (!handle)
    return;

  pthread_mutex_destroy((pthread_mutex_t*)handle);
  free(handle);
}
