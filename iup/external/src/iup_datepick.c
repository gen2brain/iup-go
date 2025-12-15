/** \file
 * \brief DatePick Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_register.h"
#include "iup_childtree.h"
#include "iup_mask.h"
#include "iup_array.h"
#include "iup_text.h"


static int is_leap_year(int year)
{
  if (year % 400 == 0)
    return 1;
  else if (year % 100 == 0)
    return 0;
  else if (year % 4 == 0)
    return 1;
  else
    return 0;
}

static void iDatePickUpdateDayLimits(Ihandle* ih)
{
  Ihandle* txt_month = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_MONTH");
  Ihandle* txt_day = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_DAY");
  int day = IupGetInt(txt_day, "VALUE");
  int month = IupGetInt(txt_month, "VALUE");
  if (month == 2)
  {
    Ihandle* txt_year = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_YEAR");
    int year = IupGetInt(txt_year, "VALUE");
    if (is_leap_year(year))
    {
      IupSetAttribute(txt_day, "MASKINT", "1:29");
      if (day > 29)
        IupSetInt(txt_day, "VALUE", 29);
    }
    else
    {
      IupSetAttribute(txt_day, "MASKINT", "1:28");
      if (day > 28)
        IupSetInt(txt_day, "VALUE", 28);
    }
  }
  else if (month == 4 || month == 6 || month == 9 || month == 11)
  {
    IupSetAttribute(txt_day, "MASKINT", "1:30");
    if (day > 30)
      IupSetInt(txt_day, "VALUE", 30);
  }
  else
    IupSetAttribute(txt_day, "MASKINT", "1:31");
}

static int iDatePickCalendarValueChanged_CB(Ihandle* ih_calendar)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(ih_calendar, "_IUP_DATEPICK");
  Ihandle* ih_toggle = (Ihandle*)iupAttribGet(ih_calendar, "_IUP_DATEPICK_TOGGLE");
  Ihandle* popover = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER");

  IupSetStrAttribute(ih, "VALUE", IupGetAttribute(ih_calendar, "VALUE"));

  iupBaseCallValueChangedCb(ih);

  IupSetAttribute(ih_toggle, "VALUE", "OFF");
  if (popover)
    IupSetAttribute(popover, "VISIBLE", "NO");

  return IUP_DEFAULT;
}

static int iDatePickPopoverShow_CB(Ihandle* popover, int state)
{
  if (state == IUP_HIDE)
  {
    Ihandle* ih_toggle = (Ihandle*)iupAttribGet(popover, "_IUP_DATEPICK_TOGGLE");
    if (ih_toggle)
      IupSetAttribute(ih_toggle, "VALUE", "OFF");
  }
  return IUP_DEFAULT;
}

static int iDatePickToggleAction_CB(Ihandle* ih_toggle, int state)
{
  Ihandle* ih = IupGetParent(IupGetParent(ih_toggle));
  Ihandle* popover = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER");
  Ihandle* calendar = (Ihandle*)iupAttribGet(ih, "_IUP_CALENDAR");

  if (state == 1)
  {
    if (!popover)
    {
      char* weeknumbers;

      calendar = IupCalendar();
      weeknumbers = iupAttribGet(ih, "CALENDARWEEKNUMBERS");
      if (weeknumbers) IupSetStrAttribute(calendar, "WEEKNUMBERS", weeknumbers);
      IupSetCallback(calendar, "VALUECHANGED_CB", iDatePickCalendarValueChanged_CB);
      iupAttribSet(calendar, "_IUP_DATEPICK", (char*)ih);
      iupAttribSet(calendar, "_IUP_DATEPICK_TOGGLE", (char*)ih_toggle);
      iupAttribSet(ih, "_IUP_CALENDAR", (char*)calendar);

      popover = IupPopover(calendar);
      IupSetAttributeHandle(popover, "ANCHOR", ih);
      IupSetAttribute(popover, "POSITION", "BOTTOM");
      IupSetAttribute(popover, "ARROW", "NO");
      IupSetAttribute(popover, "AUTOHIDE", "NO");
      IupSetCallback(popover, "SHOW_CB", (Icallback)iDatePickPopoverShow_CB);
      iupAttribSet(popover, "_IUP_DATEPICK_TOGGLE", (char*)ih_toggle);
      iupAttribSet(ih, "_IUP_POPOVER", (char*)popover);
    }

    IupSetStrAttribute(calendar, "VALUE", IupGetAttribute(ih, "VALUE"));
    IupSetAttribute(popover, "VISIBLE", "YES");
  }
  else
  {
    if (popover)
      IupSetAttribute(popover, "VISIBLE", "NO");
  }

  return IUP_DEFAULT;
}

static int iDatePickTextValueChanged_CB(Ihandle* ih_text)
{
  Ihandle* ih = IupGetParent(IupGetParent(ih_text));

  if ((Ihandle*)iupAttribGet(ih, "_IUP_DATE_DAY") != ih_text)
    iDatePickUpdateDayLimits(ih);

  iupBaseCallValueChangedCb(ih);
  return IUP_DEFAULT;
}

static int iDatePickTextKAny_CB(Ihandle* ih_text, int key)
{
  Ihandle* ih = IupGetParent(IupGetParent(ih_text));

  if (key == K_UP || key == K_plus || key == K_sPlus)
  {
    int value = IupGetInt(ih_text, "VALUE");
    value++;
    if (iupAttribGetBoolean(ih, "ZEROPRECED"))
      IupSetStrf(ih_text, "VALUEMASKED", "%02d", value);
    else
      IupSetInt(ih_text, "VALUEMASKED", value);

    if (IupGetInt(ih_text, "VALUE") == value)
      iDatePickTextValueChanged_CB(ih_text);
    return IUP_IGNORE;
  }
  else if (key == K_DOWN || key == K_minus || key == K_sMinus)
  {
    int value = IupGetInt(ih_text, "VALUE");
    value--;
    if (iupAttribGetBoolean(ih, "ZEROPRECED"))
      IupSetStrf(ih_text, "VALUEMASKED", "%02d", value);
    else
      IupSetInt(ih_text, "VALUEMASKED", value);

    if (IupGetInt(ih_text, "VALUE") == value)
      iDatePickTextValueChanged_CB(ih_text);
    return IUP_IGNORE;
  }
  else if (key == K_LEFT)
  {
    int caret = IupGetInt(ih_text, "CARET");
    if (caret == 1)
    {
      int pos = IupGetChildPos(IupGetParent(ih_text), ih_text);
      if (pos == 2)
      {
        Ihandle* next = IupGetChild(IupGetParent(ih_text), 0);
        int count = IupGetInt(next, "COUNT");
        IupSetFocus(next);
        IupSetInt(next, "CARET", count + 1);
      }
      else if (pos == 4)
      {
        Ihandle* next = IupGetChild(IupGetParent(ih_text), 2);
        int count = IupGetInt(next, "COUNT");
        IupSetFocus(next);
        IupSetInt(next, "CARET", count + 1);
      }
    }
  }
  else if (key == K_RIGHT)
  {
    int caret = IupGetInt(ih_text, "CARET");
    int count = IupGetInt(ih_text, "COUNT");
    if (caret == count + 1)
    {
      int pos = IupGetChildPos(IupGetParent(ih_text), ih_text);
      if (pos == 0)
      {
        Ihandle* next = IupGetChild(IupGetParent(ih_text), 2);
        IupSetFocus(next);
        IupSetInt(next, "CARET", 1);
      }
      else if (pos == 2)
      {
        Ihandle* next = IupGetChild(IupGetParent(ih_text), 4);
        IupSetFocus(next);
        IupSetInt(next, "CARET", 1);
      }
    }
  }
  return IUP_CONTINUE;
}


/*********************************************************************************************/


