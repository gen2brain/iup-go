/** \file
 * \brief WinUI Driver
 *
 * See Copyright Notice in "iup.h"
 */

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_class.h"
#include "iup_childtree.h"
#include "iup_canvas.h"
#include "iup_image.h"
#include "iup_dlglist.h"
#include "iup_globalattrib.h"
}

#include <string>

#include "iupwinui_drv.h"

#include <winrt/Microsoft.UI.Xaml.Automation.h>
#include <winrt/Microsoft.UI.Xaml.Documents.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Windows::Foundation;

/****************************************************************************
 * Color Management
 ****************************************************************************/

IUP_DRV_API void iupwinuiSetBgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b)
{
  if (!handle)
    return;

  IInspectable obj{nullptr};
  winrt::copy_from_abi(obj, handle);
  Control ctrl = obj.try_as<Control>();
  if (ctrl)
  {
    Windows::UI::Color color;
    color.A = 255;
    color.R = r;
    color.G = g;
    color.B = b;
    ctrl.Background(SolidColorBrush(color));
  }
}

IUP_DRV_API void iupwinuiSetStateBrushes(Ihandle* ih, FrameworkElement const& fe, const wchar_t* const* keys, int count, Windows::UI::Color color)
{
  if (!fe)
    return;

  auto resources = fe.Resources();
  for (int i = 0; i < count; i++)
  {
    char name[100];
    snprintf(name, sizeof(name), "_IUPWINUI_BRUSH_%ls", keys[i]);

    auto key = box_value(keys[i]);
    void* abi = iupAttribGet(ih, name);
    SolidColorBrush current = resources.HasKey(key) ? resources.Lookup(key).try_as<SolidColorBrush>() : nullptr;
    if (abi && current && winrt::get_abi(current) == abi)
    {
      SolidColorBrush brush{nullptr};
      winrt::copy_from_abi(brush, abi);
      brush.Color(color);
    }
    else
    {
      SolidColorBrush brush(color);
      resources.Insert(key, brush);
      if (abi)
      {
        SolidColorBrush old{nullptr};
        winrt::attach_abi(old, abi);
      }
      iupAttribSet(ih, name, static_cast<char*>(winrt::detach_abi(brush)));
    }
  }
}

IUP_DRV_API void iupwinuiReleaseStateBrushes(Ihandle* ih)
{
  char* name = iupTableFirst(ih->attrib);
  while (name)
  {
    if (iupStrEqualPartial(name, "_IUPWINUI_BRUSH_"))
    {
      void* abi = iupTableGetCurr(ih->attrib);
      if (abi)
      {
        SolidColorBrush brush{nullptr};
        winrt::attach_abi(brush, abi);
        iupTableSetCurr(ih->attrib, nullptr, IUPTABLE_POINTER);
      }
    }
    name = iupTableNext(ih->attrib);
  }
}

IUP_DRV_API void iupwinuiApplyAccent(Ihandle* ih)
{
  static const wchar_t* switch_keys[] = {L"ToggleSwitchFillOn", L"ToggleSwitchFillOnPointerOver", L"ToggleSwitchFillOnPressed",
                                         L"ToggleSwitchStrokeOn", L"ToggleSwitchStrokeOnPointerOver", L"ToggleSwitchStrokeOnPressed"};
  unsigned char r, g, b;

  if (!ih->handle || !iupGlobalDefaultColorChanged("ACCENTCOLOR") || !iupStrToRGB(IupGetGlobal("ACCENTCOLOR"), &r, &g, &b))
    return;

  Windows::UI::Color color{255, r, g, b};

  if (IupClassMatch(ih, "progressbar"))
  {
    auto pb = winuiGetHandle<ProgressBar>(ih);
    if (pb && !iupAttribGet(ih, "FGCOLOR"))
      pb.Foreground(SolidColorBrush(color));
    auto ring = winuiGetHandle<ProgressRing>(ih);
    if (ring && !iupAttribGet(ih, "FGCOLOR"))
      ring.Foreground(SolidColorBrush(color));
  }
  else if (IupClassMatch(ih, "toggle") && iupAttribGetBoolean(ih, "SWITCH"))
  {
    auto ts = winuiGetHandle<ToggleSwitch>(ih);
    if (ts)
      iupwinuiSetStateBrushes(ih, ts, switch_keys, 6, color);
  }
}

IUP_DRV_API void iupwinuiSetFgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b)
{
  if (!handle)
    return;

  IInspectable obj{nullptr};
  winrt::copy_from_abi(obj, handle);
  Control ctrl = obj.try_as<Control>();
  if (ctrl)
  {
    Windows::UI::Color color;
    color.A = 255;
    color.R = r;
    color.G = g;
    color.B = b;
    ctrl.Foreground(SolidColorBrush(color));
  }
}

/****************************************************************************
 * Widget Management
 ****************************************************************************/

IUP_DRV_API void iupwinuiAddToParent(Ihandle* ih)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return;

  Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
  if (!parentCanvas)
    return;

  auto elem = winuiGetHandle<UIElement>(ih);
  if (elem)
    parentCanvas.Children().Append(elem);
}

IUP_DRV_API void iupwinuiRemoveFromParent(Ihandle* ih)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return;

  auto elem = winuiGetHandle<UIElement>(ih);
  if (!elem)
    return;

  FrameworkElement fe = elem.try_as<FrameworkElement>();
  if (!fe)
    return;

  Panel panel = fe.Parent().try_as<Panel>();
  if (!panel)
    return;

  uint32_t index;
  if (panel.Children().IndexOf(elem, index))
    panel.Children().RemoveAt(index);
}

/****************************************************************************
 * Base Driver Functions
 ****************************************************************************/

static void winuiSubtractHwndChildren(Ihandle* ih, HRGN rgn, HWND dialogHwnd)
{
  while (ih)
  {
    if (ih->handle && winuiHandleIsHWND(ih) && ih->iclass->nativetype != IUP_TYPEDIALOG && IsWindowVisible(reinterpret_cast<HWND>(ih->handle)))
    {
      RECT r;
      GetWindowRect(reinterpret_cast<HWND>(ih->handle), &r);
      MapWindowPoints(nullptr, dialogHwnd, reinterpret_cast<POINT*>(&r), 2);
      HRGN childRgn = CreateRectRgnIndirect(&r);
      CombineRgn(rgn, rgn, childRgn, RGN_DIFF);
      DeleteObject(childRgn);
    }

    if (ih->firstchild)
      winuiSubtractHwndChildren(ih->firstchild, rgn, dialogHwnd);

    ih = ih->brother;
  }
}

