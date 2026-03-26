/** \file
 * \brief Windows Driver - Thread Support
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <stdint.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_thread.h"

static DWORD WINAPI iupwin_ThreadFunc(LPVOID obj)
{
  Ihandle* ih = (Ihandle*)obj;
  Icallback cb = IupGetCallback(ih, "THREAD_CB");
  if (cb)
    cb(ih);
  return 0;
}

void* iupdrvThreadStart(Ihandle* ih)
{
  DWORD threadId;
  HANDLE thread = CreateThread(0, 0, iupwin_ThreadFunc, ih, 0, &threadId);
  return (void*)thread;
}

void iupdrvThreadJoin(void* handle)
{
  WaitForSingleObject((HANDLE)handle, INFINITE);
}

void iupdrvThreadYield(void)
{
  SwitchToThread();
}

int iupdrvThreadIsCurrent(void* handle)
{
  return (HANDLE)handle == GetCurrentThread();
}

void iupdrvThreadExit(int code)
{
  ExitThread(code);
}

void iupdrvThreadDestroy(void* handle)
{
  if (handle)
    CloseHandle((HANDLE)handle);
}

void* iupdrvMutexCreate(void)
{
  return (void*)CreateMutexA(NULL, FALSE, NULL);
}

void iupdrvMutexLock(void* handle)
{
  WaitForSingleObject((HANDLE)handle, INFINITE);
}

void iupdrvMutexUnlock(void* handle)
{
  ReleaseMutex((HANDLE)handle);
}

void iupdrvMutexDestroy(void* handle)
{
  if (handle)
    CloseHandle((HANDLE)handle);
}
