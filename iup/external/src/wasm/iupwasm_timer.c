/** \file
 * \brief WebAssembly Timer
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdint.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_timer.h"


static int wasm_timer_last_id = 0;

EM_JS(void, iupwasmJsTimerRun, (int tid, int ihptr, int ms), {
  globalThis.__iupApply({ op: 'timerrun', tid: tid, ih: ihptr, ms: ms });
})

EM_JS(void, iupwasmJsTimerStop, (int tid), {
  globalThis.__iupApply({ op: 'timerstop', tid: tid });
})

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchTimer(int ihptr)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihptr;
  Icallback cb;
  if (!iupObjectCheck(ih))
    return;
  cb = IupGetCallback(ih, "ACTION_CB");
  if (cb && cb(ih) == IUP_CLOSE)
    IupExitLoop();
}

IUP_SDK_API void iupdrvTimerRun(Ihandle* ih)
{
  int time_ms;
  if (ih->serial > 0)
    return;
  time_ms = iupAttribGetInt(ih, "TIME");
  if (time_ms > 0)
  {
    ih->serial = ++wasm_timer_last_id;
    iupwasmJsTimerRun(ih->serial, (int)(intptr_t)ih, time_ms);
  }
}

IUP_SDK_API void iupdrvTimerStop(Ihandle* ih)
{
  if (ih->serial > 0)
  {
    iupwasmJsTimerStop(ih->serial);
    ih->serial = -1;
  }
}

IUP_SDK_API void iupdrvTimerInitClass(Iclass* ic)
{
  (void)ic;
}
