/** \file
 * \brief WebAssembly IupCalendar
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"

#include "iupwasm_drv.h"


EM_JS(void, iupwasmJsCalStyle, (int id), {
  globalThis.__iupApply({ op: 'calstyle', id: id });
})

EM_JS(void, iupwasmJsCalHeader, (int id, const char* label, int weeknumbers), {
  globalThis.__iupApply({ op: 'calheader', id: id, label: UTF8ToString(label), weeknumbers: weeknumbers });
})

EM_JS(void, iupwasmJsCalCell, (int id, int idx, int day, int year, int month, int outside, int today, int selected), {
  globalThis.__iupApply({ op: 'calcell', id: id, idx: idx, day: day, year: year, month: month, outside: outside, today: today, selected: selected });
})

EM_JS(void, iupwasmJsCalWeek, (int id, int row, int week), {
  globalThis.__iupApply({ op: 'calweek', id: id, row: row, week: week });
})

static void wasmCalendarToday(int* y, int* m, int* d)
{
  time_t timer;
  struct tm* t;
  time(&timer);
  t = localtime(&timer);
  *y = t ? t->tm_year + 1900 : 2000;
  *m = t ? t->tm_mon + 1 : 1;
  *d = t ? t->tm_mday : 1;
}

static int wasmCalendarLeap(int y)
{
  return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static int wasmCalendarDaysInMonth(int y, int m)
{
  static const int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if (m == 2 && wasmCalendarLeap(y))
    return 29;
  return days[m - 1];
}

/* 0=Sunday .. 6=Saturday for the 1st of month m in year y */
static int wasmCalendarDow(int y, int m, int d)
{
  static const int t[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
  if (m < 3) y -= 1;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

/* ISO 8601 week number for a date */
static int wasmCalendarIsoWeek(int y, int m, int d)
{
  int doy, jan1dow, week;
  static const int cum[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
  doy = cum[m - 1] + d + ((m > 2 && wasmCalendarLeap(y)) ? 1 : 0);
  jan1dow = wasmCalendarDow(y, 1, 1);
  jan1dow = (jan1dow + 6) % 7; /* Monday=0 */
  week = (doy + jan1dow - 1) / 7;
  if (jan1dow <= 3) week += 1;
  if (week == 0)
    return wasmCalendarIsoWeek(y - 1, 12, 31);
  if (week == 53)
  {
    int dec31dow = (wasmCalendarDow(y, 12, 31) + 6) % 7;
    if (dec31dow < 3)
      return 1;
  }
  return week;
}

static void wasmCalendarMonthLabel(int y, int m, char* buf, int len)
{
  static const char* names[12] = { "January", "February", "March", "April", "May", "June",
                                   "July", "August", "September", "October", "November", "December" };
  snprintf(buf, len, "%s %d", names[m - 1], y);
}

static void wasmCalendarRender(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  int sel_y, sel_m, sel_d, show_y, show_m, today_y, today_m, today_d;
  int weeknumbers, first_dow, dim, prev_dim, i;
  char label[64];

  if (!id)
    return;

  sel_y = iupAttribGetInt(ih, "_IUP_SEL_Y");
  sel_m = iupAttribGetInt(ih, "_IUP_SEL_M");
  sel_d = iupAttribGetInt(ih, "_IUP_SEL_D");
  show_y = iupAttribGetInt(ih, "_IUP_SHOW_Y");
  show_m = iupAttribGetInt(ih, "_IUP_SHOW_M");
  weeknumbers = iupAttribGetBoolean(ih, "WEEKNUMBERS");
  wasmCalendarToday(&today_y, &today_m, &today_d);

  wasmCalendarMonthLabel(show_y, show_m, label, sizeof(label));
  iupwasmJsCalHeader(id, label, weeknumbers);

  first_dow = wasmCalendarDow(show_y, show_m, 1);
  dim = wasmCalendarDaysInMonth(show_y, show_m);
  prev_dim = wasmCalendarDaysInMonth(show_m == 1 ? show_y - 1 : show_y, show_m == 1 ? 12 : show_m - 1);

  for (i = 0; i < 42; i++)
  {
    int cell_y = show_y, cell_m = show_m, cell_d, outside = 0;
    int day_index = i - first_dow; /* 0-based day within shown month */

    if (day_index < 0)
    {
      outside = 1;
      cell_d = prev_dim + day_index + 1;
      cell_m = show_m == 1 ? 12 : show_m - 1;
      cell_y = show_m == 1 ? show_y - 1 : show_y;
    }
    else if (day_index >= dim)
    {
      outside = 1;
      cell_d = day_index - dim + 1;
      cell_m = show_m == 12 ? 1 : show_m + 1;
      cell_y = show_m == 12 ? show_y + 1 : show_y;
    }
    else
      cell_d = day_index + 1;

    {
      int is_today = (cell_y == today_y && cell_m == today_m && cell_d == today_d);
      int is_sel = (cell_y == sel_y && cell_m == sel_m && cell_d == sel_d);
      iupwasmJsCalCell(id, i, cell_d, cell_y, cell_m, outside, is_today, is_sel);
    }

    if (weeknumbers && (i % 7) == 0)
    {
      int wy = cell_y, wm = cell_m, wd = cell_d;
      iupwasmJsCalWeek(id, i / 7, wasmCalendarIsoWeek(wy, wm, wd));
    }
  }
}

static int wasmCalendarSetValueAttrib(Ihandle* ih, const char* value)
{
  int y, m, d;

  if (!value || iupStrEqualNoCase(value, "TODAY"))
    wasmCalendarToday(&y, &m, &d);
  else if (sscanf(value, "%d%*[/-]%d%*[/-]%d", &y, &m, &d) != 3)
    return 0;

  if (m < 1) m = 1;
  if (m > 12) m = 12;
  if (d < 1) d = 1;
  if (d > 31) d = 31;

  iupAttribSetInt(ih, "_IUP_SEL_Y", y);
  iupAttribSetInt(ih, "_IUP_SEL_M", m);
  iupAttribSetInt(ih, "_IUP_SEL_D", d);
  iupAttribSetInt(ih, "_IUP_SHOW_Y", y);
  iupAttribSetInt(ih, "_IUP_SHOW_M", m);
  iupAttribSetInt(ih, "_IUP_OLD_DAY", d);

  if (ih->handle)
    wasmCalendarRender(ih);
  return 0;
}

static char* wasmCalendarGetValueAttrib(Ihandle* ih)
{
  int y = iupAttribGetInt(ih, "_IUP_SEL_Y");
  int m = iupAttribGetInt(ih, "_IUP_SEL_M");
  int d = iupAttribGetInt(ih, "_IUP_SEL_D");
  if (!y)
    wasmCalendarToday(&y, &m, &d);
  return iupStrReturnStrf("%d/%d/%d", y, m, d);
}

static char* wasmCalendarGetTodayAttrib(Ihandle* ih)
{
  int y, m, d;
  (void)ih;
  wasmCalendarToday(&y, &m, &d);
  return iupStrReturnStrf("%d/%d/%d", y, m, d);
}

static int wasmCalendarSetWeekNumbersAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "WEEKNUMBERS", iupStrBoolean(value) ? "YES" : "NO");
  if (ih->handle)
    wasmCalendarRender(ih);
  return 1;
}

EMSCRIPTEN_KEEPALIVE void iupwasmCalendarSelect(int id, int year, int month, int day)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  Icallback cb;
  int old_day;

  if (!ih)
    return;

  iupAttribSetInt(ih, "_IUP_SEL_Y", year);
  iupAttribSetInt(ih, "_IUP_SEL_M", month);
  iupAttribSetInt(ih, "_IUP_SEL_D", day);
  iupAttribSetInt(ih, "_IUP_SHOW_Y", year);
  iupAttribSetInt(ih, "_IUP_SHOW_M", month);

  wasmCalendarRender(ih);

  old_day = iupAttribGetInt(ih, "_IUP_OLD_DAY");
  if (day != old_day)
  {
    iupAttribSetInt(ih, "_IUP_OLD_DAY", day);
    cb = IupGetCallback(ih, "VALUECHANGED_CB");
    if (cb)
      cb(ih);
  }
}

