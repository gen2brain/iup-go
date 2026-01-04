/** \file
 * \brief Calendar Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"

#include "iupefl_drv.h"


static void eflCalendarChangedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  struct tm selected_time;
  int old_day;

  (void)ev;

  selected_time = efl_ui_calendar_date_get(ev->object);

  old_day = iupAttribGetInt(ih, "_IUP_OLD_DAY");

  if ((int)selected_time.tm_mday != old_day)
  {
    iupAttribSetInt(ih, "_IUP_OLD_DAY", (int)selected_time.tm_mday);
    iupBaseCallValueChangedCb(ih);
  }
}

static int eflCalendarSetValueAttrib(Ihandle* ih, const char* value)
{
  Eo* calendar = iupeflGetWidget(ih);

  if (!calendar)
    return 0;

  if (iupStrEqualNoCase(value, "TODAY"))
  {
    struct tm timeinfo;
    time_t timer;
    time(&timer);
    localtime_r(&timer, &timeinfo);

    efl_ui_calendar_date_set(calendar, timeinfo);
  }
  else
  {
    int year, month, day;
    if (sscanf(value, "%d/%d/%d", &year, &month, &day) == 3)
    {
      struct tm selected_time;

      if (month < 1) month = 1;
      if (month > 12) month = 12;
      if (day < 1) day = 1;
      if (day > 31) day = 31;

      memset(&selected_time, 0, sizeof(struct tm));
      selected_time.tm_year = year - 1900;
      selected_time.tm_mon = month - 1;
      selected_time.tm_mday = day;

      efl_ui_calendar_date_set(calendar, selected_time);
    }
  }

  return 0;
}

static char* eflCalendarGetValueAttrib(Ihandle* ih)
{
  Eo* calendar = iupeflGetWidget(ih);
  struct tm selected_time;

  if (!calendar)
    return NULL;

  selected_time = efl_ui_calendar_date_get(calendar);

  return iupStrReturnStrf("%d/%d/%d",
                          selected_time.tm_year + 1900,
                          selected_time.tm_mon + 1,
                          selected_time.tm_mday);
}

static char* eflCalendarGetTodayAttrib(Ihandle* ih)
{
  struct tm timeinfo;
  time_t timer;
  (void)ih;
  time(&timer);
  localtime_r(&timer, &timeinfo);
  return iupStrReturnStrf("%d/%d/%d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
}

static void eflCalendarComputeNaturalSizeMethod(Ihandle* ih, int* w, int* h, int* children_expand)
{
  (void)children_expand;

  iupdrvFontGetMultiLineStringSize(ih, "W8W", w, h);

  *h += 4;

  (*w) *= 7;
  (*h) *= 8;

  *h += 4;
}

static int eflCalendarMapMethod(Ihandle* ih)
{
  Eo* parent;
  Eo* calendar;

  parent = iupeflGetParentWidget(ih);
  if (!parent)
    return IUP_ERROR;

  calendar = efl_add(EFL_UI_CALENDAR_CLASS, parent, efl_ui_format_string_set(efl_added, "%B %Y", EFL_UI_FORMAT_STRING_TYPE_TIME));
  if (!calendar)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)calendar;

  efl_event_callback_add(calendar, EFL_UI_CALENDAR_EVENT_CHANGED, eflCalendarChangedCallback, ih);

  iupeflBaseAddCallbacks(ih, calendar);

  iupeflAddToParent(ih);

  return IUP_NOERROR;
}

static void eflCalendarUnMapMethod(Ihandle* ih)
{
  Eo* calendar = iupeflGetWidget(ih);

  if (calendar)
  {
    efl_event_callback_del(calendar, EFL_UI_CALENDAR_EVENT_CHANGED, eflCalendarChangedCallback, ih);
    iupeflDelete(calendar);
  }

  ih->handle = NULL;
}

void iupdrvCalendarInitClass(Iclass* ic)
{
  ic->Map = eflCalendarMapMethod;
  ic->UnMap = eflCalendarUnMapMethod;
  ic->ComputeNaturalSize = eflCalendarComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  iupBaseRegisterCommonCallbacks(ic);

  iupBaseRegisterCommonAttrib(ic);

  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "VALUE", eflCalendarGetValueAttrib, eflCalendarSetValueAttrib, NULL, "TODAY", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", eflCalendarGetTodayAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
}

Iclass* iupCalendarNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "calendar";
  ic->cons = "Calendar";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  ic->New = iupCalendarNewClass;

  iupdrvCalendarInitClass(ic);

  return ic;
}

IUP_API Ihandle* IupCalendar(void)
{
  return IupCreate("calendar");
}
