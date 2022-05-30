/** \file
 * \brief Timer Control.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_timer.h"
#include "iup_attrib.h"


long long iupTimerGetLongLong(Ihandle* ih, const char* name)
{
  long long i = 0;
  char *value = iupAttribGetStr(ih, name);
  if (value)
  {
    if (sscanf(value, "%lld", &i) != 1)
      return 0;
  }
  return i;
}

static int iTimerSetRunAttrib(Ihandle *ih, const char *value)
{
  if (iupStrBoolean(value))
    iupdrvTimerRun(ih);
  else
    iupdrvTimerStop(ih);

  return 0;
}

static char* iTimerGetRunAttrib(Ihandle *ih)
{
  return iupStrReturnBoolean (ih->serial > 0); 
}

static char* iTimerGetWidAttrib(Ihandle *ih)
{
  return iupStrReturnInt(ih->serial);
}

static void iTimerDestroyMethod(Ihandle* ih)
{
  iupdrvTimerStop(ih);
}

/******************************************************************************/

IUP_API Ihandle* IupTimer(void)
{
  return IupCreate("timer");
}

Iclass* iupTimerNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "timer";
  ic->format = NULL;  /* no parameters */
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupTimerNewClass;
  ic->Destroy = iTimerDestroyMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "ACTION_CB", "");

  /* Attribute functions */
  iupClassRegisterAttribute(ic, "WID", iTimerGetWidAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "RUN", iTimerGetRunAttrib, iTimerSetRunAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIME", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupdrvTimerInitClass(ic);

  return ic;
}
