/** \file
 * \brief GTK4 Driver - Thread Support
 *
 * See Copyright Notice in "iup.h"
 */

#include <glib.h>
#include <stdint.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_thread.h"

static void* iupgtk4_ThreadFunc(void* obj)
{
  Ihandle* ih = (Ihandle*)obj;
  Icallback cb = IupGetCallback(ih, "THREAD_CB");
  if (cb)
    cb(ih);
  return NULL;
}

void* iupdrvThreadStart(Ihandle* ih)
{
  char* name = iupAttribGet(ih, "THREADNAME");
  if (!name) name = "IupThread";

  return (void*)g_thread_new(name, iupgtk4_ThreadFunc, ih);
}

void iupdrvThreadJoin(void* handle)
{
  g_thread_join((GThread*)handle);
}

void iupdrvThreadYield(void)
{
  g_thread_yield();
}

int iupdrvThreadIsCurrent(void* handle)
{
  return (GThread*)handle == g_thread_self();
}

void iupdrvThreadExit(int code)
{
  g_thread_exit((gpointer)(intptr_t)code);
}

void iupdrvThreadDestroy(void* handle)
{
  if (handle)
    g_thread_unref((GThread*)handle);
}

void* iupdrvMutexCreate(void)
{
  GMutex* mutex = (GMutex*)malloc(sizeof(GMutex));
  if (mutex)
    g_mutex_init(mutex);
  return (void*)mutex;
}

void iupdrvMutexLock(void* handle)
{
  g_mutex_lock((GMutex*)handle);
}

void iupdrvMutexUnlock(void* handle)
{
  g_mutex_unlock((GMutex*)handle);
}

void iupdrvMutexDestroy(void* handle)
{
  if (!handle)
    return;

  g_mutex_clear((GMutex*)handle);
  free(handle);
}
