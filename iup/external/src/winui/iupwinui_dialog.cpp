/** \file
 * \brief WinUI Dialog using Win32 HWND with DesktopWindowXamlSource
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <windowsx.h>
#include <shobjidl.h>

#define _IUPDLG_PRIVATE

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_dialog.h"
#include "iup_register.h"
#include "iup_layout.h"
#include "iup_focus.h"
#include "iup_childtree.h"
#include "iup_classbase.h"
#include "iup_image.h"
#include "iup_drvinfo.h"
}

#include "iupwinui_drv.h"

#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

using namespace winrt;
using namespace Microsoft::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Content;
using namespace Windows::Foundation;
using namespace Windows::Graphics;


#define WINUI_DIALOG_CLASS L"IupWinUIDialog"

static void winuiDialogUpdateXamlIsland(Ihandle* ih)
{
  IupWinUIDialogAux* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
  if (!aux)
    return;

  RECT rect;
  GetClientRect((HWND)ih->handle, &rect);

  if (aux->siteBridge)
  {
    RectInt32 islandRect;
    islandRect.X = 0;
    islandRect.Y = 0;
    islandRect.Width = rect.right;
    islandRect.Height = rect.bottom;
    aux->siteBridge.MoveAndResize(islandRect);
  }

  if (aux->rootPanel)
  {
    aux->rootPanel.Width(static_cast<double>(rect.right));
    aux->rootPanel.Height(static_cast<double>(rect.bottom));
  }
}

static void winuiDialogResize(Ihandle* ih, int width, int height)
{
  IFnii cb;

  iupdrvDialogGetSize(ih, NULL, &(ih->currentwidth), &(ih->currentheight));

  cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (!cb || cb(ih, width, height) != IUP_IGNORE)
  {
    ih->data->ignore_resize = 1;
    IupRefresh(ih);
    ih->data->ignore_resize = 0;
  }
}

static void winuiDialogRefreshThemeColors(Ihandle* ih)
{
  Ihandle* child = ih->firstchild;
  while (child)
  {
    if (winuiGetAux<IupWinUIFrameAux>(child, IUPWINUI_FRAME_AUX))
      winuiFrameUpdateBorderColor(child);
    else if (winuiGetAux<IupWinUITableAux>(child, IUPWINUI_TABLE_AUX))
      winuiTableRefreshThemeColors(child);
    else if (winuiGetAux<IupWinUITreeAux>(child, IUPWINUI_TREE_AUX))
      winuiTreeRefreshThemeColors(child);

    winuiDialogRefreshThemeColors(child);
    child = child->brother;
  }
}

static void winuiDialogTitleBarThemeColor(HWND hwnd)
{
  typedef HRESULT(STDAPICALLTYPE *PtrDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
  static PtrDwmSetWindowAttribute pDwmSetWindowAttribute = NULL;
  static int initialized = 0;

  if (!initialized)
  {
    HMODULE dwmLibrary = LoadLibrary(TEXT("dwmapi.dll"));
    if (dwmLibrary)
      pDwmSetWindowAttribute = (PtrDwmSetWindowAttribute)GetProcAddress(dwmLibrary, "DwmSetWindowAttribute");
    initialized = 1;
  }

  if (pDwmSetWindowAttribute)
  {
    BOOL value = iupwinuiIsSystemDarkMode() ? TRUE : FALSE;
    pDwmSetWindowAttribute(hwnd, 20, &value, sizeof(value));
  }
}

static LRESULT CALLBACK winuiDialogWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  Ihandle* ih = (Ihandle*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

  switch (msg)
  {
    case WM_CREATE:
    {
      CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
      return 0;
    }

    case WM_ERASEBKGND:
    {
      if (ih)
      {
        unsigned char r, g, b;
        const char* color = iupAttribGet(ih, "_IUPWINUI_BACKGROUND_COLOR");
        if (color && iupStrToRGB(color, &r, &g, &b))
        {
          HDC hdc = (HDC)wParam;
          RECT rect;
          GetClientRect(hwnd, &rect);
          SetDCBrushColor(hdc, RGB(r, g, b));
          FillRect(hdc, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
          return 1;
        }
      }
      break;
    }

    case WM_SETCURSOR:
    {
      if (LOWORD(lParam) == HTCLIENT)
      {
        HCURSOR hCur = ih ? (HCURSOR)iupAttribGet(ih, "_IUPWIN_HCURSOR") : NULL;
        SetCursor(hCur ? hCur : LoadCursor(NULL, IDC_ARROW));
        return TRUE;
      }
      break;
    }

    case WM_ACTIVATE:
    {
      if (ih)
      {
        IupWinUIDialogAux* dlgaux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
        if (dlgaux && dlgaux->windowCreated)
        {
          if (LOWORD(wParam) != WA_INACTIVE)
            iupCallGetFocusCb(ih);
          else
            iupCallKillFocusCb(ih);
          return 0;
        }
      }
      break;
    }

    case WM_SETFOCUS:
    {
      if (ih)
      {
        IupWinUIDialogAux* dlgaux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
        if (dlgaux && dlgaux->islandHwnd)
        {
          Ihandle* lastfocus = (Ihandle*)iupAttribGet(ih, "_IUPWINUI_LASTFOCUS");
          if (iupObjectCheck(lastfocus) && !iupAttribGetBoolean(ih, "IGNORELASTFOCUS"))
          {
            IupSetFocus(lastfocus);
            return 0;
          }

          SetFocus(dlgaux->islandHwnd);
        }
      }
      return 0;
    }

    case WM_SIZE:
    {
      IupWinUIDialogAux* dlgaux = ih ? winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX) : NULL;
      if (ih && !ih->data->ignore_resize && dlgaux && dlgaux->windowCreated)
      {
        winuiDialogUpdateXamlIsland(ih);

        switch (wParam)
        {
        case SIZE_MINIMIZED:
          if (ih->data->show_state != IUP_MINIMIZE)
          {
            IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
            ih->data->show_state = IUP_MINIMIZE;
            if (show_cb && show_cb(ih, IUP_MINIMIZE) == IUP_CLOSE)
              IupExitLoop();
          }
          break;
        case SIZE_MAXIMIZED:
          if (ih->data->show_state != IUP_MAXIMIZE)
          {
            IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
            ih->data->show_state = IUP_MAXIMIZE;
            if (show_cb && show_cb(ih, IUP_MAXIMIZE) == IUP_CLOSE)
              IupExitLoop();
          }
          winuiDialogResize(ih, LOWORD(lParam), HIWORD(lParam));
          break;
        case SIZE_RESTORED:
          if (ih->data->show_state == IUP_MAXIMIZE || ih->data->show_state == IUP_MINIMIZE)
          {
            IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
            ih->data->show_state = IUP_RESTORE;
            if (show_cb && show_cb(ih, IUP_RESTORE) == IUP_CLOSE)
              IupExitLoop();
          }
          winuiDialogResize(ih, LOWORD(lParam), HIWORD(lParam));
          break;
        default:
          winuiDialogResize(ih, LOWORD(lParam), HIWORD(lParam));
          break;
        }
      }
      return 0;
    }

    case WM_GETMINMAXINFO:
    {
      if (ih)
      {
        MINMAXINFO* minmax = (MINMAXINFO*)lParam;
        int min_w = 1, min_h = 1;
        int max_w = 65535, max_h = 65535;

        iupStrToIntInt(iupAttribGet(ih, "MINSIZE"), &min_w, &min_h, 'x');
        iupStrToIntInt(iupAttribGet(ih, "MAXSIZE"), &max_w, &max_h, 'x');

        minmax->ptMinTrackSize.x = min_w;
        minmax->ptMinTrackSize.y = min_h;
        minmax->ptMaxTrackSize.x = max_w;
        minmax->ptMaxTrackSize.y = max_h;
        return 0;
      }
      break;
    }

    case WM_MOVE:
    {
      if (ih)
      {
        IFnii cb = (IFnii)IupGetCallback(ih, "MOVE_CB");
        if (cb)
        {
          int x, y;
          iupdrvDialogGetPosition(ih, NULL, &x, &y);
          cb(ih, x, y);
        }
      }
      break;
    }

    case WM_NCHITTEST:
    {
      if (ih && iupAttribGetBoolean(ih, "CUSTOMFRAME"))
      {
        int captionHeight = iupAttribGetInt(ih, "CUSTOMFRAMECAPTIONHEIGHT");
        if (captionHeight > 0)
        {
          POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
          ScreenToClient(hwnd, &pt);

          Ihandle* ih_caption = IupGetDialogChild(ih, "CUSTOMFRAMECAPTION");
          if (ih_caption)
          {
            if (pt.x >= ih_caption->x && pt.x < ih_caption->x + ih_caption->currentwidth &&
                pt.y >= ih_caption->y && pt.y < ih_caption->y + ih_caption->currentheight)
              return HTCAPTION;
          }
          else if (pt.y < captionHeight)
          {
            return HTCAPTION;
          }
        }
      }
      break;
    }

    case WM_CLOSE:
    {
      if (ih)
      {
        Icallback cb = IupGetCallback(ih, "CLOSE_CB");
        if (cb)
        {
          int ret = cb(ih);
          if (ret == IUP_IGNORE)
            return 0;
          if (ret == IUP_CLOSE)
          {
            IupExitLoop();
            return 0;
          }
        }
        IupHide(ih);
      }
      return 0;
    }

    case WM_SETTINGCHANGE:
    {
      if (ih && wParam == 0 && lParam != 0)
      {
        LPCTSTR area = (LPCTSTR)lParam;
        if (lstrcmp(area, TEXT("ImmersiveColorSet")) == 0)
        {
          winuiDialogTitleBarThemeColor(hwnd);
          iupwinuiSetGlobalColors();

          int dark_mode = iupwinuiIsSystemDarkMode();
          IupSetGlobal("DARKMODE", dark_mode ? "YES" : NULL);

          IupWinUIDialogAux* dlgaux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
          if (dlgaux && dlgaux->rootPanel)
            dlgaux->rootPanel.RequestedTheme(dark_mode ? ElementTheme::Dark : ElementTheme::Light);

          const char* bgcolor = IupGetGlobal("DLGBGCOLOR");
          if (bgcolor)
          {
            unsigned char r, g, b;
            iupAttribSetStr(ih, "_IUPWINUI_BACKGROUND_COLOR", bgcolor);
            if (dlgaux && dlgaux->rootPanel && iupStrToRGB(bgcolor, &r, &g, &b))
            {
              Windows::UI::Color color;
              color.A = 255;
              color.R = r;
              color.G = g;
              color.B = b;
              dlgaux->rootPanel.Background(SolidColorBrush(color));
            }
          }

          IFni cb = (IFni)IupGetCallback(ih, "THEMECHANGED_CB");
          if (cb)
          {
            if (cb(ih, dark_mode) == IUP_CLOSE)
              IupExitLoop();
          }

          winuiDialogRefreshThemeColors(ih);

          RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        }
      }
      break;
    }

    case WM_DESTROY:
      break;

    case WM_NCDESTROY:
      if (ih)
      {
        winuiFreeAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
        ih->handle = NULL;
      }
      SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
      return 0;
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void winuiDialogRegisterClass(void)
{
  static bool registered = false;
  if (registered)
    return;

  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = winuiDialogWndProc;
  wc.hInstance = GetModuleHandle(NULL);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszClassName = WINUI_DIALOG_CLASS;

  RegisterClassExW(&wc);
  registered = true;
}

static int winuiDialogMapMethod(Ihandle* ih)
{
  IupWinUIDialogAux* aux = new IupWinUIDialogAux();
  winuiSetAux(ih, IUPWINUI_DIALOG_AUX, aux);

  winuiDialogRegisterClass();

  DWORD dwStyle = WS_CLIPSIBLINGS;
  DWORD dwExStyle = 0;
  int has_titlebar = 0;
  int has_border = 0;

  const char* title = iupAttribGet(ih, "TITLE");
  if (title)
    has_titlebar = 1;

  int customframe = iupAttribGetBoolean(ih, "CUSTOMFRAME");

  if (customframe)
  {
    dwStyle |= WS_OVERLAPPEDWINDOW;
    dwExStyle |= WS_EX_APPWINDOW;
  }
  else
  {
    if (iupAttribGetBoolean(ih, "RESIZE"))
    {
      dwStyle |= WS_THICKFRAME;
      has_border = 1;
    }
    else
      iupAttribSet(ih, "MAXBOX", "NO");

    if (iupAttribGetBoolean(ih, "MENUBOX"))
    {
      dwStyle |= WS_SYSMENU;
      has_titlebar = 1;
    }
    if (iupAttribGetBoolean(ih, "MAXBOX"))
    {
      dwStyle |= WS_MAXIMIZEBOX;
      has_titlebar = 1;
    }
    if (iupAttribGetBoolean(ih, "MINBOX"))
    {
      dwStyle |= WS_MINIMIZEBOX;
      has_titlebar = 1;
    }
    if (iupAttribGetBoolean(ih, "BORDER") || has_titlebar)
      has_border = 1;

    InativeHandle* native_parent = iupDialogGetNativeParent(ih);

    if (native_parent)
    {
      dwStyle |= WS_POPUP;

      if (has_titlebar)
        dwStyle |= WS_CAPTION;
      else if (has_border)
        dwStyle |= WS_BORDER;
    }
    else
    {
      if (has_titlebar)
      {
        dwStyle |= WS_OVERLAPPED;
      }
      else
      {
        if (has_border)
          dwStyle |= WS_POPUP | WS_BORDER;
        else
          dwStyle |= WS_POPUP;

        dwExStyle |= WS_EX_NOACTIVATE;
      }
    }

    char* taskbarButton = iupAttribGet(ih, "TASKBARBUTTON");
    if (taskbarButton)
    {
      if (iupStrEqualNoCase(taskbarButton, "SHOW"))
        dwExStyle |= WS_EX_APPWINDOW;
      else if (iupStrEqualNoCase(taskbarButton, "HIDE"))
        dwExStyle |= WS_EX_TOOLWINDOW;
    }

    if (iupAttribGetBoolean(ih, "TOOLBOX") && native_parent)
      dwExStyle |= WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE;

    if (iupAttribGetBoolean(ih, "DIALOGFRAME") && native_parent)
      dwExStyle |= WS_EX_DLGMODALFRAME;

    if (iupAttribGet(ih, "OPACITY") || iupAttribGet(ih, "LAYERALPHA"))
      dwExStyle |= WS_EX_LAYERED;

    if (!(dwExStyle & (WS_EX_APPWINDOW | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE)))
      dwExStyle |= WS_EX_APPWINDOW;
  }

  std::wstring wtitle = title ? iupwinuiStringToWString(title) : L"";
  HWND parentHwnd = (HWND)iupDialogGetNativeParent(ih);

  HWND hwnd = CreateWindowExW(
    dwExStyle,
    WINUI_DIALOG_CLASS,
    wtitle.c_str(),
    dwStyle,
    0, 0,
    100, 100,
    parentHwnd,
    NULL,
    GetModuleHandle(NULL),
    ih
  );

  if (!hwnd)
  {
    winuiFreeAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
    return IUP_ERROR;
  }

  ih->handle = (InativeHandle*)hwnd;

  winuiDialogTitleBarThemeColor(hwnd);

  WindowId parentWindowId = winrt::Microsoft::UI::GetWindowIdFromWindow(hwnd);

  aux->xamlSource = DesktopWindowXamlSource();
  aux->xamlSource.Initialize(parentWindowId);

  RECT rect;
  GetClientRect(hwnd, &rect);

  aux->siteBridge = aux->xamlSource.SiteBridge();
  if (aux->siteBridge)
  {
    aux->islandHwnd = GetWindowFromWindowId(aux->siteBridge.WindowId());
    if (aux->islandHwnd)
      SetWindowLong(aux->islandHwnd, GWL_STYLE, WS_TABSTOP | WS_CHILD | WS_VISIBLE);

    RectInt32 islandRect;
    islandRect.X = 0;
    islandRect.Y = 0;
    islandRect.Width = rect.right;
    islandRect.Height = rect.bottom;
    aux->siteBridge.MoveAndResize(islandRect);
  }

  aux->rootPanel = Grid();
  aux->rootPanel.Width(static_cast<double>(rect.right));
  aux->rootPanel.Height(static_cast<double>(rect.bottom));

  RowDefinition menuRow;
  menuRow.Height(GridLengthHelper::Auto());
  aux->rootPanel.RowDefinitions().Append(menuRow);

  RowDefinition contentRow;
  contentRow.Height(GridLength{1, GridUnitType::Star});
  aux->rootPanel.RowDefinitions().Append(contentRow);

  aux->contentCanvas = Canvas();
  Grid::SetRow(aux->contentCanvas, 1);
  aux->contentCanvas.HorizontalAlignment(HorizontalAlignment::Stretch);
  aux->contentCanvas.VerticalAlignment(VerticalAlignment::Stretch);

  aux->rootPanel.Children().Append(aux->contentCanvas);

  if (iupwinuiIsSystemDarkMode())
    aux->rootPanel.RequestedTheme(ElementTheme::Dark);

  {
    unsigned char r, g, b;
    const char* bgcolor = IupGetGlobal("DLGBGCOLOR");
    if (bgcolor && iupStrToRGB(bgcolor, &r, &g, &b))
    {
      Windows::UI::Color color;
      color.A = 255;
      color.R = r;
      color.G = g;
      color.B = b;
      aux->rootPanel.Background(SolidColorBrush(color));
    }
  }

  aux->xamlSource.Content(aux->rootPanel);

  aux->takeFocusToken = aux->xamlSource.TakeFocusRequested(
    [ih](DesktopWindowXamlSource const&, DesktopWindowXamlSourceTakeFocusRequestedEventArgs const& args)
    {
      (void)args;
    });

  aux->gotFocusToken = aux->xamlSource.GotFocus(
    [ih](DesktopWindowXamlSource const&, DesktopWindowXamlSourceGotFocusEventArgs const&)
    {
      (void)ih;
    });

  if (customframe)
  {
    SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_CLIPSIBLINGS);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
      SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    iupDialogCustomFrameSimulateCheckCallbacks(ih);
  }

  iupwinuiProcessPendingMessages();

  if (iupAttribGetInt(ih, "TASKBARPROGRESS"))
  {
    ITaskbarList3* tbl = NULL;
    CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&tbl));
    if (tbl)
      iupAttribSet(ih, "_IUPWINUI_TASKBARLIST", (char*)tbl);
  }

  aux->windowCreated = true;

  return IUP_NOERROR;
}

static void winuiDialogUnMapMethod(Ihandle* ih)
{
  ITaskbarList3* tbl = (ITaskbarList3*)iupAttribGet(ih, "_IUPWINUI_TASKBARLIST");
  if (tbl)
  {
    tbl->Release();
    iupAttribSet(ih, "_IUPWINUI_TASKBARLIST", NULL);
  }

  IupWinUIDialogAux* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
  if (aux)
  {
    if (aux->xamlSource)
    {
      if (aux->takeFocusToken)
        aux->xamlSource.TakeFocusRequested(aux->takeFocusToken);
      if (aux->gotFocusToken)
        aux->xamlSource.GotFocus(aux->gotFocusToken);

      aux->xamlSource.Close();
      aux->xamlSource = nullptr;
    }

    aux->siteBridge = nullptr;
    aux->menuBar = nullptr;
    aux->contentCanvas = nullptr;
    aux->rootPanel = nullptr;
  }

  if (ih->handle)
  {
    HWND hwnd = (HWND)ih->handle;
    DestroyWindow(hwnd);
  }
  winuiFreeAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
  ih->handle = NULL;
}

static void winuiDialogUpdateDescendants(Ihandle* ih)
{
  Ihandle* child = ih->firstchild;
  while (child)
  {
    if (child->handle)
      iupLayoutUpdate(child);
    else
      winuiDialogUpdateDescendants(child);
    child = child->brother;
  }
}

static void winuiDialogLayoutUpdateMethod(Ihandle* ih)
{
  if (!ih->handle)
    return;

  if (ih->data->ignore_resize)
    return;

  ih->data->ignore_resize = 1;

  SetWindowPos((HWND)ih->handle, 0, 0, 0, ih->currentwidth, ih->currentheight,
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);

  winuiDialogUpdateXamlIsland(ih);

  winuiDialogUpdateDescendants(ih);

  ih->data->ignore_resize = 0;
}

static int winuiDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    HWND hwnd = (HWND)ih->handle;
    std::wstring wtitle = value ? iupwinuiStringToWString(value) : L"";
    SetWindowTextW(hwnd, wtitle.c_str());
  }
  return 1;
}

static char* winuiDialogGetTitleAttrib(Ihandle* ih)
{
  if (ih->handle)
  {
    HWND hwnd = (HWND)ih->handle;
    int len = GetWindowTextLengthW(hwnd);
    if (len > 0)
    {
      std::wstring wtitle(len + 1, L'\0');
      GetWindowTextW(hwnd, &wtitle[0], len + 1);
      return iupwinuiHStringToString(hstring(wtitle.c_str()));
    }
  }
  return NULL;
}

static void* winuiDialogGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return ih->handle;
}

static char* winuiDialogGetClientSizeAttrib(Ihandle* ih)
{
  if (ih->handle)
  {
    RECT rect;
    GetClientRect((HWND)ih->handle, &rect);
    return iupStrReturnIntInt((int)(rect.right - rect.left), (int)(rect.bottom - rect.top), 'x');
  }
  return NULL;
}

static char* winuiDialogGetClientOffsetAttrib(Ihandle* ih)
{
  (void)ih;
  return iupStrReturnIntInt(0, 0, 'x');
}

static int winuiDialogSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    iupAttribSetStr(ih, "_IUPWINUI_BACKGROUND_COLOR", value);
    if (ih->handle)
    {
      IupWinUIDialogAux* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
      if (aux && aux->rootPanel)
      {
        Windows::UI::Color color;
        color.A = 255;
        color.R = r;
        color.G = g;
        color.B = b;
        aux->rootPanel.Background(SolidColorBrush(color));
      }
      RedrawWindow((HWND)ih->handle, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
    return 1;
  }
  return 0;
}

static int winuiDialogSetTopMostAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    if (iupStrBoolean(value))
      SetWindowPos((HWND)ih->handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
    else
      SetWindowPos((HWND)ih->handle, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
  }
  return 1;
}

static int winuiDialogSetBringFrontAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle && iupStrBoolean(value))
  {
    HWND hwnd = (HWND)ih->handle;

    if (IsIconic(hwnd))
      ShowWindow(hwnd, SW_RESTORE);

    iupwinuiBringWindowToForeground(hwnd);
  }
  return 0;
}

static char* winuiDialogGetActiveWindowAttrib(Ihandle* ih)
{
  if (ih->handle)
  {
    WINDOWINFO wi;
    wi.cbSize = sizeof(WINDOWINFO);
    GetWindowInfo((HWND)ih->handle, &wi);
    return iupStrReturnBoolean(wi.dwWindowStatus & WS_ACTIVECAPTION);
  }
  return NULL;
}

static char* winuiDialogGetMaximizedAttrib(Ihandle* ih)
{
  if (ih->handle)
    return iupStrReturnBoolean(IsZoomed((HWND)ih->handle));
  return NULL;
}

static char* winuiDialogGetMinimizedAttrib(Ihandle* ih)
{
  if (ih->handle)
    return iupStrReturnBoolean(IsIconic((HWND)ih->handle));
  return NULL;
}

static int winuiDialogSetIconAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 1;

  HWND hwnd = (HWND)ih->handle;

  if (!value)
  {
    SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)NULL);
    SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)NULL);
  }
  else
  {
    HICON icon = (HICON)iupImageGetIcon(value);
    if (icon)
    {
      SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)icon);
      SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)icon);
    }
  }

  if (IsIconic(hwnd))
    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
  else
    RedrawWindow(hwnd, NULL, NULL, RDW_FRAME | RDW_UPDATENOW);

  return 1;
}

extern "C" void iupdrvDialogGetPosition(Ihandle* ih, InativeHandle* handle, int* x, int* y)
{
  RECT rect;
  if (!handle)
    handle = ih->handle;
  GetWindowRect((HWND)handle, &rect);
  if (x) *x = rect.left;
  if (y) *y = rect.top;
}

extern "C" void iupdrvDialogSetPosition(Ihandle* ih, int x, int y)
{
  int flags = SWP_NOSIZE;
  if (iupAttribGetBoolean(ih, "SHOWNOACTIVATE"))
    flags |= SWP_NOACTIVATE;
  SetWindowPos((HWND)ih->handle, HWND_TOP, x, y, 0, 0, flags);
}

extern "C" void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int* w, int* h)
{
  RECT rect;
  if (!handle)
    handle = ih->handle;
  GetWindowRect((HWND)handle, &rect);
  if (w) *w = rect.right - rect.left;
  if (h) *h = rect.bottom - rect.top;
}

extern "C" void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  IupWinUIDialogAux* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
  if (aux)
    aux->isVisible = visible ? true : false;

  if (ih->handle)
  {
    HWND hwnd = (HWND)ih->handle;

    if (visible)
    {
      ShowWindow(hwnd, ih->data->cmd_show);
      UpdateWindow(hwnd);
    }
    else
    {
      ShowWindow(hwnd, SW_HIDE);
    }
  }
}

extern "C" int iupdrvDialogIsVisible(Ihandle* ih)
{
  if (ih->handle)
  {
    HWND hwnd = (HWND)ih->handle;
    return IsWindowVisible(hwnd) ? 1 : 0;
  }
  return 0;
}

extern "C" int iupdrvDialogSetPlacement(Ihandle* ih)
{
  char* placement;
  int no_activate = iupAttribGetBoolean(ih, "SHOWNOACTIVATE");

  if (no_activate)
    ih->data->cmd_show = SW_SHOWNOACTIVATE;
  else
    ih->data->cmd_show = SW_SHOWNORMAL;
  ih->data->show_state = IUP_SHOW;

  if (iupAttribGetBoolean(ih, "FULLSCREEN"))
    return 1;

  placement = iupAttribGet(ih, "PLACEMENT");
  if (!placement)
  {
    if (IsIconic((HWND)ih->handle) || IsZoomed((HWND)ih->handle))
      ih->data->show_state = IUP_RESTORE;

    return 0;
  }

  if (iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    if (no_activate)
      ih->data->cmd_show = SW_MAXIMIZE;
    else
      ih->data->cmd_show = SW_SHOWMAXIMIZED;
    ih->data->show_state = IUP_MAXIMIZE;
  }
  else if (iupStrEqualNoCase(placement, "MINIMIZED"))
  {
    if (no_activate)
      ih->data->cmd_show = SW_SHOWMINNOACTIVE;
    else if (iupAttribGetBoolean(ih, "SHOWMINIMIZENEXT"))
      ih->data->cmd_show = SW_MINIMIZE;
    else
      ih->data->cmd_show = SW_SHOWMINIMIZED;
    ih->data->show_state = IUP_MINIMIZE;
  }
  else if (iupStrEqualNoCase(placement, "FULL"))
  {
    int width, height, x, y;
    int caption, border, menu;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    iupdrvGetFullSize(&width, &height);

    x = -(border);
    y = -(border + caption + menu);

    width += 2 * border;
    height += 2 * border + caption + menu;

    SetWindowPos((HWND)ih->handle, HWND_TOP, x, y, width, height, 0);

    if (IsIconic((HWND)ih->handle) || IsZoomed((HWND)ih->handle))
      ih->data->show_state = IUP_RESTORE;
  }

  iupAttribSet(ih, "PLACEMENT", NULL);
  return 1;
}

extern "C" void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* parent)
{
  if (!ih || !ih->handle)
    return;

  HWND hwnd = (HWND)ih->handle;
  SetWindowLongPtr(hwnd, GWLP_HWNDPARENT, (LONG_PTR)parent);
}

extern "C" void iupdrvDialogGetDecoration(Ihandle* ih, int* border, int* caption, int* menu)
{
  if (ih->data->menu)
    *menu = iupdrvMenuGetMenuBarSize(ih->data->menu);
  else
    *menu = 0;

  if (ih->handle)
  {
    WINDOWINFO wi;
    wi.cbSize = sizeof(WINDOWINFO);
    GetWindowInfo((HWND)ih->handle, &wi);
    *border = wi.cxWindowBorders;

    *caption = iupAttribGetInt(ih, "CUSTOMFRAMECAPTIONHEIGHT");
    if (*caption == 0)
    {
      Ihandle* ih_caption = IupGetDialogChild(ih, "CUSTOMFRAMECAPTION");
      if (ih_caption)
      {
        *caption = ih_caption->currentheight;
      }
      else if (wi.rcClient.bottom == wi.rcClient.top ||
               wi.rcClient.top > wi.rcWindow.bottom ||
               (wi.rcWindow.bottom - wi.rcWindow.top) == (wi.rcClient.bottom - wi.rcClient.top))
      {
        if (wi.dwStyle & WS_CAPTION)
          *caption = GetSystemMetrics(SM_CYCAPTION);
        else
          *caption = 0;
      }
      else
      {
        *caption = (wi.rcWindow.bottom - wi.rcWindow.top) - 2 * wi.cyWindowBorders - (wi.rcClient.bottom - wi.rcClient.top);
      }
    }
  }
  else
  {
    int padded_border = 0;
    int has_titlebar = iupAttribGetBoolean(ih, "MAXBOX") ||
                       iupAttribGetBoolean(ih, "MINBOX") ||
                       iupAttribGetBoolean(ih, "MENUBOX") ||
                       iupAttribGet(ih, "TITLE");

    *caption = 0;
    if (has_titlebar)
    {
      *caption = iupAttribGetInt(ih, "CUSTOMFRAMECAPTIONHEIGHT");
      if (*caption == 0)
      {
        Ihandle* ih_caption = IupGetDialogChild(ih, "CUSTOMFRAMECAPTION");
        if (ih_caption)
          *caption = ih_caption->currentheight;
        else
          *caption = GetSystemMetrics(SM_CYCAPTION);
      }
      padded_border = GetSystemMetrics(SM_CXPADDEDBORDER);
    }

    *border = 0;
    if (iupAttribGetBoolean(ih, "CUSTOMFRAME"))
    {
      *border = 0;
    }
    else if (iupAttribGetBoolean(ih, "RESIZE"))
      *border = GetSystemMetrics(SM_CXFRAME);
    else if (has_titlebar)
      *border = GetSystemMetrics(SM_CXFIXEDFRAME);
    else if (iupAttribGetBoolean(ih, "BORDER"))
      *border = GetSystemMetrics(SM_CXBORDER);

    if (*border)
      *border += padded_border;
  }
}

static int winuiDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    if (!iupAttribGet(ih, "_IUPWINUI_FS_STYLE"))
    {
      int width, height;
      LONG off_style, new_style;
      HWND hwnd = (HWND)ih->handle;
      BOOL visible = ShowWindow(hwnd, SW_HIDE);

      off_style = WS_BORDER | WS_THICKFRAME | WS_CAPTION |
                  WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;
      new_style = GetWindowLong(hwnd, GWL_STYLE);
      iupAttribSet(ih, "_IUPWINUI_FS_STYLE", (char*)(intptr_t)new_style);
      new_style &= (~off_style);
      SetWindowLong(hwnd, GWL_STYLE, new_style);

      iupAttribSetStr(ih, "_IUPWINUI_FS_MAXBOX", iupAttribGet(ih, "MAXBOX"));
      iupAttribSetStr(ih, "_IUPWINUI_FS_MINBOX", iupAttribGet(ih, "MINBOX"));
      iupAttribSetStr(ih, "_IUPWINUI_FS_MENUBOX", iupAttribGet(ih, "MENUBOX"));
      iupAttribSetStr(ih, "_IUPWINUI_FS_RESIZE", iupAttribGet(ih, "RESIZE"));
      iupAttribSetStr(ih, "_IUPWINUI_FS_BORDER", iupAttribGet(ih, "BORDER"));
      iupAttribSetStr(ih, "_IUPWINUI_FS_TITLE", iupAttribGet(ih, "TITLE"));

      iupAttribSetStr(ih, "_IUPWINUI_FS_X", IupGetAttribute(ih, "X"));
      iupAttribSetStr(ih, "_IUPWINUI_FS_Y", IupGetAttribute(ih, "Y"));
      iupAttribSetStr(ih, "_IUPWINUI_FS_SIZE", IupGetAttribute(ih, "RASTERSIZE"));

      iupAttribSet(ih, "MAXBOX", "NO");
      iupAttribSet(ih, "MINBOX", "NO");
      iupAttribSet(ih, "MENUBOX", "NO");
      IupSetAttribute(ih, "TITLE", NULL);
      iupAttribSet(ih, "RESIZE", "NO");
      iupAttribSet(ih, "BORDER", "NO");

      iupdrvGetFullSize(&width, &height);

      SetWindowPos(hwnd, HWND_TOP, 0, 0, width, height, SWP_FRAMECHANGED);

      winuiDialogUpdateXamlIsland(ih);

      if (visible)
        ShowWindow(hwnd, SW_SHOW);
    }
  }
  else
  {
    LONG style = (LONG)(intptr_t)iupAttribGet(ih, "_IUPWINUI_FS_STYLE");
    if (style)
    {
      HWND hwnd = (HWND)ih->handle;
      BOOL visible = ShowWindow(hwnd, SW_HIDE);

      iupAttribSetStr(ih, "MAXBOX", iupAttribGet(ih, "_IUPWINUI_FS_MAXBOX"));
      iupAttribSetStr(ih, "MINBOX", iupAttribGet(ih, "_IUPWINUI_FS_MINBOX"));
      iupAttribSetStr(ih, "MENUBOX", iupAttribGet(ih, "_IUPWINUI_FS_MENUBOX"));
      IupSetAttribute(ih, "TITLE", iupAttribGet(ih, "_IUPWINUI_FS_TITLE"));
      iupAttribSetStr(ih, "RESIZE", iupAttribGet(ih, "_IUPWINUI_FS_RESIZE"));
      iupAttribSetStr(ih, "BORDER", iupAttribGet(ih, "_IUPWINUI_FS_BORDER"));

      SetWindowLong(hwnd, GWL_STYLE, style);

      SetWindowPos(hwnd, HWND_TOP,
                   iupAttribGetInt(ih, "_IUPWINUI_FS_X"),
                   iupAttribGetInt(ih, "_IUPWINUI_FS_Y"),
                   IupGetInt(ih, "_IUPWINUI_FS_SIZE"),
                   IupGetInt2(ih, "_IUPWINUI_FS_SIZE"), 0);

      winuiDialogUpdateXamlIsland(ih);

      if (visible)
        ShowWindow(hwnd, SW_SHOW);

      iupAttribSet(ih, "_IUPWINUI_FS_STYLE", NULL);
      iupAttribSet(ih, "_IUPWINUI_FS_MAXBOX", NULL);
      iupAttribSet(ih, "_IUPWINUI_FS_MINBOX", NULL);
      iupAttribSet(ih, "_IUPWINUI_FS_MENUBOX", NULL);
      iupAttribSet(ih, "_IUPWINUI_FS_TITLE", NULL);
      iupAttribSet(ih, "_IUPWINUI_FS_RESIZE", NULL);
      iupAttribSet(ih, "_IUPWINUI_FS_BORDER", NULL);
      iupAttribSet(ih, "_IUPWINUI_FS_X", NULL);
      iupAttribSet(ih, "_IUPWINUI_FS_Y", NULL);
      iupAttribSet(ih, "_IUPWINUI_FS_SIZE", NULL);
    }
  }
  return 1;
}

static int winuiDialogSetOpacityAttrib(Ihandle* ih, const char* value)
{
  int opacity;
  if (!iupStrToInt(value, &opacity))
    return 0;

  HWND hwnd = (HWND)ih->handle;
  LONG exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
  if (!(exstyle & WS_EX_LAYERED))
    SetWindowLong(hwnd, GWL_EXSTYLE, exstyle | WS_EX_LAYERED);

  SetLayeredWindowAttributes(hwnd, 0, (BYTE)opacity, LWA_ALPHA);
  RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
  return 1;
}

static int winuiDialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  if (winuiDialogSetBgColorAttrib(ih, value))
    return 1;

  void* handle = iupImageGetImage(value, ih, 0, NULL);
  if (handle)
  {
    WriteableBitmap bitmap = winuiGetBitmapFromHandle(handle);
    if (bitmap)
    {
      iupAttribSet(ih, "_IUPWINUI_BACKGROUND_COLOR", NULL);

      if (ih->handle)
      {
        IupWinUIDialogAux* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
        if (aux && aux->rootPanel)
        {
          ImageBrush brush;
          brush.ImageSource(bitmap);
          brush.Stretch(Stretch::Fill);
          aux->rootPanel.Background(brush);
        }
        RedrawWindow((HWND)ih->handle, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
      }
      return 1;
    }
  }

  return 0;
}

static int winuiDialogSetShapeImageAttrib(Ihandle* ih, const char* value)
{
  Ihandle* image = IupGetHandle(value);
  if (!image)
  {
    SetWindowRgn((HWND)ih->handle, NULL, TRUE);
    return 0;
  }

  unsigned char* imgdata = (unsigned char*)iupAttribGet(image, "WID");
  int channels = iupAttribGetInt(image, "CHANNELS");
  int w = image->currentwidth;
  int h = image->currentheight;

  if (!imgdata || channels != 4)
    return 0;

  HRGN hRgn = CreateRectRgn(0, 0, 0, 0);

  for (int y = 0; y < h; y++)
  {
    int start_x = -1;

    for (int x = 0; x < w; x++)
    {
      if (imgdata[3] == 0 || x == w - 1)
      {
        if (start_x != -1)
        {
          HRGN hTmpRgn = CreateRectRgn(start_x, y, x, y + 1);
          CombineRgn(hRgn, hRgn, hTmpRgn, RGN_OR);
          DeleteObject(hTmpRgn);
          start_x = -1;
        }
      }
      else
      {
        if (start_x == -1)
          start_x = x;
      }

      imgdata += 4;
    }
  }

  SetWindowRgn((HWND)ih->handle, hRgn, TRUE);

  return 1;
}

static int winuiDialogSetTaskBarProgressValueAttrib(Ihandle* ih, const char* value)
{
  ITaskbarList3* tbl = (ITaskbarList3*)iupAttribGet(ih, "_IUPWINUI_TASKBARLIST");
  if (tbl)
  {
    int perc;
    iupStrToInt(value, &perc);
    tbl->SetProgressValue((HWND)ih->handle, perc, 100);

    if (perc == 100)
      tbl->SetProgressState((HWND)ih->handle, TBPF_NOPROGRESS);
  }

  return 0;
}

static int winuiDialogSetTaskBarProgressStateAttrib(Ihandle* ih, const char* value)
{
  ITaskbarList3* tbl = (ITaskbarList3*)iupAttribGet(ih, "_IUPWINUI_TASKBARLIST");
  if (tbl)
  {
    if (iupStrEqualNoCase(value, "NOPROGRESS"))
      tbl->SetProgressState((HWND)ih->handle, TBPF_NOPROGRESS);
    else if (iupStrEqualNoCase(value, "INDETERMINATE"))
      tbl->SetProgressState((HWND)ih->handle, TBPF_INDETERMINATE);
    else if (iupStrEqualNoCase(value, "ERROR"))
      tbl->SetProgressState((HWND)ih->handle, TBPF_ERROR);
    else if (iupStrEqualNoCase(value, "PAUSED"))
      tbl->SetProgressState((HWND)ih->handle, TBPF_PAUSED);
    else
      tbl->SetProgressState((HWND)ih->handle, TBPF_NORMAL);
  }

  return 0;
}

void winuiDialogSetMenuBar(Ihandle* ih, MenuBar menuBar)
{
  IupWinUIDialogAux* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
  if (!aux || !aux->rootPanel)
    return;

  if (aux->menuBar)
  {
    uint32_t index;
    if (aux->rootPanel.Children().IndexOf(aux->menuBar, index))
      aux->rootPanel.Children().RemoveAt(index);
    aux->menuBar = nullptr;
  }

  if (menuBar)
  {
    Grid::SetRow(menuBar, 0);
    aux->rootPanel.Children().InsertAt(0, menuBar);
    aux->menuBar = menuBar;
  }
}

extern "C" void iupdrvDialogInitClass(Iclass* ic)
{
  ic->Map = winuiDialogMapMethod;
  ic->UnMap = winuiDialogUnMapMethod;
  ic->LayoutUpdate = winuiDialogLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = winuiDialogGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, winuiDialogSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "TITLE", winuiDialogGetTitleAttrib, winuiDialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CLIENTSIZE", winuiDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", winuiDialogGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MINSIZE", NULL, iupBaseSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXSIZE", NULL, iupBaseSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ACTIVEWINDOW", winuiDialogGetActiveWindowAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPMOST", NULL, winuiDialogSetTopMostAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BRINGFRONT", NULL, winuiDialogSetBringFrontAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MAXIMIZED", winuiDialogGetMaximizedAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINIMIZED", winuiDialogGetMinimizedAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ICON", NULL, winuiDialogSetIconAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HWND", iupBaseGetWidAttrib, NULL, NULL, NULL, IUPAF_NO_STRING | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CUSTOMFRAME", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUSTOMFRAMECAPTIONHEIGHT", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FULLSCREEN", NULL, winuiDialogSetFullScreenAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "OPACITY", NULL, winuiDialogSetOpacityAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LAYERALPHA", NULL, winuiDialogSetOpacityAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, winuiDialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHAPEIMAGE", NULL, winuiDialogSetShapeImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TASKBARPROGRESS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TASKBARPROGRESSSTATE", NULL, winuiDialogSetTaskBarProgressStateAttrib, IUPAF_SAMEASSYSTEM, "NORMAL", IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TASKBARPROGRESSVALUE", NULL, winuiDialogSetTaskBarProgressValueAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHOWNOACTIVATE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWMINIMIZENEXT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "OPACITYIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMPOSITED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTROL", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HELPBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
