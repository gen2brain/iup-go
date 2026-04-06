/** \file
 * \brief WinUI Driver - System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>

#include <windows.h>
#include <shlobj.h>

extern "C" {
#include "iup.h"
#include "iup_str.h"
#include "iup_drvinfo.h"
}

#include "iupwinui_drv.h"

typedef LONG (WINAPI *PFN_RtlGetVersion)(OSVERSIONINFOW*);

static void iupwinuiGetVersionInfo(OSVERSIONINFOW* osvi)
{
  HMODULE ntdll = GetModuleHandleA("ntdll.dll");
  PFN_RtlGetVersion pRtlGetVersion = (PFN_RtlGetVersion)GetProcAddress(ntdll, "RtlGetVersion");
  ZeroMemory(osvi, sizeof(OSVERSIONINFOW));
  osvi->dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
  pRtlGetVersion(osvi);
}

extern "C" IUP_SDK_API char* iupdrvGetSystemName(void)
{
  OSVERSIONINFOW osvi;
  iupwinuiGetVersionInfo(&osvi);

  if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
  {
    if (osvi.dwMajorVersion <= 4)
      return (char*)"WinNT";

    if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
      return (char*)"Win2K";

    if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion > 0)
      return (char*)"WinXP";

    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
      return (char*)"Vista";

    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
      return (char*)"Win7";

    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2)
      return (char*)"Win8";

    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3)
      return (char*)"Win81";

    if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0)
    {
      if (osvi.dwBuildNumber >= 22000)
        return (char*)"Win11";
      return (char*)"Win10";
    }
  }

  return (char*)"Windows";
}

extern "C" IUP_SDK_API char* iupdrvGetSystemVersion(void)
{
  static char version[50] = "";
  if (version[0] == 0)
  {
    OSVERSIONINFOW osvi;
    iupwinuiGetVersionInfo(&osvi);

    snprintf(version, sizeof(version), "%lu.%lu.%lu",
            osvi.dwMajorVersion,
            osvi.dwMinorVersion,
            osvi.dwBuildNumber);
  }
  return version;
}

extern "C" IUP_SDK_API char* iupdrvGetComputerName(void)
{
  DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
  char* str = iupStrGetMemory(size);
  GetComputerNameA((LPSTR)str, &size);
  return str;
}

extern "C" IUP_SDK_API char* iupdrvGetUserName(void)
{
  DWORD size = 256;
  char* str = iupStrGetMemory(size);
  GetUserNameA((LPSTR)str, &size);
  return str;
}

static int iupwinuiMakeDirectory(const char* path)
{
  DWORD attrib = GetFileAttributesA(path);
  if (attrib != INVALID_FILE_ATTRIBUTES)
  {
    if (attrib & FILE_ATTRIBUTE_DIRECTORY)
      return 1;
    return 0;
  }
  return CreateDirectoryA(path, NULL) ? 1 : 0;
}

extern "C" IUP_SDK_API int iupdrvGetPreferencePath(char* filename, const char* app_name, int use_system)
{
  char* homedrive;
  char* homepath;

  if (!app_name || !app_name[0])
  {
    filename[0] = '\0';
    return 0;
  }

  if (use_system)
  {
    if (SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, filename) == S_OK)
    {
      snprintf(filename + strlen(filename), 10240 - strlen(filename), "\\%s", app_name);
      iupwinuiMakeDirectory(filename);

      snprintf(filename + strlen(filename), 10240 - strlen(filename), "\\config.cfg");
      return 1;
    }
  }

  homedrive = getenv("HOMEDRIVE");
  homepath = getenv("HOMEPATH");
  if (homedrive && homepath)
  {
    snprintf(filename, 10240, "%s%s\\%s.cfg", homedrive, homepath, app_name);
    return 1;
  }

  filename[0] = '\0';
  return 0;
}

extern "C" IUP_SDK_API void iupdrvGetScreenSize(int* width, int* height)
{
  RECT area;
  SystemParametersInfoA(SPI_GETWORKAREA, 0, &area, 0);
  *width = (int)(area.right - area.left);
  *height = (int)(area.bottom - area.top);
}

extern "C" IUP_SDK_API void iupdrvGetFullSize(int* width, int* height)
{
  RECT rect;
  GetWindowRect(GetDesktopWindow(), &rect);
  *width = rect.right - rect.left;
  *height = rect.bottom - rect.top;
}

extern "C" IUP_SDK_API int iupdrvGetScreenDepth(void)
{
  int bpp;
  HDC hDCDisplay = GetDC(NULL);
  bpp = GetDeviceCaps(hDCDisplay, BITSPIXEL);
  ReleaseDC(NULL, hDCDisplay);
  return bpp;
}

extern "C" IUP_SDK_API double iupdrvGetScreenDpi(void)
{
  double dpi;
  HDC hDCDisplay = GetDC(NULL);
  dpi = (double)GetDeviceCaps(hDCDisplay, LOGPIXELSY);
  ReleaseDC(NULL, hDCDisplay);
  return dpi;
}

extern "C" IUP_SDK_API void iupdrvGetCursorPos(int* x, int* y)
{
  POINT cursorPoint;
  GetCursorPos(&cursorPoint);
  *x = (int)cursorPoint.x;
  *y = (int)cursorPoint.y;

  iupdrvAddScreenOffset(x, y, -1);
}

extern "C" IUP_SDK_API void iupdrvGetKeyState(char* key)
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

extern "C" IUP_SDK_API char* iupdrvLocaleInfo(void)
{
  static char locale[10] = "";
  if (locale[0] == 0)
  {
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, locale, sizeof(locale));
    int len = (int)strlen(locale);
    locale[len] = '_';
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, locale + len + 1, sizeof(locale) - len - 1);
  }
  return locale;
}

extern "C" IUP_SDK_API void iupdrvAddScreenOffset(int* x, int* y, int add)
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

extern "C" IUP_SDK_API void* iupdrvGetDisplay(void)
{
  return NULL;
}

extern "C" IUP_SDK_API char* iupdrvGetCurrentDirectory(void)
{
  char* cur_dir = NULL;

  int len = GetCurrentDirectoryA(0, NULL);
  if (len == 0) return NULL;

  cur_dir = iupStrGetMemory(len + 2);
  GetCurrentDirectoryA(len + 1, cur_dir);
  cur_dir[len] = '\\';
  cur_dir[len + 1] = 0;

  return cur_dir;
}

extern "C" IUP_SDK_API int iupdrvSetCurrentDirectory(const char* path)
{
  return SetCurrentDirectoryA(path);
}
