/** \file
 * \brief Haiku Thread / Mutex helpers
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <pthread.h>
#include <sched.h>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_thread.h"
}


static void* haikuThreadFunc(void* obj)
{
  Ihandle* ih = (Ihandle*)obj;
  Icallback cb = IupGetCallback(ih, "THREAD_CB");
  if (cb) cb(ih);
  return NULL;
}

extern "C" IUP_SDK_API void* iupdrvThreadStart(Ihandle* ih)
{
  pthread_t* t = (pthread_t*)malloc(sizeof(pthread_t));
  if (!t) return NULL;
  if (pthread_create(t, NULL, haikuThreadFunc, ih) != 0)
  {
    free(t);
    return NULL;
  }
  return t;
}

extern "C" IUP_SDK_API void iupdrvThreadJoin(void* handle)
{
  if (handle) pthread_join(*(pthread_t*)handle, NULL);
}

extern "C" IUP_SDK_API void iupdrvThreadYield(void)
{
  sched_yield();
}

extern "C" IUP_SDK_API int iupdrvThreadIsCurrent(void* handle)
{
  if (!handle) return 0;
  return pthread_equal(*(pthread_t*)handle, pthread_self());
}

extern "C" IUP_SDK_API void iupdrvThreadExit(int code)
{
  pthread_exit((void*)(intptr_t)code);
}

extern "C" IUP_SDK_API void iupdrvThreadDestroy(void* handle)
{
  free(handle);
}

extern "C" IUP_SDK_API void* iupdrvMutexCreate(void)
{
  pthread_mutex_t* m = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  if (m) pthread_mutex_init(m, NULL);
  return m;
}

extern "C" IUP_SDK_API void iupdrvMutexLock(void* handle)
{
  if (handle) pthread_mutex_lock((pthread_mutex_t*)handle);
}

extern "C" IUP_SDK_API void iupdrvMutexUnlock(void* handle)
{
  if (handle) pthread_mutex_unlock((pthread_mutex_t*)handle);
}

extern "C" IUP_SDK_API void iupdrvMutexDestroy(void* handle)
{
  if (!handle) return;
  pthread_mutex_destroy((pthread_mutex_t*)handle);
  free(handle);
}
