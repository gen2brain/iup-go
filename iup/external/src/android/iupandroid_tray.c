/** \file
 * \brief Tray Icon Control
 *
 * See Copyright Notice in "iup.h"
 */

#include "iup.h"

#include "iup_object.h"
#include "iup_tray.h"


/* Android has no system tray, but the core still links against these symbols. */

IUP_SDK_API void iupdrvTrayInitClass(Iclass* ic)
{
  (void)ic;
}

IUP_SDK_API int iupdrvTraySetVisible(Ihandle* ih, int visible)
{
  (void)ih;
  (void)visible;
  return 0;
}

IUP_SDK_API int iupdrvTraySetImage(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

IUP_SDK_API int iupdrvTraySetTip(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

IUP_SDK_API int iupdrvTraySetMenu(Ihandle* ih, Ihandle* menu)
{
  (void)ih;
  (void)menu;
  return 0;
}

IUP_SDK_API void iupdrvTrayDestroy(Ihandle* ih)
{
  (void)ih;
}

IUP_SDK_API int iupdrvTrayIsAvailable(void)
{
  return 0;
}
