/** \file
 * \brief Stub System Tray Driver (no-op implementation)
 *
 * See Copyright Notice in "iup.h"
 */

#include "iup.h"
#include "iup_class.h"

void iupdrvTrayInitClass(Iclass* ic)
{
  (void)ic;
}

int iupdrvTraySetVisible(Ihandle* ih, int visible)
{
  (void)ih;
  (void)visible;
  return 0;
}

int iupdrvTraySetTip(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

int iupdrvTraySetImage(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

int iupdrvTraySetMenu(Ihandle* ih, Ihandle* menu)
{
  (void)ih;
  (void)menu;
  return 0;
}

void iupdrvTrayDestroy(Ihandle* ih)
{
  (void)ih;
}

int iupdrvTrayIsAvailable(void)
{
  return 0;
}
