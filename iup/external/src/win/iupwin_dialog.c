/** \file
 * \brief IupDialog class
 *
 * See Copyright Notice in "iup.h"
 */
#include <windows.h>
#include <commdlg.h>
#undef NTDDI_VERSION 
#define NTDDI_VERSION NTDDI_VISTA
#include <uxtheme.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_dlglist.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_focus.h"
#include "iup_str.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"
#include "iup_image.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_brush.h"
#include "iupwin_info.h"
#include "iupwin_str.h"


#ifndef SM_CXPADDEDBORDER
#define SM_CXPADDEDBORDER     92
#endif

#ifndef WM_DPICHANGED
#define WM_DPICHANGED       0x02E0
#endif


#define IUPWIN_TRAY_NOTIFICATION 102

static int WM_HELPMSG;

static int winDialogSetBgColorAttrib(Ihandle* ih, const char* value);
static int winDialogSetTrayAttrib(Ihandle *ih, const char *value);

/****************************************************************
                     ITaskbarList3 resources
****************************************************************/
#ifndef __MINGW32__
#include "Shobjidl.h"

#ifdef __ITaskbarList3_FWD_DEFINED__  /* Only available since VC10 */

static int winDialogSetTaskBarProgressValueAttrib(Ihandle *ih, const char *value)
{
  ITaskbarList3* tbl = (ITaskbarList3*)iupAttribGet(ih, "_IUPWIN_TASKBARLIST");
  if(tbl)
  {
    int perc;
    iupStrToInt(value, &perc);
    tbl->lpVtbl->SetProgressValue(tbl, ih->handle, perc, 100);

    if (perc == 100)
      tbl->lpVtbl->SetProgressState(tbl, ih->handle, TBPF_NOPROGRESS);
  }

  return 0;
}

static int winDialogSetTaskBarProgressStateAttrib(Ihandle *ih, const char *value)
{
  ITaskbarList3* tbl = (ITaskbarList3*)iupAttribGet(ih, "_IUPWIN_TASKBARLIST");
  if(tbl)
  {
    if(iupStrEqualNoCase(value, "NOPROGRESS"))
      tbl->lpVtbl->SetProgressState(tbl, ih->handle, TBPF_NOPROGRESS);
    else if(iupStrEqualNoCase(value, "INDETERMINATE"))
      tbl->lpVtbl->SetProgressState(tbl, ih->handle, TBPF_INDETERMINATE);
    else if(iupStrEqualNoCase(value, "ERROR"))
      tbl->lpVtbl->SetProgressState(tbl, ih->handle, TBPF_ERROR);
    else if(iupStrEqualNoCase(value, "PAUSED"))
      tbl->lpVtbl->SetProgressState(tbl, ih->handle, TBPF_PAUSED);
    else  /* NORMAL */
      tbl->lpVtbl->SetProgressState(tbl, ih->handle, TBPF_NORMAL);
  }

  return 0;
}

#endif  /* __ITaskbarList3_FWD_DEFINED__ */
#endif

/****************************************************************
                     Utilities
****************************************************************/

void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* parent)
{
  SetParent(ih->handle, parent);
}

int iupdrvDialogIsVisible(Ihandle* ih)
{
  return iupdrvIsVisible(ih);
}

void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int *w, int *h)
{
  RECT rect;
  if (!handle)
    handle = ih->handle;
  GetWindowRect(handle, &rect);
  if (w) *w = rect.right-rect.left;
  if (h) *h = rect.bottom-rect.top;
}

static void winDialogMaximizeAtParent(HWND hWnd, HWND hWndParent)
{
  MONITORINFO mi = { sizeof(MONITORINFO) };
  const LONG currStyles = GetWindowLong(hWnd, GWL_STYLE);
  GetMonitorInfo(MonitorFromWindow(hWndParent, MONITOR_DEFAULTTONEAREST), &mi);
  SetWindowLong(hWnd, GWL_STYLE, currStyles | WS_MAXIMIZE);
  SetWindowPos(hWnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, 0, 0, SWP_NOSIZE);
}

void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  if (visible && (ih->data->cmd_show == SW_MAXIMIZE || ih->data->cmd_show == SW_SHOWMAXIMIZED))
  {
    if (iupAttribGetBoolean(ih, "MAXIMIZEATPARENT"))
    {
      Ihandle* parent = IupGetAttributeHandle(ih, "PARENTDIALOG");
      if (parent && parent->handle)
        winDialogMaximizeAtParent(ih->handle, parent->handle);
    }
    else if (iupAttribGetBoolean(ih, "MAXIMIZEATDIALOG"))
    {
      Ihandle* dialog = IupGetAttributeHandle(ih, "MAXIMIZEDIALOG");
      if (dialog && dialog->handle)
        winDialogMaximizeAtParent(ih->handle, dialog->handle);
    }
  }

  ShowWindow(ih->handle, visible? ih->data->cmd_show: SW_HIDE);
}

void iupdrvDialogGetPosition(Ihandle *ih, InativeHandle* handle, int *x, int *y)
{
  RECT rect;
  if (!handle)
    handle = ih->handle;
  GetWindowRect(handle, &rect);
  if (x) *x = rect.left;
  if (y) *y = rect.top;

  iupdrvAddScreenOffset(x, y, -1);
}

void iupdrvDialogSetPosition(Ihandle *ih, int x, int y)
{
  /* Only moves the window and places it at the top of the Z order. */
  int flags = SWP_NOSIZE;
  if (iupAttribGetBoolean(ih, "SHOWNOACTIVATE"))
    flags |= SWP_NOACTIVATE;
  SetWindowPos(ih->handle, HWND_TOP, x, y, 0, 0, flags);
}

static void winDialogGetWindowDecor(Ihandle* ih, int *border, int *caption, int menu)
{
  WINDOWINFO wi;
  wi.cbSize = sizeof(WINDOWINFO);
  GetWindowInfo(ih->handle, &wi);

  *border = wi.cxWindowBorders;

  *caption = iupAttribGetInt(ih, "CUSTOMFRAMECAPTIONHEIGHT");
  if (*caption == 0)
  {
    Ihandle* ih_caption = IupGetDialogChild(ih, "CUSTOMFRAMECAPTION");
    if (ih_caption)
    {
      *caption = ih_caption->currentheight;
      return;
    }

    if (wi.rcClient.bottom == wi.rcClient.top ||
        wi.rcClient.top > wi.rcWindow.bottom ||
        wi.rcClient.bottom > wi.rcWindow.bottom ||
        (wi.rcWindow.bottom - wi.rcWindow.top) == (wi.rcClient.bottom - wi.rcClient.top))
    {
      if (wi.dwStyle & WS_CAPTION)
      {
        if (wi.dwExStyle & WS_EX_TOOLWINDOW)
          *caption = GetSystemMetrics(SM_CYSMCAPTION); /* tool window */
        else
          *caption = GetSystemMetrics(SM_CYCAPTION);   /* normal window */
      }
      else
        *caption = 0;
    }
    else
    {
      /* caption = window height - borders - client height - menu */
      *caption = (wi.rcWindow.bottom - wi.rcWindow.top) - 2 * wi.cyWindowBorders - (wi.rcClient.bottom - wi.rcClient.top) - menu;
    }
  }
}

void iupdrvDialogGetDecoration(Ihandle* ih, int *border, int *caption, int *menu)
{
  if (ih->data->menu)
    *menu = iupdrvMenuGetMenuBarSize(ih->data->menu);
  else
    *menu = 0;

  if (ih->handle)
  {
    winDialogGetWindowDecor(ih, border, caption, *menu);
  }
  else
  {
    int padded_border = 0;
    int has_titlebar = iupAttribGetBoolean(ih, "MAXBOX")  ||
                       iupAttribGetBoolean(ih, "MINBOX")  ||
                       iupAttribGetBoolean(ih, "MENUBOX") || 
                       iupAttribGet(ih, "TITLE");

    *caption = 0;
    if (has_titlebar)
    {
      *caption = iupAttribGetInt(ih, "CUSTOMFRAMECAPTIONHEIGHT");
      if (*caption == 0)
      {
        Ihandle* ih_caption = IupGetDialogChild(ih, "CUSTOMFRAMECAPTION");
        if (ih_caption)
          *caption = ih_caption->currentheight;
        else
        {
          if (iupAttribGetBoolean(ih, "TOOLBOX") && iupAttribGet(ih, "PARENTDIALOG"))
            *caption = GetSystemMetrics(SM_CYSMCAPTION); /* tool window */
          else
            *caption = GetSystemMetrics(SM_CYCAPTION);   /* normal window */
        }
      }

      padded_border = GetSystemMetrics(SM_CXPADDEDBORDER);
    }

    *border = 0;
    if (iupAttribGetBoolean(ih, "RESIZE"))
    {
      /* has_border */
      *border = GetSystemMetrics(SM_CXFRAME);     /* Thickness of the sizing border around the perimeter of a window  */
    }                                             /* that can be resized, in pixels.                                  */
    else if (has_titlebar)
    {
      /* has_border */
      *border = GetSystemMetrics(SM_CXFIXEDFRAME);  /* Thickness of the frame around the perimeter of a window        */
    }                                               /* that has a caption but is not sizable, in pixels.              */
    else if (iupAttribGetBoolean(ih, "BORDER"))
    {
      /* has_border */
      *border = GetSystemMetrics(SM_CXBORDER);
    }

    if (*border)
      *border += padded_border;
  }
}

