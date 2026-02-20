/** \file
 * \brief WinUI Driver - Canvas Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <climits>
#include <cstdio>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_canvas.h"
#include "iup_key.h"
#include "iup_classbase.h"
#include "iup_register.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Input;
using namespace Windows::Foundation;


static int winuiCanvasIsGLCanvas(Ihandle* ih)
{
  return iupAttribGet(ih, "_IUP_GLCONTROLDATA") != NULL;
}


/***********************************************************************************
 * GL Canvas - Win32 child HWND for OpenGL rendering
 *
 * When a Canvas is used as the base for a GLCanvas, we create a Win32 child HWND
 * (with CS_OWNDC) instead of a XAML Canvas. This HWND is a sibling to the XAML
 * Island HWND, rendered on top of XAML content (standard Win32 airspace behavior).
 * The iup_glcanvas_win.c finds this HWND and creates the WGL context.
 ***********************************************************************************/

static LRESULT CALLBACK winuiGLCanvasWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  Ihandle* ih = (Ihandle*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  if (!ih)
    return DefWindowProc(hwnd, msg, wParam, lParam);

  switch (msg)
  {
  case WM_SIZE:
  {
    int w = LOWORD(lParam);
    int h = HIWORD(lParam);
    IFnii resize_cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
    if (resize_cb && !ih->data->inside_resize)
    {
      ih->data->inside_resize = 1;
      resize_cb(ih, w, h);
      ih->data->inside_resize = 0;
    }
    break;
  }

  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
  {
    IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
    if (cb)
    {
      int button = IUP_BUTTON1;
      if (msg == WM_MBUTTONDOWN) button = IUP_BUTTON2;
      else if (msg == WM_RBUTTONDOWN) button = IUP_BUTTON3;
      int x = (short)LOWORD(lParam);
      int y = (short)HIWORD(lParam);
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iupwinuiButtonKeySetStatus((int)wParam, button, status, 0);
      cb(ih, button, 1, x, y, status);
    }
    break;
  }

  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
  {
    IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
    if (cb)
    {
      int button = IUP_BUTTON1;
      if (msg == WM_MBUTTONUP) button = IUP_BUTTON2;
      else if (msg == WM_RBUTTONUP) button = IUP_BUTTON3;
      int x = (short)LOWORD(lParam);
      int y = (short)HIWORD(lParam);
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iupwinuiButtonKeySetStatus((int)wParam, button, status, 0);
      cb(ih, button, 0, x, y, status);
    }
    break;
  }

  case WM_MOUSEMOVE:
  {
    IFniis cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
    if (cb)
    {
      int x = (short)LOWORD(lParam);
      int y = (short)HIWORD(lParam);
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iupwinuiButtonKeySetStatus((int)wParam, 0, status, 0);
      cb(ih, x, y, status);
    }
    break;
  }

  case WM_MOUSEWHEEL:
  {
    IFnfiis cb = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");
    if (cb)
    {
      int delta = GET_WHEEL_DELTA_WPARAM(wParam);
      POINT pt;
      pt.x = (short)LOWORD(lParam);
      pt.y = (short)HIWORD(lParam);
      ScreenToClient(hwnd, &pt);
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iupwinuiButtonKeySetStatus(GET_KEYSTATE_WPARAM(wParam), 0, status, 0);
      cb(ih, (float)delta / 120.0f, pt.x, pt.y, status);
    }
    break;
  }

  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
  {
    int code = iupwinuiKeyDecode((int)wParam);
    if (code)
    {
      int ret = iupKeyCallKeyPressCb(ih, code, 1);
      if (ret == IUP_CLOSE)
      {
        IupExitLoop();
        return 0;
      }
      if (ret == IUP_IGNORE)
        return 0;

      ret = iupKeyCallKeyCb(ih, code);
      if (ret == IUP_CLOSE)
      {
        IupExitLoop();
        return 0;
      }
      if (ret == IUP_IGNORE)
        return 0;
    }
    break;
  }

  case WM_KEYUP:
  case WM_SYSKEYUP:
  {
    int code = iupwinuiKeyDecode((int)wParam);
    if (code)
    {
      int ret = iupKeyCallKeyPressCb(ih, code, 0);
      if (ret == IUP_CLOSE)
      {
        IupExitLoop();
        return 0;
      }
      if (ret == IUP_IGNORE)
        return 0;
    }
    break;
  }

  case WM_SETFOCUS:
  {
    IFni cb = (IFni)IupGetCallback(ih, "FOCUS_CB");
    if (cb)
      cb(ih, 1);
    break;
  }

  case WM_KILLFOCUS:
  {
    IFni cb = (IFni)IupGetCallback(ih, "FOCUS_CB");
    if (cb)
      cb(ih, 0);
    break;
  }

  case WM_PAINT:
  {
    IFn cb = (IFn)IupGetCallback(ih, "ACTION");
    if (cb && !(ih->data->inside_resize))
    {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      iupAttribSet(ih, "HDC_WMPAINT", (char*)hdc);
      iupAttribSetStrf(ih, "CLIPRECT", "%d %d %d %d", ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top);

      cb(ih);

      iupAttribSet(ih, "CLIPRECT", NULL);
      iupAttribSet(ih, "HDC_WMPAINT", NULL);
      EndPaint(hwnd, &ps);
      return 0;
    }
    break;
  }

  case WM_ERASEBKGND:
    return 1;
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void winuiGLCanvasRegisterClass(void)
{
  static bool registered = false;
  if (registered)
    return;

  WNDCLASS wc = {};
  wc.hInstance = GetModuleHandle(NULL);
  wc.lpszClassName = TEXT("IupWinUIGLCanvas");
  wc.lpfnWndProc = winuiGLCanvasWndProc;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  wc.hbrBackground = NULL;
  RegisterClass(&wc);
  registered = true;
}

static int winuiGLCanvasMapMethod(Ihandle* ih)
{
  winuiGLCanvasRegisterClass();

  Ihandle* dialog = IupGetDialog(ih);
  if (!dialog || !dialog->handle)
    return IUP_ERROR;

  HWND dialogHwnd = (HWND)dialog->handle;

  DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    dwStyle |= WS_TABSTOP;

  HWND glHwnd = CreateWindowExW(
    0, TEXT("IupWinUIGLCanvas"), NULL,
    dwStyle,
    0, 0, 100, 100,
    dialogHwnd, NULL, GetModuleHandle(NULL), NULL);

  if (!glHwnd)
    return IUP_ERROR;

  SetWindowLongPtr(glHwnd, GWLP_USERDATA, (LONG_PTR)ih);

  iupAttribSet(ih, "HWND", (char*)glHwnd);
  ih->handle = (InativeHandle*)glHwnd;

  iupwinuiBringWindowToForeground(dialogHwnd);

  return IUP_NOERROR;
}

static void winuiGLCanvasUnMapMethod(Ihandle* ih)
{
  HWND glHwnd = (HWND)iupAttribGet(ih, "HWND");
  if (glHwnd)
  {
    SetWindowLongPtr(glHwnd, GWLP_USERDATA, 0);
    DestroyWindow(glHwnd);
  }
  iupAttribSet(ih, "HWND", NULL);
  ih->handle = NULL;
}

static void winuiGLCanvasLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
}

/***********************************************************************************
 * Standard XAML Canvas
 ***********************************************************************************/

static void winuiCanvasCallScrollCallback(Ihandle* ih, int op)
{
  IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
  if (scroll_cb)
    scroll_cb(ih, op, (float)ih->data->posx, (float)ih->data->posy);
  else
    iupwinuiCanvasCallAction(ih);
}

static void winuiCanvasProcessHorScroll(Ihandle* ih, double newValue)
{
  IupWinUICanvasAux* aux = winuiGetAux<IupWinUICanvasAux>(ih, IUPWINUI_CANVAS_AUX);
  if (!aux || !aux->sbHoriz)
    return;

  double xmin = iupAttribGetDouble(ih, "XMIN");
  double xmax = iupAttribGetDouble(ih, "XMAX");
  double dx = iupAttribGetDouble(ih, "DX");
  int iposx, ipagex;

  iupCanvasCalcScrollIntPos(xmin, xmax, dx, 0, IUP_SB_MIN, IUP_SB_MAX, &ipagex, NULL);

  iposx = (int)newValue;
  double posx;
  iupCanvasCalcScrollRealPos(xmin, xmax, &posx, IUP_SB_MIN, IUP_SB_MAX, ipagex, &iposx);
  ih->data->posx = posx;

  winuiCanvasCallScrollCallback(ih, IUP_SBPOSH);
}

static void winuiCanvasProcessVerScroll(Ihandle* ih, double newValue)
{
  IupWinUICanvasAux* aux = winuiGetAux<IupWinUICanvasAux>(ih, IUPWINUI_CANVAS_AUX);
  if (!aux || !aux->sbVert)
    return;

  double ymin = iupAttribGetDouble(ih, "YMIN");
  double ymax = iupAttribGetDouble(ih, "YMAX");
  double dy = iupAttribGetDouble(ih, "DY");
  int iposy, ipagey;

  iupCanvasCalcScrollIntPos(ymin, ymax, dy, 0, IUP_SB_MIN, IUP_SB_MAX, &ipagey, NULL);

  iposy = (int)newValue;
  double posy;
  iupCanvasCalcScrollRealPos(ymin, ymax, &posy, IUP_SB_MIN, IUP_SB_MAX, ipagey, &iposy);
  ih->data->posy = posy;

  winuiCanvasCallScrollCallback(ih, IUP_SBPOSV);
}

static int winuiCanvasSetBgColorAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  iupdrvPostRedraw(ih);
  return 1;
}

