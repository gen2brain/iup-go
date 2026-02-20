/** \file
 * \brief WinUI Driver - Message Dialog
 *
 * Uses WinUI ContentDialog.
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
  if (msg == WM_MOUSEACTIVATE)
    return MA_NOACTIVATE;
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

static char* winuiMessageDlgGetAutoModalAttrib(Ihandle* ih)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  int automodal = 1;
  if (parent)
    automodal = 0;
  return iupStrReturnBoolean(automodal);
}

static int winuiMessageDlgPopup(Ihandle* ih, int x, int y)
{
  char* buttons;
  int button_def;

  (void)x;
  (void)y;

  XamlRoot xamlRoot{nullptr};
  HWND tempHwnd = NULL;
  DesktopWindowXamlSource tempXamlSource{nullptr};
  Grid tempRoot{nullptr};

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
  HWND parentHwnd = (parent && parent->handle) ? (HWND)parent->handle : NULL;

  winuiMsgDlgRegisterHostClass();

  int screenW = GetSystemMetrics(SM_CXSCREEN);
  int screenH = GetSystemMetrics(SM_CYSCREEN);

  tempHwnd = CreateWindowExW(
    WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
    WINUI_MSGDLG_HOST_CLASS,
    L"",
    WS_POPUP,
    0, 0, screenW, screenH,
    parentHwnd, NULL, GetModuleHandle(NULL), NULL
  );

  if (!tempHwnd)
    return IUP_ERROR;

  tempXamlSource = DesktopWindowXamlSource();

  Microsoft::UI::WindowId wid = winrt::Microsoft::UI::GetWindowIdFromWindow(tempHwnd);
  tempXamlSource.Initialize(wid);

  DesktopChildSiteBridge siteBridge = tempXamlSource.SiteBridge();
  if (siteBridge)
  {
    HWND islandHwnd = GetWindowFromWindowId(siteBridge.WindowId());
    if (islandHwnd)
    {
      SetWindowLong(islandHwnd, GWL_STYLE, WS_CHILD | WS_VISIBLE);
      SetWindowLongPtr(tempHwnd, GWLP_USERDATA, (LONG_PTR)islandHwnd);
    }

    Windows::Graphics::RectInt32 islandRect = {0, 0, screenW, screenH};
    siteBridge.MoveAndResize(islandRect);
  }

  tempRoot = Grid();
  tempRoot.Width(static_cast<double>(screenW));
  tempRoot.Height(static_cast<double>(screenH));
  tempXamlSource.Content(tempRoot);

  ShowWindow(tempHwnd, SW_SHOW);
  UpdateWindow(tempHwnd);

  if (parentHwnd)
    EnableWindow(parentHwnd, FALSE);

  {
    MSG flush;
    while (PeekMessage(&flush, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {}
  }

  {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      BOOL handled = iupwinuiContentPreTranslateMessage(&msg);
      if (!handled)
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }

  xamlRoot = tempRoot.XamlRoot();

  if (!xamlRoot)
  {
    if (parentHwnd)
      EnableWindow(parentHwnd, TRUE);
    if (tempXamlSource) { tempXamlSource.Close(); tempXamlSource = nullptr; }
    if (tempHwnd) DestroyWindow(tempHwnd);
    return IUP_ERROR;
  }

  ContentDialog dialog;

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

  dialog.XamlRoot(xamlRoot);

  buttons = iupAttribGetStr(ih, "BUTTONS");

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

  button_def = iupAttribGetInt(ih, "BUTTONDEFAULT");
  if (button_def == 3)
    dialog.DefaultButton(ContentDialogButton::Close);
  else if (button_def == 2)
    dialog.DefaultButton(ContentDialogButton::Secondary);
  else
    dialog.DefaultButton(ContentDialogButton::Primary);

  ContentDialogResult dialogResult = ContentDialogResult::None;
  bool dialogCompleted = false;

  auto asyncOp = dialog.ShowAsync();
  asyncOp.Completed([&dialogResult, &dialogCompleted](
      IAsyncOperation<ContentDialogResult> const& op, AsyncStatus status) {
    if (status == AsyncStatus::Completed)
      dialogResult = op.GetResults();
    dialogCompleted = true;
  });

  {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      BOOL handled = iupwinuiContentPreTranslateMessage(&msg);
      if (!handled)
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      if (dialogCompleted)
        break;
    }
  }

  iupwinuiBringWindowToForeground(tempHwnd);
  {
    HWND islandHwnd = (HWND)GetWindowLongPtr(tempHwnd, GWLP_USERDATA);
    if (islandHwnd)
      SetFocus(islandHwnd);
  }

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

  if (dialogResult == ContentDialogResult::Primary)
    iupAttribSet(ih, "BUTTONRESPONSE", "1");
  else if (dialogResult == ContentDialogResult::Secondary)
    iupAttribSet(ih, "BUTTONRESPONSE", "2");
  else
    iupAttribSet(ih, "BUTTONRESPONSE", iupStrEqualNoCase(buttons, "YESNOCANCEL") ? "3" : "2");

  {
    MSG flush;
    while (PeekMessage(&flush, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {}
  }

  if (parentHwnd)
  {
    EnableWindow(parentHwnd, TRUE);
    iupwinuiBringWindowToForeground(parentHwnd);
  }

  if (tempHwnd)
    SetWindowLongPtr(tempHwnd, GWLP_USERDATA, 0);
  if (tempXamlSource)
  {
    tempXamlSource.Close();
    tempXamlSource = nullptr;
  }
  if (tempHwnd)
    DestroyWindow(tempHwnd);

  return IUP_NOERROR;
}

extern "C" void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = winuiMessageDlgPopup;

  iupClassRegisterAttribute(ic, "AUTOMODAL", winuiMessageDlgGetAutoModalAttrib, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIALOGTYPE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
