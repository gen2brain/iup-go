/** \file
 * \brief File Descriptor Watch
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_fdwatch.h"


IUP_SDK_API int iupFdWatchGetFd(Ihandle* ih)
{
  return iupAttribGetInt(ih, "FD");
}

IUP_SDK_API int iupFdWatchGetMode(Ihandle* ih)
{
  char* mode = iupAttribGetStr(ih, "MODE");

  if (iupStrEqualNoCase(mode, "WRITE"))
    return IUP_FDWATCH_WRITE;
  if (iupStrEqualNoCase(mode, "READWRITE"))
    return IUP_FDWATCH_READ | IUP_FDWATCH_WRITE;

  return IUP_FDWATCH_READ;
}

IUP_SDK_API void iupFdWatchCallAction(Ihandle* ih)
{
  Icallback cb;

  if (!iupObjectCheck(ih))
    return;

  cb = IupGetCallback(ih, "ACTION_CB");
  if (cb && cb(ih) == IUP_CLOSE)
    IupExitLoop();
}

static int iFdWatchSetRunAttrib(Ihandle* ih, const char* value)
{
  int running = iupAttribGetInt(ih, "_IUP_FDWATCH_RUNNING");

  if (iupStrBoolean(value))
  {
    if (!running && iupFdWatchGetFd(ih) >= 0 && iupdrvFdWatchStart(ih))
      iupAttribSet(ih, "_IUP_FDWATCH_RUNNING", "1");
  }
  else if (running)
  {
    iupdrvFdWatchStop(ih);
    iupAttribSet(ih, "_IUP_FDWATCH_RUNNING", NULL);
  }

  return 0;
}

static char* iFdWatchGetRunAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(iupAttribGetInt(ih, "_IUP_FDWATCH_RUNNING"));
}

static void iFdWatchDestroyMethod(Ihandle* ih)
{
  if (iupAttribGetInt(ih, "_IUP_FDWATCH_RUNNING"))
    iupdrvFdWatchStop(ih);
}

IUP_API Ihandle* IupFdWatch(void)
{
  return IupCreate("fdwatch");
}

Iclass* iupFdWatchNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "fdwatch";
  ic->format = NULL;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupFdWatchNewClass;
  ic->Destroy = iFdWatchDestroyMethod;

  iupClassRegisterCallback(ic, "ACTION_CB", "");

  iupClassRegisterAttribute(ic, "FD", NULL, NULL, IUPAF_SAMEASSYSTEM, "-1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MODE", NULL, NULL, IUPAF_SAMEASSYSTEM, "READ", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RUN", iFdWatchGetRunAttrib, iFdWatchSetRunAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupdrvFdWatchInitClass(ic);

  return ic;
}
