/** \file
 * \brief IupPopover control for Windows
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <commctrl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_childtree.h"
#include "iup_class.h"
#include "iup_register.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"

#include "iup_popover.h"


#define IUPWIN_POPOVER_CLASS TEXT("IupPopover")


static LRESULT CALLBACK winPopoverProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  switch (msg)
  {
    case WM_NCPAINT:
    {
      HDC hdc = GetWindowDC(hwnd);
      if (hdc)
      {
        RECT rect;
        GetWindowRect(hwnd, &rect);
        rect.right -= rect.left;
        rect.bottom -= rect.top;
        rect.left = 0;
        rect.top = 0;

        /* Draw border using system button shadow color */
        HPEN hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        ReleaseDC(hwnd, hdc);
      }
      return 0;
    }

    case WM_ACTIVATE:
    {
      if (LOWORD(wp) == WA_INACTIVE)
      {
        Ihandle* ih = iupwinHandleGet(hwnd);
        HWND new_hwnd = (HWND)lp;
        if (ih && iupAttribGetBoolean(ih, "AUTOHIDE"))
        {
          Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
          HWND anchor_hwnd = anchor ? (HWND)anchor->handle : NULL;
          HWND parent_hwnd = anchor ? GetAncestor((HWND)anchor->handle, GA_ROOT) : NULL;

          /* Don't auto-hide if clicking on popover itself or its children */
          if (new_hwnd && (IsChild(hwnd, new_hwnd) || new_hwnd == hwnd))
            break;

          /* Don't auto-hide if clicking on anchor or its children, let anchor handle toggle */
          if (anchor_hwnd && (new_hwnd == anchor_hwnd || IsChild(anchor_hwnd, new_hwnd)))
            break;

          /* Check if clicking on parent dialog, need to find actual clicked control */
          if (parent_hwnd && (new_hwnd == parent_hwnd || IsChild(parent_hwnd, new_hwnd)))
          {
            POINT pt;
            GetCursorPos(&pt);
            HWND clicked = WindowFromPoint(pt);
            /* If clicked on anchor or its children, let anchor handle it */
            if (clicked && anchor_hwnd && (clicked == anchor_hwnd || IsChild(anchor_hwnd, clicked)))
              break;
          }

          IupSetAttribute(ih, "VISIBLE", "NO");
        }
      }
      break;
    }

    case WM_CLOSE:
    {
      Ihandle* ih = iupwinHandleGet(hwnd);
      if (ih)
        IupSetAttribute(ih, "VISIBLE", "NO");
      return 0;
    }

    default:
    {
      Ihandle* ih = iupwinHandleGet(hwnd);
      if (ih)
      {
        LRESULT result = 0;
        if (iupwinBaseContainerMsgProc(ih, msg, wp, lp, &result))
          return result;
      }
      break;
    }
  }

  return DefWindowProc(hwnd, msg, wp, lp);
}

static void winPopoverRegisterClass(void)
{
  static int registered = 0;
  if (!registered)
  {
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(WNDCLASS));

    wc.lpfnWndProc = winPopoverProc;
    wc.hInstance = iupwin_hinstance;
    wc.lpszClassName = IUPWIN_POPOVER_CLASS;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;

    RegisterClass(&wc);
    registered = 1;
  }
}

static int winPopoverSetVisibleAttrib(Ihandle* ih, const char* value)
{
  HWND hwnd;

  if (iupStrBoolean(value))
  {
    Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
    HWND anchor_hwnd;
    RECT anchor_rect;
    int x, y;

    if (!anchor || !anchor->handle)
      return 0;

    /* Map if not yet mapped */
    if (!ih->handle)
    {
      if (IupMap(ih) == IUP_ERROR)
        return 0;
    }

    hwnd = (HWND)ih->handle;
    anchor_hwnd = (HWND)anchor->handle;

    GetWindowRect(anchor_hwnd, &anchor_rect);

    if (ih->firstchild)
    {
      iupLayoutCompute(ih);
      iupLayoutUpdate(ih);
    }

    {
      const char* pos = iupAttribGetStr(ih, "POSITION");

      if (iupStrEqualNoCase(pos, "TOP"))
      {
        x = anchor_rect.left;
        y = anchor_rect.top - ih->currentheight;
      }
      else if (iupStrEqualNoCase(pos, "LEFT"))
      {
        x = anchor_rect.left - ih->currentwidth;
        y = anchor_rect.top;
      }
      else if (iupStrEqualNoCase(pos, "RIGHT"))
      {
        x = anchor_rect.right;
        y = anchor_rect.top;
      }
      else
      {
        x = anchor_rect.left;
        y = anchor_rect.bottom;
      }
    }

    if (iupAttribGetBoolean(ih, "AUTOHIDE"))
    {
      SetWindowPos(hwnd, HWND_TOPMOST, x, y, ih->currentwidth, ih->currentheight, SWP_SHOWWINDOW);
      ShowWindow(hwnd, SW_SHOW);
      SetForegroundWindow(hwnd);
    }
    else
    {
      SetWindowPos(hwnd, HWND_TOPMOST, x, y, ih->currentwidth, ih->currentheight,
                   SWP_NOACTIVATE | SWP_SHOWWINDOW);
      ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    }

    {
      IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
      if (show_cb)
        show_cb(ih, IUP_SHOW);
    }
  }
  else
  {
    if (ih->handle)
    {
      hwnd = (HWND)ih->handle;
      ShowWindow(hwnd, SW_HIDE);

      {
        IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
        if (show_cb)
          show_cb(ih, IUP_HIDE);
      }
    }
  }

  return 0;
}

static char* winPopoverGetVisibleAttrib(Ihandle* ih)
{
  HWND hwnd;

  if (!ih->handle)
    return "NO";

  hwnd = (HWND)ih->handle;
  return iupStrReturnBoolean(IsWindowVisible(hwnd));
}

static void winPopoverLayoutUpdateMethod(Ihandle* ih)
{
  if (ih->firstchild)
  {
    ih->iclass->SetChildrenPosition(ih, 0, 0);
    iupLayoutUpdate(ih->firstchild);
  }
}

static int winPopoverMapMethod(Ihandle* ih)
{
  HWND hwnd;
  DWORD dwStyle = WS_POPUP | WS_BORDER | WS_CLIPCHILDREN;
  DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;

  winPopoverRegisterClass();

  hwnd = CreateWindowEx(
    dwExStyle,
    IUPWIN_POPOVER_CLASS,
    NULL,
    dwStyle,
    CW_USEDEFAULT, CW_USEDEFAULT,
    10, 10,
    NULL,
    NULL,
    iupwin_hinstance,
    NULL
  );

  if (!hwnd)
    return IUP_ERROR;

  ih->handle = hwnd;

  iupwinHandleAdd(ih, hwnd);

  return IUP_NOERROR;
}

static void winPopoverUnMapMethod(Ihandle* ih)
{
  HWND hwnd = (HWND)ih->handle;

  if (hwnd)
  {
    iupwinHandleRemove(hwnd);
    DestroyWindow(hwnd);
  }

  ih->handle = NULL;
}

static void* winPopoverGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return ih->handle;
}

void iupdrvPopoverInitClass(Iclass* ic)
{
  ic->Map = winPopoverMapMethod;
  ic->UnMap = winPopoverUnMapMethod;
  ic->LayoutUpdate = winPopoverLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = winPopoverGetInnerNativeContainerHandleMethod;

  /* Override VISIBLE attribute, NOT_MAPPED because setter handles mapping */
  iupClassRegisterAttribute(ic, "VISIBLE", winPopoverGetVisibleAttrib, winPopoverSetVisibleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}
