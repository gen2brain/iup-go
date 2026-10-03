/** \file
 * \brief Timer for the Windows Driver.
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_timer.h"


static Itable* wintimer_id_table = NULL; /* table indexed by ID containing Ihandle* address */

static VOID CALLBACK winTimerFunc(HWND hwnd, UINT msg, UINT_PTR wid, DWORD time)
{
  Icallback cb;
  Ihandle* ih;

  (void)time;
  (void)msg;
  (void)hwnd;

  ih = (Ihandle*)iupTableGet(wintimer_id_table, (char*)wid);

  if (!iupObjectCheck(ih))   /* control could be destroyed before timer callback */
    return;

  cb = IupGetCallback(ih, "ACTION_CB");
  if(cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

IUP_SDK_API void iupdrvTimerRun(Ihandle* ih)
{
  unsigned int time_ms;

  if (ih->serial > 0) /* timer already started */
    return;

  time_ms = iupAttribGetInt(ih, "TIME");
  if (time_ms > 0)
  {
    ih->serial = (int)SetTimer(NULL, 0, time_ms, winTimerFunc);  /* minimum is 10 ms */
    iupTableSet(wintimer_id_table, (const char*)(intptr_t)ih->serial, ih, IUPTABLE_POINTER);
  }
}

IUP_SDK_API void iupdrvTimerStop(Ihandle* ih)
{
  if (ih->serial > 0)
  {
    KillTimer(NULL, ih->serial);
    iupTableRemove(wintimer_id_table, (const char*)(intptr_t)ih->serial);
    ih->serial = -1;
  }
}

static void winTimerRelease(Iclass* ic)
{
  (void)ic;

  if (wintimer_id_table)
  {
    iupTableDestroy(wintimer_id_table);
    wintimer_id_table = NULL;
  }
}

IUP_SDK_API void iupdrvTimerInitClass(Iclass* ic)
{
  ic->Release = winTimerRelease;

  if (!wintimer_id_table)
    wintimer_id_table = iupTableCreate(IUPTABLE_POINTERINDEXED);
}
