/** \file
 * \brief WinUI Driver TIPS management
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <chrono>

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
using namespace Windows::Foundation;


static Microsoft::UI::Dispatching::DispatcherQueueTimer winui_tip_timer{nullptr};
static event_token winui_tip_timer_token{};
static Ihandle* winui_tip_ih = NULL;

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

static void winuiTipRestore(Ihandle* ih)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih) || !iupObjectCheck(ih))
    return;

  UIElement elem = winuiGetHandle<UIElement>(ih);
  if (!elem)
    return;

  DependencyObject dep = elem.try_as<DependencyObject>();
  if (!dep)
    return;

  IInspectable tipObj = ToolTipService::GetToolTip(dep);
  if (!tipObj)
    return;

  ToolTip oldTip = tipObj.try_as<ToolTip>();
  if (!oldTip)
    return;

  auto content = oldTip.Content();
  oldTip.IsOpen(false);
  ToolTipService::SetToolTip(dep, nullptr);

  ToolTip newTip;
  newTip.Content(content);
  ToolTipService::SetToolTip(dep, newTip);
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
    Ihandle* ih = winui_tip_ih;
    winuiTipTimerStop();
    winuiTipRestore(ih);
  });
  winui_tip_timer.Start();
}

extern "C" int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return 0;

  UIElement elem = winuiGetHandle<UIElement>(ih);
  if (!elem)
    return 0;

  DependencyObject dep = elem.try_as<DependencyObject>();
  if (!dep)
    return 0;

  IInspectable tipObj = ToolTipService::GetToolTip(dep);
  if (tipObj)
  {
    ToolTip tip = tipObj.try_as<ToolTip>();
    if (tip)
    {
      if (iupStrBoolean(value))
      {
        tip.Placement(Primitives::PlacementMode::Mouse);
        tip.IsOpen(true);
        winuiTipDismissStart(ih);
      }
      else
      {
        winuiTipTimerStop();
        winuiTipRestore(ih);
      }
    }
  }

  return 0;
}

extern "C" char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return NULL;

  UIElement elem = winuiGetHandle<UIElement>(ih);
  if (!elem)
    return NULL;

  DependencyObject dep = elem.try_as<DependencyObject>();
  if (!dep)
    return NULL;

  IInspectable tipObj = ToolTipService::GetToolTip(dep);
  if (tipObj)
  {
    ToolTip tip = tipObj.try_as<ToolTip>();
    if (tip)
      return iupStrReturnBoolean(tip.IsOpen() ? 1 : 0);
  }

  return NULL;
}

extern "C" int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return 0;

  UIElement elem = winuiGetHandle<UIElement>(ih);
  if (!elem)
    return 0;

  DependencyObject dep = elem.try_as<DependencyObject>();
  if (!dep)
    return 0;

  if (value)
  {
    ToolTip tip{nullptr};
    IInspectable tipObj = ToolTipService::GetToolTip(dep);
    if (tipObj)
      tip = tipObj.try_as<ToolTip>();

    if (tip)
    {
      tip.Content(winrt::box_value(iupwinuiStringToHString(value)));
    }
    else
    {
      tip = ToolTip();
      tip.Content(winrt::box_value(iupwinuiStringToHString(value)));
      ToolTipService::SetToolTip(dep, tip);
    }
  }
  else
  {
    winuiTipTimerStop();
    ToolTipService::SetToolTip(dep, nullptr);
  }

  return 1;
}
