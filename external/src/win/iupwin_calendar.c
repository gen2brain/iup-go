/** \file
 * \brief Calendar Control
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
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_mask.h"
#include "iup_array.h"
#include "iup_text.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_str.h"
#include "iupwin_info.h"


#ifndef MCS_NOSELCHANGEONNAV
#define MCS_NOSELCHANGEONNAV 0x0100
#define MCS_SHORTDAYSOFWEEK  0x0080
#endif

#if NOT_WORKING
#ifndef MCM_GETCALENDARBORDER
#define MCM_SETCALENDARBORDER (MCM_FIRST + 30)
#define MCM_GETCALENDARBORDER (MCM_FIRST + 31)
#endif

static int winCalendarSetBorderAttrib(Ihandle* ih, const char* value)
{
  int border;
  if (iupStrToInt(value, &border))
    SendMessage(ih->handle, MCM_SETCALENDARBORDER, TRUE, (LPARAM)border);  /* NOT working */
  else
    SendMessage(ih->handle, MCM_SETCALENDARBORDER, FALSE, 0);
  return 0; /* do not store value in hash table */
}

static char* winCalendarGetBorderAttrib(Ihandle* ih)
{
  int border = (int)SendMessage(ih->handle, MCM_GETCALENDARBORDER, 0, 0);
  return iupStrReturnInt(border);
}

static int winCalendarSetBackColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    COLORREF cr = RGB(r, g, b);
    SendMessage(ih->handle, MCM_SETCOLOR, MCSC_BACKGROUND, (LPARAM)cr);  /* works only when NOT using Visual Styles */
    SendMessage(ih->handle, MCM_SETCOLOR, MCSC_MONTHBK, (LPARAM)cr);
    SendMessage(ih->handle, MCM_SETCOLOR, MCSC_TITLEBK, (LPARAM)cr);
  }
  return 0; /* do not store value in hash table */
}

static char* winCalendarGetBackColorAttrib(Ihandle* ih)
{
  COLORREF cr = (COLORREF)SendMessage(ih->handle, MCM_GETCOLOR, MCSC_BACKGROUND, 0);
  return iupStrReturnStrf("%d %d %d", (int)GetRValue(cr), (int)GetGValue(cr), (int)GetBValue(cr));
}

static int winCalendarSetForeColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    COLORREF cr = RGB(r, g, b);
    SendMessage(ih->handle, MCM_SETCOLOR, MCSC_TEXT, (LPARAM)cr);  /* works only when NOT using Visual Styles */
    SendMessage(ih->handle, MCM_SETCOLOR, MCSC_TITLETEXT, (LPARAM)cr);
    SendMessage(ih->handle, MCM_SETCOLOR, MCSC_TRAILINGTEXT, (LPARAM)cr);
  }
  return 0; /* do not store value in hash table */
}

static char* winCalendarGetForeColorAttrib(Ihandle* ih)
{
  COLORREF cr = (COLORREF)SendMessage(ih->handle, MCM_GETCOLOR, MCSC_TEXT, 0);
  return iupStrReturnStrf("%d %d %d", (int)GetRValue(cr), (int)GetGValue(cr), (int)GetBValue(cr));
}
#endif

static int winCalendarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "TODAY"))
  {
    SYSTEMTIME st;
    GetLocalTime(&st);
    SendMessage(ih->handle, MCM_SETCURSEL, 0, (LPARAM)&st);
  }
  else
  {
    int year, month, day;
    if (sscanf(value, "%d/%d/%d", &year, &month, &day) == 3)
    {
      SYSTEMTIME st;

      if (month < 1) month = 1;
      if (month > 12) month = 12;
      if (day < 1) day = 1;
      if (day > 31) day = 31;

      st.wYear = (WORD)year;
      st.wMonth = (WORD)month;
      st.wDay = (WORD)day;

      SendMessage(ih->handle, MCM_SETCURSEL, 0, (LPARAM)&st);
    }
  }
  return 0; /* do not store value in hash table */
}

static char* winCalendarGetValueAttrib(Ihandle* ih)
{
  SYSTEMTIME st;
  SendMessage(ih->handle, MCM_GETCURSEL, 0, (LPARAM)&st);
  return iupStrReturnStrf("%d/%d/%d", st.wYear, st.wMonth, st.wDay);
}

static char* winCalendarGetTodayAttrib(Ihandle* ih)
{
  SYSTEMTIME st;
  (void)ih;
  GetLocalTime(&st);
  return iupStrReturnStrf("%d/%d/%d", st.wYear, st.wMonth, st.wDay);
}

static void winCalendarComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  (void)children_expand; /* unset if not a container */

  if (ih->handle)
  {
    RECT rect;
    SendMessage(ih->handle, MCM_GETMINREQRECT, 0, (LPARAM)&rect);
    *w =  rect.right - rect.left;
    *h = rect.bottom - rect.top;
  }
  else
  {
    iupdrvFontGetMultiLineStringSize(ih, "WW", w, h);

    *h += 2;

    (*w) *= 7; /* 7 columns */
    (*h) *= 8; /* 8 lines */

    *h += 2;

    iupdrvTextAddBorders(ih, w, h);
  }
}


static int winCalendarWmNotify(Ihandle* ih, NMHDR* msg_info, int *result)
{
  if (msg_info->code == MCN_SELECT)
  {
    iupBaseCallValueChangedCb(ih);
  }

  (void)result;
  return 0; /* result not used */
}

static int winCalendarMapMethod(Ihandle* ih)
{
  DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS | MCS_NOTODAY;

  if (!ih->parent)
    return IUP_ERROR;

  if (iupwinIsVistaOrNew())
    dwStyle |= MCS_NOSELCHANGEONNAV | MCS_SHORTDAYSOFWEEK;

  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    dwStyle |= WS_TABSTOP;

  if (iupAttribGetBoolean(ih, "WEEKNUMBERS"))
    dwStyle |= MCS_WEEKNUMBERS;

  if (!iupwinCreateWindow(ih, MONTHCAL_CLASS, 0, dwStyle, NULL))
    return IUP_ERROR;

  /* Process WM_NOTIFY */
  IupSetCallback(ih, "_IUPWIN_NOTIFY_CB", (Icallback)winCalendarWmNotify);

  return IUP_NOERROR;
}

Iclass* iupCalendarNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "calendar";
  ic->format = NULL; /* none */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupCalendarNewClass;

  ic->Map = winCalendarMapMethod;
  ic->ComputeNaturalSize = winCalendarComputeNaturalSizeMethod;

  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  /* Common Callbacks */
  iupBaseRegisterCommonCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* IupCalendar only */
  iupClassRegisterAttribute(ic, "VALUE", winCalendarGetValueAttrib, winCalendarSetValueAttrib, NULL, "TODAY", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WEEKNUMBERS", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", winCalendarGetTodayAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

#if NOT_WORKING
  /* Works only when NOT using Visual Styles */
  iupClassRegisterAttribute(ic, "BACKCOLOR", winCalendarGetBackColorAttrib, winCalendarSetBackColorAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORECOLOR", winCalendarGetForeColorAttrib, winCalendarSetForeColorAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* NOT working */
  iupClassRegisterAttribute(ic, "BORDER", winCalendarGetBorderAttrib, winCalendarSetBorderAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
#endif

  iupClassRegisterAttribute(ic, "CONTROLID", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupCalendar(void)
{
  return IupCreate("calendar");
}
