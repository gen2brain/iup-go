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
  auto* ih = static_cast<Ihandle*>(obj);
  Icallback cb = IupGetCallback(ih, "THREAD_CB");
  if (cb) cb(ih);
  return nullptr;
}

extern "C" IUP_SDK_API void* iupdrvThreadStart(Ihandle* ih)
{
  auto* t = static_cast<pthread_t*>(malloc(sizeof(pthread_t)));
  if (!t) return nullptr;
  if (pthread_create(t, nullptr, haikuThreadFunc, ih) != 0)
  {
    free(t);
    return nullptr;
  }
  return t;
}

extern "C" IUP_SDK_API void iupdrvThreadJoin(void* handle)
{
  if (handle) pthread_join(*static_cast<pthread_t*>(handle), nullptr);
}

extern "C" IUP_SDK_API void iupdrvThreadYield(void)
{
  sched_yield();
}

extern "C" IUP_SDK_API int iupdrvThreadIsCurrent(void* handle)
{
  if (!handle) return 0;
  return pthread_equal(*static_cast<pthread_t*>(handle), pthread_self());
}

extern "C" IUP_SDK_API void iupdrvThreadExit(int code)
{
  pthread_exit(reinterpret_cast<void*>(static_cast<intptr_t>(code)));
}

extern "C" IUP_SDK_API void iupdrvThreadDestroy(void* handle)
{
  free(handle);
}

extern "C" IUP_SDK_API void* iupdrvMutexCreate(void)
{
  auto* m = static_cast<pthread_mutex_t*>(malloc(sizeof(pthread_mutex_t)));
  if (m) pthread_mutex_init(m, nullptr);
  return m;
}

extern "C" IUP_SDK_API void iupdrvMutexLock(void* handle)
{
  if (handle) pthread_mutex_lock(static_cast<pthread_mutex_t*>(handle));
}

extern "C" IUP_SDK_API void iupdrvMutexUnlock(void* handle)
{
  if (handle) pthread_mutex_unlock(static_cast<pthread_mutex_t*>(handle));
}

extern "C" IUP_SDK_API void iupdrvMutexDestroy(void* handle)
{
  if (!handle) return;
  pthread_mutex_destroy(static_cast<pthread_mutex_t*>(handle));
  free(handle);
}
