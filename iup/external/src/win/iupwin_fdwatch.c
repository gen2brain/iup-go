/** \file
 * \brief File Descriptor Watch for the Windows Drivers.
 *
 * See Copyright Notice in "iup.h"
 */

#include <winsock2.h>
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_thread.h"
#include "iup_fdwatch.h"


typedef int (WSAAPI *winWSAEventSelect)(SOCKET s, WSAEVENT ev, long events);
typedef int (WSAAPI *winWSAEnumNetworkEvents)(SOCKET s, WSAEVENT ev, LPWSANETWORKEVENTS out);

static winWSAEventSelect my_WSAEventSelect = NULL;
static winWSAEnumNetworkEvents my_WSAEnumNetworkEvents = NULL;

typedef struct _IwinFdWatch
{
  Ihandle* ih;
  SOCKET sock;
  WSAEVENT event;
  int pending;
  struct _IwinFdWatch* next;
} IwinFdWatch;

static IwinFdWatch* win_fdwatch_list = NULL;
static void* win_fdwatch_mutex = NULL;
static HANDLE win_fdwatch_thread = NULL;
static HANDLE win_fdwatch_wake = NULL;
static volatile int win_fdwatch_running = 0;

static int winFdWatchInitWinsock(void)
{
  static int initialized = 0;

  if (!initialized)
  {
    HMODULE hWs2 = LoadLibrary(TEXT("ws2_32.dll"));
    if (hWs2)
    {
      my_WSAEventSelect = (winWSAEventSelect)GetProcAddress(hWs2, "WSAEventSelect");
      my_WSAEnumNetworkEvents = (winWSAEnumNetworkEvents)GetProcAddress(hWs2, "WSAEnumNetworkEvents");
    }
    initialized = 1;
  }

  return my_WSAEventSelect && my_WSAEnumNetworkEvents;
}

static int winFdWatchPostMessage(Ihandle* ih, char* s, int i, double d, void* p)
{
  IwinFdWatch* entry;

  (void)s;
  (void)i;
  (void)d;
  (void)p;

  iupFdWatchCallAction(ih);

  iupdrvMutexLock(win_fdwatch_mutex);
  for (entry = win_fdwatch_list; entry; entry = entry->next)
  {
    if (entry->ih == ih)
    {
      entry->pending = 0;
      break;
    }
  }
  iupdrvMutexUnlock(win_fdwatch_mutex);

  SetEvent(win_fdwatch_wake);
  return IUP_DEFAULT;
}

static DWORD WINAPI winFdWatchThread(LPVOID arg)
{
  (void)arg;

  while (win_fdwatch_running)
  {
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    Ihandle* owners[MAXIMUM_WAIT_OBJECTS];
    IwinFdWatch* entry;
    DWORD count = 0;
    DWORD ret;

    handles[count++] = win_fdwatch_wake;

    iupdrvMutexLock(win_fdwatch_mutex);
    for (entry = win_fdwatch_list; entry && count < MAXIMUM_WAIT_OBJECTS; entry = entry->next)
    {
      if (entry->pending || entry->event == WSA_INVALID_EVENT)
        continue;
      owners[count] = entry->ih;
      handles[count++] = entry->event;
    }
    iupdrvMutexUnlock(win_fdwatch_mutex);

    ret = WaitForMultipleObjects(count, handles, FALSE, INFINITE);
    if (ret == WAIT_FAILED)
      break;

    if (ret == WAIT_OBJECT_0)
      continue;

    if (ret > WAIT_OBJECT_0 && ret < WAIT_OBJECT_0 + count)
    {
      DWORD index = ret - WAIT_OBJECT_0;
      Ihandle* ih = owners[index];

      iupdrvMutexLock(win_fdwatch_mutex);
      for (entry = win_fdwatch_list; entry; entry = entry->next)
      {
        if (entry->ih == ih)
        {
          WSANETWORKEVENTS events;
          my_WSAEnumNetworkEvents(entry->sock, entry->event, &events);
          entry->pending = 1;
          break;
        }
      }
      iupdrvMutexUnlock(win_fdwatch_mutex);

      IupPostMessage(ih, "FDWATCH", 0, 0, NULL);
    }
  }

  return 0;
}

