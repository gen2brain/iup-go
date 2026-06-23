/** \file
 * \brief Windows Dark Mode support (undocumented uxtheme API)
 *
 * See Copyright Notice in "iup.h"
 */
#undef NOTREEVIEW  /* the build defines it; the tree view messages are needed here */
#include <windows.h>
#include <uxtheme.h>
#include <commctrl.h>

#include "iup.h"
#include "iup_str.h"

#include "iupwin_info.h"
#include "iupwin_brush.h"
#include "iupwin_darkmode.h"

/* Undocumented menu bar drawing messages. */
#define IUPWIN_UAHDRAWMENU      0x0091
#define IUPWIN_UAHDRAWMENUITEM  0x0092

/* VSCLASS_MENU parts/states missing from the mingw headers. */
#define IUPWIN_MENU_BARITEM     8
#define IUPWIN_MBI_NORMAL       1
#define IUPWIN_MBI_HOT          2
#define IUPWIN_MBI_PUSHED       3
#define IUPWIN_MBI_DISABLED     4
#define IUPWIN_MBI_DISABLEDHOT  5

/* VSCLASS_TASKDIALOG parts + theme property ids, missing from the mingw headers. */
#define IUPWIN_TDLG_PRIMARYPANEL        1
#define IUPWIN_TDLG_MAININSTRUCTIONPANE 2
#define IUPWIN_TDLG_CONTENTPANE         4
#define IUPWIN_TDLG_EXPANDOTEXT         6
#define IUPWIN_TDLG_EXPANDEDFOOTERAREA  8
#define IUPWIN_TDLG_VERIFICATIONTEXT    14
#define IUPWIN_TDLG_FOOTNOTEPANE        15
#define IUPWIN_TDLG_SECONDARYPANEL      18
#define IUPWIN_TMT_FILLCOLOR            3802
#define IUPWIN_TMT_TEXTCOLOR            3803

typedef struct _UAHMENU
{
  HMENU hmenu;
  HDC hdc;
  DWORD dwFlags;
} UAHMENU;

typedef struct _UAHDRAWMENUITEM
{
  DRAWITEMSTRUCT dis;
  UAHMENU um;
  int iPosition;  /* first field of the real umi; the trailing metrics are unused */
} UAHDRAWMENUITEM;


enum WinPreferredAppMode
{
  WIN_APPMODE_DEFAULT,
  WIN_APPMODE_ALLOWDARK,
};

typedef enum WinPreferredAppMode (WINAPI *winSetPreferredAppMode)(enum WinPreferredAppMode appmode);  /* ordinal 135 */
typedef BOOL (WINAPI *winAllowDarkModeForWindow)(HWND hwnd, BOOL allow);                              /* ordinal 133 */
typedef void (WINAPI *winRefreshImmersiveColorPolicyState)(void);                                    /* ordinal 104 */
typedef void (WINAPI *winFlushMenuThemes)(void);                                                     /* ordinal 136 */

typedef HRESULT (STDAPICALLTYPE *winSetWindowTheme)(HWND hwnd, LPCWSTR sub, LPCWSTR id);
typedef HTHEME (STDAPICALLTYPE *winOpenThemeData)(HWND hwnd, LPCWSTR classlist);
typedef HRESULT (STDAPICALLTYPE *winCloseThemeData)(HTHEME htheme);
typedef HRESULT (STDAPICALLTYPE *winDrawThemeTextEx)(HTHEME, HDC, int, int, LPCWSTR, int, DWORD, RECT*, const DTTOPTS*);

static winSetPreferredAppMode              my_SetPreferredAppMode = NULL;
static winAllowDarkModeForWindow           my_AllowDarkModeForWindow = NULL;
static winRefreshImmersiveColorPolicyState my_RefreshImmersiveColorPolicyState = NULL;
static winFlushMenuThemes                  my_FlushMenuThemes = NULL;
static winSetWindowTheme                   my_SetWindowTheme = NULL;
static winOpenThemeData                    my_OpenThemeData = NULL;
static winCloseThemeData                   my_CloseThemeData = NULL;
static winDrawThemeTextEx                  my_DrawThemeTextEx = NULL;

static int win_darkmode_supported = 0;

static void winDarkModeMenuFree(void);
static COLORREF winDarkModeColor(const char* name, int r, int g, int b);

static int winIsHighContrast(void)
{
  HIGHCONTRASTA hc;
  hc.cbSize = sizeof(HIGHCONTRASTA);
  if (SystemParametersInfoA(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRASTA), &hc, FALSE))
    return (hc.dwFlags & HCF_HIGHCONTRASTON) ? 1 : 0;
  return 0;
}

