/** \file
 * \brief WinUI Driver
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
#include "iup_class.h"
#include "iup_childtree.h"
#include "iup_canvas.h"
#include "iup_image.h"
}

#include "iupwinui_drv.h"

#include <winrt/Microsoft.UI.Xaml.Automation.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Windows::Foundation;

/****************************************************************************
 * Color Management
 ****************************************************************************/

extern "C" void iupwinuiSetBgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b)
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

extern "C" void iupwinuiSetFgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b)
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

extern "C" void iupwinuiAddToParent(Ihandle* ih)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return;

  Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
  if (!parentCanvas)
    return;

  UIElement elem = winuiGetHandle<UIElement>(ih);
  if (elem)
    parentCanvas.Children().Append(elem);
}

/****************************************************************************
 * Base Driver Functions
 ****************************************************************************/

static void winuiSubtractHwndChildren(Ihandle* ih, HRGN rgn)
{
  while (ih)
  {
    if (ih->handle && winuiHandleIsHWND(ih) && ih->iclass->nativetype != IUP_TYPEDIALOG)
    {
      if (ih->currentwidth > 0 && ih->currentheight > 0)
      {
        HRGN childRgn = CreateRectRgn(ih->x, ih->y, ih->x + ih->currentwidth, ih->y + ih->currentheight);
        CombineRgn(rgn, rgn, childRgn, RGN_DIFF);
        DeleteObject(childRgn);
      }
    }

    if (ih->firstchild)
      winuiSubtractHwndChildren(ih->firstchild, rgn);

    ih = ih->brother;
  }
}

static void winuiUpdateIslandClipRegion(Ihandle* ih)
{
  Ihandle* dialog = IupGetDialog(ih);
  if (!dialog || !dialog->handle)
    return;

  IupWinUIDialogAux* aux = winuiGetAux<IupWinUIDialogAux>(dialog, IUPWINUI_DIALOG_AUX);
  if (!aux || !aux->islandHwnd)
    return;

  RECT rect;
  GetClientRect((HWND)dialog->handle, &rect);

  HRGN rgn = CreateRectRgn(0, 0, rect.right, rect.bottom);
  winuiSubtractHwndChildren(dialog->firstchild, rgn);
  SetWindowRgn(aux->islandHwnd, rgn, TRUE);
}