static int iDatePickSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  iDatePickToggleAction_CB(ih, iupStrBoolean(value));
  return 0;
}

static int iDatePickSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "TODAY"))
  {
    Ihandle* txt_year = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_YEAR");
    Ihandle* txt_month = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_MONTH");
    Ihandle* txt_day = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_DAY");
    struct tm * timeinfo;
    time_t timer;
    time(&timer);
    timeinfo = localtime(&timer);

    if (iupAttribGetBoolean(ih, "ZEROPRECED"))
    {
      IupSetInt(txt_year, "VALUE", timeinfo->tm_year + 1900);
      IupSetStrf(txt_month, "VALUE", "%02d", timeinfo->tm_mon + 1);
      IupSetStrf(txt_day, "VALUE", "%02d", timeinfo->tm_mday);
    }
    else
    {
      IupSetInt(txt_year, "VALUE", timeinfo->tm_year + 1900);
      IupSetInt(txt_month, "VALUE", timeinfo->tm_mon + 1);
      IupSetInt(txt_day, "VALUE", timeinfo->tm_mday);
    }

    iDatePickUpdateDayLimits(ih);
  }
  else
  {
    int year, month, day;
    if (sscanf(value, "%d/%d/%d", &year, &month, &day) == 3)
    {
      Ihandle* txt_year = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_YEAR");
      Ihandle* txt_month = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_MONTH");
      Ihandle* txt_day = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_DAY");

      if (month < 1) month = 1;
      if (month > 12) month = 12;

      IupSetInt(txt_year, "VALUE", year);
      if (iupAttribGetBoolean(ih, "ZEROPRECED"))
        IupSetStrf(txt_month, "VALUE", "%02d", month);
      else
       IupSetInt(txt_month, "VALUE", month);

      iDatePickUpdateDayLimits(ih);

      if (iupAttribGetBoolean(ih, "ZEROPRECED"))
        IupSetStrf(txt_day, "VALUEMASKED", "%02d", day);
      else
        IupSetInt(txt_day, "VALUEMASKED", day);
    }
  }
  return 0; /* do not store value in hash table */
}

