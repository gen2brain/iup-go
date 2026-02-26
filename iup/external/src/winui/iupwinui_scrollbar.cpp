/** \file
 * \brief WinUI Driver - Scrollbar Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <cmath>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_scrollbar.h"
#include "iup_register.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Input;
using namespace Windows::Foundation;


struct IupWinUIScrollbarAux
{
  event_token scrollToken;
  bool ignore_changed;

  IupWinUIScrollbarAux() : scrollToken{}, ignore_changed(false) {}
};

#define IUPWINUI_SCROLLBAR_AUX "_IUPWINUI_SCROLLBAR_AUX"

static void winuiScrollbarUpdateValue(Ihandle* ih, ScrollEventType scrollType)
{
  IupWinUIScrollbarAux* aux = winuiGetAux<IupWinUIScrollbarAux>(ih, IUPWINUI_SCROLLBAR_AUX);
  if (!aux || aux->ignore_changed)
    return;

  double old_val = ih->data->val;

  ScrollBar sb = winuiGetHandle<ScrollBar>(ih);
  if (!sb)
    return;

  ih->data->val = sb.Value();
  iupScrollbarCropValue(ih);

  int op = -1;
  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    switch (scrollType)
    {
    case ScrollEventType::SmallDecrement: op = IUP_SBLEFT; break;
    case ScrollEventType::SmallIncrement: op = IUP_SBRIGHT; break;
    case ScrollEventType::LargeDecrement: op = IUP_SBPGLEFT; break;
    case ScrollEventType::LargeIncrement: op = IUP_SBPGRIGHT; break;
    case ScrollEventType::ThumbTrack:     op = IUP_SBDRAGH; break;
    case ScrollEventType::ThumbPosition:  op = IUP_SBPOSH; break;
    default:                              op = IUP_SBPOSH; break;
    }
  }
  else
  {
    switch (scrollType)
    {
    case ScrollEventType::SmallDecrement: op = IUP_SBUP; break;
    case ScrollEventType::SmallIncrement: op = IUP_SBDN; break;
    case ScrollEventType::LargeDecrement: op = IUP_SBPGUP; break;
    case ScrollEventType::LargeIncrement: op = IUP_SBPGDN; break;
    case ScrollEventType::ThumbTrack:     op = IUP_SBDRAGV; break;
    case ScrollEventType::ThumbPosition:  op = IUP_SBPOSV; break;
    default:                              op = IUP_SBPOSV; break;
    }
  }

  IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
  if (scroll_cb)
  {
    float posx = 0, posy = 0;
    if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
      posx = (float)ih->data->val;
    else
      posy = (float)ih->data->val;

    scroll_cb(ih, op, posx, posy);
  }

  IFn valuechanged_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (valuechanged_cb)
  {
    if (ih->data->val != old_val)
    {
      if (valuechanged_cb(ih) == IUP_CLOSE)
        IupExitLoop();
    }
  }
}

static int winuiScrollbarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    ScrollBar sb = winuiGetHandle<ScrollBar>(ih);
    if (sb)
    {
      IupWinUIScrollbarAux* aux = winuiGetAux<IupWinUIScrollbarAux>(ih, IUPWINUI_SCROLLBAR_AUX);

      iupScrollbarCropValue(ih);

      if (aux) aux->ignore_changed = true;
      sb.Value(ih->data->val);
      if (aux) aux->ignore_changed = false;
    }
  }
  return 0;
}

static int winuiScrollbarSetLineStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->linestep), 0.01))
  {
    ScrollBar sb = winuiGetHandle<ScrollBar>(ih);
    if (sb)
      sb.SmallChange(ih->data->linestep);
  }
  return 0;
}

static int winuiScrollbarSetPageStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1))
  {
    ScrollBar sb = winuiGetHandle<ScrollBar>(ih);
    if (sb)
      sb.LargeChange(ih->data->pagestep);
  }
  return 0;
}

static int winuiScrollbarSetPageSizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagesize), 0.1))
  {
    ScrollBar sb = winuiGetHandle<ScrollBar>(ih);
    if (sb)
    {
      iupScrollbarCropValue(ih);
      sb.ViewportSize(ih->data->pagesize);
      sb.Maximum(ih->data->vmax - ih->data->pagesize);
    }
  }
  return 0;
}


/*********************************************************************************************/


