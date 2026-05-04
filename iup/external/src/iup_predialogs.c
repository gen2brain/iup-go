/** \file
 * \brief pre-defined dialogs
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "iup.h"

#include "iup_predialogs.h"
#include "iup_attrib.h"
#include "iup_str.h"


int iupDialogButtonOrder(void)
{
  const char* sys = IupGetGlobal("SYSTEM");
  if (sys && iupStrEqualNoCasePartial(sys, "Win"))
    return IUP_BUTTON_ORDER_OK_FIRST;
  return IUP_BUTTON_ORDER_CANCEL_FIRST;
}

static int CB_button_OK (Ihandle* ih)
{
  iupAttribSet(IupGetDialog(ih), "STATUS", "1");
  return IUP_CLOSE;
}

static int CB_button_CANCEL (Ihandle* ih)
{
  iupAttribSet(IupGetDialog(ih), "STATUS", "-1");
  return IUP_CLOSE;
}

static int CB_dblclick(Ihandle *ih, int item, char *text)
{
  (void)text;
  iupAttribSetInt(IupGetDialog(ih), "_IUP_LIST_NUMBER", item-1);
  iupAttribSet(IupGetDialog(ih), "STATUS", "1");
  return IUP_CLOSE;
}

static int CB_list (Ihandle *ih, char *text, int item, int state)
{
  (void)text;
  if (state)
    iupAttribSetInt(IupGetDialog(ih), "_IUP_LIST_NUMBER", item-1);
  return IUP_DEFAULT;
}

IUP_API int IupListDialog (int type, const char *title, int size, const char** list_str,
                   int op, int max_col, int max_lin, int* marks)
{
  Ihandle *lst, *ok, *dlg, *cancel, *dlg_box, *button_box;
  int i, bt;
  char *m=NULL;

  lst = IupList(NULL);

  for (i=0;i<size;i++)
    IupSetAttributeId(lst,"",i+1,list_str[i]);
  IupSetAttributeId(lst,"",i+1,NULL);
  IupSetAttribute(lst,"EXPAND","YES");

  ok = IupButton("_@IUP_OK", NULL);
  IupSetStrAttribute(ok, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(ok, "ACTION", (Icallback)CB_button_OK);

  cancel = IupButton("_@IUP_CANCEL", NULL);
  IupSetStrAttribute(cancel, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(cancel, "ACTION", (Icallback)CB_button_CANCEL);

  if (iupDialogButtonOrder() == IUP_BUTTON_ORDER_CANCEL_FIRST)
    button_box = IupHbox(IupFill(), cancel, ok, NULL);
  else
    button_box = IupHbox(IupFill(), ok, cancel, NULL);
  IupSetAttribute(button_box,"MARGIN","0x0");
  IupSetAttribute(button_box, "NORMALIZESIZE", "HORIZONTAL");

  dlg_box = IupVbox(
    lst,
    button_box,
    NULL);

  IupSetAttribute(dlg_box,"MARGIN","10x10");
  IupSetAttribute(dlg_box,"GAP","10");

  dlg = IupDialog(dlg_box);

  if (type == 1)
  {
    if (op<1 || op>size) op=1;
    iupAttribSetInt(dlg, "_IUP_LIST_NUMBER", op-1);
    IupSetInt(lst,"VALUE",op);
    IupSetCallback(lst, "ACTION", (Icallback)CB_list);
    IupSetCallback(lst, "DBLCLICK_CB", (Icallback)CB_dblclick);
  }
  else if ((type == 2) && (marks != NULL))
  {
    m = (char*)malloc(size + 1);
    if (m)
    {
      for (i=0;i<size;i++)
        m[i] = marks[i] ? '+' : '-';
      m[i]='\0';
      IupSetAttribute(lst,"MULTIPLE","YES");
      IupSetAttribute(lst,"VALUE",m);
      free(m);
    }
  }

  if (max_lin < 4) max_lin = 4;
  IupSetInt(lst, "VISIBLELINES", max_lin);
  IupSetInt(lst, "VISIBLECOLUMNS", max_col);

  IupSetStrAttribute(dlg,"TITLE", title);
  IupSetAttribute(dlg,"MINBOX","NO");
  IupSetAttribute(dlg,"MAXBOX","NO");
  IupSetAttributeHandle(dlg,"DEFAULTENTER", ok);
  IupSetAttributeHandle(dlg,"DEFAULTESC", cancel);
  IupSetAttribute(dlg,"PARENTDIALOG", IupGetGlobal("PARENTDIALOG"));
  IupSetAttribute(dlg,"ICON", IupGetGlobal("ICON"));

  IupPopup(dlg,IUP_CENTERPARENT,IUP_CENTERPARENT);

  if ((type == 2) && (marks != NULL))
  {
    m=IupGetAttribute(lst,"VALUE");
    for (i=0;i<size;i++)
      marks[i] = (m[i] == '+');
  }

  bt = IupGetInt(dlg, "STATUS");
  if (type == 1)
  {
    if (bt == 1)
      bt = iupAttribGetInt(dlg, "_IUP_LIST_NUMBER");
    else
      bt = -1;
  }
  else
  {
    if (bt != 1)
      bt = -1;
  }

  IupDestroy(dlg);

  return bt;
}

static int iAlarmButtonAction_CB(Ihandle *ih)
{
  iupAttribSet(IupGetDialog(ih), "_IUP_BUTTON_NUMBER", iupAttribGet(ih, "_IUP_BUTTON_NUMBER"));
  return IUP_CLOSE;
}

IUP_API int IupAlarm(const char *title, const char *msg, const char *b1, const char *b2, const char *b3)
{
  Ihandle  *dlg, *dlg_box, *button_box, *buttons[3], *default_esc, *default_enter;
  int i, count, bt;

  msg = msg? msg: "";

  if (b1 == NULL)
    return 0;

  button_box = IupHbox(NULL);
  IupSetAttribute(button_box, "NORMALIZESIZE", "HORIZONTAL");
  IupSetAttribute(button_box,"MARGIN","0x0");
  IupAppend(button_box, IupFill());

  count = 1;
  buttons[0] = IupButton(b1, NULL);
  iupAttribSet(buttons[0], "_IUP_BUTTON_NUMBER", "1");
  IupSetStrAttribute(buttons[0], "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(buttons[0], "ACTION", (Icallback)iAlarmButtonAction_CB);

  if (b2 != NULL)
  {
    buttons[count] = IupButton(b2, NULL);
    iupAttribSet(buttons[count], "_IUP_BUTTON_NUMBER", "2");
    IupSetStrAttribute(buttons[count], "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
    IupSetCallback(buttons[count], "ACTION", (Icallback)iAlarmButtonAction_CB);
    count++;
  }
  if (b3 != NULL)
  {
    buttons[count] = IupButton(b3, NULL);
    iupAttribSet(buttons[count], "_IUP_BUTTON_NUMBER", "3");
    IupSetStrAttribute(buttons[count], "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
    IupSetCallback(buttons[count], "ACTION", (Icallback)iAlarmButtonAction_CB);
    count++;
  }

  default_enter = buttons[0];
  default_esc = buttons[count - 1];

  if (iupDialogButtonOrder() == IUP_BUTTON_ORDER_CANCEL_FIRST)
    for (i = count - 1; i >= 0; i--) IupAppend(button_box, buttons[i]);
  else
    for (i = 0; i < count; i++) IupAppend(button_box, buttons[i]);

  IupAppend(button_box, IupFill());

  dlg_box = IupVbox(
    IupLabel(msg),
    IupSetAttributes(IupLabel(NULL), "SEPARATOR=HORIZONTAL"),
    button_box,
    NULL);

  IupSetAttribute(dlg_box,"MARGIN","10x10");
  IupSetAttribute(dlg_box,"GAP","10");

  dlg = IupDialog(dlg_box);

  IupSetStrAttribute(dlg,"TITLE", title);
  IupSetAttribute(dlg,"DIALOGFRAME","YES");
  IupSetAttribute(dlg,"DIALOGHINT","YES");
  IupSetAttributeHandle(dlg,"DEFAULTENTER", default_enter);
  IupSetAttributeHandle(dlg,"DEFAULTESC", default_esc);
  IupSetAttribute(dlg,"PARENTDIALOG", IupGetGlobal("PARENTDIALOG"));
  IupSetAttribute(dlg,"ICON", IupGetGlobal("ICON"));

  IupPopup(dlg,IUP_CENTERPARENT,IUP_CENTERPARENT);

  bt = iupAttribGetInt(dlg, "_IUP_BUTTON_NUMBER");

  IupDestroy(dlg);

  return bt;
}

IUP_API int IupGetFile(char* filename)
{
  Ihandle *dlg = 0;
  int ret;
  char filter[4096] = "*.*";
  static char dir[4096] = "";  /* static will make the dir persist from one call to another if not defined */

  if (!filename) return -1;

  dlg = IupFileDlg();

  iupStrFileNameSplit(filename, dir, sizeof(dir), filter, sizeof(filter));

  IupSetAttribute(dlg, "FILTER", filter);
  IupSetAttribute(dlg, "DIRECTORY", dir);
  IupSetAttribute(dlg, "ALLOWNEW", "YES");
  IupSetAttribute(dlg, "NOCHANGEDIR", "YES");
  IupSetAttribute(dlg, "PARENTDIALOG", IupGetGlobal("PARENTDIALOG"));
  IupSetAttribute(dlg, "ICON", IupGetGlobal("ICON"));

  IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

  ret = IupGetInt(dlg, "STATUS");
  if (ret != -1)
  {
    char* value = IupGetAttribute(dlg, "VALUE");
    if (value)
    {
      iupStrCopyN(filename, 4096, value);
      iupStrFileNameSplit(filename, dir, sizeof(dir), NULL, 0);
    }
  }

  IupDestroy(dlg);

  return ret;
}

