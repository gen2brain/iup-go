/** \file
 * \brief Windows System Tray Driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <shellapi.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_class.h"
#include "iup_tray.h"

#include "iupwin_drv.h"
#include "iupwin_str.h"

#define IUPWIN_TRAY_NOTIFICATION 102

typedef struct _IupWinTray {
  HWND hwnd;
  Ihandle* ih;
  int visible;
} IupWinTray;

static LRESULT CALLBACK winTrayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  if (msg == WM_USER + IUPWIN_TRAY_NOTIFICATION)
  {
    Ihandle* ih = (Ihandle*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (ih)
    {
      int dclick = 0;
      int button = 0;
      int pressed = 0;

      switch (lp)
      {
        case WM_LBUTTONDOWN:
          pressed = 1;
          button = 1;
          break;
        case WM_MBUTTONDOWN:
          pressed = 1;
          button = 2;
          break;
        case WM_RBUTTONDOWN:
          pressed = 1;
          button = 3;
          break;
        case WM_LBUTTONDBLCLK:
          dclick = 1;
          button = 1;
          break;
        case WM_MBUTTONDBLCLK:
          dclick = 1;
          button = 2;
          break;
        case WM_RBUTTONDBLCLK:
          dclick = 1;
          button = 3;
          break;
        case WM_LBUTTONUP:
          button = 1;
          break;
        case WM_MBUTTONUP:
          button = 2;
          break;
        case WM_RBUTTONUP:
          button = 3;
          break;
      }

      if (button != 0)
      {
        IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");
        int ret = IUP_DEFAULT;

        if (cb)
          ret = cb(ih, button, pressed, dclick);

        if (ret == IUP_CLOSE)
          IupExitLoop();

        /* Show popup menu on right-click press if menu is set and callback didn't handle it */
        if (button == 3 && pressed && ret != IUP_IGNORE)
        {
          Ihandle* menu = (Ihandle*)iupAttribGet(ih, "_IUPWIN_TRAYMENU");
          if (menu)
          {
            int x, y;
            SetForegroundWindow(hwnd);
            IupGetIntInt(NULL, "CURSORPOS", &x, &y);
            IupPopup(menu, x, y);
            PostMessage(hwnd, WM_NULL, 0, 0);
          }
        }
      }
    }
    return 0;
  }

  return DefWindowProc(hwnd, msg, wp, lp);
}

static void winTrayMessage(HWND hwnd, DWORD dwMessage, HICON hIcon, const char* value)
{
  NOTIFYICONDATA tnd;
  memset(&tnd, 0, sizeof(NOTIFYICONDATA));

  tnd.cbSize = sizeof(NOTIFYICONDATA);
  tnd.hWnd = hwnd;
  tnd.uID = 1000;

  if (dwMessage == NIM_ADD)
  {
    tnd.uFlags = NIF_MESSAGE;
    tnd.uCallbackMessage = WM_USER + IUPWIN_TRAY_NOTIFICATION;
  }
  else if (dwMessage == NIM_MODIFY)
  {
    if (hIcon)
    {
      tnd.uFlags |= NIF_ICON;
      tnd.hIcon = hIcon;
    }

    if (value)
    {
      tnd.uFlags |= NIF_TIP;
      iupwinStrCopy(tnd.szTip, value, sizeof(tnd.szTip));
    }
  }

  Shell_NotifyIcon(dwMessage, &tnd);
}