typedef struct _IUPWIN_DTBGOPTS { DWORD dwSize; DWORD dwFlags; RECT rcClip; } IUPWIN_DTBGOPTS;

#define IUPWIN_BP_PUSHBUTTON 1

typedef HTHEME  (WINAPI *winOpenNcThemeData)(HWND hwnd, LPCWSTR classlist); /* uxtheme ordinal 49 */
typedef HRESULT (WINAPI *winGetThemeColor)(HTHEME, int, int, int, COLORREF*);
typedef HRESULT (WINAPI *winDrawThemeBackground)(HTHEME, HDC, int, int, const RECT*, const RECT*);
typedef HRESULT (WINAPI *winDrawThemeBackgroundEx)(HTHEME, HDC, int, int, const RECT*, const IUPWIN_DTBGOPTS*);
typedef HRESULT (WINAPI *winDrawThemeText)(HTHEME, HDC, int, int, LPCWSTR, int, DWORD, DWORD, const RECT*);

static winOpenNcThemeData       my_OpenNcThemeData = NULL;
static winGetThemeColor         my_GetThemeColor = NULL;
static winDrawThemeBackground   my_DrawThemeBackground = NULL;
static winDrawThemeBackgroundEx my_DrawThemeBackgroundEx = NULL;
static winDrawThemeText         my_DrawThemeText = NULL;

static HTHEME WINAPI winDarkModeOpenNcThemeData(HWND hwnd, LPCWSTR classlist)
{
  if (iupwinDarkModeEnabled() && lstrcmpW(classlist, L"ScrollBar") == 0)
  {
    hwnd = NULL;
    classlist = L"Explorer::ScrollBar";
  }
  return my_OpenNcThemeData(hwnd, classlist);
}

static HRESULT WINAPI winDarkModeGetThemeColor(HTHEME hTheme, int part, int state, int prop, COLORREF* color)
{
  HRESULT hr = my_GetThemeColor(hTheme, part, state, prop, color);
  if (iupwinDarkModeEnabled() && color && prop == IUPWIN_TMT_TEXTCOLOR)
  {
    switch (part)
    {
    case IUPWIN_TDLG_MAININSTRUCTIONPANE:
    case IUPWIN_TDLG_CONTENTPANE:
    case IUPWIN_TDLG_EXPANDOTEXT:
    case IUPWIN_TDLG_VERIFICATIONTEXT:
    case IUPWIN_TDLG_FOOTNOTEPANE:
    case IUPWIN_TDLG_EXPANDEDFOOTERAREA:
      *color = winDarkModeColor("DLGFGCOLOR", 255, 255, 255);
      break;
    }
  }
  return hr;
}

/* The real panel fills from the dark TaskDialog theme; falls back to the Fluent palette. */
static COLORREF winDarkModeTaskDlgFill(int secondary)
{
  static int queried = 0;
  static COLORREF primary = 0, footer = 0;
  if (!queried)
  {
    queried = 1;
    primary = winDarkModeColor("DLGBGCOLOR", 32, 32, 32);
    footer = winDarkModeColor("MENUBGCOLOR", 24, 24, 24);
    if (my_OpenThemeData && my_GetThemeColor)
    {
      HTHEME t = my_OpenThemeData(NULL, L"DarkMode_Explorer::TaskDialog");
      if (t)
      {
        COLORREF c;
        if (SUCCEEDED(my_GetThemeColor(t, IUPWIN_TDLG_PRIMARYPANEL, 0, IUPWIN_TMT_FILLCOLOR, &c)))   primary = c;
        if (SUCCEEDED(my_GetThemeColor(t, IUPWIN_TDLG_SECONDARYPANEL, 0, IUPWIN_TMT_FILLCOLOR, &c)))  footer = c;
        my_CloseThemeData(t);
      }
    }
  }
  return secondary? footer: primary;
}

static int winDarkModeFillTaskDlgPanel(HDC hdc, int part, const RECT* prc, const RECT* clip)
{
  if (!iupwinDarkModeEnabled())
    return 0;
  if (part == IUPWIN_TDLG_PRIMARYPANEL)
  {
    FillRect(hdc, prc, iupwinBrushGet(winDarkModeTaskDlgFill(0)));
    return 1;
  }
  if (part == IUPWIN_TDLG_SECONDARYPANEL || part == IUPWIN_TDLG_FOOTNOTEPANE || part == IUPWIN_TDLG_EXPANDEDFOOTERAREA)
  {
    const RECT* r = (clip && clip->right > clip->left)? clip: prc;
    FillRect(hdc, r, iupwinBrushGet(winDarkModeTaskDlgFill(1)));
    return 1;
  }
  return 0;
}

