/** \file
 * \brief File Descriptor Watch for the Motif Driver.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include <Xm/Xm.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_fdwatch.h"

#include "iupmot_drv.h"


static void motFdWatchProc(XtPointer client_data, int* fd, XtInputId* id)
{
  Ihandle* ih = (Ihandle*)client_data;

  (void)fd;
  (void)id;

  if (!iupObjectCheck(ih))
    return;

  iupFdWatchCallAction(ih);
}

IUP_SDK_API int iupdrvFdWatchStart(Ihandle* ih)
{
  int mode = iupFdWatchGetMode(ih);
  XtPointer condition = 0;
  XtInputId id;

  if (iupAttribGet(ih, "_IUPMOT_FDWATCH"))
    return 1;

  if (mode & IUP_FDWATCH_READ)
    condition = (XtPointer)(XtInputReadMask | XtInputExceptMask);
  if (mode & IUP_FDWATCH_WRITE)
    condition = (XtPointer)((long)condition | XtInputWriteMask);

  id = XtAppAddInput(iupmot_appcontext, iupFdWatchGetFd(ih), condition, motFdWatchProc, (XtPointer)ih);
  if (!id)
    return 0;

  iupAttribSetInt(ih, "_IUPMOT_FDWATCH", (int)id);
  return 1;
}

IUP_SDK_API void iupdrvFdWatchStop(Ihandle* ih)
{
  XtInputId id = (XtInputId)iupAttribGetInt(ih, "_IUPMOT_FDWATCH");
  if (id)
  {
    XtRemoveInput(id);
    iupAttribSet(ih, "_IUPMOT_FDWATCH", NULL);
  }
}

IUP_SDK_API void iupdrvFdWatchInitClass(Iclass* ic)
{
  (void)ic;
}