static void winuiUpdateIslandClipRegion(Ihandle* ih)
{
  Ihandle* dialog = IupGetDialog(ih);
  if (!dialog || !dialog->handle)
    return;

  auto* aux = winuiGetAux<IupWinUIDialogAux>(dialog, IUPWINUI_DIALOG_AUX);
  if (!aux || !aux->islandHwnd)
    return;

  RECT rect;
  GetClientRect(reinterpret_cast<HWND>(dialog->handle), &rect);

  HRGN rgn = CreateRectRgn(0, 0, rect.right, rect.bottom);
  winuiSubtractHwndChildren(dialog->firstchild, rgn, reinterpret_cast<HWND>(dialog->handle));
  SetWindowRgn(aux->islandHwnd, rgn, TRUE);
}

static bool winuiElementShown(UIElement const& elem)
{
  DependencyObject obj = elem;
  while (obj)
  {
    UIElement u = obj.try_as<UIElement>();
    if (u && u.Visibility() == Visibility::Collapsed)
      return false;
    obj = Media::VisualTreeHelper::GetParent(obj);
  }
  return true;
}

static void winuiHwndHostPlace(Ihandle* ih, FrameworkElement const& host)
{
  HWND hwnd = reinterpret_cast<HWND>(ih->handle);

  if (!host.IsLoaded() || !winuiElementShown(host) || iupStrEqualNoCase(iupAttribGet(ih, "VISIBLE"), "NO"))
    ShowWindow(hwnd, SW_HIDE);
  else
  {
    double scale = iupwinuiGetScale(ih);
    Windows::Foundation::Point p = host.TransformToVisual(nullptr).TransformPoint(Windows::Foundation::Point{0, 0});
    SetWindowPos(hwnd, nullptr, static_cast<int>(floor(p.X * scale + 0.5)), static_cast<int>(floor(p.Y * scale + 0.5)), ih->currentwidth, ih->currentheight,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
  }

  winuiUpdateIslandClipRegion(ih);
}

static FrameworkElement winuiHwndHostGet(Ihandle* ih)
{
  void* abi = iupAttribGet(ih, "_IUPWINUI_HWNDHOST");
  if (abi)
  {
    Border host{nullptr};
    winrt::copy_from_abi(host, abi);
    return host;
  }

  Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
  if (!parentCanvas)
    return nullptr;

  Border host;
  host.IsHitTestVisible(false);
  parentCanvas.Children().Append(host);

  Ihandle* dialog = IupGetDialog(ih);
  host.LayoutUpdated([ih, dialog, weak = winrt::make_weak(host)](Windows::Foundation::IInspectable const&, Windows::Foundation::IInspectable const&) {
    Border h = weak.get();
    if (!h)
      return;
    if (!iupObjectCheck(ih) || !ih->handle || iupAttribGet(ih, "_IUPWINUI_HWNDHOST") != winrt::get_abi(h))
    {
      Panel panel = Media::VisualTreeHelper::GetParent(h).try_as<Panel>();
      uint32_t index;
      if (panel && panel.Children().IndexOf(h, index))
        panel.Children().RemoveAt(index);
      if (iupObjectCheck(dialog))
        winuiUpdateIslandClipRegion(dialog);
      return;
    }
    winuiHwndHostPlace(ih, h);
  });

  iupAttribSet(ih, "_IUPWINUI_HWNDHOST", static_cast<char*>(winrt::get_abi(host)));
  return host;
}

IUP_DRV_API void iupwinuiHwndHostRemove(Ihandle* ih)
{
  void* abi = iupAttribGet(ih, "_IUPWINUI_HWNDHOST");
  if (!abi)
    return;

  Border host{nullptr};
  winrt::copy_from_abi(host, abi);
  iupAttribSet(ih, "_IUPWINUI_HWNDHOST", nullptr);

  Panel panel = Media::VisualTreeHelper::GetParent(host).try_as<Panel>();
  uint32_t index;
  if (panel && panel.Children().IndexOf(host, index))
    panel.Children().RemoveAt(index);
}

/* XAML positions and sizes are DIPs, IUP computes physical pixels, and the island rasterizes by the monitor scale on top */
IUP_DRV_API UINT iupwinuiGetDpi(Ihandle* ih)
{
  Ihandle* dialog = ih ? IupGetDialog(ih) : nullptr;
  HWND hwnd = (dialog && dialog->handle) ? reinterpret_cast<HWND>(dialog->handle) : nullptr;
  UINT dpi = hwnd ? GetDpiForWindow(hwnd) : 0;
  return dpi ? dpi : static_cast<UINT>(iupdrvGetScreenDpi());
}

IUP_DRV_API double iupwinuiGetScale(Ihandle* ih)
{
  Ihandle* dialog = ih ? IupGetDialog(ih) : nullptr;
  HWND hwnd = (dialog && dialog->handle) ? reinterpret_cast<HWND>(dialog->handle) : nullptr;
  if (!hwnd)
    return 1.0;

  UINT dpi = GetDpiForWindow(hwnd);
  if (!dpi)
    return 1.0;

  return static_cast<double>(dpi) / 96.0;
}

extern "C" IUP_SDK_API void iupdrvBaseLayoutUpdateMethod(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  if (winuiHandleIsHWND(ih))
  {
    HWND hwnd = reinterpret_cast<HWND>(ih->handle);
    FrameworkElement host = (ih->iclass->nativetype != IUP_TYPEDIALOG) ? winuiHwndHostGet(ih) : nullptr;
    if (host)
    {
      double scale = iupwinuiGetScale(ih);
      Canvas::SetLeft(host, ih->x / scale);
      Canvas::SetTop(host, ih->y / scale);
      host.Width(ih->currentwidth / scale);
      host.Height(ih->currentheight / scale);
      winuiHwndHostPlace(ih, host);
      return;
    }

    if (ih->currentwidth > 0 && ih->currentheight > 0)
      SetWindowPos(hwnd, nullptr, ih->x, ih->y, ih->currentwidth, ih->currentheight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    else
      SetWindowPos(hwnd, nullptr, ih->x, ih->y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

    if (ih->iclass->nativetype != IUP_TYPEDIALOG)
      winuiUpdateIslandClipRegion(ih);

    return;
  }

  auto elem = winuiGetHandle<UIElement>(ih);
  if (elem)
  {
    double scale = iupwinuiGetScale(ih);

    Canvas::SetLeft(elem, ih->x / scale);
    Canvas::SetTop(elem, ih->y / scale);

    FrameworkElement fe = elem.try_as<FrameworkElement>();
    if (fe)
    {
      if (ih->currentwidth > 0)
        fe.Width(ih->currentwidth / scale);
      if (ih->currentheight > 0)
        fe.Height(ih->currentheight / scale);
    }
  }
}

extern "C" IUP_SDK_API void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  iupwinuiTipsDestroy(ih);

  if (winuiHandleIsHWND(ih))
  {
    iupwinuiHwndHostRemove(ih);
    return;
  }

  iupwinuiRemoveFromParent(ih);

  winuiReleaseHandle<UIElement>(ih);
}

static int winuiGetDialogMenuHeight(Ihandle* dialog)
{
  auto* aux = winuiGetAux<IupWinUIDialogAux>(dialog, IUPWINUI_DIALOG_AUX);
  if (aux && aux->menuBar)
  {
    double h = aux->menuBar.ActualHeight();
    return (h > 0) ? static_cast<int>(h) : 40;
  }
  return 0;
}

static bool winuiElementClientOrigin(Ihandle* ih, POINT* origin)
{
  if (winuiHandleIsHWND(ih))
    return false;

  UIElement elem = winuiGetHandle<UIElement>(ih);
  if (!elem || !elem.XamlRoot())
    return false;

  double scale = iupwinuiGetScale(ih);
  Windows::Foundation::Point p = elem.TransformToVisual(nullptr).TransformPoint(Windows::Foundation::Point{0, 0});
  origin->x = static_cast<LONG>(floor(p.X * scale + 0.5));
  origin->y = static_cast<LONG>(floor(p.Y * scale + 0.5));
  return true;
}

extern "C" IUP_SDK_API void iupdrvScreenToClient(Ihandle* ih, int* x, int* y)
{
  if (!ih || !ih->handle)
    return;

  Ihandle* dialog = IupGetDialog(ih);
  if (!dialog || !dialog->handle)
    return;

  HWND hwnd = reinterpret_cast<HWND>(dialog->handle);
  POINT p;
  p.x = *x;
  p.y = *y;
  ScreenToClient(hwnd, &p);

  if (ih != dialog)
  {
    POINT origin;
    if (winuiElementClientOrigin(ih, &origin))
    {
      p.x -= origin.x;
      p.y -= origin.y;
    }
    else
    {
      p.x -= ih->x;
      p.y -= ih->y + winuiGetDialogMenuHeight(dialog);
    }
  }

  *x = p.x;
  *y = p.y;
}

extern "C" IUP_SDK_API void iupdrvClientToScreen(Ihandle* ih, int* x, int* y)
{
  if (!ih || !ih->handle)
    return;

  Ihandle* dialog = IupGetDialog(ih);
  if (!dialog || !dialog->handle)
    return;

  HWND hwnd = reinterpret_cast<HWND>(dialog->handle);
  POINT p;
  POINT origin;

  if (ih != dialog && winuiElementClientOrigin(ih, &origin))
  {
    p.x = origin.x + *x;
    p.y = origin.y + *y;
  }
  else if (ih != dialog)
  {
    p.x = ih->x + *x;
    p.y = ih->y + *y + winuiGetDialogMenuHeight(dialog);
  }
  else
  {
    p.x = *x;
    p.y = *y;
  }

  ClientToScreen(hwnd, &p);
  *x = p.x;
  *y = p.y;
}

extern "C" IUP_SDK_API int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return 0;

  auto elem = winuiGetHandle<UIElement>(ih);
  if (elem)
  {
    if (iupStrEqualNoCase(value, "TOP"))
    {
      int z = Canvas::GetZIndex(elem);
      Canvas::SetZIndex(elem, z + 1);
    }
    else if (iupStrEqualNoCase(value, "BOTTOM"))
    {
      int z = Canvas::GetZIndex(elem);
      Canvas::SetZIndex(elem, z - 1);
    }
    return 1;
  }

  return 0;
}

extern "C" IUP_SDK_API void iupdrvSetVisible(Ihandle* ih, int enable)
{
  if (!ih || !ih->handle)
    return;

  if (winuiHandleIsHWND(ih))
  {
    HWND hwnd = reinterpret_cast<HWND>(ih->handle);
    if (hwnd)
      ShowWindow(hwnd, enable ? SW_SHOWNORMAL : SW_HIDE);
    return;
  }

  auto elem = winuiGetHandle<UIElement>(ih);
  if (elem)
    elem.Visibility(enable ? Visibility::Visible : Visibility::Collapsed);
}

extern "C" IUP_SDK_API int iupdrvIsVisible(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return 0;

  if (winuiHandleIsHWND(ih))
  {
    HWND hwnd = reinterpret_cast<HWND>(ih->handle);
    if (hwnd)
      return IsWindowVisible(hwnd);
    return 0;
  }

  auto elem = winuiGetHandle<UIElement>(ih);
  if (elem)
  {
    if (elem.Visibility() != Visibility::Visible)
      return 0;

    return iupwinuiIsInCurrentTab(ih);
  }

  return 0;
}

extern "C" IUP_SDK_API void iupdrvSetActive(Ihandle* ih, int enable)
{
  if (!ih || !ih->handle)
    return;

  if (winuiHandleIsHWND(ih))
  {
    if (IupClassMatch(ih, "dialog"))
    {
      auto* dlgaux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
      if (dlgaux && dlgaux->rootPanel)
        dlgaux->rootPanel.IsHitTestVisible(enable ? true : false);
      return;
    }

    HWND hwnd = reinterpret_cast<HWND>(ih->handle);
    if (hwnd)
      EnableWindow(hwnd, enable);
    return;
  }

  auto ctrl = winuiGetHandle<Control>(ih);
  if (ctrl)
  {
    ctrl.IsEnabled(enable ? true : false);
    return;
  }

  auto elem = winuiGetHandle<UIElement>(ih);
  if (elem)
    elem.IsHitTestVisible(enable ? true : false);
}

extern "C" IUP_SDK_API int iupdrvIsActive(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return 0;

  if (winuiHandleIsHWND(ih))
  {
    HWND hwnd = reinterpret_cast<HWND>(ih->handle);
    if (hwnd)
      return IsWindowEnabled(hwnd);
    return 0;
  }

  auto ctrl = winuiGetHandle<Control>(ih);
  if (ctrl)
    return ctrl.IsEnabled() ? 1 : 0;

  auto elem = winuiGetHandle<UIElement>(ih);
  if (elem)
    return elem.IsHitTestVisible() ? 1 : 0;

  return 1;
}

IUP_DRV_API void iupwinuiCanvasCallAction(Ihandle* ih)
{
  IFn cb = static_cast<IFn>(IupGetCallback(ih, "ACTION"));
  iupAttribSet(ih, "_IUPWINUI_UPDATERECT", nullptr);
  if (cb && !(ih->data->inside_resize) && ih->currentwidth > 0 && ih->currentheight > 0)
  {
    iupAttribSetStrf(ih, "CLIPRECT", "%d %d %d %d", 0, 0, ih->currentwidth - 1, ih->currentheight - 1);
    cb(ih);
    iupAttribSet(ih, "CLIPRECT", nullptr);
  }
}

static void winuiCanvasRedraw(Ihandle* ih)
{
  int x1, y1, x2, y2;
  char* rect = iupAttribGet(ih, "_IUPWINUI_UPDATERECT");
  IFn cb = static_cast<IFn>(IupGetCallback(ih, "ACTION"));
  if (rect && cb && !(ih->data->inside_resize) && ih->currentwidth > 0 && ih->currentheight > 0
      && iupStrToRect(rect, &x1, &y1, &x2, &y2))
  {
    iupAttribSet(ih, "_IUPWINUI_UPDATERECT", nullptr);
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > ih->currentwidth - 1) x2 = ih->currentwidth - 1;
    if (y2 > ih->currentheight - 1) y2 = ih->currentheight - 1;
    if (x1 > x2 || y1 > y2)
      return;
    iupAttribSetStrf(ih, "CLIPRECT", "%d %d %d %d", x1, y1, x2, y2);
    cb(ih);
    iupAttribSet(ih, "CLIPRECT", nullptr);
    return;
  }
  iupwinuiCanvasCallAction(ih);
}

