/** \file
 * \brief IupThread element
 *
 * See Copyright Notice in "iup.h"
 */

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
#include "iup_thread.h"


static int iThreadSetJoinAttrib(Ihandle* ih, const char* value)
{
  void* thread = iupAttribGet(ih, "THREAD");
  if (thread)
    iupdrvThreadJoin(thread);

  (void)value;
  return 0;
}

static int iThreadSetYieldAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  iupdrvThreadYield();
  return 0;
}

static char* iThreadGetIsCurrentAttrib(Ihandle* ih)
{
  void* thread = iupAttribGet(ih, "THREAD");
  if (thread)
    return iupStrReturnBoolean(iupdrvThreadIsCurrent(thread));
  return iupStrReturnBoolean(0);
}

static int iThreadSetExitAttrib(Ihandle* ih, const char* value)
{
  int exit_code = 0;
  iupStrToInt(value, &exit_code);
  iupdrvThreadExit(exit_code);

  (void)ih;
  return 0;
}

static int iThreadSetLockAttrib(Ihandle* ih, const char* value)
{
  void* mutex = iupAttribGet(ih, "MUTEX");
  if (mutex)
  {
    if (iupStrBoolean(value))
      iupdrvMutexLock(mutex);
    else
      iupdrvMutexUnlock(mutex);
  }

  return 1;
}

static int iThreadSetStartAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    void* old_thread = iupAttribGet(ih, "THREAD");
    void* thread = iupdrvThreadStart(ih);
    if (thread)
    {
      if (old_thread)
        iupdrvThreadDestroy(old_thread);
      iupAttribSet(ih, "THREAD", (char*)thread);
    }
  }

  return 0;
}

static int iThreadCreateMethod(Ihandle* ih, void **params)
{
  void* mutex = iupdrvMutexCreate();
  iupAttribSet(ih, "MUTEX", (char*)mutex);

  (void)params;
  return IUP_NOERROR;
}

static void iThreadDestroyMethod(Ihandle* ih)
{
  void* mutex = iupAttribGet(ih, "MUTEX");
  void* thread = iupAttribGet(ih, "THREAD");

  if (mutex)
    iupdrvMutexDestroy(mutex);
  if (thread)
    iupdrvThreadDestroy(thread);
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
