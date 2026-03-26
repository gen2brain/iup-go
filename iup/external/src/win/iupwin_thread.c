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

IUP_SDK_API void* iupdrvThreadStart(Ihandle* ih)
{
  DWORD threadId;
  HANDLE thread = CreateThread(0, 0, iupwin_ThreadFunc, ih, 0, &threadId);
  return (void*)thread;
}

IUP_SDK_API void iupdrvThreadJoin(void* handle)
{
  WaitForSingleObject((HANDLE)handle, INFINITE);
}

IUP_SDK_API void iupdrvThreadYield(void)
{
  SwitchToThread();
}

IUP_SDK_API int iupdrvThreadIsCurrent(void* handle)
{
  return (HANDLE)handle == GetCurrentThread();
}

IUP_SDK_API void iupdrvThreadExit(int code)
{
  ExitThread(code);
}

IUP_SDK_API void iupdrvThreadDestroy(void* handle)
{
  if (handle)
    CloseHandle((HANDLE)handle);
}

IUP_SDK_API void* iupdrvMutexCreate(void)
{
  return (void*)CreateMutexA(NULL, FALSE, NULL);
}

IUP_SDK_API void iupdrvMutexLock(void* handle)
{
  WaitForSingleObject((HANDLE)handle, INFINITE);
}

IUP_SDK_API void iupdrvMutexUnlock(void* handle)
{
  ReleaseMutex((HANDLE)handle);
}

IUP_SDK_API void iupdrvMutexDestroy(void* handle)
{
  if (handle)
    CloseHandle((HANDLE)handle);
}