extern "C" void iupdrvBaseLayoutUpdateMethod(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  if (winuiHandleIsHWND(ih))
  {
    HWND hwnd = (HWND)ih->handle;
    if (ih->currentwidth > 0 && ih->currentheight > 0)
      SetWindowPos(hwnd, NULL, ih->x, ih->y, ih->currentwidth, ih->currentheight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    else
      SetWindowPos(hwnd, NULL, ih->x, ih->y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

    if (ih->iclass->nativetype != IUP_TYPEDIALOG)
      winuiUpdateIslandClipRegion(ih);

    return;
  }

  UIElement elem = winuiGetHandle<UIElement>(ih);
  if (elem)
  {
    Canvas::SetLeft(elem, ih->x);
    Canvas::SetTop(elem, ih->y);

    FrameworkElement fe = elem.try_as<FrameworkElement>();
    if (fe)
    {
      if (ih->currentwidth > 0)
        fe.Width(ih->currentwidth);
      if (ih->currentheight > 0)
        fe.Height(ih->currentheight);
    }
  }
}

extern "C" void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return;

  UIElement elem = winuiGetHandle<UIElement>(ih);
  if (elem)
  {
    FrameworkElement fe = elem.try_as<FrameworkElement>();
    if (fe)
    {
      DependencyObject parent = fe.Parent();
      if (parent)
      {
        Panel panel = parent.try_as<Panel>();
        if (panel)
        {
          uint32_t index;
          if (panel.Children().IndexOf(elem, index))
            panel.Children().RemoveAt(index);
        }
      }
    }
  }

  winuiReleaseHandle<UIElement>(ih);
}

static int winuiGetDialogMenuHeight(Ihandle* dialog)
{
  IupWinUIDialogAux* aux = winuiGetAux<IupWinUIDialogAux>(dialog, IUPWINUI_DIALOG_AUX);
  if (aux && aux->menuBar)
    return 40;
  return 0;
}

extern "C" void iupdrvScreenToClient(Ihandle* ih, int *x, int *y)
{
  if (!ih || !ih->handle)
    return;

  Ihandle* dialog = IupGetDialog(ih);
  if (!dialog || !dialog->handle)
    return;

  HWND hwnd = (HWND)dialog->handle;
  POINT p;
  p.x = *x;
  p.y = *y;
  ScreenToClient(hwnd, &p);

  if (ih != dialog)
  {
    p.x -= ih->x;
    p.y -= ih->y + winuiGetDialogMenuHeight(dialog);
  }

  *x = p.x;
  *y = p.y;
}

extern "C" void iupdrvClientToScreen(Ihandle* ih, int *x, int *y)
{
  if (!ih || !ih->handle)
    return;

  Ihandle* dialog = IupGetDialog(ih);
  if (!dialog || !dialog->handle)
    return;

  HWND hwnd = (HWND)dialog->handle;
  POINT p;

  if (ih != dialog)
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

extern "C" int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return 0;

  UIElement elem = winuiGetHandle<UIElement>(ih);
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

extern "C" void iupdrvSetVisible(Ihandle* ih, int visible)
{
  if (!ih || !ih->handle)
    return;

  if (winuiHandleIsHWND(ih))
  {
    HWND hwnd = (HWND)ih->handle;
    if (hwnd)
      ShowWindow(hwnd, visible ? SW_SHOWNORMAL : SW_HIDE);
    return;
  }

  UIElement elem = winuiGetHandle<UIElement>(ih);
  if (elem)
    elem.Visibility(visible ? Visibility::Visible : Visibility::Collapsed);
}

extern "C" int iupdrvIsVisible(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return 0;

  if (winuiHandleIsHWND(ih))
  {
    HWND hwnd = (HWND)ih->handle;
    if (hwnd)
      return IsWindowVisible(hwnd);
    return 0;
  }

  UIElement elem = winuiGetHandle<UIElement>(ih);
  if (elem)
    return elem.Visibility() == Visibility::Visible ? 1 : 0;

  return 0;
}

extern "C" void iupdrvSetActive(Ihandle* ih, int active)
{
  if (!ih || !ih->handle)
    return;

  if (winuiHandleIsHWND(ih))
  {
    HWND hwnd = (HWND)ih->handle;
    if (hwnd)
      EnableWindow(hwnd, active);
    return;
  }

  Control ctrl = winuiGetHandle<Control>(ih);
  if (ctrl)
    ctrl.IsEnabled(active ? true : false);
}

extern "C" int iupdrvIsActive(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return 0;

  if (winuiHandleIsHWND(ih))
  {
    HWND hwnd = (HWND)ih->handle;
    if (hwnd)
      return IsWindowEnabled(hwnd);
    return 0;
  }

  Control ctrl = winuiGetHandle<Control>(ih);
  if (ctrl)
    return ctrl.IsEnabled() ? 1 : 0;

  return 1;
}

void iupwinuiCanvasCallAction(Ihandle* ih)
{
  IFn cb = (IFn)IupGetCallback(ih, "ACTION");
  if (cb && !(ih->data->inside_resize) && ih->currentwidth > 0 && ih->currentheight > 0)
  {
    iupAttribSetStrf(ih, "CLIPRECT", "%d %d %d %d", 0, 0, ih->currentwidth, ih->currentheight);
    cb(ih);
    iupAttribSet(ih, "CLIPRECT", NULL);
  }
}

extern "C" void iupdrvPostRedraw(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  if (winuiHandleIsHWND(ih))
  {
    HWND hwnd = (HWND)ih->handle;
    if (hwnd)
      InvalidateRect(hwnd, NULL, FALSE);
    return;
  }

  if (IupClassMatch(ih, "canvas"))
  {
    if (iupAttribGet(ih, "_IUPWINUI_REDRAW_PENDING"))
      return;

    iupAttribSet(ih, "_IUPWINUI_REDRAW_PENDING", "1");

    void* dq_ptr = iupwinuiGetDispatcherQueue();
    if (dq_ptr)
    {
      IupWinUICanvasAux* aux = winuiGetAux<IupWinUICanvasAux>(ih, IUPWINUI_CANVAS_AUX);
      if (!aux)
        return;
      auto alive = aux->alive;

      Windows::Foundation::IInspectable dq_obj{nullptr};
      winrt::copy_from_abi(dq_obj, dq_ptr);
      Microsoft::UI::Dispatching::DispatcherQueue dq = dq_obj.as<Microsoft::UI::Dispatching::DispatcherQueue>();

      dq.TryEnqueue([ih, alive]() {
        if (!*alive)
          return;
        iupAttribSet(ih, "_IUPWINUI_REDRAW_PENDING", NULL);
        iupwinuiCanvasCallAction(ih);
      });
    }
    else
    {
      iupAttribSet(ih, "_IUPWINUI_REDRAW_PENDING", NULL);
      iupwinuiCanvasCallAction(ih);
    }
    return;
  }

  UIElement elem = winuiGetHandle<UIElement>(ih);
  if (elem)
    elem.InvalidateArrange();
}

extern "C" void iupdrvRedrawNow(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  if (winuiHandleIsHWND(ih))
  {
    HWND hwnd = (HWND)ih->handle;
    if (hwnd)
      RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_UPDATENOW);
    return;
  }

  if (IupClassMatch(ih, "canvas"))
  {
    iupwinuiCanvasCallAction(ih);
    return;
  }

  UIElement elem = winuiGetHandle<UIElement>(ih);
  if (elem)
  {
    elem.InvalidateArrange();
    elem.UpdateLayout();
  }
}

extern "C" void iupdrvReparent(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  if (winuiHandleIsHWND(ih))
  {
    Ihandle* dialog = IupGetDialog(ih);
    if (dialog && dialog->handle)
      SetParent((HWND)ih->handle, (HWND)dialog->handle);
    return;
  }

  UIElement elem = winuiGetHandle<UIElement>(ih);
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

extern "C" void iupdrvSendKey(int key, int press)
{
  INPUT input;
  ZeroMemory(&input, sizeof(INPUT));
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = (WORD)key;
  input.ki.dwFlags = press ? 0 : KEYEVENTF_KEYUP;
  SendInput(1, &input, sizeof(INPUT));
}

extern "C" void iupdrvWarpPointer(int x, int y)
{
  SetCursorPos(x, y);
}

extern "C" void iupdrvSendMouse(int x, int y, int bt, int status)
{
  INPUT input;
  ZeroMemory(&input, sizeof(INPUT));
  input.type = INPUT_MOUSE;
  input.mi.dx = x * (65535 / GetSystemMetrics(SM_CXSCREEN));
  input.mi.dy = y * (65535 / GetSystemMetrics(SM_CYSCREEN));
  input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;

  if (status == 1)
  {
    if (bt == IUP_BUTTON1)
      input.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
    else if (bt == IUP_BUTTON2)
      input.mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
    else if (bt == IUP_BUTTON3)
      input.mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
  }
  else if (status == 0)
  {
    if (bt == IUP_BUTTON1)
      input.mi.dwFlags |= MOUSEEVENTF_LEFTUP;
    else if (bt == IUP_BUTTON2)
      input.mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
    else if (bt == IUP_BUTTON3)
      input.mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
  }

  SendInput(1, &input, sizeof(INPUT));
}

extern "C" int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;
  if (winuiHandleIsHWND(ih))
    return 1;
  iupwinuiSetBgColor(ih->handle, r, g, b);
  return 1;
}

extern "C" int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value)
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
  { "NONE",           NULL,            -1 },
  { "NULL",           NULL,            -1 }
};