static HTHEME winDarkModeButtonTheme(void)
{
  return my_OpenThemeData? my_OpenThemeData(NULL, L"DarkMode_Explorer::Button"): NULL;
}

static HRESULT WINAPI winDarkModeDrawThemeBackgroundEx(HTHEME hTheme, HDC hdc, int part, int state, const RECT* prc, const IUPWIN_DTBGOPTS* opts)
{
  if (winDarkModeFillTaskDlgPanel(hdc, part, prc, opts? &opts->rcClip: NULL))
    return S_OK;
  return my_DrawThemeBackgroundEx(hTheme, hdc, part, state, prc, opts);
}

static HRESULT WINAPI winDarkModeDrawThemeBackground(HTHEME hTheme, HDC hdc, int part, int state, const RECT* prc, const RECT* clip)
{
  if (iupwinDarkModeEnabled() && part == IUPWIN_BP_PUSHBUTTON)  /* non-Ex part 1 is the button, not a panel */
  {
    HTHEME dark = winDarkModeButtonTheme();
    if (dark)
    {
      HRESULT hr = my_DrawThemeBackground(dark, hdc, part, state, prc, clip);
      my_CloseThemeData(dark);
      return hr;
    }
  }
  if (part != IUPWIN_TDLG_PRIMARYPANEL && winDarkModeFillTaskDlgPanel(hdc, part, prc, clip))
    return S_OK;
  return my_DrawThemeBackground(hTheme, hdc, part, state, prc, clip);
}

static HRESULT WINAPI winDarkModeDrawThemeText(HTHEME hTheme, HDC hdc, int part, int state, LPCWSTR text, int len, DWORD flags, DWORD flags2, const RECT* prc)
{
  if (iupwinDarkModeEnabled() && part == IUPWIN_BP_PUSHBUTTON && my_DrawThemeTextEx)
  {
    HTHEME dark = winDarkModeButtonTheme();
    if (dark)
    {
      DTTOPTS o;
      RECT r = *prc;
      HRESULT hr;
      ZeroMemory(&o, sizeof(o));
      o.dwSize = sizeof(o);
      o.dwFlags = DTT_TEXTCOLOR;
      o.crText = winDarkModeColor("DLGFGCOLOR", 255, 255, 255);
      hr = my_DrawThemeTextEx(dark, hdc, part, state, text, len, flags, &r, &o);
      my_CloseThemeData(dark);
      return hr;
    }
  }
  return my_DrawThemeText(hTheme, hdc, part, state, text, len, flags, flags2, prc);
}

static DWORD WINAPI winDarkModeGetSysColor(int index)
{
  if (iupwinDarkModeEnabled())
  {
    switch (index)
    {
    case COLOR_WINDOW:     return winDarkModeColor("TXTBGCOLOR", 45, 45, 45);
    case COLOR_WINDOWTEXT: return winDarkModeColor("TXTFGCOLOR", 255, 255, 255);
    case COLOR_BTNFACE:    return winDarkModeColor("MENUBGCOLOR", 24, 24, 24);
    case COLOR_BTNTEXT:    return winDarkModeColor("DLGFGCOLOR", 255, 255, 255);
    }
  }
  return GetSysColor(index);
}

static PIMAGE_THUNK_DATA winDarkModeFindStaticImport(HMODULE hMod, LPCSTR dll, LPCSTR func)
{
  PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)hMod;
  PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((BYTE*)hMod + dos->e_lfanew);
  IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
  PIMAGE_IMPORT_DESCRIPTOR imp;

  if (!dir.VirtualAddress)
    return NULL;

  imp = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)hMod + dir.VirtualAddress);
  for (; imp->Name; ++imp)
  {
    PIMAGE_THUNK_DATA name, addr;
    if (lstrcmpiA((LPCSTR)((BYTE*)hMod + imp->Name), dll) != 0)
      continue;

    name = (PIMAGE_THUNK_DATA)((BYTE*)hMod + (imp->OriginalFirstThunk? imp->OriginalFirstThunk: imp->FirstThunk));
    addr = (PIMAGE_THUNK_DATA)((BYTE*)hMod + imp->FirstThunk);
    for (; name->u1.Ordinal; ++name, ++addr)
    {
      if (!IMAGE_SNAP_BY_ORDINAL(name->u1.Ordinal))
      {
        PIMAGE_IMPORT_BY_NAME ibn = (PIMAGE_IMPORT_BY_NAME)((BYTE*)hMod + name->u1.AddressOfData);
        if (lstrcmpA((LPCSTR)ibn->Name, func) == 0)
          return addr;
      }
    }
  }
  return NULL;
}