int iupdrvDialogSetPlacement(Ihandle* ih)
{
  char* placement;
  int no_activate = iupAttribGetBoolean(ih, "SHOWNOACTIVATE");

  if (no_activate)
    ih->data->cmd_show = SW_SHOWNOACTIVATE;
  else
    ih->data->cmd_show = SW_SHOWNORMAL;
  ih->data->show_state = IUP_SHOW;

  if (iupAttribGetBoolean(ih, "FULLSCREEN"))
    return 1;

  placement = iupAttribGet(ih, "PLACEMENT");
  if (!placement)
  {
    if (IsIconic(ih->handle) || IsZoomed(ih->handle))
      ih->data->show_state = IUP_RESTORE;

    if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE") && iupDialogCustomFrameRestore(ih))
    {
      ih->data->show_state = IUP_RESTORE;
      return 1;
    }

    return 0;
  }

  if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE") && iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    iupDialogCustomFrameMaximize(ih);
    iupAttribSet(ih, "PLACEMENT", NULL); /* reset to NORMAL */
    ih->data->show_state = IUP_MAXIMIZE;
    return 1;
  }

  if (iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    if (no_activate)
      ih->data->cmd_show = SW_MAXIMIZE;
    else
      ih->data->cmd_show = SW_SHOWMAXIMIZED;
    ih->data->show_state = IUP_MAXIMIZE;
  }
  else if (iupStrEqualNoCase(placement, "MINIMIZED"))
  {
    if (no_activate)
      ih->data->cmd_show = SW_SHOWMINNOACTIVE;
    else if (iupAttribGetBoolean(ih, "SHOWMINIMIZENEXT"))
      ih->data->cmd_show = SW_MINIMIZE;
    else
      ih->data->cmd_show = SW_SHOWMINIMIZED;
    ih->data->show_state = IUP_MINIMIZE;
  }
  else if (iupStrEqualNoCase(placement, "FULL"))
  {
    int width, height, x, y;
    int caption, border, menu;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    /* the dialog will cover the task bar */
    iupdrvGetFullSize(&width, &height);

    /* position the decoration and menu outside the screen */
    x = -(border);
    y = -(border+caption+menu);

    width += 2*border;
    height += 2*border + caption + menu;

    /* set the new size and position */
    /* WM_SIZE will update the layout */
    SetWindowPos(ih->handle, HWND_TOP, x, y, width, height, 0);

    if (IsIconic(ih->handle) || IsZoomed(ih->handle))
      ih->data->show_state = IUP_RESTORE;
  }

  iupAttribSet(ih, "PLACEMENT", NULL); /* reset to NORMAL */

  return 1;
}

static void winDialogMDIRefreshMenu(Ihandle* ih)
{
  /* We manually update the menu when a MDI child is added or removed. */
  Ihandle* client = (Ihandle*)iupAttribGet(ih, "MDICLIENT_HANDLE");
  PostMessage(client->handle, WM_MDIREFRESHMENU, 0, 0);
}

static int winDialogMDICloseChildren(Ihandle* ih)
{
  Ihandle* client = (Ihandle*)iupAttribGet(ih, "MDICLIENT_HANDLE");
  if (iupObjectCheck(client))
  {
    HWND hWndChild = (HWND)SendMessage(client->handle, WM_MDIGETACTIVE, 0, 0);

    /* As long as the MDI client has a child, close it */
    while (hWndChild)
    {
      Ihandle* child = iupwinHandleGet(hWndChild); 
      if (iupObjectCheck(child) && iupAttribGetBoolean(child, "MDICHILD"))
      {
        Icallback cb = IupGetCallback(child, "CLOSE_CB");
        if (cb)
        {
          int ret = cb(child);
          if (ret == IUP_IGNORE)
            return 0;
          if (ret == IUP_CLOSE)
            IupExitLoop();
        }

        IupDestroy(child);
      }

      hWndChild = (HWND)SendMessage(client->handle, WM_MDIGETACTIVE, 0, 0);
    }
  }
  return 1;
}


/****************************************************************************
                            WindowProc
****************************************************************************/


static int winDialogCheckMinMaxInfo(Ihandle* ih, MINMAXINFO* minmax)
{
  int min_w = 1, min_h = 1;          /* MINSIZE default value */
  int max_w = 65535, max_h = 65535;  /* MAXSIZE default value */

  iupStrToIntInt(iupAttribGet(ih, "MINSIZE"), &min_w, &min_h, 'x');
  iupStrToIntInt(iupAttribGet(ih, "MAXSIZE"), &max_w, &max_h, 'x');

  minmax->ptMinTrackSize.x = min_w;
  minmax->ptMinTrackSize.y = min_h;
  minmax->ptMaxTrackSize.x = max_w;
  minmax->ptMaxTrackSize.y = max_h;

  return 1;
}

static void winDialogResize(Ihandle* ih, int width, int height)
{
  IFnii cb;

  iupdrvDialogGetSize(ih, NULL, &(ih->currentwidth), &(ih->currentheight));

  cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (!cb || cb(ih, width, height)!=IUP_IGNORE)  /* width and height here are for the client area */
  {
    ih->data->ignore_resize = 1;
    IupRefresh(ih);
    ih->data->ignore_resize = 0;
  }
}

static int winDialogGetChildPosX(Ihandle* child)
{
  int caption_x = 0;
  while (child)
  {
    caption_x += child->x;
    child = child->parent;
  }
  return caption_x;
}

static int winDialogDrawBackground(Ihandle* ih, HDC hdc, int force_bgcolor)
{
  HBITMAP hBitmap = (HBITMAP)iupAttribGet(ih, "_IUPWIN_BACKGROUND_BITMAP");
  if (hBitmap)
  {
    RECT rect;
    HBRUSH hBrush = CreatePatternBrush(hBitmap);
    GetClientRect(ih->handle, &rect);
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);
    return 1;
  }
  else
  {
    unsigned char r, g, b;
    char* color = force_bgcolor? iupAttribGetStr(ih, "BGCOLOR"): iupAttribGet(ih, "_IUPWIN_BACKGROUND_COLOR");
    if (iupStrToRGB(color, &r, &g, &b))
    {
      RECT rect;
      SetDCBrushColor(hdc, RGB(r, g, b));
      GetClientRect(ih->handle, &rect);
      FillRect(hdc, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
      return 1;
    }
  }
  return 0;
}

static LRESULT winDialogCustomFrameHitTest(Ihandle* ih, LPARAM lp)
{
  RECT rcWindow;
  LRESULT result;
  int x = GET_X_LPARAM(lp);
  int y = GET_Y_LPARAM(lp);
  int w, h;
  int border, caption;
  iupdrvScreenToClient(ih, &x, &y);

  GetWindowRect(ih->handle, &rcWindow);
  w = rcWindow.right - rcWindow.left - 1;
  h = rcWindow.bottom - rcWindow.top - 1;

  winDialogGetWindowDecor(ih, &border, &caption, 0);

  result = HTNOWHERE;

  if (x >= 0 && x < border)
  {
    if (y >= 0 && y < border)
      result = HTTOPLEFT;
    else if (y > h - border && y <= h)
      result = HTBOTTOMLEFT;
    else if (y >= border && y <= h - border)
      result = HTLEFT;
  }
  else if (x > w - border && x <= w)
  {
    if (y >= 0 && y < border)
      result = HTTOPRIGHT;
    else if (y > h - border && y <= h)
      result = HTBOTTOMRIGHT;
    else if (y >= border && y <= h - border)
      result = HTRIGHT;
  }
  else if (x >= border && x <= w - border)
  {
    if (y >= 0 && y < border)
      result = HTTOP;
    else if (y > h - border && y <= h)
      result = HTBOTTOM;
    else if (y >= border && y < caption + border)
    {
      int caption_left = 0, caption_right = 0;
      char* value = iupAttribGet(ih, "CUSTOMFRAMECAPTIONLIMITS");
      if (value) 
        iupStrToIntInt(value, &caption_left, &caption_right, ':');
      else
      {
        Ihandle* ih_caption = IupGetDialogChild(ih, "CUSTOMFRAMECAPTION");
        if (ih_caption)
        {
          int caption_x = winDialogGetChildPosX(ih_caption);
          if (x >= caption_x && x <= caption_x + ih_caption->currentwidth)
            result = HTCAPTION;

          return result;
        }
      }

      if (x >= border + caption_left && x <= w - border - caption_right)
        result = HTCAPTION;
    }
    else if (y >= caption + border && y < h - border)
      result = HTCLIENT;
  }

  return result;
}