static char* iDatePickGetValueAttrib(Ihandle* ih)
{
  Ihandle* txt_year = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_YEAR");
  int year = IupGetInt(txt_year, "VALUE");

  Ihandle* txt_month = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_MONTH");
  int month = IupGetInt(txt_month, "VALUE");

  Ihandle* txt_day = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_DAY");
  int day = IupGetInt(txt_day, "VALUE");

  return iupStrReturnStrf("%d/%d/%d", year, month, day);
}

static int iDatePickSetSeparatorAttrib(Ihandle* ih, const char* value)
{
  Ihandle* lbl = IupGetChild(ih->firstchild, 1);
  IupSetStrAttribute(lbl, "TITLE", value);
  lbl = IupGetChild(ih->firstchild, 3);
  IupSetStrAttribute(lbl, "TITLE", value);
  return 1;
}

static void iDatePickSetDayTextBox(Ihandle* ih, int pos)
{
  Ihandle* txt = IupGetChild(ih->firstchild, pos);
  IupSetAttribute(txt, "MASKINT", "1:31");
  IupSetAttribute(txt, "MASKNOEMPTY", "Yes");
  IupSetAttribute(txt, "NC", "2");
  IupSetAttribute(txt, "VISIBLECOLUMNS", "2");

  iupAttribSet(ih, "_IUP_DATE_DAY", (char*)txt);
}

static void iDatePickSetMonthTextBox(Ihandle* ih, int pos)
{
  Ihandle* txt = IupGetChild(ih->firstchild, pos);

  IupSetAttribute(txt, "MASKINT", "1:12");
  IupSetAttribute(txt, "MASKNOEMPTY", "Yes");
  IupSetAttribute(txt, "NC", "2");
  IupSetAttribute(txt, "VISIBLECOLUMNS", "2");

  iupAttribSet(ih, "_IUP_DATE_MONTH", (char*)txt);
}

static void iDatePickSetYearTextBox(Ihandle* ih, int pos)
{
  Ihandle* txt = IupGetChild(ih->firstchild, pos);
  IupSetAttribute(txt, "MASK", IUP_MASK_UINT);
  IupSetAttribute(txt, "MASKNOEMPTY", "Yes");
  IupSetAttribute(txt, "NC", "4");
  IupSetAttribute(txt, "VISIBLECOLUMNS", "4");

  iupAttribSet(ih, "_IUP_DATE_YEAR", (char*)txt);
}

static int iDatePickSetOrderAttrib(Ihandle* ih, const char* value)
{
  int i;
  int year = 0, month = 0, day = 0;
  Ihandle* txt_year = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_YEAR");
  Ihandle* txt_month = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_MONTH");
  Ihandle* txt_day = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_DAY");

  if (!value || strlen(value) != 3)
    return 0;

  if (txt_year) year = IupGetInt(txt_year, "VALUE");
  if (txt_month) month = IupGetInt(txt_month, "VALUE");
  if (txt_day) day = IupGetInt(txt_day, "VALUE");

  for (i = 0; i < 3; i++)
  {
    if (value[i] == 'D' || value[i] == 'd')
      iDatePickSetDayTextBox(ih, i * 2);
    else if (value[i] == 'M' || value[i] == 'm')
      iDatePickSetMonthTextBox(ih, i * 2);
    else if (value[i] == 'Y' || value[i] == 'y')
      iDatePickSetYearTextBox(ih, i * 2);
    else
      return 0;
  }

  txt_year = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_YEAR");
  txt_month = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_MONTH");
  txt_day = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_DAY");

  if (year && txt_year) IupSetInt(txt_year, "VALUE", year);
  if (month && txt_month)
  {
    if (iupAttribGetBoolean(ih, "ZEROPRECED"))
      IupSetStrf(txt_month, "VALUE", "%02d", month);
    else
      IupSetInt(txt_month, "VALUE", month);
  }
  if (day && txt_day)
  {
    if (iupAttribGetBoolean(ih, "ZEROPRECED"))
      IupSetStrf(txt_day, "VALUE", "%02d", day);
    else
      IupSetInt(txt_day, "VALUE", day);
  }

  return 1;
}

