/** \file
 * \brief Windows Desktop Notifications Driver
 *
 * This implementation is standalone and can be used with any IUP backend
 * on Windows (native Win32, Qt, GTK).
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
#include "iup_class.h"
#include "iup_notify.h"
#include "iup_image.h"
#include "iup_tray.h"

#define WM_IUPNOTIFY (WM_APP + 100)
#define NOTIFY_ICON_ID 1
#define IUPWIN_NOTIFY_MAX_ICON_SIZE 256

typedef struct _IupWinNotify {
  Ihandle* ih;
  HWND hwnd;
  NOTIFYICONDATAW nid;
  HICON hBalloonIcon;
  int balloon_icon_size;
  int icon_added;
  int destroying;
} IupWinNotify;

static HICON winNotifyCreateIconFromPixels(int width, int height, unsigned char* pixels)
{
  BITMAPV5HEADER bi;
  HDC hdc;
  HBITMAP hBitmap, hMonoBitmap;
  HICON hIcon = NULL;
  ICONINFO ii;
  unsigned char* destPixels;
  int x, y;

  ZeroMemory(&bi, sizeof(bi));
  bi.bV5Size = sizeof(bi);
  bi.bV5Width = width;
  bi.bV5Height = -height;
  bi.bV5Planes = 1;
  bi.bV5BitCount = 32;
  bi.bV5Compression = BI_BITFIELDS;
  bi.bV5RedMask = 0x00FF0000;
  bi.bV5GreenMask = 0x0000FF00;
  bi.bV5BlueMask = 0x000000FF;
  bi.bV5AlphaMask = 0xFF000000;

  hdc = GetDC(NULL);
  hBitmap = CreateDIBSection(hdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&destPixels, NULL, 0);
  ReleaseDC(NULL, hdc);

  if (!hBitmap)
    return NULL;

  for (y = 0; y < height; y++)
  {
    for (x = 0; x < width; x++)
    {
      int srcOffset = (y * width + x) * 4;
      int dstOffset = (y * width + x) * 4;
      unsigned char a = pixels[srcOffset + 0];
      unsigned char r = pixels[srcOffset + 1];
      unsigned char g = pixels[srcOffset + 2];
      unsigned char b = pixels[srcOffset + 3];

      destPixels[dstOffset + 0] = b;
      destPixels[dstOffset + 1] = g;
      destPixels[dstOffset + 2] = r;
      destPixels[dstOffset + 3] = a;
    }
  }

  hMonoBitmap = CreateBitmap(width, height, 1, 1, NULL);
  if (hMonoBitmap)
  {
    ii.fIcon = TRUE;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmMask = hMonoBitmap;
    ii.hbmColor = hBitmap;
    hIcon = CreateIconIndirect(&ii);
    DeleteObject(hMonoBitmap);
  }

  DeleteObject(hBitmap);
  return hIcon;
}

static HICON winNotifyGetIcon(Ihandle* ih, const char* icon_name, int* out_size)
{
  HICON hIcon = NULL;
  int width, height;
  unsigned char* pixels;

  if (out_size)
    *out_size = 0;

  if (!icon_name || !icon_name[0])
    return NULL;

  if (iupdrvGetIconPixels(ih, icon_name, &width, &height, &pixels))
  {
    unsigned char* use_pixels = pixels;
    int use_width = width;
    int use_height = height;
    unsigned char* scaled_pixels = NULL;

    if (width > IUPWIN_NOTIFY_MAX_ICON_SIZE || height > IUPWIN_NOTIFY_MAX_ICON_SIZE)
    {
      double scale;
      if (width > height)
        scale = (double)IUPWIN_NOTIFY_MAX_ICON_SIZE / width;
      else
        scale = (double)IUPWIN_NOTIFY_MAX_ICON_SIZE / height;

      use_width = (int)(width * scale);
      use_height = (int)(height * scale);
      if (use_width < 1) use_width = 1;
      if (use_height < 1) use_height = 1;

      scaled_pixels = (unsigned char*)malloc(use_width * use_height * 4);
      if (scaled_pixels)
      {
        iupImageResizeRGBA(width, height, pixels, use_width, use_height, scaled_pixels, 4);
        use_pixels = scaled_pixels;
      }
      else
      {
        use_width = width;
        use_height = height;
      }
    }

    hIcon = winNotifyCreateIconFromPixels(use_width, use_height, use_pixels);

    if (hIcon && out_size)
      *out_size = (use_width > use_height) ? use_width : use_height;

    if (scaled_pixels)
      free(scaled_pixels);
    free(pixels);
  }

  return hIcon;
}

static LRESULT CALLBACK iupwinNotifyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_IUPNOTIFY)
  {
    Ihandle* ih = (Ihandle*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (ih)
    {
      IupWinNotify* notify = (IupWinNotify*)iupAttribGet(ih, "_IUPWIN_NOTIFY");
      if (notify && !notify->destroying && notify->icon_added)
      {
        switch (lParam)
        {
          case NIN_BALLOONUSERCLICK:
          {
            IFni notify_cb = (IFni)IupGetCallback(ih, "NOTIFY_CB");
            if (notify_cb)
            {
              int ret = notify_cb(ih, 0);
              if (ret == IUP_CLOSE)
                IupExitLoop();
            }
            break;
          }

          case NIN_BALLOONTIMEOUT:
          {
            IFni close_cb = (IFni)IupGetCallback(ih, "CLOSE_CB");
            if (close_cb)
            {
              int ret = close_cb(ih, 1);
              if (ret == IUP_CLOSE)
                IupExitLoop();
            }
            break;
          }

          case NIN_BALLOONHIDE:
          {
            IFni close_cb = (IFni)IupGetCallback(ih, "CLOSE_CB");
            if (close_cb)
            {
              int ret = close_cb(ih, 2);
              if (ret == IUP_CLOSE)
                IupExitLoop();
            }
            break;
          }
        }
      }
    }
    return 0;
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

static IupWinNotify* iupwinNotifyCreate(Ihandle* ih)
{
  IupWinNotify* notify;
  WNDCLASSEXW wc;
  static int registered = 0;

  if (!registered)
  {
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = iupwinNotifyWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"IupNotifyClass";
    RegisterClassExW(&wc);
    registered = 1;
  }

  notify = (IupWinNotify*)calloc(1, sizeof(IupWinNotify));
  notify->ih = ih;
  notify->icon_added = 0;

  notify->hwnd = CreateWindowExW(0, L"IupNotifyClass", L"IupNotify", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);

  if (!notify->hwnd)
  {
    free(notify);
    return NULL;
  }

  SetWindowLongPtr(notify->hwnd, GWLP_USERDATA, (LONG_PTR)ih);

  ZeroMemory(&notify->nid, sizeof(notify->nid));
  notify->nid.cbSize = sizeof(notify->nid);
  notify->nid.hWnd = notify->hwnd;
  notify->nid.uID = NOTIFY_ICON_ID;
  notify->nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_STATE;
  notify->nid.uCallbackMessage = WM_IUPNOTIFY;
  notify->nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  notify->nid.dwState = NIS_HIDDEN;
  notify->nid.dwStateMask = NIS_HIDDEN;
  notify->nid.uVersion = NOTIFYICON_VERSION_4;

  iupAttribSet(ih, "_IUPWIN_NOTIFY", (char*)notify);

  return notify;
}

static void iupwinNotifyDestroy(IupWinNotify* notify)
{
  if (!notify)
    return;

  notify->destroying = 1;
  notify->ih = NULL;

  if (notify->hwnd)
    SetWindowLongPtr(notify->hwnd, GWLP_USERDATA, 0);

  if (notify->icon_added)
  {
    Shell_NotifyIconW(NIM_DELETE, &notify->nid);
    notify->icon_added = 0;
  }

  if (notify->hBalloonIcon)
  {
    DestroyIcon(notify->hBalloonIcon);
    notify->hBalloonIcon = NULL;
  }

  if (notify->hwnd)
  {
    DestroyWindow(notify->hwnd);
    notify->hwnd = NULL;
  }

  free(notify);
}

int iupdrvNotifyShow(Ihandle* ih)
{
  IupWinNotify* notify = (IupWinNotify*)iupAttribGet(ih, "_IUPWIN_NOTIFY");
  const char* title;
  const char* body;
  const char* icon;
  int silent;
  DWORD info_flags;

  if (!notify)
  {
    notify = iupwinNotifyCreate(ih);
    if (!notify)
    {
      IFns error_cb = (IFns)IupGetCallback(ih, "ERROR_CB");
      if (error_cb)
        error_cb(ih, "Failed to create notification window");
      return 0;
    }
  }

  title = IupGetAttribute(ih, "TITLE");
  body = IupGetAttribute(ih, "BODY");
  icon = IupGetAttribute(ih, "ICON");
  silent = IupGetInt(ih, "SILENT");

  if (!notify->icon_added)
  {
    if (!Shell_NotifyIconW(NIM_ADD, &notify->nid))
    {
      IFns error_cb = (IFns)IupGetCallback(ih, "ERROR_CB");
      if (error_cb)
        error_cb(ih, "Failed to add notification icon");
      return 0;
    }
    Shell_NotifyIconW(NIM_SETVERSION, &notify->nid);
    notify->icon_added = 1;
  }

  if (notify->hBalloonIcon)
  {
    DestroyIcon(notify->hBalloonIcon);
    notify->hBalloonIcon = NULL;
  }

  if (icon && icon[0])
    notify->hBalloonIcon = winNotifyGetIcon(ih, icon, &notify->balloon_icon_size);

  notify->nid.uFlags = NIF_INFO;

  if (title)
  {
    MultiByteToWideChar(CP_UTF8, 0, title, -1, notify->nid.szInfoTitle, 64);
    notify->nid.szInfoTitle[63] = L'\0';
  }
  else
  {
    notify->nid.szInfoTitle[0] = L'\0';
  }

  if (body)
  {
    MultiByteToWideChar(CP_UTF8, 0, body, -1, notify->nid.szInfo, 256);
    notify->nid.szInfo[255] = L'\0';
  }
  else
  {
    notify->nid.szInfo[0] = L'\0';
  }

  if (notify->hBalloonIcon)
  {
    info_flags = NIIF_USER;
    if (notify->balloon_icon_size >= 32)
      info_flags |= NIIF_LARGE_ICON;
    notify->nid.hBalloonIcon = notify->hBalloonIcon;
  }
  else
  {
    info_flags = NIIF_INFO;
  }

  if (silent)
    info_flags |= NIIF_NOSOUND;

  notify->nid.dwInfoFlags = info_flags;

  if (!Shell_NotifyIconW(NIM_MODIFY, &notify->nid))
  {
    IFns error_cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (error_cb)
      error_cb(ih, "Failed to show notification");
    return 0;
  }

  return 1;
}

int iupdrvNotifyClose(Ihandle* ih)
{
  IupWinNotify* notify = (IupWinNotify*)iupAttribGet(ih, "_IUPWIN_NOTIFY");

  if (!notify || !notify->icon_added)
    return 0;

  notify->nid.uFlags = NIF_INFO;
  notify->nid.szInfo[0] = L'\0';
  notify->nid.szInfoTitle[0] = L'\0';

  Shell_NotifyIconW(NIM_MODIFY, &notify->nid);

  return 1;
}

void iupdrvNotifyDestroy(Ihandle* ih)
{
  IupWinNotify* notify = (IupWinNotify*)iupAttribGet(ih, "_IUPWIN_NOTIFY");

  if (notify)
  {
    iupwinNotifyDestroy(notify);
    iupAttribSet(ih, "_IUPWIN_NOTIFY", NULL);
  }
}

int iupdrvNotifyIsAvailable(void)
{
  return 1;
}

void iupdrvNotifyInitClass(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "ACTION1", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTION2", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTION3", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTION4", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIMEOUT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
