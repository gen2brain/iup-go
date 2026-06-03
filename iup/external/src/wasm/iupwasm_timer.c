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


EM_JS(int, iupwasmJsSetInterval, (int ihptr, int ms), {
  return setInterval(function() { Module._iupwasmDispatchTimer(ihptr); }, ms);
})

EM_JS(void, iupwasmJsClearInterval, (int sid), {
  clearInterval(sid);
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
    ih->serial = iupwasmJsSetInterval((int)(intptr_t)ih, time_ms);
}

IUP_SDK_API void iupdrvTimerStop(Ihandle* ih)
{
  if (ih->serial > 0)
  {
    iupwasmJsClearInterval(ih->serial);
    ih->serial = -1;
  }
}

IUP_SDK_API void iupdrvTimerInitClass(Iclass* ic)
{
  (void)ic;
}
