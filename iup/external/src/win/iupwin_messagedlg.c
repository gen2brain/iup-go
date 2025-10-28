/** \file
 * \brief Windows IupMessageDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <commctrl.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"

#include "iup_drv.h"
#include "iupwin_drv.h"
#include "iupwin_str.h"
#include "iupwin_info.h"


static void CALLBACK winMessageDlgHelpCallback(HELPINFO* HelpInfo)
{
  Ihandle* ih = (Ihandle*)HelpInfo->dwContextId;
  Icallback cb = (Icallback)IupGetCallback(ih, "HELP_CB");
  if (cb && cb(ih) == IUP_CLOSE)
  {
    if (iupStrEqualNoCase(iupAttribGetStr(ih, "BUTTONS"), "OK"))
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

static HRESULT CALLBACK winMessageDlgTaskDialogCallback(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, LONG_PTR lpRefData)
{
  (void)wp;
  (void)lp;

  if (msg == TDN_CREATED)
  {
    iupwinTitleBarThemeColor(hwnd);
    return S_OK;
  }

  if (msg == TDN_HYPERLINK_CLICKED)
    return S_OK;

  if (msg == TDN_HELP)
  {
    Ihandle* ih = (Ihandle*)lpRefData;
    Icallback cb = (Icallback)IupGetCallback(ih, "HELP_CB");
    if (cb && cb(ih) == IUP_CLOSE)
      SendMessage(hwnd, TDM_CLICK_BUTTON, IDCANCEL, 0);
    return S_OK;
  }

  return S_OK;
}

static int winMessageDlgPopupTaskDialog(Ihandle* ih, InativeHandle* parent, char* icon, char* buttons)
{
  typedef HRESULT (WINAPI *TaskDialogIndirectFunc)(const TASKDIALOGCONFIG*, int*, int*, BOOL*);
  static TaskDialogIndirectFunc pTaskDialogIndirect = NULL;
  static int initialized = 0;

  TASKDIALOGCONFIG config;
  int nButton = 0;
  int button_def;
  TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons = 0;
  PCWSTR pszIcon = NULL;
  HRESULT hr;
  WCHAR* wszTitle = NULL;
  WCHAR* wszContent = NULL;

  if (!initialized)
  {
    HMODULE hComCtl = LoadLibrary(TEXT("comctl32.dll"));
    if (hComCtl)
      pTaskDialogIndirect = (TaskDialogIndirectFunc)GetProcAddress(hComCtl, "TaskDialogIndirect");
    initialized = 1;
  }

  if (!pTaskDialogIndirect)
    return 0;

  wszTitle = iupwinStrChar2Wide(iupAttribGet(ih, "TITLE"));
  wszContent = iupwinStrChar2Wide(iupAttribGet(ih, "VALUE"));

  ZeroMemory(&config, sizeof(TASKDIALOGCONFIG));
  config.cbSize = sizeof(TASKDIALOGCONFIG);
  config.hwndParent = parent;
  config.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW | TDF_ALLOW_DIALOG_CANCELLATION;
  config.pszWindowTitle = wszTitle;
  config.pszContent = wszContent;
  config.pfCallback = winMessageDlgTaskDialogCallback;
  config.lpCallbackData = (LONG_PTR)ih;

  if (iupStrEqualNoCase(icon, "ERROR"))
    pszIcon = TD_ERROR_ICON;
  else if (iupStrEqualNoCase(icon, "WARNING"))
    pszIcon = TD_WARNING_ICON;
  else if (iupStrEqualNoCase(icon, "INFORMATION"))
    pszIcon = TD_INFORMATION_ICON;
  else if (iupStrEqualNoCase(icon, "QUESTION"))
    pszIcon = TD_INFORMATION_ICON;

  config.pszMainIcon = pszIcon;

  if (iupStrEqualNoCase(buttons, "OKCANCEL"))
    dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
  else if (iupStrEqualNoCase(buttons, "RETRYCANCEL"))
    dwCommonButtons = TDCBF_RETRY_BUTTON | TDCBF_CANCEL_BUTTON;
  else if (iupStrEqualNoCase(buttons, "YESNO"))
    dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
  else if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
    dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON;
  else
    dwCommonButtons = TDCBF_OK_BUTTON;

  config.dwCommonButtons = dwCommonButtons;

  button_def = iupAttribGetInt(ih, "BUTTONDEFAULT");
  if (button_def == 1)
  {
    if (dwCommonButtons & TDCBF_OK_BUTTON)
      config.nDefaultButton = IDOK;
    else if (dwCommonButtons & TDCBF_YES_BUTTON)
      config.nDefaultButton = IDYES;
    else if (dwCommonButtons & TDCBF_RETRY_BUTTON)
      config.nDefaultButton = IDRETRY;
  }
  else if (button_def == 2)
  {
    if (dwCommonButtons & TDCBF_CANCEL_BUTTON)
      config.nDefaultButton = IDCANCEL;
    else if (dwCommonButtons & TDCBF_NO_BUTTON)
      config.nDefaultButton = IDNO;
  }
  else if (button_def == 3)
  {
    if (dwCommonButtons & TDCBF_CANCEL_BUTTON)
      config.nDefaultButton = IDCANCEL;
  }

  hr = pTaskDialogIndirect(&config, &nButton, NULL, NULL);

  if (wszTitle) free(wszTitle);
  if (wszContent) free(wszContent);

  if (FAILED(hr))
    return 0;

  if (nButton == IDOK || nButton == IDYES || nButton == IDRETRY)
    iupAttribSet(ih, "BUTTONRESPONSE", "1");
  else if (nButton == IDCANCEL && iupStrEqualNoCase(buttons, "YESNOCANCEL"))
    iupAttribSet(ih, "BUTTONRESPONSE", "3");
  else
    iupAttribSet(ih, "BUTTONRESPONSE", "2");

  return 1;
}

static int winMessageDlgPopup(Ihandle* ih, int x, int y)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  char *icon, *buttons;
  (void)x;
  (void)y;

  icon = iupAttribGetStr(ih, "DIALOGTYPE");
  buttons = iupAttribGetStr(ih, "BUTTONS");

  if (iupwinIsSystemDarkMode() && iupwinIsVistaOrNew())
  {
    if (winMessageDlgPopupTaskDialog(ih, parent, icon, buttons))
      return IUP_NOERROR;
  }

  {
    MSGBOXPARAMS MsgBoxParams;
    int result, button_def;
    DWORD dwStyle = MB_TASKMODAL;

    if (iupStrEqualNoCase(icon, "ERROR"))
      dwStyle |= MB_ICONERROR;
    else if (iupStrEqualNoCase(icon, "WARNING"))
      dwStyle |= MB_ICONWARNING;
    else if (iupStrEqualNoCase(icon, "INFORMATION"))
      dwStyle |= MB_ICONINFORMATION;
    else if (iupStrEqualNoCase(icon, "QUESTION"))
      dwStyle |= MB_ICONQUESTION;

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

    if (result == 0)
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
  }

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
