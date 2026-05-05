/** \file
 * \brief WinUI Driver - Global Attributes
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <windowsx.h>
#include <cstdio>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_key.h"
#include "iup_singleinstance.h"
}

#include "iupwinui_drv.h"

static int winui_monitor_index = 0;
static HHOOK winui_OldGetMessageHook = NULL;

static LRESULT CALLBACK winuiHookGetMessageProc(int hcode, WPARAM gm_wp, LPARAM gm_lp)
{
  MSG* gm_msg = (MSG*)gm_lp;
  UINT msg = gm_msg->message;
  WPARAM wp = gm_msg->wParam;
  LPARAM lp = gm_msg->lParam;
  POINT pt = gm_msg->pt;
  static int last_button = 0;
  static int last_pressed = 0;
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

  if (hcode != HC_ACTION)
    return CallNextHookEx(winui_OldGetMessageHook, hcode, gm_wp, gm_lp);

  switch (msg)
  {
  case WM_MOUSEWHEEL:
    {
      IFfiis cb = (IFfiis)IupGetFunction("GLOBALWHEEL_CB");
      if (cb)
      {
        short delta = (short)HIWORD(wp);
        iupwinuiButtonKeySetStatus(LOWORD(wp), 0, status, 0);
        cb((float)delta / 120.0f, GET_X_LPARAM(lp), GET_Y_LPARAM(lp), status);
      }
      break;
    }
  case WM_LBUTTONDBLCLK:
  case WM_MBUTTONDBLCLK:
  case WM_RBUTTONDBLCLK:
  case WM_XBUTTONDBLCLK:
  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
  case WM_XBUTTONDOWN:
    {
      int doubleclick = 0, button = 0;
      IFiiiis cb = (IFiiiis)IupGetFunction("GLOBALBUTTON_CB");
      if (!cb) break;

      if (msg == WM_LBUTTONDBLCLK || msg == WM_MBUTTONDBLCLK ||
          msg == WM_RBUTTONDBLCLK || msg == WM_XBUTTONDBLCLK)
        doubleclick = 1;

      iupwinuiButtonKeySetStatus(LOWORD(wp), 0, status, doubleclick);

      if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) button = IUP_BUTTON1;
      else if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) button = IUP_BUTTON2;
      else if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) button = IUP_BUTTON3;
      else
        button = (HIWORD(wp) == XBUTTON1) ? IUP_BUTTON4 : IUP_BUTTON5;

      if (last_button == button && last_pressed == 1) break;
      cb(button, 1, pt.x, pt.y, status);
      last_button = button;
      last_pressed = 1;
      break;
    }
  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
  case WM_XBUTTONUP:
    {
      int button = 0;
      IFiiiis cb = (IFiiiis)IupGetFunction("GLOBALBUTTON_CB");
      if (!cb) break;

      iupwinuiButtonKeySetStatus(LOWORD(wp), 0, status, 0);

      if (msg == WM_LBUTTONUP) button = IUP_BUTTON1;
      else if (msg == WM_MBUTTONUP) button = IUP_BUTTON2;
      else if (msg == WM_RBUTTONUP) button = IUP_BUTTON3;
      else
        button = (HIWORD(wp) == XBUTTON1) ? IUP_BUTTON4 : IUP_BUTTON5;

      if (last_button == button && last_pressed == 0) break;
      cb(button, 0, pt.x, pt.y, status);
      last_button = button;
      last_pressed = 0;
      break;
    }
  case WM_MOUSEMOVE:
    {
      IFiis cb = (IFiis)IupGetFunction("GLOBALMOTION_CB");
      if (cb)
      {
        iupwinuiButtonKeySetStatus(LOWORD(wp), 0, status, 0);
        cb(pt.x, pt.y, status);
      }
      break;
    }
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
  case WM_KEYUP:
  case WM_SYSKEYUP:
    {
      IFii cb = (IFii)IupGetFunction("GLOBALKEYPRESS_CB");
      if (cb)
      {
        int pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) ? 1 : 0;
        int code = iupwinuiKeyDecode((int)wp);
        if (code != 0) cb(code, pressed);
      }
      break;
    }
  default:
    break;
  }

  return CallNextHookEx(winui_OldGetMessageHook, hcode, gm_wp, gm_lp);
}

static BOOL CALLBACK winuiMonitorInfoEnum(HMONITOR handle, HDC handle_dc, LPRECT rect, LPARAM data)
{
  RECT* monitors_rect = (RECT*)data;
  monitors_rect[winui_monitor_index] = *rect;
  winui_monitor_index++;
  (void)handle_dc;
  (void)handle;
  return TRUE;
}

extern "C" IUP_SDK_API int iupdrvSetGlobal(const char* name, const char* value)
{
  if (iupStrEqual(name, "SINGLEINSTANCE"))
  {
    if (iupdrvSingleInstanceSet(value))
      return 0;
    else
      return 1;
  }
  if (iupStrEqual(name, "INPUTCALLBACKS"))
  {
    if (iupStrBoolean(value))
    {
      if (!winui_OldGetMessageHook)
        winui_OldGetMessageHook = SetWindowsHookEx(WH_GETMESSAGE, winuiHookGetMessageProc,NULL, GetCurrentThreadId());
    }
    else
    {
      if (winui_OldGetMessageHook)
      {
        UnhookWindowsHookEx(winui_OldGetMessageHook);
        winui_OldGetMessageHook = NULL;
      }
    }
    return 1;
  }
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

extern "C" IUP_SDK_API char* iupdrvGetGlobal(const char* name)
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
      pstr += snprintf(pstr, (monitors_count * 50) - (int)(pstr - str), "%d %d %d %d\n", (int)monitors_rect[i].left, (int)monitors_rect[i].top,
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