EMSCRIPTEN_KEEPALIVE void iupwasmCalendarNav(int id, int delta)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  int y, m;

  if (!ih)
    return;

  y = iupAttribGetInt(ih, "_IUP_SHOW_Y");
  m = iupAttribGetInt(ih, "_IUP_SHOW_M");
  m += delta;
  while (m > 12) { m -= 12; y += 1; }
  while (m < 1) { m += 12; y -= 1; }
  iupAttribSetInt(ih, "_IUP_SHOW_Y", y);
  iupAttribSetInt(ih, "_IUP_SHOW_M", m);

  wasmCalendarRender(ih);
}

static int wasmCalendarMapMethod(Ihandle* ih)
{
  int id = iupwasmJsCreate("div");
  if (!id)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);
  iupwasmAddToParent(ih);
  iupwasmJsCalStyle(id);

  if (!iupAttribGetInt(ih, "_IUP_SEL_Y"))
    wasmCalendarSetValueAttrib(ih, "TODAY");
  else
    wasmCalendarRender(ih);
  return IUP_NOERROR;
}

static void wasmCalendarComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int cw, ch;
  (void)children_expand;

  iupdrvFontGetMultiLineStringSize(ih, "W8", &cw, &ch);
  if (cw < 14) cw = 14;

  /* header + weekday row + 6 day rows, with an extra week-number column when on */
  *w = (cw + 8) * (iupAttribGetBoolean(ih, "WEEKNUMBERS") ? 8 : 7) + 4;
  *h = (ch + 6) * 8 + 4;
}

Iclass* iupCalendarNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "calendar";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  ic->New = iupCalendarNewClass;

  ic->Map = wasmCalendarMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->ComputeNaturalSize = wasmCalendarComputeNaturalSizeMethod;

  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  iupBaseRegisterCommonCallbacks(ic);
  iupBaseRegisterCommonAttrib(ic);
  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "VALUE", wasmCalendarGetValueAttrib, wasmCalendarSetValueAttrib, NULL, "TODAY", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", wasmCalendarGetTodayAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WEEKNUMBERS", NULL, wasmCalendarSetWeekNumbersAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupCalendar(void)
{
  return IupCreate("calendar");
}
