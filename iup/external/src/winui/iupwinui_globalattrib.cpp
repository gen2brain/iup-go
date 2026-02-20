/** \file
 * \brief WinUI Driver - Global Attributes
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>

extern "C" {
#include "iup.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_globalattrib.h"
}

#include "iupwinui_drv.h"

static int winui_monitor_index = 0;

static BOOL CALLBACK winuiMonitorInfoEnum(HMONITOR handle, HDC handle_dc, LPRECT rect, LPARAM data)
{
  RECT* monitors_rect = (RECT*)data;
  monitors_rect[winui_monitor_index] = *rect;
  winui_monitor_index++;
  (void)handle_dc;
  (void)handle;
  return TRUE;
}

extern "C" int iupdrvSetGlobal(const char* name, const char* value)
{
  if (iupStrEqual(name, "UTF8MODE"))
  {
    return 1;
  }
  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
    return 0;
  }
  if (iupStrEqual(name, "SHOWMENUIMAGES"))
  {
    (void)value;
    return 1;
  }
  return 1;
}

extern "C" char* iupdrvGetGlobal(const char* name)
{
  if (iupStrEqual(name, "VIRTUALSCREEN"))
  {
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    return iupStrReturnStrf("%d %d %d %d", x, y, w, h);
  }
  if (iupStrEqual(name, "MONITORSINFO"))
  {
    int i;
    int monitors_count = GetSystemMetrics(SM_CMONITORS);
    RECT* monitors_rect = (RECT*)malloc(monitors_count * sizeof(RECT));
    char* str = iupStrGetMemory(monitors_count * 50);
    char* pstr = str;

    winui_monitor_index = 0;
    EnumDisplayMonitors(NULL, NULL, winuiMonitorInfoEnum, (LPARAM)monitors_rect);

    for (i = 0; i < monitors_count; i++)
      pstr += sprintf(pstr, "%d %d %d %d\n", (int)monitors_rect[i].left, (int)monitors_rect[i].top,
                      (int)(monitors_rect[i].right - monitors_rect[i].left),
                      (int)(monitors_rect[i].bottom - monitors_rect[i].top));

    free(monitors_rect);
    return str;
  }
  if (iupStrEqual(name, "MONITORSCOUNT"))
  {
    int monitors_count = GetSystemMetrics(SM_CMONITORS);
    return iupStrReturnInt(monitors_count);
  }
  if (iupStrEqual(name, "TRUECOLORCANVAS"))
  {
    return iupStrReturnBoolean(iupdrvGetScreenDepth() > 8);
  }
  if (iupStrEqual(name, "UTF8MODE"))
  {
    return iupStrReturnBoolean(1);
  }
  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
    return iupStrReturnBoolean(0);
  }
  if (iupStrEqual(name, "EXEFILENAME"))
  {
    wchar_t filename[10240];
    GetModuleFileNameW(NULL, filename, 10240);
    winrt::hstring hs(filename);
    return iupwinuiHStringToString(hs);
  }
  if (iupStrEqual(name, "DARKMODE"))
  {
    return iupStrReturnBoolean(iupwinuiIsSystemDarkMode());
  }
  if (iupStrEqual(name, "SHOWMENUIMAGES"))
  {
    return iupStrReturnBoolean(1);
  }
  if (iupStrEqual(name, "LASTERROR"))
  {
    DWORD error = GetLastError();
    if (error)
    {
      LPWSTR lpMsgBuf = NULL;
      FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                     NULL, error, 0, (LPWSTR)&lpMsgBuf, 0, NULL);
      if (lpMsgBuf)
      {
        winrt::hstring hs(lpMsgBuf);
        char* str = iupwinuiHStringToString(hs);
        LocalFree(lpMsgBuf);
        return str;
      }
      return (char*)"Unknown Error";
    }
  }
  return NULL;
}