static HCURSOR winuiGetCursor(Ihandle* ih, const char* name)
{
  int i, count = sizeof(winuiCursorTable) / sizeof(winuiCursorTable[0]);

  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(name, winuiCursorTable[i].iupname))
    {
      if (!winuiCursorTable[i].sysid)
        return NULL;
      return LoadCursor(NULL, winuiCursorTable[i].sysid);
    }
  }

  HCURSOR cur = (HCURSOR)iupImageGetCursor(name);
  if (cur)
    return cur;

  (void)ih;
  return LoadCursor(NULL, IDC_ARROW);
}

static void winuiSetXamlCursor(Ihandle* ih, const char* name)
{
  using namespace Microsoft::UI::Input;

  int shape = -1;
  int i, count = sizeof(winuiCursorTable) / sizeof(winuiCursorTable[0]);

  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(name, winuiCursorTable[i].iupname))
    {
      shape = winuiCursorTable[i].xamlShape;
      break;
    }
  }

  if (shape < 0)
    shape = 0;  /* Arrow */

  UIElement element{nullptr};
  Windows::Foundation::IInspectable obj{nullptr};
  winrt::copy_from_abi(obj, ih->handle);
  element = obj.try_as<UIElement>();

  if (element)
  {
    auto protectedUI = element.try_as<Microsoft::UI::Xaml::IUIElementProtected>();
    if (protectedUI)
      protectedUI.ProtectedCursor(InputSystemCursor::Create(static_cast<InputSystemCursorShape>(shape)));
  }
}

