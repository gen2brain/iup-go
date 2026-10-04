/** \file
 * \brief WinUI Dialog using Win32 HWND with DesktopWindowXamlSource
 *
 * See Copyright Notice in "iup.h"
 */

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
#include "iup_dialog.h"
#include "iup_layout.h"
#include "iup_focus.h"
#include "iup_classbase.h"
#include "iup_image.h"
#include "iup_drvinfo.h"
#include "iup_globalattrib.h"
#include "iup_dlglist.h"
#include "iup_key.h"
}

#include "iupwinui_drv.h"

#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>

using namespace winrt;
using namespace Microsoft::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Content;
using namespace Microsoft::UI::Windowing;
using namespace Windows::Foundation;
using namespace Windows::Graphics;


#define WINUI_DIALOG_CLASS L"IupWinUIDialog"

static void winuiDialogSetPanelBgColor(Grid rootPanel, const char* color)
{
  unsigned char r, g, b;
  if (color && iupStrToRGB(color, &r, &g, &b))
  {
    Windows::UI::Color c;
    c.A = 255;
    c.R = r;
    c.G = g;
    c.B = b;
    rootPanel.Background(SolidColorBrush(c));
  }
}

static void winuiDialogUpdateXamlIsland(Ihandle* ih)
{
  auto* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
  if (!aux)
    return;

  RECT rect;
  GetClientRect(static_cast<HWND>(ih->handle), &rect);

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
    double scale = iupwinuiGetScale(ih);
    aux->rootPanel.Width(static_cast<double>(rect.right) / scale);
    aux->rootPanel.Height(static_cast<double>(rect.bottom) / scale);
  }
}

static void winuiDialogResize(Ihandle* ih, int width, int height)
{
  IFnii cb;

  iupdrvDialogGetSize(ih, nullptr, &(ih->currentwidth), &(ih->currentheight));

  cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "RESIZE_CB"));
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

static void winuiDialogUpdateDpi(Ihandle* ih)
{
  Ihandle* child = ih->firstchild;
  while (child)
  {
    iupBaseUpdateAttribFromFont(child);

    if (winuiGetAux<IupWinUITableAux>(child, IUPWINUI_TABLE_AUX))
      winuiTableUpdateDpi(child);
    else if (winuiGetAux<IupWinUITreeAux>(child, IUPWINUI_TREE_AUX))
      winuiTreeUpdateDpi(child);

    winuiDialogUpdateDpi(child);
    child = child->brother;
  }
}

