/** \file
 * \brief Keep the display awake
 *
 * See Copyright Notice in "iup.h"
 */

#include <InterfaceDefs.h>
#include <WindowScreen.h>

#include "iup.h"
#include "iupcbs.h"
#include "iup_drvinfo.h"

static Ihandle* haiku_keepawake_timer = NULL;

static int haikuKeepAwakeNudge(Ihandle* timer)
{
  BPoint where;
  uint32 buttons = 0;
  (void)timer;
  if (get_mouse(&where, &buttons) == B_OK && buttons == 0)
    set_mouse_position((int32)where.x, (int32)where.y);
  return IUP_DEFAULT;
}

extern "C" IUP_SDK_API void iupdrvSetKeepAwake(int inhibit)
{
  if (inhibit && !haiku_keepawake_timer)
  {
    haiku_keepawake_timer = IupTimer();
    IupSetInt(haiku_keepawake_timer, "TIME", 10000);
    IupSetCallback(haiku_keepawake_timer, "ACTION_CB", (Icallback)haikuKeepAwakeNudge);
    IupSetAttribute(haiku_keepawake_timer, "RUN", "YES");
  }
  else if (!inhibit && haiku_keepawake_timer)
  {
    IupDestroy(haiku_keepawake_timer);
    haiku_keepawake_timer = NULL;
  }
}