static void winuiCanvasGetContentSize(Ihandle* ih, int* w, int* h)
{
  *w = ih->currentwidth;
  *h = ih->currentheight;

  IupWinUICanvasAux* aux = winuiGetAux<IupWinUICanvasAux>(ih, IUPWINUI_CANVAS_AUX);
  if (aux)
  {
    if (aux->sbVert && aux->sbVert.Visibility() == Visibility::Visible)
      *w -= iupdrvGetScrollbarSize();
    if (aux->sbHoriz && aux->sbHoriz.Visibility() == Visibility::Visible)
      *h -= iupdrvGetScrollbarSize();
  }
}

static char* winuiCanvasGetDrawSizeAttrib(Ihandle* ih)
{
  int w, h;
  winuiCanvasGetContentSize(ih, &w, &h);
  if (w > 0 && h > 0)
    return iupStrReturnIntInt(w, h, 'x');
  return NULL;
}

static int winuiCanvasSetDXAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
  {
    IupWinUICanvasAux* aux = winuiGetAux<IupWinUICanvasAux>(ih, IUPWINUI_CANVAS_AUX);
    if (!aux || !aux->sbHoriz)
      return 1;

    double posx, xmin, xmax;
    double dx;
    int iposx, ipagex;

    if (!iupStrToDoubleDef(value, &dx, 0.1))
      return 1;

    iupAttribSet(ih, "SB_RESIZE", NULL);

    xmin = iupAttribGetDouble(ih, "XMIN");
    xmax = iupAttribGetDouble(ih, "XMAX");
    posx = ih->data->posx;

    iupCanvasCalcScrollIntPos(xmin, xmax, dx, posx, IUP_SB_MIN, IUP_SB_MAX, &ipagex, &iposx);

    double linex;
    if (!iupAttribGet(ih, "LINEX"))
    {
      linex = dx / 10;
      if (linex == 0)
        linex = 1;
    }
    else
      linex = iupAttribGetDouble(ih, "LINEX");

    int ilinex;
    iupCanvasCalcScrollIntPos(xmin, xmax, linex, 0, IUP_SB_MIN, IUP_SB_MAX, &ilinex, NULL);

    if (dx >= (xmax - xmin))
    {
      if (iupAttribGetBoolean(ih, "XAUTOHIDE"))
      {
        if (aux->sbHoriz.Visibility() == Visibility::Visible && iupdrvIsVisible(ih))
          iupAttribSet(ih, "SB_RESIZE", "YES");
        iupAttribSet(ih, "XHIDDEN", "YES");
        aux->sbHoriz.Visibility(Visibility::Collapsed);
      }
      else
        aux->sbHoriz.IsEnabled(false);

      ih->data->posx = xmin;
    }
    else
    {
      if (iupAttribGetBoolean(ih, "XAUTOHIDE"))
      {
        if (aux->sbHoriz.Visibility() != Visibility::Visible && iupdrvIsVisible(ih))
          iupAttribSet(ih, "SB_RESIZE", "YES");
        iupAttribSet(ih, "XHIDDEN", "NO");
        aux->sbHoriz.Visibility(Visibility::Visible);
      }
      else
        aux->sbHoriz.IsEnabled(true);

      aux->sbHoriz.Minimum(IUP_SB_MIN);
      aux->sbHoriz.Maximum(IUP_SB_MAX);
      aux->sbHoriz.ViewportSize(ipagex);
      aux->sbHoriz.SmallChange(ilinex);
      aux->sbHoriz.LargeChange(ipagex);

      iupCanvasCalcScrollRealPos(xmin, xmax, &posx, IUP_SB_MIN, IUP_SB_MAX, ipagex, &iposx);
      aux->sbHoriz.Value(iposx);
      ih->data->posx = posx;
    }
  }
  return 1;
}