IUP_DRV_API void iupwinuiCanvasQueueRedraw(Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPWINUI_REDRAW_PENDING"))
    return;

  iupAttribSet(ih, "_IUPWINUI_REDRAW_PENDING", "1");

  void* dq_ptr = iupwinuiGetDispatcherQueue();
  if (dq_ptr)
  {
    auto* aux = winuiGetAux<IupWinUICanvasAux>(ih, IUPWINUI_CANVAS_AUX);
    if (!aux)
      return;
    auto alive = aux->alive;

    Windows::Foundation::IInspectable dq_obj{nullptr};
    winrt::copy_from_abi(dq_obj, dq_ptr);
    Microsoft::UI::Dispatching::DispatcherQueue dq = dq_obj.as<Microsoft::UI::Dispatching::DispatcherQueue>();

    dq.TryEnqueue([ih, alive]() {
      if (!*alive)
        return;
      iupAttribSet(ih, "_IUPWINUI_REDRAW_PENDING", nullptr);
      winuiCanvasRedraw(ih);
    });
  }
  else
  {
    iupAttribSet(ih, "_IUPWINUI_REDRAW_PENDING", nullptr);
    winuiCanvasRedraw(ih);
  }
}

extern "C" IUP_SDK_API void iupdrvPostRedraw(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  if (winuiHandleIsHWND(ih))
  {
    HWND hwnd = reinterpret_cast<HWND>(ih->handle);
    if (hwnd)
      InvalidateRect(hwnd, nullptr, FALSE);
    return;
  }

  if (IupClassMatch(ih, "canvas"))
  {
    iupAttribSet(ih, "_IUPWINUI_UPDATERECT", nullptr);
    iupwinuiCanvasQueueRedraw(ih);
    return;
  }

  auto elem = winuiGetHandle<UIElement>(ih);
  if (elem)
    elem.InvalidateArrange();
}