/* Match by name, or by ordinal when func_name is NULL. */
static PIMAGE_THUNK_DATA winDarkModeFindComctlThunk(HMODULE hComctl, LPCSTR func_name, WORD ordinal)
{
  PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)hComctl;
  PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((BYTE*)hComctl + dos->e_lfanew);
  IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
  PIMAGE_DELAYLOAD_DESCRIPTOR imp;

  if (!dir.VirtualAddress)
    return NULL;

  imp = (PIMAGE_DELAYLOAD_DESCRIPTOR)((BYTE*)hComctl + dir.VirtualAddress);
  for (; imp->DllNameRVA; ++imp)
  {
    PIMAGE_THUNK_DATA name, addr;
    if (lstrcmpiA((LPCSTR)((BYTE*)hComctl + imp->DllNameRVA), "uxtheme.dll") != 0)
      continue;

    name = (PIMAGE_THUNK_DATA)((BYTE*)hComctl + imp->ImportNameTableRVA);
    addr = (PIMAGE_THUNK_DATA)((BYTE*)hComctl + imp->ImportAddressTableRVA);
    for (; name->u1.Ordinal; ++name, ++addr)
    {
      if (IMAGE_SNAP_BY_ORDINAL(name->u1.Ordinal))
      {
        if (func_name == NULL && IMAGE_ORDINAL(name->u1.Ordinal) == ordinal)
          return addr;
      }
      else if (func_name)
      {
        PIMAGE_IMPORT_BY_NAME ibn = (PIMAGE_IMPORT_BY_NAME)((BYTE*)hComctl + name->u1.AddressOfData);
        if (lstrcmpA((LPCSTR)ibn->Name, func_name) == 0)
          return addr;
      }
    }
  }
  return NULL;
}

static void winDarkModePatchThunk(PIMAGE_THUNK_DATA thunk, ULONG_PTR value)
{
  DWORD old_protect;
  if (thunk && VirtualProtect(thunk, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &old_protect))
  {
    thunk->u1.Function = value;
    VirtualProtect(thunk, sizeof(IMAGE_THUNK_DATA), old_protect, &old_protect);
  }
}