static void winuiDialogTitleBarThemeColor(HWND hwnd)
{
  typedef HRESULT(STDAPICALLTYPE* PtrDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
  static PtrDwmSetWindowAttribute pDwmSetWindowAttribute = nullptr;
  static int initialized = 0;

  if (!initialized)
  {
    HMODULE dwmLibrary = LoadLibrary(TEXT("dwmapi.dll"));
    if (dwmLibrary)
      pDwmSetWindowAttribute = reinterpret_cast<PtrDwmSetWindowAttribute>(GetProcAddress(dwmLibrary, "DwmSetWindowAttribute"));
    initialized = 1;
  }

  if (pDwmSetWindowAttribute)
  {
    BOOL value = iupGlobalIsDarkMode() ? TRUE : FALSE;
    pDwmSetWindowAttribute(hwnd, 20, &value, sizeof(value));
  }
}

extern "C" IUP_SDK_API void iupdrvSetAppearance(int appearance)
{
  Ihandle* ih;

  iupwinuiSetGlobalColors();

  for (ih = iupDlgListFirst(); ih; ih = iupDlgListNext())
  {
    auto* dlgaux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
    const char* bgcolor;

    if (!dlgaux || !dlgaux->rootPanel)
      continue;

    if (appearance == IUP_APPEARANCE_SYSTEM)
      dlgaux->rootPanel.RequestedTheme(ElementTheme::Default);
    else
      dlgaux->rootPanel.RequestedTheme(appearance == IUP_APPEARANCE_DARK? ElementTheme::Dark: ElementTheme::Light);

    if (ih->handle)
      winuiDialogTitleBarThemeColor(static_cast<HWND>(ih->handle));

    bgcolor = IupGetGlobal("DLGBGCOLOR");
    if (bgcolor)
    {
      iupAttribSetStr(ih, "_IUPWINUI_BACKGROUND_COLOR", bgcolor);
      if (!iupAttribGet(ih, "_IUPWINUI_BACKDROP_ACTIVE"))
        winuiDialogSetPanelBgColor(dlgaux->rootPanel, bgcolor);
    }

    winuiDialogRefreshThemeColors(ih);
  }
}

/* Windows has no resize increment hint, the drag rectangle has to be rounded here */
static int winuiDialogCheckSizing(Ihandle* ih, WPARAM edge, RECT* rect)
{
  int inc_w = 0, inc_h = 0;
  int min_w = 1, min_h = 1;
  int width, height, steps;

  if (!iupStrToIntInt(iupAttribGet(ih, "RESIZEINC"), &inc_w, &inc_h, 'x') || (inc_w <= 1 && inc_h <= 1))
    return 0;

  iupStrToIntInt(iupAttribGet(ih, "MINSIZE"), &min_w, &min_h, 'x');

  width = rect->right - rect->left;
  height = rect->bottom - rect->top;

  if (inc_w > 1)
  {
    steps = (width - min_w) / inc_w;
    if (steps < 0) steps = 0;
    width = min_w + steps * inc_w;
  }

  if (inc_h > 1)
  {
    steps = (height - min_h) / inc_h;
    if (steps < 0) steps = 0;
    height = min_h + steps * inc_h;
  }

  if (edge == WMSZ_LEFT || edge == WMSZ_TOPLEFT || edge == WMSZ_BOTTOMLEFT)
    rect->left = rect->right - width;
  else
    rect->right = rect->left + width;

  if (edge == WMSZ_TOP || edge == WMSZ_TOPLEFT || edge == WMSZ_TOPRIGHT)
    rect->top = rect->bottom - height;
  else
    rect->bottom = rect->top + height;

  return 1;
}

static LRESULT CALLBACK winuiDialogWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  auto* ih = reinterpret_cast<Ihandle*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

  switch (msg)
  {
    case WM_CREATE:
    {
      auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
      SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
      return 0;
    }

    case WM_NCCALCSIZE:
    {
      LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
      if (wParam && ih && !(style & WS_CAPTION) && (style & WS_THICKFRAME) && !IsZoomed(hwnd) &&
          !iupAttribGetBoolean(ih, "CUSTOMFRAME"))
      {
        auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
        LONG top = params->rgrc[0].top;
        LRESULT result = DefWindowProc(hwnd, msg, wParam, lParam);
        params->rgrc[0].top = top;
        return result;
      }
      break;
    }

    case WM_ERASEBKGND:
    {
      if (ih)
      {
        if (iupAttribGet(ih, "_IUPWINUI_BACKDROP_ACTIVE"))
          return 1;

        unsigned char r, g, b;
        const char* color = iupAttribGet(ih, "_IUPWINUI_BACKGROUND_COLOR");
        if (color && iupStrToRGB(color, &r, &g, &b))
        {
          HDC hdc = reinterpret_cast<HDC>(wParam);
          RECT rect;
          GetClientRect(hwnd, &rect);
          SetDCBrushColor(hdc, RGB(r, g, b));
          FillRect(hdc, &rect, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
          return 1;
        }
      }
      break;
    }

    case WM_SETCURSOR:
    {
      if (LOWORD(lParam) == HTCLIENT)
      {
        HCURSOR hCur = ih ? reinterpret_cast<HCURSOR>(iupAttribGet(ih, "_IUPWIN_HCURSOR")) : nullptr;
        SetCursor(hCur ? hCur : LoadCursor(nullptr, IDC_ARROW));
        return TRUE;
      }
      break;
    }

    case WM_ACTIVATE:
    {
      if (ih)
      {
        auto* dlgaux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
        if (dlgaux && dlgaux->windowCreated)
        {
          if (LOWORD(wParam) == WA_INACTIVE)
          {
            dlgaux->lastFocusedHwnd = GetFocus();
            iupCallKillFocusCb(ih);
          }
          else
            iupCallGetFocusCb(ih);
        }
      }
      break;
    }

    case WM_SETFOCUS:
    {
      if (ih)
      {
        auto* dlgaux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
        if (dlgaux && dlgaux->lastFocusedHwnd && dlgaux->lastFocusedHwnd != hwnd)
          SetFocus(dlgaux->lastFocusedHwnd);
        else if (dlgaux && dlgaux->xamlSource)
        {
          XamlSourceFocusNavigationRequest request(XamlSourceFocusNavigationReason::First);
          dlgaux->xamlSource.NavigateFocus(request);
        }

        auto* lastfocus = reinterpret_cast<Ihandle*>(iupAttribGet(ih, "_IUPWINUI_LASTFOCUS"));
        if (iupObjectCheck(lastfocus) && IupGetFocus() != lastfocus &&
            !iupAttribGetBoolean(ih, "IGNORELASTFOCUS"))
          IupSetFocus(lastfocus);
      }
      return 0;
    }

    case WM_SIZE:
    {
      IupWinUIDialogAux* dlgaux = ih ? winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX) : nullptr;
      if (ih && !ih->data->ignore_resize && dlgaux && dlgaux->windowCreated)
      {
        winuiDialogUpdateXamlIsland(ih);

        switch (wParam)
        {
        case SIZE_MINIMIZED:
          if (ih->data->show_state != IUP_MINIMIZE)
          {
            IFni show_cb = reinterpret_cast<IFni>(IupGetCallback(ih, "SHOW_CB"));
            ih->data->show_state = IUP_MINIMIZE;
            if (show_cb && show_cb(ih, IUP_MINIMIZE) == IUP_CLOSE)
              IupExitLoop();
          }
          break;
        case SIZE_MAXIMIZED:
          if (ih->data->show_state != IUP_MAXIMIZE)
          {
            IFni show_cb = reinterpret_cast<IFni>(IupGetCallback(ih, "SHOW_CB"));
            ih->data->show_state = IUP_MAXIMIZE;
            if (show_cb && show_cb(ih, IUP_MAXIMIZE) == IUP_CLOSE)
              IupExitLoop();
          }
          winuiDialogResize(ih, LOWORD(lParam), HIWORD(lParam));
          break;
        case SIZE_RESTORED:
          if (ih->data->show_state == IUP_MAXIMIZE || ih->data->show_state == IUP_MINIMIZE)
          {
            IFni show_cb = reinterpret_cast<IFni>(IupGetCallback(ih, "SHOW_CB"));
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

    case WM_SIZING:
    {
      if (ih && winuiDialogCheckSizing(ih, wParam, reinterpret_cast<RECT*>(lParam)))
        return TRUE;
      break;
    }

    case WM_DPICHANGED:
    {
      IupWinUIDialogAux* dlgaux = ih ? winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX) : nullptr;
      if (dlgaux)
      {
        iupBaseUpdateAttribFromFont(ih);
        winuiDialogUpdateDpi(ih);
        if (dlgaux->rootPanel)
          winuiImageUpdateScale(dlgaux->rootPanel, iupwinuiGetScale(ih));
        IupRefresh(ih);
      }
      break;
    }

    case WM_GETMINMAXINFO:
    {
      if (ih)
      {
        auto* minmax = reinterpret_cast<MINMAXINFO*>(lParam);
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
        auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "MOVE_CB"));
        if (cb)
        {
          int x, y;
          iupdrvDialogGetPosition(ih, nullptr, &x, &y);
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
        if (iupAttribGet(ih, "_IUPWINUI_CONTENT_DIALOG_ACTIVE"))
          return 0;

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
        auto area = reinterpret_cast<LPCTSTR>(lParam);
        if (lstrcmp(area, TEXT("ImmersiveColorSet")) == 0)
        {
          winuiDialogTitleBarThemeColor(hwnd);
          iupwinuiSetGlobalColors();

          int dark_mode = iupGlobalIsDarkMode();

          auto* dlgaux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
          if (dlgaux && dlgaux->rootPanel)
            dlgaux->rootPanel.RequestedTheme(dark_mode ? ElementTheme::Dark : ElementTheme::Light);

          const char* bgcolor = IupGetGlobal("DLGBGCOLOR");
          if (bgcolor)
          {
            iupAttribSetStr(ih, "_IUPWINUI_BACKGROUND_COLOR", bgcolor);
            if (dlgaux && dlgaux->rootPanel && !iupAttribGet(ih, "_IUPWINUI_BACKDROP_ACTIVE"))
              winuiDialogSetPanelBgColor(dlgaux->rootPanel, bgcolor);
          }

          iupGlobalNotifyThemeChanged();

          winuiDialogRefreshThemeColors(ih);

          RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        }
      }
      break;
    }

    case WM_DESTROY:
      break;

    case WM_NCDESTROY:
      SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
      return 0;
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

static BOOL CALLBACK winuiDialogFirstIconProc(HMODULE module, LPCWSTR type, LPWSTR name, LONG_PTR param)
{
  auto* icons = reinterpret_cast<HICON*>(param);
  (void)type;
  icons[0] = static_cast<HICON>(LoadImageW(module, name, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
  icons[1] = static_cast<HICON>(LoadImageW(module, name, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
  return FALSE;
}

static void winuiDialogRegisterClass()
{
  static bool registered = false;
  if (registered)
    return;

  HICON icons[2] = { nullptr, nullptr };
  EnumResourceNamesW(GetModuleHandle(nullptr), RT_GROUP_ICON, winuiDialogFirstIconProc, reinterpret_cast<LONG_PTR>(icons));

  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = winuiDialogWndProc;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.hIcon = icons[0];
  wc.hIconSm = icons[1];
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(COLOR_WINDOW + 1));
  wc.lpszClassName = WINUI_DIALOG_CLASS;

  RegisterClassExW(&wc);
  registered = true;
}

IUP_DRV_API Ihandle* iupwinuiDialogFromHwnd(HWND hwnd)
{
  wchar_t name[32];
  if (!hwnd || !GetClassNameW(hwnd, name, 32) || lstrcmpW(name, WINUI_DIALOG_CLASS) != 0)
    return nullptr;

  auto* ih = reinterpret_cast<Ihandle*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  return (ih && iupObjectCheck(ih)) ? ih : nullptr;
}

static int winuiDialogSetBackdropAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 1;

  auto* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
  if (!aux || !aux->xamlSource)
    return 1;

  if (!value || value[0] == '\0')
  {
    aux->xamlSource.SystemBackdrop(nullptr);
    iupAttribSet(ih, "_IUPWINUI_BACKDROP_ACTIVE", nullptr);

    if (aux->rootPanel)
      winuiDialogSetPanelBgColor(aux->rootPanel, iupAttribGet(ih, "_IUPWINUI_BACKGROUND_COLOR"));

    winuiDialogRefreshThemeColors(ih);
    return 1;
  }

  if (iupStrEqualNoCase(value, "MICA"))
  {
    Xaml::Media::MicaBackdrop backdrop;
    aux->xamlSource.SystemBackdrop(backdrop);
  }
  else if (iupStrEqualNoCase(value, "MICAALT"))
  {
    Xaml::Media::MicaBackdrop backdrop;
    backdrop.Kind(Microsoft::UI::Composition::SystemBackdrops::MicaKind::BaseAlt);
    aux->xamlSource.SystemBackdrop(backdrop);
  }
  else if (iupStrEqualNoCase(value, "ACRYLIC"))
  {
    Xaml::Media::DesktopAcrylicBackdrop backdrop;
    aux->xamlSource.SystemBackdrop(backdrop);
  }
  else
    return 1;

  if (aux->rootPanel)
    aux->rootPanel.Background(nullptr);
  iupAttribSetStr(ih, "_IUPWINUI_BACKDROP_ACTIVE", value);

  winuiDialogRefreshThemeColors(ih);

  return 1;
}

static int winuiDialogMapMethod(Ihandle* ih)
{
  auto* aux = new IupWinUIDialogAux();
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

    if (iupAttribGetBoolean(ih, "HIDETITLEBAR"))
      has_titlebar = 0;

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
  HWND parentHwnd = static_cast<HWND>(iupDialogGetNativeParent(ih));

  HWND hwnd = CreateWindowExW(
    dwExStyle,
    WINUI_DIALOG_CLASS,
    wtitle.c_str(),
    dwStyle,
    0, 0,
    100, 100,
    parentHwnd,
    nullptr,
    GetModuleHandle(nullptr),
    ih
  );

  if (!hwnd)
  {
    winuiFreeAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
    return IUP_ERROR;
  }

  ih->handle = static_cast<InativeHandle*>(hwnd);

  winuiDialogTitleBarThemeColor(hwnd);

  WindowId parentWindowId = winrt::Microsoft::UI::GetWindowIdFromWindow(hwnd);

  aux->appWindow = AppWindow::GetFromWindowId(parentWindowId);
  if (aux->appWindow)
    aux->appWindow.AssociateWithDispatcherQueue(winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread());

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
  {
    double scale = iupwinuiGetScale(ih);
    aux->rootPanel.Width(static_cast<double>(rect.right) / scale);
    aux->rootPanel.Height(static_cast<double>(rect.bottom) / scale);
  }

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

  aux->rootPanel.KeyDown([ih](IInspectable const&, Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& args) {
    if (args.Handled())
      return;

    int code = iupwinuiKeyDecode(static_cast<int>(args.Key()), args.KeyStatus().IsExtendedKey? 1: 0);
    if (code == K_ESC || code == K_CR)
    {
      Ihandle* focus = IupGetFocus();
      Ihandle* nav_ih = (focus && iupObjectCheck(focus)) ? focus : ih;
      if (iupKeyProcessNavigation(nav_ih, code, (GetKeyState(VK_SHIFT) & 0x8000)))
        args.Handled(true);
    }
  });

  aux->rootPanel.PointerEntered([ih](IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&) {
    Icallback cb = IupGetCallback(ih, "ENTERWINDOW_CB");
    if (cb)
      cb(ih);
  });

  aux->rootPanel.PointerExited([ih](IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&) {
    Icallback cb = IupGetCallback(ih, "LEAVEWINDOW_CB");
    if (cb)
      cb(ih);
  });

  if (iupGlobalIsDarkMode())
    aux->rootPanel.RequestedTheme(ElementTheme::Dark);

  winuiDialogSetPanelBgColor(aux->rootPanel, IupGetGlobal("DLGBGCOLOR"));

  aux->xamlSource.Content(aux->rootPanel);

  aux->takeFocusToken = aux->xamlSource.TakeFocusRequested(
    [](DesktopWindowXamlSource const& sender, DesktopWindowXamlSourceTakeFocusRequestedEventArgs const& args) {
      static thread_local bool navigating = false;
      if (navigating)
        return;

      XamlSourceFocusNavigationReason reason = args.Request().Reason();
      if (reason != XamlSourceFocusNavigationReason::First && reason != XamlSourceFocusNavigationReason::Last)
        return;

      navigating = true;
      XamlSourceFocusNavigationRequest request(reason);
      sender.NavigateFocus(request);
      navigating = false;
    });

  {
    const char* backdrop = iupAttribGet(ih, "BACKDROP");
    if (backdrop && backdrop[0])
      winuiDialogSetBackdropAttrib(ih, backdrop);
  }

  if (customframe)
  {
    SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_CLIPSIBLINGS);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
      SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    iupDialogCustomFrameSimulateCheckCallbacks(ih);
  }

  iupwinuiProcessPendingMessages();

  if (iupAttribGetInt(ih, "TASKBARPROGRESS"))
  {
    ITaskbarList3* tbl = nullptr;
    CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&tbl));
    if (tbl)
      iupAttribSet(ih, "_IUPWINUI_TASKBARLIST", reinterpret_cast<char*>(tbl));
  }

  aux->windowCreated = true;

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  return IUP_NOERROR;
}

static void winuiDialogUnMapMethod(Ihandle* ih)
{
  if (ih->data->menu)
  {
    IupDestroy(ih->data->menu);
    ih->data->menu = nullptr;
  }

  iupwinuiTipsDestroy(ih);

  auto* tbl = reinterpret_cast<ITaskbarList3*>(iupAttribGet(ih, "_IUPWINUI_TASKBARLIST"));
  if (tbl)
  {
    tbl->Release();
    iupAttribSet(ih, "_IUPWINUI_TASKBARLIST", nullptr);
  }

  auto* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
  if (aux)
  {
    if (aux->xamlSource)
    {
      if (aux->takeFocusToken.value)
      {
        aux->xamlSource.TakeFocusRequested(aux->takeFocusToken);
        aux->takeFocusToken = {};
      }
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
    HWND hwnd = static_cast<HWND>(ih->handle);
    DestroyWindow(hwnd);
  }
  winuiFreeAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
  ih->handle = nullptr;
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

  SetWindowPos(static_cast<HWND>(ih->handle), nullptr, 0, 0, ih->currentwidth, ih->currentheight,
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);

  winuiDialogUpdateXamlIsland(ih);

  winuiDialogUpdateDescendants(ih);

  if (!iupAttribGetBoolean(ih, "BACKIMAGEZOOM") && iupAttribGet(ih, "BACKGROUND"))
  {
    char* background = iupAttribGet(ih, "BACKGROUND");
    unsigned char r, g, b;
    if (!iupStrToRGB(background, &r, &g, &b))
      IupSetAttribute(ih, "BACKGROUND", background);
  }

  ih->data->ignore_resize = 0;
}

static int winuiDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    HWND hwnd = static_cast<HWND>(ih->handle);
    std::wstring wtitle = value ? iupwinuiStringToWString(value) : L"";
    SetWindowTextW(hwnd, wtitle.c_str());
  }
  return 1;
}

static char* winuiDialogGetTitleAttrib(Ihandle* ih)
{
  if (ih->handle)
  {
    HWND hwnd = static_cast<HWND>(ih->handle);
    int len = GetWindowTextLengthW(hwnd);
    if (len > 0)
    {
      std::wstring wtitle(len + 1, L'\0');
      GetWindowTextW(hwnd, &wtitle[0], len + 1);
      return iupwinuiHStringToString(hstring(wtitle.c_str()));
    }
  }
  return nullptr;
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
    GetClientRect(static_cast<HWND>(ih->handle), &rect);
    return iupStrReturnIntInt(static_cast<int>(rect.right - rect.left), static_cast<int>(rect.bottom - rect.top), 'x');
  }
  return nullptr;
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
      auto* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
      if (aux && aux->rootPanel && !iupAttribGet(ih, "_IUPWINUI_BACKDROP_ACTIVE"))
        winuiDialogSetPanelBgColor(aux->rootPanel, value);
      RedrawWindow(static_cast<HWND>(ih->handle), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
      winuiDialogRefreshThemeColors(ih);
    }
    return 1;
  }
  return 0;
}

