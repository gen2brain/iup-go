/** \file
 * \brief Progress bar Control
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
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_progressbar.h"
#include "iup_drv.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_str.h"
#include "iupwin_draw.h"


/* Not defined in Cygwin and MingW */
#ifndef PBS_MARQUEE            
#define PBS_MARQUEE             0x08
#define PBM_SETMARQUEE          (WM_USER+10)
#endif

#define IUP_PB_MAX 32000


static void winProgressBarDraw(Ihandle* ih, HDC hDC)
{
  RECT rect;
  HBRUSH hBrush;
  COLORREF bgcolor, fgcolor;
  unsigned char r, g, b;
  char* value;
  int width, height;
  int is_vertical = iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL");
  int progress;
  double factor;

  GetClientRect(ih->handle, &rect);
  width = rect.right - rect.left;
  height = rect.bottom - rect.top;

  value = iupAttribGetStr(ih, "BGCOLOR");
  if (iupStrToRGB(value, &r, &g, &b))
    bgcolor = RGB(r, g, b);
  else
    bgcolor = GetSysColor(COLOR_3DFACE);

  value = iupAttribGetStr(ih, "FGCOLOR");
  if (iupStrToRGB(value, &r, &g, &b))
    fgcolor = RGB(r, g, b);
  else
    fgcolor = GetSysColor(COLOR_HIGHLIGHT);

  hBrush = CreateSolidBrush(bgcolor);
  FillRect(hDC, &rect, hBrush);
  DeleteObject(hBrush);

  if (!ih->data->marquee)
  {
    factor = (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);

    if (is_vertical)
    {
      progress = (int)(height * factor);
      SetRect(&rect, 0, height - progress, width, height);
    }
    else
    {
      progress = (int)(width * factor);
      SetRect(&rect, 0, 0, progress, height);
    }

    if (progress > 0)
    {
      hBrush = CreateSolidBrush(fgcolor);
      FillRect(hDC, &rect, hBrush);
      DeleteObject(hBrush);
    }
  }
}

static int winProgressBarMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  if (msg == WM_PAINT)
  {
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(ih->handle, &ps);
    winProgressBarDraw(ih, hDC);
    EndPaint(ih->handle, &ps);
    *result = 0;
    return 1;
  }

  if (msg == WM_ERASEBKGND)
  {
    *result = 1;
    return 1;
  }

  return iupwinBaseMsgProc(ih, msg, wp, lp, result);
}

static int winProgressBarSetMarqueeAttrib(Ihandle* ih, const char* value)
{
  /* MARQUEE only works when using XP Styles */
  if (!iupwin_comctl32ver6)
    return 0;

  if (iupStrBoolean(value))
    SendMessage(ih->handle, PBM_SETMARQUEE, TRUE, 100);
  else
    SendMessage(ih->handle, PBM_SETMARQUEE, FALSE, 0);

  return 1;
}

static int winProgressBarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    ih->data->value = 0;
  else
    iupStrToDouble(value, &(ih->data->value));

  iProgressBarCropValue(ih);

  if (!ih->data->marquee)
  {
    if (iupwin_comctl32ver6)
    {
      iupdrvRedrawNow(ih);
    }
    else
    {
      double factor = (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);
      int val = (int)(IUP_PB_MAX * factor);
      SendMessage(ih->handle, PBM_SETPOS, (WPARAM)val, 0);
    }
  }

  return 0;
}

static int winProgressBarSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (iupwin_comctl32ver6)
  {
    iupdrvRedrawNow(ih);
    return 1;
  }

  if (iupStrToRGB(value, &r, &g, &b))
  {
    COLORREF color = RGB(r,g,b);
    SendMessage(ih->handle, PBM_SETBKCOLOR, 0, (LPARAM)color);
  }
  else
    SendMessage(ih->handle, PBM_SETBKCOLOR, 0, (LPARAM)CLR_DEFAULT);
  return 1;
}

static int winProgressBarSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (iupwin_comctl32ver6)
  {
    iupdrvRedrawNow(ih);
    return 1;
  }

  if (iupStrToRGB(value, &r, &g, &b))
  {
    COLORREF color = RGB(r,g,b);
    SendMessage(ih->handle, PBM_SETBARCOLOR, 0, (LPARAM)color);
  }
  else
    SendMessage(ih->handle, PBM_SETBARCOLOR, 0, (LPARAM)CLR_DEFAULT);
  return 1;
}

static int winProgressBarMapMethod(Ihandle* ih)
{
  DWORD dwStyle = WS_CHILD|WS_CLIPSIBLINGS;

  if (!ih->parent)
    return IUP_ERROR;

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    dwStyle |= PBS_VERTICAL;

    if (ih->userheight < ih->userwidth)
    {
      int tmp = ih->userheight;
      ih->userheight = ih->userwidth;
      ih->userwidth = tmp;
    }
  }

  if (!iupwin_comctl32ver6 && !iupAttribGetBoolean(ih, "DASHED"))
    dwStyle |= PBS_SMOOTH;

  if (iupwin_comctl32ver6 && iupAttribGetBoolean(ih, "MARQUEE"))
  {
    dwStyle |= PBS_MARQUEE;
    ih->data->marquee = 1;
  }

  if (!iupwinCreateWindow(ih, PROGRESS_CLASS, 0, dwStyle, NULL))
    return IUP_ERROR;

  if (iupwin_comctl32ver6 && !ih->data->marquee)
  {
    iupwinDrawRemoveTheme(ih->handle);
    IupSetCallback(ih, "_IUPWIN_CTRLMSGPROC_CB", (Icallback)winProgressBarMsgProc);
  }

  SendMessage(ih->handle, PBM_SETRANGE, 0, MAKELPARAM(0, IUP_PB_MAX));

  return IUP_NOERROR;
}

void iupdrvProgressBarInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = winProgressBarMapMethod;

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, winProgressBarSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, winProgressBarSetFgColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* IupProgressBar only */
  iupClassRegisterAttribute(ic, "VALUE",  iProgressBarGetValueAttrib,  winProgressBarSetValueAttrib,  NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARQUEE",     NULL, winProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DASHED",      NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CONTROLID", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
