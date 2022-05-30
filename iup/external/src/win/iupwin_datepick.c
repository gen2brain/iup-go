/** \file
 * \brief DatePick Control
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

#ifndef DTM_FIRST
#define DTM_FIRST        0x1000
#endif

#ifndef DTM_SETMCSTYLE
#define DTM_SETMCSTYLE   (DTM_FIRST + 11)
#define DTM_CLOSEMONTHCAL (DTM_FIRST + 13)
#define DTM_GETIDEALSIZE (DTM_FIRST + 15)
#endif
#ifndef MCS_NOSELCHANGEONNAV
#define MCS_NOSELCHANGEONNAV 0x0100
#endif


static int winDatePickSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "TODAY"))
  {
    SYSTEMTIME st;
    GetLocalTime(&st);
    SendMessage(ih->handle, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
  }
  else
  {
    int year, month, day;
    if (sscanf(value, "%d/%d/%d", &year, &month, &day) == 3)
    {
      SYSTEMTIME st;
      ZeroMemory(&st, sizeof(SYSTEMTIME));

      if (month < 1) month = 1;
      if (month > 12) month = 12;
      if (day < 1) day = 1;
      if (day > 31) day = 31;

      st.wYear = (WORD)year;
      st.wMonth = (WORD)month;
      st.wDay = (WORD)day;

      SendMessage(ih->handle, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    }
  }
  return 0; /* do not store value in hash table */
}

static char* winDatePickGetValueAttrib(Ihandle* ih)
{
  SYSTEMTIME st;
  SendMessage(ih->handle, DTM_GETSYSTEMTIME, 0, (LPARAM)&st);
  return iupStrReturnStrf("%d/%d/%d", st.wYear, st.wMonth, st.wDay);
}

static int winDatePickSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    HFONT hFont = (HFONT)SendMessage(ih->handle, WM_GETFONT, 0, 0);
    SendMessage(ih->handle, DTM_SETMCFONT, (WPARAM)hFont, (LPARAM)TRUE); /* not working in Windows 10 (not tested in 7 or 8) - works in XP */
  }

  return 1;
}

static char* winDatePickGetTodayAttrib(Ihandle* ih)
{
  SYSTEMTIME st;
  (void)ih;
  GetLocalTime(&st);
  return iupStrReturnStrf("%d/%d/%d", st.wYear, st.wMonth, st.wDay);
}

static int winDatePickSetFormatAttrib(Ihandle* ih, const char* value)
{
  SendMessage(ih->handle, DTM_SETFORMAT, 0, (LPARAM)iupwinStrToSystem(value));
  return 1;
}

static int winDatePickSetOrderAttrib(Ihandle* ih, const char* value)
{
  int i;
  char format[50] = "";
  char* separator = iupAttribGetStr(ih, "SEPARATOR");
  int zeropreced = iupAttribGetBoolean(ih, "ZEROPRECED");
  int monthshortnames = iupAttribGetBoolean(ih, "MONTHSHORTNAMES");

  if (!value || strlen(value) != 3)
    return 0;

  for (i = 0; i < 3; i++)
  {
    if (value[i] == 'D' || value[i] == 'd')
    {
      if (zeropreced)
        strcat(format, "dd");
      else
        strcat(format, "d");
    }
    else if (value[i] == 'M' || value[i] == 'm')
    {
      if (monthshortnames)
        strcat(format, "MMM");
      else if (zeropreced)
        strcat(format, "MM");
      else
        strcat(format, "M");
    }
    else if (value[i] == 'Y' || value[i] == 'y')
      strcat(format, "yyyy");
    else
      return 0;

    if (i < 2)
    {
      strcat(format, "'");
      strcat(format, separator);
      strcat(format, "'");
    }
  }

  IupSetStrAttribute(ih, "FORMAT", format);
  return 1;
}

