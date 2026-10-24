/** \file
 * \brief File Descriptor Watch for the EFL Driver.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include <Ecore.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_fdwatch.h"


static Eina_Bool eflFdWatchProc(void* data, Ecore_Fd_Handler* handler)
{
  Ihandle* ih = (Ihandle*)data;

  (void)handler;

  if (!iupObjectCheck(ih))
    return ECORE_CALLBACK_CANCEL;

  iupFdWatchCallAction(ih);
  return ECORE_CALLBACK_RENEW;
}

IUP_SDK_API int iupdrvFdWatchStart(Ihandle* ih)
{
  int mode = iupFdWatchGetMode(ih);
  Ecore_Fd_Handler_Flags flags = 0;
  Ecore_Fd_Handler* handler;

  if (iupAttribGet(ih, "_IUPEFL_FDWATCH"))
    return 1;

  if (mode & IUP_FDWATCH_READ)
    flags |= ECORE_FD_READ | ECORE_FD_ERROR;
  if (mode & IUP_FDWATCH_WRITE)
    flags |= ECORE_FD_WRITE;

  handler = ecore_main_fd_handler_add(iupFdWatchGetFd(ih), flags, eflFdWatchProc, ih, NULL, NULL);
  if (!handler)
    return 0;

  iupAttribSet(ih, "_IUPEFL_FDWATCH", (char*)handler);
  return 1;
}

IUP_SDK_API void iupdrvFdWatchStop(Ihandle* ih)
{
  Ecore_Fd_Handler* handler = (Ecore_Fd_Handler*)iupAttribGet(ih, "_IUPEFL_FDWATCH");
  if (handler)
  {
    ecore_main_fd_handler_del(handler);
    iupAttribSet(ih, "_IUPEFL_FDWATCH", NULL);
  }
}

IUP_SDK_API void iupdrvFdWatchInitClass(Iclass* ic)
{
  (void)ic;
}