static char* iDatePickGetTodayAttrib(Ihandle* ih)
{
  struct tm * timeinfo;
  time_t timer;
  time(&timer);
  timeinfo = localtime(&timer);
  (void)ih;
  return iupStrReturnStrf("%d/%d/%d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
}

static int iDatePickSetZeroprecedAttrib(Ihandle* ih, const char* value)
{
  Ihandle* txt_day = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_DAY");
  Ihandle* txt_month = (Ihandle*)iupAttribGet(ih, "_IUP_DATE_MONTH");

  if (txt_day && txt_month)
  {
    int day = IupGetInt(txt_day, "VALUE");
    int month = IupGetInt(txt_month, "VALUE");

    if (iupStrBoolean(value))
    {
      IupSetStrf(txt_day, "VALUE", "%02d", day);
      IupSetStrf(txt_month, "VALUE", "%02d", month);
    }
    else
    {
      IupSetInt(txt_day, "VALUE", day);
      IupSetInt(txt_month, "VALUE", month);
    }
  }

  return 1;
}


/*********************************************************************************************/


static Ihandle* iDatePickCreateText(void)
{
  Ihandle* txt = IupText(NULL);
  IupSetAttribute(txt, "BORDER", "NO");
  IupSetAttribute(txt, "NOHIDESEL", "NO");
  IupSetAttribute(txt, "ALIGNMENT", "ACENTER");
  IupSetCallback(txt, "K_ANY", (Icallback)iDatePickTextKAny_CB);
  IupSetCallback(txt, "VALUECHANGED_CB", iDatePickTextValueChanged_CB);
  return txt;
}

static int iDatePickCreateMethod(Ihandle* ih, void** params)
{
  Ihandle *box, *tgl;
  (void)params;
  
  tgl = IupToggle(NULL, NULL);
  IupSetAttribute(tgl, "IMAGE", "IupArrowDown");
  IupSetAttribute(tgl, "EXPAND", "VERTICALFREE");
  IupSetAttribute(tgl, "FLAT", "YES");
  IupSetAttribute(tgl, "IGNOREDOUBLECLICK", "YES");
  IupSetCallback(tgl, "ACTION", (Icallback)iDatePickToggleAction_CB);

  {
    Ihandle* lbl1 = IupLabel("/");
    Ihandle* lbl2 = IupLabel("/");
    int extra_w = 0, extra_h = 0;
    iupdrvTextAddExtraPadding(NULL, &extra_w, &extra_h);
    if (extra_h > 0 || extra_w > 0)
    {
      int horiz_padding = extra_w / 2;
      int vert_padding = extra_h / 2;
      IupSetfAttribute(lbl1, "PADDING", "%dx%d", horiz_padding, vert_padding);
      IupSetfAttribute(lbl2, "PADDING", "%dx%d", horiz_padding, vert_padding);
    }
    box = IupHbox(iDatePickCreateText(), lbl1, iDatePickCreateText(), lbl2, iDatePickCreateText(), tgl, NULL);
    IupSetAttribute(box, "MARGIN", "0x0");
    IupSetAttribute(box, "GAP", "0");
    IupSetAttribute(box, "ALIGNMENT", "ACENTER");
  }

  iupChildTreeAppend(ih, box);
  box->flags |= IUP_INTERNAL;

  iDatePickSetOrderAttrib(ih, "DMY");
  iDatePickSetValueAttrib(ih, iDatePickGetTodayAttrib(ih));
  
#if 1 /* GTK_CHECK_VERSION(3, 16, 0) */
  IupSetAttribute(ih, "BACKCOLOR", IupGetGlobal("TXTBGCOLOR")); /* workaround */
#else
  IupSetAttribute(ih, "BGCOLOR", IupGetGlobal("TXTBGCOLOR")); /* NOT Working in GTK > 3.16 */
#endif

  return IUP_NOERROR;
}

static void iDatePickUnMapMethod(Ihandle* ih)
{
  Ihandle* popover = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER");
  if (iupObjectCheck(popover))
  {
    IupDestroy(popover);
    iupAttribSet(ih, "_IUP_POPOVER", NULL);
    iupAttribSet(ih, "_IUP_CALENDAR", NULL);
  }
}

Iclass* iupDatePickNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("frame"));

  ic->name = "datepick";
  ic->cons = "DatePick";
  ic->format = NULL;  /* no parameters */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupDatePickNewClass;
  ic->Create = iDatePickCreateMethod;
  ic->UnMap = iDatePickUnMapMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  iupClassRegisterAttribute(ic, "VALUE", iDatePickGetValueAttrib, iDatePickSetValueAttrib, IUPAF_SAMEASSYSTEM, "TODAY", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", iDatePickGetTodayAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SEPARATOR", NULL, iDatePickSetSeparatorAttrib, IUPAF_SAMEASSYSTEM, "/", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORDER", NULL, iDatePickSetOrderAttrib, IUPAF_SAMEASSYSTEM, "DMY", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZEROPRECED", NULL, iDatePickSetZeroprecedAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CALENDARWEEKNUMBERS", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, iDatePickSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupDatePick(void)
{
  return IupCreate("datepick");
}
