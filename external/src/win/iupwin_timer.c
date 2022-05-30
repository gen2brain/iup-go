/** \file
 * \brief Timer for the Windows Driver.
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
 
#include <stdio.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_timer.h"


static ULONGLONG(WINAPI *winTimerGetTickCount64) (void) = NULL;

static Itable* wintimer_id_table = NULL; /* table indexed by ID containing Ihandle* address */

static long long winTimerGetTickCount(void)
{
  /* both have 10 ms precision only */
  if (winTimerGetTickCount64)
    return (long long)winTimerGetTickCount64();
  else 
    return (long long)GetTickCount();
}

static VOID CALLBACK winTimerFunc(HWND hwnd, UINT msg, UINT_PTR wid, DWORD time)
{
  Icallback cb;
  Ihandle *ih;

  (void)time;
  (void)msg;
  (void)hwnd;

  ih = (Ihandle*)iupTableGet(wintimer_id_table, (char*)wid);

  if (!iupObjectCheck(ih))   /* control could be destroyed before timer callback */
    return;

  cb = IupGetCallback(ih, "ACTION_CB");
  if(cb)
  {
    long long end = winTimerGetTickCount();
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

    ih->serial = (int)SetTimer(NULL, 0, time_ms, winTimerFunc);  /* minimum is 10 ms */
    iupTableSet(wintimer_id_table, (const char*)ih->serial, ih, IUPTABLE_POINTER);

    start = winTimerGetTickCount();
    iupAttribSetStrf(ih, "STARTCOUNT", "%lld", start);
  }
}

void iupdrvTimerStop(Ihandle* ih)
{
  if (ih->serial > 0)
  {
    KillTimer(NULL, ih->serial);
    iupTableRemove(wintimer_id_table, (const char*)ih->serial);
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

void iupdrvTimerInitClass(Iclass* ic)
{
  ic->Release = winTimerRelease;

  if (!wintimer_id_table)
    wintimer_id_table = iupTableCreate(IUPTABLE_POINTERINDEXED);

  {
    HMODULE kernel32 = GetModuleHandle(TEXT("KERNEL32.DLL"));
    winTimerGetTickCount64 = (ULONGLONG(WINAPI *)(void))GetProcAddress(kernel32, "GetTickCount64");
  }
}