static int winDialogCustomFrameProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  switch (msg)
  {
  case WM_CREATE:
    {
      RECT rcClient;
      GetWindowRect(ih->handle, &rcClient);

      /* Inform the application of the frame change. - Not sure if this is necessary */
      SetWindowPos(ih->handle,
                   NULL,
                   rcClient.left, rcClient.top,
                   rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
                   SWP_FRAMECHANGED);

      *result = 0;
      return 1;
    }
  case WM_NCACTIVATE:
    {
      IFni cb = (IFni)IupGetCallback(ih, "CUSTOMFRAMEACTIVATE_CB");
      if (cb)
      {
        int active = 1;
        if (wp == FALSE)
          active = 0;

        cb(ih, active);
      }

      if (!iupwin_comctl32ver6) /* visual style not active */
      {
        DefWindowProc(ih->handle, msg, wp, (LPARAM)-1);  /* use -1 to not repaint the nonclient area */

        *result = 1; /* allow the change */
        return 1;
      }

      break;
    }
  case WM_NCCALCSIZE:
  {
    if (wp == TRUE)
    {
      *result = 0;
      return 1;
    }

    break;
  }
  case WM_NCHITTEST:
  {
    *result = winDialogCustomFrameHitTest(ih, lp);

    if (*result != HTNOWHERE)
      return 1;

    break;
  }
  case WM_ERASEBKGND:
  {
    if (!iupwinIsVistaOrNew())
      InvalidateRect(ih->handle, NULL, FALSE);

    /* return non zero value */
    *result = 1;
    return 1;
  }
  case WM_PAINT:
  {
    IFn cb = (IFn)IupGetCallback(ih, "CUSTOMFRAMEDRAW_CB");

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(ih->handle, &ps);

    if (cb)
    {
      iupAttribSet(ih, "HDC_WMPAINT", (char*)hdc);
      iupAttribSetStrf(ih, "CLIPRECT", "0 0 %d %d", 0, 0, ih->currentwidth, ih->currentheight);

      cb(ih);

      iupAttribSet(ih, "CLIPRECT", NULL);
      iupAttribSet(ih, "HDC_WMPAINT", NULL);
    }
    else
      winDialogDrawBackground(ih, hdc, 1);

    EndPaint(ih->handle, &ps);

    *result = 0;
    return 1;
  }
  case WM_XBUTTONDBLCLK:
  case WM_LBUTTONDBLCLK:
  case WM_MBUTTONDBLCLK:
  case WM_RBUTTONDBLCLK:
  case WM_XBUTTONDOWN:
  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
    {
      SetCapture(ih->handle);

      iupwinFlagButtonDown(ih, msg);

      if (iupwinButtonDown(ih, msg, wp, lp))
      {
        /* refresh the cursor, it could have been changed in BUTTON_CB */
        iupwinRefreshCursor(ih);
      }

      if (msg==WM_XBUTTONDOWN || msg==WM_XBUTTONDBLCLK)
        *result = 1;
      else
        *result = 0;
      return 1;
    }
  case WM_MOUSEMOVE:
    {
      if (iupwinMouseMove(ih, msg, wp, lp))
      {
        /* refresh the cursor, it could have been changed in MOTION_CB */
        iupwinRefreshCursor(ih);
      }

      break; /* let iupwinBaseMsgProc process enter/leavewin */
    }
  case WM_XBUTTONUP:
  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
    {
      if (!iupwinFlagButtonUp(ih, msg))
      {
        *result = 0;
        return 1;
      }

      ReleaseCapture();

      if (iupwinButtonUp(ih, msg, wp, lp))
      {
        /* refresh the cursor, it could have been changed in BUTTON_CB */
        iupwinRefreshCursor(ih);
      }

      *result = 0;
      if (msg==WM_XBUTTONUP)
        *result = 1;
      return 1;
    }
  }

  return 0;
}

static int winDialogBaseProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  if (iupAttribGetBoolean(ih, "CUSTOMFRAMEDRAW") || iupAttribGetBoolean(ih, "CUSTOMFRAME"))
  {
    if (winDialogCustomFrameProc(ih, msg, wp, lp, result))
      return 1;
  }

  if (iupwinBaseContainerMsgProc(ih, msg, wp, lp, result))
    return 1;

  iupwinMenuDialogProc(ih, msg, wp, lp);

  switch (msg)
  {
  case WM_GETMINMAXINFO:
    {
      if (winDialogCheckMinMaxInfo(ih, (MINMAXINFO*)lp))
      {
        *result = 0;
        return 1;
      }
      break;
    }
  case WM_MOVE:
    {
      IFnii cb = (IFnii)IupGetCallback(ih, "MOVE_CB");
      int x, y;
      /* ignore LPARAM because they are the clientpos */
      iupdrvDialogGetPosition(ih, NULL, &x, &y);
      if (cb) cb(ih, x, y);
      break;
    }
  case WM_SIZE:
    {
      if (ih->data->ignore_resize)
        break;

      switch(wp)
      {
      case SIZE_MINIMIZED:
        {
          if (ih->data->show_state != IUP_MINIMIZE)
          {
            IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
            ih->data->show_state = IUP_MINIMIZE;
            if (show_cb && show_cb(ih, IUP_MINIMIZE) == IUP_CLOSE)
              IupExitLoop();
          }
          break;
        }
      case SIZE_MAXIMIZED:
        {
          if (ih->data->show_state != IUP_MAXIMIZE)
          {
            IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
            ih->data->show_state = IUP_MAXIMIZE;
            if (show_cb && show_cb(ih, IUP_MAXIMIZE) == IUP_CLOSE)
              IupExitLoop();
          }

          winDialogResize(ih, LOWORD(lp), HIWORD(lp));

          if (iupAttribGetBoolean(ih, "MDICHILD"))
          {
            /* WORKAROUND: when a child MDI dialog is maximized, 
               its title is displayed inside the MDI client area.
               So we force a MDI client size update */
            RECT rect;
            Ihandle* client = (Ihandle*)iupAttribGet(ih, "MDICLIENT_HANDLE");
            GetClientRect(client->handle, &rect);
            PostMessage(client->handle, WM_SIZE, (WPARAM)SIZE_RESTORED, MAKELPARAM(rect.right-rect.left, rect.bottom-rect.top));
          }
          break;
        }
      case SIZE_RESTORED:
        {
          if (ih->data->show_state == IUP_MAXIMIZE || ih->data->show_state == IUP_MINIMIZE)
          {
            IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
            ih->data->show_state = IUP_RESTORE;
            if (show_cb && show_cb(ih, IUP_RESTORE) == IUP_CLOSE)
              IupExitLoop();
          }

          winDialogResize(ih, LOWORD(lp), HIWORD(lp));
          break;
        }
      }

      if (iupAttribGetBoolean(ih, "MDIFRAME"))
      {
        /* We are going to manually position the MDI client, 
           so abort MDI frame processing. */
        *result = 0;
        return 1;
      }
      else
        break;
    }
  case WM_USER+IUPWIN_TRAY_NOTIFICATION:
    {
      int dclick  = 0;
      int button  = 0;
      int pressed = 0;

      switch (lp)
      {
      case WM_LBUTTONDOWN:
        pressed = 1;
        button  = 1;
        break;
      case WM_MBUTTONDOWN:
        pressed = 1;
        button  = 2;
        break;
      case WM_RBUTTONDOWN:
        pressed = 1;
        button  = 3;
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
        if (cb && cb(ih, button, pressed, dclick) == IUP_CLOSE)
          IupExitLoop();
      }

      break;
    }
  case WM_CLOSE:
    {
      Icallback cb = IupGetCallback(ih, "CLOSE_CB");
      if (cb)
      {
        int ret = cb(ih);
        if (ret == IUP_IGNORE)
        {
          *result = 0;
          return 1;
        }
        if (ret == IUP_CLOSE)
          IupExitLoop();
      }

      /* child mdi is automatically destroyed */
      if (iupAttribGetBoolean(ih, "MDICHILD"))
        IupDestroy(ih);
      else
      {
        if (!winDialogMDICloseChildren(ih))
        {
          *result = 0;
          return 1;
        }

        IupHide(ih); /* IUP default processing */
      }

      *result = 0;
      return 1;
    }
  case WM_COPYDATA:  
    {
      IFnsi cb = (IFnsi)IupGetCallback(ih, "COPYDATA_CB");
      if (cb)
      {
        COPYDATASTRUCT* cds = (COPYDATASTRUCT*)lp;
        char* iup_id = (char*)cds->dwData;

        /* from SetGlobal("SINGLEINSTANCE") */
        if (iup_id && iup_id[0] == 'I' &&
                      iup_id[1] == 'U' &&
                      iup_id[2] == 'P')
        {
          char* data = iupwinStrFromSystem((TCHAR*)cds->lpData);
          cb(ih, data, (int)strlen(data)+1);
        }
      }
      break; 
    }
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    {
      if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
      {
        SetCapture(ih->handle);

        if (iupwinButtonDown(ih, msg, wp, lp))
        {
          /* refresh the cursor, it could have been changed in BUTTON_CB */
          iupwinRefreshCursor(ih);
        }

        *result = 0;
        return 1;
      }
      break;
    }
    case WM_MOUSEMOVE:
    {
      if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
      {
        if (iupwinMouseMove(ih, msg, wp, lp))
        {
          /* refresh the cursor, it could have been changed in MOTION_CB */
          iupwinRefreshCursor(ih);
        }
      }
      break;
    }
    case WM_LBUTTONUP:
    {
      if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
      {
        ReleaseCapture();

        if (iupwinButtonUp(ih, msg, wp, lp))
        {
          /* refresh the cursor, it could have been changed in BUTTON_CB */
          iupwinRefreshCursor(ih);
        }

        *result = 0;
        return 1;
      }
      break;
    }
  case WM_ERASEBKGND:
    {
      if (winDialogDrawBackground(ih, (HDC)wp, 0))
      {
        /* return non zero value */
        *result = 1;
        return 1;
      }
      break;
    }
  case WM_DESTROY:
    {
      /* Since WM_CLOSE changed the Windows default processing                            */
      /* WM_DESTROY is NOT received by IupDialogs                                         */
      /* Except when they are children of other IupDialogs and the parent is destroyed.   */
      /* So we have to destroy the child dialog.                                          */
      /* The application is responsible for destroying the children before this happen.   */
      IupDestroy(ih);
      break;
    }
  case WM_DPICHANGED:
    {
      IupRefresh(ih);
      break;
    }
  }

  if (msg == (UINT)WM_HELPMSG)
  {
    Ihandle* child = NULL;
    DWORD* struct_ptr = (DWORD*)lp;
    if (*struct_ptr == sizeof(CHOOSECOLOR))
    {
      CHOOSECOLOR* choosecolor = (CHOOSECOLOR*)lp;
      child = (Ihandle*)choosecolor->lCustData;
    }
    else if (*struct_ptr == sizeof(CHOOSEFONT))
    {
      CHOOSEFONT* choosefont = (CHOOSEFONT*)lp;
      child = (Ihandle*)choosefont->lCustData;
    }

    if (child)
    {
      Icallback cb = IupGetCallback(child, "HELP_CB");
      if (cb && cb(child) == IUP_CLOSE)
        EndDialog((HWND)iupAttribGet(child, "HWND"), IDCANCEL);
    }
  }

  return 0;
}

