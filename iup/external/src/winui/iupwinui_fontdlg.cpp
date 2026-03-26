/** \file
 * \brief WinUI Driver - Font Dialog
 *
 * Custom font picker using WinUI ContentDialog with DirectWrite font enumeration.
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

#include <windows.h>
#include <dwrite.h>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_dialog.h"
#include "iup_drvfont.h"
#include "iup_register.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Content;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics;

static const int winui_font_sizes[] = { 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72 };
static const int winui_font_sizes_count = sizeof(winui_font_sizes) / sizeof(winui_font_sizes[0]);

struct WinUIFontDlgState
{
  std::vector<std::wstring> fontFamilies;
  ListView familyList;
  ListView sizeList;
  TextBox sizeBox;
  CheckBox boldCheck;
  CheckBox italicCheck;
  CheckBox underlineCheck;
  CheckBox strikeoutCheck;
  TextBlock preview;
  ColorPicker colorPicker;
  bool showColor;

  WinUIFontDlgState() : familyList(nullptr), sizeList(nullptr), sizeBox(nullptr),
                         boldCheck(nullptr), italicCheck(nullptr),
                         underlineCheck(nullptr), strikeoutCheck(nullptr),
                         preview(nullptr), colorPicker(nullptr), showColor(false) {}
};

static void winuiFontDlgEnumerateFonts(WinUIFontDlgState& state)
{
  if (!winui_dwrite_factory)
    return;

  IDWriteFontCollection* collection = NULL;
  winui_dwrite_factory->GetSystemFontCollection(&collection, FALSE);
  if (!collection)
    return;

  UINT32 count = collection->GetFontFamilyCount();
  for (UINT32 i = 0; i < count; i++)
  {
    IDWriteFontFamily* family = NULL;
    collection->GetFontFamily(i, &family);
    if (!family)
      continue;

    IDWriteLocalizedStrings* names = NULL;
    family->GetFamilyNames(&names);
    if (names)
    {
      UINT32 idx = 0;
      BOOL exists = FALSE;
      names->FindLocaleName(L"en-us", &idx, &exists);
      if (!exists)
        idx = 0;

      UINT32 len = 0;
      names->GetStringLength(idx, &len);
      if (len > 0)
      {
        std::wstring name(len, L'\0');
        names->GetString(idx, &name[0], len + 1);
        state.fontFamilies.push_back(name);
      }
      names->Release();
    }
    family->Release();
  }
  collection->Release();

  std::sort(state.fontFamilies.begin(), state.fontFamilies.end(),
    [](const std::wstring& a, const std::wstring& b) {
      return _wcsicmp(a.c_str(), b.c_str()) < 0;
    });
}

static void winuiFontDlgUpdatePreview(WinUIFontDlgState& state)
{
  if (!state.preview)
    return;

  int selIdx = state.familyList.SelectedIndex();
  hstring familyName = L"Segoe UI";
  if (selIdx >= 0 && selIdx < (int)state.fontFamilies.size())
    familyName = hstring(state.fontFamilies[selIdx]);

  state.preview.FontFamily(Microsoft::UI::Xaml::Media::FontFamily(familyName));

  hstring sizeText = state.sizeBox.Text();
  int size = _wtoi(sizeText.c_str());
  if (size < 1) size = 12;
  float px = size * winui_screen_dpi / 72.0f;
  state.preview.FontSize(static_cast<double>(px));

  if (state.boldCheck.IsChecked().Value())
    state.preview.FontWeight(Windows::UI::Text::FontWeights::Bold());
  else
    state.preview.FontWeight(Windows::UI::Text::FontWeights::Normal());

  if (state.italicCheck.IsChecked().Value())
    state.preview.FontStyle(Windows::UI::Text::FontStyle::Italic);
  else
    state.preview.FontStyle(Windows::UI::Text::FontStyle::Normal);

  Windows::UI::Text::TextDecorations decor = Windows::UI::Text::TextDecorations::None;
  if (state.underlineCheck.IsChecked().Value())
    decor = decor | Windows::UI::Text::TextDecorations::Underline;
  if (state.strikeoutCheck.IsChecked().Value())
    decor = decor | Windows::UI::Text::TextDecorations::Strikethrough;
  state.preview.TextDecorations(decor);

  if (state.showColor && state.colorPicker)
  {
    auto color = state.colorPicker.Color();
    state.preview.Foreground(Microsoft::UI::Xaml::Media::SolidColorBrush(color));
  }
}

static Grid winuiFontDlgBuildContent(WinUIFontDlgState& state,
                                      const char* initTypeface, int initSize,
                                      int initBold, int initItalic,
                                      int initUnderline, int initStrikeout,
                                      unsigned char initR, unsigned char initG, unsigned char initB)
{
  Grid root;

  /* Main layout: 4 rows
     Row 0: font family label + size label
     Row 1: font family list + size list (with textbox above)
     Row 2: style checkboxes
     Row 3: preview
     Row 4: color picker (optional) */
  auto rowDefs = root.RowDefinitions();
  Controls::RowDefinition row0;
  row0.Height(GridLengthHelper::Auto());
  rowDefs.Append(row0);

  Controls::RowDefinition row1;
  row1.Height(GridLength{1, GridUnitType::Star});
  rowDefs.Append(row1);

  Controls::RowDefinition row2;
  row2.Height(GridLengthHelper::Auto());
  rowDefs.Append(row2);

  Controls::RowDefinition row3;
  row3.Height(GridLengthHelper::Auto());
  rowDefs.Append(row3);

  if (state.showColor)
  {
    Controls::RowDefinition row4;
    row4.Height(GridLengthHelper::Auto());
    rowDefs.Append(row4);
  }

  auto colDefs = root.ColumnDefinitions();
  Controls::ColumnDefinition col0;
  col0.Width(GridLength{2, GridUnitType::Star});
  colDefs.Append(col0);

  Controls::ColumnDefinition col1;
  col1.Width(GridLength{1, GridUnitType::Star});
  colDefs.Append(col1);

  /* Row 0: Labels */
  TextBlock familyLabel;
  familyLabel.Text(L"Font family:");
  familyLabel.Margin(ThicknessHelper::FromLengths(0, 0, 8, 4));
  Grid::SetRow(familyLabel, 0);
  Grid::SetColumn(familyLabel, 0);
  root.Children().Append(familyLabel);

  TextBlock sizeLabel;
  sizeLabel.Text(L"Size:");
  sizeLabel.Margin(ThicknessHelper::FromLengths(0, 0, 0, 4));
  Grid::SetRow(sizeLabel, 0);
  Grid::SetColumn(sizeLabel, 1);
  root.Children().Append(sizeLabel);

  /* Row 1: Font family list + Size column */
  state.familyList = ListView();
  state.familyList.SelectionMode(ListViewSelectionMode::Single);
  state.familyList.Margin(ThicknessHelper::FromLengths(0, 0, 8, 8));
  state.familyList.MinHeight(200);

  int selectedFamily = -1;
  std::wstring initTypefaceW;
  if (initTypeface && initTypeface[0])
  {
    int wlen = MultiByteToWideChar(CP_UTF8, 0, initTypeface, -1, NULL, 0);
    initTypefaceW.assign(wlen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, initTypeface, -1, &initTypefaceW[0], wlen);
  }

  for (size_t i = 0; i < state.fontFamilies.size(); i++)
  {
    state.familyList.Items().Append(box_value(hstring(state.fontFamilies[i])));
    if (!initTypefaceW.empty() && _wcsicmp(state.fontFamilies[i].c_str(), initTypefaceW.c_str()) == 0)
      selectedFamily = (int)i;
  }

  if (selectedFamily >= 0)
    state.familyList.SelectedIndex(selectedFamily);

  Grid::SetRow(state.familyList, 1);
  Grid::SetColumn(state.familyList, 0);
  root.Children().Append(state.familyList);

  /* Size column: TextBox on top + ListView below */
  Grid sizeGrid;
  auto sizeRows = sizeGrid.RowDefinitions();
  Controls::RowDefinition sizeRow0;
  sizeRow0.Height(GridLengthHelper::Auto());
  sizeRows.Append(sizeRow0);
  Controls::RowDefinition sizeRow1;
  sizeRow1.Height(GridLength{1, GridUnitType::Star});
  sizeRows.Append(sizeRow1);

  state.sizeBox = TextBox();
  wchar_t sizeBuf[16];
  _itow(initSize, sizeBuf, 10);
  state.sizeBox.Text(sizeBuf);
  state.sizeBox.Margin(ThicknessHelper::FromLengths(0, 0, 0, 4));
  Grid::SetRow(state.sizeBox, 0);
  sizeGrid.Children().Append(state.sizeBox);

  state.sizeList = ListView();
  state.sizeList.SelectionMode(ListViewSelectionMode::Single);
  int selectedSize = -1;
  for (int i = 0; i < winui_font_sizes_count; i++)
  {
    wchar_t buf[16];
    _itow(winui_font_sizes[i], buf, 10);
    state.sizeList.Items().Append(box_value(hstring(buf)));
    if (winui_font_sizes[i] == initSize)
      selectedSize = i;
  }
  if (selectedSize >= 0)
    state.sizeList.SelectedIndex(selectedSize);

  Grid::SetRow(state.sizeList, 1);
  sizeGrid.Children().Append(state.sizeList);

  sizeGrid.Margin(ThicknessHelper::FromLengths(0, 0, 0, 8));
  Grid::SetRow(sizeGrid, 1);
  Grid::SetColumn(sizeGrid, 1);
  root.Children().Append(sizeGrid);

  /* Row 2: Style checkboxes in a 2x2 grid */
  Grid styleGrid;
  styleGrid.Margin(ThicknessHelper::FromLengths(0, 0, 0, 8));
  auto styleColDefs = styleGrid.ColumnDefinitions();
  Controls::ColumnDefinition styleCol0;
  styleCol0.Width(GridLength{1, GridUnitType::Star});
  styleColDefs.Append(styleCol0);
  Controls::ColumnDefinition styleCol1;
  styleCol1.Width(GridLength{1, GridUnitType::Star});
  styleColDefs.Append(styleCol1);
  auto styleRowDefs = styleGrid.RowDefinitions();
  Controls::RowDefinition styleRow0;
  styleRow0.Height(GridLengthHelper::Auto());
  styleRowDefs.Append(styleRow0);
  Controls::RowDefinition styleRow1;
  styleRow1.Height(GridLengthHelper::Auto());
  styleRowDefs.Append(styleRow1);

  state.boldCheck = CheckBox();
  state.boldCheck.Content(box_value(L"Bold"));
  state.boldCheck.IsChecked(initBold != 0);
  Grid::SetRow(state.boldCheck, 0);
  Grid::SetColumn(state.boldCheck, 0);
  styleGrid.Children().Append(state.boldCheck);

  state.italicCheck = CheckBox();
  state.italicCheck.Content(box_value(L"Italic"));
  state.italicCheck.IsChecked(initItalic != 0);
  Grid::SetRow(state.italicCheck, 0);
  Grid::SetColumn(state.italicCheck, 1);
  styleGrid.Children().Append(state.italicCheck);

  state.underlineCheck = CheckBox();
  state.underlineCheck.Content(box_value(L"Underline"));
  state.underlineCheck.IsChecked(initUnderline != 0);
  Grid::SetRow(state.underlineCheck, 1);
  Grid::SetColumn(state.underlineCheck, 0);
  styleGrid.Children().Append(state.underlineCheck);

  state.strikeoutCheck = CheckBox();
  state.strikeoutCheck.Content(box_value(L"Strikeout"));
  state.strikeoutCheck.IsChecked(initStrikeout != 0);
  Grid::SetRow(state.strikeoutCheck, 1);
  Grid::SetColumn(state.strikeoutCheck, 1);
  styleGrid.Children().Append(state.strikeoutCheck);

  Grid::SetRow(styleGrid, 2);
  Grid::SetColumnSpan(styleGrid, 2);
  root.Children().Append(styleGrid);

  /* Row 3: Preview */
  Border previewBorder;
  previewBorder.BorderThickness(ThicknessHelper::FromUniformLength(1));
  previewBorder.BorderBrush(Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Gray()));
  previewBorder.Padding(ThicknessHelper::FromUniformLength(8));
  previewBorder.MinHeight(60);
  previewBorder.Margin(ThicknessHelper::FromLengths(0, 0, 0, 8));

  state.preview = TextBlock();
  state.preview.Text(L"AaBbCcDdEeFf 123");
  state.preview.TextWrapping(TextWrapping::Wrap);
  state.preview.VerticalAlignment(VerticalAlignment::Center);
  previewBorder.Child(state.preview);

  Grid::SetRow(previewBorder, 3);
  Grid::SetColumnSpan(previewBorder, 2);
  root.Children().Append(previewBorder);

  /* Row 4: Color picker (optional) */
  if (state.showColor)
  {
    state.colorPicker = ColorPicker();
    state.colorPicker.IsAlphaEnabled(false);
    state.colorPicker.IsMoreButtonVisible(false);
    state.colorPicker.IsHexInputVisible(true);
    Windows::UI::Color initColor;
    initColor.A = 255;
    initColor.R = initR;
    initColor.G = initG;
    initColor.B = initB;
    state.colorPicker.Color(initColor);
    state.colorPicker.Margin(ThicknessHelper::FromLengths(0, 0, 0, 0));

    Grid::SetRow(state.colorPicker, 4);
    Grid::SetColumnSpan(state.colorPicker, 2);
    root.Children().Append(state.colorPicker);
  }

  /* Wire up events for live preview */
  auto updatePreview = [&state](auto const&, auto const&) { winuiFontDlgUpdatePreview(state); };

  state.familyList.SelectionChanged(updatePreview);
  state.sizeList.SelectionChanged([&state](auto const&, auto const&) {
    int idx = state.sizeList.SelectedIndex();
    if (idx >= 0 && idx < winui_font_sizes_count)
    {
      wchar_t buf[16];
      _itow(winui_font_sizes[idx], buf, 10);
      state.sizeBox.Text(buf);
    }
    winuiFontDlgUpdatePreview(state);
  });
  state.sizeBox.TextChanged([&state](auto const&, auto const&) {
    winuiFontDlgUpdatePreview(state);
  });
  state.boldCheck.Checked(updatePreview);
  state.boldCheck.Unchecked(updatePreview);
  state.italicCheck.Checked(updatePreview);
  state.italicCheck.Unchecked(updatePreview);
  state.underlineCheck.Checked(updatePreview);
  state.underlineCheck.Unchecked(updatePreview);
  state.strikeoutCheck.Checked(updatePreview);
  state.strikeoutCheck.Unchecked(updatePreview);

  if (state.showColor && state.colorPicker)
    state.colorPicker.ColorChanged([&state](auto const&, auto const&) { winuiFontDlgUpdatePreview(state); });

  winuiFontDlgUpdatePreview(state);

  root.Width(450);

  return root;
}