static int winuiDialogSetTopMostAttrib(Ihandle* ih, const char* value)
{
  auto* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
  if (aux && aux->appWindow)
  {
    OverlappedPresenter presenter = aux->appWindow.Presenter().try_as<OverlappedPresenter>();
    if (presenter)
      presenter.IsAlwaysOnTop(iupStrBoolean(value) != 0);
  }
  return 1;
}

static int winuiDialogSetHideTitleBarAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 1;
  HWND hwnd = static_cast<HWND>(ih->handle);
  RECT client_before, client_after, window;
  LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
  if (iupStrBoolean(value))
    style &= ~WS_CAPTION;
  else
    style |= WS_CAPTION;

  GetClientRect(hwnd, &client_before);
  SetWindowLongPtr(hwnd, GWL_STYLE, style);
  SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

  if (!IsZoomed(hwnd) && !IsIconic(hwnd))
  {
    GetClientRect(hwnd, &client_after);
    GetWindowRect(hwnd, &window);
    SetWindowPos(hwnd, nullptr, 0, 0,
                 (window.right - window.left) + (client_before.right - client_after.right),
                 (window.bottom - window.top) + (client_before.bottom - client_after.bottom),
                 SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
  }

  winuiDialogUpdateXamlIsland(ih);
  RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
  return 1;
}

static int winuiDialogSetBringFrontAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle && iupStrBoolean(value))
  {
    HWND hwnd = static_cast<HWND>(ih->handle);

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
    GetWindowInfo(static_cast<HWND>(ih->handle), &wi);
    return iupStrReturnBoolean(wi.dwWindowStatus & WS_ACTIVECAPTION);
  }
  return nullptr;
}

static char* winuiDialogGetMaximizedAttrib(Ihandle* ih)
{
  if (ih->handle)
    return iupStrReturnBoolean(IsZoomed(static_cast<HWND>(ih->handle)));
  return nullptr;
}

static char* winuiDialogGetMinimizedAttrib(Ihandle* ih)
{
  if (ih->handle)
    return iupStrReturnBoolean(IsIconic(static_cast<HWND>(ih->handle)));
  return nullptr;
}

static int winuiDialogSetIconAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 1;

  HWND hwnd = static_cast<HWND>(ih->handle);

  if (!value)
  {
    SendMessage(hwnd, WM_SETICON, static_cast<WPARAM>(ICON_SMALL), 0);
    SendMessage(hwnd, WM_SETICON, static_cast<WPARAM>(ICON_BIG), 0);
  }
  else
  {
    auto icon = static_cast<HICON>(iupImageGetIcon(value));
    if (icon)
    {
      SendMessage(hwnd, WM_SETICON, static_cast<WPARAM>(ICON_SMALL), reinterpret_cast<LPARAM>(icon));
      SendMessage(hwnd, WM_SETICON, static_cast<WPARAM>(ICON_BIG), reinterpret_cast<LPARAM>(icon));
    }
  }

  if (IsIconic(hwnd))
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
  else
    RedrawWindow(hwnd, nullptr, nullptr, RDW_FRAME | RDW_UPDATENOW);

  return 1;
}