static Ihandle* winMinMaxHandle = NULL;

static LRESULT CALLBACK winDialogWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{   
  LRESULT result;
  Ihandle *ih = iupwinHandleGet(hwnd); 
  if (!iupObjectCheck(ih))
  {
    /* the first time WM_GETMINMAXINFO is called, Ihandle is not associated yet */
    if (msg == WM_GETMINMAXINFO && winMinMaxHandle)
    {
      if (winDialogCheckMinMaxInfo(winMinMaxHandle, (MINMAXINFO*)lp))
        return 0;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
  }

  if (winDialogBaseProc(ih, msg, wp, lp, &result))
    return result;

  return DefWindowProc(hwnd, msg, wp, lp);
}

static LRESULT CALLBACK winDialogMDIChildWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{   
  LRESULT result;
  Ihandle *ih = iupwinHandleGet(hwnd); 
  if (!iupObjectCheck(ih))
  {
    /* the first time WM_GETMINMAXINFO is called, Ihandle is not associated yet */
    if (msg == WM_GETMINMAXINFO && winMinMaxHandle)
    {
      if (winDialogCheckMinMaxInfo(winMinMaxHandle, (MINMAXINFO*)lp))
        return 0;
    }

    return DefMDIChildProc(hwnd, msg, wp, lp);
  }

  if (msg == WM_MDIACTIVATE)
  {
    HWND hNewActive = (HWND)lp;
    if (hNewActive == ih->handle)
    {
      Icallback cb = (Icallback)IupGetCallback(ih, "MDIACTIVATE_CB");
      if (cb) cb(ih);
    }
  }

  if (winDialogBaseProc(ih, msg, wp, lp, &result))
    return result;

  return DefMDIChildProc(hwnd, msg, wp, lp);
}

static LRESULT CALLBACK winDialogMDIFrameWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{   
  LRESULT result;
  HWND hWndClient = NULL;
  Ihandle *ih = iupwinHandleGet(hwnd); 
  if (!iupObjectCheck(ih))
  {
    /* the first time WM_GETMINMAXINFO is called, Ihandle is not associated yet */
    if (msg == WM_GETMINMAXINFO && winMinMaxHandle)
    {
      if (winDialogCheckMinMaxInfo(winMinMaxHandle, (MINMAXINFO*)lp))
        return 0;
    }

    return DefFrameProc(hwnd, hWndClient, msg, wp, lp);
  }

  {
    Ihandle* client = (Ihandle*)iupAttribGet(ih, "MDICLIENT_HANDLE");
    if (client) hWndClient = client->handle;
  }

  if (winDialogBaseProc(ih, msg, wp, lp, &result))
    return result;

  if (msg == WM_MENUCOMMAND)
  {
    int menuId = GetMenuItemID((HMENU)lp, (int)wp);
    if (menuId >= IUP_MDI_FIRSTCHILD && hWndClient)
    {
      /* we manually activate the MDI child when its menu item is selected. */
      HWND hChild = GetDlgItem(hWndClient, menuId);
      if (hChild)
        SendMessage(hWndClient, WM_MDIACTIVATE, (WPARAM)hChild, 0);
    }
    else if (menuId >= SC_SIZE && menuId <= SC_CONTEXTHELP)
    {
      /* we manually forward the message to the MDI child */
      HWND hChild = (HWND)SendMessage(hWndClient, WM_MDIGETACTIVE, 0, 0);
      if (hChild)
        SendMessage(hChild, WM_SYSCOMMAND, (WPARAM)menuId, 0);
    }
  }

  return DefFrameProc(hwnd, hWndClient, msg, wp, lp);
}

enum { IUPWIN_DIALOG, IUPWIN_DIALOGCONTROL, IUPWIN_MDIFRAME, IUPWIN_MDICHILD, IUPWIN_DIALOG_SAVEBITS };

static void winDialogRegisterClass(int type)
{
  TCHAR* name;
  WNDCLASS wndclass;
  WNDPROC wndProc;
  ZeroMemory(&wndclass, sizeof(WNDCLASS));
  
  if (type == IUPWIN_MDICHILD)
  {
    name = TEXT("IupDialogMDIChild");
    wndProc = (WNDPROC)winDialogMDIChildWndProc;
  }
  else if (type == IUPWIN_MDIFRAME)
  {
    name = TEXT("IupDialogMDIFrame");
    wndProc = (WNDPROC)winDialogMDIFrameWndProc;
  }
  else
  {
    if (type == IUPWIN_DIALOGCONTROL)
      name = TEXT("IupDialogControl");
    else if (type == IUPWIN_DIALOG_SAVEBITS)
      name = TEXT("IupDialogSaveBits");
    else
      name = TEXT("IupDialog");
    wndProc = (WNDPROC)winDialogWndProc;
  }

  wndclass.hInstance      = iupwin_hinstance;
  wndclass.lpszClassName  = name;
  wndclass.lpfnWndProc    = (WNDPROC)wndProc;
  wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);

  /* To use a standard system color, must increase the background-color value by one */
  if (type == IUPWIN_MDIFRAME)
    wndclass.hbrBackground  = (HBRUSH)(COLOR_APPWORKSPACE+1);  
  else
    wndclass.hbrBackground  = (HBRUSH)(COLOR_BTNFACE+1);

  if (type == IUPWIN_DIALOG_SAVEBITS)
    wndclass.style |= CS_SAVEBITS;

  if (type == IUPWIN_DIALOGCONTROL)
    wndclass.style |=  CS_HREDRAW | CS_VREDRAW;
    
  RegisterClass(&wndclass); 
}

static void winDialogRelease(Iclass* ic)
{
  (void)ic;

  if (iupwinClassExist(TEXT("IupDialog")))
  {
    UnregisterClass(TEXT("IupDialogMDIChild"), iupwin_hinstance);
    UnregisterClass(TEXT("IupDialogMDIFrame"), iupwin_hinstance);
    UnregisterClass(TEXT("IupDialogControl"), iupwin_hinstance);
    UnregisterClass(TEXT("IupDialogSaveBits"), iupwin_hinstance);
    UnregisterClass(TEXT("IupDialog"), iupwin_hinstance);
  }
}


/****************************************************************
                     dialog class functions
****************************************************************/