static void winuiFontDlgRunMessageLoop(bool& dialogCompleted)
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

static Ihandle* winuiFontDlgFindParent(Ihandle* ih)
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

static XamlRoot winuiFontDlgGetParentXamlRoot(Ihandle* parent)
{
  if (!parent)
    return nullptr;

  IupWinUIDialogAux* parentAux = winuiGetAux<IupWinUIDialogAux>(parent, IUPWINUI_DIALOG_AUX);
  if (parentAux && parentAux->rootPanel)
    return parentAux->rootPanel.XamlRoot();

  return nullptr;
}

static int winuiFontDlgShow(ContentDialog& dialog, Ihandle* parent)
{
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

  if (parent)
  {
    IupWinUIDialogAux* parentAux = winuiGetAux<IupWinUIDialogAux>(parent, IUPWINUI_DIALOG_AUX);
    if (parentAux && parentAux->islandHwnd)
      SetFocus(parentAux->islandHwnd);
  }

  winuiFontDlgRunMessageLoop(dialogCompleted);

  return (dialogResult == ContentDialogResult::Primary) ? 1 : 0;
}

#define WINUI_FONTDLG_HOST_CLASS L"IupWinUIFontDlgHost"

static LRESULT CALLBACK winuiFontDlgHostWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

static void winuiFontDlgRegisterHostClass(void)
{
  static bool registered = false;
  if (registered)
    return;

  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.lpfnWndProc = winuiFontDlgHostWndProc;
  wc.hInstance = GetModuleHandle(NULL);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = WINUI_FONTDLG_HOST_CLASS;
  RegisterClassExW(&wc);
  registered = true;
}

