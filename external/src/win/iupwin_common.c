/** \file
 * \brief Windows Base Procedure
 *
 * See Copyright Notice in "iup.h"
 */
#include <windows.h>
#include <commctrl.h>

#include <stdio.h>              
#include <stdlib.h>
#include <string.h>             

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_focus.h"
#include "iup_image.h"
#include "iup_dialog.h"
#include "iup_drvinfo.h"
#include "iup_drv.h"
#include "iup_assert.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_brush.h"
#include "iupwin_info.h"
#include "iupwin_str.h"


static UINT WM_DRAGLISTMSG = 0;

/* Not defined in compilers older than VC9 or WinSDK older than 6.0 */
#ifndef MAPVK_VK_TO_VSC
#define MAPVK_VK_TO_VSC     (0)
#endif
#ifndef WM_TOUCH
#define WM_TOUCH            0x0240
#endif
#ifndef WS_EX_COMPOSITED
#define WS_EX_COMPOSITED 0x02000000L
#endif

int iupwinClassExist(const TCHAR* name)
{
  WNDCLASS WndClass;
  if (GetClassInfo(iupwin_hinstance, name, &WndClass))
    return 1;
  return 0;
}

int iupwinGetScreenRes(void)
{
  /* same as iupdrvGetScreenDpi, but returns an integer value */
  int res;
  HDC ScreenDC = GetDC(NULL);
  res = GetDeviceCaps(ScreenDC, LOGPIXELSY);
  ReleaseDC(NULL, ScreenDC);
  return res;
}

IUP_SDK_API void iupdrvActivate(Ihandle* ih)
{
  int is_toggle = IupClassMatch(ih, "toggle");
  int is_radio = 0;
  if (is_toggle)
    is_radio = IupGetInt(ih, "RADIO");

  /* do not use BM_CLICK because it changes the focus 
     and does not animates the button press */

  /* draw highlight */
  SendMessage(ih->handle, BM_SETSTATE, TRUE, 0);
  IupFlush();
  Sleep(150);

  /* must force a toggle change */
  if (is_toggle && !is_radio)
    IupSetAttribute(ih, "VALUE", "TOGGLE");

  /* notify */
  SendMessage(GetParent(ih->handle), WM_COMMAND, MAKEWPARAM(0, BN_CLICKED), (LPARAM)ih->handle);

  /* remove highlight */
  SendMessage(ih->handle, BM_SETSTATE, FALSE, 0);

  /* must force a radio change */
  if (is_toggle && is_radio)
    IupSetAttribute(ih, "VALUE", "TOGGLE");
}

IUP_SDK_API int iupdrvGetScrollbarSize(void)
{
  int xv = GetSystemMetrics(SM_CXVSCROLL);
  int yh = GetSystemMetrics(SM_CYHSCROLL);
  return xv > yh ? xv : yh;
}

IUP_SDK_API void iupdrvReparent(Ihandle* ih)
{
  HWND newParent = iupChildTreeGetNativeParentHandle(ih);
  if (GetParent(ih->handle) != newParent)
  {
    HWND oldParent = SetParent(ih->handle, newParent);
    if (!iupAttribGet(ih, "_IUPWIN_REPARENT"))
      iupAttribSet(ih, "_IUPWIN_REPARENT", (char*)oldParent);
  }
}