static int winDialogMapMethod(Ihandle* ih)
{
  InativeHandle* native_parent;
  DWORD dwStyle = WS_CLIPSIBLINGS, 
        dwExStyle = 0;
  int has_titlebar = 0,
      has_border = 0;
  TCHAR* classname = TEXT("IupDialog");
  char* title, *value;

  if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
    iupDialogCustomFrameSimulateCheckCallbacks(ih);

  title = iupAttribGet(ih, "TITLE"); 
  if (title)
    has_titlebar = 1;

  if (iupAttribGetBoolean(ih, "RESIZE"))
  {
    dwStyle |= WS_THICKFRAME;
    has_border = 1;
  }
  else
    iupAttribSet(ih, "MAXBOX", "NO");
  if (iupAttribGetBoolean(ih, "MENUBOX"))
  {
    dwStyle |= WS_SYSMENU;
    has_titlebar = 1;
  }
  if (iupAttribGetBoolean(ih, "MAXBOX"))
  {
    dwStyle |= WS_MAXIMIZEBOX;
    has_titlebar = 1;
  }
  if (iupAttribGetBoolean(ih, "MINBOX"))
  {
    dwStyle |= WS_MINIMIZEBOX;
    has_titlebar = 1;
  }
  if (iupAttribGetBoolean(ih, "BORDER") || has_titlebar)
    has_border = 1;

  if (iupAttribGetBoolean(ih, "MDICHILD"))
  {
    Ihandle *client;

    /* must have a parent dialog (the mdi frame) */
    Ihandle* parent = IupGetAttributeHandle(ih, "PARENTDIALOG");
    if (!parent || !parent->handle)
      return IUP_ERROR;

    /* set when the mdi client is mapped */
    client = (Ihandle*)iupAttribGet(parent, "MDICLIENT_HANDLE");
    if (!client)
      return IUP_ERROR;

    /* store the mdi client handle in each mdi child also */
    iupAttribSet(ih, "MDICLIENT_HANDLE", (char*)client);

    classname = TEXT("IupDialogMDIChild");

    /* The actual parent is the mdi client */
    native_parent = client->handle;

    dwStyle |= WS_CHILD;
    if (has_titlebar)
      dwStyle |= WS_CAPTION;
    else if (has_border)
      dwStyle |= WS_BORDER;

    /* make sure it has at least one name */
    if (!iupAttribGetHandleName(ih))
      iupAttribSetHandleName(ih);
  }
  else
  {
    native_parent = iupDialogGetNativeParent(ih);

    if (native_parent)
    {
      if (iupAttribGetBoolean(ih, "SAVEUNDER"))
        classname = TEXT("IupDialogSaveBits");

      dwStyle |= WS_POPUP;

      if (has_titlebar)
        dwStyle |= WS_CAPTION;
      else if (has_border)
        dwStyle |= WS_BORDER;
    }
    else
    {
      if (has_titlebar)
      {
        dwStyle |= WS_OVERLAPPED;
      }
      else 
      {
        if (has_border)
          dwStyle |= WS_POPUP | WS_BORDER;
        else
          dwStyle |= WS_POPUP;

        dwExStyle |= WS_EX_NOACTIVATE; /* this will hide it from the taskbar */ 
      }
    }

    if (iupAttribGetBoolean(ih, "MDIFRAME"))
    {
      COLORREF color = GetSysColor(COLOR_BTNFACE);
      iupAttribSetStrf(ih, "_IUPWIN_BACKGROUND_COLOR", "%d %d %d", (int)GetRValue(color), 
                                                                   (int)GetGValue(color), 
                                                                   (int)GetBValue(color));
      classname = TEXT("IupDialogMDIFrame");
    }
  }

  value = iupAttribGet(ih, "TASKBARBUTTON");
  if (value)
  {
    if (iupStrEqualNoCase(value, "SHOW"))
      dwExStyle |= WS_EX_APPWINDOW;   /* Force Show on taskbar even with no decorations */
    else if (iupStrEqualNoCase(value, "HIDE"))
      dwExStyle |= WS_EX_TOOLWINDOW;  /* Force Hide from taskbar even with all decorations */
  }

  if (iupAttribGetBoolean(ih, "TOOLBOX") && native_parent)
    dwExStyle |= WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE;

  if (iupAttribGetBoolean(ih, "DIALOGFRAME") && native_parent)
    dwExStyle |= WS_EX_DLGMODALFRAME;  /* this will hide the MENUBOX but not the close button */

  if (iupAttribGet(ih, "OPACITY") || iupAttribGet(ih, "OPACITYIMAGE"))
  {
    /* The window is a layered window. 
       This style cannot be used if the window has a class style of either CS_OWNDC or CS_CLASSDC.
       Windows >= 8 toplevel and child. Windows < 8 toplevel only.
       IMPORTANT: IupCanvas uses CS_OWNDC. */
    dwExStyle |= WS_EX_LAYERED;
  }

  iupwinGetNativeParentStyle(ih, &dwExStyle, &dwStyle);

  if (iupAttribGetBoolean(ih, "HELPBUTTON"))
    dwExStyle |= WS_EX_CONTEXTHELP;

  if (iupAttribGetBoolean(ih, "CONTROL") && native_parent) 
  {
    /* this is used to create embedded controls in COM or OLE */
    dwStyle = WS_CHILD | WS_TABSTOP | WS_CLIPCHILDREN;
    classname = TEXT("IupDialogControl");
  }

  /* CreateWindowEx will send WM_GETMINMAXINFO before Ihandle is associated with HWND */
  if (iupAttribGet(ih, "MINSIZE") || iupAttribGet(ih, "MAXSIZE"))
    winMinMaxHandle = ih;

  /* Windows 7 Taskbar */
#ifdef __ITaskbarList3_FWD_DEFINED__
  if (iupAttribGetInt(ih, "TASKBARPROGRESS"))
  {
    ITaskbarList3* tbl;
    CoCreateInstance(&CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskbarList3, &tbl);
    iupAttribSet(ih, "_IUPWIN_TASKBARLIST", (char*)((ITaskbarList3*)tbl));
  }
#endif

  /* size will be updated in IupRefresh -> winDialogLayoutUpdate */
  /* position will be updated in iupDialogShowXY              */

  if (iupAttribGetBoolean(ih, "MDICHILD"))
  {
    ih->handle = CreateMDIWindow(classname, 
                                iupwinStrToSystem(title), /* title */
                                dwStyle,            /* style */
                                0,                  /* x-position */
                                0,                  /* y-position */
                                100,                /* horizontal size - set this to avoid size calculation problems */
                                100,                /* vertical size */
                                native_parent,      /* owner window */
                                iupwin_hinstance,   /* instance of app. */
                                0);                 /* no creation parameters */
  }
  else
  {
      ih->handle = CreateWindowEx(dwExStyle,          /* extended styles */
                                classname,          /* class */
                                iupwinStrToSystem(title), /* title */
                                dwStyle,            /* style */
                                0,                  /* x-position */
                                0,                  /* y-position */
                                100,                /* horizontal size - set this to avoid size calculation problems */
                                100,                /* vertical size */
                                native_parent,      /* owner window */
                                (HMENU)0,           /* Menu or child-window identifier */
                                iupwin_hinstance,   /* instance of app. */
                                NULL);              /* no creation parameters */
  }

  /* clear handle right after CreateWindowEx */
  winMinMaxHandle = NULL;

  if (!ih->handle)
    return IUP_ERROR;

  /* associate HWND with Ihandle*, all Win32 controls must call this. */
  iupwinHandleAdd(ih, ih->handle);

  if (iupAttribGetBoolean(ih, "MDICHILD"))  /* hides the mdi child */
    ShowWindow(ih->handle, SW_HIDE);

  /* configure for DROP of files */
  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  /* Ignore VISIBLE before mapping */
  iupAttribSet(ih, "VISIBLE", NULL);

  /* Set the default CmdShow for ShowWindow */
  ih->data->cmd_show = SW_SHOWNORMAL;

  if (iupAttribGetBoolean(ih, "MDICHILD"))
    winDialogMDIRefreshMenu(ih);

  return IUP_NOERROR;
}

