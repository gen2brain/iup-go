/** \file
 * \brief WebAssembly Thread and Mutex
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdint.h>
#include <stdlib.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_thread.h"


#ifdef __EMSCRIPTEN_PTHREADS__

#include <pthread.h>
#include <sched.h>

static void* iupwasm_ThreadFunc(void* obj)
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
    if (pthread_create(thread, NULL, iupwasm_ThreadFunc, ih) == 0)
      pthread_detach(*thread);
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

#else

IUP_SDK_API void* iupdrvThreadStart(Ihandle* ih) { (void)ih; return NULL; }
IUP_SDK_API void iupdrvThreadJoin(void* handle) { (void)handle; }
IUP_SDK_API void iupdrvThreadYield(void) { }
IUP_SDK_API int iupdrvThreadIsCurrent(void* handle) { (void)handle; return 0; }
IUP_SDK_API void iupdrvThreadExit(int code) { (void)code; }
IUP_SDK_API void iupdrvThreadDestroy(void* handle) { (void)handle; }
IUP_SDK_API void* iupdrvMutexCreate(void) { return NULL; }
IUP_SDK_API void iupdrvMutexLock(void* handle) { (void)handle; }
IUP_SDK_API void iupdrvMutexUnlock(void* handle) { (void)handle; }
IUP_SDK_API void iupdrvMutexDestroy(void* handle) { (void)handle; }

#endif