IUP_SDK_API void iupdrvBaseLayoutUpdateMethod(Ihandle *ih)
{
  if (ih->currentwidth > 0 && ih->currentheight > 0)
    SetWindowPos(ih->handle, NULL, ih->x, ih->y, ih->currentwidth, ih->currentheight,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
  else /* move only */
    SetWindowPos(ih->handle, NULL, ih->x, ih->y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

IUP_SDK_API void iupdrvRedrawNow(Ihandle *ih)
{
  /* REDRAW Now - IupRedraw */
  RedrawWindow(ih->handle,NULL,NULL,RDW_ERASE|RDW_INVALIDATE|RDW_INTERNALPAINT|RDW_UPDATENOW);
}

IUP_SDK_API void iupdrvPostRedraw(Ihandle *ih)
{
  /* Post a REDRAW - IupUpdate */
  /* can NOT use RDW_NOCHILDREN because IupList has internal children that needs to be redraw */
  RedrawWindow(ih->handle,NULL,NULL,RDW_ERASE|RDW_INVALIDATE|RDW_INTERNALPAINT);  
}

IUP_SDK_API void iupdrvScreenToClient(Ihandle* ih, int *x, int *y)
{
  POINT p;
  p.x = *x;
  p.y = *y;
  ScreenToClient(ih->handle, &p);
  *x = p.x;
  *y = p.y;
}

IUP_SDK_API void iupdrvClientToScreen(Ihandle* ih, int *x, int *y)
{
  POINT p;
  p.x = *x;
  p.y = *y;
  ClientToScreen(ih->handle, &p);
  *x = p.x;
  *y = p.y;
}

void iupwinTrackMouseLeave(Ihandle* ih)
{
  TRACKMOUSEEVENT mouse;
  mouse.cbSize = sizeof(TRACKMOUSEEVENT);
  mouse.dwFlags = TME_LEAVE;
  mouse.hwndTrack = ih->handle;
  mouse.dwHoverTime = 0;
  TrackMouseEvent(&mouse);
}

static void winTrackMouseHoverLeave(Ihandle* ih)
{
  TRACKMOUSEEVENT mouse;
  mouse.cbSize = sizeof(TRACKMOUSEEVENT);
  mouse.dwFlags = TME_HOVER | TME_LEAVE;
  mouse.hwndTrack = ih->handle;
  mouse.dwHoverTime = 10;
  TrackMouseEvent(&mouse);
}

void iupwinMergeStyle(Ihandle* ih, DWORD old_mask, DWORD value)
{
  DWORD dwStyle = GetWindowLong(ih->handle, GWL_STYLE);
  dwStyle &= ~(old_mask); /* clear old bits */
  dwStyle |= value;
  SetWindowLong(ih->handle, GWL_STYLE, dwStyle);
}

void iupwinSetStyle(Ihandle* ih, DWORD value, int set)
{
  DWORD dwStyle = GetWindowLong(ih->handle, GWL_STYLE);
  if (set)
    dwStyle |= value;
  else
    dwStyle &= ~(value);
  SetWindowLong(ih->handle, GWL_STYLE, dwStyle);
}

IUP_DRV_API int iupwinBaseMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{ 
  switch (msg)
  {
  case WM_INPUTLANGCHANGE:
    iupwinKeyInit();
    break;
  case WM_GETDLGCODE:
    {
      *result = DLGC_WANTALLKEYS;
      return 1;
    }
  case WM_NOTIFY: /* usually sent only to parent,               */
    {             /* but TIPs are configured to be handled here */
      NMHDR* msg_info = (NMHDR*)lp;
      if (msg_info->code == TTN_GETDISPINFO)
        iupwinTipsGetDispInfo(lp);
      break;
    }
  case WM_DROPFILES:
    iupwinDropFiles((HDROP)wp, ih);
    break;
  case WM_LBUTTONDOWN:
    if(iupAttribGetBoolean(ih, "DRAGSOURCE"))
    {
      if (iupwinDragDetectStart(ih))
      {
        *result = 0;
        return 1;  /* abort default processing */
      }
    }
    break;
  case WM_HELP:
    {
      Ihandle* child;
      HELPINFO* help_info = (HELPINFO*)lp;

      if (help_info->iContextType == HELPINFO_MENUITEM)
        child = iupwinMenuGetItemHandle((HMENU)help_info->hItemHandle, (int)help_info->iCtrlId);
      else
        child = iupwinHandleGet((HWND)help_info->hItemHandle);

      if (iupObjectCheck(child))
      {
        Icallback cb = (Icallback) IupGetCallback(child, "HELP_CB");
        if (cb) 
        {
          if (cb(child) == IUP_CLOSE) 
            IupExitLoop();

          *result = 0;
          return 1;  /* abort default processing */
        }
      }
      break;
    }
  case WM_MOUSELEAVE:
    {
      Icallback leave_cb = IupGetCallback(ih, "LEAVEWINDOW_CB");
      if (leave_cb)
        leave_cb(ih);

      iupAttribSet(ih, "_IUPWIN_ENTERWIN", NULL);  /* enable tracking again */
      break;
    }
  case WM_MOUSEHOVER:
    {
      Icallback enter_cb = IupGetCallback(ih, "ENTERWINDOW_CB");
      if (enter_cb)
        enter_cb(ih);
      break;
    }
  case WM_MOUSEMOVE:
    {
      /* set tracking only once, but only until track message is processed. */
      if (!iupAttribGet(ih, "_IUPWIN_ENTERWIN"))
      {
        /* set tracking only if enter or leave callbacks are defined */
        Icallback enter_cb = IupGetCallback(ih, "ENTERWINDOW_CB");
        Icallback leave_cb = IupGetCallback(ih, "LEAVEWINDOW_CB");
        if (enter_cb || leave_cb)
        {
          /* must be called so WM_MOUSEHOVER and WM_MOUSELEAVE will be called */
          winTrackMouseHoverLeave(ih);

          iupAttribSet(ih, "_IUPWIN_ENTERWIN", "1");
        }
      }
      break;
    }
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
    if (!iupwinKeyEvent(ih, (int)wp, 1))
    {
      *result = 0;
      return 1;   /* abort default processing */
    }
    break;
  case WM_SYSKEYUP:
  case WM_KEYUP:
    {
      int ret;
      if (wp == VK_SNAPSHOT) /* called only on key up */
      {
        ret = iupwinKeyEvent(ih, (int)wp, 1);
        if (ret && iupObjectCheck(ih))
          ret = iupwinKeyEvent(ih, (int)wp, 0);
      }
      else
        ret = iupwinKeyEvent(ih, (int)wp, 0);

      if (!ret)
      {
        *result = 0;
        return 1;  /* abort default processing */
      }

      break;
    }
  case WM_SETFOCUS:
    /* TODO: Not setting WS_TABSTOP still allows the control 
       to receive the focus when clicked. 
       But this code does make several controls 
       with CANFOCUS=NO to stop working. 
       So we use it inside some of the controls. */
/*  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    {
      HWND previous = (HWND)wp;
      if (previous && previous != ih->handle)
        SetFocus(previous);
      break;
    }  */
    iupwinWmSetFocus(ih);
    break;
  case WM_KILLFOCUS:
    iupCallKillFocusCb(ih);
    break;
  case WM_SETCURSOR: 
    { 
      if (ih->handle == (HWND)wp && LOWORD(lp)==HTCLIENT)
      {
        HCURSOR hCur = (HCURSOR)iupAttribGet(ih, "_IUPWIN_HCURSOR");
        if (hCur)
        {
          SetCursor(hCur); 
          *result = 1;
          return 1;
        }
        else if (iupAttribGet(ih, "CURSOR"))
        {
          SetCursor(NULL); 
          *result = 1;
          return 1;
        }
      }
      break; 
    } 
  case WM_TOUCH:
    /* TODO: 
     - considering touch messages are greedy, one window got it all?
     - how this work? only for the dialog, or also for the children?
     - should a container forward to its children?
    */
    if (LOWORD(wp))
      iupwinTouchProcessInput(ih, (int)LOWORD(wp), (void*)lp);
    break;
  case WOM_CLOSE:
  case WOM_DONE:
  case WOM_OPEN:
    {
      IFni cb = (IFni)IupGetCallback(ih, "WOM_CB");
      if (cb)
      {
        int v = -2; /* Error */
        switch(msg)
        {
        case WOM_OPEN:  v = 1;  break;
        case WOM_DONE:  v = 0;  break;
        case WOM_CLOSE: v = -1; break;
        }
        cb(ih, v);
      }
      break;
    }
  }

  return 0;
}

LRESULT CALLBACK iupwinBaseWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{   
  int ret;
  LRESULT result = 0;
  IwinMsgProc MsgProc;
  Ihandle *ih;
  WNDPROC oldProc;

  ih = iupwinHandleGet(hwnd); 
  if (!iupObjectCheck(ih))
    return DefWindowProc(hwnd, msg, wp, lp);  /* should never happen */

  /* retrieve the control previous procedure for subclassing */
  oldProc = (WNDPROC)IupGetCallback(ih, "_IUPWIN_OLDWNDPROC_CB");

  /* check if the element defines a custom procedure */
  MsgProc = (IwinMsgProc)IupGetCallback(ih, "_IUPWIN_CTRLMSGPROC_CB");
  if (MsgProc)
    ret = MsgProc(ih, msg, wp, lp, &result);
  else
    ret = iupwinBaseMsgProc(ih, msg, wp, lp, &result);

  if (ret)
    return result;
  else 
    return CallWindowProc(oldProc, hwnd, msg, wp, lp);
}

static Ihandle* winContainerWmCommandGetIhandle(Ihandle *ih, WPARAM wp, LPARAM lp)
{
  /* WPARAM - if HIWORD is 0 if the message is from a menu.
              or HIWORD is 1 if the message is from an accelerator.
              or HIWORD is the notification code if the message is from a control.
              LOWORD is the identifier.
     LPARAM - the control sending the message or 0. */

  Ihandle *child = NULL;

  if (HIWORD(wp)==0 && lp==0 && LOWORD(wp)>10)
  {
    Ihandle* dlg_menu = IupGetAttributeHandle(ih, "MENU");
    if (dlg_menu)
      child = iupwinMenuGetItemHandle((HMENU)dlg_menu->handle, LOWORD(wp));  /* menu */
  }
  else 
  {
    if (lp==0)
      child = ih;                              /* native parent */
    else
    {
      child = iupwinHandleGet((HWND)lp);       /* control */
      if (!iupObjectCheck(child))
        child = iupwinHandleGet(GetParent((HWND)lp));       /* control */
    }
  }

  return child;
}

static int winCheckParent(Ihandle* child, Ihandle* ih)
{
  Ihandle* parent = iupChildTreeGetNativeParent(child);
  if (parent==ih)
    return 1;
  else
  {
    /* TODO: this is weird... */
    HWND oldParent = (HWND)iupAttribGet(child, "_IUPWIN_REPARENT");
    if (oldParent && oldParent==ih->handle)
      return 1;

    return 0;
  }
}

int iupwinBaseContainerMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  /* All messages here are sent to the parent Window, 
     but they are useful for child controls.  */

  switch (msg)
  {
  case WM_COMMAND:
    {
      Ihandle* child = winContainerWmCommandGetIhandle(ih, wp, lp);
      if (child)
      {
        IFwmCommand cb = (IFwmCommand)IupGetCallback(child, "_IUPWIN_COMMAND_CB");
        if (cb)
          cb(child, wp, lp);
      }

      break;
    }
  case WM_CTLCOLOREDIT:
  case WM_CTLCOLORLISTBOX:
  case WM_CTLCOLORBTN:
  case WM_CTLCOLORSCROLLBAR:
  case WM_CTLCOLORSTATIC:
    {
      Ihandle* child = iupwinHandleGet((HWND)lp);
      if (iupObjectCheck(child) && winCheckParent(child, ih))
      {
        IFctlColor cb = (IFctlColor)IupGetCallback(child, "_IUPWIN_CTLCOLOR_CB");
        if (cb)
          return cb(child, (HDC)wp, result);
      }
      break;
    }
  case WM_DRAWITEM:   /* for OWNERDRAW controls */
    {
      Ihandle *child;
      DRAWITEMSTRUCT *drawitem = (LPDRAWITEMSTRUCT)lp;
      if (!drawitem)
        break;

      if (wp == 0) /* a menu */
        child = iupwinMenuGetItemHandle((HMENU)drawitem->hwndItem, drawitem->itemID);
      else
      {
        child = iupwinHandleGet(drawitem->hwndItem); 
        if (iupObjectCheck(child) && !winCheckParent(child, ih))
          child = NULL;
      }

      if (child)
      {
        IFdrawItem cb = (IFdrawItem)IupGetCallback(child, "_IUPWIN_DRAWITEM_CB");
        if (cb) 
        {
          cb(child, (void*)drawitem);
          *result = TRUE;
          return 1;
        }
      }
      break;
    }
  case WM_HSCROLL:
  case WM_VSCROLL:
    {
      Ihandle *child = iupwinHandleGet((HWND)lp);
      if (iupObjectCheck(child) && winCheckParent(child, ih))
      {
        IFni cb = (IFni)IupGetCallback(child, "_IUPWIN_CUSTOMSCROLL_CB");
        if (cb)
          cb(child, LOWORD(wp));
      }
      break;
    }
  case WM_NOTIFY:   /* Currently, the following controls support custom draw functionality: 
                       Header, List-view, Rebar, Toolbar, ToolTip, Trackbar, Tree-view.
                       And for Button if using Windows XP Style. */
    {
      Ihandle *child;
      NMHDR* msg_info = (NMHDR*)lp;
      if (!msg_info)
        break;

      child = iupwinHandleGet(msg_info->hwndFrom);
      if (iupObjectCheck(child) && winCheckParent(child, ih))
      {
        IFnotify cb = (IFnotify)IupGetCallback(child, "_IUPWIN_NOTIFY_CB");
        if (cb)
        {
          int ret;
          if (cb(child, (void*)msg_info, &ret))
          {
            *result = (LRESULT)ret;
            return 1;
          }
        }
      }

      break;
    }
  case WM_MOUSEWHEEL:
  {
    /* Notice that this message is sent to the focus window. Also it will be automatically propagated to the native parent. 
       But we want it to be sent to the canvas under the cursor.
       If this is a canvas based container, the message was already processed. 
       So, if it is not a canvas based container, we will do the propagation to the canvas under the cursor, if any.
       But this can potentially generate an infinite loop, so we use _IUP_WHEEL_PROPAGATING as a stop condition. 
       */
    if (!IupClassMatch(ih, "canvas"))
    {
      HWND hChild;
      POINT p;

      p.x = GET_X_LPARAM(lp); p.y = GET_Y_LPARAM(lp);
      ScreenToClient(ih->handle, &p);

      hChild = ChildWindowFromPointEx(ih->handle, p, CWP_SKIPDISABLED | CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);
      if (hChild)
      {
        Ihandle* child = iupwinHandleGet(hChild);
        if (iupObjectCheck(child) && IupClassMatch(child, "canvas"))
        {
          if (!iupAttribGet(child, "_IUP_WHEEL_PROPAGATING")) /* to avoid a parent to propagate again to the child */
            SendMessage(child->handle, WM_MOUSEWHEEL, wp, lp);
          else
            iupAttribSet(child, "_IUP_WHEEL_PROPAGATING", NULL);

          *result = 0; /* process the message and abort parent propagation */
          return 1;
        }
      }
    }
    break;
  }
  default:
    {
      /* sent to the list parent */
      if (msg == WM_DRAGLISTMSG)
      {
        DRAGLISTINFO* lpDrag = (DRAGLISTINFO*) lp;
        Ihandle *child = iupwinHandleGet(lpDrag->hWnd);
        if (iupObjectCheck(child) && winCheckParent(child, ih))
        {
          *result = iupwinListDND(child, lpDrag->uNotification, lpDrag->ptCursor);
          return 1;
        }
      }
      break;
    }
  }

  return iupwinBaseMsgProc(ih, msg, wp, lp, result);
}

void iupwinChangeWndProc(Ihandle *ih, WNDPROC newProc)
{
  IupSetCallback(ih, "_IUPWIN_OLDWNDPROC_CB", (Icallback)GetWindowLongPtr(ih->handle, GWLP_WNDPROC));
  SetWindowLongPtr(ih->handle, GWLP_WNDPROC, (LONG_PTR)newProc);
}

IUP_SDK_API void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  WNDPROC oldProc = (WNDPROC)IupGetCallback(ih, "_IUPWIN_OLDWNDPROC_CB");
  if (oldProc)
  {
    SetWindowLongPtr(ih->handle, GWLP_WNDPROC, (LONG_PTR)oldProc);
    IupSetCallback(ih, "_IUPWIN_OLDWNDPROC_CB",  NULL);
  }

  iupwinTipsDestroy(ih);
  iupwinDestroyDragDrop(ih);

  /* remove the association before destroying */
  iupwinHandleRemove(ih->handle);

  /* destroys window (it will remove from parent) */
  DestroyWindow(ih->handle);
}

int iupwinGetColorRef(Ihandle *ih, char *name, COLORREF *color)
{
  unsigned char r, g, b;
  /* must use IupGetAttribute to use inheritance */
  if (iupStrToRGB(IupGetAttribute(ih, name), &r, &g, &b))
  {
    *color = RGB(r,g,b);
    return 1;
  }
  return 0;
}

int iupwinGetParentBgColor(Ihandle* ih, COLORREF* cr)
{
  unsigned char r, g, b;
  char* color = iupBaseNativeParentGetBgColorAttrib(ih);
  if (iupStrToRGB(color, &r, &g, &b))
  {
    *cr = RGB(r,g,b);
    return 1;
  }
  return 0;
}

IUP_SDK_API int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  if (IsWindowVisible(ih->handle))
  {
    if (iupStrEqualNoCase(value, "TOP"))
      SetWindowPos(ih->handle, HWND_TOP, 0, 0, 0, 0, 
                  SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
    else
      SetWindowPos(ih->handle, HWND_BOTTOM, 0, 0, 0, 0, 
                  SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
  }

  return 0;
}

IUP_SDK_API void iupdrvSetVisible(Ihandle* ih, int visible)
{
  ShowWindow(ih->handle, visible? SW_SHOWNORMAL: SW_HIDE);
}

IUP_SDK_API int iupdrvIsVisible(Ihandle* ih)
{
  return IsWindowVisible(ih->handle);
}

IUP_SDK_API int iupdrvIsActive(Ihandle* ih)
{
  return IsWindowEnabled(ih->handle);
}

IUP_SDK_API void iupdrvSetActive(Ihandle* ih, int enable)
{
  EnableWindow(ih->handle, enable);
}

int iupwinSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    SetWindowText(ih->handle, TEXT(""));
  else
    SetWindowText(ih->handle, iupwinStrToSystem(value));
  return 1;
}

IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle *ih, const char* title)
{
  if (!title) 
    SetWindowText(ih->handle, TEXT(""));
  else
    SetWindowText(ih->handle, iupwinStrToSystem(title));
}