static int winuiFontDlgShowOnParent(ContentDialog& dialog, Ihandle* parent)
{
  XamlRoot xamlRoot = winuiFontDlgGetParentXamlRoot(parent);
  if (!xamlRoot)
    return -1;

  iupAttribSet(parent, "_IUPWINUI_CONTENT_DIALOG_ACTIVE", "1");

  dialog.XamlRoot(xamlRoot);
  int result = winuiFontDlgShow(dialog, parent);

  iupAttribSet(parent, "_IUPWINUI_CONTENT_DIALOG_ACTIVE", NULL);
  return result;
}

static int winuiFontDlgShowStandalone(ContentDialog& dialog)
{
  winuiFontDlgRegisterHostClass();

  int screenW = GetSystemMetrics(SM_CXSCREEN);
  int screenH = GetSystemMetrics(SM_CYSCREEN);

  HWND tempHwnd = CreateWindowExW(
    WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
    WINUI_FONTDLG_HOST_CLASS,
    L"",
    WS_POPUP,
    0, 0, screenW, screenH,
    NULL, NULL, GetModuleHandle(NULL), NULL
  );

  if (!tempHwnd)
    return -1;

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
    return -1;
  }

  dialog.XamlRoot(xamlRoot);

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

  winuiFontDlgRunMessageLoop(dialogCompleted);

  SetWindowLongPtr(tempHwnd, GWLP_USERDATA, 0);
  tempXamlSource.Close();
  DestroyWindow(tempHwnd);

  return (dialogResult == ContentDialogResult::Primary) ? 1 : 0;
}

