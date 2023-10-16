/** \file
 * \brief IupThread element
 *
 * See Copyright Notice in "iup.h"
 */

#ifdef GTK_DISABLE_DEPRECATED
#define IUP_USE_GTK
#ifndef GLIB_CHECK_VERSION
#define OLD_GLIB
#endif
#endif

#ifdef IUP_USE_GTK
#include <glib.h>
/* #include <gthread.h> */
#elif defined(WIN32)
#include <windows.h>
#else
#warning "FIXME: IupThread platform not identified/supported"
#endif

#include <stdio.h>              
#include <stdlib.h>
#include <string.h>             

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_register.h"
#include "iup_stdcontrols.h"


static int iThreadSetJoinAttrib(Ihandle* ih, const char* value)
{
#ifdef IUP_USE_GTK
  GThread* thread = (GThread*)iupAttribGet(ih, "THREAD");
  g_thread_join(thread);
#elif defined(WIN32)
  HANDLE thread = (HANDLE)iupAttribGet(ih, "THREAD");
  WaitForSingleObject(thread, INFINITE);
#endif

  (void)value;
  return 0;
}

static int iThreadSetYieldAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;

#ifdef IUP_USE_GTK
  g_thread_yield();
#elif defined(WIN32)
  SwitchToThread();
#endif
  return 0;
}

static char* iThreadGetIsCurrentAttrib(Ihandle* ih)
{
#ifdef IUP_USE_GTK
  GThread* thread = (GThread*)iupAttribGet(ih, "THREAD");
  return iupStrReturnBoolean(thread == g_thread_self());
#elif defined(WIN32)
  HANDLE thread = (HANDLE)iupAttribGet(ih, "THREAD");
  return iupStrReturnBoolean(thread == GetCurrentThread());
#else
	return NULL;
#endif
}

static int iThreadSetExitAttrib(Ihandle* ih, const char* value)
{
  int exit_code = 0;
  iupStrToInt(value, &exit_code);

#ifdef IUP_USE_GTK
  g_thread_exit((gpointer)exit_code);
#elif defined(WIN32)
  ExitThread(exit_code);
#endif	

  (void)ih;
  return 0;
}

static int iThreadSetLockAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
#ifdef IUP_USE_GTK
    GMutex* mutex = (GMutex*)iupAttribGet(ih, "MUTEX");
    g_mutex_lock(mutex);
#elif defined(WIN32)
    HANDLE mutex = (HANDLE)iupAttribGet(ih, "MUTEX");
    WaitForSingleObject(mutex, INFINITE);
#endif	
  }
  else
  {
#ifdef IUP_USE_GTK
    GMutex* mutex = (GMutex*)iupAttribGet(ih, "MUTEX");
    g_mutex_unlock(mutex);
#elif defined(WIN32)
    HANDLE mutex = (HANDLE)iupAttribGet(ih, "MUTEX");
    ReleaseMutex(mutex);
#endif
  }

  return 1;
}

#ifdef IUP_USE_GTK
static void* ClientThreadFunc(void* obj)
#elif defined(WIN32)
static DWORD WINAPI ClientThreadFunc(LPVOID obj)
#else
static void* ClientThreadFunc(void* obj)
#endif
{
  Ihandle* ih = (Ihandle*)obj;
  Icallback cb = IupGetCallback(ih, "THREAD_CB");
  if (cb)
    cb(ih);
  return 0;
};

static int iThreadSetStartAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
#ifdef IUP_USE_GTK
    GThread* thread, *old_thread;
    char* name = iupAttribGet(ih, "THREADNAME");
    if (!name) name = "IupThread";
#ifdef OLD_GLIB
    thread = g_thread_create(ClientThreadFunc, ih, FALSE, NULL);
#else
    thread = g_thread_new(name, ClientThreadFunc, ih);
#endif
    old_thread = (GThread*)iupAttribGet(ih, "THREAD");
#ifndef OLD_GLIB
    if (old_thread) g_thread_unref(old_thread);
#endif
    iupAttribSet(ih, "THREAD", (char*)thread);
#elif defined(WIN32)
    DWORD threadId;
    HANDLE thread = CreateThread(0, 0, ClientThreadFunc, ih, 0, &threadId);
    HANDLE old_thread = (HANDLE)iupAttribGet(ih, "THREAD");
    if (old_thread) CloseHandle(old_thread);
    iupAttribSet(ih, "THREAD", (char*)thread);
#endif
  }

  return 0;
}

static int iThreadCreateMethod(Ihandle* ih, void **params)
{
#ifdef IUP_USE_GTK
#ifdef OLD_GLIB
  GMutex* mutex = g_mutex_new();
#else
  GMutex* mutex = (GMutex*)malloc(sizeof(GMutex));
#endif
#elif defined(WIN32)
  HANDLE mutex;
#else
  void* mutex = NULL;
#endif
  (void)params;

#ifdef IUP_USE_GTK
#ifndef OLD_GLIB
  g_mutex_init(mutex);
#endif
#elif defined(WIN32)
  mutex = CreateMutexA(NULL, FALSE, "mutex");
#endif

  iupAttribSet(ih, "MUTEX", (char*)mutex);

  return IUP_NOERROR;
}

static void iThreadDestroyMethod(Ihandle* ih)
{
#ifdef IUP_USE_GTK
  GMutex* mutex = (GMutex*)iupAttribGet(ih, "MUTEX");
  GThread* thread = (GThread*)iupAttribGet(ih, "THREAD");
#ifdef OLD_GLIB
  g_mutex_free(mutex);
#else
  g_mutex_clear(mutex);
#endif
  free(mutex);
#ifndef OLD_GLIB
  if (thread) g_thread_unref(thread);
#endif
#elif defined(WIN32)
  HANDLE mutex = (HANDLE)iupAttribGet(ih, "MUTEX");
  HANDLE thread = (HANDLE)iupAttribGet(ih, "THREAD");
  CloseHandle(mutex);
  if (thread) CloseHandle(thread);
#endif
}

Iclass* iupThreadNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "thread";
  ic->format = NULL; /* no parameters */
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupThreadNewClass;
  ic->Create = iThreadCreateMethod;
  ic->Destroy = iThreadDestroyMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "THREAD_CB", "");

  /* Attributes */
  iupClassRegisterAttribute(ic, "START", NULL, iThreadSetStartAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_NO_DEFAULTVALUE);
  iupClassRegisterAttribute(ic, "LOCK", NULL, iThreadSetLockAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_NO_DEFAULTVALUE);
  iupClassRegisterAttribute(ic, "EXIT", NULL, iThreadSetExitAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_NO_DEFAULTVALUE);
  iupClassRegisterAttribute(ic, "ISCURRENT", iThreadGetIsCurrentAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_NO_DEFAULTVALUE);
  iupClassRegisterAttribute(ic, "YIELD", NULL, iThreadSetYieldAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_NO_DEFAULTVALUE);
  iupClassRegisterAttribute(ic, "JOIN", NULL, iThreadSetJoinAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_NO_DEFAULTVALUE);

  return ic;
}

IUP_API Ihandle* IupThread(void)
{
  return IupCreate("thread");
}