void iupwinSetMnemonicTitle(Ihandle *ih, int pos, const char* value)
{
  int c;

  if (!value) 
    value = "";

  c = iupStrFindMnemonic(value);
  if (c != 0)
  {
    if (ih->iclass->nativetype != IUP_TYPEMENU)
      iupKeySetMnemonic(ih, c, pos);
  }
}

TCHAR* iupwinGetWindowText(HWND hWnd)
{
  int len = GetWindowTextLength(hWnd);
  if (len)
  {
    TCHAR* str = (TCHAR*)iupStrGetMemory((len+1)*sizeof(TCHAR));
    GetWindowText(hWnd, str, len+1);
    return str;
  }
  else
    return NULL;
}

#ifndef IDC_HAND
#define IDC_HAND            MAKEINTRESOURCE(32649)
#endif
#ifndef IDC_APPSTARTING
#define IDC_APPSTARTING     MAKEINTRESOURCE(32650) 
#endif
#ifndef IDC_HELP
#define IDC_HELP            MAKEINTRESOURCE(32651)
#endif

static HCURSOR winLoadComCtlCursor(LPCTSTR lpCursorName)
{
  HCURSOR cur = NULL;
  HINSTANCE hinstDll = LoadLibrary(TEXT("comctl32.dll"));
  if (hinstDll)
  {
    cur = LoadCursor(hinstDll, lpCursorName);
    FreeLibrary(hinstDll);
  }
  return cur;
}

