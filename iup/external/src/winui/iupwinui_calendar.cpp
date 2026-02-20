/** \file
 * \brief WinUI Driver - Calendar Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>

#include <cstdlib>
#include <cstring>
#include <cstdio>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_class.h"
#include "iup_register.h"
}

#include "iupwinui_drv.h"

#include <winrt/Windows.Globalization.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;


#define IUPWINUI_CALENDAR_AUX "_IUPWINUI_CALENDAR_AUX"

struct IupWinUICalendarAux
{
  event_token selectedDatesChangedToken;

  IupWinUICalendarAux() : selectedDatesChangedToken{} {}
};

static DateTime winuiCalendarMakeDateTime(int year, int month, int day)
{
  SYSTEMTIME st = {0};
  FILETIME ft;

  st.wYear = (WORD)year;
  st.wMonth = (WORD)month;
  st.wDay = (WORD)day;
  st.wHour = 12;

  SystemTimeToFileTime(&st, &ft);
  ULARGE_INTEGER uli;
  uli.LowPart = ft.dwLowDateTime;
  uli.HighPart = ft.dwHighDateTime;

  return DateTime{TimeSpan{(int64_t)uli.QuadPart}};
}

static void winuiCalendarGetDate(DateTime dt, int* year, int* month, int* day)
{
  auto ticks = dt.time_since_epoch().count();

  ULARGE_INTEGER uli;
  uli.QuadPart = (ULONGLONG)ticks;

  FILETIME ft;
  ft.dwLowDateTime = uli.LowPart;
  ft.dwHighDateTime = uli.HighPart;

  SYSTEMTIME st;
  FileTimeToSystemTime(&ft, &st);

  *year = st.wYear;
  *month = st.wMonth;
  *day = st.wDay;
}

static void winuiCalendarCallValueChanged(Ihandle* ih)
{
  IFn cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

static int winuiCalendarSetValueAttrib(Ihandle* ih, const char* value)
{
  CalendarView cv = winuiGetHandle<CalendarView>(ih);
  if (!cv)
    return 0;

  if (value && iupStrEqualNoCase(value, "TODAY"))
  {
    auto now = winrt::clock::now();
    cv.SelectedDates().Clear();
    cv.SelectedDates().Append(now);
    cv.SetDisplayDate(now);
  }
  else if (value)
  {
    int year, month, day;
    if (sscanf(value, "%d/%d/%d", &year, &month, &day) == 3)
    {
      auto dt = winuiCalendarMakeDateTime(year, month, day);
      cv.SelectedDates().Clear();
      cv.SelectedDates().Append(dt);
      cv.SetDisplayDate(dt);
    }
  }

  return 0;
}

static char* winuiCalendarGetValueAttrib(Ihandle* ih)
{
  CalendarView cv = winuiGetHandle<CalendarView>(ih);
  if (cv && cv.SelectedDates().Size() > 0)
  {
    auto dt = cv.SelectedDates().GetAt(0);
    int year, month, day;
    winuiCalendarGetDate(dt, &year, &month, &day);
    return iupStrReturnStrf("%d/%02d/%02d", year, month, day);
  }
  return NULL;
}

static char* winuiCalendarGetTodayAttrib(Ihandle* ih)
{
  (void)ih;
  SYSTEMTIME st;
  GetLocalTime(&st);
  return iupStrReturnStrf("%d/%02d/%02d", st.wYear, st.wMonth, st.wDay);
}

static int winuiCalendarMapMethod(Ihandle* ih)
{
  IupWinUICalendarAux* aux = new IupWinUICalendarAux();

  CalendarView cv = CalendarView();
  cv.HorizontalAlignment(HorizontalAlignment::Left);
  cv.VerticalAlignment(VerticalAlignment::Top);
  cv.SelectionMode(CalendarViewSelectionMode::Single);

  auto now = winrt::clock::now();
  cv.SelectedDates().Append(now);

  aux->selectedDatesChangedToken = cv.SelectedDatesChanged([ih](CalendarView const&, CalendarViewSelectedDatesChangedEventArgs const&) {
    winuiCalendarCallValueChanged(ih);
  });

  Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
  if (parentCanvas)
    parentCanvas.Children().Append(cv);

  winuiSetAux(ih, IUPWINUI_CALENDAR_AUX, aux);
  winuiStoreHandle(ih, cv);
  return IUP_NOERROR;
}

static void winuiCalendarUnMapMethod(Ihandle* ih)
{
  IupWinUICalendarAux* aux = winuiGetAux<IupWinUICalendarAux>(ih, IUPWINUI_CALENDAR_AUX);

  if (ih->handle && aux)
  {
    CalendarView cv = winuiGetHandle<CalendarView>(ih);
    if (cv && aux->selectedDatesChangedToken)
      cv.SelectedDatesChanged(aux->selectedDatesChangedToken);
    winuiReleaseHandle<CalendarView>(ih);
  }

  winuiFreeAux<IupWinUICalendarAux>(ih, IUPWINUI_CALENDAR_AUX);
  ih->handle = NULL;
}

static void winuiCalendarComputeNaturalSizeMethod(Ihandle* ih, int* w, int* h, int* children_expand)
{
  (void)ih;
  (void)children_expand;
  *w = 250;
  *h = 300;
}

extern "C" Iclass* iupCalendarNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = (char*)"calendar";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  ic->New = iupCalendarNewClass;
  ic->Map = winuiCalendarMapMethod;
  ic->UnMap = winuiCalendarUnMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->ComputeNaturalSize = winuiCalendarComputeNaturalSizeMethod;

  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  iupBaseRegisterCommonCallbacks(ic);
  iupBaseRegisterCommonAttrib(ic);
  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "VALUE", winuiCalendarGetValueAttrib, winuiCalendarSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", winuiCalendarGetTodayAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WEEKNUMBERS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  return ic;
}

extern "C" Ihandle* IupCalendar(void)
{
  return IupCreate("calendar");
}