IUP_API int IupGetText(const char* title, char* text, int maxsize)
{
  Ihandle *ok, *cancel = NULL, *multi_text, *button_box, *dlg_box, *dlg;
  int bt;

  if (maxsize == 0)
    maxsize = (int)strlen(text);

  multi_text = IupMultiLine(NULL);
  IupSetAttribute(multi_text,"EXPAND", "YES");
  IupSetAttribute(multi_text,"VALUE", text);
  IupSetAttribute(multi_text,"FONT", "Courier, 12");
  IupSetAttribute(multi_text, "VISIBLELINES", "10");
  IupSetAttribute(multi_text, "VISIBLECOLUMNS", "50");
  if (maxsize <= 0) IupSetAttribute(multi_text, "READONLY", "YES");

  ok = IupButton("_@IUP_OK", NULL);
  IupSetStrAttribute(ok, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(ok, "ACTION", (Icallback)CB_button_OK);

  if (maxsize > 0)
  {
    cancel = IupButton("_@IUP_CANCEL", NULL);
    IupSetStrAttribute(cancel, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
    IupSetCallback(cancel, "ACTION", (Icallback)CB_button_CANCEL);
  }

  if (cancel && iupDialogButtonOrder() == IUP_BUTTON_ORDER_CANCEL_FIRST)
    button_box = IupHbox(IupFill(), cancel, ok, NULL);
  else
    button_box = IupHbox(IupFill(), ok, cancel, NULL);
  IupSetAttribute(button_box,"MARGIN","0x0");
  IupSetAttribute(button_box, "NORMALIZESIZE", "HORIZONTAL");

  dlg_box = IupVbox(
    multi_text,
    button_box,
    NULL);

  IupSetAttribute(dlg_box,"MARGIN","10x10");
  IupSetAttribute(dlg_box,"GAP","10");

  dlg = IupDialog (dlg_box);

  IupSetStrAttribute(dlg,"TITLE", title);
  IupSetAttribute(dlg,"MINBOX","NO");
  IupSetAttribute(dlg,"MAXBOX","NO");
  IupSetAttributeHandle(dlg,"DEFAULTENTER", ok);
  if (cancel) IupSetAttributeHandle(dlg,"DEFAULTESC", cancel);
  else IupSetAttributeHandle(dlg, "DEFAULTESC", ok);
  IupSetAttribute(dlg,"PARENTDIALOG", IupGetGlobal("PARENTDIALOG"));
  IupSetAttribute(dlg,"ICON", IupGetGlobal("ICON"));

  IupMap(dlg);

  IupSetAttribute(multi_text, "VISIBLELINES", NULL);
  IupSetAttribute(multi_text, "VISIBLECOLUMNS", NULL);

  IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

  bt = IupGetInt(dlg, "STATUS");
  if (bt==1 && maxsize > 0)
    iupStrCopyN(text, maxsize, IupGetAttribute(multi_text, "VALUE"));
  else
    bt = 0; /* return 0 instead of -1 */

  IupDestroy(dlg);
  return bt;
}

IUP_API int IupGetColor(int x, int y, unsigned char *r, unsigned char *g, unsigned char *b)
{
  int ret;
  Ihandle* dlg = IupColorDlg();

  IupSetStrAttribute(dlg, "TITLE",  "_@IUP_GETCOLOR");
  IupSetfAttribute(dlg, "VALUE", "%d %d %d", *r, *g, *b);
  IupSetAttribute(dlg, "PARENTDIALOG", IupGetGlobal("PARENTDIALOG"));
  IupSetAttribute(dlg, "ICON", IupGetGlobal("ICON"));

  IupPopup(dlg, x, y);

  ret = IupGetInt(dlg, "STATUS");
  if (ret)
    iupStrToRGB(IupGetAttribute(dlg, "VALUE"), r, g, b);

  IupDestroy(dlg);

  return ret;
}