extern "C" IUP_SDK_API void iupdrvRedrawNow(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  if (winuiHandleIsHWND(ih))
  {
    HWND hwnd = reinterpret_cast<HWND>(ih->handle);
    if (hwnd)
      RedrawWindow(hwnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_UPDATENOW);
    return;
  }

  if (IupClassMatch(ih, "canvas"))
  {
    iupwinuiCanvasCallAction(ih);
    return;
  }

  auto elem = winuiGetHandle<UIElement>(ih);
  if (elem)
  {
    elem.InvalidateArrange();
    elem.UpdateLayout();
  }
}

extern "C" IUP_SDK_API void iupdrvReparent(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  if (winuiHandleIsHWND(ih))
  {
    Ihandle* dialog = IupGetDialog(ih);
    if (dialog && dialog->handle)
      SetParent(reinterpret_cast<HWND>(ih->handle), reinterpret_cast<HWND>(dialog->handle));
    return;
  }

  auto elem = winuiGetHandle<UIElement>(ih);
  if (!elem)
    return;

  FrameworkElement fe = elem.try_as<FrameworkElement>();
  if (fe)
  {
    DependencyObject oldParent = fe.Parent();
    if (oldParent)
    {
      Panel oldPanel = oldParent.try_as<Panel>();
      if (oldPanel)
      {
        uint32_t index;
        if (oldPanel.Children().IndexOf(elem, index))
          oldPanel.Children().RemoveAt(index);
      }
    }
  }

  Canvas newParent = iupwinuiGetParentCanvas(ih);
  if (newParent)
    newParent.Children().Append(elem);
}

