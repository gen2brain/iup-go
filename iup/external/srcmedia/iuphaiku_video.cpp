/** \file
 * \brief Video control, Haiku screen saver
 *
 * See Copyright Notice in "iup.h"
 */

#include <InterfaceDefs.h>
#include <WindowScreen.h>

#include "iup.h"
#include "iupcbs.h"
#include "iup_video.h"

static Ihandle* haiku_video_timer = NULL;

static int haikuVideoNudge(Ihandle* timer)
{
  BPoint where;
  uint32 buttons = 0;
  (void)timer;
  if (get_mouse(&where, &buttons) == B_OK && buttons == 0)
    set_mouse_position((int32)where.x, (int32)where.y);
  return IUP_DEFAULT;
}

extern "C" void iupdrvVideoInhibitScreenSaver(int inhibit)
{
  if (inhibit && !haiku_video_timer)
  {
    haiku_video_timer = IupTimer();
    IupSetInt(haiku_video_timer, "TIME", 10000);
    IupSetCallback(haiku_video_timer, "ACTION_CB", (Icallback)haikuVideoNudge);
    IupSetAttribute(haiku_video_timer, "RUN", "YES");
  }
  else if (!inhibit && haiku_video_timer)
  {
    IupDestroy(haiku_video_timer);
    haiku_video_timer = NULL;
  }
}