static int winDatePickSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  if (!iupStrBoolean(value))
    SendMessage(ih->handle, DTM_CLOSEMONTHCAL, 0, 0);

  return 0;
}

/*********************************************************************************************/


static int winDatePickWmNotify(Ihandle* ih, NMHDR* msg_info, int *result)
{
  if (msg_info->code == DTN_DATETIMECHANGE)
  {
    HWND drop_down = (HWND)SendMessage(ih->handle, DTM_GETMONTHCAL, 0, 0);
    if (drop_down && !iupAttribGet(ih, "_IUP_DROPFIRST"))
    {
      /* when user uses the dropdown, notification is called twice, avoid the first one */
      iupAttribSet(ih, "_IUP_DROPFIRST", "1");
      return 0;
    }

    iupAttribSetStr(ih, "_IUP_DROPFIRST", NULL);

    iupBaseCallValueChangedCb(ih);
  }

  (void)result;
  return 0; /* result not used */
}


/*********************************************************************************************/

static void winDatePickComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  (void)children_expand; /* unset if not a container */

  if (ih->handle && iupwinIsVistaOrNew() && iupwin_comctl32ver6)
  {
    SIZE size;
    SendMessage(ih->handle, DTM_GETIDEALSIZE, 0, (LPARAM)&size);
    *w = size.cx;
    *h = size.cy;
  }
  else
  {
    iupdrvFontGetMultiLineStringSize(ih, "WW/WW/WWWW", w, h);
    iupdrvTextAddBorders(ih, w, h);
    *w += iupdrvGetScrollbarSize();
  }
}

static int winDatePickMapMethod(Ihandle* ih)
{
  DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS | DTS_SHORTDATECENTURYFORMAT;

  if (!ih->parent)
    return IUP_ERROR;

  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    dwStyle |= WS_TABSTOP;

  if (!iupwinCreateWindow(ih, DATETIMEPICK_CLASS, 0, dwStyle, NULL))
    return IUP_ERROR;

  /* Process WM_NOTIFY */
  IupSetCallback(ih, "_IUPWIN_NOTIFY_CB", (Icallback)winDatePickWmNotify);

  if (iupwinIsVistaOrNew())
  {
    dwStyle = MCS_NOTODAY | MCS_NOSELCHANGEONNAV;

    if (iupAttribGetBoolean(ih, "CALENDARWEEKNUMBERS"))
      dwStyle |= MCS_WEEKNUMBERS;

    SendMessage(ih->handle, DTM_SETMCSTYLE, 0, (LPARAM)dwStyle);
  }

  if (iupAttribGet(ih, "SEPARATOR") || iupAttribGet(ih, "ZEROPRECED") || iupAttribGet(ih, "MONTHSHORTNAMES"))
    winDatePickSetOrderAttrib(ih, "DMY");

  return IUP_NOERROR;
}

Iclass* iupDatePickNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "datepick";
  ic->cons = "DatePick";
  ic->format = NULL;  /* no parameters */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupDatePickNewClass;

  ic->Map = winDatePickMapMethod;
  ic->ComputeNaturalSize = winDatePickComputeNaturalSizeMethod;

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

  iupClassRegisterAttribute(ic, "FONT", NULL, winDatePickSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);  /* inherited */

  iupClassRegisterAttribute(ic, "VALUE", winDatePickGetValueAttrib, winDatePickSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", winDatePickGetTodayAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SEPARATOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "/", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZEROPRECED", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORDER", NULL, winDatePickSetOrderAttrib, IUPAF_SAMEASSYSTEM, "DMY", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, winDatePickSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* Windows Only */
  iupClassRegisterAttribute(ic, "MONTHSHORTNAMES", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", NULL, winDatePickSetFormatAttrib, "d'/'M'/'yyyy", NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CALENDARWEEKNUMBERS", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CONTROLID", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  return ic;
}

Ihandle *IupDatePick(void)
{
  return IupCreate("datepick");
}
