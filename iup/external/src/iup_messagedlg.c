/** \file
 * \brief IupMessageDlg class
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
#include "iup_stdcontrols.h"
#include "iup_register.h"
#include "iup_str.h"
#include "iup_varg.h"


IUP_API Ihandle* IupMessageDlg(void)
{
  return IupCreate("messagedlg");
}

Iclass* iupMessageDlgNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("dialog"));

  ic->name = "messagedlg";
  ic->cons = "MessageDlg";
  ic->nativetype = IUP_TYPEDIALOG;
  ic->is_interactive = 1;

  ic->New = iupMessageDlgNewClass;

  /* reset not used native dialog methods */
  ic->parent->LayoutUpdate = NULL;
  ic->parent->SetChildrenPosition = NULL;
  ic->parent->Map = NULL;
  ic->parent->UnMap = NULL;

  iupdrvMessageDlgInitClass(ic);

  /* only the default values */
  iupClassRegisterAttribute(ic, "DIALOGTYPE", NULL, NULL, IUPAF_SAMEASSYSTEM, "MESSAGE", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BUTTONS", NULL, NULL, IUPAF_SAMEASSYSTEM, "OK", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BUTTONDEFAULT", NULL, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BUTTONRESPONSE", NULL, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  return ic;
}

IUP_API void IupMessage(const char* title, const char* message)
{
  Ihandle* dlg = IupCreate("messagedlg");

  IupSetAttribute(dlg, "TITLE", (char*)title);
  IupSetAttribute(dlg, "VALUE", (char*)message);
  IupSetAttribute(dlg, "PARENTDIALOG", IupGetGlobal("PARENTDIALOG"));

  IupPopup(dlg, IUP_CENTER, IUP_CENTER);
  IupDestroy(dlg);
}

IUP_API void IupMessageV(const char *title, const char *format, va_list arglist)
{
  int size;
  char* str = iupStrGetLargeMem(&size);
  vsnprintf(str, size, format, arglist);
  IupMessage(title, str);
}

IUP_API void IupMessagef(const char *title, const char *format, ...)
{
  va_list arglist;
  va_start(arglist, format);
  IupMessageV(title, format, arglist);
  va_end(arglist);
}

IUP_API void IupMessageError(Ihandle* parent, const char* message)
{
  Ihandle* dlg = IupMessageDlg();
  char* title = NULL, *str_message;

  if (!parent)
  {
    IupSetStrAttribute(dlg, "PARENTDIALOG", IupGetGlobal("PARENTDIALOG"));
    parent = IupGetAttributeHandle(dlg, "PARENTDIALOG");
  }
  else
    IupSetAttributeHandle(dlg, "PARENTDIALOG", parent);

  if (parent)
    title = IupGetAttribute(parent, "TITLE");

  if (!title)
    title = "_@IUP_ERROR";

  IupSetStrAttribute(dlg, "TITLE", title);
  IupSetAttribute(dlg, "DIALOGTYPE", "ERROR");
  IupSetAttribute(dlg, "BUTTONS", "OK");

  str_message = IupGetLanguageString(message);
  if (!str_message)
    str_message = (char*)message;
  IupStoreAttribute(dlg, "VALUE", str_message);

  IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

  IupDestroy(dlg);
}

IUP_API int IupMessageAlarm(Ihandle* parent, const char* title, const char *message, const char *buttons)
{
  Ihandle* dlg = IupMessageDlg();
  char *str_message, *str_title;
  int ret;

  if (!parent)
    IupSetStrAttribute(dlg, "PARENTDIALOG", IupGetGlobal("PARENTDIALOG"));
  else
    IupSetAttributeHandle(dlg, "PARENTDIALOG", parent);

  if (!title)
    title = "IUP_ATTENTION";

  str_title = IupGetLanguageString(title);
  if (!str_title)
    str_title = (char*)title;
  IupStoreAttribute(dlg, "TITLE", str_title);

  str_message = IupGetLanguageString(message);
  if (!str_message)
    str_message = (char*)message;
  IupStoreAttribute(dlg, "VALUE", str_message);

  IupSetAttribute(dlg, "DIALOGTYPE", "QUESTION");
  IupSetStrAttribute(dlg, "BUTTONS", buttons);

  IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

  ret = IupGetInt(dlg, "BUTTONRESPONSE");

  IupDestroy(dlg);

  return ret;
}
