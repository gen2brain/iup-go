/** \file
 * \brief EFL Driver - Thread Support
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>

#include <Eina.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sched.h>
#include <pthread.h>
#endif

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_thread.h"

typedef struct _IupEflThread
{
  Eina_Thread thread;
  Eina_Bool started;
} IupEflThread;

static void* iupefl_ThreadFunc(void* data, Eina_Thread t)
{
  Ihandle* ih = (Ihandle*)data;
  Icallback cb = IupGetCallback(ih, "THREAD_CB");
  (void)t;
  if (cb)
    cb(ih);
  return NULL;
}

void* iupdrvThreadStart(Ihandle* ih)
{
  IupEflThread* et = (IupEflThread*)calloc(1, sizeof(IupEflThread));
  if (!et)
    return NULL;

  if (!eina_thread_create(&et->thread, EINA_THREAD_NORMAL, -1, iupefl_ThreadFunc, ih))
  {
    free(et);
    return NULL;
  }

  et->started = EINA_TRUE;

  char* name = iupAttribGet(ih, "THREADNAME");
  if (name)
    eina_thread_name_set(et->thread, name);

  return (void*)et;
}

void iupdrvThreadJoin(void* handle)
{
  IupEflThread* et = (IupEflThread*)handle;
  if (et && et->started)
    eina_thread_join(et->thread);
}

void iupdrvThreadYield(void)
{
#ifdef _WIN32
  SwitchToThread();
#else
  sched_yield();
#endif
}

int iupdrvThreadIsCurrent(void* handle)
{
  IupEflThread* et = (IupEflThread*)handle;
  if (!et || !et->started)
    return 0;
  return eina_thread_equal(et->thread, eina_thread_self());
}

void iupdrvThreadExit(int code)
{
#ifdef _WIN32
  ExitThread((DWORD)code);
#else
  pthread_exit((void*)(intptr_t)code);
#endif
}

void iupdrvThreadDestroy(void* handle)
{
  if (handle)
  {
    IupEflThread* et = (IupEflThread*)handle;
    if (et->started)
      eina_thread_join(et->thread);
    free(et);
  }
}

void* iupdrvMutexCreate(void)
{
  Eina_Lock* lock = (Eina_Lock*)malloc(sizeof(Eina_Lock));
  if (lock)
    eina_lock_new(lock);
  return (void*)lock;
}

void iupdrvMutexLock(void* handle)
{
  eina_lock_take((Eina_Lock*)handle);
}

void iupdrvMutexUnlock(void* handle)
{
  eina_lock_release((Eina_Lock*)handle);
}

void iupdrvMutexDestroy(void* handle)
{
  if (handle)
  {
    eina_lock_free((Eina_Lock*)handle);
    free(handle);
  }
}
