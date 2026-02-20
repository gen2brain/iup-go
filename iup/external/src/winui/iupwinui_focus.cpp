/** \file
 * \brief WinUI Driver - Focus Management
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
#include "iup_focus.h"
#include "iup_class.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Input;


static UIElement winuiGetFocusableElement(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return nullptr;

  if (IupClassMatch(ih, "toggle"))
  {
    IupWinUIToggleAux* aux = winuiGetAux<IupWinUIToggleAux>(ih, IUPWINUI_TOGGLE_AUX);
    if (aux)
    {
      switch (aux->controlType)
      {
      case IUPWINUI_TOGGLE_TOGGLESWITCH:
        return winuiGetHandle<ToggleSwitch>(ih);
      case IUPWINUI_TOGGLE_TOGGLEBUTTON:
        return winuiGetHandle<ToggleButton>(ih);
      case IUPWINUI_TOGGLE_RADIOBUTTON:
        return winuiGetHandle<RadioButton>(ih);
      case IUPWINUI_TOGGLE_CHECKBOX:
      default:
        return winuiGetHandle<CheckBox>(ih);
      }
    }
    return nullptr;
  }
  else if (IupClassMatch(ih, "text") || IupClassMatch(ih, "multiline"))
  {
    IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
    if (aux && aux->isPassword)
      return winuiGetHandle<PasswordBox>(ih);
    else
      return winuiGetHandle<TextBox>(ih);
  }
  else if (IupClassMatch(ih, "list"))
  {
    IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
    if (aux && aux->isDropdown)
      return winuiGetHandle<ComboBox>(ih);
    else
      return winuiGetHandle<ListBox>(ih);
  }
  else if (IupClassMatch(ih, "table"))
  {
    char* ptr = iupAttribGet(ih, "_IUPWINUI_TABLE_LISTVIEW");
    if (ptr)
    {
      Windows::Foundation::IInspectable obj{nullptr};
      winrt::copy_from_abi(obj, ptr);
      return obj.try_as<UIElement>();
    }
    return nullptr;
  }

  Windows::Foundation::IInspectable obj{nullptr};
  winrt::copy_from_abi(obj, ih->handle);
  UIElement elem = obj.try_as<UIElement>();
  return elem;
}

extern "C" void iupwinuiSetCanFocus(void* widget, int can)
{
  if (!widget)
    return;

  winrt::Windows::Foundation::IInspectable obj{nullptr};
  winrt::copy_from_abi(obj, widget);
  Control ctrl = obj.try_as<Control>();
  if (ctrl)
    ctrl.IsTabStop(can ? true : false);
}

extern "C" void iupwinuiBringWindowToForeground(HWND hwnd)
{
  if (!hwnd)
    return;

  if (GetForegroundWindow() != hwnd)
  {
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_MENU;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_MENU;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));

    SetForegroundWindow(hwnd);
  }
}

static void winuiSetFocusToIsland(Ihandle* dialog)
{
  if (!dialog || !dialog->handle)
    return;

  iupwinuiBringWindowToForeground((HWND)dialog->handle);

  IupWinUIDialogAux* aux = winuiGetAux<IupWinUIDialogAux>(dialog, IUPWINUI_DIALOG_AUX);
  if (aux && aux->islandHwnd)
    SetFocus(aux->islandHwnd);
}

extern "C" void iupdrvSetFocus(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  if (IupClassMatch(ih, "dialog"))
  {
    winuiSetFocusToIsland(ih);
    iupSetCurrentFocus(ih);
    return;
  }

  if (iupAttribGet(ih, "_IUP_GLCONTROLDATA"))
  {
    SetFocus((HWND)ih->handle);
    iupSetCurrentFocus(ih);
    return;
  }

  Ihandle* dialog = IupGetDialog(ih);

  if (dialog)
    iupAttribSet(dialog, "_IUPWINUI_LASTFOCUS", (char*)ih);

  winuiSetFocusToIsland(dialog);

  UIElement elem = winuiGetFocusableElement(ih);
  if (elem)
    elem.Focus(FocusState::Programmatic);

  iupSetCurrentFocus(ih);
}

extern "C" void iupdrvActivate(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  iupdrvSetFocus(ih);

  if (IupClassMatch(ih, "button"))
  {
    Button btn = winuiGetHandle<Button>(ih);
    if (btn)
    {
      IFn cb = (IFn)IupGetCallback(ih, "ACTION");
      if (cb)
      {
        int ret = cb(ih);
        if (ret == IUP_CLOSE)
          IupExitLoop();
      }
    }
  }
  else if (IupClassMatch(ih, "toggle"))
  {
    IupSetAttribute(ih, "VALUE", "TOGGLE");
  }
}