static void winuiScrollbarForceVisible(ScrollBar sb)
{
  sb.IndicatorMode(ScrollingIndicatorMode::MouseIndicator);

  sb.Loaded([](IInspectable const& sender, RoutedEventArgs const&) {
    auto ctrl = sender.as<Controls::Control>();
    VisualStateManager::GoToState(ctrl, L"MouseIndicatorFull", false);

    auto root = VisualTreeHelper::GetChild(ctrl, 0);
    if (!root)
      return;

    auto rootFE = root.as<FrameworkElement>();
    auto groups = VisualStateManager::GetVisualStateGroups(rootFE);
    for (uint32_t i = 0; i < groups.Size(); i++)
    {
      auto group = groups.GetAt(i);
      if (group.Name() == L"ScrollingIndicatorStates")
      {
        group.CurrentStateChanged([ctrl](IInspectable const&, VisualStateChangedEventArgs const&) {
          VisualStateManager::GoToState(ctrl, L"MouseIndicatorFull", false);
        });
        break;
      }
    }
  });
}

static int winuiScrollbarMapMethod(Ihandle* ih)
{
  IupWinUIScrollbarAux* aux = new IupWinUIScrollbarAux();

  ScrollBar sb = ScrollBar();
  sb.HorizontalAlignment(HorizontalAlignment::Left);
  sb.VerticalAlignment(VerticalAlignment::Top);

  sb.Minimum(ih->data->vmin);
  sb.Maximum(ih->data->vmax - ih->data->pagesize);
  sb.Value(ih->data->val);
  sb.ViewportSize(ih->data->pagesize);
  sb.SmallChange(ih->data->linestep);
  sb.LargeChange(ih->data->pagestep);

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
    sb.Orientation(Orientation::Horizontal);
  else
    sb.Orientation(Orientation::Vertical);

  winuiScrollbarForceVisible(sb);

  aux->scrollToken = sb.Scroll([ih](IInspectable const&, ScrollEventArgs const& args) {
    winuiScrollbarUpdateValue(ih, args.ScrollEventType());
  });

  Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
  if (parentCanvas)
    parentCanvas.Children().Append(sb);

  winuiSetAux(ih, IUPWINUI_SCROLLBAR_AUX, aux);
  winuiStoreHandle(ih, sb);
  return IUP_NOERROR;
}

static void winuiScrollbarUnMapMethod(Ihandle* ih)
{
  IupWinUIScrollbarAux* aux = winuiGetAux<IupWinUIScrollbarAux>(ih, IUPWINUI_SCROLLBAR_AUX);

  if (ih->handle && aux)
  {
    ScrollBar sb = winuiGetHandle<ScrollBar>(ih);
    if (sb)
    {
      if (aux->scrollToken)
        sb.Scroll(aux->scrollToken);
    }
    winuiReleaseHandle<ScrollBar>(ih);
  }

  winuiFreeAux<IupWinUIScrollbarAux>(ih, IUPWINUI_SCROLLBAR_AUX);
  ih->handle = NULL;
}

extern "C" void iupdrvScrollbarGetMinSize(Ihandle* ih, int* w, int* h)
{
  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    *w = 100;
    *h = 16;
  }
  else
  {
    *w = 16;
    *h = 100;
  }
}

extern "C" void iupdrvScrollbarInitClass(Iclass* ic)
{
  ic->Map = winuiScrollbarMapMethod;
  ic->UnMap = winuiScrollbarUnMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", iupScrollbarGetValueAttrib, winuiScrollbarSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINESTEP", iupScrollbarGetLineStepAttrib, winuiScrollbarSetLineStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupScrollbarGetPageStepAttrib, winuiScrollbarSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESIZE", iupScrollbarGetPageSizeAttrib, winuiScrollbarSetPageSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