/* Redirect comctl32's OpenNcThemeData (ord 49) so scroll bars use the dark Explorer theme. */
static void winDarkModeFixScrollBars(void)
{
  HMODULE hUxtheme, hComctl;

  hUxtheme = GetModuleHandle(TEXT("uxtheme.dll"));
  if (hUxtheme)
  {
    my_OpenNcThemeData = (winOpenNcThemeData)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(49));
    my_GetThemeColor = (winGetThemeColor)GetProcAddress(hUxtheme, "GetThemeColor");
    my_DrawThemeBackground = (winDrawThemeBackground)GetProcAddress(hUxtheme, "DrawThemeBackground");
    my_DrawThemeBackgroundEx = (winDrawThemeBackgroundEx)GetProcAddress(hUxtheme, "DrawThemeBackgroundEx");
    my_DrawThemeText = (winDrawThemeText)GetProcAddress(hUxtheme, "DrawThemeText");
  }
  if (!my_OpenNcThemeData)
    return;

  hComctl = LoadLibraryEx(TEXT("comctl32.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (!hComctl)
    return;

  winDarkModePatchThunk(winDarkModeFindComctlThunk(hComctl, NULL, 49), (ULONG_PTR)winDarkModeOpenNcThemeData);
}

/* Scoped to the modal popup: the TDLG_* part ids collide with other theme classes. */
IUP_DRV_API void iupwinDarkModeHookTaskDialog(int enable)
{
  static PIMAGE_THUNK_DATA thunk_color = NULL, thunk_bg = NULL, thunk_bgex = NULL, thunk_sys = NULL, thunk_text = NULL;
  static ULONG_PTR orig_color = 0, orig_bg = 0, orig_bgex = 0, orig_sys = 0, orig_text = 0;
  HMODULE hComctl;

  if (!win_darkmode_supported || !my_GetThemeColor || !my_DrawThemeBackground || !my_DrawThemeBackgroundEx)
    return;

  if (enable)
  {
    hComctl = GetModuleHandle(TEXT("comctl32.dll"));
    if (!hComctl)
      return;
    thunk_color = winDarkModeFindComctlThunk(hComctl, "GetThemeColor", 0);
    thunk_bg = winDarkModeFindComctlThunk(hComctl, "DrawThemeBackground", 0);
    thunk_bgex = winDarkModeFindComctlThunk(hComctl, "DrawThemeBackgroundEx", 0);
    if (!thunk_bgex)
      thunk_bgex = winDarkModeFindComctlThunk(hComctl, NULL, 47);
    thunk_text = winDarkModeFindComctlThunk(hComctl, "DrawThemeText", 0);
    thunk_sys = winDarkModeFindStaticImport(hComctl, "user32.dll", "GetSysColor");
    if (thunk_color)
    {
      orig_color = thunk_color->u1.Function;
      winDarkModePatchThunk(thunk_color, (ULONG_PTR)winDarkModeGetThemeColor);
    }
    if (thunk_bg)
    {
      orig_bg = thunk_bg->u1.Function;
      winDarkModePatchThunk(thunk_bg, (ULONG_PTR)winDarkModeDrawThemeBackground);
    }
    if (thunk_bgex)
    {
      orig_bgex = thunk_bgex->u1.Function;
      winDarkModePatchThunk(thunk_bgex, (ULONG_PTR)winDarkModeDrawThemeBackgroundEx);
    }
    if (thunk_text && my_DrawThemeText)
    {
      orig_text = thunk_text->u1.Function;
      winDarkModePatchThunk(thunk_text, (ULONG_PTR)winDarkModeDrawThemeText);
    }
    if (thunk_sys)
    {
      orig_sys = thunk_sys->u1.Function;
      winDarkModePatchThunk(thunk_sys, (ULONG_PTR)winDarkModeGetSysColor);
    }
  }
  else
  {
    if (thunk_color && orig_color)
      winDarkModePatchThunk(thunk_color, orig_color);
    if (thunk_bg && orig_bg)
      winDarkModePatchThunk(thunk_bg, orig_bg);
    if (thunk_bgex && orig_bgex)
      winDarkModePatchThunk(thunk_bgex, orig_bgex);
    if (thunk_text && orig_text)
      winDarkModePatchThunk(thunk_text, orig_text);
    if (thunk_sys && orig_sys)
      winDarkModePatchThunk(thunk_sys, orig_sys);
    thunk_color = thunk_bg = thunk_bgex = thunk_sys = thunk_text = NULL;
    orig_color = orig_bg = orig_bgex = orig_sys = orig_text = 0;
  }
}

IUP_DRV_API void iupwinDarkModeInit(void)
{
  static int initialized = 0;
  HMODULE hUxtheme;

  if (initialized)
    return;
  initialized = 1;

  if (!iupwinIsWin10OrNew())
    return;

  hUxtheme = LoadLibraryEx(TEXT("uxtheme.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (hUxtheme)
  {
    my_SetPreferredAppMode = (winSetPreferredAppMode)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
    my_AllowDarkModeForWindow = (winAllowDarkModeForWindow)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133));
    my_RefreshImmersiveColorPolicyState = (winRefreshImmersiveColorPolicyState)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104));
    my_FlushMenuThemes = (winFlushMenuThemes)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136));
    my_SetWindowTheme = (winSetWindowTheme)GetProcAddress(hUxtheme, "SetWindowTheme");
    my_OpenThemeData = (winOpenThemeData)GetProcAddress(hUxtheme, "OpenThemeData");
    my_CloseThemeData = (winCloseThemeData)GetProcAddress(hUxtheme, "CloseThemeData");
    my_DrawThemeTextEx = (winDrawThemeTextEx)GetProcAddress(hUxtheme, "DrawThemeTextEx");

    if (my_SetPreferredAppMode && my_AllowDarkModeForWindow && my_RefreshImmersiveColorPolicyState && my_FlushMenuThemes && my_SetWindowTheme)
      win_darkmode_supported = 1;
  }

  if (win_darkmode_supported)
    winDarkModeFixScrollBars();
}

static int win_darkmode_optin = 0;

IUP_DRV_API void iupwinDarkModeSetOptIn(int enable)
{
  win_darkmode_optin = enable? 1: 0;
  if (win_darkmode_supported)
  {
    my_SetPreferredAppMode(win_darkmode_optin? WIN_APPMODE_ALLOWDARK: WIN_APPMODE_DEFAULT);
    my_RefreshImmersiveColorPolicyState();
    my_FlushMenuThemes();
  }
}

