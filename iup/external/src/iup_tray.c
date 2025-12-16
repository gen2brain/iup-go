/** \file
 * \brief Tray Control - System Tray Icon
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_tray.h"
#include "iup_attrib.h"

static int iTraySetVisibleAttrib(Ihandle* ih, const char* value)
{
  return iupdrvTraySetVisible(ih, iupStrBoolean(value));
}

static int iTraySetImageAttrib(Ihandle* ih, const char* value)
{
  return iupdrvTraySetImage(ih, value);
}

static int iTraySetTipAttrib(Ihandle* ih, const char* value)
{
  return iupdrvTraySetTip(ih, value);
}

static int iTraySetMenuAttrib(Ihandle* ih, const char* value)
{
  Ihandle* menu_ih = NULL;
  if (value && value[0])
    menu_ih = IupGetHandle(value);
  return iupdrvTraySetMenu(ih, menu_ih);
}

static void iTrayDestroyMethod(Ihandle* ih)
{
  iupdrvTrayDestroy(ih);
}

/******************************************************************************/

IUP_API Ihandle* IupTray(void)
{
  return IupCreate("tray");
}

Iclass* iupTrayNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "tray";
  ic->format = NULL;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupTrayNewClass;
  ic->Destroy = iTrayDestroyMethod;

  iupClassRegisterCallback(ic, "TRAYCLICK_CB", "iii");

  iupClassRegisterAttribute(ic, "VISIBLE", NULL, iTraySetVisibleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, iTraySetImageAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIP", NULL, iTraySetTipAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MENU", NULL, iTraySetMenuAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupdrvTrayInitClass(ic);

  return ic;
}
