/** \file
 * \brief Windows System Tray Driver
 *
 * This implementation is standalone and can be used with any IUP backend on Windows (native Win32, Qt, GTK).
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

#if defined(IUP_USE_GTK3) || defined(IUP_USE_GTK4) || defined(IUP_USE_GTK2)
#define IUPWIN_TRAY_USE_EXTERNAL_IMAGE
#include <gtk/gtk.h>

#if defined(IUP_USE_GTK3) || defined(IUP_USE_GTK2)
static gboolean winTrayDeferredExitLoop(gpointer data)
{
  (void)data;
  if (gtk_main_level() > 1)
    return G_SOURCE_CONTINUE;
  IupExitLoop();
  return G_SOURCE_REMOVE;
}
#endif
#elif defined(IUP_USE_QT)
#define IUPWIN_TRAY_USE_EXTERNAL_IMAGE
#endif

#ifdef IUPWIN_TRAY_USE_EXTERNAL_IMAGE
static HICON winTrayCreateIconFromPixels(int width, int height, unsigned char* pixels)
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
#endif

#define IUPWIN_TRAY_NOTIFICATION 102

typedef struct _IupWinTray {
  HWND hwnd;
  Ihandle* ih;
  int visible;
} IupWinTray;

static int winTrayGetIconRect(HWND hwnd, RECT* rect)
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

static void winTrayGetMenuPosition(HWND hwnd, int* x, int* y)
{
  RECT icon_rect;
  APPBARDATA abd;

  if (!winTrayGetIconRect(hwnd, &icon_rect))
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
            winTrayGetMenuPosition(hwnd, &x, &y);
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
  NOTIFYICONDATAW tnd;
  memset(&tnd, 0, sizeof(NOTIFYICONDATAW));

  tnd.cbSize = sizeof(NOTIFYICONDATAW);
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
      MultiByteToWideChar(CP_UTF8, 0, value, -1, tnd.szTip, 128);
      tnd.szTip[127] = L'\0';
    }
  }

  Shell_NotifyIconW(dwMessage, &tnd);
}

static IupWinTray* winGetTray(Ihandle* ih, int create)
{
  IupWinTray* tray = (IupWinTray*)iupAttribGet(ih, "_IUPWIN_TRAY");

  if (!tray && create)
  {
    static ATOM wc_atom = 0;
    HINSTANCE hInstance = GetModuleHandle(NULL);

    if (wc_atom == 0)
    {
      WNDCLASSW wc;
      memset(&wc, 0, sizeof(WNDCLASSW));
      wc.lpfnWndProc = winTrayWndProc;
      wc.hInstance = hInstance;
      wc.lpszClassName = L"IupTrayWindow";
      wc_atom = RegisterClassW(&wc);
    }

    tray = (IupWinTray*)calloc(1, sizeof(IupWinTray));
    tray->ih = ih;
    tray->visible = 0;

    tray->hwnd = CreateWindowW(L"IupTrayWindow", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

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
#ifdef IUPWIN_TRAY_USE_EXTERNAL_IMAGE
        int width, height;
        unsigned char* pixels;
        if (iupdrvGetIconPixels(ih, image, &width, &height, &pixels))
        {
          hIcon = winTrayCreateIconFromPixels(width, height, pixels);
          free(pixels);
        }
#else
        hIcon = (HICON)iupImageGetIcon(image);
#endif
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
  HICON hIcon = NULL;

  /* Store the image name for later use when tray becomes visible */
  iupAttribSetStr(ih, "_IUPWIN_TRAYIMAGE", value);

  if (!tray || !tray->visible)
    return 0;

#ifdef IUPWIN_TRAY_USE_EXTERNAL_IMAGE
  {
    int width, height;
    unsigned char* pixels;
    if (iupdrvGetIconPixels(ih, value, &width, &height, &pixels))
    {
      hIcon = winTrayCreateIconFromPixels(width, height, pixels);
      free(pixels);
    }
  }
#else
  hIcon = (HICON)iupImageGetIcon(value);
#endif

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

#if defined(IUP_USE_GTK3) || defined(IUP_USE_GTK2)
    g_idle_add(winTrayDeferredExitLoop, NULL);
#endif
  }
}

int iupdrvTrayIsAvailable(void)
{
  return 1;
}

#ifndef IUPWIN_TRAY_USE_EXTERNAL_IMAGE
int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
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
#endif

void iupdrvTrayInitClass(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "TIPBALLOON", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPBALLOONTITLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPBALLOONTITLEICON", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPDELAY", NULL, NULL, IUPAF_SAMEASSYSTEM, "10000", IUPAF_NO_INHERIT);
}
