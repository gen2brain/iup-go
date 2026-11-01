/** \file
 * \brief IupShareDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_register.h"
#include "iup_stdcontrols.h"


IUP_API Ihandle* IupShareDlg(void)
{
  return IupCreate("sharedlg");
}

static int iShareDlgPopupNotSupported(Ihandle* ih, int x, int y)
{
  (void)x;
  (void)y;
  iupAttribSet(ih, "STATUS", "-1");
  return IUP_ERROR;
}

Iclass* iupShareDlgNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("dialog"));

  ic->name = "sharedlg";
  ic->cons = "ShareDlg";
  ic->nativetype = IUP_TYPEDIALOG;
  ic->is_interactive = 1;

  ic->New = iupShareDlgNewClass;

  ic->parent->LayoutUpdate = NULL;
  ic->parent->SetChildrenPosition = NULL;
  ic->parent->Map = NULL;
  ic->parent->UnMap = NULL;

  iupdrvShareDlgInitClass(ic);

  if (!ic->DlgPopup)
    ic->DlgPopup = iShareDlgPopupNotSupported;

  iupClassRegisterAttribute(ic, "TEXT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "URL", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATUS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_READONLY);

  return ic;
}
