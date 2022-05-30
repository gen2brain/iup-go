/** \file
 * \brief Windows IupMessageDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"

#include "iup_drv.h"
#include "iupwin_drv.h"
#include "iupwin_str.h"


static void CALLBACK winMessageDlgHelpCallback(HELPINFO* HelpInfo)
{
  Ihandle* ih = (Ihandle*)HelpInfo->dwContextId;
  Icallback cb = (Icallback)IupGetCallback(ih, "HELP_CB");
  if (cb && cb(ih) == IUP_CLOSE)
  {
    if (iupStrEqualNoCase(iupAttribGetStr(ih, "BUTTONS"), "OK")) /* only one button */
      EndDialog((HWND)HelpInfo->hItemHandle, IDOK);
    else
      EndDialog((HWND)HelpInfo->hItemHandle, IDCANCEL);
  }
}

static char* winMessageDlgGetAutoModalAttrib(Ihandle* ih)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  int automodal = 1;
  if (parent)
    automodal = 0;
  return iupStrReturnBoolean(automodal);
}

static int winMessageDlgPopup(Ihandle* ih, int x, int y)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  MSGBOXPARAMS MsgBoxParams;
  int result, button_def;
  DWORD dwStyle = MB_TASKMODAL;
  char *icon, *buttons;
  (void)x;
  (void)y;

  /* MessageBox is the only Windows pre-defined dialog that has a modal control,
     no need to force a parent dialog here. */

  icon = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(icon, "ERROR"))
    dwStyle |= MB_ICONERROR;
  else if (iupStrEqualNoCase(icon, "WARNING"))
    dwStyle |= MB_ICONWARNING;
  else if (iupStrEqualNoCase(icon, "INFORMATION"))
    dwStyle |= MB_ICONINFORMATION;
  else if (iupStrEqualNoCase(icon, "QUESTION"))
    dwStyle |= MB_ICONQUESTION;

  buttons = iupAttribGetStr(ih, "BUTTONS");
  if (iupStrEqualNoCase(buttons, "OKCANCEL"))
    dwStyle |= MB_OKCANCEL;
  else if (iupStrEqualNoCase(buttons, "RETRYCANCEL"))
    dwStyle |= MB_RETRYCANCEL;
  else if (iupStrEqualNoCase(buttons, "YESNO"))
    dwStyle |= MB_YESNO;
  else if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
    dwStyle |= MB_YESNOCANCEL;
  else
    dwStyle |= MB_OK;

  if (IupGetCallback(ih, "HELP_CB"))
    dwStyle |= MB_HELP;

  button_def = iupAttribGetInt(ih, "BUTTONDEFAULT");
  if (button_def == 3)
    dwStyle |= MB_DEFBUTTON3;
  else if (button_def == 2)
    dwStyle |= MB_DEFBUTTON2;
  else
    dwStyle |= MB_DEFBUTTON1;

  MsgBoxParams.cbSize = sizeof(MSGBOXPARAMS);
  MsgBoxParams.hwndOwner = parent;
  MsgBoxParams.hInstance = NULL;
  MsgBoxParams.lpszText = iupwinStrToSystem(iupAttribGet(ih, "VALUE"));
  MsgBoxParams.lpszCaption = iupwinStrToSystem(iupAttribGet(ih, "TITLE"));
  MsgBoxParams.dwStyle = dwStyle;
  MsgBoxParams.lpszIcon = NULL;
  MsgBoxParams.dwContextHelpId = (DWORD_PTR)ih;
  MsgBoxParams.lpfnMsgBoxCallback = (MSGBOXCALLBACK)winMessageDlgHelpCallback;
  MsgBoxParams.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

  result = MessageBoxIndirect(&MsgBoxParams);

  if (result == 0)  /* memory error */
  {
    iupAttribSet(ih, "BUTTONRESPONSE", NULL);
    return IUP_ERROR;
  }

  if (result == IDOK || result == IDYES || result == IDRETRY)
    iupAttribSet(ih, "BUTTONRESPONSE", "1");
  else if (result == IDCANCEL && iupStrEqualNoCase(buttons, "YESNOCANCEL"))
    iupAttribSet(ih, "BUTTONRESPONSE", "3");
  else
    iupAttribSet(ih, "BUTTONRESPONSE", "2");

  return IUP_NOERROR;
}

void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = winMessageDlgPopup;

  iupClassRegisterAttribute(ic, "AUTOMODAL", winMessageDlgGetAutoModalAttrib, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
}

/* 
In Windows it will always sound a beep. The beep is different for each dialog type.
*/
