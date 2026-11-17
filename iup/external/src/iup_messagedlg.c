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

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"
#include "iup_str.h"
#include "iup_varg.h"


IUP_SDK_API int iupMessageDlgCustomCount(Ihandle* ih)
{
  int count = 0;

  if (!iupStrEqualNoCase(iupAttribGetStr(ih, "BUTTONS"), "CUSTOM"))
    return 0;

  while (iupAttribGetId(ih, "BUTTONTITLE", count + 1))
    count++;

  return count;
}

IUP_SDK_API int iupMessageDlgCustomDefault(Ihandle* ih)
{
  int count = iupMessageDlgCustomCount(ih);
  int def = iupAttribGetInt(ih, "BUTTONDEFAULT");
  if (def < 1 || def > count)
    def = 1;
  return def;
}

IUP_SDK_API int iupMessageDlgCustomCancel(Ihandle* ih)
{
  int count = iupMessageDlgCustomCount(ih);
  int cancel = iupAttribGetInt(ih, "BUTTONCANCEL");
  if (cancel < 1 || cancel > count)
    cancel = count;
  return cancel;
}

static int iMessageDlgCustomButton_CB(Ihandle* button)
{
  Ihandle* dlg = IupGetDialog(button);
  Ihandle* ih = (Ihandle*)iupAttribGet(dlg, "_IUP_MESSAGEDLG");
  int number = iupAttribGetInt(button, "_IUP_BUTTON_NUMBER");

  if (number == 0)
  {
    Icallback cb = IupGetCallback(ih, "HELP_CB");
    if (cb && cb(ih) == IUP_CLOSE)
      return IUP_CLOSE;
    return IUP_DEFAULT;
  }

  iupAttribSetInt(dlg, "_IUP_BUTTON_NUMBER", number);
  return IUP_CLOSE;
}

IUP_SDK_API int iupMessageDlgCustomPopup(Ihandle* ih, int x, int y)
{
  Ihandle *dlg, *button_box, *button, *default_enter = NULL, *default_esc = NULL;
  int i, count = iupMessageDlgCustomCount(ih);
  int def = iupMessageDlgCustomDefault(ih);
  int cancel = iupMessageDlgCustomCancel(ih);
  char* value = iupAttribGet(ih, "VALUE");
  int number;

  button_box = IupHbox(NULL);
  IupSetAttribute(button_box, "NORMALIZESIZE", "HORIZONTAL");
  IupSetAttribute(button_box, "MARGIN", "0x0");

  if (IupGetCallback(ih, "HELP_CB"))
  {
    button = IupButton("_@IUP_HELP");
    IupSetStrAttribute(button, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
    IupSetCallback(button, "ACTION", (Icallback)iMessageDlgCustomButton_CB);
    IupAppend(button_box, button);
  }

  IupAppend(button_box, IupFill());

  for (i = 1; i <= count; i++)
  {
    button = IupButton(iupAttribGetId(ih, "BUTTONTITLE", i));
    iupAttribSetInt(button, "_IUP_BUTTON_NUMBER", i);
    IupSetStrAttribute(button, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
    IupSetCallback(button, "ACTION", (Icallback)iMessageDlgCustomButton_CB);
    IupAppend(button_box, button);

    if (i == def)
      default_enter = button;
    if (i == cancel)
      default_esc = button;
  }

  dlg = IupDialog(IupVbox(IupLabel(value ? value : ""), button_box, NULL));
  IupSetAttribute(IupGetChild(dlg, 0), "MARGIN", "10x10");
  IupSetAttribute(IupGetChild(dlg, 0), "GAP", "10");

  IupSetStrAttribute(dlg, "TITLE", iupAttribGet(ih, "TITLE"));
  IupSetAttribute(dlg, "DIALOGFRAME", "YES");
  IupSetAttribute(dlg, "DIALOGHINT", "YES");
  IupSetAttributeHandle(dlg, "DEFAULTENTER", default_enter);
  IupSetAttributeHandle(dlg, "DEFAULTESC", default_esc);
  IupSetAttributeHandle(dlg, "STARTFOCUS", default_enter);
  IupSetStrAttribute(dlg, "PARENTDIALOG", iupAttribGet(ih, "PARENTDIALOG"));
  IupSetStrAttribute(dlg, "ICON", IupGetGlobal("ICON"));
  iupAttribSet(dlg, "_IUP_MESSAGEDLG", (char*)ih);

  IupPopup(dlg, x, y);

  number = iupAttribGetInt(dlg, "_IUP_BUTTON_NUMBER");
  if (number > 0)
    iupAttribSetInt(ih, "BUTTONRESPONSE", number);
  else
    iupMessageDlgSetCancelResponse(ih);

  IupDestroy(dlg);

  return IUP_NOERROR;
}

IUP_SDK_API void iupMessageDlgSetCancelResponse(Ihandle* ih)
{
  char* buttons = iupAttribGetStr(ih, "BUTTONS");

  if (iupStrEqualNoCase(buttons, "CUSTOM"))
  {
    if (iupMessageDlgCustomCount(ih) > 0)
      iupAttribSetInt(ih, "BUTTONRESPONSE", iupMessageDlgCustomCancel(ih));
    else
      iupAttribSet(ih, "BUTTONRESPONSE", "1");
  }
  else if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
    iupAttribSet(ih, "BUTTONRESPONSE", "3");
  else if (iupStrEqualNoCase(buttons, "OK"))
    iupAttribSet(ih, "BUTTONRESPONSE", "1");
  else
    iupAttribSet(ih, "BUTTONRESPONSE", "2");
}

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
  iupClassRegisterAttribute(ic, "BUTTONCANCEL", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "BUTTONTITLE", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  return ic;
}

IUP_API void IupMessage(const char* title, const char* msg)
{
  Ihandle* dlg = IupCreate("messagedlg");

  IupSetAttribute(dlg, "TITLE", (char*)title);
  IupSetAttribute(dlg, "VALUE", (char*)msg);
  IupSetAttribute(dlg, "PARENTDIALOG", IupGetGlobal("PARENTDIALOG"));

  IupPopup(dlg, IUP_CENTER, IUP_CENTER);
  IupDestroy(dlg);
}

IUP_API void IupMessageV(const char* title, const char* format, va_list arglist)
{
  int size;
  char* str = iupStrGetLargeMem(&size);
  vsnprintf(str, size, format, arglist);
  IupMessage(title, str);
}

IUP_API void IupMessagef(const char* title, const char* format, ...)
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

IUP_API int IupMessageAlarm(Ihandle* parent, const char* title, const char* message, const char* buttons)
{
  Ihandle* dlg = IupMessageDlg();
  char* str_message, *str_title;
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