extern "C" int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle || !value)
    return 0;

  if (winuiHandleIsHWND(ih))
  {
    HCURSOR hCur = winuiGetCursor(ih, value);
    iupAttribSet(ih, "_IUPWIN_HCURSOR", (char*)hCur);
    if (hCur)
      SetCursor(hCur);
  }
  else
    winuiSetXamlCursor(ih, value);

  return 1;
}

extern "C" int iupdrvGetScrollbarSize(void)
{
  return 16;
}

extern "C" void iupdrvSetAccessibleTitle(Ihandle* ih, const char* title)
{
  if (!ih || !ih->handle)
    return;

  if (winuiHandleIsHWND(ih))
  {
    HWND hwnd = (HWND)ih->handle;
    if (hwnd)
    {
      if (!title)
        SetWindowTextA(hwnd, "");
      else
        SetWindowTextA(hwnd, title);
    }
    return;
  }

  UIElement elem = winuiGetHandle<UIElement>(ih);
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

extern "C" void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
  (void)ic;
}

extern "C" void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "TIPMARKUP", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_DEFAULT);
}

Canvas iupwinuiGetParentCanvas(Ihandle* ih)
{
  Ihandle* parent = iupChildTreeGetNativeParent(ih);
  if (!parent || !parent->handle)
    return nullptr;

  if (IupClassMatch(parent, "dialog"))
  {
    IupWinUIDialogAux* aux = winuiGetAux<IupWinUIDialogAux>(parent, IUPWINUI_DIALOG_AUX);
    if (aux)
      return aux->contentCanvas;
  }
  else if (IupClassMatch(parent, "frame"))
  {
    IupWinUIFrameAux* aux = winuiGetAux<IupWinUIFrameAux>(parent, IUPWINUI_FRAME_AUX);
    if (aux)
      return aux->innerCanvas;
  }
  else if (IupClassMatch(parent, "popover"))
  {
    IupWinUIPopoverAux* aux = winuiGetAux<IupWinUIPopoverAux>(parent, IUPWINUI_POPOVER_AUX);
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
