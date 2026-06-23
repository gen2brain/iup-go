/** \file
 * \brief Windows Driver Core
 *
 * See Copyright Notice in "iup.h"
 */
#include <windows.h>
#include <commctrl.h>
#include <shobjidl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_globalattrib.h"
#include "iup_str.h"

#include "iupwin_drv.h"
#include "iupwin_info.h"
#include "iupwin_handle.h"
#include "iupwin_brush.h"
#include "iupwin_draw.h"
#include "iupwin_str.h"
#include "iupwin_darkmode.h"


#ifndef ICC_LINK_CLASS
#define ICC_LINK_CLASS         0x00008000
#endif

HINSTANCE iupwin_hinstance = 0;
int       iupwin_comctl32ver6 = 0;
DWORD     iupwin_mainthreadid = 0;
#ifdef USE_WINHOOKPOST
HHOOK     iupwin_threadmsghook = 0;
#endif

IUP_SDK_API void* iupdrvGetDisplay(void)
{
  return NULL;
}

IUP_DRV_API void iupwinSetInstance(HINSTANCE hInstance)
{
  iupwin_hinstance = hInstance;
}

IUP_DRV_API void iupwinShowLastError(void)
{
  DWORD error = GetLastError();
  if (error)
  {
    LPTSTR lpMsgBuf = NULL;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|
                  FORMAT_MESSAGE_FROM_SYSTEM|
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, error, 0,
                  (LPTSTR)&lpMsgBuf, /* weird but that's correct */
                  0, NULL);

    if (lpMsgBuf)
    {
      MessageBox(NULL, lpMsgBuf, TEXT("GetLastError:"), MB_OK|MB_ICONERROR);
      LocalFree(lpMsgBuf);
    }
    else
    {
      MessageBox(NULL, TEXT("Unknown Error"), TEXT("GetLastError:"), MB_OK|MB_ICONERROR);
    }
  }
}

static void winSetGlobalColor(int index, const char* name)
{
  COLORREF color = GetSysColor(index);
  iupGlobalSetDefaultColorAttrib(name, (int)GetRValue(color),
                                       (int)GetGValue(color),
                                       (int)GetBValue(color));
}

