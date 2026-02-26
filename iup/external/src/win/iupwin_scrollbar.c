/** \file
 * \brief Scrollbar Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>

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
#include "iup_scrollbar.h"
#include "iup_drv.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_draw.h"
#include "iupwin_str.h"


#define IWIN_SB_MAX 32767

void iupdrvScrollbarGetMinSize(Ihandle* ih, int *w, int *h)
{
  int sb_size = GetSystemMetrics(SM_CXVSCROLL);

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    *w = 3 * sb_size;
    *h = sb_size;
  }
  else
  {
    *w = sb_size;
    *h = 3 * sb_size;
  }
}

static void winScrollbarSetScrollInfo(Ihandle* ih)
{
  SCROLLINFO si;
  double range = ih->data->vmax - ih->data->vmin;
  int ipage, ipos;

  if (range <= 0)
    return;

  ipage = (int)((ih->data->pagesize / range) * IWIN_SB_MAX);
  if (ipage < 1) ipage = 1;

  ipos = (int)(((ih->data->val - ih->data->vmin) / range) * IWIN_SB_MAX);
  if (ih->data->inverted)
    ipos = IWIN_SB_MAX - ipage - ipos;

  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
  si.nMin = 0;
  si.nMax = IWIN_SB_MAX;
  si.nPage = ipage;
  si.nPos = ipos;

  SetScrollInfo(ih->handle, SB_CTL, &si, TRUE);
}

static int winScrollbarSetLineStepAttrib(Ihandle* ih, const char* value)
{
  iupStrToDoubleDef(value, &(ih->data->linestep), 0.01);
  return 0;
}

static int winScrollbarSetPageStepAttrib(Ihandle* ih, const char* value)
{
  iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1);
  return 0;
}

static int winScrollbarSetPageSizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagesize), 0.1))
  {
    iupScrollbarCropValue(ih);
    winScrollbarSetScrollInfo(ih);
  }
  return 0;
}

static int winScrollbarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    iupScrollbarCropValue(ih);
    winScrollbarSetScrollInfo(ih);
  }
  return 0;
}


/*********************************************************************************************/


static int winScrollbarProcessScroll(Ihandle* ih, int msg)
{
  double old_val = ih->data->val;
  double range = ih->data->vmax - ih->data->vmin;
  int op = -1;
  SCROLLINFO si;

  si.cbSize = sizeof(SCROLLINFO);

  if (msg == SB_THUMBTRACK || msg == SB_THUMBPOSITION)
    si.fMask = SIF_PAGE | SIF_TRACKPOS;
  else
    si.fMask = SIF_PAGE | SIF_POS;

  GetScrollInfo(ih->handle, SB_CTL, &si);

  {
    int ipos;
    int ipage = si.nPage;

    if (msg == SB_THUMBTRACK || msg == SB_THUMBPOSITION)
      ipos = si.nTrackPos;
    else
      ipos = si.nPos;

    switch (msg)
    {
    case SB_LINEUP:
      {
        int linestep = (int)((ih->data->linestep / range) * IWIN_SB_MAX);
        if (linestep < 1) linestep = 1;
        ipos -= linestep;
        if (ipos < 0) ipos = 0;
        break;
      }
    case SB_LINEDOWN:
      {
        int linestep = (int)((ih->data->linestep / range) * IWIN_SB_MAX);
        if (linestep < 1) linestep = 1;
        ipos += linestep;
        if (ipos > IWIN_SB_MAX - ipage) ipos = IWIN_SB_MAX - ipage;
        break;
      }
    case SB_PAGEUP:
      ipos -= ipage;
      if (ipos < 0) ipos = 0;
      break;
    case SB_PAGEDOWN:
      ipos += ipage;
      if (ipos > IWIN_SB_MAX - ipage) ipos = IWIN_SB_MAX - ipage;
      break;
    case SB_TOP:
      ipos = 0;
      break;
    case SB_BOTTOM:
      ipos = IWIN_SB_MAX - ipage;
      break;
    }

    if (ih->data->inverted)
      ipos = IWIN_SB_MAX - ipage - ipos;

    ih->data->val = ((double)ipos / (double)IWIN_SB_MAX) * range + ih->data->vmin;
    iupScrollbarCropValue(ih);

    if (msg != SB_THUMBTRACK)
      winScrollbarSetScrollInfo(ih);
  }

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    switch (msg)
    {
    case SB_LINEUP:        op = IUP_SBLEFT; break;
    case SB_LINEDOWN:      op = IUP_SBRIGHT; break;
    case SB_PAGEUP:        op = IUP_SBPGLEFT; break;
    case SB_PAGEDOWN:      op = IUP_SBPGRIGHT; break;
    case SB_THUMBTRACK:    op = IUP_SBDRAGH; break;
    case SB_THUMBPOSITION: op = IUP_SBPOSH; break;
    default:               op = IUP_SBPOSH; break;
    }
  }
  else
  {
    switch (msg)
    {
    case SB_LINEUP:        op = IUP_SBUP; break;
    case SB_LINEDOWN:      op = IUP_SBDN; break;
    case SB_PAGEUP:        op = IUP_SBPGUP; break;
    case SB_PAGEDOWN:      op = IUP_SBPGDN; break;
    case SB_THUMBTRACK:    op = IUP_SBDRAGV; break;
    case SB_THUMBPOSITION: op = IUP_SBPOSV; break;
    default:               op = IUP_SBPOSV; break;
    }
  }

  {
    IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
    if (scroll_cb)
    {
      float posx = 0, posy = 0;
      if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
        posx = (float)ih->data->val;
      else
        posy = (float)ih->data->val;

      scroll_cb(ih, op, posx, posy);
    }
  }

  {
    IFn valuechanged_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
    if (valuechanged_cb)
    {
      if (ih->data->val != old_val)
        valuechanged_cb(ih);
    }
  }

  return 0;
}

static int winScrollbarMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
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
  }

  return iupwinBaseMsgProc(ih, msg, wp, lp, result);
}

static int winScrollbarCustomScroll(Ihandle* ih, int msg)
{
  return winScrollbarProcessScroll(ih, msg);
}


/*********************************************************************************************/


static int winScrollbarMapMethod(Ihandle* ih)
{
  DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS;

  if (!ih->parent)
    return IUP_ERROR;

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
    dwStyle |= SBS_HORZ;
  else
    dwStyle |= SBS_VERT;

  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    dwStyle |= WS_TABSTOP;

  if (!iupwinCreateWindow(ih, TEXT("SCROLLBAR"), 0, dwStyle, NULL))
    return IUP_ERROR;

  IupSetCallback(ih, "_IUPWIN_CTRLMSGPROC_CB", (Icallback)winScrollbarMsgProc);
  IupSetCallback(ih, "_IUPWIN_CUSTOMSCROLL_CB", (Icallback)winScrollbarCustomScroll);

  winScrollbarSetScrollInfo(ih);

  return IUP_NOERROR;
}

void iupdrvScrollbarInitClass(Iclass* ic)
{
  ic->Map = winScrollbarMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", iupScrollbarGetValueAttrib, winScrollbarSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINESTEP", iupScrollbarGetLineStepAttrib, winScrollbarSetLineStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupScrollbarGetPageStepAttrib, winScrollbarSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESIZE", iupScrollbarGetPageSizeAttrib, winScrollbarSetPageSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CONTROLID", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