static void winDialogUnMapMethod(Ihandle* ih)
{
  if (ih->data->menu) 
  {
    ih->data->menu->handle = NULL; /* the dialog will destroy the native menu */
    IupDestroy(ih->data->menu);  
    ih->data->menu = NULL;
  }

#ifdef __ITaskbarList3_FWD_DEFINED__
  if ((ITaskbarList3*)iupAttribGet(ih, "_IUPWIN_TASKBARLIST"))
  {
    ITaskbarList3* tbl = (ITaskbarList3*)iupAttribGet(ih, "_IUPWIN_TASKBARLIST");
    tbl->lpVtbl->Release(tbl);
    iupAttribSet(ih, "_IUPWIN_TASKBARLIST", NULL);
  }
#endif

  if (iupAttribGet(ih, "_IUPDLG_HASTRAY"))
    winDialogSetTrayAttrib(ih, NULL);

  iupwinTipsDestroy(ih);
  iupwinDestroyDragDrop(ih);

  /* remove the association before destroying */
  iupwinHandleRemove(ih->handle);

  /* Destroys the window, so we can destroy the class */
  if (iupAttribGetBoolean(ih, "MDICHILD")) 
  {
    /* for MDICHILDs must send WM_MDIDESTROY, instead of calling DestroyWindow */
    Ihandle* client = (Ihandle*)iupAttribGet(ih, "MDICLIENT_HANDLE");
    SendMessage(client->handle, WM_MDIDESTROY, (WPARAM)ih->handle, 0);

    winDialogMDIRefreshMenu(ih);
  }
  else
    DestroyWindow(ih->handle); /* this will destroy the Windows children also. */
                               /* but IupDestroy already destroyed the IUP children */
                               /* so it is safe to call DestroyWindow */
}

static void winDialogLayoutUpdateMethod(Ihandle *ih)
{
  if (ih->data->ignore_resize)
    return;

  ih->data->ignore_resize = 1;

  /* for dialogs the position is not updated here */
  SetWindowPos(ih->handle, 0, 0, 0, ih->currentwidth, ih->currentheight,
               SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSENDCHANGING);

  ih->data->ignore_resize = 0;
}


/****************************************************************************
                                   Attributes
****************************************************************************/


static char* winDialogGetClientOffsetAttrib(Ihandle *ih)
{
  (void)ih;

  if (iupAttribGetBoolean(ih, "CUSTOMFRAMEDRAW"))
  {
    int x, y;
    int border, caption, menu;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    x = border;
    y = border + caption + menu;

    return iupStrReturnIntInt(x, y, 'x');
  }

  return "0x0";
}

static char* winDialogGetClientSizeAttrib(Ihandle* ih)
{
  if (ih->handle)
  {
    int width, height;
    RECT rect;

    GetClientRect(ih->handle, &rect);
    width = (int)(rect.right - rect.left);
    height = (int)(rect.bottom - rect.top);

    if (iupAttribGetBoolean(ih, "CUSTOMFRAMEDRAW"))
    {
      int border, caption, menu;
      iupdrvDialogGetDecoration(ih, &border, &caption, &menu);
      width -= 2 * border;
      height -= caption + menu + 2 * border;
    }

    return iupStrReturnIntInt(width, height, 'x');
  }
  else
    return iupDialogGetClientSizeAttrib(ih);
}

static int winDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (!iupwin_comctl32ver6 && iupAttribGetBoolean(ih, "CUSTOMFRAME"))
  {
    LockWindowUpdate(ih->handle);
    iupwinSetTitleAttrib(ih, value);
    LockWindowUpdate(NULL);
  }
  else
    iupwinSetTitleAttrib(ih, value);

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME") || iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
    return 0;

  return 1;
}

static int winDialogSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    iupAttribSetStr(ih, "_IUPWIN_BACKGROUND_COLOR", value);
    iupAttribSet(ih, "_IUPWIN_BACKGROUND_BITMAP", NULL);
    RedrawWindow(ih->handle, NULL, NULL, RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN); /* post WM_ERASEBKGND and WM_PAINT */
    return 1;
  }
  return 0;
}

static int winDialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  if (winDialogSetBgColorAttrib(ih, value))
    return 1;
  else
  {
    HBITMAP hBitmap = iupImageGetImage(value, ih, 0, NULL);
    if (hBitmap)
    {
      iupAttribSet(ih, "_IUPWIN_BACKGROUND_COLOR", NULL);
      iupAttribSet(ih, "_IUPWIN_BACKGROUND_BITMAP", (char*)hBitmap);
      RedrawWindow(ih->handle, NULL, NULL, RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN); /* post WM_ERASEBKGND and WM_PAINT */
      return 1;
    }
  }

  return 0;
}

static HWND iupwin_mdifirst = NULL;
static HWND iupwin_mdinext = NULL;

static char* winDialogGetMdiActiveAttrib(Ihandle *ih)
{
  Ihandle* client = (Ihandle*)iupAttribGet(ih, "MDICLIENT_HANDLE");
  if (client)
  {
    HWND hchild = (HWND)SendMessage(client->handle, WM_MDIGETACTIVE, 0, 0);
    Ihandle* child = iupwinHandleGet(hchild); 
    if (iupObjectCheck(child))
    {
      iupwin_mdinext = NULL;
      iupwin_mdifirst = hchild;
      return IupGetName(child);
    }
  }

  iupwin_mdifirst = NULL;
  iupwin_mdinext = NULL;
  return NULL;
}

static char* winDialogGetMdiNextAttrib(Ihandle *ih)
{
  Ihandle* client = (Ihandle*)iupAttribGet(ih, "MDICLIENT_HANDLE");
  if (client)
  {
    Ihandle* child;
    HWND hchild = iupwin_mdinext? iupwin_mdinext: iupwin_mdifirst;

    /* Skip the icon title windows */
    while (hchild && GetWindow (hchild, GW_OWNER))
      hchild = GetWindow(hchild, GW_HWNDNEXT);

    if (!hchild || hchild == iupwin_mdifirst)
    {
      iupwin_mdinext = NULL;
      return NULL;
    }

    child = iupwinHandleGet(hchild); 
    if (iupObjectCheck(child))
    {
      iupwin_mdinext = hchild;
      return IupGetName(child);
    }
  }

  iupwin_mdinext = NULL;
  return NULL;
}

static int winDialogSetOpacityAttrib(Ihandle *ih, const char *value)
{
  int opacity;
  if (!iupStrToInt(value, &opacity))
    return 0;

  SetLayeredWindowAttributes(ih->handle, 0, (BYTE)opacity, LWA_ALPHA);
  RedrawWindow(ih->handle, NULL, NULL, RDW_ERASE|RDW_INVALIDATE|RDW_FRAME|RDW_ALLCHILDREN); /* invalidate everything and all children */
  return 1;
}

static int winDialogSetOpacityImageAttrib(Ihandle *ih, const char *value)
{
  HBITMAP hBitmap = (HBITMAP)iupImageGetImage(value, ih, 0, NULL);
  if (!hBitmap)
    return 0;
  else
  {
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    POINT ptSrc = { 0, 0 };

    HDC hDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
    int img_w, img_h, bpp;
    SIZE size;

    iupdrvImageGetInfo(hBitmap, &img_w, &img_h, &bpp);
    size.cx = img_w;
    size.cy = img_h;

    UpdateLayeredWindow(ih->handle, hDC, NULL, &size, hMemDC, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hMemDC, oldBitmap);
    DeleteDC(hMemDC);  /* to match CreateCompatibleDC */
    ReleaseDC(NULL, hDC);

    return 1;
  }
}

static int winDialogSetShapeImageAttrib(Ihandle *ih, const char *value)
{
  Ihandle* image = IupGetHandle(value);
  if (!image)
  {
    SetWindowRgn(ih->handle, NULL, TRUE);
    return 0;
  }
  else
  {
    unsigned char* imgdata = (unsigned char*)iupAttribGet(image, "WID");
    int channels = iupAttribGetInt(image, "CHANNELS");
    int w = image->currentwidth;
    int h = image->currentheight;
    int x, y;
    HRGN hRgn, hTmpRgn;

    if (!imgdata || channels != 4)
      return 0;

    /* accumulate non transparent regions */
    hRgn = CreateRectRgn(0, 0, 0, 0);

    for (y = 0; y < h; y++)
    {
      int start_x = -1;

      for (x = 0; x < w; x++)
      {
        if (imgdata[3] == 0 || x == w-1) /* fully transparent or last column */
        {
          if (start_x != -1) /* this is the end of a non transparent line */
          {
            hTmpRgn = CreateRectRgn(start_x, y, x, y + 1);
            CombineRgn(hRgn, hRgn, hTmpRgn, RGN_OR); /* Union */
            DeleteObject(hTmpRgn);

            start_x = -1;
          }
        }
        else /* opaque */
        {
          if (start_x == -1) /* this is the start of a non transparent line */
            start_x = x;
        }

        imgdata += 4;
      }
    }

    SetWindowRgn(ih->handle, hRgn, TRUE);

    return 1;
  }
}

static int winDialogSetMdiArrangeAttrib(Ihandle *ih, const char *value)
{
  Ihandle* client = (Ihandle*)iupAttribGet(ih, "MDICLIENT_HANDLE");
  if (client)
  {
    UINT msg = 0;
    WPARAM wp = 0;

    if (iupStrEqualNoCase(value, "TILEHORIZONTAL"))
    {
      msg = WM_MDITILE;
      wp = MDITILE_HORIZONTAL;
    }
    else if (iupStrEqualNoCase(value, "TILEVERTICAL"))
    {
      msg = WM_MDITILE;
      wp = MDITILE_VERTICAL;
    }
    else if (iupStrEqualNoCase(value, "CASCADE"))
      msg = WM_MDICASCADE;
    else if (iupStrEqualNoCase(value, "ICON")) 
      msg = WM_MDIICONARRANGE;

    if (msg)
      SendMessage(client->handle, msg, wp, 0);
  }
  return 0;
}