extern "C" IUP_SDK_API void iupdrvDialogGetPosition(Ihandle* ih, InativeHandle* handle, int* x, int* y)
{
  RECT rect;
  if (!handle)
    handle = ih->handle;
  GetWindowRect(static_cast<HWND>(handle), &rect);
  if (x) *x = rect.left;
  if (y) *y = rect.top;
}

extern "C" IUP_SDK_API void iupdrvDialogSetPosition(Ihandle* ih, int x, int y)
{
  int flags = SWP_NOSIZE;
  if (iupAttribGetBoolean(ih, "SHOWNOACTIVATE"))
    flags |= SWP_NOACTIVATE;
  SetWindowPos(static_cast<HWND>(ih->handle), HWND_TOP, x, y, 0, 0, flags);
}

extern "C" IUP_SDK_API void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int* w, int* h)
{
  RECT rect;
  if (!handle)
    handle = ih->handle;
  GetWindowRect(static_cast<HWND>(handle), &rect);
  if (w) *w = rect.right - rect.left;
  if (h) *h = rect.bottom - rect.top;
}

extern "C" IUP_SDK_API void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  auto* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
  if (aux)
    aux->isVisible = visible ? true : false;

  if (ih->handle)
  {
    HWND hwnd = static_cast<HWND>(ih->handle);

    if (visible)
    {
      if (aux && aux->appWindow)
      {
        aux->appWindow.Show(ih->data->cmd_show != SW_SHOWNOACTIVATE);

        if ((ih->data->cmd_show != SW_SHOWNORMAL && ih->data->cmd_show != SW_SHOWNOACTIVATE) || IsIconic(hwnd) || IsZoomed(hwnd))
          ShowWindow(hwnd, ih->data->cmd_show);
      }
      else
      {
        ShowWindow(hwnd, ih->data->cmd_show);
      }

      UpdateWindow(hwnd);
    }
    else
    {
      if (aux && aux->appWindow)
        aux->appWindow.Hide();
      else
        ShowWindow(hwnd, SW_HIDE);
    }
  }
}

