/** \file
 * \brief IupFileDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"


IUP_API Ihandle* IupFileDlg(void)
{
  return IupCreate("filedlg");
}

Iclass* iupFileDlgNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("dialog"));

  ic->name = "filedlg";
  ic->cons = "FileDlg";
  ic->nativetype = IUP_TYPEDIALOG;
  ic->is_interactive = 1;

  iupClassRegisterCallback(ic, "FILE_CB", "ss");

  ic->New = iupFileDlgNewClass;

  /* reset not used native dialog methods */
  ic->parent->LayoutUpdate = NULL;
  ic->parent->SetChildrenPosition = NULL;
  ic->parent->Map = NULL;
  ic->parent->UnMap = NULL;

  iupdrvFileDlgInitClass(ic);

  /* only the default value */
  iupClassRegisterAttribute(ic, "NOCHANGEDIR", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIALOGTYPE", NULL, NULL, IUPAF_SAMEASSYSTEM, "OPEN", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PREVIEWDC", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_READONLY|IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "PREVIEWWIDTH", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "PREVIEWHEIGHT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_READONLY);

  iupClassRegisterAttribute(ic, "ALLOWNEW", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIRECTORY", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NOOVERWRITEPROMPT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWHIDDEN", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWPREVIEW", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MULTIVALUEPATH", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXTDEFAULT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MULTIPLEFILES", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FILEEXIST", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "STATUS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "VALUE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_READONLY);

  return ic;
}
