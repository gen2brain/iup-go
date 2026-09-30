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

typedef LONG (WINAPI* PFN_RtlGetVersion)(OSVERSIONINFOW*);

static void iupwinuiGetVersionInfo(OSVERSIONINFOW* osvi)
{
  HMODULE ntdll = GetModuleHandleA("ntdll.dll");
  auto pRtlGetVersion = reinterpret_cast<PFN_RtlGetVersion>(GetProcAddress(ntdll, "RtlGetVersion"));
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
      return const_cast<char*>("WinNT");

    if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
      return const_cast<char*>("Win2K");

    if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion > 0)
      return const_cast<char*>("WinXP");

    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
      return const_cast<char*>("Vista");

    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
      return const_cast<char*>("Win7");

    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2)
      return const_cast<char*>("Win8");

    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3)
      return const_cast<char*>("Win81");

    if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0)
    {
      if (osvi.dwBuildNumber >= 22000)
        return const_cast<char*>("Win11");
      return const_cast<char*>("Win10");
    }
  }

  return const_cast<char*>("Windows");
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
  GetComputerNameA(static_cast<LPSTR>(str), &size);
  return str;
}

extern "C" IUP_SDK_API char* iupdrvGetUserName(void)
{
  DWORD size = 256;
  char* str = iupStrGetMemory(size);
  GetUserNameA(static_cast<LPSTR>(str), &size);
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
  return CreateDirectoryA(path, nullptr) ? 1 : 0;
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
    if (!iupdrvGetUserDir(filename, 10240, IUP_USER_DIR_CONFIG))
    {
      filename[0] = '\0';
      return 0;
    }
    snprintf(filename + strlen(filename), 10240 - strlen(filename), "\\%s", app_name);
    iupwinuiMakeDirectory(filename);
    snprintf(filename + strlen(filename), 10240 - strlen(filename), "\\config.cfg");
    return 1;
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

extern "C" IUP_SDK_API int iupdrvGetUserDir(char* path, int size, int kind)
{
  if (!path || size <= 0) return 0;
  path[0] = '\0';

  if (kind == IUP_USER_DIR_TEMP)
  {
    DWORD n = GetTempPathA(static_cast<DWORD>(size), path);
    if (n == 0 || n >= static_cast<DWORD>(size)) return 0;
    if (n >= 1 && (path[n - 1] == '\\' || path[n - 1] == '/'))
      path[n - 1] = '\0';
    return 1;
  }

  if (SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, path) != S_OK)
    return 0;
  return 1;
}

extern "C" IUP_SDK_API void iupdrvGetScreenSize(int* width, int* height)
{
  RECT area;
  SystemParametersInfoA(SPI_GETWORKAREA, 0, &area, 0);
  *width = static_cast<int>(area.right - area.left);
  *height = static_cast<int>(area.bottom - area.top);
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
  HDC hDCDisplay = GetDC(nullptr);
  bpp = GetDeviceCaps(hDCDisplay, BITSPIXEL);
  ReleaseDC(nullptr, hDCDisplay);
  return bpp;
}

extern "C" IUP_SDK_API double iupdrvGetScreenDpi(void)
{
  double dpi;
  HDC hDCDisplay = GetDC(nullptr);
  dpi = static_cast<double>(GetDeviceCaps(hDCDisplay, LOGPIXELSY));
  ReleaseDC(nullptr, hDCDisplay);
  return dpi;
}

extern "C" IUP_SDK_API int iupdrvScaleNaturalPx(int px)
{
  return px;
}

extern "C" IUP_SDK_API void iupdrvGetCursorPos(int* x, int* y)
{
  POINT cursorPoint;
  GetCursorPos(&cursorPoint);
  *x = static_cast<int>(cursorPoint.x);
  *y = static_cast<int>(cursorPoint.y);

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
  UINT codepage = GetACP();
  if (codepage == CP_UTF8)
    return const_cast<char*>("UTF-8");
  return iupStrReturnStrf("CP%u", codepage);
}

extern "C" IUP_SDK_API char* iupdrvExeFileName(void)
{
  wchar_t filename[10240];
  if (GetModuleFileNameW(nullptr, filename, 10240) == 0)
    return nullptr;
  return iupwinuiHStringToString(winrt::hstring(filename));
}

extern "C" IUP_SDK_API char* iupdrvLanguageInfo(void)
{
  WCHAR wname[LOCALE_NAME_MAX_LENGTH];
  char name[LOCALE_NAME_MAX_LENGTH];

  if (!LCIDToLocaleName(MAKELCID(GetUserDefaultUILanguage(), SORT_DEFAULT), wname, LOCALE_NAME_MAX_LENGTH, 0))
    return nullptr;
  if (!WideCharToMultiByte(CP_UTF8, 0, wname, -1, name, sizeof(name), nullptr, nullptr))
    return nullptr;
  return iupStrLanguageTag(name);
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
  return nullptr;
}

extern "C" IUP_SDK_API char* iupdrvGetCurrentDirectory(void)
{
  char* cur_dir = nullptr;

  int len = GetCurrentDirectoryA(0, nullptr);
  if (len == 0) return nullptr;

  cur_dir = iupStrGetMemory(len + 2);
  GetCurrentDirectoryA(len + 1, cur_dir);
  cur_dir[len] = '\\';
  cur_dir[len + 1] = 0;

  return cur_dir;
}

extern "C" IUP_SDK_API int iupdrvSetCurrentDirectory(const char* dir)
{
  return SetCurrentDirectoryA(dir);
}