HCURSOR iupwinGetCursor(Ihandle* ih, const char* name)
{
  static struct {
    const char* iupname;
    const TCHAR* sysname;
  } table[] = {
    {"NONE",      NULL}, 
    {"NULL",      NULL}, 
    {"ARROW",     IDC_ARROW},
    {"BUSY",      IDC_WAIT},
    {"CROSS",     IDC_CROSS},
    {"HAND",      IDC_HAND},
    {"MOVE",      IDC_SIZEALL},
    {"RESIZE_N",  IDC_SIZENS},
    {"RESIZE_S",  IDC_SIZENS},
    {"RESIZE_NS", IDC_SIZENS},
    {"RESIZE_W",  IDC_SIZEWE},
    {"RESIZE_E",  IDC_SIZEWE},
    {"RESIZE_WE", IDC_SIZEWE},
    {"RESIZE_NE", IDC_SIZENESW},
    {"RESIZE_SE", IDC_SIZENWSE},
    {"RESIZE_NW", IDC_SIZENWSE},
    {"RESIZE_SW", IDC_SIZENESW},
    {"TEXT",      IDC_IBEAM},
    {"HELP",      IDC_HELP},
    {"IUP",       IDC_HELP},
    {"NO",        IDC_NO},
    {"UPARROW",   IDC_UPARROW},
    {"APPSTARTING", IDC_APPSTARTING}
  };

  HCURSOR cur;
  char str[200];
  int i, count = sizeof(table)/sizeof(table[0]);

  /* check the cursor cache first (per control)*/
  sprintf(str, "_IUPWIN_CURSOR_%s", name);
  cur = (HCURSOR)iupAttribGet(ih, str);
  if (cur)
    return cur;

  /* check the pre-defined IUP names first */
  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(name, table[i].iupname)) 
    {
      if (table[i].sysname)
        cur = LoadCursor(NULL, table[i].sysname);
      else
        cur = NULL;

      break;
    }
  }

  if (i == count)
  {
    /* check other system cursors */

    if (iupStrEqualNoCase(name, "PEN"))
      name = "CURSOR_PEN"; /* name in "iup.rc" */

    /* check for an name defined cursor */
    cur = iupImageGetCursor(name);
  }

  if (!cur)
  {
    if (iupStrEqualNoCase(name, "SPLITTER_VERT"))
      cur = winLoadComCtlCursor(MAKEINTRESOURCE(107));
    else if (iupStrEqualNoCase(name, "SPLITTER_HORIZ"))
      cur = winLoadComCtlCursor(MAKEINTRESOURCE(135));
  }

  iupAttribSet(ih, str, (char*)cur);
  return cur;
}