extern "C" IUP_SDK_API void iupdrvSendKey(int key, int press)
{
  unsigned int keyval, state;
  INPUT input[2];
  WORD state_scan = 0, key_scan;
  ZeroMemory(input, 2 * sizeof(INPUT));

  iupdrvKeyEncode(key, &keyval, &state);
  if (!keyval)
    return;

  LPARAM extra_info = GetMessageExtraInfo();
  if (state)
    state_scan = static_cast<WORD>(MapVirtualKey(state, MAPVK_VK_TO_VSC));
  key_scan = static_cast<WORD>(MapVirtualKey(keyval, MAPVK_VK_TO_VSC));
  DWORD key_flags = iupwinuiKeyIsExtended(key)? KEYEVENTF_EXTENDEDKEY: 0;

  if (press & 0x01)
  {
    if (state)
    {
      input[0].type = INPUT_KEYBOARD;
      input[0].ki.wVk = static_cast<WORD>(state);
      input[0].ki.wScan = state_scan;
      input[0].ki.dwExtraInfo = extra_info;

      input[1].type = INPUT_KEYBOARD;
      input[1].ki.dwFlags = key_flags;
      input[1].ki.wVk = static_cast<WORD>(keyval);
      input[1].ki.wScan = key_scan;
      input[1].ki.dwExtraInfo = extra_info;

      SendInput(2, input, sizeof(INPUT));
    }
    else
    {
      input[0].type = INPUT_KEYBOARD;
      input[0].ki.dwFlags = key_flags;
      input[0].ki.wVk = static_cast<WORD>(keyval);
      input[0].ki.wScan = key_scan;
      input[0].ki.dwExtraInfo = extra_info;

      SendInput(1, input, sizeof(INPUT));
    }
  }

  if (press & 0x02)
  {
    ZeroMemory(input, 2 * sizeof(INPUT));

    if (state)
    {
      input[0].type = INPUT_KEYBOARD;
      input[0].ki.dwFlags = KEYEVENTF_KEYUP | key_flags;
      input[0].ki.wVk = static_cast<WORD>(keyval);
      input[0].ki.wScan = key_scan;
      input[0].ki.dwExtraInfo = extra_info;

      input[1].type = INPUT_KEYBOARD;
      input[1].ki.dwFlags = KEYEVENTF_KEYUP;
      input[1].ki.wVk = static_cast<WORD>(state);
      input[1].ki.wScan = state_scan;
      input[1].ki.dwExtraInfo = extra_info;

      SendInput(2, input, sizeof(INPUT));
    }
    else
    {
      input[0].type = INPUT_KEYBOARD;
      input[0].ki.dwFlags = KEYEVENTF_KEYUP | key_flags;
      input[0].ki.wVk = static_cast<WORD>(keyval);
      input[0].ki.wScan = key_scan;
      input[0].ki.dwExtraInfo = extra_info;

      SendInput(1, input, sizeof(INPUT));
    }
  }
}

extern "C" IUP_SDK_API void iupdrvWarpPointer(int x, int y)
{
  SetCursorPos(x, y);
}

static DWORD winuiGetButtonFlags(int bt, int pressed)
{
  if (pressed)
  {
    switch (bt)
    {
    case IUP_BUTTON1: return MOUSEEVENTF_LEFTDOWN;
    case IUP_BUTTON2: return MOUSEEVENTF_MIDDLEDOWN;
    case IUP_BUTTON3: return MOUSEEVENTF_RIGHTDOWN;
    case IUP_BUTTON4: return MOUSEEVENTF_XDOWN;
    case IUP_BUTTON5: return MOUSEEVENTF_XDOWN;
    }
  }
  else
  {
    switch (bt)
    {
    case IUP_BUTTON1: return MOUSEEVENTF_LEFTUP;
    case IUP_BUTTON2: return MOUSEEVENTF_MIDDLEUP;
    case IUP_BUTTON3: return MOUSEEVENTF_RIGHTUP;
    case IUP_BUTTON4: return MOUSEEVENTF_XUP;
    case IUP_BUTTON5: return MOUSEEVENTF_XUP;
    }
  }
  return 0;
}

extern "C" IUP_SDK_API void iupdrvSendMouse(int x, int y, int bt, int status)
{
  INPUT input;
  int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
  int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
  int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  if (vw <= 0) vw = 1;
  if (vh <= 0) vh = 1;

  SetCursorPos(x, y);

  ZeroMemory(&input, sizeof(INPUT));
  input.type = INPUT_MOUSE;
  input.mi.dx = static_cast<LONG>((static_cast<LONGLONG>(x - vx) * 65535 + vw / 2) / vw);
  input.mi.dy = static_cast<LONG>((static_cast<LONGLONG>(y - vy) * 65535 + vh / 2) / vh);
  input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
  input.mi.dwExtraInfo = GetMessageExtraInfo();

  if (bt != 'W' && status == -1)
  {
    input.mi.dwFlags |= MOUSEEVENTF_MOVE;
  }
  else
  {
    if (bt != 'W')
      input.mi.dwFlags |= winuiGetButtonFlags(bt, status);

    switch (bt)
    {
    case 'W':
      input.mi.mouseData = status * 120;
      input.mi.dwFlags |= MOUSEEVENTF_WHEEL;
      break;
    case IUP_BUTTON4:
      input.mi.mouseData = XBUTTON1;
      break;
    case IUP_BUTTON5:
      input.mi.mouseData = XBUTTON2;
      break;
    }
  }

  if (status == 2)
  {
    SendInput(1, &input, sizeof(INPUT));

    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
    input.mi.dwFlags |= winuiGetButtonFlags(bt, 0);
    SendInput(1, &input, sizeof(INPUT));

    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
    input.mi.dwFlags |= winuiGetButtonFlags(bt, 1);
    SendInput(1, &input, sizeof(INPUT));

    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
    input.mi.dwFlags |= winuiGetButtonFlags(bt, 0);
    SendInput(1, &input, sizeof(INPUT));
  }
  else
    SendInput(1, &input, sizeof(INPUT));
}

extern "C" IUP_SDK_API int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;
  if (winuiHandleIsHWND(ih))
    return 1;
  iupwinuiSetBgColor(ih->handle, r, g, b);
  return 1;
}

