/** \file
 * \brief WinUI Driver - Calendar Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>

#include <cstdlib>
#include <cmath>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_dlglist.h"
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
  bool ignoreChange;

  winrt::event_token gotFocusToken{};
  winrt::event_token lostFocusToken{};

  IupWinUICalendarAux() : selectedDatesChangedToken{}, ignoreChange(false) {}
};

static DateTime winuiCalendarMakeDateTime(int year, int month, int day)
{
  SYSTEMTIME st = {0};
  FILETIME ft;

  st.wYear = static_cast<WORD>(year);
  st.wMonth = static_cast<WORD>(month);
  st.wDay = static_cast<WORD>(day);
  st.wHour = 12;

  SystemTimeToFileTime(&st, &ft);
  ULARGE_INTEGER uli;
  uli.LowPart = ft.dwLowDateTime;
  uli.HighPart = ft.dwHighDateTime;

  return DateTime{TimeSpan{static_cast<int64_t>(uli.QuadPart)}};
}

static void winuiCalendarGetDate(DateTime dt, int* year, int* month, int* day)
{
  auto ticks = dt.time_since_epoch().count();

  ULARGE_INTEGER uli;
  uli.QuadPart = static_cast<ULONGLONG>(ticks);

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
  IFn cb = static_cast<IFn>(IupGetCallback(ih, "VALUECHANGED_CB"));
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

static int winuiCalendarSetValueAttrib(Ihandle* ih, const char* value)
{
  auto cv = winuiGetHandle<CalendarView>(ih);
  auto* aux = winuiGetAux<IupWinUICalendarAux>(ih, IUPWINUI_CALENDAR_AUX);
  if (!cv || !aux)
    return 0;

  aux->ignoreChange = true;
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
    if (iupStrToDate(value, &year, &month, &day))
    {
      auto dt = winuiCalendarMakeDateTime(year, month, day);
      cv.SelectedDates().Clear();
      cv.SelectedDates().Append(dt);
      cv.SetDisplayDate(dt);
    }
  }
  aux->ignoreChange = false;

  return 0;
}

static char* winuiCalendarGetValueAttrib(Ihandle* ih)
{
  auto cv = winuiGetHandle<CalendarView>(ih);
  if (cv && cv.SelectedDates().Size() > 0)
  {
    auto dt = cv.SelectedDates().GetAt(0);
    int year, month, day;
    winuiCalendarGetDate(dt, &year, &month, &day);
    return iupStrReturnStrf("%d/%02d/%02d", year, month, day);
  }
  return nullptr;
}

static char* winuiCalendarGetTodayAttrib(Ihandle* ih)
{
  (void)ih;
  SYSTEMTIME st;
  GetLocalTime(&st);
  return iupStrReturnStrf("%d/%02d/%02d", st.wYear, st.wMonth, st.wDay);
}

static void winuiCalendarInitResources(CalendarView const& cv)
{
  cv.Resources().Insert(box_value(L"CalendarViewWeekDayPadding"), box_value(Thickness{0, 12, 0, 12}));
}

static int winuiCalendarMapMethod(Ihandle* ih)
{
  auto* aux = new IupWinUICalendarAux();

  CalendarView cv = CalendarView();
  winuiCalendarInitResources(cv);
  cv.HorizontalAlignment(HorizontalAlignment::Left);
  cv.VerticalAlignment(VerticalAlignment::Top);
  cv.SelectionMode(CalendarViewSelectionMode::Single);

  auto now = winrt::clock::now();
  cv.SelectedDates().Append(now);

  aux->selectedDatesChangedToken = cv.SelectedDatesChanged([ih](CalendarView const&, CalendarViewSelectedDatesChangedEventArgs const&) {
    auto* a = winuiGetAux<IupWinUICalendarAux>(ih, IUPWINUI_CALENDAR_AUX);
    if (a && !a->ignoreChange)
      winuiCalendarCallValueChanged(ih);
  });

  Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
  if (parentCanvas)
    parentCanvas.Children().Append(cv);

  aux->gotFocusToken = cv.GotFocus([ih](IInspectable const&, RoutedEventArgs const&) { iupwinuiFocusInOutEvent(ih, 1); });
  aux->lostFocusToken = cv.LostFocus([ih](IInspectable const&, RoutedEventArgs const&) { iupwinuiFocusInOutEvent(ih, 0); });

  winuiSetAux(ih, IUPWINUI_CALENDAR_AUX, aux);
  winuiStoreHandle(ih, cv);
  return IUP_NOERROR;
}

static void winuiCalendarUnMapMethod(Ihandle* ih)
{
  auto* aux = winuiGetAux<IupWinUICalendarAux>(ih, IUPWINUI_CALENDAR_AUX);

  if (ih->handle && aux)
  {
    auto cv = winuiGetHandle<CalendarView>(ih);
    if (cv)
    {
      if (aux->selectedDatesChangedToken)
        cv.SelectedDatesChanged(aux->selectedDatesChangedToken);
      if (aux->gotFocusToken)
        cv.GotFocus(aux->gotFocusToken);
      if (aux->lostFocusToken)
        cv.LostFocus(aux->lostFocusToken);
    }
    iupwinuiRemoveFromParent(ih);
    winuiReleaseHandle<CalendarView>(ih);
  }

  winuiFreeAux<IupWinUICalendarAux>(ih, IUPWINUI_CALENDAR_AUX);
  ih->handle = nullptr;
}

static void winuiCalendarComputeNaturalSizeMethod(Ihandle* ih, int* w, int* h, int* children_expand)
{
  static Size desired = {0, 0};
  (void)children_expand;

  if (desired.Width <= 0 || desired.Height <= 0)
  {
    Ihandle* dlg;
    for (dlg = iupDlgListFirst(); dlg; dlg = iupDlgListNext())
    {
      IupWinUIDialogAux* dlgAux = dlg->handle ? winuiGetAux<IupWinUIDialogAux>(dlg, IUPWINUI_DIALOG_AUX) : nullptr;
      if (dlgAux && dlgAux->rootPanel && dlgAux->rootPanel.XamlRoot())
      {
        CalendarView cv;
        winuiCalendarInitResources(cv);
        cv.Opacity(0);
        dlgAux->rootPanel.Children().Append(cv);
        cv.UpdateLayout();
        cv.Measure(Size(10000, 10000));
        desired = cv.DesiredSize();

        uint32_t index;
        if (dlgAux->rootPanel.Children().IndexOf(cv, index))
          dlgAux->rootPanel.Children().RemoveAt(index);
        break;
      }
    }
  }

  if (desired.Width <= 0 || desired.Height <= 0)
  {
    *w = 300;
    *h = 330;
    return;
  }

  double scale = iupwinuiGetScale(ih);
  *w = static_cast<int>(ceil(desired.Width * scale));
  *h = static_cast<int>(ceil(desired.Height * scale));
}

extern "C" Iclass* iupCalendarNewClass(void)
{
  Iclass* ic = iupClassNew(nullptr);

  ic->name = const_cast<char*>("calendar");
  ic->format = nullptr;
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

  iupClassRegisterAttribute(ic, "VALUE", winuiCalendarGetValueAttrib, winuiCalendarSetValueAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", winuiCalendarGetTodayAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WEEKNUMBERS", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  return ic;
}

extern "C" Ihandle* IupCalendar(void)
{
  return IupCreate("calendar");
}
