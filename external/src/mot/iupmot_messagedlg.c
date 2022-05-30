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
#include <Xm/Protocols.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvinfo.h"
#include "iup_dialog.h"
#include "iup_strmessage.h"

#include "iupmot_drv.h"


static void motMessageDlgDeleteWindowCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Ihandle *ih = (Ihandle*)client_data;
  if (!ih) return;
  (void)call_data;
  (void)w;
  if (iupStrEqualNoCase(iupAttribGetStr(ih, "BUTTONS"), "OK"))
    iupAttribSet(ih, "BUTTONRESPONSE", "1");
  else
    iupAttribSet(ih, "BUTTONRESPONSE", "2");
  iupAttribSet(ih, "_IUP_WM_DELETE", "1");
}

static void motMessageDlgCallback(Widget w, Ihandle* ih, XmAnyCallbackStruct* call_data)
{
  (void)w;
  if (call_data->reason == XmCR_OK)
    iupAttribSet(ih, "BUTTONRESPONSE", "1");
  else if (call_data->reason == XmCR_CANCEL)
    iupAttribSet(ih, "BUTTONRESPONSE", "2");
}

static void motMessageDlgHelpCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Ihandle *ih = (Ihandle*)client_data;
  Icallback cb = (Icallback)IupGetCallback(ih, "HELP_CB");
  if (cb && cb(ih) == IUP_CLOSE)
  {
    if (iupStrEqualNoCase(iupAttribGetStr(ih, "BUTTONS"), "OK"))
      iupAttribSet(ih, "BUTTONRESPONSE", "1");
    else
      iupAttribSet(ih, "BUTTONRESPONSE", "2");
  }

  (void)call_data;
  (void)w;
}

static int motMessageDlgPopup(Ihandle* ih, int x, int y)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  Widget msgbox, dialog;
  int style = XmDIALOG_FULL_APPLICATION_MODAL;
  int type = XmDIALOG_MESSAGE;
  int num_but = 2;
  char *value;

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
  if (iupStrEqualNoCase(value, "OK"))
  {
    XtUnmanageChild(XmMessageBoxGetChild(msgbox, XmDIALOG_CANCEL_BUTTON));
    num_but = 1;
  }
  else if (iupStrEqualNoCase(value, "YESNO"))
  {
    iupmotSetXmString(msgbox, XmNokLabelString, IupGetLanguageString("IUP_YES"));
    iupmotSetXmString(msgbox, XmNcancelLabelString, IupGetLanguageString("IUP_NO"));
  }

  if (!IupGetCallback(ih, "HELP_CB"))
    XtUnmanageChild(XmMessageBoxGetChild(msgbox, XmDIALOG_HELP_BUTTON));

  if (num_but == 2 && iupAttribGetInt(ih, "BUTTONDEFAULT") == 2)
    XtVaSetValues(msgbox, XmNdefaultButtonType, XmDIALOG_CANCEL_BUTTON, NULL);
  else
    XtVaSetValues(msgbox, XmNdefaultButtonType, XmDIALOG_OK_BUTTON, NULL);

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

  return IUP_NOERROR;
}

void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = motMessageDlgPopup;
}