static int winDialogSetMdiActivateAttrib(Ihandle *ih, const char *value)
{
  Ihandle* client = (Ihandle*)iupAttribGet(ih, "MDICLIENT_HANDLE");
  if (client)
  {
    Ihandle* child = IupGetHandle(value);
    if (child)
      SendMessage(client->handle, WM_MDIACTIVATE, (WPARAM)child->handle, 0);
    else
    {
      HWND hchild = (HWND)SendMessage(client->handle, WM_MDIGETACTIVE, 0, 0);
      if (iupStrEqualNoCase(value, "NEXT"))
        SendMessage(client->handle, WM_MDINEXT, (WPARAM)hchild, TRUE);
      else if (iupStrEqualNoCase(value, "PREVIOUS"))
        SendMessage(client->handle, WM_MDINEXT, (WPARAM)hchild, FALSE);
    }
  }
  return 0;
}

static int winDialogSetMdiCloseAllAttrib(Ihandle *ih, const char *value)
{
  (void)value;
  winDialogMDICloseChildren(ih);
  return 0;
}

static char* winDialogGetMaximizedAttrib(Ihandle *ih)
{
  if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
    return iupAttribGet(ih, "MAXIMIZED");
  else
    return iupStrReturnBoolean(IsZoomed(ih->handle));
}

static char* winDialogGetMinimizedAttrib(Ihandle *ih)
{
  return iupStrReturnBoolean(IsIconic(ih->handle));
}

static void winDialogTrayMessage(HWND hWnd, DWORD dwMessage, HICON hIcon, const char* value)
{
  NOTIFYICONDATA tnd;
  memset(&tnd, 0, sizeof(NOTIFYICONDATA));

  tnd.cbSize  = sizeof(NOTIFYICONDATA);
  tnd.hWnd    = hWnd;
  tnd.uID     = 1000;

  if (dwMessage == NIM_ADD)
  {
    tnd.uFlags = NIF_MESSAGE;
    tnd.uCallbackMessage = WM_USER+IUPWIN_TRAY_NOTIFICATION;
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

static void winDialogTrayBalloonMessage(Ihandle *ih, const char* value)
{
  NOTIFYICONDATA tnd;
  memset(&tnd, 0, sizeof(NOTIFYICONDATA));

  tnd.cbSize  = sizeof(NOTIFYICONDATA);
  tnd.hWnd    = ih->handle;
  tnd.uID     = 1000;
  tnd.uFlags |= NIF_INFO;

  /* set to NULL to remove the tooltip */
  if (value) 
  {
    char* balloon_title;

    iupwinStrCopy(tnd.szInfo, value, sizeof(tnd.szInfo));

    tnd.uTimeout = IupGetInt(ih, "TRAYTIPBALLOONDELAY"); /* must use IupGetInt to use inheritance */

    balloon_title = IupGetAttribute(ih, "TRAYTIPBALLOONTITLE");
    if (balloon_title)
      iupwinStrCopy(tnd.szInfoTitle, balloon_title, sizeof(tnd.szInfoTitle));

    tnd.dwInfoFlags = IupGetInt(ih, "TRAYTIPBALLOONTITLEICON");
  }

  Shell_NotifyIcon(NIM_MODIFY, &tnd);
}

static int winDialogCheckTray(Ihandle *ih)
{
  if (iupAttribGet(ih, "_IUPDLG_HASTRAY"))
    return 1;

  if (iupAttribGetBoolean(ih, "TRAY"))
  {
    winDialogTrayMessage(ih->handle, NIM_ADD, NULL, NULL);
    iupAttribSet(ih, "_IUPDLG_HASTRAY", "YES");
    return 1;
  }

  return 0;
}

static int winDialogSetTrayAttrib(Ihandle *ih, const char *value)
{
  int tray = iupStrBoolean(value);
  if (iupAttribGet(ih, "_IUPDLG_HASTRAY"))
  {
    if (!tray)
    {
      winDialogTrayMessage(ih->handle, NIM_DELETE, NULL, NULL);
      iupAttribSet(ih, "_IUPDLG_HASTRAY", NULL);
    }
  }
  else
  {
    if (tray)
    {
      winDialogTrayMessage(ih->handle, NIM_ADD, NULL, NULL);
      iupAttribSet(ih, "_IUPDLG_HASTRAY", "YES");
    }
  }
  return 1;
}

static int winDialogSetTrayTipAttrib(Ihandle *ih, const char *value)
{
  if (winDialogCheckTray(ih))
  {
    int balloon = IupGetInt(ih, "TRAYTIPBALLOON");  /* must use IupGetInt to use inheritance */
    if (balloon)
      winDialogTrayBalloonMessage(ih, value);
    else
      winDialogTrayMessage(ih->handle, NIM_MODIFY, NULL, value);
  }
  return 1;
}

static int winDialogSetTrayImageAttrib(Ihandle *ih, const char *value)
{
  if (winDialogCheckTray(ih))
  {
    HICON hIcon = (HICON)iupImageGetIcon(value);
    if (hIcon)
      winDialogTrayMessage(ih->handle, NIM_MODIFY, hIcon, NULL);
  }
  return 1;
}

static int winDialogSetBringFrontAttrib(Ihandle *ih, const char *value)
{
  if (iupStrBoolean(value))
    SetForegroundWindow(ih->handle);
  return 0;
}

static char* winDialogGetActiveWindowAttrib(Ihandle* ih)
{
  WINDOWINFO wininfo;
  wininfo.cbSize = sizeof(WINDOWINFO);
  GetWindowInfo(ih->handle, &wininfo);
  return iupStrReturnBoolean (wininfo.dwWindowStatus & WS_ACTIVECAPTION); 
}    

static int winDialogSetTopMostAttrib(Ihandle *ih, const char *value)
{
  if (iupStrBoolean(value))
    SetWindowPos(ih->handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
  else
    SetWindowPos(ih->handle, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
  return 1;
}

static HICON winDialogLoadIcon(const char* name, int size)
{
  int w = (size == ICON_SMALL) ? GetSystemMetrics(SM_CXSMICON) : GetSystemMetrics(SM_CXICON);
  int h = (size == ICON_SMALL) ? GetSystemMetrics(SM_CYSMICON) : GetSystemMetrics(SM_CYICON);
  /* same as iupdrvImageLoad but using w and h to control icon size */
  HICON hIcon = LoadImage(iupwin_hinstance, iupwinStrToSystem(name), IMAGE_ICON, w, h, 0);
  if (!hIcon && iupwin_dll_hinstance)
    hIcon = LoadImage(iupwin_dll_hinstance, iupwinStrToSystem(name), IMAGE_ICON, w, h, 0);
  if (!hIcon)
    hIcon = LoadImage(NULL, iupwinStrToSystemFilename(name), IMAGE_ICON, w, h, LR_LOADFROMFILE);
  return hIcon;
}

static int winDialogSetIconAttrib(Ihandle* ih, const char *value)
{
  if (!value)
  {
    SendMessage(ih->handle, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)NULL);
    SendMessage(ih->handle, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)NULL);
  }
  else
  {
    /* check first in the resources using size hits */
    HICON icon = winDialogLoadIcon(value, ICON_SMALL);
    if (icon)
      SendMessage(ih->handle, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)icon);

    icon = winDialogLoadIcon(value, ICON_BIG);
    if (icon)
      SendMessage(ih->handle, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)icon);
    else
    {
      icon = iupImageGetIcon(value);
      if (icon)
        SendMessage(ih->handle, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)icon);
    }
  }

  if (IsIconic(ih->handle))
    RedrawWindow(ih->handle, NULL, NULL, RDW_INVALIDATE|RDW_ERASE|RDW_UPDATENOW); /* redraw the icon */
  else
    RedrawWindow(ih->handle, NULL, NULL, RDW_FRAME|RDW_UPDATENOW); /* only the frame needs to be redrawn */

  return 1;
}