extern "C" IUP_SDK_API int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;
  if (winuiHandleIsHWND(ih))
    return 1;
  iupwinuiSetFgColor(ih->handle, r, g, b);
  return 1;
}

static struct {
  const char* iupname;
  LPCTSTR sysid;
  int xamlShape;  /* Microsoft::UI::Input::InputSystemCursorShape or -1 */
} winuiCursorTable[] = {
  { "ARROW",          IDC_ARROW,       0 },   /* Arrow */
  { "BUSY",           IDC_WAIT,        13 },  /* Wait */
  { "CROSS",          IDC_CROSS,       1 },   /* Cross */
  { "HAND",           IDC_HAND,        3 },   /* Hand */
  { "HELP",           IDC_HELP,        4 },   /* Help */
  { "IUP",            IDC_HELP,        4 },   /* Help */
  { "MOVE",           IDC_SIZEALL,     6 },   /* SizeAll */
  { "PEN",            IDC_CROSS,       1 },   /* Cross */
  { "RESIZE_N",       IDC_SIZENS,      8 },   /* SizeNorthSouth */
  { "RESIZE_S",       IDC_SIZENS,      8 },   /* SizeNorthSouth */
  { "RESIZE_NS",      IDC_SIZENS,      8 },   /* SizeNorthSouth */
  { "SPLITTER_HORIZ", IDC_SIZENS,      8 },   /* SizeNorthSouth */
  { "RESIZE_W",       IDC_SIZEWE,      10 },  /* SizeWestEast */
  { "RESIZE_E",       IDC_SIZEWE,      10 },  /* SizeWestEast */
  { "RESIZE_WE",      IDC_SIZEWE,      10 },  /* SizeWestEast */
  { "SPLITTER_VERT",  IDC_SIZEWE,      10 },  /* SizeWestEast */
  { "RESIZE_NE",      IDC_SIZENESW,    7 },   /* SizeNortheastSouthwest */
  { "RESIZE_SW",      IDC_SIZENESW,    7 },   /* SizeNortheastSouthwest */
  { "RESIZE_SE",      IDC_SIZENWSE,    9 },   /* SizeNorthwestSoutheast */
  { "RESIZE_NW",      IDC_SIZENWSE,    9 },   /* SizeNorthwestSoutheast */
  { "TEXT",           IDC_IBEAM,       5 },   /* IBeam */
  { "UPARROW",        IDC_UPARROW,     12 },  /* UpArrow */
  { "APPSTARTING",    IDC_APPSTARTING,  16 },  /* AppStarting */
  { "NO",             IDC_NO,          11 },   /* UniversalNo */
  { "NONE",           nullptr,            -1 },
  { "NULL",           nullptr,            -1 }
};

static HCURSOR winuiGetCursor(Ihandle* ih, const char* name)
{
  int i, count = sizeof(winuiCursorTable) / sizeof(winuiCursorTable[0]);

  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(name, winuiCursorTable[i].iupname))
    {
      if (!winuiCursorTable[i].sysid)
        return nullptr;
      return LoadCursor(nullptr, winuiCursorTable[i].sysid);
    }
  }

  auto cur = static_cast<HCURSOR>(iupImageGetCursor(name));
  if (cur)
    return cur;

  (void)ih;
  return LoadCursor(nullptr, IDC_ARROW);
}

struct IInputCursorStaticsInterop : ::IUnknown
{
  virtual HRESULT __stdcall GetIids(ULONG* count, GUID** iids) = 0;
  virtual HRESULT __stdcall GetRuntimeClassName(void** name) = 0;
  virtual HRESULT __stdcall GetTrustLevel(int* level) = 0;
  virtual HRESULT __stdcall CreateFromHCursor(HCURSOR hcursor, void** inputCursor) = 0;
};

static Microsoft::UI::Input::InputCursor winuiCreateInputCursor(HCURSOR hcursor)
{
  static const GUID iid = {0xac6f5065, 0x90c4, 0x46ce, {0xbe, 0xb7, 0x05, 0xe1, 0x38, 0xe5, 0x41, 0x17}};
  Microsoft::UI::Input::InputCursor cursor{nullptr};

  auto factory = winrt::get_activation_factory<Microsoft::UI::Input::InputCursor>();
  IInputCursorStaticsInterop* interop = nullptr;
  if (FAILED(((::IUnknown*)winrt::get_abi(factory))->QueryInterface(iid, (void**)&interop)))
    return cursor;

  void* abi = nullptr;
  if (SUCCEEDED(interop->CreateFromHCursor(hcursor, &abi)) && abi)
    cursor = Microsoft::UI::Input::InputCursor{abi, winrt::take_ownership_from_abi};
  interop->Release();
  return cursor;
}

static void winuiSetXamlCursor(UIElement const& element, const char* name)
{
  using namespace Microsoft::UI::Input;

  int shape = -1;
  bool found = false;
  int i, count = sizeof(winuiCursorTable) / sizeof(winuiCursorTable[0]);

  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(name, winuiCursorTable[i].iupname))
    {
      shape = winuiCursorTable[i].xamlShape;
      found = true;
      break;
    }
  }

  if (!element)
    return;

  auto protectedUI = element.try_as<Microsoft::UI::Xaml::IUIElementProtected>();
  if (!protectedUI)
    return;

  if (!found || shape < 0)
  {
    HCURSOR hcursor;
    if (found)
    {
      static HCURSOR blank = nullptr;
      if (!blank)
      {
        BYTE and_mask[4] = {0xFF, 0xFF, 0xFF, 0xFF};
        BYTE xor_mask[4] = {0, 0, 0, 0};
        blank = CreateCursor(GetModuleHandle(nullptr), 0, 0, 1, 1, and_mask, xor_mask);
      }
      hcursor = blank;
    }
    else
      hcursor = static_cast<HCURSOR>(iupImageGetCursor(name));

    InputCursor cursor = hcursor ? winuiCreateInputCursor(hcursor) : InputCursor{nullptr};
    if (cursor)
    {
      protectedUI.ProtectedCursor(cursor);
      return;
    }
  }

  if (shape < 0)
    shape = 0;  /* Arrow */

  protectedUI.ProtectedCursor(InputSystemCursor::Create(static_cast<InputSystemCursorShape>(shape)));
}