extern "C" IUP_SDK_API int iupdrvDialogIsVisible(Ihandle* ih)
{
  if (ih->handle)
  {
    HWND hwnd = static_cast<HWND>(ih->handle);
    return IsWindowVisible(hwnd) ? 1 : 0;
  }
  return 0;
}

extern "C" IUP_SDK_API int iupdrvDialogSetPlacement(Ihandle* ih)
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
    if (IsIconic(static_cast<HWND>(ih->handle)) || IsZoomed(static_cast<HWND>(ih->handle)))
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

    SetWindowPos(static_cast<HWND>(ih->handle), HWND_TOP, x, y, width, height, 0);

    if (IsIconic(static_cast<HWND>(ih->handle)) || IsZoomed(static_cast<HWND>(ih->handle)))
      ih->data->show_state = IUP_RESTORE;
  }

  iupAttribSet(ih, "PLACEMENT", nullptr);
  return 1;
}

extern "C" IUP_SDK_API void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* parent)
{
  if (!ih || !ih->handle)
    return;

  HWND hwnd = static_cast<HWND>(ih->handle);
  SetWindowLongPtr(hwnd, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(parent));
}

extern "C" IUP_SDK_API void iupdrvDialogGetDecoration(Ihandle* ih, int* border, int* caption, int* menu)
{
  if (ih->data->menu)
    *menu = iupdrvMenuGetMenuBarSize(ih->data->menu);
  else
    *menu = 0;

  if (ih->handle)
  {
    WINDOWINFO wi;
    wi.cbSize = sizeof(WINDOWINFO);
    GetWindowInfo(static_cast<HWND>(ih->handle), &wi);
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
        else if (iupAttribGetBoolean(ih, "TOOLBOX") && iupAttribGet(ih, "PARENTDIALOG"))
          *caption = GetSystemMetrics(SM_CYSMCAPTION);
        else
          *caption = GetSystemMetrics(SM_CYCAPTION);
      }
      padded_border = GetSystemMetrics(SM_CXPADDEDBORDER);
    }

    *border = 0;
    if (iupAttribGetBoolean(ih, "RESIZE"))
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
      HWND hwnd = static_cast<HWND>(ih->handle);
      BOOL visible = ShowWindow(hwnd, SW_HIDE);

      off_style = WS_BORDER | WS_THICKFRAME | WS_CAPTION |
                  WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;
      new_style = GetWindowLong(hwnd, GWL_STYLE);
      iupAttribSet(ih, "_IUPWINUI_FS_STYLE", reinterpret_cast<char*>(static_cast<intptr_t>(new_style)));
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
      IupSetAttribute(ih, "TITLE", nullptr);
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
    LONG style = static_cast<LONG>(reinterpret_cast<intptr_t>(iupAttribGet(ih, "_IUPWINUI_FS_STYLE")));
    if (style)
    {
      HWND hwnd = static_cast<HWND>(ih->handle);
      BOOL visible = ShowWindow(hwnd, SW_HIDE);

      iupAttribSetStr(ih, "MAXBOX", iupAttribGet(ih, "_IUPWINUI_FS_MAXBOX"));
      iupAttribSetStr(ih, "MINBOX", iupAttribGet(ih, "_IUPWINUI_FS_MINBOX"));
      iupAttribSetStr(ih, "MENUBOX", iupAttribGet(ih, "_IUPWINUI_FS_MENUBOX"));
      IupSetStrAttribute(ih, "TITLE", iupAttribGet(ih, "_IUPWINUI_FS_TITLE"));
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

      iupAttribSet(ih, "_IUPWINUI_FS_STYLE", nullptr);
      iupAttribSet(ih, "_IUPWINUI_FS_MAXBOX", nullptr);
      iupAttribSet(ih, "_IUPWINUI_FS_MINBOX", nullptr);
      iupAttribSet(ih, "_IUPWINUI_FS_MENUBOX", nullptr);
      iupAttribSet(ih, "_IUPWINUI_FS_TITLE", nullptr);
      iupAttribSet(ih, "_IUPWINUI_FS_RESIZE", nullptr);
      iupAttribSet(ih, "_IUPWINUI_FS_BORDER", nullptr);
      iupAttribSet(ih, "_IUPWINUI_FS_X", nullptr);
      iupAttribSet(ih, "_IUPWINUI_FS_Y", nullptr);
      iupAttribSet(ih, "_IUPWINUI_FS_SIZE", nullptr);
    }
  }
  return 1;
}

