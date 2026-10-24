/** \file
 * \brief File Descriptor Watch for the FLTK Driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_fdwatch.h"
}


static void fltkFdWatchProc(FL_SOCKET fd, void* data)
{
  Ihandle* ih = (Ihandle*)data;

  (void)fd;

  if (!iupObjectCheck(ih))
    return;

  iupFdWatchCallAction(ih);
}

extern "C" IUP_SDK_API int iupdrvFdWatchStart(Ihandle* ih)
{
  int mode = iupFdWatchGetMode(ih);
  int when = 0;

  if (iupAttribGet(ih, "_IUPFLTK_FDWATCH"))
    return 1;

  if (mode & IUP_FDWATCH_READ)
    when |= FL_READ | FL_EXCEPT;
  if (mode & IUP_FDWATCH_WRITE)
    when |= FL_WRITE;

  Fl::add_fd(iupFdWatchGetFd(ih), when, fltkFdWatchProc, (void*)ih);
  iupAttribSet(ih, "_IUPFLTK_FDWATCH", "1");
  return 1;
}

extern "C" IUP_SDK_API void iupdrvFdWatchStop(Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPFLTK_FDWATCH"))
  {
    Fl::remove_fd(iupFdWatchGetFd(ih));
    iupAttribSet(ih, "_IUPFLTK_FDWATCH", NULL);
  }
}

extern "C" IUP_SDK_API void iupdrvFdWatchInitClass(Iclass* ic)
{
  (void)ic;
}
