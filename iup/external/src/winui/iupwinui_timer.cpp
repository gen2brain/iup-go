/** \file
 * \brief WinUI Driver - Timer
 *
 * Uses WinUI DispatcherQueueTimer.
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <chrono>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_timer.h"
#include "iup_register.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Dispatching;
using namespace Windows::Foundation;


struct IupWinUITimer
{
  DispatcherQueueTimer timer;
  event_token tickToken;
  std::chrono::steady_clock::time_point startTime;
  Ihandle* ih;

  IupWinUITimer() : timer(nullptr), tickToken{}, ih(nullptr) {}
};

#define IUPWINUI_TIMER_DATA "_IUPWINUI_TIMER_DATA"

static void winuiTimerProc(IupWinUITimer* timer_data)
{
  Ihandle* ih = timer_data->ih;
  Icallback cb;

  if (!iupObjectCheck(ih))
    return;

  cb = IupGetCallback(ih, "ACTION_CB");
  if (cb)
  {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - timer_data->startTime).count();
    iupAttribSetInt(ih, "ELAPSEDTIME", (int)elapsed);

    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

extern "C" void iupdrvTimerRun(Ihandle* ih)
{
  unsigned int time_ms;

  if (ih->serial > 0)
    return;

  time_ms = iupAttribGetInt(ih, "TIME");
  if (time_ms > 0)
  {
    void* dq_ptr = iupwinuiGetDispatcherQueue();
    if (!dq_ptr)
      return;

    IInspectable dq_obj{nullptr};
    winrt::copy_from_abi(dq_obj, dq_ptr);
    DispatcherQueue dq = dq_obj.as<DispatcherQueue>();

    IupWinUITimer* timer_data = new IupWinUITimer();
    if (!timer_data)
      return;

    timer_data->ih = ih;
    timer_data->timer = dq.CreateTimer();
    timer_data->timer.Interval(std::chrono::milliseconds(time_ms));
    timer_data->timer.IsRepeating(true);

    timer_data->tickToken = timer_data->timer.Tick([timer_data](IInspectable const&, IInspectable const&) {
      winuiTimerProc(timer_data);
    });

    timer_data->startTime = std::chrono::steady_clock::now();
    timer_data->timer.Start();

    ih->serial = 1;
    iupAttribSet(ih, IUPWINUI_TIMER_DATA, (char*)timer_data);
  }
}

extern "C" void iupdrvTimerStop(Ihandle* ih)
{
  if (ih->serial > 0)
  {
    IupWinUITimer* timer_data = (IupWinUITimer*)iupAttribGet(ih, IUPWINUI_TIMER_DATA);

    if (timer_data)
    {
      if (timer_data->timer)
      {
        timer_data->timer.Stop();
        timer_data->timer.Tick(timer_data->tickToken);
        timer_data->timer = nullptr;
      }

      delete timer_data;
      iupAttribSet(ih, IUPWINUI_TIMER_DATA, nullptr);
    }

    ih->serial = -1;
  }
}

extern "C" void iupdrvTimerInitClass(Iclass* ic)
{
  (void)ic;
}