void iupwinRefreshCursor(Ihandle* ih)
{
  SendMessage(ih->handle, WM_SETCURSOR, (WPARAM)ih->handle, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
}

IUP_SDK_API int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  /* Cursor can be NULL in Windows. */
  HCURSOR hCur = iupwinGetCursor(ih, value);
  iupAttribSet(ih, "_IUPWIN_HCURSOR", (char*)hCur);  /* To be used in WM_SETCURSOR */
  /* refresh the cursor */
  iupwinRefreshCursor(ih);
  return 1;
}

IUP_SDK_API void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "HFONT", iupwinGetHFontAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);

  if (!WM_DRAGLISTMSG)
    WM_DRAGLISTMSG = RegisterWindowMessage(DRAGLISTMSGSTRING);
}

IUP_SDK_API void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
  if (iupwinIsWin7OrNew())
    iupwinTouchRegisterAttrib(ic);

  iupClassRegisterAttribute(ic, "TIPBALLOON", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TIPBALLOONTITLE", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TIPBALLOONTITLEICON", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_MAPPED);
}

IUP_DRV_API int iupwinButtonDown(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp)
{
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  int ret, doubleclick = 0;
  int b = 0;

  IFniiiis cb = (IFniiiis) IupGetCallback(ih, "BUTTON_CB");
  if (!cb)
    return 0;

  if (msg==WM_XBUTTONDBLCLK ||
      msg==WM_LBUTTONDBLCLK ||
      msg==WM_MBUTTONDBLCLK ||
      msg==WM_RBUTTONDBLCLK)
    doubleclick = 1;

  iupwinButtonKeySetStatus(LOWORD(wp), status, doubleclick);

  if (msg==WM_LBUTTONDOWN || msg==WM_LBUTTONDBLCLK)
    b = IUP_BUTTON1;
  else if (msg==WM_MBUTTONDOWN || msg==WM_MBUTTONDBLCLK)
    b = IUP_BUTTON2;
  else if (msg==WM_RBUTTONDOWN || msg==WM_RBUTTONDBLCLK)
    b = IUP_BUTTON3;
  else if (msg==WM_XBUTTONDOWN || msg==WM_XBUTTONDBLCLK)
  {
    if (HIWORD(wp) == XBUTTON1)
      b = IUP_BUTTON4;
    else
      b = IUP_BUTTON5;
  }

  ret = cb(ih, b, 1, GET_X_LPARAM(lp), GET_Y_LPARAM(lp), status);
  if (ret == IUP_CLOSE)
    IupExitLoop();
  else if (ret == IUP_IGNORE)
    return -1;

  return 1;
}

