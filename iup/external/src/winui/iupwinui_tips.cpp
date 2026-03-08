/** \file
 * \brief WinUI Driver TIPS management
 *
 * Uses a custom Popup with ShouldConstrainToRootBounds to avoid the broken
 * popup-window positioning in XAML Islands.
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <chrono>
#include <unordered_map>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Windows::Foundation;


static Primitives::Popup winui_tip_popup{nullptr};
static TextBlock winui_tip_text{nullptr};
static Ihandle* winui_tip_popup_dlg = NULL;

static Microsoft::UI::Dispatching::DispatcherQueueTimer winui_tip_timer{nullptr};
static event_token winui_tip_timer_token{};
static Ihandle* winui_tip_ih = NULL;

struct WinUITipHover
{
  event_token enteredToken;
  event_token exitedToken;
};
static std::unordered_map<Ihandle*, WinUITipHover> winui_tip_hover;


static void winuiTipTimerStop(void)
{
  if (winui_tip_timer)
  {
    winui_tip_timer.Stop();
    winui_tip_timer.Tick(winui_tip_timer_token);
    winui_tip_timer = nullptr;
  }
  winui_tip_ih = NULL;
}

static void winuiTipHide(void)
{
  if (winui_tip_popup)
    winui_tip_popup.IsOpen(false);
}

static void winuiTipEnsurePopup(Ihandle* ih)
{
  Ihandle* dlg = IupGetDialog(ih);
  if (!dlg || !dlg->handle)
    return;

  if (winui_tip_popup && winui_tip_popup_dlg == dlg)
    return;

  if (winui_tip_popup)
  {
    winui_tip_popup.IsOpen(false);
    if (winui_tip_popup_dlg)
    {
      IupWinUIDialogAux* oldAux = winuiGetAux<IupWinUIDialogAux>(winui_tip_popup_dlg, IUPWINUI_DIALOG_AUX);
      if (oldAux && oldAux->rootPanel)
      {
        auto children = oldAux->rootPanel.Children();
        uint32_t idx;
        if (children.IndexOf(winui_tip_popup, idx))
          children.RemoveAt(idx);
      }
    }
    winui_tip_popup = nullptr;
    winui_tip_text = nullptr;
  }

  IupWinUIDialogAux* aux = winuiGetAux<IupWinUIDialogAux>(dlg, IUPWINUI_DIALOG_AUX);
  if (!aux || !aux->rootPanel)
    return;

  auto resources = Application::Current().Resources();

  TextBlock text;
  text.TextWrapping(TextWrapping::Wrap);
  text.MaxWidth(400);
  text.FontSize(12);

  auto fgKey = box_value(L"ToolTipForeground");
  if (resources.HasKey(fgKey))
    text.Foreground(resources.Lookup(fgKey).as<Media::Brush>());

  Border border;
  border.BorderThickness(ThicknessHelper::FromUniformLength(1));
  border.Padding(ThicknessHelper::FromLengths(8, 4, 8, 4));

  auto bgKey = box_value(L"ToolTipBackground");
  if (resources.HasKey(bgKey))
    border.Background(resources.Lookup(bgKey).as<Media::Brush>());

  auto borderKey = box_value(L"ToolTipBorderBrush");
  if (resources.HasKey(borderKey))
    border.BorderBrush(resources.Lookup(borderKey).as<Media::Brush>());
  border.Child(text);

  Primitives::Popup popup;
  popup.ShouldConstrainToRootBounds(true);
  popup.IsHitTestVisible(false);
  popup.Child(border);

  aux->rootPanel.Children().Append(popup);

  winui_tip_popup = popup;
  winui_tip_text = text;
  winui_tip_popup_dlg = dlg;
}

static void winuiTipShowAt(Ihandle* ih, const char* text, double x, double y)
{
  winuiTipEnsurePopup(ih);
  if (!winui_tip_popup || !winui_tip_text)
    return;

  winui_tip_text.Text(iupwinuiStringToHString(text));
  winui_tip_popup.HorizontalOffset(x);
  winui_tip_popup.VerticalOffset(y + 20);
  winui_tip_popup.IsOpen(true);
}

static bool winuiTipGetIslandCursorPos(Ihandle* ih, POINT* pt)
{
  Ihandle* dlg = IupGetDialog(ih);
  if (!dlg || !dlg->handle)
    return false;

  IupWinUIDialogAux* aux = winuiGetAux<IupWinUIDialogAux>(dlg, IUPWINUI_DIALOG_AUX);
  if (!aux || !aux->islandHwnd)
    return false;

  GetCursorPos(pt);
  ScreenToClient(aux->islandHwnd, pt);
  return true;
}

static void winuiTipDismissStart(Ihandle* ih)
{
  winuiTipTimerStop();

  void* dq_ptr = iupwinuiGetDispatcherQueue();
  if (!dq_ptr)
    return;

  IInspectable dq_obj{nullptr};
  winrt::copy_from_abi(dq_obj, dq_ptr);
  auto dq = dq_obj.as<Microsoft::UI::Dispatching::DispatcherQueue>();

  winui_tip_ih = ih;
  winui_tip_timer = dq.CreateTimer();
  winui_tip_timer.Interval(std::chrono::milliseconds{5000});
  winui_tip_timer.IsRepeating(false);
  winui_tip_timer_token = winui_tip_timer.Tick([](auto&&, auto&&) {
    winuiTipTimerStop();
    winuiTipHide();
  });
  winui_tip_timer.Start();
}

static void winuiTipUnhookHover(Ihandle* ih)
{
  auto it = winui_tip_hover.find(ih);
  if (it == winui_tip_hover.end())
    return;

  if (ih->handle && !winuiHandleIsHWND(ih))
  {
    UIElement elem = winuiGetHandle<UIElement>(ih);
    if (elem)
    {
      elem.PointerEntered(it->second.enteredToken);
      elem.PointerExited(it->second.exitedToken);
    }
  }

  winui_tip_hover.erase(it);
}

extern "C" int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  winuiTipHide();
  winuiTipTimerStop();

  if (iupStrBoolean(value))
  {
    const char* tip = iupAttribGet(ih, "TIP");
    if (!tip)
      return 0;

    POINT pt;
    if (!winuiTipGetIslandCursorPos(ih, &pt))
      return 0;

    winuiTipShowAt(ih, tip, (double)pt.x, (double)pt.y);
    winuiTipDismissStart(ih);
  }

  return 0;
}

extern "C" char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  (void)ih;

  if (winui_tip_popup)
    return iupStrReturnBoolean(winui_tip_popup.IsOpen() ? 1 : 0);

  return NULL;
}

extern "C" int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return 0;

  UIElement elem = winuiGetHandle<UIElement>(ih);
  if (!elem)
    return 0;

  if (value)
  {
    if (winui_tip_hover.find(ih) == winui_tip_hover.end())
    {
      WinUITipHover hover;

      hover.enteredToken = elem.PointerEntered([ih](IInspectable const&, Input::PointerRoutedEventArgs const&) {
        if (!iupObjectCheck(ih))
          return;
        const char* tip = iupAttribGet(ih, "TIP");
        if (!tip)
          return;

        POINT pt;
        if (!winuiTipGetIslandCursorPos(ih, &pt))
          return;

        winuiTipShowAt(ih, tip, (double)pt.x, (double)pt.y);
      });

      hover.exitedToken = elem.PointerExited([](IInspectable const&, Input::PointerRoutedEventArgs const&) {
        winuiTipHide();
      });

      winui_tip_hover[ih] = hover;
    }
  }
  else
  {
    winuiTipUnhookHover(ih);
    winuiTipTimerStop();
    winuiTipHide();
  }

  return 1;
}