static int winDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    if (!iupAttribGet(ih, "_IUPWIN_FS_STYLE"))
    {
      int width, height;
      LONG off_style, new_style;

      BOOL visible = ShowWindow (ih->handle, SW_HIDE);

      /* remove the decorations */
      off_style = WS_BORDER | WS_THICKFRAME | WS_CAPTION | 
                  WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU;
      new_style = GetWindowLong(ih->handle, GWL_STYLE);
      iupAttribSet(ih, "_IUPWIN_FS_STYLE", (char*)new_style);
      new_style &= (~off_style);
      SetWindowLong(ih->handle, GWL_STYLE, new_style);

      /* save the previous decoration attributes */
      iupAttribSetStr(ih, "_IUPWIN_FS_MAXBOX", iupAttribGet(ih, "MAXBOX"));
      iupAttribSetStr(ih, "_IUPWIN_FS_MINBOX", iupAttribGet(ih, "MINBOX"));
      iupAttribSetStr(ih, "_IUPWIN_FS_MENUBOX",iupAttribGet(ih, "MENUBOX"));
      iupAttribSetStr(ih, "_IUPWIN_FS_RESIZE", iupAttribGet(ih, "RESIZE"));
      iupAttribSetStr(ih, "_IUPWIN_FS_BORDER", iupAttribGet(ih, "BORDER"));
      iupAttribSetStr(ih, "_IUPWIN_FS_TITLE",  iupAttribGet(ih, "TITLE"));

      /* save the previous position and size */
      iupAttribSetStr(ih, "_IUPWIN_FS_X", IupGetAttribute(ih, "X"));  /* must use IupGetAttribute to check from the native implementation */
      iupAttribSetStr(ih, "_IUPWIN_FS_Y", IupGetAttribute(ih, "Y"));
      iupAttribSetStr(ih, "_IUPWIN_FS_SIZE", IupGetAttribute(ih, "RASTERSIZE"));

      /* remove the decorations attributes */
      iupAttribSet(ih, "MAXBOX", "NO");
      iupAttribSet(ih, "MINBOX", "NO");
      iupAttribSet(ih, "MENUBOX", "NO");
      IupSetAttribute(ih, "TITLE", NULL);  /* must use IupSetAttribute to update the native implementation */
      iupAttribSet(ih, "RESIZE", "NO");
      iupAttribSet(ih, "BORDER", "NO");

      /* full screen size */
      iupdrvGetFullSize(&width, &height);

      SetWindowPos(ih->handle, HWND_TOP, 0, 0, width, height, SWP_FRAMECHANGED);

      /* layout will be updated in WM_SIZE */
      if (visible)
        ShowWindow(ih->handle, SW_SHOW);
    }
  }
  else
  {
    LONG style = (LONG)iupAttribGet(ih, "_IUPWIN_FS_STYLE");
    if (style)
    {
      BOOL visible = ShowWindow(ih->handle, SW_HIDE);

      /* restore the decorations attributes */
      iupAttribSetStr(ih, "MAXBOX", iupAttribGet(ih, "_IUPWIN_FS_MAXBOX"));
      iupAttribSetStr(ih, "MINBOX", iupAttribGet(ih, "_IUPWIN_FS_MINBOX"));
      iupAttribSetStr(ih, "MENUBOX",iupAttribGet(ih, "_IUPWIN_FS_MENUBOX"));
      IupSetAttribute(ih, "TITLE",  iupAttribGet(ih, "_IUPWIN_FS_TITLE"));  /* must use IupSetAttribute to update the native implementation */
      iupAttribSetStr(ih, "RESIZE", iupAttribGet(ih, "_IUPWIN_FS_RESIZE"));
      iupAttribSetStr(ih, "BORDER", iupAttribGet(ih, "_IUPWIN_FS_BORDER"));

      /* restore the decorations */
      SetWindowLong(ih->handle, GWL_STYLE, style);

      /* restore position and size */
      SetWindowPos(ih->handle, HWND_TOP, 
                   iupAttribGetInt(ih, "_IUPWIN_FS_X"), 
                   iupAttribGetInt(ih, "_IUPWIN_FS_Y"), 
                   IupGetInt(ih, "_IUPWIN_FS_SIZE"), 
                   IupGetInt2(ih, "_IUPWIN_FS_SIZE"), 0);

      /* layout will be updated in WM_SIZE */
      if (visible)
        ShowWindow(ih->handle, SW_SHOW);

      /* remove auxiliary attributes */
      iupAttribSet(ih, "_IUPWIN_FS_MAXBOX", NULL);
      iupAttribSet(ih, "_IUPWIN_FS_MINBOX", NULL);
      iupAttribSet(ih, "_IUPWIN_FS_MENUBOX",NULL);
      iupAttribSet(ih, "_IUPWIN_FS_TITLE",  NULL);
      iupAttribSet(ih, "_IUPWIN_FS_RESIZE", NULL);
      iupAttribSet(ih, "_IUPWIN_FS_BORDER", NULL);

      iupAttribSet(ih, "_IUPWIN_FS_X", NULL);
      iupAttribSet(ih, "_IUPWIN_FS_Y", NULL);
      iupAttribSet(ih, "_IUPWIN_FS_SIZE", NULL);

      iupAttribSet(ih, "_IUPWIN_FS_STYLE", NULL);
    }
  }
  return 1;
}

void iupdrvDialogInitClass(Iclass* ic)
{
  if (!iupwinClassExist(TEXT("IupDialog")))
  {
    winDialogRegisterClass(IUPWIN_DIALOG);
    winDialogRegisterClass(IUPWIN_DIALOGCONTROL);
    winDialogRegisterClass(IUPWIN_MDIFRAME);
    winDialogRegisterClass(IUPWIN_MDICHILD);
    winDialogRegisterClass(IUPWIN_DIALOG_SAVEBITS);

    WM_HELPMSG = RegisterWindowMessage(HELPMSGSTRING);
  }

  /* Driver Dependent Class functions */
  ic->Map = winDialogMapMethod;
  ic->UnMap = winDialogUnMapMethod;
  ic->LayoutUpdate = winDialogLayoutUpdateMethod;
  ic->Release = winDialogRelease;

  /* Callback Windows Only*/
  iupClassRegisterCallback(ic, "MDIACTIVATE_CB", "");
  iupClassRegisterCallback(ic, "CUSTOMFRAMEDRAW_CB", "");
  iupClassRegisterCallback(ic, "CUSTOMFRAMEACTIVATE_CB", "i");

  /* Callback Windows and GTK Only */
  iupClassRegisterCallback(ic, "TRAYCLICK_CB", "iii");

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, winDialogSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "TITLE", NULL, winDialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* Base Container */
  iupClassRegisterAttribute(ic, "CLIENTSIZE", winDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);  /* dialog is the only not read-only */
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", winDialogGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* IupDialog only */
  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, winDialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ICON", NULL, winDialogSetIconAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FULLSCREEN", NULL, winDialogSetFullScreenAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEUNDER", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINSIZE", NULL, iupBaseSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXSIZE", NULL, iupBaseSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* IupDialog Windows Only */
  iupClassRegisterAttribute(ic, "HWND", iupBaseGetWidAttrib, NULL, NULL, NULL, IUPAF_NO_STRING|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIARRANGE", NULL, winDialogSetMdiArrangeAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIACTIVATE", NULL, winDialogSetMdiActivateAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICLOSEALL", NULL, winDialogSetMdiCloseAllAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIACTIVE", winDialogGetMdiActiveAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDINEXT", winDialogGetMdiNextAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITY", NULL, winDialogSetOpacityAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITYIMAGE", NULL, winDialogSetOpacityImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHAPEIMAGE", NULL, winDialogSetShapeImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LAYERALPHA", NULL, winDialogSetOpacityAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BRINGFRONT", NULL, winDialogSetBringFrontAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXIMIZED", winDialogGetMaximizedAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINIMIZED", winDialogGetMinimizedAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWNOACTIVATE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWMINIMIZENEXT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXIMIZEATPARENT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXIMIZEATDIALOG", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXIMIZEDIALOG", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COMPOSITED", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "CONTROL", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HELPBUTTON", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOOLBOX", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIFRAME", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICLIENT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIMENU", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICHILD", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* IupDialog Windows and GTK Only */
  iupClassRegisterAttribute(ic, "ACTIVEWINDOW", winDialogGetActiveWindowAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPMOST", NULL, winDialogSetTopMostAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPMOST", NULL, winDialogSetTopMostAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAY", NULL, winDialogSetTrayAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYIMAGE", NULL, winDialogSetTrayImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIP", NULL, winDialogSetTrayTipAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUSTOMFRAME", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NO_INHERIT);

  /* IupDialog Windows Only */
  iupClassRegisterAttribute(ic, "TRAYTIPDELAY", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIPBALLOON", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIPBALLOONTITLE", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIPBALLOONTITLEICON", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TASKBARBUTTON", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CUSTOMFRAMEDRAW", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUSTOMFRAMECAPTIONHEIGHT", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUSTOMFRAMECAPTIONLIMITS", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NO_INHERIT);

#ifdef __ITaskbarList3_FWD_DEFINED__
  iupClassRegisterAttribute(ic, "TASKBARPROGRESS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TASKBARPROGRESSSTATE", NULL, winDialogSetTaskBarProgressStateAttrib, IUPAF_SAMEASSYSTEM, "NORMAL", IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TASKBARPROGRESSVALUE", NULL, winDialogSetTaskBarProgressValueAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
#endif

  /* Not Supported */
  iupClassRegisterAttribute(ic, "DIALOGHINT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CONTROLID", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}