IUP_DRV_API void iupwinFlagButtonDown(Ihandle* ih, UINT msg)
{
  if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK)
    iupAttribSet(ih, "_IUP_WM_XBUTTONDOWN", "1");
  else if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK)
    iupAttribSet(ih, "_IUP_WM_LBUTTONDOWN", "1");
  else if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK)
    iupAttribSet(ih, "_IUP_WM_MBUTTONDOWN", "1");
  else if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK)
    iupAttribSet(ih, "_IUP_WM_RBUTTONDOWN", "1");
}

IUP_DRV_API int iupwinFlagButtonUp(Ihandle* ih, UINT msg)
{
  /* return false if a match button down was NOT flagged */

  if (msg == WM_XBUTTONDOWN)
  {
    if (!iupAttribGet(ih, "_IUP_WM_XBUTTONDOWN"))
      return 0;
    else
      iupAttribSet(ih, "_IUP_WM_XBUTTONDOWN", NULL);
  }
  else if (msg == WM_LBUTTONUP)
  {
    if (!iupAttribGet(ih, "_IUP_WM_LBUTTONDOWN"))
      return 0;
    else
      iupAttribSet(ih, "_IUP_WM_LBUTTONDOWN", NULL);
  }
  else if (msg == WM_MBUTTONDOWN)
  {
    if (!iupAttribGet(ih, "_IUP_WM_MBUTTONDOWN"))
      return 0;
    else
      iupAttribSet(ih, "_IUP_WM_MBUTTONDOWN", NULL);
  }
  else if (msg == WM_RBUTTONDOWN)
  {
    if (!iupAttribGet(ih, "_IUP_WM_RBUTTONDOWN"))
      return 0;
    else
      iupAttribSet(ih, "_IUP_WM_RBUTTONDOWN", NULL);
  }
  return 1;
}

