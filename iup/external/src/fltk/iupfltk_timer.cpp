/** \file
 * \brief Timer for the FLTK Driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>

#include <cstdlib>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_timer.h"
}

#include "iupfltk_drv.h"


typedef struct _IupFltkTimer
{
  Ihandle* ih;
  double interval_sec;
  int active;
  Fl_Timestamp start_time;
} IupFltkTimer;

static void fltkTimerProc(void* data)
{
  IupFltkTimer* timer_data = (IupFltkTimer*)data;
  if (!timer_data || !timer_data->active)
    return;

  Ihandle* ih = timer_data->ih;
  if (!iupObjectCheck(ih))
    return;

  double elapsed = Fl::seconds_since(timer_data->start_time);
  iupAttribSetInt(ih, "ELAPSEDTIME", (int)(elapsed * 1000));

  Icallback cb = IupGetCallback(ih, "ACTION_CB");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
    {
      IupExitLoop();
      return;
    }
  }

  if (timer_data->active)
    Fl::repeat_timeout(timer_data->interval_sec, fltkTimerProc, data);
}

extern "C" IUP_SDK_API void iupdrvTimerRun(Ihandle *ih)
{
  unsigned int time_ms;

  if (ih->serial > 0)
    return;

  time_ms = iupAttribGetInt(ih, "TIME");
  if (time_ms > 0)
  {
    IupFltkTimer* timer_data = (IupFltkTimer*)malloc(sizeof(IupFltkTimer));
    timer_data->ih = ih;
    timer_data->interval_sec = time_ms / 1000.0;
    timer_data->active = 1;
    timer_data->start_time = Fl::now();

    Fl::add_timeout(timer_data->interval_sec, fltkTimerProc, (void*)timer_data);

    ih->serial = 1;
    iupAttribSet(ih, "_IUP_FLTKTIMER", (char*)timer_data);
  }
}

extern "C" IUP_SDK_API void iupdrvTimerStop(Ihandle* ih)
{
  if (ih->serial > 0)
  {
    IupFltkTimer* timer_data = (IupFltkTimer*)iupAttribGet(ih, "_IUP_FLTKTIMER");

    if (timer_data)
    {
      timer_data->active = 0;
      Fl::remove_timeout(fltkTimerProc, (void*)timer_data);

      free(timer_data);
      iupAttribSet(ih, "_IUP_FLTKTIMER", NULL);
    }

    ih->serial = -1;
  }
}

extern "C" IUP_SDK_API void iupdrvTimerInitClass(Iclass* ic)
{
  (void)ic;
}
