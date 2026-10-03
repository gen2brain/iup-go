/** \file
 * \brief Timer Control.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_timer.h"
#include "iup_attrib.h"
#include "iup_drv.h"


static int iTimerSetRunAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    int was_running = ih->serial > 0;
    unsigned int start = iupdrvGetTickCount();
    iupdrvTimerRun(ih);
    if (!was_running && ih->serial > 0)
      iupAttribSetInt(ih, "_IUP_TIMER_START", (int)start);
  }
  else
    iupdrvTimerStop(ih);

  return 0;
}

static char* iTimerGetRunAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->serial > 0);
}

static char* iTimerGetElapsedTimeAttrib(Ihandle* ih)
{
  unsigned int start;
  if (ih->serial <= 0)
    return NULL;
  start = (unsigned int)iupAttribGetInt(ih, "_IUP_TIMER_START");
  return iupStrReturnInt((int)(iupdrvGetTickCount() - start));
}

static char* iTimerGetWidAttrib(Ihandle* ih)
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
  iupClassRegisterAttribute(ic, "WID", iTimerGetWidAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RUN", iTimerGetRunAttrib, iTimerSetRunAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ELAPSEDTIME", iTimerGetElapsedTimeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIME", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupdrvTimerInitClass(ic);

  return ic;
}
