/** \file
 * \brief GTK2/GTK3 Driver - Thread Support
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

#if !GLIB_CHECK_VERSION(2, 32, 0)
#define OLD_GLIB
#endif

static void* iupgtk_ThreadFunc(void* obj)
{
  Ihandle* ih = (Ihandle*)obj;
  Icallback cb = IupGetCallback(ih, "THREAD_CB");
  if (cb)
    cb(ih);
  return NULL;
}

void* iupdrvThreadStart(Ihandle* ih)
{
  GThread* thread;
  char* name = iupAttribGet(ih, "THREADNAME");
  if (!name) name = "IupThread";

#ifdef OLD_GLIB
  thread = g_thread_create(iupgtk_ThreadFunc, ih, FALSE, NULL);
#else
  thread = g_thread_new(name, iupgtk_ThreadFunc, ih);
#endif

  return (void*)thread;
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
#ifndef OLD_GLIB
  if (handle)
    g_thread_unref((GThread*)handle);
#else
  (void)handle;
#endif
}

void* iupdrvMutexCreate(void)
{
#ifdef OLD_GLIB
  return (void*)g_mutex_new();
#else
  GMutex* mutex = (GMutex*)malloc(sizeof(GMutex));
  if (mutex)
    g_mutex_init(mutex);
  return (void*)mutex;
#endif
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

#ifdef OLD_GLIB
  g_mutex_free((GMutex*)handle);
#else
  g_mutex_clear((GMutex*)handle);
  free(handle);
#endif
}
