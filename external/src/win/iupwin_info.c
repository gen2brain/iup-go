/** \file
 * \brief Windows System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h> 

#include <windows.h>
#include <uxtheme.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_drv.h"

#include "iupwin_info.h"
#include "iupwin_str.h"

#include <windows.h>
#include <ShlObj.h> /* for SHGetFolderPath */


#ifdef _MSC_VER
/* warning C4996: 'GetVersionExW': was declared deprecated */
#pragma warning( disable : 4996 )
#endif

/* other method, when replaced should do it in CD also (cdwinp.cpp) */
static BOOL winCheckWindowsVersion(DWORD major, DWORD minor) 
{
  OSVERSIONINFOEX osvi;
  DWORDLONG dwlConditionMask = 0;

  ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
  osvi.dwMajorVersion = major;
  osvi.dwMinorVersion = minor;

  VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
  VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);

  return VerifyVersionInfo(&osvi, VER_MAJORVERSION|VER_MINORVERSION, dwlConditionMask);
}
        
int iupwinCheckWindowsVersion(DWORD major, DWORD minor)
{
  OSVERSIONINFO osvi;
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&osvi);

  if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && 
      (osvi.dwMajorVersion > major || (osvi.dwMajorVersion == major && osvi.dwMinorVersion >= minor)))
    return 1;

  return 0;
}

int iupwinIsWinXPOrNew(void)
{
  return iupwinCheckWindowsVersion(5, 1);
}

int iupwinIsVistaOrNew(void)
{
  return iupwinCheckWindowsVersion(6, 0);
}

int iupwinIsWin7OrNew(void)
{
  return iupwinCheckWindowsVersion(6, 1);
}

int iupwinIsWin8OrNew(void)
{
  return iupwinCheckWindowsVersion(6, 2);
}

int iupwinIsWin10OrNew(void)
{
  return iupwinCheckWindowsVersion(6, 4);
}

