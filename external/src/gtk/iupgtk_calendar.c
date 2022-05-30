/** \file
 * \brief Calendar Control
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

#include "iupgtk_drv.h"



static int gtkCalendarSetWeekNumbersAttrib(Ihandle* ih, const char* value)
{
  GtkCalendarDisplayOptions flags = gtk_calendar_get_display_options(GTK_CALENDAR(ih->handle));
  if (iupStrBoolean(value))
    flags |= GTK_CALENDAR_SHOW_WEEK_NUMBERS;
  else
    flags &= ~GTK_CALENDAR_SHOW_WEEK_NUMBERS;
  gtk_calendar_set_display_options(GTK_CALENDAR(ih->handle), flags);
  return 1;
}

static int gtkCalendarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "TODAY"))
  {
    struct tm * timeinfo;
    time_t timer;
    time(&timer);
    timeinfo = localtime(&timer);

    gtk_calendar_select_month(GTK_CALENDAR(ih->handle), timeinfo->tm_mon, timeinfo->tm_year + 1900);
    gtk_calendar_select_day(GTK_CALENDAR(ih->handle), timeinfo->tm_mday);

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

      /*  month is zero-based (i.e it allowed values are 0-11) while selected_day is one-based (i.e. allowed values are 1-31). */
      gtk_calendar_select_month(GTK_CALENDAR(ih->handle), month - 1, year);
      gtk_calendar_select_day(GTK_CALENDAR(ih->handle), day);
    }
  }
  return 0; /* do not store value in hash table */
}

static char* gtkCalendarGetValueAttrib(Ihandle* ih)
{
  guint year, month, day;
  gtk_calendar_get_date(GTK_CALENDAR(ih->handle), &year, &month, &day);
  return iupStrReturnStrf("%d/%d/%d", year, month + 1, day);
}

static char* gtkCalendarGetTodayAttrib(Ihandle* ih)
{
  struct tm * timeinfo;
  time_t timer;
  time(&timer);  
  timeinfo = localtime(&timer);
  (void)ih;
  return iupStrReturnStrf("%d/%d/%d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
}

static void gtkCalendarComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  (void)children_expand; /* unset if not a container */

  iupdrvFontGetMultiLineStringSize(ih, "W8W", w, h);

  *h += 4; /* for each line */

  (*w) *= 7; /* 7 columns */
  (*h) *= 8; /* 8 lines */

  *h += 4; /* for the last or first line */

  iupdrvTextAddBorders(ih, w, h);
}

static void gtkCalendarDaySelected(GtkCalendar *calendar, Ihandle* ih)
{
  iupBaseCallValueChangedCb(ih);
  (void)calendar;
}

static int gtkCalendarMapMethod(Ihandle* ih)
{
  ih->handle = gtk_calendar_new();
  if (!ih->handle)
    return IUP_ERROR;

  /* add to the parent, all GTK controls must call this. */
  iupgtkAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupgtkSetCanFocus(ih->handle, 0);

  g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-in-event", G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-out-event", G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "show-help", G_CALLBACK(iupgtkShowHelp), ih);
  g_signal_connect(G_OBJECT(ih->handle), "key-press-event", G_CALLBACK(iupgtkKeyPressEvent), ih);

  g_signal_connect(G_OBJECT(ih->handle), "day-selected", G_CALLBACK(gtkCalendarDaySelected), ih);

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

  /* Class functions */
  ic->New = iupCalendarNewClass;

  ic->Map = gtkCalendarMapMethod;
  ic->ComputeNaturalSize = gtkCalendarComputeNaturalSizeMethod;

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
  iupClassRegisterAttribute(ic, "VALUE", gtkCalendarGetValueAttrib, gtkCalendarSetValueAttrib, NULL, "TODAY", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WEEKNUMBERS", NULL, gtkCalendarSetWeekNumbersAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", gtkCalendarGetTodayAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupCalendar(void)
{
  return IupCreate("calendar");
}