extern "C" IUP_SDK_API int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle || !value)
    return 0;

  if (winuiHandleIsHWND(ih))
  {
    HCURSOR hCur = winuiGetCursor(ih, value);
    iupAttribSet(ih, "_IUPWIN_HCURSOR", reinterpret_cast<char*>(hCur));
    if (hCur)
      SetCursor(hCur);

    if (ih->iclass->nativetype == IUP_TYPEDIALOG)
    {
      auto* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
      if (aux && aux->rootPanel)
        winuiSetXamlCursor(aux->rootPanel, value);
    }
  }
  else
    winuiSetXamlCursor(winuiGetHandle<UIElement>(ih), value);

  return 1;
}

extern "C" IUP_SDK_API int iupdrvGetScrollbarSize(void)
{
  UINT dpi = static_cast<UINT>(iupdrvGetScreenDpi());
  int xv = GetSystemMetricsForDpi(SM_CXVSCROLL, dpi);
  int yh = GetSystemMetricsForDpi(SM_CYHSCROLL, dpi);
  return xv > yh ? xv : yh;
}

extern "C" IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle* ih, const char* title)
{
  if (!ih || !ih->handle)
    return;

  if (winuiHandleIsHWND(ih))
  {
    HWND hwnd = reinterpret_cast<HWND>(ih->handle);
    if (hwnd)
    {
      if (!title)
        SetWindowTextA(hwnd, "");
      else
        SetWindowTextA(hwnd, title);
    }
    return;
  }

  auto elem = winuiGetHandle<UIElement>(ih);
  if (elem)
  {
    DependencyObject dep = elem.try_as<DependencyObject>();
    if (dep)
    {
      hstring name = title ? iupwinuiStringToHString(title) : hstring();
      Automation::AutomationProperties::SetName(dep, name);
    }
  }
}

/* a Border content stops UIA deriving the button name from its caption */
IUP_DRV_API void iupwinuiSetAutomationName(Ihandle* ih, const char* title)
{
  char* stripped;

  if (!ih || !ih->handle || iupAttribGet(ih, "ACCESSIBLETITLE"))
    return;

  stripped = title ? iupStrProcessMnemonic(title, nullptr, 0) : nullptr;
  iupdrvSetAccessibleTitle(ih, stripped ? stripped : title);
  if (stripped && stripped != title)
    free(stripped);
}

extern "C" IUP_SDK_API void iupdrvSetAccessibleDescription(Ihandle* ih, const char* description)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return;

  auto elem = winuiGetHandle<UIElement>(ih);
  if (elem)
  {
    DependencyObject dep = elem.try_as<DependencyObject>();
    if (dep)
    {
      hstring help = description ? iupwinuiStringToHString(description) : hstring();
      Automation::AutomationProperties::SetHelpText(dep, help);
    }
  }
}

extern "C" IUP_SDK_API void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
  (void)ic;
}

extern "C" IUP_SDK_API void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "TIPMARKUP", nullptr, nullptr, IUPAF_SAMEASSYSTEM, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TIPICON", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TIPDELAY", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TIPRECT", nullptr, nullptr, nullptr, nullptr, IUPAF_DEFAULT);
}

IUP_DRV_API Canvas iupwinuiGetParentCanvas(Ihandle* ih)
{
  Ihandle* parent = iupChildTreeGetNativeParent(ih);
  if (!parent || !parent->handle)
    return nullptr;

  if (IupClassMatch(parent, "dialog"))
  {
    auto* aux = winuiGetAux<IupWinUIDialogAux>(parent, IUPWINUI_DIALOG_AUX);
    if (aux)
      return aux->contentCanvas;
  }
  else if (IupClassMatch(parent, "frame"))
  {
    auto* aux = winuiGetAux<IupWinUIFrameAux>(parent, IUPWINUI_FRAME_AUX);
    if (aux)
      return aux->innerCanvas;
  }
  else if (IupClassMatch(parent, "popover"))
  {
    auto* aux = winuiGetAux<IupWinUIPopoverAux>(parent, IUPWINUI_POPOVER_AUX);
    if (aux)
      return aux->innerCanvas;
  }
  else if (IupClassMatch(parent, "tabs"))
  {
    Ihandle* tabChild = ih;
    while (tabChild && tabChild->parent != parent)
      tabChild = tabChild->parent;

    if (tabChild)
    {
      char* containerPtr = iupAttribGet(tabChild, "_IUPTAB_CONTAINER");
      if (containerPtr)
      {
        IInspectable obj{nullptr};
        winrt::copy_from_abi(obj, containerPtr);
        return obj.try_as<Canvas>();
      }
    }
  }

  return winuiGetHandle<Canvas>(parent);
}

/****************************************************************************
 * String Conversion (UTF-8/UTF-16)
 ****************************************************************************/

IUP_DRV_API hstring iupwinuiStringToHString(const char* str)
{
  if (!str || !str[0])
    return {};

  int wlen = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
  if (wlen <= 0)
    return {};

  std::wstring wstr(wlen - 1, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, str, -1, &wstr[0], wlen);

  return hstring(wstr);
}

IUP_DRV_API char* iupwinuiHStringToString(const hstring& str)
{
  if (str.empty())
    return nullptr;

  int len = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
  if (len <= 0)
    return nullptr;

  char* buf = iupStrGetMemory(len);
  WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, buf, len, nullptr, nullptr);

  return buf;
}

IUP_DRV_API std::wstring iupwinuiStringToWString(const char* str)
{
  if (!str || !str[0])
    return {};

  int wlen = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
  if (wlen <= 0)
    return {};

  std::wstring wstr(wlen - 1, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, str, -1, &wstr[0], wlen);

  return wstr;
}

IUP_DRV_API hstring iupwinuiProcessMnemonic(const char* str, char* c)
{
  if (c)
    *c = 0;

  if (!str || !str[0])
    return {};

  char mnemonic = 0;
  char* processed = iupStrProcessMnemonic(str, &mnemonic, -1);
  if (!processed)
    return {};

  hstring result = iupwinuiStringToHString(processed);

  if (processed != str)
    free(processed);

  if (c)
    *c = mnemonic;

  return result;
}

