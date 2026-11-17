/** \file
 * \brief Motif IupMessageDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Xm/Xm.h>
#include <Xm/MwmUtil.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/Protocols.h>
#include <X11/keysym.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"

#include "iupmot_drv.h"


static void motMessageDlgDeleteWindowCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Ihandle* ih = (Ihandle*)client_data;
  if (!ih) return;
  (void)call_data;
  (void)w;
  iupMessageDlgSetCancelResponse(ih);
  iupAttribSet(ih, "_IUP_WM_DELETE", "1");
}

static void motMessageDlgCallback(Widget w, Ihandle* ih, XmAnyCallbackStruct* call_data)
{
  (void)w;
  if (call_data->reason == XmCR_OK)
    iupAttribSet(ih, "BUTTONRESPONSE", "1");
  else if (call_data->reason == XmCR_CANCEL)
    iupMessageDlgSetCancelResponse(ih);
}

static void motMessageDlgNoCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Ihandle* ih = (Ihandle*)client_data;
  (void)call_data;
  (void)w;
  iupAttribSet(ih, "BUTTONRESPONSE", "2");
}

typedef struct _ImotMessageDlgCustom
{
  Ihandle* ih;
  int count;
  char mnemonics[1];
} ImotMessageDlgCustom;

static void motMessageDlgCustomCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Ihandle* ih = (Ihandle*)client_data;
  XtPointer number = NULL;
  (void)call_data;
  XtVaGetValues(w, XmNuserData, &number, NULL);
  iupAttribSetInt(ih, "BUTTONRESPONSE", (int)(intptr_t)number);
}

static void motMessageDlgCustomKeyEvent(Widget w, XtPointer client_data, XEvent* evt, Boolean* cont)
{
  ImotMessageDlgCustom* custom = (ImotMessageDlgCustom*)client_data;
  KeySym keysym = XLookupKeysym(&evt->xkey, 0);
  (void)w;

  if (keysym == XK_Escape)
  {
    iupMessageDlgSetCancelResponse(custom->ih);
    *cont = False;
  }
  else if ((evt->xkey.state & Mod1Mask) && keysym < 128)
  {
    int i;
    for (i = 0; i < custom->count; i++)
    {
      if (custom->mnemonics[i] && iup_tolower(custom->mnemonics[i]) == iup_tolower((int)keysym))
      {
        iupAttribSetInt(custom->ih, "BUTTONRESPONSE", i + 1);
        *cont = False;
        break;
      }
    }
  }
}

static void motMessageDlgHelpCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Ihandle* ih = (Ihandle*)client_data;
  Icallback cb = (Icallback)IupGetCallback(ih, "HELP_CB");
  if (cb && cb(ih) == IUP_CLOSE)
    iupMessageDlgSetCancelResponse(ih);

  (void)call_data;
  (void)w;
}

static int motMessageDlgPopup(Ihandle* ih, int x, int y)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  Widget msgbox, dialog, no_button = NULL;
  int style = XmDIALOG_FULL_APPLICATION_MODAL;
  int type = XmDIALOG_MESSAGE;
  int num_but = 2, button_def;
  int custom_count = iupMessageDlgCustomCount(ih);
  ImotMessageDlgCustom* custom = NULL;
  char* value;

  iupAttribSetInt(ih, "_IUPDLG_X", x);   /* used in iupDialogUpdatePosition */
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  if (parent)
  {
    msgbox = XmCreateMessageDialog(parent, "messagedialog", NULL, 0);
    dialog = XtParent(msgbox);
  }
  else
  {
    dialog = XtAppCreateShell(NULL, "messagedialog", topLevelShellWidgetClass, iupmot_display, NULL, 0);
    msgbox = XmCreateMessageBox(dialog, "messagebox", NULL, 0);
    style = XmDIALOG_MODELESS;
    XtVaSetValues(dialog,
      XmNmwmInputMode, MWM_INPUT_FULL_APPLICATION_MODAL,
      XmNmappedWhenManaged, False,
      XmNsaveUnder, True,
      NULL);
  }
  if (!msgbox)
    return IUP_NOERROR;

  value = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(value, "ERROR"))
    type = XmDIALOG_ERROR;
  else if (iupStrEqualNoCase(value, "WARNING"))
    type = XmDIALOG_WARNING;
  else if (iupStrEqualNoCase(value, "INFORMATION"))
    type = XmDIALOG_INFORMATION;
  else if (iupStrEqualNoCase(value, "QUESTION"))
    type = XmDIALOG_QUESTION;

  value = iupAttribGet(ih, "TITLE");
  if (value)
    iupmotSetXmString(msgbox, XmNdialogTitle, value);
  else
  {
    if (parent)
    {
      XmString title;
      XtVaGetValues(parent, XmNdialogTitle, &title, NULL);
      XtVaSetValues(msgbox, XmNdialogTitle, title, NULL);
    }
  }

  value = iupAttribGet(ih, "VALUE");
  if (value)
    iupmotSetXmString(msgbox, XmNmessageString, value);

  XtVaSetValues(msgbox,
                XmNdialogType, type,
                XmNdialogStyle, style,
                XmNautoUnmanage, False,
                XmNnoResize, True,
                NULL);

  value = iupAttribGetStr(ih, "BUTTONS");
  if (custom_count > 0)
  {
    int i, def = iupMessageDlgCustomDefault(ih);

    custom = (ImotMessageDlgCustom*)calloc(1, sizeof(ImotMessageDlgCustom) + custom_count);
    custom->ih = ih;
    custom->count = custom_count;

    XtUnmanageChild(XtNameToWidget(msgbox, "OK"));
    XtUnmanageChild(XtNameToWidget(msgbox, "Cancel"));

    for (i = 1; i <= custom_count; i++)
    {
      Widget button = XmCreatePushButton(msgbox, "custom", NULL, 0);
      const char* title = iupAttribGetId(ih, "BUTTONTITLE", i);
      char c = 0;
      char* str = iupStrProcessMnemonic(title, &c, -1);

      iupmotSetXmString(button, XmNlabelString, str);
      if (str != title)
      {
        XtVaSetValues(button, XmNmnemonic, iupmotKeyCharToKeySym(c), NULL);
        custom->mnemonics[i - 1] = c;
        free(str);
      }

      XtVaSetValues(button, XmNuserData, (XtPointer)(intptr_t)i, NULL);
      XtAddCallback(button, XmNactivateCallback, motMessageDlgCustomCallback, (XtPointer)ih);
      XtAddEventHandler(button, KeyPressMask, False, motMessageDlgCustomKeyEvent, (XtPointer)custom);
      XtManageChild(button);

      if (i == def)
        XtVaSetValues(msgbox, XmNdefaultButton, button, XmNinitialFocus, button, NULL);
    }
  }
  else if (iupStrEqualNoCase(value, "OK"))
  {
    XtUnmanageChild(XtNameToWidget(msgbox, "Cancel"));
    num_but = 1;
  }
  else if (iupStrEqualNoCase(value, "YESNO"))
  {
    iupmotSetXmString(msgbox, XmNokLabelString, IupGetLanguageString("IUP_YES"));
    iupmotSetXmString(msgbox, XmNcancelLabelString, IupGetLanguageString("IUP_NO"));
  }
  else if (iupStrEqualNoCase(value, "RETRYCANCEL"))
    iupmotSetXmString(msgbox, XmNokLabelString, IupGetLanguageString("IUP_RETRY"));
  else if (iupStrEqualNoCase(value, "YESNOCANCEL"))
  {
    iupmotSetXmString(msgbox, XmNokLabelString, IupGetLanguageString("IUP_YES"));
    no_button = XmCreatePushButton(msgbox, "no", NULL, 0);
    iupmotSetXmString(no_button, XmNlabelString, IupGetLanguageString("IUP_NO"));
    XtAddCallback(no_button, XmNactivateCallback, motMessageDlgNoCallback, (XtPointer)ih);
    XtManageChild(no_button);
    num_but = 3;
  }

  if (!IupGetCallback(ih, "HELP_CB"))
    XtUnmanageChild(XtNameToWidget(msgbox, "Help"));

  button_def = iupAttribGetInt(ih, "BUTTONDEFAULT");
  if (custom_count == 0)
  {
    if (no_button && button_def == 2)
      XtVaSetValues(msgbox, XmNdefaultButton, no_button, XmNinitialFocus, no_button, NULL);
    else if (num_but > 1 && button_def == num_but)
      XtVaSetValues(msgbox, XmNdefaultButtonType, XmDIALOG_CANCEL_BUTTON, NULL);
    else
      XtVaSetValues(msgbox, XmNdefaultButtonType, XmDIALOG_OK_BUTTON, NULL);
  }

  XtAddCallback(msgbox, XmNokCallback, (XtCallbackProc)motMessageDlgCallback, (XtPointer)ih);
  XtAddCallback(msgbox, XmNcancelCallback, (XtCallbackProc)motMessageDlgCallback, (XtPointer)ih);
  XtAddCallback(msgbox, XmNhelpCallback, (XtCallbackProc)motMessageDlgHelpCallback, (XtPointer)ih);

  XmAddWMProtocolCallback(dialog, iupmot_wm_deletewindow, motMessageDlgDeleteWindowCallback, (XtPointer)ih);
  XtManageChild(msgbox);

  XtRealizeWidget(dialog);
  ih->handle = dialog;
  iupDialogUpdatePosition(ih);
  ih->handle = NULL;  /* reset handle */

  if (style == XmDIALOG_MODELESS)
    XtPopup(dialog, XtGrabExclusive);

  /* while the user hasn't provided an answer, simulate main loop.
  ** The answer changes as soon as the user selects one of the
  ** buttons and the callback routine changes its value. */
  iupAttribSet(ih, "BUTTONRESPONSE", NULL);
  while (iupAttribGet(ih, "BUTTONRESPONSE") == NULL)
    XtAppProcessEvent(iupmot_appcontext, XtIMAll);

  if (!iupAttribGet(ih, "_IUP_WM_DELETE"))
  {
    XtUnmanageChild(msgbox);

    if (style == XmDIALOG_MODELESS)
    {
      XtPopdown(dialog);
      XtDestroyWidget(dialog);
    }
  }

  free(custom);

  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = motMessageDlgPopup;
}
