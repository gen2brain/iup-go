/** \file
 * \brief Windows System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This module should depend only on IUP core headers 
   and Windows system headers. */

#include <windows.h>
#include <shlobj.h>

#include "iup_export.h"
#include "iup_str.h"
#include "iup_drvinfo.h"
#include "iup_varg.h"


typedef LONG (WINAPI *PFN_RtlGetVersion)(OSVERSIONINFOW*);

static void iupwinGetVersionInfo(OSVERSIONINFOW* osvi)
{
  HMODULE ntdll = GetModuleHandleA("ntdll.dll");
  PFN_RtlGetVersion pRtlGetVersion = (PFN_RtlGetVersion)GetProcAddress(ntdll, "RtlGetVersion");
  ZeroMemory(osvi, sizeof(OSVERSIONINFOW));
  osvi->dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
  pRtlGetVersion(osvi);
}

IUP_SDK_API char* iupdrvLocaleInfo(void)
{
  CPINFOEXA info;
  GetCPInfoExA(CP_ACP, 0, &info);
  return iupStrReturnStr(info.CodePageName);
}

IUP_SDK_API char *iupdrvGetSystemName(void)
{
  OSVERSIONINFOW osvi;
  iupwinGetVersionInfo(&osvi);

  if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
  {
    if (osvi.dwMajorVersion <= 4)
      return "WinNT";

    if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
      return "Win2K";

    if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion > 0)
      return "WinXP";

    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
      return "Vista";

    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
      return "Win7";

    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2)
      return "Win8";

    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3)
      return "Win81";

    if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0)
    {
      if (osvi.dwBuildNumber >= 22000)
        return "Win11";
      return "Win10";
    }
  }

  return "Windows";
}

IUP_SDK_API char *iupdrvGetSystemVersion(void)
{
  char *str = iupStrGetMemory(256);
  OSVERSIONINFOW osvi;
  SYSTEM_INFO si;

  ZeroMemory(&si, sizeof(SYSTEM_INFO));
  GetSystemInfo(&si);

  iupwinGetVersionInfo(&osvi);

  snprintf(str, 256, "%d.%d.%d", (int)osvi.dwMajorVersion, (int)osvi.dwMinorVersion, (int)osvi.dwBuildNumber);

  {
    int pos = (int)strlen(str);

    /* Display service pack (if any). */
    if (osvi.szCSDVersion[0] != 0)
    {
      char csd[128];
      WideCharToMultiByte(CP_ACP, 0, osvi.szCSDVersion, -1, csd, sizeof(csd), NULL, NULL);
      pos += snprintf(str + pos, 256 - pos, " %s", csd);
    }

    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
      snprintf(str + pos, 256 - pos, " (IA64)");
    else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
      snprintf(str + pos, 256 - pos, " (x64)");
    else
      snprintf(str + pos, 256 - pos, " (x86)");
  }

  return str;
}

static int iupwinMakeDirectory(const char* path)
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

IUP_SDK_API int iupdrvGetPreferencePath(char *filename, const char *app_name, int use_system)
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
    /* Windows Local AppData: %LOCALAPPDATA%\appname\config.cfg */
    if (SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, filename) == S_OK)
    {
      /* Add app directory */
      snprintf(filename + strlen(filename), 10240 - strlen(filename), "\\%s", app_name);
      iupwinMakeDirectory(filename);

      /* Add config filename */
      snprintf(filename + strlen(filename), 10240 - strlen(filename), "\\config.cfg");
      return 1;
    }
  }

  /* Legacy: %HOMEDRIVE%%HOMEPATH%\appname.cfg */
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

IUP_SDK_API char *iupdrvGetComputerName(void)
{
  DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
  char* str = iupStrGetMemory(size);
  GetComputerNameA((LPSTR)str, &size);
  return str;
}

IUP_SDK_API char *iupdrvGetUserName(void)
{
  DWORD size = 256;
  char* str = iupStrGetMemory(size);
  GetUserNameA((LPSTR)str, &size);
  return str;
}

IUP_SDK_API int iupdrvSetCurrentDirectory(const char* path)
{
  return SetCurrentDirectoryA(path);
}

IUP_SDK_API char* iupdrvGetCurrentDirectory(void)
{
  int len = GetCurrentDirectoryA(0, NULL);
  if (len == 0) return NULL;

  char* cur_dir = iupStrGetMemory(len + 2);
  GetCurrentDirectoryA(len + 1, cur_dir);
  cur_dir[len] = '\\';
  cur_dir[len + 1] = 0;

  return cur_dir;
}

IUP_API void IupLogV(const char* type, const char* format, va_list arglist)
{
  HANDLE EventSource;
  WORD wtype = 0;

  int size;
  char* value = iupStrGetLargeMem(&size);
  vsnprintf(value, size, format, arglist);

  if (iupStrEqualNoCase(type, "DEBUG"))
  {
    OutputDebugStringA(value);
    return;
  }
  else if (iupStrEqualNoCase(type, "ERROR"))
    wtype = EVENTLOG_ERROR_TYPE;
  else if (iupStrEqualNoCase(type, "WARNING"))
    wtype = EVENTLOG_WARNING_TYPE;
  else if (iupStrEqualNoCase(type, "INFO"))
    wtype = EVENTLOG_INFORMATION_TYPE;

  EventSource = RegisterEventSourceA(NULL, "Application");
  if (EventSource)
  {
    const char* strings[1];
    strings[0] = value;
    ReportEventA(EventSource, wtype, 0, 0, NULL, 1, 0, strings, NULL);
    DeregisterEventSource(EventSource);
  }
}

IUP_API void IupLog(const char* type, const char* format, ...)
{
  va_list arglist;
  va_start(arglist, format);
  IupLogV(type, format, arglist);
  va_end(arglist);
}