IUP_DRV_API int iupwinDarkModeOptIn(void)
{
  return win_darkmode_optin;
}

IUP_DRV_API int iupwinDarkModeEnabled(void)
{
  return (win_darkmode_optin && iupwinIsSystemDarkMode() && !winIsHighContrast()) ? 1 : 0;
}

IUP_DRV_API void iupwinDarkModeRefresh(void)
{
  if (!win_darkmode_supported)
    return;
  my_RefreshImmersiveColorPolicyState();
  my_FlushMenuThemes();
  winDarkModeMenuFree();
}

static void winDarkModeTheme(HWND hwnd, LPCWSTR dark_theme, int dark)
{
  if (win_darkmode_supported)
    my_SetWindowTheme(hwnd, dark ? dark_theme : NULL, NULL);
}

IUP_DRV_API void iupwinDarkModeApplyToWindow(HWND hwnd)
{
  WCHAR cls[64];
  int dark;

  if (!hwnd)
    return;

  dark = iupwinDarkModeEnabled();
  if (win_darkmode_supported)
    my_AllowDarkModeForWindow(hwnd, dark ? TRUE : FALSE);

  if (GetClassNameW(hwnd, cls, 64) == 0)
    return;

  if (lstrcmpiW(cls, L"SysListView32") == 0 || lstrcmpiW(cls, L"SysTreeView32") == 0)
  {
    int treeview = lstrcmpiW(cls, L"SysTreeView32") == 0;
    if (!treeview && GetPropA(hwnd, "_IUP_NOHOVER"))
    {
      if (win_darkmode_supported)
        my_SetWindowTheme(hwnd, NULL, NULL);
    }
    else
      winDarkModeTheme(hwnd, L"DarkMode_Explorer", dark);

    if (!treeview)
    {
      HWND header = (HWND)SendMessage(hwnd, LVM_GETHEADER, 0, 0);
      if (header)
        iupwinDarkModeApplyToWindow(header);
    }
  }
  else if (lstrcmpiW(cls, L"Button") == 0 || lstrcmpiW(cls, L"ListBox") == 0)
    winDarkModeTheme(hwnd, L"DarkMode_Explorer", dark);
  else if (lstrcmpiW(cls, L"Edit") == 0)
    winDarkModeTheme(hwnd, L"DarkMode_CFD", dark);
  else if (lstrcmpiW(cls, L"ComboBox") == 0)
  {
    COMBOBOXINFO cbi;
    winDarkModeTheme(hwnd, L"DarkMode_CFD", dark);
    ZeroMemory(&cbi, sizeof(cbi));
    cbi.cbSize = sizeof(cbi);
    if (GetComboBoxInfo(hwnd, &cbi))  /* edit + drop-down list are not reached by EnumChildWindows */
    {
      if (cbi.hwndItem)
        winDarkModeTheme(cbi.hwndItem, L"DarkMode_CFD", dark);
      if (cbi.hwndList)
      {
        if (win_darkmode_supported)
          my_AllowDarkModeForWindow(cbi.hwndList, dark? TRUE: FALSE);
        winDarkModeTheme(cbi.hwndList, L"DarkMode_Explorer", dark);
      }
    }
  }
  else if (lstrcmpiW(cls, L"SysHeader32") == 0)
    winDarkModeTheme(hwnd, L"ItemsView", dark);
}

static BOOL CALLBACK winDarkModeEnumChild(HWND hwnd, LPARAM lp)
{
  (void)lp;
  iupwinDarkModeApplyToWindow(hwnd);
  return TRUE;
}

