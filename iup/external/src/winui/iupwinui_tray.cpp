/** \file
 * \brief WinUI Driver - Tray Control
 *
 * Uses Win32 Shell_NotifyIcon API.
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <shellapi.h>

#include <cstdlib>
#include <cstring>
#include <cstdio>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_class.h"
#include "iup_tray.h"
}

#include "iupwinui_drv.h"

#define IUPWINUI_TRAY_NOTIFICATION 102

typedef struct _IupWinUITray {
  HWND hwnd;
  Ihandle* ih;
  int visible;
} IupWinUITray;

static int winuiTrayGetIconRect(HWND hwnd, RECT* rect)
{
  NOTIFYICONIDENTIFIER nii;
  memset(&nii, 0, sizeof(nii));
  nii.cbSize = sizeof(nii);
  nii.hWnd = hwnd;
  nii.uID = 1000;

  if (Shell_NotifyIconGetRect(&nii, rect) == S_OK)
    return 1;

  return 0;
}

static void winuiTrayGetMenuPosition(HWND hwnd, int* x, int* y)
{
  RECT icon_rect;
  APPBARDATA abd;

  if (!winuiTrayGetIconRect(hwnd, &icon_rect))
  {
    IupGetIntInt(NULL, "CURSORPOS", x, y);
    return;
  }

  memset(&abd, 0, sizeof(abd));
  abd.cbSize = sizeof(abd);

  if (SHAppBarMessage(ABM_GETTASKBARPOS, &abd))
  {
    switch (abd.uEdge)
    {
      case ABE_BOTTOM:
        *x = (icon_rect.left + icon_rect.right) / 2;
        *y = icon_rect.top;
        break;
      case ABE_TOP:
        *x = (icon_rect.left + icon_rect.right) / 2;
        *y = icon_rect.bottom;
        break;
      case ABE_LEFT:
        *x = icon_rect.right;
        *y = (icon_rect.top + icon_rect.bottom) / 2;
        break;
      case ABE_RIGHT:
        *x = icon_rect.left;
        *y = (icon_rect.top + icon_rect.bottom) / 2;
        break;
      default:
        *x = (icon_rect.left + icon_rect.right) / 2;
        *y = icon_rect.top;
        break;
    }
  }
  else
  {
    *x = (icon_rect.left + icon_rect.right) / 2;
    *y = icon_rect.top;
  }
}

static LRESULT CALLBACK winuiTrayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  if (msg == WM_USER + IUPWINUI_TRAY_NOTIFICATION)
  {
    Ihandle* ih = (Ihandle*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (ih)
    {
      int dclick = 0;
      int button = 0;
      int pressed = 0;

      switch (lp)
      {
        case WM_LBUTTONDOWN:   pressed = 1; button = 1; break;
        case WM_MBUTTONDOWN:   pressed = 1; button = 2; break;
        case WM_RBUTTONDOWN:   pressed = 1; button = 3; break;
        case WM_LBUTTONDBLCLK: dclick = 1;  button = 1; break;
        case WM_MBUTTONDBLCLK: dclick = 1;  button = 2; break;
        case WM_RBUTTONDBLCLK: dclick = 1;  button = 3; break;
        case WM_LBUTTONUP:     button = 1; break;
        case WM_MBUTTONUP:     button = 2; break;
        case WM_RBUTTONUP:     button = 3; break;
      }

      if (button != 0)
      {
        IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");
        int ret = IUP_DEFAULT;

        if (cb)
          ret = cb(ih, button, pressed, dclick);

        if (ret == IUP_CLOSE)
          IupExitLoop();

        if (button == 3 && pressed && ret != IUP_IGNORE)
        {
          Ihandle* menu = (Ihandle*)iupAttribGet(ih, "_IUPWINUI_TRAYMENU");
          if (menu)
          {
            int x, y;
            iupwinuiBringWindowToForeground(hwnd);
            winuiTrayGetMenuPosition(hwnd, &x, &y);
            IupPopup(menu, x, y);
            PostMessage(hwnd, WM_NULL, 0, 0);
          }
        }
      }
    }
    return 0;
  }

  (void)wp;
  return DefWindowProc(hwnd, msg, wp, lp);
}

static void winuiTrayMessage(HWND hwnd, DWORD dwMessage, HICON hIcon, const char* value)
{
  NOTIFYICONDATAW tnd;
  memset(&tnd, 0, sizeof(NOTIFYICONDATAW));

  tnd.cbSize = sizeof(NOTIFYICONDATAW);
  tnd.hWnd = hwnd;
  tnd.uID = 1000;

  if (dwMessage == NIM_ADD)
  {
    tnd.uFlags = NIF_MESSAGE;
    tnd.uCallbackMessage = WM_USER + IUPWINUI_TRAY_NOTIFICATION;
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
      MultiByteToWideChar(CP_UTF8, 0, value, -1, tnd.szTip, 128);
      tnd.szTip[127] = L'\0';
    }
  }

  Shell_NotifyIconW(dwMessage, &tnd);
}

static IupWinUITray* winuiGetTray(Ihandle* ih, int create)
{
  IupWinUITray* tray = (IupWinUITray*)iupAttribGet(ih, "_IUPWINUI_TRAY");

  if (!tray && create)
  {
    static ATOM wc_atom = 0;
    HINSTANCE hInstance = GetModuleHandle(NULL);

    if (wc_atom == 0)
    {
      WNDCLASSW wc;
      memset(&wc, 0, sizeof(WNDCLASSW));
      wc.lpfnWndProc = winuiTrayWndProc;
      wc.hInstance = hInstance;
      wc.lpszClassName = L"IupWinUITrayWindow";
      wc_atom = RegisterClassW(&wc);
    }

    tray = (IupWinUITray*)calloc(1, sizeof(IupWinUITray));
    tray->ih = ih;
    tray->visible = 0;

    tray->hwnd = CreateWindowW(L"IupWinUITrayWindow", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

    if (tray->hwnd)
    {
      SetWindowLongPtr(tray->hwnd, GWLP_USERDATA, (LONG_PTR)ih);
      iupAttribSet(ih, "_IUPWINUI_TRAY", (char*)tray);
    }
    else
    {
      free(tray);
      return NULL;
    }
  }

  return tray;
}

extern "C" int iupdrvTraySetVisible(Ihandle* ih, int visible)
{
  IupWinUITray* tray = winuiGetTray(ih, 1);
  if (!tray)
    return 0;

  if (tray->visible)
  {
    if (!visible)
    {
      winuiTrayMessage(tray->hwnd, NIM_DELETE, NULL, NULL);
      tray->visible = 0;
    }
  }
  else
  {
    if (visible)
    {
      HICON hIcon = NULL;
      char* image;
      char* tip;

      winuiTrayMessage(tray->hwnd, NIM_ADD, NULL, NULL);
      tray->visible = 1;

      image = iupAttribGet(ih, "_IUPWINUI_TRAYIMAGE");
      if (image)
      {
        hIcon = (HICON)iupImageGetIcon(image);
        if (hIcon)
          winuiTrayMessage(tray->hwnd, NIM_MODIFY, hIcon, NULL);
      }

      tip = iupAttribGet(ih, "_IUPWINUI_TRAYTIP");
      if (tip)
        winuiTrayMessage(tray->hwnd, NIM_MODIFY, NULL, tip);
    }
  }

  return 1;
}

extern "C" int iupdrvTraySetTip(Ihandle* ih, const char* value)
{
  IupWinUITray* tray = winuiGetTray(ih, 1);

  iupAttribSetStr(ih, "_IUPWINUI_TRAYTIP", value);

  if (!tray || !tray->visible)
    return 0;

  winuiTrayMessage(tray->hwnd, NIM_MODIFY, NULL, value);
  return 1;
}

extern "C" int iupdrvTraySetImage(Ihandle* ih, const char* value)
{
  IupWinUITray* tray = winuiGetTray(ih, 1);
  HICON hIcon = NULL;

  iupAttribSetStr(ih, "_IUPWINUI_TRAYIMAGE", value);

  if (!tray || !tray->visible)
    return 0;

  hIcon = (HICON)iupImageGetIcon(value);
  if (hIcon)
    winuiTrayMessage(tray->hwnd, NIM_MODIFY, hIcon, NULL);

  return 1;
}

extern "C" int iupdrvTraySetMenu(Ihandle* ih, Ihandle* menu)
{
  iupAttribSet(ih, "_IUPWINUI_TRAYMENU", (char*)menu);
  return 1;
}

extern "C" void iupdrvTrayDestroy(Ihandle* ih)
{
  IupWinUITray* tray = (IupWinUITray*)iupAttribGet(ih, "_IUPWINUI_TRAY");

  if (tray)
  {
    if (tray->visible)
      winuiTrayMessage(tray->hwnd, NIM_DELETE, NULL, NULL);

    if (tray->hwnd)
      DestroyWindow(tray->hwnd);

    free(tray);
    iupAttribSet(ih, "_IUPWINUI_TRAY", NULL);
  }
}

extern "C" int iupdrvTrayIsAvailable(void)
{
  return 1;
}

extern "C" int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
{
  Ihandle* image;
  unsigned char* imgdata;
  unsigned char* dst;
  int w, h, channels, bpp;
  int x, y;

  (void)ih;

  if (!value)
    return 0;

  image = IupGetHandle(value);
  if (!image)
    return 0;

  imgdata = (unsigned char*)iupAttribGet(image, "WID");
  if (!imgdata)
    return 0;

  w = image->currentwidth;
  h = image->currentheight;
  channels = iupAttribGetInt(image, "CHANNELS");
  bpp = iupAttribGetInt(image, "BPP");

  if (w <= 0 || h <= 0)
    return 0;

  dst = (unsigned char*)malloc(w * h * 4);
  if (!dst)
    return 0;

  if (bpp == 32 && channels == 4)
  {
    for (y = 0; y < h; y++)
    {
      for (x = 0; x < w; x++)
      {
        int src_offset = (y * w + x) * 4;
        int dst_offset = (y * w + x) * 4;
        unsigned char r = imgdata[src_offset + 0];
        unsigned char g = imgdata[src_offset + 1];
        unsigned char b = imgdata[src_offset + 2];
        unsigned char a = imgdata[src_offset + 3];

        dst[dst_offset + 0] = a;
        dst[dst_offset + 1] = r;
        dst[dst_offset + 2] = g;
        dst[dst_offset + 3] = b;
      }
    }
  }
  else if (bpp == 24 && channels == 3)
  {
    for (y = 0; y < h; y++)
    {
      for (x = 0; x < w; x++)
      {
        int src_offset = (y * w + x) * 3;
        int dst_offset = (y * w + x) * 4;
        unsigned char r = imgdata[src_offset + 0];
        unsigned char g = imgdata[src_offset + 1];
        unsigned char b = imgdata[src_offset + 2];

        dst[dst_offset + 0] = 255;
        dst[dst_offset + 1] = r;
        dst[dst_offset + 2] = g;
        dst[dst_offset + 3] = b;
      }
    }
  }
  else
  {
    free(dst);
    return 0;
  }

  *width = w;
  *height = h;
  *pixels = dst;
  return 1;
}

extern "C" void iupdrvTrayInitClass(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "TIPBALLOON", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPBALLOONTITLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPBALLOONTITLEICON", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPDELAY", NULL, NULL, IUPAF_SAMEASSYSTEM, "10000", IUPAF_NO_INHERIT);
}
