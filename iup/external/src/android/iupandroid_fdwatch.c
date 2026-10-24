/** \file
 * \brief File Descriptor Watch for the Android Driver.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include <android/looper.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_fdwatch.h"

#include "iupandroid_drv.h"


static int androidFdWatchProc(int fd, int events, void* data)
{
  Ihandle* ih = (Ihandle*)data;

  (void)fd;
  (void)events;

  if (!iupObjectCheck(ih))
    return 0;

  iupFdWatchCallAction(ih);
  return 1;
}

IUP_SDK_API int iupdrvFdWatchStart(Ihandle* ih)
{
  ALooper* looper = ALooper_forThread();
  int mode = iupFdWatchGetMode(ih);
  int events = 0;

  if (iupAttribGet(ih, "_IUPANDROID_FDWATCH"))
    return 1;

  if (!looper)
    return 0;

  if (mode & IUP_FDWATCH_READ)
    events |= ALOOPER_EVENT_INPUT;
  if (mode & IUP_FDWATCH_WRITE)
    events |= ALOOPER_EVENT_OUTPUT;

  if (ALooper_addFd(looper, iupFdWatchGetFd(ih), ALOOPER_POLL_CALLBACK, events, androidFdWatchProc, (void*)ih) != 1)
    return 0;

  iupAttribSet(ih, "_IUPANDROID_FDWATCH", (char*)looper);
  return 1;
}

IUP_SDK_API void iupdrvFdWatchStop(Ihandle* ih)
{
  ALooper* looper = (ALooper*)iupAttribGet(ih, "_IUPANDROID_FDWATCH");
  if (looper)
  {
    ALooper_removeFd(looper, iupFdWatchGetFd(ih));
    iupAttribSet(ih, "_IUPANDROID_FDWATCH", NULL);
  }
}

IUP_SDK_API void iupdrvFdWatchInitClass(Iclass* ic)
{
  (void)ic;
}
