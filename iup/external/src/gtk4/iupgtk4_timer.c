/** \file
 * \brief Timer for the GTK4 Driver.
 *
 * See Copyright Notice in "iup.h"
 */


#include <stdio.h>

#include <gtk/gtk.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_timer.h"


static gboolean gtk4TimerProc(gpointer data)
{
  Ihandle* ih = (Ihandle*)data;
  Icallback cb;

  if (!iupObjectCheck(ih))   /* control could be destroyed before timer callback */
    return FALSE;

  cb = IupGetCallback(ih, "ACTION_CB");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }

  return TRUE;
}

IUP_SDK_API void iupdrvTimerRun(Ihandle* ih)
{
  unsigned int time_ms;

  if (ih->serial > 0) /* timer already started */
    return;

  time_ms = iupAttribGetInt(ih, "TIME");
  if (time_ms > 0)
  {
    if (iupAttribGetBoolean(ih, "PRIORITY_HIGH"))
      ih->serial = g_timeout_add_full(G_PRIORITY_HIGH, time_ms, gtk4TimerProc, (gpointer)ih, NULL);
    else
      ih->serial = g_timeout_add(time_ms, gtk4TimerProc, (gpointer)ih);
  }
}

IUP_SDK_API void iupdrvTimerStop(Ihandle* ih)
{
  if (ih->serial > 0)
  {
    g_source_remove(ih->serial);
    ih->serial = -1;
  }
}

IUP_SDK_API void iupdrvTimerInitClass(Iclass* ic)
{
  (void)ic;
}