static IupWinTray* winGetTray(Ihandle* ih, int create)
{
  IupWinTray* tray = (IupWinTray*)iupAttribGet(ih, "_IUPWIN_TRAY");

  if (!tray && create)
  {
    static ATOM wc_atom = 0;
    HINSTANCE hInstance = iupwin_hinstance;

    if (wc_atom == 0)
    {
      WNDCLASS wc;
      memset(&wc, 0, sizeof(WNDCLASS));
      wc.lpfnWndProc = winTrayWndProc;
      wc.hInstance = hInstance;
      wc.lpszClassName = TEXT("IupTrayWindow");
      wc_atom = RegisterClass(&wc);
    }

    tray = (IupWinTray*)calloc(1, sizeof(IupWinTray));
    tray->ih = ih;
    tray->visible = 0;

    tray->hwnd = CreateWindow(TEXT("IupTrayWindow"), TEXT(""), 0,
                              0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

    if (tray->hwnd)
    {
      SetWindowLongPtr(tray->hwnd, GWLP_USERDATA, (LONG_PTR)ih);
      iupAttribSet(ih, "_IUPWIN_TRAY", (char*)tray);
    }
    else
    {
      free(tray);
      return NULL;
    }
  }

  return tray;
}

/******************************************************************************/
/* Driver Interface Implementation                                            */
/******************************************************************************/

void iupdrvTrayInitClass(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "TIPBALLOON", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPBALLOONTITLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPBALLOONTITLEICON", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPDELAY", NULL, NULL, IUPAF_SAMEASSYSTEM, "10000", IUPAF_NO_INHERIT);
}

int iupdrvTraySetVisible(Ihandle* ih, int visible)
{
  IupWinTray* tray = winGetTray(ih, 1);
  if (!tray)
    return 0;

  if (tray->visible)
  {
    if (!visible)
    {
      winTrayMessage(tray->hwnd, NIM_DELETE, NULL, NULL);
      tray->visible = 0;
    }
  }
  else
  {
    if (visible)
    {
      char* image;
      char* tip;
      HICON hIcon = NULL;

      winTrayMessage(tray->hwnd, NIM_ADD, NULL, NULL);
      tray->visible = 1;

      image = iupAttribGet(ih, "_IUPWIN_TRAYIMAGE");
      if (image)
      {
        hIcon = (HICON)iupImageGetIcon(image);
        if (hIcon)
          winTrayMessage(tray->hwnd, NIM_MODIFY, hIcon, NULL);
      }

      tip = iupAttribGet(ih, "_IUPWIN_TRAYTIP");
      if (tip)
        winTrayMessage(tray->hwnd, NIM_MODIFY, NULL, tip);
    }
  }

  return 1;
}

int iupdrvTraySetTip(Ihandle* ih, const char* value)
{
  IupWinTray* tray = winGetTray(ih, 1);

  /* Store the tip for later use when tray becomes visible */
  iupAttribSetStr(ih, "_IUPWIN_TRAYTIP", value);

  if (!tray || !tray->visible)
    return 0;

  winTrayMessage(tray->hwnd, NIM_MODIFY, NULL, value);
  return 1;
}

int iupdrvTraySetImage(Ihandle* ih, const char* value)
{
  IupWinTray* tray = winGetTray(ih, 1);
  HICON hIcon;

  /* Store the image name for later use when tray becomes visible */
  iupAttribSetStr(ih, "_IUPWIN_TRAYIMAGE", value);

  if (!tray || !tray->visible)
    return 0;

  hIcon = (HICON)iupImageGetIcon(value);
  if (hIcon)
    winTrayMessage(tray->hwnd, NIM_MODIFY, hIcon, NULL);

  return 1;
}

int iupdrvTraySetMenu(Ihandle* ih, Ihandle* menu)
{
  /* Store the menu handle, it will be shown via IupPopup on right-click */
  iupAttribSet(ih, "_IUPWIN_TRAYMENU", (char*)menu);
  return 1;
}

void iupdrvTrayDestroy(Ihandle* ih)
{
  IupWinTray* tray = (IupWinTray*)iupAttribGet(ih, "_IUPWIN_TRAY");

  if (tray)
  {
    if (tray->visible)
      winTrayMessage(tray->hwnd, NIM_DELETE, NULL, NULL);

    if (tray->hwnd)
      DestroyWindow(tray->hwnd);

    free(tray);
    iupAttribSet(ih, "_IUPWIN_TRAY", NULL);
  }
}

int iupdrvTrayIsAvailable(void)
{
  return 1;
}