static int winuiCanvasSetDYAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_VERT)
  {
    IupWinUICanvasAux* aux = winuiGetAux<IupWinUICanvasAux>(ih, IUPWINUI_CANVAS_AUX);
    if (!aux || !aux->sbVert)
      return 1;

    double posy, ymin, ymax;
    double dy;
    int iposy, ipagey;

    if (!iupStrToDoubleDef(value, &dy, 0.1))
      return 1;

    iupAttribSet(ih, "SB_RESIZE", NULL);

    ymin = iupAttribGetDouble(ih, "YMIN");
    ymax = iupAttribGetDouble(ih, "YMAX");
    posy = ih->data->posy;

    iupCanvasCalcScrollIntPos(ymin, ymax, dy, posy, IUP_SB_MIN, IUP_SB_MAX, &ipagey, &iposy);

    double liney;
    if (!iupAttribGet(ih, "LINEY"))
    {
      liney = dy / 10;
      if (liney == 0)
        liney = 1;
    }
    else
      liney = iupAttribGetDouble(ih, "LINEY");

    int iliney;
    iupCanvasCalcScrollIntPos(ymin, ymax, liney, 0, IUP_SB_MIN, IUP_SB_MAX, &iliney, NULL);

    if (dy >= (ymax - ymin))
    {
      if (iupAttribGetBoolean(ih, "YAUTOHIDE"))
      {
        if (aux->sbVert.Visibility() == Visibility::Visible && iupdrvIsVisible(ih))
          iupAttribSet(ih, "SB_RESIZE", "YES");
        iupAttribSet(ih, "YHIDDEN", "YES");
        aux->sbVert.Visibility(Visibility::Collapsed);
      }
      else
        aux->sbVert.IsEnabled(false);

      ih->data->posy = ymin;
    }
    else
    {
      if (iupAttribGetBoolean(ih, "YAUTOHIDE"))
      {
        if (aux->sbVert.Visibility() != Visibility::Visible && iupdrvIsVisible(ih))
          iupAttribSet(ih, "SB_RESIZE", "YES");
        iupAttribSet(ih, "YHIDDEN", "NO");
        aux->sbVert.Visibility(Visibility::Visible);
      }
      else
        aux->sbVert.IsEnabled(true);

      aux->sbVert.Minimum(IUP_SB_MIN);
      aux->sbVert.Maximum(IUP_SB_MAX);
      aux->sbVert.ViewportSize(ipagey);
      aux->sbVert.SmallChange(iliney);
      aux->sbVert.LargeChange(ipagey);

      iupCanvasCalcScrollRealPos(ymin, ymax, &posy, IUP_SB_MIN, IUP_SB_MAX, ipagey, &iposy);
      aux->sbVert.Value(iposy);
      ih->data->posy = posy;
    }
  }
  return 1;
}

static int winuiCanvasSetPosXAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
  {
    IupWinUICanvasAux* aux = winuiGetAux<IupWinUICanvasAux>(ih, IUPWINUI_CANVAS_AUX);
    if (!aux || !aux->sbHoriz)
      return 1;

    double xmin, xmax, dx;
    double posx;
    int iposx, ipagex;

    if (!iupStrToDouble(value, &posx))
      return 1;

    xmin = iupAttribGetDouble(ih, "XMIN");
    xmax = iupAttribGetDouble(ih, "XMAX");
    dx = iupAttribGetDouble(ih, "DX");

    if (dx >= xmax - xmin)
      return 0;

    if (posx < xmin) posx = xmin;
    if (posx > (xmax - dx)) posx = xmax - dx;
    ih->data->posx = posx;

    iupCanvasCalcScrollIntPos(xmin, xmax, dx, posx, IUP_SB_MIN, IUP_SB_MAX, &ipagex, &iposx);

    aux->sbHoriz.Value(iposx);
  }
  return 1;
}

static int winuiCanvasSetPosYAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_VERT)
  {
    IupWinUICanvasAux* aux = winuiGetAux<IupWinUICanvasAux>(ih, IUPWINUI_CANVAS_AUX);
    if (!aux || !aux->sbVert)
      return 1;

    double ymin, ymax, dy;
    double posy;
    int iposy, ipagey;

    if (!iupStrToDouble(value, &posy))
      return 1;

    ymin = iupAttribGetDouble(ih, "YMIN");
    ymax = iupAttribGetDouble(ih, "YMAX");
    dy = iupAttribGetDouble(ih, "DY");

    if (dy >= ymax - ymin)
      return 0;

    if (posy < ymin) posy = ymin;
    if (posy > (ymax - dy)) posy = ymax - dy;
    ih->data->posy = posy;

    iupCanvasCalcScrollIntPos(ymin, ymax, dy, posy, IUP_SB_MIN, IUP_SB_MAX, &ipagey, &iposy);

    aux->sbVert.Value(iposy);
  }
  return 1;
}