/* The user-chosen accent (HKCU\..\DWM\AccentColor, ABGR), not the COLOR_HIGHLIGHT selection shade. */
static int winGetDwmAccentColor(int* r, int* g, int* b)
{
  HKEY hKey;
  DWORD value = 0, size = sizeof(value), type = 0;
  int ok = 0;

  if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\DWM", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    return 0;
  if (RegQueryValueExA(hKey, "AccentColor", NULL, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS && type == REG_DWORD)
  {
    *r = (int)(value & 0xFF);
    *g = (int)((value >> 8) & 0xFF);
    *b = (int)((value >> 16) & 0xFF);
    ok = 1;
  }
  RegCloseKey(hKey);
  return ok;
}

IUP_DRV_API COLORREF iupwinGetAccentColor(void)
{
  unsigned char r, g, b;
  if (iupStrToRGB(IupGetGlobal("ACCENTCOLOR"), &r, &g, &b))
    return RGB(r, g, b);
  return GetSysColor(COLOR_HIGHLIGHT);
}

IUP_DRV_API void iupwinSetGlobalColors(void)
{
  if (iupwinDarkModeEnabled())  /* GetSysColor stays light in dark mode; seed the Fluent dark palette */
  {
    iupGlobalSetDefaultColorAttrib("DLGBGCOLOR", 32, 32, 32);
    iupGlobalSetDefaultColorAttrib("DLGFGCOLOR", 255, 255, 255);
    iupGlobalSetDefaultColorAttrib("TXTBGCOLOR", 45, 45, 45);
    iupGlobalSetDefaultColorAttrib("TXTFGCOLOR", 255, 255, 255);
    iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", 43, 43, 43);
    iupGlobalSetDefaultColorAttrib("MENUFGCOLOR", 255, 255, 255);
  }
  else
  {
    winSetGlobalColor(COLOR_BTNFACE, "DLGBGCOLOR");
    winSetGlobalColor(COLOR_BTNTEXT, "DLGFGCOLOR");
    winSetGlobalColor(COLOR_WINDOW, "TXTBGCOLOR");
    winSetGlobalColor(COLOR_WINDOWTEXT, "TXTFGCOLOR");
    winSetGlobalColor(COLOR_MENU, "MENUBGCOLOR");
    winSetGlobalColor(COLOR_MENUTEXT, "MENUFGCOLOR");
  }

  winSetGlobalColor(COLOR_HIGHLIGHT, "TXTHLCOLOR");
  winSetGlobalColor(COLOR_HOTLIGHT, "LINKFGCOLOR");

  {
    int r, g, b;
    if (winGetDwmAccentColor(&r, &g, &b))
      iupGlobalSetDefaultColorAttrib("ACCENTCOLOR", r, g, b);
    else
      winSetGlobalColor(COLOR_HIGHLIGHT, "ACCENTCOLOR");
  }
}

IUP_SDK_API int iupdrvOpen(int *argc, char ***argv)
{
  (void)argc; /* unused in the Windows driver */
  (void)argv;

  if (!iupwinIsWinXPOrNew()) /* older than Windows XP */
    return IUP_ERROR;

  IupSetGlobal("DRIVER",  "Win32");
  IupSetGlobal("WINDOWING", "DWM");

  if (!iupwin_hinstance)
  {
#ifdef __MINGW32__
    /* MingW fails to create windows if using a console and HINSTANCE is not from the console */
    HWND win = GetConsoleWindow();
    if (win)
      iupwin_hinstance = (HINSTANCE)GetWindowLongPtr(win, GWLP_HINSTANCE);
    else
#endif
      iupwin_hinstance = GetModuleHandle(NULL);
  }

  if (CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) == RPC_E_CHANGED_MODE)
    IupSetGlobal("_IUPWIN_COINIT_MULTITHREADED", "1");

  {
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitCtrls.dwICC = ICC_WIN95_CLASSES | ICC_LINK_CLASS | ICC_DATE_CLASSES;  /* trackbar, tooltips, updown, tab, progress */
    InitCommonControlsEx(&InitCtrls);
  }

  iupwin_comctl32ver6 = (iupwinGetComCtl32Version() >= 0x060000)? 1: 0;
  if (iupwin_comctl32ver6 && !iupwinIsAppThemed())  /* When the user selected the Windows Classic theme or visual styles not active */
    iupwin_comctl32ver6 = 0;

  iupwin_mainthreadid = GetCurrentThreadId();
#ifdef USE_WINHOOKPOST
  iupwin_threadmsghook = SetWindowsHookEx(WH_MSGFILTER, iupwinPostMessageFilterProc, NULL, iupwin_mainthreadid);
#endif
  IupSetGlobal("SYSTEMLANGUAGE", iupwinGetSystemLanguage());

  iupwinDarkModeInit();
  iupwinSetGlobalColors();

  iupwinHandleInit();
  iupwinBrushInit();
  iupwinDrawInit();
  iupwinKeyInit();

  if (iupwinIsWin7OrNew())
    iupwinTouchInit();

  return IUP_NOERROR;
}

IUP_SDK_API int iupdrvSetGlobalAppIDAttrib(const char* value)
{
  static int appid_set = 0;
  if (appid_set || !value || !value[0])
    return 0;

  IupStoreGlobal("_IUP_APPID_INTERNAL", value);
  appid_set = 1;
  return 1;
}

IUP_SDK_API int iupdrvSetGlobalAppNameAttrib(const char* value)
{
  static int appname_set = 0;
  if (appname_set || !value || !value[0])
    return 0;

  WCHAR wappname[129];
  MultiByteToWideChar(CP_UTF8, 0, value, -1, wappname, 129);

  HRESULT hr = SetCurrentProcessExplicitAppUserModelID(wappname);
  if (SUCCEEDED(hr))
  {
    appname_set = 1;
    return 1;
  }
  return 0;
}

IUP_SDK_API void iupdrvClose(void)
{
  iupwinHandleFinish();
  iupwinBrushFinish();
  iupwinStrRelease();
  iupwinDrawFinish();

  if (IupGetGlobal("_IUPWIN_OLEINITIALIZE"))
    OleUninitialize();

  CoUninitialize();
}
