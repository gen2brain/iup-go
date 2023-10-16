/** \file
 * \brief Valuator Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <commctrl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_val.h"
#include "iup_drv.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_draw.h"
#include "iupwin_str.h"


void iupdrvValGetMinSize(Ihandle* ih, int *w, int *h)
{
  /* LAYOUT_DECORATION_ESTIMATE */
  int ticks_size = 0;
  if (iupAttribGetInt(ih, "SHOWTICKS"))
  {
    char* tickspos = iupAttribGetStr(ih, "TICKSPOS");
    if(iupStrEqualNoCase(tickspos, "BOTH"))
      ticks_size = 2*8;
    else 
      ticks_size = 8;
  }

  if (ih->data->orientation == IVAL_HORIZONTAL)
  {
    *w = 35;
    *h = 30+ticks_size;
  }
  else
  {
    *w = 30+ticks_size;
    *h = 35;
  }
}

static int winValSetBgColorAttrib(Ihandle *ih, const char *value)
{
  (void)value;
  iupdrvPostRedraw(ih);
  return 1;
}

static int winValSetStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->step), 0.01))
  {
    int linesize = (int)(ih->data->step*SHRT_MAX);
    SendMessage(ih->handle, TBM_SETLINESIZE, 0, linesize);
  }
  return 0; /* do not store value in hash table */
}

static int winValSetPageStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1))
  {
    int pagesize = (int)(ih->data->pagestep*SHRT_MAX);
    SendMessage(ih->handle, TBM_SETPAGESIZE, 0, pagesize);
  }
  return 0; /* do not store value in hash table */
}

static int winValSetShowTicksAttrib(Ihandle* ih, const char* value)
{
  int tick_freq, show_ticks;

  if (!ih->data->show_ticks)  /* can only set if already not zero */
    return 0;

  show_ticks = atoi(value);
  if (show_ticks<2) show_ticks=2;
  ih->data->show_ticks = show_ticks;

  /* Defines the interval frequency for tick marks */
  tick_freq = SHRT_MAX/(show_ticks-1);
  SendMessage(ih->handle, TBM_SETTICFREQ, tick_freq, 0);
  return 0;
}

static int winValSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    int ival;

    iupValCropValue(ih);

    ival = (int)(((ih->data->val - ih->data->vmin) / (ih->data->vmax - ih->data->vmin))*SHRT_MAX);
    if (ih->data->inverted)
      ival = SHRT_MAX - ival;

    SendMessage(ih->handle, TBM_SETPOS, TRUE, ival);
  }
  return 0; /* do not store value in hash table */
}


/*********************************************************************************************/


static int winValCustomScroll(Ihandle* ih, int msg)
{
  double old_val = ih->data->val;
  int ival;
  IFn cb;

  ival = (int)SendMessage(ih->handle, TBM_GETPOS, 0, 0);
  if (ih->data->inverted)
    ival = SHRT_MAX-ival;

  ih->data->val = (((double)ival/(double)SHRT_MAX)*(ih->data->vmax - ih->data->vmin)) + ih->data->vmin;
  iupValCropValue(ih);

  cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
  {
    if (ih->data->val == old_val)
      return 0;

    cb(ih);
  }
  else
  {
    IFnd cb_old = NULL;
    switch (msg)
    {
      case TB_BOTTOM:
      case TB_TOP:
      case TB_LINEDOWN:
      case TB_LINEUP:
      case TB_PAGEDOWN:
      case TB_PAGEUP:
      {
        cb_old = (IFnd) IupGetCallback(ih, "BUTTON_PRESS_CB");
        break;
      }
      case TB_THUMBPOSITION:
      {
        cb_old = (IFnd) IupGetCallback(ih, "BUTTON_RELEASE_CB");
        break;
      }
      case TB_THUMBTRACK:
      {
        cb_old = (IFnd) IupGetCallback(ih, "MOUSEMOVE_CB");
        break;
      }
    }
    if (cb_old)
      cb_old(ih, ih->data->val);
  }

  return 0; /* not used */
}

static void winValIncPageValue(Ihandle *ih, int dir)
{
  int pagesize, ival;
  pagesize = (int)(ih->data->pagestep*SHRT_MAX);

  ival = (int)SendMessage(ih->handle, TBM_GETPOS, 0, 0);
  ival += dir*pagesize;
  if (ival < 0) ival = 0;
  if (ival > SHRT_MAX) ival = SHRT_MAX;
  SendMessage(ih->handle, TBM_SETPOS, TRUE, ival);

  winValCustomScroll(ih, 0);
}

static int winValCtlColor(Ihandle* ih, HDC hdc, LRESULT *result)
{
  COLORREF cr;
  if (iupwinGetParentBgColor(ih, &cr))
  {
    SetBkColor(hdc, cr);
    SetDCBrushColor(hdc, cr);
    *result = (LRESULT)GetStockObject(DC_BRUSH);
    return 1;
  }
  return 0;
}

