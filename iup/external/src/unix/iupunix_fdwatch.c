/** \file
 * \brief File Descriptor Watch for drivers whose loop cannot watch descriptors.
 *
 * See Copyright Notice in "iup.h"
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_thread.h"
#include "iup_fdwatch.h"


typedef struct _IunixFdWatch
{
  Ihandle* ih;
  int fd;
  int mode;
  int pending;
  struct _IunixFdWatch* next;
} IunixFdWatch;

static IunixFdWatch* unix_fdwatch_list = NULL;
static void* unix_fdwatch_mutex = NULL;
static pthread_t unix_fdwatch_thread;
static int unix_fdwatch_running = 0;
static int unix_fdwatch_wake[2] = { -1, -1 };

static void unixFdWatchKick(void)
{
  if (unix_fdwatch_wake[1] >= 0)
  {
    char c = 'w';
    if (write(unix_fdwatch_wake[1], &c, 1) != 1)
      return;
  }
}

static int unixFdWatchPostMessage(Ihandle* ih, char* s, int i, double d, void* p)
{
  IunixFdWatch* entry;

  (void)s;
  (void)i;
  (void)d;
  (void)p;

  iupFdWatchCallAction(ih);

  iupdrvMutexLock(unix_fdwatch_mutex);
  for (entry = unix_fdwatch_list; entry; entry = entry->next)
  {
    if (entry->ih == ih)
    {
      entry->pending = 0;
      break;
    }
  }
  iupdrvMutexUnlock(unix_fdwatch_mutex);

  unixFdWatchKick();
  return IUP_DEFAULT;
}

static void* unixFdWatchThread(void* arg)
{
  (void)arg;

  while (unix_fdwatch_running)
  {
    fd_set read_set, write_set;
    IunixFdWatch* entry;
    int maxfd = unix_fdwatch_wake[0];
    int ready;

    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_SET(unix_fdwatch_wake[0], &read_set);

    iupdrvMutexLock(unix_fdwatch_mutex);
    for (entry = unix_fdwatch_list; entry; entry = entry->next)
    {
      if (entry->pending || entry->fd < 0)
        continue;
      if (entry->mode & IUP_FDWATCH_READ)
        FD_SET(entry->fd, &read_set);
      if (entry->mode & IUP_FDWATCH_WRITE)
        FD_SET(entry->fd, &write_set);
      if (entry->fd > maxfd)
        maxfd = entry->fd;
    }
    iupdrvMutexUnlock(unix_fdwatch_mutex);

    ready = select(maxfd + 1, &read_set, &write_set, NULL, NULL);
    if (ready < 0)
    {
      if (errno == EINTR)
        continue;
      break;
    }

    if (FD_ISSET(unix_fdwatch_wake[0], &read_set))
    {
      char buf[64];
      while (read(unix_fdwatch_wake[0], buf, sizeof(buf)) > 0)
        ;
    }

    {
      Ihandle* ready_list[FD_SETSIZE];
      int ready_count = 0;
      int i;

      iupdrvMutexLock(unix_fdwatch_mutex);
      for (entry = unix_fdwatch_list; entry; entry = entry->next)
      {
        if (entry->pending || entry->fd < 0)
          continue;
        if (FD_ISSET(entry->fd, &read_set) || FD_ISSET(entry->fd, &write_set))
        {
          entry->pending = 1;
          if (ready_count < FD_SETSIZE)
            ready_list[ready_count++] = entry->ih;
        }
      }
      iupdrvMutexUnlock(unix_fdwatch_mutex);

      for (i = 0; i < ready_count; i++)
        IupPostMessage(ready_list[i], "FDWATCH", 0, 0, NULL);
    }
  }

  return NULL;
}

static int unixFdWatchStartThread(void)
{
  if (unix_fdwatch_running)
    return 1;

  if (pipe(unix_fdwatch_wake) != 0)
    return 0;

  fcntl(unix_fdwatch_wake[0], F_SETFL, fcntl(unix_fdwatch_wake[0], F_GETFL, 0) | O_NONBLOCK);
  fcntl(unix_fdwatch_wake[1], F_SETFL, fcntl(unix_fdwatch_wake[1], F_GETFL, 0) | O_NONBLOCK);

  unix_fdwatch_running = 1;
  if (pthread_create(&unix_fdwatch_thread, NULL, unixFdWatchThread, NULL) != 0)
  {
    unix_fdwatch_running = 0;
    close(unix_fdwatch_wake[0]);
    close(unix_fdwatch_wake[1]);
    unix_fdwatch_wake[0] = unix_fdwatch_wake[1] = -1;
    return 0;
  }

  return 1;
}

static void unixFdWatchStopThread(void)
{
  if (!unix_fdwatch_running)
    return;

  unix_fdwatch_running = 0;
  unixFdWatchKick();
  pthread_join(unix_fdwatch_thread, NULL);

  close(unix_fdwatch_wake[0]);
  close(unix_fdwatch_wake[1]);
  unix_fdwatch_wake[0] = unix_fdwatch_wake[1] = -1;
}

IUP_SDK_API int iupdrvFdWatchStart(Ihandle* ih)
{
  IunixFdWatch* entry;

  if (iupAttribGet(ih, "_IUPUNIX_FDWATCH"))
    return 1;

  if (!unix_fdwatch_mutex)
    unix_fdwatch_mutex = iupdrvMutexCreate();

  entry = (IunixFdWatch*)calloc(1, sizeof(IunixFdWatch));
  if (!entry)
    return 0;

  entry->ih = ih;
  entry->fd = iupFdWatchGetFd(ih);
  entry->mode = iupFdWatchGetMode(ih);

  IupSetCallback(ih, "POSTMESSAGE_CB", (Icallback)unixFdWatchPostMessage);

  iupdrvMutexLock(unix_fdwatch_mutex);
  entry->next = unix_fdwatch_list;
  unix_fdwatch_list = entry;
  iupdrvMutexUnlock(unix_fdwatch_mutex);

  if (!unixFdWatchStartThread())
  {
    iupdrvMutexLock(unix_fdwatch_mutex);
    unix_fdwatch_list = entry->next;
    iupdrvMutexUnlock(unix_fdwatch_mutex);
    free(entry);
    return 0;
  }

  iupAttribSet(ih, "_IUPUNIX_FDWATCH", (char*)entry);
  unixFdWatchKick();
  return 1;
}

IUP_SDK_API void iupdrvFdWatchStop(Ihandle* ih)
{
  IunixFdWatch* entry = (IunixFdWatch*)iupAttribGet(ih, "_IUPUNIX_FDWATCH");
  IunixFdWatch** link;
  int empty;

  if (!entry)
    return;

  iupdrvMutexLock(unix_fdwatch_mutex);
  for (link = &unix_fdwatch_list; *link; link = &(*link)->next)
  {
    if (*link == entry)
    {
      *link = entry->next;
      break;
    }
  }
  empty = (unix_fdwatch_list == NULL);
  iupdrvMutexUnlock(unix_fdwatch_mutex);

  iupAttribSet(ih, "_IUPUNIX_FDWATCH", NULL);
  free(entry);

  if (empty)
    unixFdWatchStopThread();
  else
    unixFdWatchKick();
}

IUP_SDK_API void iupdrvFdWatchInitClass(Iclass* ic)
{
  (void)ic;
}