static int winFdWatchStartThread(void)
{
  if (win_fdwatch_thread)
    return 1;

  win_fdwatch_wake = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (!win_fdwatch_wake)
    return 0;

  win_fdwatch_running = 1;
  win_fdwatch_thread = CreateThread(NULL, 0, winFdWatchThread, NULL, 0, NULL);
  if (!win_fdwatch_thread)
  {
    win_fdwatch_running = 0;
    CloseHandle(win_fdwatch_wake);
    win_fdwatch_wake = NULL;
    return 0;
  }

  return 1;
}

static void winFdWatchStopThread(void)
{
  if (!win_fdwatch_thread)
    return;

  win_fdwatch_running = 0;
  SetEvent(win_fdwatch_wake);
  WaitForSingleObject(win_fdwatch_thread, 2000);

  CloseHandle(win_fdwatch_thread);
  CloseHandle(win_fdwatch_wake);
  win_fdwatch_thread = NULL;
  win_fdwatch_wake = NULL;
}

IUP_SDK_API int iupdrvFdWatchStart(Ihandle* ih)
{
  IwinFdWatch* entry;
  long events = 0;
  int mode = iupFdWatchGetMode(ih);

  if (iupAttribGet(ih, "_IUPWIN_FDWATCH"))
    return 1;

  if (!winFdWatchInitWinsock())
    return 0;

  if (!win_fdwatch_mutex)
    win_fdwatch_mutex = iupdrvMutexCreate();

  entry = (IwinFdWatch*)calloc(1, sizeof(IwinFdWatch));
  if (!entry)
    return 0;

  entry->ih = ih;
  entry->sock = (SOCKET)iupFdWatchGetFd(ih);

  if (mode & IUP_FDWATCH_READ)
    events |= FD_READ | FD_ACCEPT | FD_CLOSE | FD_OOB;
  if (mode & IUP_FDWATCH_WRITE)
    events |= FD_WRITE | FD_CONNECT;

  entry->event = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (entry->event == WSA_INVALID_EVENT ||
      my_WSAEventSelect(entry->sock, entry->event, events) == SOCKET_ERROR)
  {
    if (entry->event != WSA_INVALID_EVENT)
      CloseHandle(entry->event);
    free(entry);
    return 0;
  }

  IupSetCallback(ih, "POSTMESSAGE_CB", (Icallback)winFdWatchPostMessage);

  iupdrvMutexLock(win_fdwatch_mutex);
  entry->next = win_fdwatch_list;
  win_fdwatch_list = entry;
  iupdrvMutexUnlock(win_fdwatch_mutex);

  if (!winFdWatchStartThread())
  {
    iupdrvMutexLock(win_fdwatch_mutex);
    win_fdwatch_list = entry->next;
    iupdrvMutexUnlock(win_fdwatch_mutex);
    CloseHandle(entry->event);
    free(entry);
    return 0;
  }

  iupAttribSet(ih, "_IUPWIN_FDWATCH", (char*)entry);
  SetEvent(win_fdwatch_wake);
  return 1;
}

IUP_SDK_API void iupdrvFdWatchStop(Ihandle* ih)
{
  IwinFdWatch* entry = (IwinFdWatch*)iupAttribGet(ih, "_IUPWIN_FDWATCH");
  IwinFdWatch** link;
  int empty;

  if (!entry)
    return;

  iupdrvMutexLock(win_fdwatch_mutex);
  for (link = &win_fdwatch_list; *link; link = &(*link)->next)
  {
    if (*link == entry)
    {
      *link = entry->next;
      break;
    }
  }
  empty = (win_fdwatch_list == NULL);
  iupdrvMutexUnlock(win_fdwatch_mutex);

  my_WSAEventSelect(entry->sock, NULL, 0);
  CloseHandle(entry->event);
  iupAttribSet(ih, "_IUPWIN_FDWATCH", NULL);
  free(entry);

  if (empty)
    winFdWatchStopThread();
  else
    SetEvent(win_fdwatch_wake);
}

IUP_SDK_API void iupdrvFdWatchInitClass(Iclass* ic)
{
  (void)ic;
}
