/** \file
 * \brief Timer for the Motif Driver.
 *
 * See Copyright Notice in "iup.h"
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <Xm/Xm.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_timer.h"

#include "iupmot_drv.h"


static void motTimerProc(XtPointer client_data, XtIntervalId *id)
{
  Ihandle *ih = (Ihandle*)client_data;
  Icallback cb;
  (void)id;

  if (!iupObjectCheck(ih))   /* control could be destroyed before timer callback */
    return;

  ih->serial = -1;
  /* we have to restart the timer every time */
  iupdrvTimerRun(ih);

  cb = IupGetCallback(ih, "ACTION_CB");
  if (cb)
  {
    long long end = (long long)clock();
    long long start = iupTimerGetLongLong(ih, "STARTCOUNT");
    iupAttribSetInt(ih, "ELAPSEDTIME", (int)(end - start));

    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

void iupdrvTimerRun(Ihandle *ih)
{
  unsigned int time_ms;

  if (ih->serial > 0) /* timer already started */
    return;
  
  time_ms = iupAttribGetInt(ih, "TIME");
  if (time_ms > 0)
  {
    long long start;

    ih->serial = XtAppAddTimeOut(iupmot_appcontext, time_ms, motTimerProc, (XtPointer)ih);

    start = (long long)clock();
    iupAttribSetStrf(ih, "STARTCOUNT", "%lld", start);
  }
}

void iupdrvTimerStop(Ihandle* ih)
{
  if (ih->serial > 0)
  {
    XtRemoveTimeOut(ih->serial);
    ih->serial = -1;
  }
}

void iupdrvTimerInitClass(Iclass* ic)
{
  (void)ic;
}