char *iupwinGetSystemLanguage(void)
{
  LANGID id = GetSystemDefaultUILanguage();
  char *lang = NULL;
  switch(id)
  {
    case 0x0000: lang = "Language Neutral"; break;
    case 0x007f: lang = "Locale Invariant"; break;
    case 0x0400: lang = "User Default Language"; break;
    case 0x0800: lang = "System Default Language"; break;
    case 0x0436: lang = "Afrikaans"; break;
    case 0x041c: lang = "Albanian"; break;
    case 0x0401: lang = "Arabic (Saudi Arabia)"; break;
    case 0x0801: lang = "Arabic (Iraq)"; break;
    case 0x0c01: lang = "Arabic (Egypt)"; break;
    case 0x1001: lang = "Arabic (Libya)"; break;
    case 0x1401: lang = "Arabic (Algeria)"; break;
    case 0x1801: lang = "Arabic (Morocco)"; break;
    case 0x1c01: lang = "Arabic (Tunisia)"; break;
    case 0x2001: lang = "Arabic (Oman)"; break;
    case 0x2401: lang = "Arabic (Yemen)"; break;
    case 0x2801: lang = "Arabic (Syria)"; break;
    case 0x2c01: lang = "Arabic (Jordan)"; break;
    case 0x3001: lang = "Arabic (Lebanon)"; break;
    case 0x3401: lang = "Arabic (Kuwait)"; break;
    case 0x3801: lang = "Arabic (U.A.E.)"; break;
    case 0x3c01: lang = "Arabic (Bahrain)"; break;
    case 0x4001: lang = "Arabic (Qatar)"; break;
    case 0x042b: lang = "Armenian"; break;
    case 0x042c: lang = "Azeri (Latin)"; break;
    case 0x082c: lang = "Azeri (Cyrillic)"; break;
    case 0x042d: lang = "Basque"; break;
    case 0x0423: lang = "Belarusian"; break;
    case 0x0402: lang = "Bulgarian"; break;
    case 0x0455: lang = "Burmese"; break;
    case 0x0403: lang = "Catalan"; break;
    case 0x0404: lang = "Chinese (Taiwan)"; break;
    case 0x0804: lang = "Chinese"; break;
    case 0x0c04: lang = "Chinese (Hong Kong)"; break;
    case 0x1004: lang = "Chinese (Singapore)"; break;
    case 0x1404: lang = "Chinese (Macau)"; break;
    case 0x041a: lang = "Croatian"; break;
    case 0x0405: lang = "Czech"; break;
    case 0x0406: lang = "Danish"; break;
    case 0x0465: lang = "Divehi"; break;
    case 0x0413: lang = "Dutch (Netherlands)"; break;
    case 0x0813: lang = "Dutch (Belgium)"; break;
    case 0x0409: lang = "English (United States)"; break;
    case 0x0809: lang = "English (United Kingdom)"; break;
    case 0x0c09: lang = "English (Australian)"; break;
    case 0x1009: lang = "English (Canadian)"; break;
    case 0x1409: lang = "English (New Zealand)"; break;
    case 0x1809: lang = "English (Ireland)"; break;
    case 0x1c09: lang = "English (South Africa)"; break;
    case 0x2009: lang = "English (Jamaica)"; break;
    case 0x2409: lang = "English (Caribbean)"; break;
    case 0x2809: lang = "English (Belize)"; break;
    case 0x2c09: lang = "English (Trinidad)"; break;
    case 0x3009: lang = "English (Zimbabwe)"; break;
    case 0x3409: lang = "English (Philippines)"; break;
    case 0x0425: lang = "Estonian"; break;
    case 0x0438: lang = "Faeroese"; break;
    case 0x0429: lang = "Farsi"; break;
    case 0x040b: lang = "Finnish"; break;
    case 0x040c: lang = "French (Standard)"; break;
    case 0x080c: lang = "French (Belgian)"; break;
    case 0x0c0c: lang = "French (Canadian)"; break;
    case 0x100c: lang = "French (Switzerland)"; break;
    case 0x140c: lang = "French (Luxembourg)"; break;
    case 0x180c: lang = "French (Monaco)"; break;
    case 0x0456: lang = "Galician"; break;
    case 0x0437: lang = "Georgian"; break;
    case 0x0407: lang = "German (Standard)"; break;
    case 0x0807: lang = "German (Switzerland)"; break;
    case 0x0c07: lang = "German (Austria)"; break;
    case 0x1007: lang = "German (Luxembourg)"; break;
    case 0x1407: lang = "German (Liechtenstein)"; break;
    case 0x0408: lang = "Greek"; break;
    case 0x0447: lang = "Gujarati"; break;
    case 0x040d: lang = "Hebrew"; break;
    case 0x0439: lang = "Hindi"; break;
    case 0x040e: lang = "Hungarian"; break;
    case 0x040f: lang = "Icelandic"; break;
    case 0x0421: lang = "Indonesian"; break;
    case 0x0410: lang = "Italian (Standard)"; break;
    case 0x0810: lang = "Italian (Switzerland)"; break;
    case 0x0411: lang = "Japanese"; break;
    case 0x044b: lang = "Kannada"; break;
    case 0x0457: lang = "Konkani"; break;
    case 0x0412: lang = "Korean"; break;
    case 0x0812: lang = "Korean (Johab)"; break;
    case 0x0440: lang = "Kyrgyz"; break;
    case 0x0426: lang = "Latvian"; break;
    case 0x0427: lang = "Lithuanian"; break;
    case 0x0827: lang = "Lithuanian (Classic)"; break;
    case 0x042f: lang = "Macedonian"; break;
    case 0x043e: lang = "Malay (Malaysian)"; break;
    case 0x083e: lang = "Malay (Brunei Darussalam)"; break;
    case 0x044e: lang = "Marathi"; break;
    case 0x0450: lang = "Mongolian"; break;
    case 0x0414: lang = "Norwegian (Bokmal)"; break;
    case 0x0814: lang = "Norwegian (Nynorsk)"; break;
    case 0x0415: lang = "Polish"; break;
    case 0x0416: lang = "Portuguese (Brazil)"; break;
    case 0x0816: lang = "Portuguese (Portugal)"; break;
    case 0x0446: lang = "Punjabi"; break;
    case 0x0418: lang = "Romanian"; break;
    case 0x0419: lang = "Russian"; break;
    case 0x044f: lang = "Sanskrit"; break;
    case 0x0c1a: lang = "Serbian (Cyrillic)"; break;
    case 0x081a: lang = "Serbian (Latin)"; break;
    case 0x041b: lang = "Slovak"; break;
    case 0x0424: lang = "Slovenian"; break;
    case 0x040a: lang = "Spanish (Spain, Traditional Sort)"; break;
    case 0x080a: lang = "Spanish (Mexican)"; break;
    case 0x0c0a: lang = "Spanish (Spain, International Sort)"; break;
    case 0x100a: lang = "Spanish (Guatemala)"; break;
    case 0x140a: lang = "Spanish (Costa Rica)"; break;
    case 0x180a: lang = "Spanish (Panama)"; break;
    case 0x1c0a: lang = "Spanish (Dominican Republic)"; break;
    case 0x200a: lang = "Spanish (Venezuela)"; break;
    case 0x240a: lang = "Spanish (Colombia)"; break;
    case 0x280a: lang = "Spanish (Peru)"; break;
    case 0x2c0a: lang = "Spanish (Argentina)"; break;
    case 0x300a: lang = "Spanish (Ecuador)"; break;
    case 0x340a: lang = "Spanish (Chile)"; break;
    case 0x380a: lang = "Spanish (Uruguay)"; break;
    case 0x3c0a: lang = "Spanish (Paraguay)"; break;
    case 0x400a: lang = "Spanish (Bolivia)"; break;
    case 0x440a: lang = "Spanish (El Salvador)"; break;
    case 0x480a: lang = "Spanish (Honduras)"; break;
    case 0x4c0a: lang = "Spanish (Nicaragua)"; break;
    case 0x500a: lang = "Spanish (Puerto Rico)"; break;
    case 0x0430: lang = "Sutu"; break;
    case 0x0441: lang = "Swahili (Kenya)"; break;
    case 0x041d: lang = "Swedish"; break;
    case 0x081d: lang = "Swedish (Finland)"; break;
    case 0x045a: lang = "Syriac"; break;
    case 0x0449: lang = "Tamil"; break;
    case 0x0444: lang = "Tatar (Tatarstan)"; break;
    case 0x044a: lang = "Telugu"; break;
    case 0x041e: lang = "Thai"; break;
    case 0x041f: lang = "Turkish"; break;
    case 0x0422: lang = "Ukrainian"; break;
    case 0x0420: lang = "Urdu (Pakistan)"; break;
    case 0x0820: lang = "Urdu (India)"; break;
    case 0x0443: lang = "Uzbek (Latin)"; break;
    case 0x0843: lang = "Uzbek (Cyrillic)"; break;
    case 0x042a: lang = "Vietnamese"; break;
  }
  return lang;
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
typedef HRESULT (CALLBACK* DLLGETVERSIONPROC)(DLLVERSIONINFO *);

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

int iupwinGetComCtl32Version(void)
{
  return winGetDllVersion(TEXT("comctl32.dll"));
}

int iupwinIsAppThemed(void)
{
  typedef BOOL (STDAPICALLTYPE *winIsAppThemed)(void);
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

IUP_SDK_API void iupdrvGetScreenSize(int *width, int *height)
{
  RECT area;
  SystemParametersInfoA(SPI_GETWORKAREA, 0, &area, 0);
  *width = (int)(area.right - area.left);
  *height = (int)(area.bottom - area.top);
}

IUP_SDK_API void iupdrvAddScreenOffset(int *x, int *y, int add)
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

IUP_SDK_API void iupdrvGetFullSize(int *width, int *height)
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

IUP_SDK_API void iupdrvGetCursorPos(int *x, int *y)
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

IUP_SDK_API char *iupdrvGetComputerName(void)
{
  DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
  TCHAR wstr[MAX_COMPUTERNAME_LENGTH + 1];
  GetComputerName(wstr, &size);
  return iupwinStrFromSystem(wstr);
}

#define UNLEN 256 /* from <Lmcons.h> */

IUP_SDK_API char *iupdrvGetUserName(void)
{
  DWORD size = UNLEN+1;
  TCHAR wstr[UNLEN+1];
  GetUserName(wstr, &size);
  return iupwinStrFromSystem(wstr);
}

IUP_SDK_API int iupdrvSetCurrentDirectory(const char* path)
{
  return SetCurrentDirectory(iupwinStrToSystemFilename(path));
}

IUP_SDK_API char* iupdrvGetCurrentDirectory(void)
{
  TCHAR* wcur_dir = NULL;
  char* cur_dir;

  int len = GetCurrentDirectory(0, NULL);
  if (len == 0) return NULL;

  wcur_dir = (TCHAR*)malloc((len + 2)*sizeof(TCHAR));
  GetCurrentDirectory(len + 1, wcur_dir);
  wcur_dir[len] = '\\';
  wcur_dir[len + 1] = 0;

  cur_dir = iupwinStrFromSystemFilename(wcur_dir);
  free(wcur_dir);
  return cur_dir;
}

/*
Windows 7 and 10
PreferencePath(0)=C:\Users\Tecgraf\
PreferencePath(1)=C:\Users\Tecgraf\AppData\Roaming\

Windows XP
PreferencePath(0)=C:\Documents and Settings\Tecgraf\
PreferencePath(1)=C:\Documents and Settings\Tecgraf\Application Data\
*/

IUP_SDK_API int iupdrvGetPreferencePath(char *filename, int use_system)
{
  char* homedrive;
  char* homepath;

  if (use_system)
  {
    TCHAR wpath[MAX_PATH];
    if (SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, wpath) == S_OK)
    {
      strcpy(filename, iupwinStrFromSystemFilename(wpath));
      strcat(filename, "\\");
      return 1;
    }
  }

  homedrive = getenv("HOMEDRIVE");
  homepath = getenv("HOMEPATH");
  if (homedrive && homepath)
  {
    strcpy(filename, homedrive);
    strcat(filename, homepath);
    strcat(filename, "\\");
    return 1;
  }

  filename[0] = '\0';
  return 0;

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
    OutputDebugString(iupwinStrToSystem(value));
    return;
  }
  else if (iupStrEqualNoCase(type, "ERROR"))
    wtype = EVENTLOG_ERROR_TYPE;
  else if (iupStrEqualNoCase(type, "WARNING"))
    wtype = EVENTLOG_WARNING_TYPE;
  else if (iupStrEqualNoCase(type, "INFO"))
    wtype = EVENTLOG_INFORMATION_TYPE;

  EventSource = RegisterEventSource(NULL, TEXT("Application"));
  if (EventSource)
  {
    TCHAR* wstr[1];
    wstr[0] = iupwinStrToSystem(value);
    ReportEvent(EventSource, wtype, 0, 0, NULL, 1, 0, wstr, NULL);
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