static bool winui_accel_alt_down = false;

IUP_DRV_API int iupwinuiShowAccelCues()
{
  BOOL always = FALSE;
  SystemParametersInfo(SPI_GETKEYBOARDCUES, 0, &always, 0);
  if (always)
    return 1;

  return winui_accel_alt_down ? 1 : 0;
}

IUP_DRV_API hstring iupwinuiTextBlockText(TextBlock const& tb)
{
  if (!tb || tb.Inlines().Size() == 0)
    return tb ? tb.Text() : hstring();

  std::wstring text;
  for (auto const& inl : tb.Inlines())
  {
    if (auto run = inl.try_as<Documents::Run>())
      text += std::wstring(run.Text());
    else if (auto span = inl.try_as<Documents::Span>())
    {
      for (auto const& inner : span.Inlines())
        if (auto run2 = inner.try_as<Documents::Run>())
          text += std::wstring(run2.Text());
    }
  }
  return hstring(text);
}

static void winuiBuildMnemonicInlines(TextBlock const& tb, std::wstring const& clean, int idx)
{
  tb.Text(hstring{});
  tb.Inlines().Clear();

  if (idx < 0 || !iupwinuiShowAccelCues())
  {
    Documents::Run run;
    run.Text(hstring(clean));
    tb.Inlines().Append(run);
    return;
  }

  if (idx > 0)
  {
    Documents::Run pre;
    pre.Text(hstring(clean.substr(0, idx)));
    tb.Inlines().Append(pre);
  }

  Documents::Underline underline;
  Documents::Run mnemonic;
  mnemonic.Text(hstring(clean.substr(idx, 1)));
  underline.Inlines().Append(mnemonic);
  tb.Inlines().Append(underline);

  if (static_cast<size_t>(idx + 1) < clean.size())
  {
    Documents::Run post;
    post.Text(hstring(clean.substr(idx + 1)));
    tb.Inlines().Append(post);
  }
}

IUP_DRV_API void iupwinuiSetMnemonicText(TextBlock const& tb, const char* title, char* c)
{
  if (c)
    *c = 0;
  if (!tb)
    return;
  if (!title)
    title = "";

  char mnemonic = 0;
  char* processed = iupStrProcessMnemonic(title, &mnemonic, -1);
  std::string clean = processed ? processed : "";

  int byte_idx = -1;
  if (mnemonic)
  {
    int ci = 0;
    for (const char* p = title; *p; p++)
    {
      if (*p == '&')
      {
        if (*(p + 1) == '&') { ci++; p++; }
        else { byte_idx = ci; break; }
      }
      else
        ci++;
    }
  }

  if (processed && processed != title)
    free(processed);

  std::wstring clean_w(iupwinuiStringToHString(clean.c_str()).c_str());
  int idx = -1;
  if (byte_idx >= 0)
    idx = static_cast<int>(std::wstring(iupwinuiStringToHString(clean.substr(0, byte_idx).c_str()).c_str()).size());

  tb.Tag(idx >= 0 ? box_value(static_cast<int32_t>(idx)) : nullptr);
  winuiBuildMnemonicInlines(tb, clean_w, idx);

  if (c)
    *c = mnemonic;
}

IUP_DRV_API ScrollViewer iupwinuiFindScrollViewer(DependencyObject const& parent)
{
  if (!parent)
    return nullptr;

  int count = VisualTreeHelper::GetChildrenCount(parent);
  for (int i = 0; i < count; i++)
  {
    DependencyObject child = VisualTreeHelper::GetChild(parent, i);

    ScrollViewer sv = child.try_as<ScrollViewer>();
    if (sv)
      return sv;

    sv = iupwinuiFindScrollViewer(child);
    if (sv)
      return sv;
  }

  return nullptr;
}

IUP_DRV_API char* iupwinuiScrollViewerVisible(ScrollViewer const& sv)
{
  if (!sv)
    return const_cast<char*>("NO");

  int sb_h = (sv.ComputedHorizontalScrollBarVisibility() == Visibility::Visible) ? 1 : 0;
  int sb_v = (sv.ComputedVerticalScrollBarVisibility() == Visibility::Visible) ? 1 : 0;

  if (sb_h && sb_v) return const_cast<char*>("YES");
  if (sb_h) return const_cast<char*>("HORIZONTAL");
  if (sb_v) return const_cast<char*>("VERTICAL");
  return const_cast<char*>("NO");
}

static void winuiRefreshTextBlocks(Windows::Foundation::IInspectable const& node)
{
  if (!node)
    return;

  int count = VisualTreeHelper::GetChildrenCount(node.as<DependencyObject>());
  for (int i = 0; i < count; i++)
  {
    DependencyObject child = VisualTreeHelper::GetChild(node.as<DependencyObject>(), i);

    TextBlock tb = child.try_as<TextBlock>();
    if (tb && tb.Tag())
    {
      auto boxed = tb.Tag().try_as<Windows::Foundation::IReference<int32_t>>();
      if (boxed)
      {
        std::wstring clean(iupwinuiTextBlockText(tb).c_str());
        winuiBuildMnemonicInlines(tb, clean, boxed.Value());
      }
    }

    winuiRefreshTextBlocks(child);
  }
}

IUP_DRV_API void iupwinuiRefreshAccelCues()
{
  for (Ihandle* dlg = iupDlgListFirst(); dlg; dlg = iupDlgListNext())
  {
    if (!IupClassMatch(dlg, "dialog"))
      continue;

    auto* aux = winuiGetAux<IupWinUIDialogAux>(dlg, IUPWINUI_DIALOG_AUX);
    if (aux && aux->rootPanel)
      winuiRefreshTextBlocks(aux->rootPanel);
  }
}

IUP_DRV_API void iupwinuiSetAccelCueAlt(int down)
{
  bool d = down ? true : false;
  if (winui_accel_alt_down == d)
    return;

  winui_accel_alt_down = d;

  BOOL always = FALSE;
  SystemParametersInfo(SPI_GETKEYBOARDCUES, 0, &always, 0);
  if (always)
    return;

  iupwinuiRefreshAccelCues();
}
