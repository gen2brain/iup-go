/** \file
 * \brief Calendar Control - Motif Implementation
 *
 * Custom calendar built with IUP controls.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_register.h"
#include "iup_childtree.h"
#include "iup_classbase.h"


typedef struct {
  int cur_year, cur_month;
  int sel_year, sel_month, sel_day;
  int show_week_numbers;
  int grid_day[6][7];
  int grid_month[6][7];
  int grid_year[6][7];
  Ihandle* lbl_title;
  Ihandle* wk_labels[6];
  Ihandle* wk_spacer;
  Ihandle* day_btns[6][7];
} IcalData;

static int motCalendarDaysInMonth(int year, int month)
{
  static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  int d;
  if (month < 1 || month > 12) return 30;
  d = days[month - 1];
  if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0))
    d = 29;
  return d;
}

static int motCalendarDayOfWeek(int year, int month, int day)
{
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  mktime(&t);
  return t.tm_wday;
}

static int motCalendarISOWeek(int year, int month, int day)
{
  struct tm t;
  int yday, wday, week;
  memset(&t, 0, sizeof(t));
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  mktime(&t);

  yday = t.tm_yday;
  wday = t.tm_wday;
  if (wday == 0) wday = 7;

  week = (yday - wday + 10) / 7;
  if (week < 1) week = 52;
  else if (week > 52)
  {
    int jan1_wday = motCalendarDayOfWeek(year + 1, 1, 1);
    if (jan1_wday == 0) jan1_wday = 7;
    if (jan1_wday <= 4) week = 1;
  }
  return week;
}

static void motCalendarRebuildGrid(Ihandle* ih)
{
  IcalData* data = (IcalData*)iupAttribGet(ih, "_IUP_CALDATA");
  int days_in_month, first_wday, prev_month, prev_year, days_in_prev;
  int next_month, next_year, day, next_day, r, c;
  static const char* month_names[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
  };
  time_t now;
  struct tm* tnow;
  int today_y, today_m, today_d;

  if (!data) return;

  days_in_month = motCalendarDaysInMonth(data->cur_year, data->cur_month);
  first_wday = motCalendarDayOfWeek(data->cur_year, data->cur_month, 1);

  prev_month = data->cur_month - 1;
  prev_year = data->cur_year;
  if (prev_month < 1) { prev_month = 12; prev_year--; }
  days_in_prev = motCalendarDaysInMonth(prev_year, prev_month);

  next_month = data->cur_month + 1;
  next_year = data->cur_year;
  if (next_month > 12) { next_month = 1; next_year++; }

  day = 1;
  next_day = 1;

  for (r = 0; r < 6; r++)
  {
    for (c = 0; c < 7; c++)
    {
      int cell = r * 7 + c;
      if (cell < first_wday)
      {
        int d = days_in_prev - first_wday + cell + 1;
        data->grid_day[r][c] = d;
        data->grid_month[r][c] = prev_month;
        data->grid_year[r][c] = prev_year;
      }
      else if (day <= days_in_month)
      {
        data->grid_day[r][c] = day;
        data->grid_month[r][c] = data->cur_month;
        data->grid_year[r][c] = data->cur_year;
        day++;
      }
      else
      {
        data->grid_day[r][c] = next_day;
        data->grid_month[r][c] = next_month;
        data->grid_year[r][c] = next_year;
        next_day++;
      }
    }
  }

  if (data->lbl_title)
    IupSetStrf(data->lbl_title, "TITLE", "%s %d", month_names[data->cur_month - 1], data->cur_year);

  now = time(NULL);
  tnow = localtime(&now);
  today_y = tnow->tm_year + 1900;
  today_m = tnow->tm_mon + 1;
  today_d = tnow->tm_mday;

  for (r = 0; r < 6; r++)
  {
    if (data->show_week_numbers)
    {
      int wk = motCalendarISOWeek(data->grid_year[r][0], data->grid_month[r][0], data->grid_day[r][0]);
      if (data->wk_labels[r])
      {
        IupSetfAttribute(data->wk_labels[r], "TITLE", "%d", wk);
        IupSetAttribute(data->wk_labels[r], "FLOATING", "NO");
        IupSetAttribute(data->wk_labels[r], "VISIBLE", "YES");
      }
      if (r == 0 && data->wk_spacer)
      {
        IupSetAttribute(data->wk_spacer, "FLOATING", "NO");
        IupSetAttribute(data->wk_spacer, "VISIBLE", "YES");
      }
    }
    else
    {
      if (data->wk_labels[r])
      {
        IupSetAttribute(data->wk_labels[r], "FLOATING", "YES");
        IupSetAttribute(data->wk_labels[r], "VISIBLE", "NO");
      }
      if (r == 0 && data->wk_spacer)
      {
        IupSetAttribute(data->wk_spacer, "FLOATING", "YES");
        IupSetAttribute(data->wk_spacer, "VISIBLE", "NO");
      }
    }

    for (c = 0; c < 7; c++)
    {
      Ihandle* btn;
      int is_cur_month, is_selected, is_today;

      btn = data->day_btns[r][c];
      if (!btn) continue;

      IupSetfAttribute(btn, "TITLE", "%d", data->grid_day[r][c]);

      is_cur_month = (data->grid_month[r][c] == data->cur_month);
      is_selected = (data->grid_year[r][c] == data->sel_year && data->grid_month[r][c] == data->sel_month && data->grid_day[r][c] == data->sel_day);
      is_today = (data->grid_year[r][c] == today_y && data->grid_month[r][c] == today_m && data->grid_day[r][c] == today_d);

      if (is_selected)
      {
        IupSetAttribute(btn, "FONTSTYLE", "Bold");
        IupSetAttribute(btn, "BGCOLOR", "60 120 220");
        IupSetAttribute(btn, "FGCOLOR", "255 255 255");
      }
      else if (is_today)
      {
        IupSetAttribute(btn, "FONTSTYLE", "Bold");
        IupSetAttribute(btn, "BGCOLOR", NULL);
        IupSetAttribute(btn, "FGCOLOR", "60 120 220");
      }
      else if (!is_cur_month)
      {
        IupSetAttribute(btn, "FONTSTYLE", "Normal");
        IupSetAttribute(btn, "BGCOLOR", NULL);
        IupSetAttribute(btn, "FGCOLOR", "160 160 160");
      }
      else
      {
        IupSetAttribute(btn, "FONTSTYLE", "Normal");
        IupSetAttribute(btn, "BGCOLOR", NULL);
        IupSetAttribute(btn, "FGCOLOR", NULL);
      }
    }
  }
}

static int motCalendarDayButton_CB(Ihandle* btn)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(btn, "_IUP_CALENDAR");
  IcalData* data;
  int r, c;

  if (!ih) return IUP_DEFAULT;
  data = (IcalData*)iupAttribGet(ih, "_IUP_CALDATA");
  if (!data) return IUP_DEFAULT;

  r = iupAttribGetInt(btn, "_IUP_CALROW");
  c = iupAttribGetInt(btn, "_IUP_CALCOL");

  data->sel_year = data->grid_year[r][c];
  data->sel_month = data->grid_month[r][c];
  data->sel_day = data->grid_day[r][c];

  if (data->grid_month[r][c] != data->cur_month)
  {
    data->cur_month = data->grid_month[r][c];
    data->cur_year = data->grid_year[r][c];
  }

  motCalendarRebuildGrid(ih);
  iupBaseCallValueChangedCb(ih);

  return IUP_DEFAULT;
}

static int motCalendarPrevMonth_CB(Ihandle* btn)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(btn, "_IUP_CALENDAR");
  IcalData* data;

  if (!ih) return IUP_DEFAULT;
  data = (IcalData*)iupAttribGet(ih, "_IUP_CALDATA");
  if (!data) return IUP_DEFAULT;

  data->cur_month--;
  if (data->cur_month < 1) { data->cur_month = 12; data->cur_year--; }
  motCalendarRebuildGrid(ih);

  return IUP_DEFAULT;
}

static int motCalendarNextMonth_CB(Ihandle* btn)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(btn, "_IUP_CALENDAR");
  IcalData* data;

  if (!ih) return IUP_DEFAULT;
  data = (IcalData*)iupAttribGet(ih, "_IUP_CALDATA");
  if (!data) return IUP_DEFAULT;

  data->cur_month++;
  if (data->cur_month > 12) { data->cur_month = 1; data->cur_year++; }
  motCalendarRebuildGrid(ih);

  return IUP_DEFAULT;
}

static int motCalendarSetValueAttrib(Ihandle* ih, const char* value)
{
  IcalData* data = (IcalData*)iupAttribGet(ih, "_IUP_CALDATA");
  if (!data) return 0;

  if (iupStrEqualNoCase(value, "TODAY"))
  {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    data->sel_year = t->tm_year + 1900;
    data->sel_month = t->tm_mon + 1;
    data->sel_day = t->tm_mday;
    data->cur_year = data->sel_year;
    data->cur_month = data->sel_month;
  }
  else
  {
    int year, month, day;
    if (sscanf(value, "%d/%d/%d", &year, &month, &day) == 3)
    {
      if (month < 1) month = 1;
      if (month > 12) month = 12;
      if (day < 1) day = 1;
      if (day > 31) day = 31;
      data->sel_year = year;
      data->sel_month = month;
      data->sel_day = day;
      data->cur_year = year;
      data->cur_month = month;
    }
  }

  if (ih->handle)
    motCalendarRebuildGrid(ih);

  return 0;
}

static char* motCalendarGetValueAttrib(Ihandle* ih)
{
  IcalData* data = (IcalData*)iupAttribGet(ih, "_IUP_CALDATA");
  if (!data) return NULL;
  return iupStrReturnStrf("%d/%d/%d", data->sel_year, data->sel_month, data->sel_day);
}

static char* motCalendarGetTodayAttrib(Ihandle* ih)
{
  time_t now;
  struct tm* t;
  (void)ih;
  now = time(NULL);
  t = localtime(&now);
  return iupStrReturnStrf("%d/%d/%d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

static int motCalendarSetWeekNumbersAttrib(Ihandle* ih, const char* value)
{
  IcalData* data = (IcalData*)iupAttribGet(ih, "_IUP_CALDATA");
  if (!data) return 0;

  data->show_week_numbers = iupStrBoolean(value);
  if (ih->handle)
    motCalendarRebuildGrid(ih);

  return 0;
}

static int motCalendarCreateMethod(Ihandle* ih, void** params)
{
  Ihandle* btn_prev;
  Ihandle* btn_next;
  Ihandle* lbl_title;
  Ihandle* header_row;
  Ihandle* vbox;
  IcalData* data;
  int r, c;
  time_t now;
  struct tm* t;
  static const char* day_names[] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };

  (void)params;

  data = (IcalData*)calloc(1, sizeof(IcalData));
  now = time(NULL);
  t = localtime(&now);
  data->cur_year = t->tm_year + 1900;
  data->cur_month = t->tm_mon + 1;
  data->sel_year = data->cur_year;
  data->sel_month = data->cur_month;
  data->sel_day = t->tm_mday;

  iupAttribSet(ih, "_IUP_CALDATA", (char*)data);

  btn_prev = IupButton("<", NULL);
  IupSetAttribute(btn_prev, "SIZE", "15x");
  IupSetCallback(btn_prev, "ACTION", (Icallback)motCalendarPrevMonth_CB);
  iupAttribSet(btn_prev, "_IUP_CALENDAR", (char*)ih);

  lbl_title = IupLabel(NULL);
  IupSetAttribute(lbl_title, "EXPAND", "HORIZONTAL");
  IupSetAttribute(lbl_title, "ALIGNMENT", "ACENTER");
  IupSetAttribute(lbl_title, "FONTSTYLE", "Bold");
  data->lbl_title = lbl_title;

  btn_next = IupButton(">", NULL);
  IupSetAttribute(btn_next, "SIZE", "15x");
  IupSetCallback(btn_next, "ACTION", (Icallback)motCalendarNextMonth_CB);
  iupAttribSet(btn_next, "_IUP_CALENDAR", (char*)ih);

  header_row = IupHbox(btn_prev, lbl_title, btn_next, NULL);
  IupSetAttribute(header_row, "ALIGNMENT", "ACENTER");
  IupSetAttribute(header_row, "MARGIN", "0x0");
  IupSetAttribute(header_row, "GAP", "2");

  vbox = IupVbox(header_row, NULL);
  IupSetAttribute(vbox, "EXPAND", "YES");
  IupSetAttribute(vbox, "MARGIN", "4x4");
  IupSetAttribute(vbox, "GAP", "0");

  /* Day name headers */
  {
    Ihandle* row = IupHbox(NULL);
    Ihandle* spacer = IupLabel("");
    IupSetAttribute(row, "MARGIN", "0x0");
    IupSetAttribute(row, "GAP", "0");

    IupSetAttribute(spacer, "SIZE", "15x");
    IupSetAttribute(spacer, "VISIBLE", "NO");
    data->wk_spacer = spacer;
    IupAppend(row, spacer);

    for (c = 0; c < 7; c++)
    {
      Ihandle* lbl = IupLabel(day_names[c]);
      IupSetAttribute(lbl, "ALIGNMENT", "ACENTER");
      IupSetAttribute(lbl, "FONTSTYLE", "Bold");
      IupSetAttribute(lbl, "EXPAND", "HORIZONTAL");
      IupAppend(row, lbl);
    }
    IupAppend(vbox, row);
  }

  /* Day grid, 6 rows */
  for (r = 0; r < 6; r++)
  {
    Ihandle* row = IupHbox(NULL);
    Ihandle* wk_lbl;

    IupSetAttribute(row, "EXPAND", "YES");
    IupSetAttribute(row, "MARGIN", "0x0");
    IupSetAttribute(row, "GAP", "0");

    wk_lbl = IupLabel("");
    IupSetAttribute(wk_lbl, "ALIGNMENT", "ACENTER");
    IupSetAttribute(wk_lbl, "FGCOLOR", "160 160 160");
    IupSetAttribute(wk_lbl, "SIZE", "15x");
    IupSetAttribute(wk_lbl, "VISIBLE", "NO");
    data->wk_labels[r] = wk_lbl;
    IupAppend(row, wk_lbl);

    for (c = 0; c < 7; c++)
    {
      Ihandle* btn = IupButton("  ", NULL);
      IupSetAttribute(btn, "EXPAND", "YES");
      IupSetAttribute(btn, "FLAT", "YES");
      IupSetAttribute(btn, "CANFOCUS", "NO");
      IupSetCallback(btn, "ACTION", (Icallback)motCalendarDayButton_CB);
      iupAttribSet(btn, "_IUP_CALENDAR", (char*)ih);
      iupAttribSetInt(btn, "_IUP_CALROW", r);
      iupAttribSetInt(btn, "_IUP_CALCOL", c);
      data->day_btns[r][c] = btn;
      IupAppend(row, btn);
    }
    IupAppend(vbox, row);
  }

  iupChildTreeAppend(ih, vbox);

  if (iupAttribGetBoolean(ih, "WEEKNUMBERS"))
    data->show_week_numbers = 1;

  motCalendarRebuildGrid(ih);

  return IUP_NOERROR;
}

static void motCalendarDestroyMethod(Ihandle* ih)
{
  IcalData* data = (IcalData*)iupAttribGet(ih, "_IUP_CALDATA");
  if (data)
  {
    free(data);
    iupAttribSet(ih, "_IUP_CALDATA", NULL);
  }
}

Iclass* iupCalendarNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("frame"));

  ic->name = "calendar";
  ic->cons = "Calendar";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDMANY+1;
  ic->is_interactive = 1;

  ic->New = iupCalendarNewClass;
  ic->Create = motCalendarCreateMethod;
  ic->Destroy = motCalendarDestroyMethod;

  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  iupClassRegisterAttribute(ic, "VALUE", motCalendarGetValueAttrib, motCalendarSetValueAttrib, NULL, "TODAY", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WEEKNUMBERS", NULL, motCalendarSetWeekNumbersAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", motCalendarGetTodayAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupCalendar(void)
{
  return IupCreate("calendar");
}