static int winuiDialogSetOpacityAttrib(Ihandle* ih, const char* value)
{
  int opacity;
  if (!iupStrToInt(value, &opacity))
    return 0;

  HWND hwnd = static_cast<HWND>(ih->handle);
  LONG exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
  if (!(exstyle & WS_EX_LAYERED))
    SetWindowLong(hwnd, GWL_EXSTYLE, exstyle | WS_EX_LAYERED);

  SetLayeredWindowAttributes(hwnd, 0, static_cast<BYTE>(opacity), LWA_ALPHA);
  RedrawWindow(hwnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
  return 1;
}

static WriteableBitmap winuiDialogCreateTiledBitmap(WriteableBitmap src, int dst_w, int dst_h)
{
  int src_w = src.PixelWidth();
  int src_h = src.PixelHeight();
  if (src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0)
    return nullptr;

  WriteableBitmap dst(dst_w, dst_h);
  uint8_t* src_data = src.PixelBuffer().data();
  uint8_t* dst_data = dst.PixelBuffer().data();
  int src_stride = src_w * 4;
  int dst_stride = dst_w * 4;

  for (int y = 0; y < dst_h; y++)
  {
    int sy = y % src_h;
    uint8_t* src_row = src_data + sy * src_stride;
    uint8_t* dst_row = dst_data + y * dst_stride;
    int x = 0;
    while (x < dst_w)
    {
      int copy_w = src_w;
      if (x + copy_w > dst_w)
        copy_w = dst_w - x;
      memcpy(dst_row + x * 4, src_row, copy_w * 4);
      x += copy_w;
    }
  }

  dst.Invalidate();
  return dst;
}

static int winuiDialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  if (winuiDialogSetBgColorAttrib(ih, value))
  {
    winuiDialogRefreshThemeColors(ih);
    return 1;
  }

  void* handle = iupImageGetImage(value, ih, 0, nullptr);
  if (handle)
  {
    WriteableBitmap bitmap = winuiGetBitmapFromHandle(handle);
    if (bitmap)
    {
      iupAttribSet(ih, "_IUPWINUI_BACKGROUND_COLOR", nullptr);

      if (ih->handle)
      {
        auto* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
        if (aux && aux->rootPanel && !iupAttribGet(ih, "_IUPWINUI_BACKDROP_ACTIVE"))
        {
          ImageBrush brush;
          if (iupAttribGetBoolean(ih, "BACKIMAGEZOOM"))
          {
            brush.ImageSource(bitmap);
            brush.Stretch(Stretch::Fill);
          }
          else
          {
            WriteableBitmap tiled = winuiDialogCreateTiledBitmap(bitmap, ih->currentwidth, ih->currentheight);
            brush.ImageSource(tiled ? tiled : bitmap);
            brush.Stretch(Stretch::Fill);
          }
          aux->rootPanel.Background(brush);
        }
        winuiDialogRefreshThemeColors(ih);
        RedrawWindow(static_cast<HWND>(ih->handle), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
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
    SetWindowRgn(static_cast<HWND>(ih->handle), nullptr, TRUE);
    return 0;
  }

  auto* imgdata = reinterpret_cast<unsigned char*>(iupAttribGet(image, "WID"));
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

  SetWindowRgn(static_cast<HWND>(ih->handle), hRgn, TRUE);

  return 1;
}

static ITaskbarList3* winuiDialogEnsureTaskBar(Ihandle* ih)
{
  auto* tbl = reinterpret_cast<ITaskbarList3*>(iupAttribGet(ih, "_IUPWINUI_TASKBARLIST"));
  if (!tbl && ih->handle)
  {
    CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&tbl));
    if (tbl)
      iupAttribSet(ih, "_IUPWINUI_TASKBARLIST", reinterpret_cast<char*>(tbl));
  }
  return tbl;
}

static int winuiDialogSetTaskBarProgressAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    winuiDialogEnsureTaskBar(ih);
  else
  {
    auto* tbl = reinterpret_cast<ITaskbarList3*>(iupAttribGet(ih, "_IUPWINUI_TASKBARLIST"));
    if (tbl && ih->handle)
      tbl->SetProgressState(static_cast<HWND>(ih->handle), TBPF_NOPROGRESS);
  }
  return 1;
}