IUP_DRV_API int iupwinButtonUp(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp)
{
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  int ret, b=0;
  IFniiiis cb = (IFniiiis) IupGetCallback(ih, "BUTTON_CB");
  if (!cb)
    return 0;

  iupwinButtonKeySetStatus(LOWORD(wp), status, 0);

  /* also updates the button status, since wp could not have the flag */
  if (msg==WM_LBUTTONUP)
  {
    b = IUP_BUTTON1;
    iupKEY_SETBUTTON1(status);  
  }
  else if (msg==WM_MBUTTONUP)
  {
    b = IUP_BUTTON2;
    iupKEY_SETBUTTON2(status);
  }
  else if (msg==WM_RBUTTONUP)
  {
    b = IUP_BUTTON3;
    iupKEY_SETBUTTON3(status);
  }
  else if (msg==WM_XBUTTONUP)
  {
    if (HIWORD(wp) == XBUTTON1)
    {
      b = IUP_BUTTON4;
      iupKEY_SETBUTTON4(status);
    }
    else
    {
      b = IUP_BUTTON5;
      iupKEY_SETBUTTON5(status);
    }
  }

  ret = cb(ih, b, 0, GET_X_LPARAM(lp), GET_Y_LPARAM(lp), status);
  if (ret == IUP_CLOSE)
    IupExitLoop();
  else if (ret == IUP_IGNORE)
    return -1;

  return 1;
}

IUP_DRV_API int iupwinMouseMove(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp)
{
  IFniis cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
  if (cb)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupwinButtonKeySetStatus(LOWORD(wp), status, 0);
    cb(ih, GET_X_LPARAM(lp), GET_Y_LPARAM(lp), status);
    return 1;
  }
  (void)msg;
  return 0;
}

void iupwinGetNativeParentStyle(Ihandle* ih, DWORD *dwExStyle, DWORD *dwStyle)
{
  *dwStyle |= WS_CLIPCHILDREN;

  if (iupAttribGetBoolean(IupGetDialog(ih), "COMPOSITED"))
  {
    /* Paints all descendants of a window in bottom-to-top painting order using double-buffering. 
       This cannot be used if the window has a class style of either CS_OWNDC or CS_CLASSDC. 
       IMPORTANT: IupCanvas uses CS_OWNDC. 
       And CD can draw at any time in the canvas, it is incompatible with the flag, see:
       https://blogs.msdn.microsoft.com/oldnewthing/20171018-00/?p=97245 */
    *dwExStyle |= WS_EX_COMPOSITED;
  }
}

HWND iupwinCreateWindowEx(HWND hParent, LPCTSTR lpClassName, DWORD dwExStyle, DWORD dwStyle, int serial, void* clientdata)
{
  return CreateWindowEx(dwExStyle,  /* extended window style */
    lpClassName,                  /* window class */
    NULL,                         /* title */
    dwStyle,                      /* window style */
    0,                            /* x-position */
    0,                            /* y-position */
    10,                           /* horizontal size - set this to avoid initial size problems */
    10,                           /* vertical size */
    hParent,                      /* window parent */
    (HMENU)serial,                /* child identifier */
    iupwin_hinstance,             /* instance of app. */
    clientdata);
}

IUP_DRV_API int iupwinCreateWindow(Ihandle* ih, LPCTSTR lpClassName, DWORD dwExStyle, DWORD dwStyle, void* clientdata)
{
  const int serial = iupAttribGetInt(ih, "CONTROLID");
  ih->serial = (serial ? serial : iupDialogGetChildId(ih));

  ih->handle = iupwinCreateWindowEx(iupChildTreeGetNativeParentHandle(ih), lpClassName, dwExStyle, dwStyle, ih->serial, clientdata);
  if (!ih->handle)
    return 0;

  /* associate HWND with Ihandle*, all Win32 controls must call this. */
  iupwinHandleAdd(ih, ih->handle);

  /* replace the WinProc to handle base callbacks */
  iupwinChangeWndProc(ih, iupwinBaseWndProc);

  return 1;
}