static void winuiScrollBarForceVisible(ScrollBar sb)
{
  sb.IndicatorMode(ScrollingIndicatorMode::MouseIndicator);

  sb.Loaded([](IInspectable const& sender, RoutedEventArgs const&) {
    auto ctrl = sender.as<Controls::Control>();
    VisualStateManager::GoToState(ctrl, L"MouseIndicatorFull", false);

    auto root = Media::VisualTreeHelper::GetChild(ctrl, 0);
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

static int winuiCanvasMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  if (winuiCanvasIsGLCanvas(ih))
    return winuiGLCanvasMapMethod(ih);

  IupWinUICanvasAux* aux = new IupWinUICanvasAux();

  Canvas canvas;
  canvas.HorizontalAlignment(HorizontalAlignment::Left);
  canvas.VerticalAlignment(VerticalAlignment::Top);
  canvas.Background(SolidColorBrush(Microsoft::UI::Colors::Transparent()));

  if (iupAttribGetBoolean(ih, "CANFOCUS"))
  {
    canvas.IsTabStop(true);
    canvas.AllowFocusOnInteraction(false);
  }

  Image displayImage;
  displayImage.Stretch(Stretch::None);
  canvas.Children().Append(displayImage);
  aux->displayImage = displayImage;

  ih->data->sb = iupBaseGetScrollbar(ih);

  if (ih->data->sb & IUP_SB_HORIZ)
  {
    ScrollBar sbHoriz;
    sbHoriz.Orientation(Orientation::Horizontal);
    winuiScrollBarForceVisible(sbHoriz);
    canvas.Children().Append(sbHoriz);
    aux->sbHoriz = sbHoriz;

    aux->sbHorizScrollToken = sbHoriz.Scroll([ih](IInspectable const&, ScrollEventArgs const& args) {
      winuiCanvasProcessHorScroll(ih, args.NewValue());
    });
  }

  if (ih->data->sb & IUP_SB_VERT)
  {
    ScrollBar sbVert;
    sbVert.Orientation(Orientation::Vertical);
    winuiScrollBarForceVisible(sbVert);
    canvas.Children().Append(sbVert);
    aux->sbVert = sbVert;

    aux->sbVertScrollToken = sbVert.Scroll([ih](IInspectable const&, ScrollEventArgs const& args) {
      winuiCanvasProcessVerScroll(ih, args.NewValue());
    });
  }

  aux->pointerPressedToken = canvas.PointerPressed([ih](IInspectable const&, PointerRoutedEventArgs const& args) {
    IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
    if (cb)
    {
      auto point = args.GetCurrentPoint(winuiGetHandle<Canvas>(ih));
      auto props = point.Properties();
      int button = iupwinuiGetPointerButton(props);
      int x = (int)point.Position().X;
      int y = (int)point.Position().Y;
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iupwinuiButtonKeySetStatus(iupwinuiGetModifierKeys(), button, status, 0);
      cb(ih, button, 1, x, y, status);
    }

    if (iupAttribGetBoolean(ih, "CANFOCUS"))
    {
      Ihandle* dlg = IupGetDialog(ih);
      if (dlg && (HWND)dlg->handle == GetActiveWindow())
      {
        Canvas c = winuiGetHandle<Canvas>(ih);
        if (c)
          c.Focus(FocusState::Pointer);
      }
    }

    args.Handled(true);
  });

  aux->pointerReleasedToken = canvas.PointerReleased([ih](IInspectable const&, PointerRoutedEventArgs const& args) {
    IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
    if (cb)
    {
      auto point = args.GetCurrentPoint(winuiGetHandle<Canvas>(ih));
      auto props = point.Properties();
      int button = iupwinuiGetPointerReleasedButton(props);
      int x = (int)point.Position().X;
      int y = (int)point.Position().Y;
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iupwinuiButtonKeySetStatus(iupwinuiGetModifierKeys(), button, status, 0);
      cb(ih, button, 0, x, y, status);
    }

    args.Handled(true);
  });

  aux->pointerMovedToken = canvas.PointerMoved([ih](IInspectable const&, PointerRoutedEventArgs const& args) {
    IFniis cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
    if (cb)
    {
      auto point = args.GetCurrentPoint(winuiGetHandle<Canvas>(ih));
      int x = (int)point.Position().X;
      int y = (int)point.Position().Y;
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iupwinuiButtonKeySetStatus(iupwinuiGetModifierKeys(), 0, status, 0);
      cb(ih, x, y, status);
    }
  });

  aux->pointerEnteredToken = canvas.PointerEntered([ih](IInspectable const&, PointerRoutedEventArgs const&) {
    Icallback cb = IupGetCallback(ih, "ENTERWINDOW_CB");
    if (cb)
      cb(ih);
  });

  aux->pointerExitedToken = canvas.PointerExited([ih](IInspectable const&, PointerRoutedEventArgs const&) {
    Icallback cb = IupGetCallback(ih, "LEAVEWINDOW_CB");
    if (cb)
      cb(ih);
  });

  aux->pointerWheelToken = canvas.PointerWheelChanged([ih](IInspectable const&, PointerRoutedEventArgs const& args) {
    auto point = args.GetCurrentPoint(winuiGetHandle<Canvas>(ih));
    auto props = point.Properties();
    int delta = props.MouseWheelDelta();

    IFnfiis cb = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");
    if (cb)
    {
      int x = (int)point.Position().X;
      int y = (int)point.Position().Y;
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iupwinuiButtonKeySetStatus(iupwinuiGetModifierKeys(), 0, status, 0);
      cb(ih, (float)delta / 120.0f, x, y, status);
    }
    else
    {
      if (ih->data->sb & IUP_SB_VERT)
      {
        double posy = ih->data->posy;
        double dy = iupAttribGetDouble(ih, "DY");
        posy -= ((double)delta / 120.0) * dy / 10.0;
        IupSetDouble(ih, "POSY", posy);

        IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
        if (scroll_cb)
        {
          int op = delta > 0 ? IUP_SBUP : IUP_SBDN;
          scroll_cb(ih, op, (float)ih->data->posx, (float)ih->data->posy);
        }
      }
    }

    args.Handled(true);
  });

  aux->keyDownToken = canvas.KeyDown([ih](IInspectable const&, KeyRoutedEventArgs const& args) {
    int code = iupwinuiKeyDecode((int)args.Key());
    if (code)
    {
      int ret = iupKeyCallKeyPressCb(ih, code, 1);
      if (ret == IUP_CLOSE)
      {
        IupExitLoop();
        args.Handled(true);
        return;
      }
      if (ret == IUP_IGNORE)
      {
        args.Handled(true);
        return;
      }

      ret = iupKeyCallKeyCb(ih, code);
      if (ret == IUP_CLOSE)
      {
        IupExitLoop();
        args.Handled(true);
        return;
      }
      if (ret == IUP_IGNORE)
      {
        args.Handled(true);
        return;
      }
    }
  });

  aux->keyUpToken = canvas.KeyUp([ih](IInspectable const&, KeyRoutedEventArgs const& args) {
    int code = iupwinuiKeyDecode((int)args.Key());
    if (code)
    {
      int ret = iupKeyCallKeyPressCb(ih, code, 0);
      if (ret == IUP_CLOSE)
      {
        IupExitLoop();
        args.Handled(true);
        return;
      }
      if (ret == IUP_IGNORE)
      {
        args.Handled(true);
        return;
      }
    }
  });

  aux->gotFocusToken = canvas.GotFocus([ih](IInspectable const&, RoutedEventArgs const&) {
    IFni cb = (IFni)IupGetCallback(ih, "FOCUS_CB");
    if (cb)
      cb(ih, 1);
  });

  aux->lostFocusToken = canvas.LostFocus([ih](IInspectable const&, RoutedEventArgs const&) {
    IFni cb = (IFni)IupGetCallback(ih, "FOCUS_CB");
    if (cb)
      cb(ih, 0);
  });

  aux->sizeChangedToken = canvas.SizeChanged([ih](IInspectable const&, SizeChangedEventArgs const&) {
    int w, h;
    winuiCanvasGetContentSize(ih, &w, &h);

    IFnii resize_cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
    if (resize_cb && !ih->data->inside_resize)
    {
      ih->data->inside_resize = 1;
      resize_cb(ih, w, h);
      ih->data->inside_resize = 0;
    }

    iupwinuiCanvasCallAction(ih);
  });

  canvas.Loaded([ih](IInspectable const&, RoutedEventArgs const&) {
    iupwinuiCanvasCallAction(ih);
  });

  Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
  if (parentCanvas)
    parentCanvas.Children().Append(canvas);

  winuiSetAux(ih, IUPWINUI_CANVAS_AUX, aux);
  winuiStoreHandle(ih, canvas);

  winuiCanvasSetDXAttrib(ih, NULL);
  winuiCanvasSetDYAttrib(ih, NULL);

  return IUP_NOERROR;
}

static void winuiCanvasUnMapMethod(Ihandle* ih)
{
  if (winuiCanvasIsGLCanvas(ih))
  {
    winuiGLCanvasUnMapMethod(ih);
    return;
  }

  IupWinUICanvasAux* aux = winuiGetAux<IupWinUICanvasAux>(ih, IUPWINUI_CANVAS_AUX);

  if (ih->handle && aux)
  {
    Canvas canvas = winuiGetHandle<Canvas>(ih);
    if (canvas)
    {
      if (aux->pointerPressedToken)
        canvas.PointerPressed(aux->pointerPressedToken);
      if (aux->pointerReleasedToken)
        canvas.PointerReleased(aux->pointerReleasedToken);
      if (aux->pointerMovedToken)
        canvas.PointerMoved(aux->pointerMovedToken);
      if (aux->pointerEnteredToken)
        canvas.PointerEntered(aux->pointerEnteredToken);
      if (aux->pointerExitedToken)
        canvas.PointerExited(aux->pointerExitedToken);
      if (aux->pointerWheelToken)
        canvas.PointerWheelChanged(aux->pointerWheelToken);
      if (aux->keyDownToken)
        canvas.KeyDown(aux->keyDownToken);
      if (aux->keyUpToken)
        canvas.KeyUp(aux->keyUpToken);
      if (aux->gotFocusToken)
        canvas.GotFocus(aux->gotFocusToken);
      if (aux->lostFocusToken)
        canvas.LostFocus(aux->lostFocusToken);
      if (aux->sizeChangedToken)
        canvas.SizeChanged(aux->sizeChangedToken);
    }

    if (aux->sbHoriz && aux->sbHorizScrollToken)
      aux->sbHoriz.Scroll(aux->sbHorizScrollToken);
    if (aux->sbVert && aux->sbVertScrollToken)
      aux->sbVert.Scroll(aux->sbVertScrollToken);

    winuiReleaseHandle<Canvas>(ih);
  }

  if (aux)
    *aux->alive = false;
  winuiFreeAux<IupWinUICanvasAux>(ih, IUPWINUI_CANVAS_AUX);
  ih->handle = NULL;
}

static void winuiCanvasUpdateChildLayout(Ihandle* ih)
{
  IupWinUICanvasAux* aux = winuiGetAux<IupWinUICanvasAux>(ih, IUPWINUI_CANVAS_AUX);
  if (!aux)
    return;

  int width = ih->currentwidth;
  int height = ih->currentheight;
  int sb_size = iupdrvGetScrollbarSize();
  int sb_vert_width = 0, sb_horiz_height = 0;

  if (aux->sbVert && aux->sbVert.Visibility() == Visibility::Visible)
    sb_vert_width = sb_size;
  if (aux->sbHoriz && aux->sbHoriz.Visibility() == Visibility::Visible)
    sb_horiz_height = sb_size;

  if (aux->sbVert && aux->sbVert.Visibility() == Visibility::Visible)
  {
    Canvas::SetLeft(aux->sbVert, (double)(width - sb_vert_width));
    Canvas::SetTop(aux->sbVert, 0.0);
    aux->sbVert.Width((double)sb_vert_width);
    aux->sbVert.Height((double)(height - sb_horiz_height));
  }

  if (aux->sbHoriz && aux->sbHoriz.Visibility() == Visibility::Visible)
  {
    Canvas::SetLeft(aux->sbHoriz, 0.0);
    Canvas::SetTop(aux->sbHoriz, (double)(height - sb_horiz_height));
    aux->sbHoriz.Width((double)(width - sb_vert_width));
    aux->sbHoriz.Height((double)sb_horiz_height);
  }
}

static void winuiCanvasLayoutUpdateMethod(Ihandle* ih)
{
  if (winuiCanvasIsGLCanvas(ih))
  {
    winuiGLCanvasLayoutUpdateMethod(ih);
    return;
  }

  Canvas canvas = winuiGetHandle<Canvas>(ih);
  if (!canvas)
    return;

  double dw = canvas.Width(), dh = canvas.Height();
  int old_w = std::isnan(dw) ? 0 : (int)dw;
  int old_h = std::isnan(dh) ? 0 : (int)dh;

  iupdrvBaseLayoutUpdateMethod(ih);
  winuiCanvasUpdateChildLayout(ih);

  int new_w = ih->currentwidth;
  int new_h = ih->currentheight;

  if (new_w != old_w || new_h != old_h)
  {
    int content_w, content_h;
    winuiCanvasGetContentSize(ih, &content_w, &content_h);

    IFnii resize_cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
    if (resize_cb && !ih->data->inside_resize)
    {
      ih->data->inside_resize = 1;
      resize_cb(ih, content_w, content_h);
      ih->data->inside_resize = 0;
    }

    iupwinuiCanvasCallAction(ih);
  }
}

extern "C" void iupdrvCanvasInitClass(Iclass* ic)
{
  ic->Map = winuiCanvasMapMethod;
  ic->UnMap = winuiCanvasUnMapMethod;
  ic->LayoutUpdate = winuiCanvasLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, winuiCanvasSetBgColorAttrib, "255 255 255", NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "DRAWSIZE", winuiCanvasGetDrawSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DX", NULL, winuiCanvasSetDXAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DY", NULL, winuiCanvasSetDYAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, winuiCanvasSetPosXAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, winuiCanvasSetPosYAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XAUTOHIDE", NULL, NULL, "YES", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YAUTOHIDE", NULL, NULL, "YES", NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAWANTIALIAS", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
}
