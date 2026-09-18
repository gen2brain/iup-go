/** \file
 * \brief Windows System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

#include "iup.h"

#include "iupwin_info.h"


typedef LONG (WINAPI* PFN_RtlGetVersion)(OSVERSIONINFOW*);

static void iupwinGetVersionInfo(OSVERSIONINFOW* osvi)
{
  HMODULE ntdll = GetModuleHandleA("ntdll.dll");
  PFN_RtlGetVersion pRtlGetVersion = (PFN_RtlGetVersion)GetProcAddress(ntdll, "RtlGetVersion");
  ZeroMemory(osvi, sizeof(OSVERSIONINFOW));
  osvi->dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
  pRtlGetVersion(osvi);
}

IUP_DRV_API int iupwinCheckWindowsVersion(DWORD major, DWORD minor)
{
  OSVERSIONINFOW osvi;
  iupwinGetVersionInfo(&osvi);

  if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
      (osvi.dwMajorVersion > major || (osvi.dwMajorVersion == major && osvi.dwMinorVersion >= minor)))
    return 1;

  return 0;
}

IUP_DRV_API DWORD iupwinGetBuildNumber(void)
{
  OSVERSIONINFOW osvi;
  iupwinGetVersionInfo(&osvi);
  return osvi.dwBuildNumber;
}

IUP_DRV_API int iupwinIsWinXPOrNew(void)
{
  return iupwinCheckWindowsVersion(5, 1);
}

IUP_DRV_API int iupwinIsVistaOrNew(void)
{
  return iupwinCheckWindowsVersion(6, 0);
}

IUP_DRV_API int iupwinIsWin7OrNew(void)
{
  return iupwinCheckWindowsVersion(6, 1);
}

IUP_DRV_API int iupwinIsWin10OrNew(void)
{
  return iupwinCheckWindowsVersion(6, 4);
}

#define PACKVERSION(major,minor) MAKELONG(minor,major)
typedef struct _DLLVERSIONINFO
{
  DWORD cbSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformID;
} DLLVERSIONINFO;
typedef HRESULT (CALLBACK* DLLGETVERSIONPROC)(DLLVERSIONINFO*);

static DWORD winGetDllVersion(LPCTSTR lpszDllName)
{
  DWORD dwVersion = 0;
  DLLGETVERSIONPROC pDllGetVersion;
  HINSTANCE hinstDll;

  /* For security purposes, LoadLibrary should be provided with a
  fully-qualified path to the DLL. The lpszDllName variable should be
  tested to ensure that it is a fully qualified path before it is used. */
  hinstDll = LoadLibrary(lpszDllName);
  if (!hinstDll)
    return 0;

  pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");

  /* Because some DLLs might not implement this function, you
  must test for it explicitly. Depending on the particular
  DLL, the lack of a DllGetVersion function can be a useful
  indicator of the version. */

  if (pDllGetVersion)
  {
    DLLVERSIONINFO dvi;
    HRESULT hr;

    ZeroMemory(&dvi, sizeof(dvi));
    dvi.cbSize = sizeof(dvi);

    hr = pDllGetVersion(&dvi);
    if (SUCCEEDED(hr))
      dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
  }

  FreeLibrary(hinstDll);

  return dwVersion;
}

IUP_DRV_API int iupwinGetComCtl32Version(void)
{
  return winGetDllVersion(TEXT("comctl32.dll"));
}

IUP_DRV_API int iupwinIsAppThemed(void)
{
  typedef BOOL (STDAPICALLTYPE* winIsAppThemed)(void);
  static winIsAppThemed myIsAppThemed = NULL;
  if (!myIsAppThemed)
  {
    HMODULE hinstDll = LoadLibrary(TEXT("uxtheme.dll"));
    if (hinstDll)
      myIsAppThemed = (winIsAppThemed)GetProcAddress(hinstDll, "IsAppThemed");
  }

  if (myIsAppThemed)
    return myIsAppThemed();
  else
    return 0;
}

IUP_DRV_API int iupwinIsSystemDarkMode(void)
{
  typedef HRESULT(STDAPICALLTYPE* PtrDwmGetWindowAttribute)(HWND, DWORD, PVOID, DWORD);
  static PtrDwmGetWindowAttribute dwmGetWindowAttribute = NULL;
  static int initialized = 0;
  static int dark_mode = 0;

  if (!iupwinIsWin10OrNew())
    return 0;

  if (!initialized)
  {
    HMODULE dwmLibrary = LoadLibrary(TEXT("dwmapi.dll"));
    if (dwmLibrary)
      dwmGetWindowAttribute = (PtrDwmGetWindowAttribute)GetProcAddress(dwmLibrary, "DwmGetWindowAttribute");
    initialized = 1;
  }

  if (dwmGetWindowAttribute)
  {
    HKEY hKey;
    DWORD value = 0;
    DWORD size = sizeof(DWORD);

    if (RegOpenKeyExA(HKEY_CURRENT_USER,
                      "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
      if (RegQueryValueExA(hKey, "AppsUseLightTheme", NULL, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS)
        dark_mode = (value == 0) ? 1 : 0;
      RegCloseKey(hKey);
    }
  }

  return dark_mode;
}

IUP_SDK_API void iupdrvGetScreenSize(int* width, int* height)
{
  RECT area;
  SystemParametersInfoA(SPI_GETWORKAREA, 0, &area, 0);
  *width = (int)(area.right - area.left);
  *height = (int)(area.bottom - area.top);
}

IUP_SDK_API void iupdrvAddScreenOffset(int* x, int* y, int add)
{
  RECT area;
  SystemParametersInfoA(SPI_GETWORKAREA, 0, &area, 0);
  if (add == 1)
  {
    if (x) *x += area.left;
    if (y) *y += area.top;
  }
  else
  {
    if (x) *x -= area.left;
    if (y) *y -= area.top;
  }
}

IUP_SDK_API void iupdrvGetFullSize(int* width, int* height)
{
  RECT rect;
  GetWindowRect(GetDesktopWindow(), &rect);
  *width = rect.right - rect.left;
  *height = rect.bottom - rect.top;
}

IUP_SDK_API int iupdrvGetScreenDepth(void)
{
  int bpp;
  HDC hDCDisplay = GetDC(NULL);
  bpp = GetDeviceCaps(hDCDisplay, BITSPIXEL);
  ReleaseDC(NULL, hDCDisplay);
  return bpp;
}

IUP_SDK_API double iupdrvGetScreenDpi(void)
{
  double dpi;
  HDC hDCDisplay = GetDC(NULL);
  dpi = (double)GetDeviceCaps(hDCDisplay, LOGPIXELSY);
  ReleaseDC(NULL, hDCDisplay);
  return dpi;
}

IUP_SDK_API int iupdrvScaleNaturalPx(int px)
{
  return px;
}

IUP_SDK_API void iupdrvGetCursorPos(int* x, int* y)
{
  POINT CursorPoint;
  GetCursorPos(&CursorPoint);
  *x = (int)CursorPoint.x;
  *y = (int)CursorPoint.y;

  iupdrvAddScreenOffset(x, y, -1);
}

IUP_SDK_API void iupdrvGetKeyState(char* key)
{
  if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
    key[0] = 'S';
  else
    key[0] = ' ';
  if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
    key[1] = 'C';
  else
    key[1] = ' ';
  if (GetAsyncKeyState(VK_MENU) & 0x8000)
    key[2] = 'A';
  else
    key[2] = ' ';
  if ((GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000))
    key[3] = 'Y';
  else
    key[3] = ' ';

  key[4] = 0;
}