static int winValMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  (void)lp;

  switch (msg)
  {
  case WM_SETFOCUS:
    {
      if (!iupAttribGetBoolean(ih, "CANFOCUS"))
      {
        HWND previous = (HWND)wp;
        if (previous && previous != ih->handle)
        {
          SetFocus(previous);
          *result = 0;
          return 1;
        }
      }
      break;
    }
  case WM_ERASEBKGND:
    {
      RECT rect;
      HDC hDC = (HDC)wp;
      GetClientRect(ih->handle, &rect); 
      iupwinDrawParentBackground(ih, hDC, &rect);
      /* return non zero value */
      *result = 1;
      return 1;
    }
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
    {
      if (iupwinBaseMsgProc(ih, msg, wp, lp, result)==1)
        return 1;

      if (!iupObjectCheck(ih))
      {
        *result = 0;
        return 1;
      }

      if (GetKeyState(VK_CONTROL) & 0x8000)  /* handle Ctrl+Arrows */
      {
        if (wp == VK_UP || wp == VK_LEFT)
        {
          winValIncPageValue(ih, -1);
          *result = 0;
          return 1;
        }
        if (wp == VK_RIGHT || wp == VK_DOWN)
        {
          winValIncPageValue(ih, 1);
          *result = 0;
          return 1;
        }
      }

      return 0;
    }
  }

  return iupwinBaseMsgProc(ih, msg, wp, lp, result);
}

static int winValBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  HWND tips_hwnd;

  iupdrvBaseSetTipAttrib(ih, value);

  tips_hwnd = (HWND)iupAttribGet(ih, "_IUPWIN_TIPSWIN");
  SendMessage(ih->handle, TBM_SETTOOLTIPS, (WPARAM)tips_hwnd, 0);
  return 1;
}


/*********************************************************************************************/


static int winValMapMethod(Ihandle* ih)
{
  DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS | TBS_AUTOTICKS;
  int show_ticks;

  if (!ih->parent)
    return IUP_ERROR;

  /* Track bar Orientation */
  if (ih->data->orientation == IVAL_HORIZONTAL)
    dwStyle |= TBS_HORZ;
  else
    dwStyle |= TBS_VERT;

  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    dwStyle |= WS_TABSTOP;

  /* Track bar Ticks */
  show_ticks = iupAttribGetInt(ih, "SHOWTICKS");
  if (!show_ticks)
  {
    dwStyle |= TBS_NOTICKS;    /* No show_ticks */
  }
  else
  {
    char* tickspos;

    if (show_ticks<2) show_ticks=2;
    ih->data->show_ticks = show_ticks; /* non zero value, can be changed later, but not to zero */

    /* Defines the position of tick marks */
    tickspos = iupAttribGetStr(ih, "TICKSPOS");
    if(iupStrEqualNoCase(tickspos, "BOTH"))
      dwStyle |= TBS_BOTH;
    else if(iupStrEqualNoCase(tickspos, "REVERSE"))
      dwStyle |= TBS_BOTTOM;  /* same as TBS_RIGHT */
    else /* NORMAL */
      dwStyle |= TBS_TOP;     /* same as TBS_LEFT  */
  }

  if (!iupwinCreateWindow(ih, TRACKBAR_CLASS, 0, dwStyle, NULL))
    return IUP_ERROR;

  /* Process Keyboard */
  IupSetCallback(ih, "_IUPWIN_CTRLMSGPROC_CB", (Icallback)winValMsgProc);

  /* Process Val Scroll commands */
  IupSetCallback(ih, "_IUPWIN_CUSTOMSCROLL_CB", (Icallback)winValCustomScroll);

  /* Process background color */
  IupSetCallback(ih, "_IUPWIN_CTLCOLOR_CB", (Icallback)winValCtlColor);

  /* configure the native range */
  SendMessage(ih->handle, TBM_SETRANGEMIN, FALSE, 0);
  SendMessage(ih->handle, TBM_SETRANGEMAX, FALSE, SHRT_MAX);

  if (ih->data->inverted)
    SendMessage(ih->handle, TBM_SETPOS, FALSE, SHRT_MAX);  /* default initial position is at MIN */

  winValSetPageStepAttrib(ih, NULL);
  winValSetStepAttrib(ih, NULL);

  return IUP_NOERROR;
}

void iupdrvValInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = winValMapMethod;

  iupClassRegisterAttribute(ic, "TIP", NULL, winValBaseSetTipAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupVal only */
  iupClassRegisterAttribute(ic, "VALUE", iupValGetValueAttrib, winValSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);  
  iupClassRegisterAttribute(ic, "SHOWTICKS", iupValGetShowTicksAttrib, winValSetShowTicksAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupValGetPageStepAttrib, winValSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);  /* force new default value */
  iupClassRegisterAttribute(ic, "STEP", iupValGetStepAttrib, winValSetStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);   /* force new default value */

  iupClassRegisterAttribute(ic, "TICKSPOS", NULL, NULL, "NORMAL", NULL, IUPAF_DEFAULT);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, winValSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "CONTROLID", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
