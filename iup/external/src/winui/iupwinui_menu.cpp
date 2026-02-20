/** \file
 * \brief WinUI Driver - Menu System
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_menu.h"
#include "iup_image.h"
#include "iup_register.h"
#include "iup_childtree.h"
#include "iup_dlglist.h"
}

#include "iupwinui_drv.h"

#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Windows::Foundation;


static double winuiMenuGetIupFontSize(Ihandle* ih)
{
  const char* fontvalue = iupGetFontValue(ih);
  if (!fontvalue)
    fontvalue = IupGetGlobal("DEFAULTFONT");
  if (!fontvalue)
    return 14.0;

  char typeface[50];
  int size, is_bold, is_italic, is_underline, is_strikeout;
  if (!iupGetFontInfo(fontvalue, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return 14.0;

  if (size < 0)
    return (double)(-size);

  HDC hdc = GetDC(NULL);
  double dpi = (double)GetDeviceCaps(hdc, LOGPIXELSY);
  ReleaseDC(NULL, hdc);
  return (double)size * dpi / 72.0;
}

static bool winuiItemIsCheckable(Ihandle* ih)
{
  char* value = iupAttribGet(ih, "VALUE");
  char* hidemark = iupAttribGet(ih, "HIDEMARK");

  if (value && (iupStrEqualNoCase(value, "ON") || iupStrEqualNoCase(value, "OFF")))
    return true;
  if (hidemark && !iupStrBoolean(hidemark))
    return true;

  return false;
}

static hstring winuiMenuGetTitle(const char* title, wchar_t* accessKey)
{
  char c = 0;
  hstring result = iupwinuiProcessMnemonic(title, &c);

  if (accessKey)
    *accessKey = c ? (wchar_t)c : 0;

  return result;
}

static void winuiItemClickHandler(Ihandle* ih)
{
  IupWinUIItemAux* aux = winuiGetAux<IupWinUIItemAux>(ih, IUPWINUI_ITEM_AUX);

  if (aux && aux->isCheckable)
  {
    ToggleMenuFlyoutItem item = winuiGetHandle<ToggleMenuFlyoutItem>(ih);
    if (item)
    {
      if (iupAttribGetBoolean(ih, "AUTOTOGGLE"))
        iupAttribSet(ih, "VALUE", item.IsChecked() ? "ON" : "OFF");
      else
        item.IsChecked(!item.IsChecked());
    }
  }

  IFn cb = (IFn)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int ret = cb(ih);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }
}

winrt::Windows::Foundation::Collections::IVector<MenuFlyoutItemBase> winuiMenuGetItemsCollection(Ihandle* menu)
{
  Ihandle* submenu = menu->parent;
  if (submenu && IupClassMatch(submenu, "submenu"))
  {
    Ihandle* grandparent = submenu->parent;
    if (grandparent && iupMenuIsMenuBar(grandparent))
    {
      MenuBarItem barItem = winuiGetHandle<MenuBarItem>(submenu);
      if (barItem)
        return barItem.Items();
    }
    else
    {
      MenuFlyoutSubItem subItem = winuiGetHandle<MenuFlyoutSubItem>(submenu);
      if (subItem)
        return subItem.Items();
    }
  }

  IupWinUIMenuAux* menuAux = winuiGetAux<IupWinUIMenuAux>(menu, IUPWINUI_MENU_AUX);
  if (menuAux && !menuAux->isMenuBar)
  {
    MenuFlyout flyout = winuiGetHandle<MenuFlyout>(menu);
    if (flyout)
      return flyout.Items();
  }

  return nullptr;
}

template<typename T>
static void winuiMenuItemAddToParent(Ihandle* ih, T item)
{
  Ihandle* parent = ih->parent;
  if (!parent || !IupClassMatch(parent, "menu"))
    return;

  IupWinUIMenuAux* menuAux = winuiGetAux<IupWinUIMenuAux>(parent, IUPWINUI_MENU_AUX);
  if (menuAux && menuAux->isMenuBar)
  {
    MenuBar menuBar = winuiGetHandle<MenuBar>(parent);
    if (menuBar)
    {
      MenuBarItem barItem;
      barItem.Title(item.Text());
      barItem.Items().Append(item);
      menuBar.Items().Append(barItem);
    }
    return;
  }

  auto items = winuiMenuGetItemsCollection(parent);
  if (items)
    items.Append(item);
}

static int winuiMenuMapMethod(Ihandle* ih)
{
  IupWinUIMenuAux* aux = new IupWinUIMenuAux();
  winuiSetAux(ih, IUPWINUI_MENU_AUX, aux);

  if (iupMenuIsMenuBar(ih))
  {
    aux->isMenuBar = true;
    MenuBar menuBar = MenuBar();
    menuBar.MinHeight(0);

    double fontSize = winuiMenuGetIupFontSize(ih);
    menuBar.Resources().Insert(
      box_value(L"ControlContentThemeFontSize"),
      box_value(fontSize));

    winuiStoreHandle(ih, menuBar);

    Ihandle* dialog = ih->parent;
    if (dialog && dialog->handle)
    {
      MenuBar storedMenuBar = winuiGetHandle<MenuBar>(ih);
      winuiDialogSetMenuBar(dialog, storedMenuBar);
    }
  }
  else
  {
    aux->isPopup = true;
    MenuFlyout flyout;
    winuiStoreHandle(ih, flyout);
  }

  return IUP_NOERROR;
}

static void winuiMenuUnMapMethod(Ihandle* ih)
{
  IupWinUIMenuAux* aux = winuiGetAux<IupWinUIMenuAux>(ih, IUPWINUI_MENU_AUX);

  if (aux && aux->isMenuBar)
  {
    Ihandle* dialog = IupGetDialog(ih);
    if (dialog)
      winuiDialogSetMenuBar(dialog, nullptr);
    winuiReleaseHandle<MenuBar>(ih);
  }
  else
  {
    winuiReleaseHandle<MenuFlyout>(ih);
  }

  winuiFreeAux<IupWinUIMenuAux>(ih, IUPWINUI_MENU_AUX);
}

static int winuiItemMapMethod(Ihandle* ih)
{
  Ihandle* parent = ih->parent;
  if (!parent)
    return IUP_ERROR;

  IupWinUIItemAux* aux = new IupWinUIItemAux();
  winuiSetAux(ih, IUPWINUI_ITEM_AUX, aux);

  const char* title = iupAttribGet(ih, "TITLE");
  wchar_t accessKey = 0;
  hstring text = winuiMenuGetTitle(title, &accessKey);

  aux->isCheckable = winuiItemIsCheckable(ih);

  if (aux->isCheckable)
  {
    ToggleMenuFlyoutItem item;
    item.Text(text);
    item.FontSize(winuiMenuGetIupFontSize(ih));

    if (accessKey)
      item.AccessKey(hstring(&accessKey, 1));

    char* value = iupAttribGet(ih, "VALUE");
    if (value && iupStrEqualNoCase(value, "ON"))
      item.IsChecked(true);

    char* active = iupAttribGet(ih, "ACTIVE");
    if (active && !iupStrBoolean(active))
      item.IsEnabled(false);

    aux->clickToken = item.Click([ih](IInspectable const&, RoutedEventArgs const&) {
      winuiItemClickHandler(ih);
    });

    winuiMenuItemAddToParent(ih, item);
    winuiStoreHandle(ih, item);
  }
  else
  {
    MenuFlyoutItem item;
    item.Text(text);
    item.FontSize(winuiMenuGetIupFontSize(ih));

    if (accessKey)
      item.AccessKey(hstring(&accessKey, 1));

    char* active = iupAttribGet(ih, "ACTIVE");
    if (active && !iupStrBoolean(active))
      item.IsEnabled(false);

    aux->clickToken = item.Click([ih](IInspectable const&, RoutedEventArgs const&) {
      winuiItemClickHandler(ih);
    });

    winuiMenuItemAddToParent(ih, item);
    winuiStoreHandle(ih, item);
  }

  return IUP_NOERROR;
}

static void winuiItemUnMapMethod(Ihandle* ih)
{
  IupWinUIItemAux* aux = winuiGetAux<IupWinUIItemAux>(ih, IUPWINUI_ITEM_AUX);

  if (ih->handle && aux)
  {
    if (aux->isCheckable)
    {
      ToggleMenuFlyoutItem item = winuiGetHandle<ToggleMenuFlyoutItem>(ih);
      if (item && aux->clickToken)
        item.Click(aux->clickToken);
      winuiReleaseHandle<ToggleMenuFlyoutItem>(ih);
    }
    else
    {
      MenuFlyoutItem item = winuiGetHandle<MenuFlyoutItem>(ih);
      if (item && aux->clickToken)
        item.Click(aux->clickToken);
      winuiReleaseHandle<MenuFlyoutItem>(ih);
    }
  }

  winuiFreeAux<IupWinUIItemAux>(ih, IUPWINUI_ITEM_AUX);
}

static int winuiSubmenuMapMethod(Ihandle* ih)
{
  Ihandle* parent = ih->parent;
  if (!parent)
    return IUP_ERROR;

  const char* title = iupAttribGet(ih, "TITLE");
  wchar_t accessKey = 0;
  hstring text = winuiMenuGetTitle(title, &accessKey);

  if (iupMenuIsMenuBar(parent))
  {
    MenuBarItem barItem;
    barItem.Title(text);

    if (accessKey)
      barItem.AccessKey(hstring(&accessKey, 1));

    char* active = iupAttribGet(ih, "ACTIVE");
    if (active && !iupStrBoolean(active))
      barItem.IsEnabled(false);

    MenuBar menuBar = winuiGetHandle<MenuBar>(parent);
    if (menuBar)
      menuBar.Items().Append(barItem);

    winuiStoreHandle(ih, barItem);
  }
  else
  {
    MenuFlyoutSubItem subItem;
    subItem.Text(text);
    subItem.FontSize(winuiMenuGetIupFontSize(ih));

    if (accessKey)
      subItem.AccessKey(hstring(&accessKey, 1));

    char* active = iupAttribGet(ih, "ACTIVE");
    if (active && !iupStrBoolean(active))
      subItem.IsEnabled(false);

    winuiMenuItemAddToParent(ih, subItem);
    winuiStoreHandle(ih, subItem);
  }

  return IUP_NOERROR;
}

static void winuiSubmenuUnMapMethod(Ihandle* ih)
{
  Ihandle* parent = ih->parent;

  if (parent && iupMenuIsMenuBar(parent))
    winuiReleaseHandle<MenuBarItem>(ih);
  else
    winuiReleaseHandle<MenuFlyoutSubItem>(ih);
}

static int winuiSeparatorMapMethod(Ihandle* ih)
{
  Ihandle* parent = ih->parent;
  if (!parent || !IupClassMatch(parent, "menu"))
    return IUP_ERROR;

  auto items = winuiMenuGetItemsCollection(parent);
  if (items)
  {
    MenuFlyoutSeparator sep;
    items.Append(sep);
    winuiStoreHandle(ih, sep);
  }

  return IUP_NOERROR;
}

static void winuiSeparatorUnMapMethod(Ihandle* ih)
{
  winuiReleaseHandle<MenuFlyoutSeparator>(ih);
}

static void winuiItemSetIcon(Ihandle* ih, const char* value)
{
  IupWinUIItemAux* aux = winuiGetAux<IupWinUIItemAux>(ih, IUPWINUI_ITEM_AUX);
  if (!aux)
    return;

  IconElement icon{nullptr};

  if (value)
  {
    void* imghandle = iupImageGetImage(value, ih, 0, NULL);
    WriteableBitmap bitmap = winuiGetBitmapFromHandle(imghandle);
    if (bitmap)
    {
      ImageIcon imageIcon;
      imageIcon.Source(bitmap);
      icon = imageIcon;
    }
  }

  if (aux->isCheckable)
  {
    ToggleMenuFlyoutItem item = winuiGetHandle<ToggleMenuFlyoutItem>(ih);
    if (item)
      item.Icon(icon);
  }
  else
  {
    MenuFlyoutItem item = winuiGetHandle<MenuFlyoutItem>(ih);
    if (item)
      item.Icon(icon);
  }
}

static int winuiItemSetImageAttrib(Ihandle* ih, const char* value)
{
  winuiItemSetIcon(ih, value);
  return 1;
}

static int winuiItemSetTitleAttrib(Ihandle* ih, const char* value)
{
  IupWinUIItemAux* aux = winuiGetAux<IupWinUIItemAux>(ih, IUPWINUI_ITEM_AUX);
  if (!aux)
    return 0;

  wchar_t accessKey = 0;
  hstring text = winuiMenuGetTitle(value, &accessKey);

  if (aux->isCheckable)
  {
    ToggleMenuFlyoutItem item = winuiGetHandle<ToggleMenuFlyoutItem>(ih);
    if (item)
    {
      item.Text(text);
      if (accessKey)
        item.AccessKey(hstring(&accessKey, 1));
    }
  }
  else
  {
    MenuFlyoutItem item = winuiGetHandle<MenuFlyoutItem>(ih);
    if (item)
    {
      item.Text(text);
      if (accessKey)
        item.AccessKey(hstring(&accessKey, 1));
    }
  }

  return 0;
}

static char* winuiItemGetTitleAttrib(Ihandle* ih)
{
  IupWinUIItemAux* aux = winuiGetAux<IupWinUIItemAux>(ih, IUPWINUI_ITEM_AUX);
  if (!aux)
    return NULL;

  hstring text;
  if (aux->isCheckable)
  {
    ToggleMenuFlyoutItem item = winuiGetHandle<ToggleMenuFlyoutItem>(ih);
    if (item)
      text = item.Text();
  }
  else
  {
    MenuFlyoutItem item = winuiGetHandle<MenuFlyoutItem>(ih);
    if (item)
      text = item.Text();
  }

  if (!text.empty())
    return iupwinuiHStringToString(text);

  return NULL;
}

static int winuiItemSetValueAttrib(Ihandle* ih, const char* value)
{
  IupWinUIItemAux* aux = winuiGetAux<IupWinUIItemAux>(ih, IUPWINUI_ITEM_AUX);
  if (!aux || !aux->isCheckable)
    return 0;

  ToggleMenuFlyoutItem item = winuiGetHandle<ToggleMenuFlyoutItem>(ih);
  if (item)
    item.IsChecked(iupStrEqualNoCase(value, "ON"));

  return 0;
}

static char* winuiItemGetValueAttrib(Ihandle* ih)
{
  IupWinUIItemAux* aux = winuiGetAux<IupWinUIItemAux>(ih, IUPWINUI_ITEM_AUX);
  if (!aux || !aux->isCheckable)
    return NULL;

  ToggleMenuFlyoutItem item = winuiGetHandle<ToggleMenuFlyoutItem>(ih);
  if (item)
    return item.IsChecked() ? (char*)"ON" : (char*)"OFF";

  return NULL;
}

static int winuiItemSetActiveAttrib(Ihandle* ih, const char* value)
{
  IupWinUIItemAux* aux = winuiGetAux<IupWinUIItemAux>(ih, IUPWINUI_ITEM_AUX);
  if (!aux)
    return 0;

  bool enabled = iupStrBoolean(value);

  if (aux->isCheckable)
  {
    ToggleMenuFlyoutItem item = winuiGetHandle<ToggleMenuFlyoutItem>(ih);
    if (item)
      item.IsEnabled(enabled);
  }
  else
  {
    MenuFlyoutItem item = winuiGetHandle<MenuFlyoutItem>(ih);
    if (item)
      item.IsEnabled(enabled);
  }

  return 0;
}

static int winuiSubmenuSetTitleAttrib(Ihandle* ih, const char* value)
{
  Ihandle* parent = ih->parent;

  wchar_t accessKey = 0;
  hstring text = winuiMenuGetTitle(value, &accessKey);

  if (parent && iupMenuIsMenuBar(parent))
  {
    MenuBarItem barItem = winuiGetHandle<MenuBarItem>(ih);
    if (barItem)
    {
      barItem.Title(text);
      if (accessKey)
        barItem.AccessKey(hstring(&accessKey, 1));
    }
  }
  else
  {
    MenuFlyoutSubItem subItem = winuiGetHandle<MenuFlyoutSubItem>(ih);
    if (subItem)
    {
      subItem.Text(text);
      if (accessKey)
        subItem.AccessKey(hstring(&accessKey, 1));
    }
  }

  return 0;
}

static int winuiSubmenuSetActiveAttrib(Ihandle* ih, const char* value)
{
  Ihandle* parent = ih->parent;
  bool enabled = iupStrBoolean(value);

  if (parent && iupMenuIsMenuBar(parent))
  {
    MenuBarItem barItem = winuiGetHandle<MenuBarItem>(ih);
    if (barItem)
      barItem.IsEnabled(enabled);
  }
  else
  {
    MenuFlyoutSubItem subItem = winuiGetHandle<MenuFlyoutSubItem>(ih);
    if (subItem)
      subItem.IsEnabled(enabled);
  }

  return 0;
}

extern "C" int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  MenuFlyout flyout = winuiGetHandle<MenuFlyout>(ih);
  if (!flyout)
    return IUP_ERROR;

  Ihandle* dialog = NULL;
  HWND hWndActive = GetActiveWindow();

  if (hWndActive)
  {
    Ihandle* dlg = iupDlgListFirst();
    while (dlg)
    {
      if (dlg->handle == (InativeHandle*)hWndActive)
      {
        dialog = dlg;
        break;
      }
      dlg = iupDlgListNext();
    }
  }

  if (!dialog)
  {
    Ihandle* dlg = iupDlgListFirst();
    while (dlg)
    {
      if (dlg->handle)
      {
        dialog = dlg;
        hWndActive = (HWND)dlg->handle;
        break;
      }
      dlg = iupDlgListNext();
    }
  }

  if (!dialog)
    return IUP_ERROR;

  IupWinUIDialogAux* dlgAux = winuiGetAux<IupWinUIDialogAux>(dialog, IUPWINUI_DIALOG_AUX);
  if (!dlgAux || !dlgAux->contentCanvas)
    return IUP_ERROR;

  bool wasHidden = !IsWindowVisible(hWndActive);
  RECT savedRect = {0};
  LONG_PTR savedExStyle = 0;

  if (wasHidden)
  {
    GetWindowRect(hWndActive, &savedRect);
    savedExStyle = GetWindowLongPtr(hWndActive, GWL_EXSTYLE);
    SetWindowLongPtr(hWndActive, GWL_EXSTYLE, (savedExStyle | WS_EX_TOOLWINDOW) & ~WS_EX_APPWINDOW);
    SetWindowPos(hWndActive, NULL, -32000, -32000, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    ShowWindow(hWndActive, SW_SHOWNA);
  }

  POINT pt = { x, y };
  ScreenToClient(hWndActive, &pt);

  FlyoutShowOptions options;
  options.Position(Point(static_cast<float>(pt.x), static_cast<float>(pt.y)));
  options.ShowMode(FlyoutShowMode::Standard);

  bool menuClosed = false;

  auto closedToken = flyout.Closed([&menuClosed](auto const&, auto const&) {
    menuClosed = true;
  });

  flyout.ShowAt(dlgAux->contentCanvas, options);

  MSG msg;
  while (!menuClosed)
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

  flyout.Closed(closedToken);

  if (wasHidden)
  {
    SetWindowLongPtr(hWndActive, GWL_EXSTYLE, savedExStyle);
    SetWindowPos(hWndActive, NULL, savedRect.left, savedRect.top, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    IupWinUIDialogAux* da = winuiGetAux<IupWinUIDialogAux>(dialog, IUPWINUI_DIALOG_AUX);
    if (!da || !da->isVisible)
      ShowWindow(hWndActive, SW_HIDE);
  }

  return IUP_NOERROR;
}

extern "C" int iupdrvMenuGetMenuBarSize(Ihandle* ih)
{
  MenuBar menuBar = winuiGetHandle<MenuBar>(ih);
  if (!menuBar)
    return 0;

  double height = menuBar.ActualHeight();
  if (height > 0)
    return (int)(height + 0.5);

  menuBar.Measure(Size{100000.0f, 100000.0f});
  height = menuBar.DesiredSize().Height;
  if (height > 0)
    return (int)(height + 0.5);

  int ch;
  iupdrvFontGetCharSize(ih, NULL, &ch);
  return 4 + ch + 4;
}

static int winuiMenuSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupWinUIMenuAux* aux = winuiGetAux<IupWinUIMenuAux>(ih, IUPWINUI_MENU_AUX);
  if (aux && aux->isMenuBar)
  {
    MenuBar menuBar = winuiGetHandle<MenuBar>(ih);
    if (menuBar)
    {
      Windows::UI::Color color;
      color.A = 255;
      color.R = r;
      color.G = g;
      color.B = b;
      menuBar.Background(SolidColorBrush(color));
    }
  }

  return 1;
}

extern "C" void iupdrvMenuInitClass(Iclass* ic)
{
  ic->Map = winuiMenuMapMethod;
  ic->UnMap = winuiMenuUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, NULL, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, winuiMenuSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "MENUBGCOLOR", IUPAF_DEFAULT);
}

static int winuiItemSetFontAttrib(Ihandle* ih, const char* value)
{
  (void)value;

  IupWinUIItemAux* aux = winuiGetAux<IupWinUIItemAux>(ih, IUPWINUI_ITEM_AUX);
  if (!aux)
    return 1;

  double fontSize = winuiMenuGetIupFontSize(ih);

  if (aux->isCheckable)
  {
    ToggleMenuFlyoutItem item = winuiGetHandle<ToggleMenuFlyoutItem>(ih);
    if (item)
      item.FontSize(fontSize);
  }
  else
  {
    MenuFlyoutItem item = winuiGetHandle<MenuFlyoutItem>(ih);
    if (item)
      item.FontSize(fontSize);
  }

  return 1;
}

extern "C" void iupdrvItemInitClass(Iclass* ic)
{
  ic->Map = winuiItemMapMethod;
  ic->UnMap = winuiItemUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, winuiItemSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "MENUBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "TITLE", winuiItemGetTitleAttrib, winuiItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", winuiItemGetValueAttrib, winuiItemSetValueAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", NULL, winuiItemSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, winuiItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, winuiItemSetImageAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDEMARK", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}

static int winuiSubmenuSetFontAttrib(Ihandle* ih, const char* value)
{
  (void)value;

  Ihandle* parent = ih->parent;
  double fontSize = winuiMenuGetIupFontSize(ih);

  if (parent && iupMenuIsMenuBar(parent))
  {
    MenuBarItem barItem = winuiGetHandle<MenuBarItem>(ih);
    if (barItem)
      barItem.FontSize(fontSize);
  }
  else
  {
    MenuFlyoutSubItem subItem = winuiGetHandle<MenuFlyoutSubItem>(ih);
    if (subItem)
      subItem.FontSize(fontSize);
  }

  return 1;
}

extern "C" void iupdrvSubmenuInitClass(Iclass* ic)
{
  ic->Map = winuiSubmenuMapMethod;
  ic->UnMap = winuiSubmenuUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, winuiSubmenuSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "MENUBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "TITLE", NULL, winuiSubmenuSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", NULL, winuiSubmenuSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
}

extern "C" void iupdrvSeparatorInitClass(Iclass* ic)
{
  ic->Map = winuiSeparatorMapMethod;
  ic->UnMap = winuiSeparatorUnMapMethod;
}
