/** \file
 * \brief WebAssembly IupMessageDlg
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_stdcontrols.h"
#include "iup_predialogs.h"


static int wasmMessageButton_CB(Ihandle* btn)
{
  iupAttribSet(IupGetDialog(btn), "_IUP_BUTTON_NUMBER", iupAttribGet(btn, "_IUP_BUTTON_NUMBER"));
  return IUP_CLOSE;
}

/* No browser message box returns 3 states, so build a real IUP dialog. */
static int wasmMessageDlgPopup(Ihandle* ih, int x, int y)
{
  char* value = iupAttribGet(ih, "VALUE");
  char* title = iupAttribGet(ih, "TITLE");
  char* buttons = iupAttribGetStr(ih, "BUTTONS");
  const char* labels[3];
  Ihandle *dlg, *dlg_box, *button_box, *bobj[3];
  int count = 1, i, bt, def;
  (void)x;
  (void)y;

  if (!value) value = (char*)"";

  if (iupStrEqualNoCase(buttons, "OKCANCEL")) { labels[0] = "_@IUP_OK"; labels[1] = "_@IUP_CANCEL"; count = 2; }
  else if (iupStrEqualNoCase(buttons, "RETRYCANCEL")) { labels[0] = "_@IUP_RETRY"; labels[1] = "_@IUP_CANCEL"; count = 2; }
  else if (iupStrEqualNoCase(buttons, "YESNO")) { labels[0] = "_@IUP_YES"; labels[1] = "_@IUP_NO"; count = 2; }
  else if (iupStrEqualNoCase(buttons, "YESNOCANCEL")) { labels[0] = "_@IUP_YES"; labels[1] = "_@IUP_NO"; labels[2] = "_@IUP_CANCEL"; count = 3; }
  else { labels[0] = "_@IUP_OK"; count = 1; }

  button_box = IupHbox(NULL);
  IupSetAttribute(button_box, "NORMALIZESIZE", "HORIZONTAL");
  IupSetAttribute(button_box, "MARGIN", "0x0");
  IupAppend(button_box, IupFill());

  for (i = 0; i < count; i++)
  {
    char num[2];
    num[0] = (char)('1' + i);
    num[1] = 0;
    bobj[i] = IupButton(labels[i]);
    iupAttribSetStr(bobj[i], "_IUP_BUTTON_NUMBER", num);
    IupSetStrAttribute(bobj[i], "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
    IupSetCallback(bobj[i], "ACTION", (Icallback)wasmMessageButton_CB);
  }

  if (iupDialogButtonOrder() == IUP_BUTTON_ORDER_CANCEL_FIRST)
    for (i = count - 1; i >= 0; i--) IupAppend(button_box, bobj[i]);
  else
    for (i = 0; i < count; i++) IupAppend(button_box, bobj[i]);

  IupAppend(button_box, IupFill());

  dlg_box = IupVbox(
    IupLabel(value),
    IupSetAttributes(IupLabel(NULL), "SEPARATOR=HORIZONTAL"),
    button_box,
    NULL);
  IupSetAttribute(dlg_box, "MARGIN", "10x10");
  IupSetAttribute(dlg_box, "GAP", "10");

  def = iupAttribGetInt(ih, "BUTTONDEFAULT");
  if (def < 1 || def > count)
    def = 1;

  dlg = IupDialog(dlg_box);
  IupSetStrAttribute(dlg, "TITLE", title);
  IupSetAttribute(dlg, "DIALOGFRAME", "YES");
  IupSetAttributeHandle(dlg, "DEFAULTENTER", bobj[def - 1]);
  IupSetAttributeHandle(dlg, "DEFAULTESC", bobj[count - 1]);
  IupSetAttribute(dlg, "PARENTDIALOG", IupGetGlobal("PARENTDIALOG"));

  IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

  bt = iupAttribGetInt(dlg, "_IUP_BUTTON_NUMBER");
  if (bt == 0)
    bt = count;
  IupDestroy(dlg);

  iupAttribSetInt(ih, "BUTTONRESPONSE", bt);
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = wasmMessageDlgPopup;
}