static int winuiDialogSetTaskBarProgressValueAttrib(Ihandle* ih, const char* value)
{
  ITaskbarList3* tbl = winuiDialogEnsureTaskBar(ih);
  if (tbl)
  {
    int perc;
    iupStrToInt(value, &perc);
    tbl->SetProgressValue(static_cast<HWND>(ih->handle), perc, 100);

    if (perc == 100)
      tbl->SetProgressState(static_cast<HWND>(ih->handle), TBPF_NOPROGRESS);
  }

  return 0;
}

static int winuiDialogSetTaskBarProgressStateAttrib(Ihandle* ih, const char* value)
{
  ITaskbarList3* tbl = winuiDialogEnsureTaskBar(ih);
  if (tbl)
  {
    if (iupStrEqualNoCase(value, "NOPROGRESS"))
      tbl->SetProgressState(static_cast<HWND>(ih->handle), TBPF_NOPROGRESS);
    else if (iupStrEqualNoCase(value, "INDETERMINATE"))
      tbl->SetProgressState(static_cast<HWND>(ih->handle), TBPF_INDETERMINATE);
    else if (iupStrEqualNoCase(value, "ERROR"))
      tbl->SetProgressState(static_cast<HWND>(ih->handle), TBPF_ERROR);
    else if (iupStrEqualNoCase(value, "PAUSED"))
      tbl->SetProgressState(static_cast<HWND>(ih->handle), TBPF_PAUSED);
    else
      tbl->SetProgressState(static_cast<HWND>(ih->handle), TBPF_NORMAL);
  }

  return 0;
}

void winuiDialogSetMenuBar(Ihandle* ih, MenuBar menuBar)
{
  auto* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
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

static int winuiDialogSetBackImageZoomAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  char* background = iupAttribGet(ih, "BACKGROUND");
  if (background)
    winuiDialogSetBackgroundAttrib(ih, background);
  return 1;
}

extern "C" IUP_SDK_API void iupdrvDialogInitClass(Iclass* ic)
{
  ic->Map = winuiDialogMapMethod;
  ic->UnMap = winuiDialogUnMapMethod;
  ic->LayoutUpdate = winuiDialogLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = winuiDialogGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, winuiDialogSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "TITLE", winuiDialogGetTitleAttrib, winuiDialogSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CLIENTSIZE", winuiDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", winuiDialogGetClientOffsetAttrib, nullptr, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MINSIZE", nullptr, iupBaseSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXSIZE", nullptr, iupBaseSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESIZEINC", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ACTIVEWINDOW", winuiDialogGetActiveWindowAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPMOST", nullptr, winuiDialogSetTopMostAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BRINGFRONT", nullptr, winuiDialogSetBringFrontAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MAXIMIZED", winuiDialogGetMaximizedAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINIMIZED", winuiDialogGetMinimizedAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ICON", nullptr, winuiDialogSetIconAttrib, nullptr, nullptr, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HWND", iupBaseGetWidAttrib, nullptr, nullptr, nullptr, IUPAF_NO_STRING | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CUSTOMFRAME", nullptr, nullptr, IUPAF_SAMEASSYSTEM, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDETITLEBAR", nullptr, winuiDialogSetHideTitleBarAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUSTOMFRAMECAPTIONHEIGHT", nullptr, nullptr, IUPAF_SAMEASSYSTEM, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FULLSCREEN", nullptr, winuiDialogSetFullScreenAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "OPACITY", nullptr, winuiDialogSetOpacityAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LAYERALPHA", nullptr, winuiDialogSetOpacityAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKGROUND", nullptr, winuiDialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGEZOOM", nullptr, winuiDialogSetBackImageZoomAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHAPEIMAGE", nullptr, winuiDialogSetShapeImageAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKDROP", nullptr, winuiDialogSetBackdropAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TASKBARPROGRESS", nullptr, winuiDialogSetTaskBarProgressAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TASKBARPROGRESSSTATE", nullptr, winuiDialogSetTaskBarProgressStateAttrib, IUPAF_SAMEASSYSTEM, "NORMAL", IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TASKBARPROGRESSVALUE", nullptr, winuiDialogSetTaskBarProgressValueAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHOWNOACTIVATE", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWMINIMIZENEXT", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "OPACITYIMAGE", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMPOSITED", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTROL", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HELPBUTTON", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOOLBOX", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIALOGHINT", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEUNDER", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