IUP_DRV_API void iupwinDarkModeApplyToTree(HWND root)
{
  if (!root)
    return;
  iupwinDarkModeApplyToWindow(root);
  EnumChildWindows(root, winDarkModeEnumChild, 0);
  RedrawWindow(root, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

/* Grid list views (IupTable) keep the default theme (no Explorer row hover) in both modes. */
IUP_DRV_API void iupwinDarkModeSetNoHover(HWND hwnd)
{
  if (!win_darkmode_supported || !hwnd)
    return;
  SetPropA(hwnd, "_IUP_NOHOVER", (HANDLE)1);
  my_AllowDarkModeForWindow(hwnd, iupwinDarkModeEnabled()? TRUE: FALSE);
  my_SetWindowTheme(hwnd, NULL, NULL);
}

static LRESULT CALLBACK winDarkModeTaskDlgSubclass(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

static BOOL CALLBACK winDarkModeEnumTaskDlg(HWND hwnd, LPARAM lp)
{
  WCHAR cls[64];
  (void)lp;
  my_AllowDarkModeForWindow(hwnd, TRUE);
  if (GetClassNameW(hwnd, cls, 64))
  {
    if (lstrcmpiW(cls, L"Button") == 0)
      my_SetWindowTheme(hwnd, L"DarkMode_Explorer", NULL);
    else
      SetWindowSubclass(hwnd, winDarkModeTaskDlgSubclass, 1, 0);
  }
  return TRUE;
}

/* The button bar is the dialog's own COLOR_BTNFACE fill, reachable only by subclassing the dialog. */
static LRESULT CALLBACK winDarkModeTaskDlgSubclass(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR ref)
{
  (void)ref;
  switch (msg)
  {
  case WM_ERASEBKGND:
    {
      RECT rc;
      GetClientRect(hwnd, &rc);
      FillRect((HDC)wp, &rc, iupwinBrushGet(winDarkModeColor("DLGBGCOLOR", 32, 32, 32)));
      return 1;
    }
  case WM_CTLCOLORDLG:
  case WM_CTLCOLORSTATIC:
  case WM_CTLCOLORBTN:
    SetBkMode((HDC)wp, TRANSPARENT);
    SetTextColor((HDC)wp, winDarkModeColor("DLGFGCOLOR", 255, 255, 255));
    SetBkColor((HDC)wp, winDarkModeColor("DLGBGCOLOR", 32, 32, 32));
    return (LRESULT)iupwinBrushGet(winDarkModeColor("DLGBGCOLOR", 32, 32, 32));
  case WM_NCDESTROY:
    RemoveWindowSubclass(hwnd, winDarkModeTaskDlgSubclass, id);
    break;
  }
  return DefSubclassProc(hwnd, msg, wp, lp);
}

IUP_DRV_API void iupwinDarkModeApplyTaskDialog(HWND root)
{
  if (!win_darkmode_supported || !root || !iupwinDarkModeEnabled())
    return;
  my_AllowDarkModeForWindow(root, TRUE);
  SetWindowSubclass(root, winDarkModeTaskDlgSubclass, 1, 0);
  EnumChildWindows(root, winDarkModeEnumTaskDlg, 0);
}

static HTHEME g_menu_theme = NULL;

static COLORREF winDarkModeColor(const char* name, int r, int g, int b)
{
  unsigned char rr, gg, bb;
  char* v = IupGetGlobal(name);
  if (v && iupStrToRGB(v, &rr, &gg, &bb))
    return RGB(rr, gg, bb);
  return RGB(r, g, b);
}

/* white at alpha over the base, how Fluent layers its dark fills: 0x19 list-low, 0x33 list-medium, 0x5D text-disabled */
static COLORREF winDarkModeFluentOverlay(int alpha)
{
  COLORREF base = winDarkModeColor("DLGBGCOLOR", 32, 32, 32);
  int r = (GetRValue(base) * (255 - alpha) + 255 * alpha) / 255;
  int g = (GetGValue(base) * (255 - alpha) + 255 * alpha) / 255;
  int b = (GetBValue(base) * (255 - alpha) + 255 * alpha) / 255;
  return RGB(r, g, b);
}

static void winDarkModeMenuFree(void)
{
  if (g_menu_theme && my_CloseThemeData)
  {
    my_CloseThemeData(g_menu_theme);
    g_menu_theme = NULL;
  }
}

static void winDarkModePaintMenuBar(HWND hwnd, HDC hdc)
{
  MENUBARINFO mbi;
  RECT rcWin, rcBar;

  mbi.cbSize = sizeof(MENUBARINFO);
  if (!GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi))
    return;

  GetWindowRect(hwnd, &rcWin);
  rcBar = mbi.rcBar;
  OffsetRect(&rcBar, -rcWin.left, -rcWin.top);
  rcBar.top -= 1;

  FillRect(hdc, &rcBar, iupwinBrushGet(winDarkModeColor("MENUBGCOLOR", 43, 43, 43)));
}

static void winDarkModePaintMenuBarItem(HWND hwnd, UAHDRAWMENUITEM* udmi)
{
  WCHAR text[256];
  MENUITEMINFOW mii;
  DTTOPTS opts;
  DWORD flags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
  int text_state = IUPWIN_MBI_NORMAL, bg_state = IUPWIN_MBI_NORMAL;

  if (!g_menu_theme && my_OpenThemeData)
    g_menu_theme = my_OpenThemeData(hwnd, L"Menu");
  if (!g_menu_theme || !my_DrawThemeTextEx)
    return;

  ZeroMemory(&mii, sizeof(mii));
  mii.cbSize = sizeof(MENUITEMINFOW);
  mii.fMask = MIIM_STRING;
  mii.dwTypeData = text;
  mii.cch = 255;
  if (!GetMenuItemInfoW(udmi->um.hmenu, (UINT)udmi->iPosition, TRUE, &mii))
    return;

  if (udmi->dis.itemState & ODS_SELECTED)
  {
    text_state = IUPWIN_MBI_PUSHED;
    bg_state = IUPWIN_MBI_PUSHED;
  }
  else if (udmi->dis.itemState & ODS_HOTLIGHT)
  {
    text_state = (udmi->dis.itemState & ODS_INACTIVE) ? IUPWIN_MBI_DISABLEDHOT : IUPWIN_MBI_HOT;
    bg_state = IUPWIN_MBI_HOT;
  }
  else if (udmi->dis.itemState & (ODS_GRAYED | ODS_DISABLED | ODS_INACTIVE))
  {
    text_state = IUPWIN_MBI_DISABLED;
    bg_state = IUPWIN_MBI_DISABLED;
  }

  if (udmi->dis.itemState & ODS_NOACCEL)
    flags |= DT_HIDEPREFIX;

  if (bg_state == IUPWIN_MBI_HOT)
    FillRect(udmi->um.hdc, &udmi->dis.rcItem, iupwinBrushGet(winDarkModeFluentOverlay(0x19)));
  else if (bg_state == IUPWIN_MBI_PUSHED)
    FillRect(udmi->um.hdc, &udmi->dis.rcItem, iupwinBrushGet(winDarkModeFluentOverlay(0x33)));
  else
    FillRect(udmi->um.hdc, &udmi->dis.rcItem, iupwinBrushGet(winDarkModeColor("MENUBGCOLOR", 43, 43, 43)));

  ZeroMemory(&opts, sizeof(opts));
  opts.dwSize = sizeof(DTTOPTS);
  opts.dwFlags = DTT_TEXTCOLOR;
  opts.crText = (text_state == IUPWIN_MBI_DISABLED || text_state == IUPWIN_MBI_DISABLEDHOT) ? winDarkModeFluentOverlay(0x5D) : winDarkModeColor("MENUFGCOLOR", 255, 255, 255);

  my_DrawThemeTextEx(g_menu_theme, udmi->um.hdc, IUPWIN_MENU_BARITEM, text_state, text, (int)mii.cch, flags, &udmi->dis.rcItem, &opts);
}

static void winDarkModeMenuBottomLine(HWND hwnd)
{
  MENUBARINFO mbi;
  RECT rcClient, rcWin, line;
  HDC hdc;

  mbi.cbSize = sizeof(MENUBARINFO);
  if (!GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi))
    return;

  GetClientRect(hwnd, &rcClient);
  MapWindowPoints(hwnd, NULL, (POINT*)&rcClient, 2);
  GetWindowRect(hwnd, &rcWin);
  OffsetRect(&rcClient, -rcWin.left, -rcWin.top);

  line = rcClient;
  line.bottom = line.top;
  line.top -= 1;

  hdc = GetWindowDC(hwnd);
  FillRect(hdc, &line, iupwinBrushGet(winDarkModeColor("MENUBGCOLOR", 43, 43, 43)));
  ReleaseDC(hwnd, hdc);
}

IUP_DRV_API int iupwinDarkModeMenuBarProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT* result)
{
  if (!win_darkmode_supported)
    return 0;

  if (msg == WM_THEMECHANGED)
  {
    winDarkModeMenuFree();
    return 0;
  }

  if (!iupwinDarkModeEnabled() || !GetMenu(hwnd))
    return 0;

  switch (msg)
  {
  case IUPWIN_UAHDRAWMENU:
    winDarkModePaintMenuBar(hwnd, ((UAHMENU*)lp)->hdc);
    *result = 0;
    return 1;
  case IUPWIN_UAHDRAWMENUITEM:
    winDarkModePaintMenuBarItem(hwnd, (UAHDRAWMENUITEM*)lp);
    *result = 0;
    return 1;
  case WM_NCACTIVATE:
  case WM_NCPAINT:
    *result = DefWindowProc(hwnd, msg, wp, lp);
    winDarkModeMenuBottomLine(hwnd);
    return 1;
  }

  return 0;
}
