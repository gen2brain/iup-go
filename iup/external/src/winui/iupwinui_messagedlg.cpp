/** \file
 * \brief WinUI Driver - Message Dialog
 *
 * Uses WinUI ContentDialog hosted on the parent dialog's XamlRoot.
 * Falls back to a temporary XAML island when no parent dialog exists.
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_class.h"
#include "iup_dialog.h"
#include "iup_register.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Content;
using namespace Windows::Foundation;
using namespace Windows::Graphics;

#define WINUI_MSGDLG_HOST_CLASS L"IupWinUIMsgDlgHost"

static LRESULT CALLBACK winuiMsgDlgHostWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_SETFOCUS)
  {
    HWND island = (HWND)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (island)
      SetFocus(island);
    return 0;
  }
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void winuiMsgDlgRegisterHostClass(void)
{
  static bool registered = false;
  if (registered)
    return;

  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.lpfnWndProc = winuiMsgDlgHostWndProc;
  wc.hInstance = GetModuleHandle(NULL);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = WINUI_MSGDLG_HOST_CLASS;
  RegisterClassExW(&wc);
  registered = true;
}

static Ihandle* winuiMsgDlgFindParent(Ihandle* ih)
{
  Ihandle* parent = IupGetAttributeHandle(ih, "PARENTDIALOG");
  if (!parent)
    parent = IupGetGlobal("PARENTDIALOG") ? IupGetHandle(IupGetGlobal("PARENTDIALOG")) : NULL;

  if (!parent || !parent->handle)
  {
    HWND active = GetActiveWindow();
    if (active)
    {
      Ihandle* ih_active = (Ihandle*)GetWindowLongPtr(active, GWLP_USERDATA);
      if (ih_active && iupObjectCheck(ih_active))
        parent = ih_active;
    }
  }

  return (parent && parent->handle) ? parent : NULL;
}

static XamlRoot winuiMsgDlgGetParentXamlRoot(Ihandle* parent)
{
  if (!parent)
    return nullptr;

  IupWinUIDialogAux* parentAux = winuiGetAux<IupWinUIDialogAux>(parent, IUPWINUI_DIALOG_AUX);
  if (parentAux && parentAux->rootPanel)
    return parentAux->rootPanel.XamlRoot();

  return nullptr;
}

static void winuiMsgDlgConfigureContent(ContentDialog& dialog, Ihandle* ih)
{
  char* title = iupAttribGet(ih, "TITLE");
  if (title)
    dialog.Title(box_value(iupwinuiStringToHString(title)));

  char* value = iupAttribGet(ih, "VALUE");
  char* dialogtype = iupAttribGetStr(ih, "DIALOGTYPE");

  wchar_t iconGlyph = 0;
  if (iupStrEqualNoCase(dialogtype, "ERROR"))
    iconGlyph = L'\uE783';
  else if (iupStrEqualNoCase(dialogtype, "WARNING"))
    iconGlyph = L'\uE7BA';
  else if (iupStrEqualNoCase(dialogtype, "INFORMATION"))
    iconGlyph = L'\uE946';
  else if (iupStrEqualNoCase(dialogtype, "QUESTION"))
    iconGlyph = L'\uE897';

  if (iconGlyph && value)
  {
    StackPanel panel;
    panel.Orientation(Orientation::Horizontal);
    panel.Spacing(12);

    FontIcon icon;
    icon.Glyph(hstring(&iconGlyph, 1));
    icon.FontSize(32);
    panel.Children().Append(icon);

    TextBlock text;
    text.Text(iupwinuiStringToHString(value));
    text.TextWrapping(TextWrapping::Wrap);
    text.VerticalAlignment(VerticalAlignment::Center);
    panel.Children().Append(text);

    dialog.Content(panel);
  }
  else if (value)
    dialog.Content(box_value(iupwinuiStringToHString(value)));
}

static void winuiMsgDlgConfigureButtons(ContentDialog& dialog, Ihandle* ih)
{
  char* buttons = iupAttribGetStr(ih, "BUTTONS");

  if (iupStrEqualNoCase(buttons, "OKCANCEL"))
  {
    dialog.PrimaryButtonText(L"OK");
    dialog.SecondaryButtonText(L"Cancel");
  }
  else if (iupStrEqualNoCase(buttons, "RETRYCANCEL"))
  {
    dialog.PrimaryButtonText(L"Retry");
    dialog.SecondaryButtonText(L"Cancel");
  }
  else if (iupStrEqualNoCase(buttons, "YESNO"))
  {
    dialog.PrimaryButtonText(L"Yes");
    dialog.SecondaryButtonText(L"No");
  }
  else if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
  {
    dialog.PrimaryButtonText(L"Yes");
    dialog.SecondaryButtonText(L"No");
    dialog.CloseButtonText(L"Cancel");
  }
  else
  {
    dialog.PrimaryButtonText(L"OK");
  }

  int button_def = iupAttribGetInt(ih, "BUTTONDEFAULT");
  if (button_def == 3)
    dialog.DefaultButton(ContentDialogButton::Close);
  else if (button_def == 2)
    dialog.DefaultButton(ContentDialogButton::Secondary);
  else
    dialog.DefaultButton(ContentDialogButton::Primary);
}

static void winuiMsgDlgSetResult(Ihandle* ih, ContentDialogResult dialogResult)
{
  char* buttons = iupAttribGetStr(ih, "BUTTONS");

  if (dialogResult == ContentDialogResult::Primary)
    iupAttribSet(ih, "BUTTONRESPONSE", "1");
  else if (dialogResult == ContentDialogResult::Secondary)
    iupAttribSet(ih, "BUTTONRESPONSE", "2");
  else
    iupAttribSet(ih, "BUTTONRESPONSE", iupStrEqualNoCase(buttons, "YESNOCANCEL") ? "3" : "2");
}

static void winuiMsgDlgRunMessageLoop(bool& dialogCompleted)
{
  MSG msg;
  while (!dialogCompleted)
  {
    BOOL ret = GetMessage(&msg, NULL, 0, 0);
    if (ret == 0 || ret == -1)
      break;

    BOOL handled = iupwinuiContentPreTranslateMessage(&msg);
    if (!handled)
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}

static int winuiMsgDlgShowOnParent(Ihandle* ih, Ihandle* parent)
{
  XamlRoot xamlRoot = winuiMsgDlgGetParentXamlRoot(parent);
  if (!xamlRoot)
    return IUP_ERROR;

  HWND parentHwnd = (HWND)parent->handle;

  iupAttribSet(parent, "_IUPWINUI_CONTENT_DIALOG_ACTIVE", "1");

  ContentDialog dialog;
  dialog.XamlRoot(xamlRoot);
  winuiMsgDlgConfigureContent(dialog, ih);
  winuiMsgDlgConfigureButtons(dialog, ih);

  ContentDialogResult dialogResult = ContentDialogResult::None;
  bool dialogCompleted = false;

  auto asyncOp = dialog.ShowAsync();
  asyncOp.Completed([&dialogResult, &dialogCompleted](
      IAsyncOperation<ContentDialogResult> const& op, AsyncStatus status) {
    if (status == AsyncStatus::Completed)
      dialogResult = op.GetResults();
    dialogCompleted = true;
  });

  iupwinuiProcessPendingMessages();

  IupWinUIDialogAux* parentAux = winuiGetAux<IupWinUIDialogAux>(parent, IUPWINUI_DIALOG_AUX);
  if (parentAux && parentAux->islandHwnd)
    SetFocus(parentAux->islandHwnd);

  winuiMsgDlgRunMessageLoop(dialogCompleted);

  winuiMsgDlgSetResult(ih, dialogResult);

  iupAttribSet(parent, "_IUPWINUI_CONTENT_DIALOG_ACTIVE", NULL);

  return IUP_NOERROR;
}

static int winuiMsgDlgShowStandalone(Ihandle* ih)
{
  winuiMsgDlgRegisterHostClass();

  int screenW = GetSystemMetrics(SM_CXSCREEN);
  int screenH = GetSystemMetrics(SM_CYSCREEN);

  HWND tempHwnd = CreateWindowExW(
    WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
    WINUI_MSGDLG_HOST_CLASS,
    L"",
    WS_POPUP,
    0, 0, screenW, screenH,
    NULL, NULL, GetModuleHandle(NULL), NULL
  );

  if (!tempHwnd)
    return IUP_ERROR;

  DesktopWindowXamlSource tempXamlSource = DesktopWindowXamlSource();

  Microsoft::UI::WindowId wid = winrt::Microsoft::UI::GetWindowIdFromWindow(tempHwnd);
  tempXamlSource.Initialize(wid);

  DesktopChildSiteBridge siteBridge = tempXamlSource.SiteBridge();
  if (siteBridge)
  {
    HWND islandHwnd = GetWindowFromWindowId(siteBridge.WindowId());
    if (islandHwnd)
    {
      SetWindowLong(islandHwnd, GWL_STYLE, WS_TABSTOP | WS_CHILD | WS_VISIBLE);
      SetWindowLongPtr(tempHwnd, GWLP_USERDATA, (LONG_PTR)islandHwnd);
    }

    RectInt32 islandRect = {0, 0, screenW, screenH};
    siteBridge.MoveAndResize(islandRect);
  }

  Grid tempRoot;
  tempRoot.Width(static_cast<double>(screenW));
  tempRoot.Height(static_cast<double>(screenH));
  tempXamlSource.Content(tempRoot);

  ShowWindow(tempHwnd, SW_SHOW);
  UpdateWindow(tempHwnd);

  iupwinuiProcessPendingMessages();

  XamlRoot xamlRoot = tempRoot.XamlRoot();
  if (!xamlRoot)
  {
    tempXamlSource.Close();
    DestroyWindow(tempHwnd);
    return IUP_ERROR;
  }

  ContentDialog dialog;
  dialog.XamlRoot(xamlRoot);
  winuiMsgDlgConfigureContent(dialog, ih);
  winuiMsgDlgConfigureButtons(dialog, ih);

  ContentDialogResult dialogResult = ContentDialogResult::None;
  bool dialogCompleted = false;

  auto asyncOp = dialog.ShowAsync();
  asyncOp.Completed([&dialogResult, &dialogCompleted](
      IAsyncOperation<ContentDialogResult> const& op, AsyncStatus status) {
    if (status == AsyncStatus::Completed)
      dialogResult = op.GetResults();
    dialogCompleted = true;
  });

  iupwinuiProcessPendingMessages();

  {
    HWND islandHwnd = (HWND)GetWindowLongPtr(tempHwnd, GWLP_USERDATA);
    if (islandHwnd)
      SetFocus(islandHwnd);
  }

  winuiMsgDlgRunMessageLoop(dialogCompleted);

  winuiMsgDlgSetResult(ih, dialogResult);

  SetWindowLongPtr(tempHwnd, GWLP_USERDATA, 0);
  tempXamlSource.Close();
  DestroyWindow(tempHwnd);

  return IUP_NOERROR;
}

static int winuiMessageDlgPopup(Ihandle* ih, int x, int y)
{
  (void)x;
  (void)y;

  Ihandle* parent = winuiMsgDlgFindParent(ih);

  if (parent)
    return winuiMsgDlgShowOnParent(ih, parent);
  else
    return winuiMsgDlgShowStandalone(ih);
}

static char* winuiMessageDlgGetAutoModalAttrib(Ihandle* ih)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  int automodal = 1;
  if (parent)
    automodal = 0;
  return iupStrReturnBoolean(automodal);
}

extern "C" IUP_SDK_API void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = winuiMessageDlgPopup;

  iupClassRegisterAttribute(ic, "AUTOMODAL", winuiMessageDlgGetAutoModalAttrib, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIALOGTYPE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
