/** \file
 * \brief Timer for the EFL Driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_timer.h"

#include "iupefl_drv.h"


static void eflTimerTickCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Icallback cb;

  (void)ev;

  if (!iupObjectCheck(ih))
    return;

  cb = IupGetCallback(ih, "ACTION_CB");
  if (cb)
  {
    Eo* timer = (Eo*)iupAttribGet(ih, "_IUP_EFL_TIMER");
    if (timer)
    {
      double pending = efl_loop_timer_time_pending_get(timer);
      int time_ms = iupAttribGetInt(ih, "TIME");
      iupAttribSetInt(ih, "ELAPSEDTIME", time_ms - (int)(pending * 1000));
    }

    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

void iupdrvTimerRun(Ihandle* ih)
{
  Eo* timer;
  unsigned int time_ms;

  if (ih->serial > 0)
    return;

  time_ms = iupAttribGetInt(ih, "TIME");
  if (time_ms > 0)
  {
    double interval = (double)time_ms / 1000.0;
    Eo* loop = iupeflGetLoop();

    if (!loop)
      return;

    timer = efl_add(EFL_LOOP_TIMER_CLASS, loop, efl_loop_timer_interval_set(efl_added, interval),
                    efl_event_callback_add(efl_added, EFL_LOOP_TIMER_EVENT_TIMER_TICK, eflTimerTickCallback, ih));
    if (timer)
    {
      iupAttribSet(ih, "_IUP_EFL_TIMER", (char*)timer);
      ih->serial = 1;
    }
  }
}

void iupdrvTimerStop(Ihandle* ih)
{
  if (ih->serial > 0)
  {
    Eo* timer = (Eo*)iupAttribGet(ih, "_IUP_EFL_TIMER");
    if (timer)
    {
      efl_event_callback_del(timer, EFL_LOOP_TIMER_EVENT_TIMER_TICK, eflTimerTickCallback, ih);
      iupeflDelete(timer);
      iupAttribSet(ih, "_IUP_EFL_TIMER", NULL);
    }
    ih->serial = -1;
  }
}

void iupdrvTimerInitClass(Iclass* ic)
{
  (void)ic;
}