static int winuiFontDlgPopup(Ihandle* ih, int x, int y)
{
  (void)x;
  (void)y;

  char typeface[50] = "";
  int height = 9;
  int is_bold = 0, is_italic = 0, is_underline = 0, is_strikeout = 0;
  unsigned char r = 0, g = 0, b = 0;

  char* font = iupAttribGet(ih, "VALUE");
  if (!font)
  {
    if (!iupGetFontInfo(IupGetGlobal("DEFAULTFONT"), typeface, &height, &is_bold, &is_italic, &is_underline, &is_strikeout))
      return IUP_ERROR;
  }
  else
  {
    if (!iupGetFontInfo(font, typeface, &height, &is_bold, &is_italic, &is_underline, &is_strikeout))
    {
      if (!iupGetFontInfo(IupGetGlobal("DEFAULTFONT"), typeface, &height, &is_bold, &is_italic, &is_underline, &is_strikeout))
        return IUP_ERROR;
    }
  }

  if (height < 0)
    height = (int)(-height * 72.0f / winui_screen_dpi);

  iupStrToRGB(iupAttribGet(ih, "COLOR"), &r, &g, &b);

  const char* mapped_name = iupFontGetWinName(typeface);
  if (mapped_name)
    iupStrCopyN(typeface, sizeof(typeface), mapped_name);

  WinUIFontDlgState state;
  state.showColor = iupAttribGetBoolean(ih, "SHOWCOLOR") ? true : false;

  winuiFontDlgEnumerateFonts(state);

  Grid content = winuiFontDlgBuildContent(state, typeface, height,
                                           is_bold, is_italic, is_underline, is_strikeout,
                                           r, g, b);

  ContentDialog dialog;

  char* title = iupAttribGet(ih, "TITLE");
  if (title)
    dialog.Title(box_value(iupwinuiStringToHString(title)));
  else
    dialog.Title(box_value(L"Font"));

  dialog.Content(content);
  dialog.PrimaryButtonText(L"OK");
  dialog.CloseButtonText(L"Cancel");
  dialog.DefaultButton(ContentDialogButton::Primary);

  Ihandle* parent = winuiFontDlgFindParent(ih);
  int result;

  if (parent)
    result = winuiFontDlgShowOnParent(dialog, parent);
  else
    result = winuiFontDlgShowStandalone(dialog);

  if (result != 1)
  {
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "COLOR", NULL);
    iupAttribSet(ih, "STATUS", NULL);
    return IUP_NOERROR;
  }

  int selIdx = state.familyList.SelectedIndex();
  char result_typeface[50] = "";
  if (selIdx >= 0 && selIdx < (int)state.fontFamilies.size())
    WideCharToMultiByte(CP_UTF8, 0, state.fontFamilies[selIdx].c_str(), -1, result_typeface, sizeof(result_typeface), NULL, NULL);
  else
    iupStrCopyN(result_typeface, sizeof(result_typeface), typeface);

  hstring sizeText = state.sizeBox.Text();
  int result_height = _wtoi(sizeText.c_str());
  if (result_height < 1) result_height = height;

  int result_bold = state.boldCheck.IsChecked().Value() ? 1 : 0;
  int result_italic = state.italicCheck.IsChecked().Value() ? 1 : 0;
  int result_underline = state.underlineCheck.IsChecked().Value() ? 1 : 0;
  int result_strikeout = state.strikeoutCheck.IsChecked().Value() ? 1 : 0;

  iupAttribSetStrf(ih, "VALUE", "%s, %s%s%s%s%d", result_typeface,
                   result_bold ? "Bold " : "",
                   result_italic ? "Italic " : "",
                   result_underline ? "Underline " : "",
                   result_strikeout ? "Strikeout " : "",
                   result_height);

  if (state.showColor && state.colorPicker)
  {
    auto color = state.colorPicker.Color();
    iupAttribSetStrf(ih, "COLOR", "%d %d %d", (int)color.R, (int)color.G, (int)color.B);
  }
  else
  {
    iupAttribSetStrf(ih, "COLOR", "%d %d %d", (int)r, (int)g, (int)b);
  }

  iupAttribSet(ih, "STATUS", "1");

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvFontDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = winuiFontDlgPopup;

  iupClassRegisterAttribute(ic, "COLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
