/** \file
 * \brief Calendar Control for GTK4
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

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

#include "iupgtk4_drv.h"


static int gtk4CalendarSetWeekNumbersAttrib(Ihandle* ih, const char* value)
{
  gtk_calendar_set_show_week_numbers(GTK_CALENDAR(ih->handle), iupStrBoolean(value));
  return 1;
}

static void gtk4CalendarSetDate(GtkCalendar* calendar, GDateTime* dt)
{
#if GTK_CHECK_VERSION(4, 20, 0)
  gtk_calendar_set_date(calendar, dt);
#else
  gtk_calendar_select_day(calendar, dt);
#endif
}

static int gtk4CalendarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "TODAY"))
  {
    struct tm * timeinfo;
    time_t timer;
    time(&timer);
    timeinfo = localtime(&timer);

    GDateTime *dt = g_date_time_new_local(timeinfo->tm_year + 1900,
                                           timeinfo->tm_mon + 1,
                                           timeinfo->tm_mday, 0, 0, 0);
    gtk4CalendarSetDate(GTK_CALENDAR(ih->handle), dt);
    g_date_time_unref(dt);
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

      GDateTime *dt = g_date_time_new_local(year, month, day, 0, 0, 0);
      gtk4CalendarSetDate(GTK_CALENDAR(ih->handle), dt);
      g_date_time_unref(dt);
    }
  }
  return 0; /* do not store value in hash table */
}

static char* gtk4CalendarGetValueAttrib(Ihandle* ih)
{
  GDateTime *dt = gtk_calendar_get_date(GTK_CALENDAR(ih->handle));
  int year = g_date_time_get_year(dt);
  int month = g_date_time_get_month(dt);
  int day = g_date_time_get_day_of_month(dt);

  return iupStrReturnStrf("%d/%d/%d", year, month, day);
}

static char* gtk4CalendarGetTodayAttrib(Ihandle* ih)
{
  struct tm * timeinfo;
  time_t timer;
  time(&timer);
  timeinfo = localtime(&timer);
  (void)ih;
  return iupStrReturnStrf("%d/%d/%d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
}

static void gtk4CalendarComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  (void)children_expand; /* unset if not a container */

  iupdrvFontGetMultiLineStringSize(ih, "W8W", w, h);

  *h += 4; /* for each line */

  (*w) *= 7; /* 7 columns */
  (*h) *= 8; /* 8 lines */

  *h += 4; /* for the last or first line */

  iupdrvTextAddBorders(ih, w, h);
}

static void gtk4CalendarDaySelected(GtkCalendar *calendar, Ihandle* ih)
{
  iupBaseCallValueChangedCb(ih);
  (void)calendar;
}

static int gtk4CalendarMapMethod(Ihandle* ih)
{
  ih->handle = gtk_calendar_new();
  if (!ih->handle)
    return IUP_ERROR;

  iupgtk4AddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupgtk4SetCanFocus(ih->handle, 0);

  iupgtk4SetupEnterLeaveEvents(ih->handle, ih);
  iupgtk4SetupFocusEvents(ih->handle, ih);
  iupgtk4SetupKeyEvents(ih->handle, ih);

  g_signal_connect(G_OBJECT(ih->handle), "day-selected", G_CALLBACK(gtk4CalendarDaySelected), ih);

  gtk_widget_realize(ih->handle);

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

  ic->New = iupCalendarNewClass;
  ic->Map = gtk4CalendarMapMethod;
  ic->ComputeNaturalSize = gtk4CalendarComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  iupBaseRegisterCommonCallbacks(ic);
  iupBaseRegisterCommonAttrib(ic);
  iupBaseRegisterVisualAttrib(ic);

  /* IupCalendar only */
  iupClassRegisterAttribute(ic, "VALUE", gtk4CalendarGetValueAttrib, gtk4CalendarSetValueAttrib, NULL, "TODAY", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WEEKNUMBERS", NULL, gtk4CalendarSetWeekNumbersAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", gtk4CalendarGetTodayAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupCalendar(void)
{
  return IupCreate("calendar");
}