IUP_SDK_API void iupdrvSendKey(int key, int press)
{
  unsigned int keyval, state;
  INPUT input[2];
  LPARAM extra_info;
  WORD state_scan = 0, key_scan;
  ZeroMemory(input, 2*sizeof(INPUT));

  iupdrvKeyEncode(key, &keyval, &state);
  if (!keyval)
    return;

  extra_info = GetMessageExtraInfo();
  if (state)
    state_scan = (WORD)MapVirtualKey(state, MAPVK_VK_TO_VSC);
  key_scan = (WORD)MapVirtualKey(keyval, MAPVK_VK_TO_VSC);

  if (press & 0x01)
  {
    if (state)
    {
      /* modifier first */
      input[0].type = INPUT_KEYBOARD;
      input[0].ki.wVk = (WORD)state;
      input[0].ki.wScan = state_scan;
      input[0].ki.dwExtraInfo = extra_info;

      /* key second */
      input[1].type = INPUT_KEYBOARD;
      input[1].ki.wVk = (WORD)keyval;
      input[1].ki.wScan = key_scan;
      input[1].ki.dwExtraInfo = extra_info;

      SendInput(2, input, sizeof(INPUT));
    }
    else
    {
      input[0].type = INPUT_KEYBOARD;
      input[0].ki.wVk = (WORD)keyval;
      input[0].ki.wScan = key_scan;
      input[0].ki.dwExtraInfo = extra_info;

      SendInput(1, input, sizeof(INPUT));
    }
  }

  if (press & 0x02)
  {
    if (state)
    {
      /* key first */
      input[0].type = INPUT_KEYBOARD;
      input[0].ki.dwFlags = KEYEVENTF_KEYUP;
      input[0].ki.wVk = (WORD)keyval;
      input[0].ki.wScan = key_scan;
      input[0].ki.dwExtraInfo = extra_info;

      /* modifier second */
      input[1].type = INPUT_KEYBOARD;
      input[1].ki.dwFlags = KEYEVENTF_KEYUP;
      input[1].ki.wVk = (WORD)state;
      input[1].ki.wScan = state_scan;
      input[1].ki.dwExtraInfo = extra_info;

      SendInput(2, input, sizeof(INPUT));
    }
    else
    {
      input[0].type = INPUT_KEYBOARD;
      input[0].ki.dwFlags = KEYEVENTF_KEYUP;
      input[0].ki.wVk = (WORD)keyval;
      input[0].ki.wScan = key_scan;
      input[0].ki.dwExtraInfo = extra_info;

      SendInput(1, input, sizeof(INPUT));
    }
  }
}

IUP_SDK_API void iupdrvWarpPointer(int x, int y)
{
  iupdrvAddScreenOffset(&x, &y, 1);
  SetCursorPos(x, y);
}

static int winGetButtonStatus(int bt, int status)
{
  switch(bt)
  {
  case IUP_BUTTON1:
    return (status==0)? MOUSEEVENTF_LEFTUP: MOUSEEVENTF_LEFTDOWN;
  case IUP_BUTTON2:
    return (status==0)? MOUSEEVENTF_MIDDLEUP: MOUSEEVENTF_MIDDLEDOWN;
  case IUP_BUTTON3:
    return (status==0)? MOUSEEVENTF_RIGHTUP: MOUSEEVENTF_RIGHTDOWN;
  case IUP_BUTTON4:
    return (status==0)? MOUSEEVENTF_XUP: MOUSEEVENTF_XDOWN;
  case IUP_BUTTON5:
    return (status==0)? MOUSEEVENTF_XUP: MOUSEEVENTF_XDOWN;
  default:
    return 0;
  }
}

IUP_SDK_API void iupdrvSendMouse(int x, int y, int bt, int status)
{
  INPUT input;
  ZeroMemory(&input, sizeof(INPUT));

  input.type = INPUT_MOUSE;
  input.mi.dx = x;
  input.mi.dy = y;
  input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
  input.mi.dwExtraInfo = GetMessageExtraInfo();

  /* PROBLEMS:
    Menu items are not activated. Although submenus open, menu items even in the menu bar are not activated.
    Inside the FileOpen dialog, clicks in the folder navigation list are not correctly interpreted.
  */

  if (status==-1)
  {
    input.mi.dwFlags |= MOUSEEVENTF_MOVE;
  }
  else
  {
    input.mi.dwFlags |= winGetButtonStatus(bt, status);

    switch(bt)
    {
    case 'W':
      input.mi.mouseData = status*120;
      input.mi.dwFlags |= MOUSEEVENTF_WHEEL;
      break;
    case IUP_BUTTON4:
      input.mi.mouseData = XBUTTON1;
      break;
    case IUP_BUTTON5:
      input.mi.mouseData = XBUTTON2;
      break;
    }
  }

  if (status == 2)  /* double click */
  {
    SendInput(1, &input, sizeof(INPUT));  /* press */

    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
    input.mi.dwFlags |= winGetButtonStatus(bt, 0);
    SendInput(1, &input, sizeof(INPUT));  /* release */

    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
    input.mi.dwFlags |= winGetButtonStatus(bt, 1);
    SendInput(1, &input, sizeof(INPUT));  /* press */

    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
    input.mi.dwFlags |= winGetButtonStatus(bt, 0);
    SendInput(1, &input, sizeof(INPUT));  /* release */
  }
  else
    SendInput(1, &input, sizeof(INPUT));

  /* always update cursor */
  /* in Windows, update cursor after mouse messages */
  /* this will NOT generate and extra motion event */
  iupdrvWarpPointer(x, y);
}

int iupwinSetAutoRedrawAttrib(Ihandle* ih, const char* value)
{
  SendMessage(ih->handle, WM_SETREDRAW, (WPARAM)iupStrBoolean(value), 0);
  return 1;
}

IUP_SDK_API void iupdrvSleep(int time)
{
  Sleep(time);
}

void iupwinDrawFocusRect(HDC hDC, int x, int y, int w, int h)
{
  RECT rect;

  rect.left = x;
  rect.top = y;
  rect.right = x + w;
  rect.bottom = y + h;

  DrawFocusRect(hDC, &rect);
}

/* Unused in Windows */
IUP_SDK_API int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

/* Unused in Windows */
IUP_SDK_API int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}
