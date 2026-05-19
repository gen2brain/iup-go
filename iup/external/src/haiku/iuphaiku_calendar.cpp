/** \file
 * \brief Haiku Calendar (BPrivate::BCalendarView + month/year nav header)
 *
 * See Copyright Notice in "iup.h"
 */

#include <cmath>
#include <cstddef>
#include <cstdio>

#include <Button.h>
#include <CalendarView.h>
#include <DateTime.h>
#include <Message.h>
#include <Rect.h>
#include <String.h>
#include <StringView.h>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
}

#include "iuphaiku_drv.h"


/* BCalendarView is in BPrivate (private/shared headers, libshared.a).
 * Same widget Tracker / Deskbar uses for the tray clock's drop-down. */

#define IUPHAIKU_CAL_MSG         'IupC'
#define IUPHAIKU_CAL_MONTH_PREV  'mDn0'
#define IUPHAIKU_CAL_MONTH_NEXT  'mUp0'
#define IUPHAIKU_CAL_YEAR_PREV   'yDn0'
#define IUPHAIKU_CAL_YEAR_NEXT   'yUp0'


class IupHaikuCalendarGrid : public BPrivate::BCalendarView
{
public:
  IupHaikuCalendarGrid(Ihandle* ih)
    : BCalendarView("iup_cal_grid",
                              B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE | B_PULSE_NEEDED),
      fIhandle(ih) {}

  void AttachedToWindow() override
  {
    BCalendarView::AttachedToWindow();
    SetTarget(this);
    SetSelectionMessage(new BMessage(IUPHAIKU_CAL_MSG));
    SetInvocationMessage(new BMessage(IUPHAIKU_CAL_MSG));
  }

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_CAL_MSG && fIhandle)
    {
      Icallback cb = IupGetCallback(fIhandle, "VALUECHANGED_CB");
      int ret = cb ? cb(fIhandle) : IUP_DEFAULT;
      if (ret == IUP_CLOSE) IupExitLoop();
      if (Parent()) Parent()->Invalidate();
      if (Parent() && Parent()->Looper())
      {
        BMessage upd('upHd');
        BMessenger(Parent()).SendMessage(&upd);
      }
      return;
    }
    BCalendarView::MessageReceived(msg);
  }

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

private:
  Ihandle* fIhandle;
};


class IupHaikuCalendar : public BView
{
public:
  static constexpr float kPad = 5.0f;
  static constexpr float kGap = 5.0f;

  IupHaikuCalendar(Ihandle* ih)
    : BView(BRect(0, 0, 0, 0), "iup_calendar", B_FOLLOW_NONE,
            B_WILL_DRAW | B_FRAME_EVENTS),
      fIhandle(ih), fGrid(NULL),
      fYearLabel(NULL), fMonthLabel(NULL),
      fMonthPrev(NULL), fMonthNext(NULL),
      fYearPrev(NULL), fYearNext(NULL)
  {
    SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

    fMonthLabel = new BStringView(BRect(0, 0, 0, 0), "month", "", B_FOLLOW_NONE, B_WILL_DRAW);
    fYearLabel  = new BStringView(BRect(0, 0, 0, 0), "year",  "", B_FOLLOW_NONE, B_WILL_DRAW);
    fMonthLabel->SetAlignment(B_ALIGN_CENTER);
    fYearLabel->SetAlignment(B_ALIGN_CENTER);

    fMonthPrev = new BButton(BRect(0, 0, 0, 0), "mp", "-",
                             new BMessage(IUPHAIKU_CAL_MONTH_PREV), B_FOLLOW_NONE);
    fMonthNext = new BButton(BRect(0, 0, 0, 0), "mn", "+",
                             new BMessage(IUPHAIKU_CAL_MONTH_NEXT), B_FOLLOW_NONE);
    fYearPrev  = new BButton(BRect(0, 0, 0, 0), "yp", "-",
                             new BMessage(IUPHAIKU_CAL_YEAR_PREV),  B_FOLLOW_NONE);
    fYearNext  = new BButton(BRect(0, 0, 0, 0), "yn", "+",
                             new BMessage(IUPHAIKU_CAL_YEAR_NEXT),  B_FOLLOW_NONE);

    fGrid = new IupHaikuCalendarGrid(ih);
    fGrid->SetResizingMode(B_FOLLOW_NONE);

    AddChild(fMonthPrev);
    AddChild(fMonthLabel);
    AddChild(fMonthNext);
    AddChild(fYearPrev);
    AddChild(fYearLabel);
    AddChild(fYearNext);
    AddChild(fGrid);
  }

  void AttachedToWindow() override
  {
    BView::AttachedToWindow();
    fMonthPrev->SetTarget(this);
    fMonthNext->SetTarget(this);
    fYearPrev->SetTarget(this);
    fYearNext->SetTarget(this);
    UpdateHeader();
    LayoutChildren();
  }

  void FrameResized(float w, float h) override
  {
    BView::FrameResized(w, h);
    LayoutChildren();
  }

  void LayoutChildren()
  {
    BRect b = Bounds();
    if (b.Width() < 2 * kPad || b.Height() < 2 * kPad) return;

    font_height fh;
    fMonthLabel->GetFontHeight(&fh);
    float nav_h = ceilf(fh.ascent + fh.descent) + 12.0f;

    float month_w = ceilf(be_plain_font->StringWidth("September") + 8.0f);
    float year_w  = ceilf(be_plain_font->StringWidth("9999") + 8.0f);

    float x_left  = b.left + kPad;
    float x_right = b.right - kPad;
    float y_top   = b.top + kPad;

    fMonthPrev->MoveTo(x_left, y_top);
    fMonthPrev->ResizeTo(nav_h - 1, nav_h - 1);
    fMonthLabel->MoveTo(x_left + nav_h + kGap, y_top + (nav_h - (fh.ascent + fh.descent)) / 2);
    fMonthLabel->ResizeTo(month_w - 1, ceilf(fh.ascent + fh.descent));
    fMonthNext->MoveTo(x_left + nav_h + kGap + month_w + kGap, y_top);
    fMonthNext->ResizeTo(nav_h - 1, nav_h - 1);

    fYearNext->MoveTo(x_right - nav_h + 1, y_top);
    fYearNext->ResizeTo(nav_h - 1, nav_h - 1);
    fYearLabel->MoveTo(x_right - nav_h - kGap - year_w + 1, y_top + (nav_h - (fh.ascent + fh.descent)) / 2);
    fYearLabel->ResizeTo(year_w - 1, ceilf(fh.ascent + fh.descent));
    fYearPrev->MoveTo(x_right - nav_h - kGap - year_w - kGap - nav_h + 1, y_top);
    fYearPrev->ResizeTo(nav_h - 1, nav_h - 1);

    float grid_y = y_top + nav_h + kGap;
    float grid_w = b.Width() - 2 * kPad;
    float grid_h = b.bottom - grid_y - kPad;
    if (grid_w < 1) grid_w = 1;
    if (grid_h < 1) grid_h = 1;
    fGrid->MoveTo(x_left, grid_y);
    fGrid->ResizeTo(grid_w, grid_h);
  }

  void MessageReceived(BMessage* msg) override
  {
    if (!msg) return;
    BDate date = fGrid->Date();
    bool handled = true;
    switch (msg->what)
    {
      case IUPHAIKU_CAL_MONTH_PREV: date.AddMonths(-1); break;
      case IUPHAIKU_CAL_MONTH_NEXT: date.AddMonths(1);  break;
      case IUPHAIKU_CAL_YEAR_PREV:  date.AddYears(-1);  break;
      case IUPHAIKU_CAL_YEAR_NEXT:  date.AddYears(1);   break;
      case 'upHd': UpdateHeader(); return;
      default: handled = false; break;
    }
    if (handled)
    {
      fGrid->SetDate(date);
      UpdateHeader();
      if (fIhandle)
      {
        Icallback cb = (Icallback)IupGetCallback(fIhandle, "VALUECHANGED_CB");
        if (cb && cb(fIhandle) == IUP_CLOSE) IupExitLoop();
      }
      return;
    }
    BView::MessageReceived(msg);
  }

  void UpdateHeader()
  {
    if (!fGrid) return;
    BDate d = fGrid->Date();
    fMonthLabel->SetText(d.LongMonthName(d.Month()).String());
    BString y; y << d.Year();
    fYearLabel->SetText(y.String());
  }

  IupHaikuCalendarGrid* Grid() const { return fGrid; }
  void SetIhandle(Ihandle* ih) { fIhandle = ih; if (fGrid) fGrid->SetIhandle(ih); }

private:
  Ihandle* fIhandle;
  IupHaikuCalendarGrid* fGrid;
  BStringView* fYearLabel;
  BStringView* fMonthLabel;
  BButton* fMonthPrev;
  BButton* fMonthNext;
  BButton* fYearPrev;
  BButton* fYearNext;
};

/* Helpers */

static int haikuCalendarParseValue(const char* value, int* y, int* m, int* d)
{
  if (!value) return 0;
  if (iupStrEqualNoCase(value, "TODAY"))
  {
    BDate today = BDate::CurrentDate(B_LOCAL_TIME);
    *y = today.Year(); *m = today.Month(); *d = today.Day();
    return 1;
  }
  return sscanf(value, "%d/%d/%d", y, m, d) == 3;
}

/* Attribute setters */

static int haikuCalendarSetValueAttrib(Ihandle* ih, const char* value)
{
  IupHaikuCalendar* cal = (IupHaikuCalendar*)ih->handle;
  if (!cal) return 0;
  int y, m, d;
  if (!haikuCalendarParseValue(value, &y, &m, &d)) return 0;
  LooperLockGuard guard(cal->Looper());
  cal->Grid()->SetDate(y, m, d);
  cal->UpdateHeader();
  return 0;
}

static char* haikuCalendarGetValueAttrib(Ihandle* ih)
{
  IupHaikuCalendar* cal = (IupHaikuCalendar*)ih->handle;
  if (!cal) return NULL;
  IupHaikuCalendarGrid* g = cal->Grid();
  return iupStrReturnStrf("%d/%d/%d", g->Year(), g->Month(), g->Day());
}

static char* haikuCalendarGetTodayAttrib(Ihandle* /*ih*/)
{
  BDate today = BDate::CurrentDate(B_LOCAL_TIME);
  return iupStrReturnStrf("%d/%d/%d", today.Year(), today.Month(), today.Day());
}

static char* haikuCalendarGetWeekDayAttrib(Ihandle* ih)
{
  IupHaikuCalendar* cal = (IupHaikuCalendar*)ih->handle;
  if (!cal) return NULL;
  IupHaikuCalendarGrid* g = cal->Grid();
  /* IUP weekday: 1=Sun..7=Sat; BDate::DayOfWeek: 1=Mon..7=Sun. Remap. */
  BDate d(g->Year(), g->Month(), g->Day());
  int dow = d.DayOfWeek();
  int iup_dow = (dow == 7) ? 1 : dow + 1;
  return iupStrReturnInt(iup_dow);
}

static int haikuCalendarSetWeekNumbersAttrib(Ihandle* ih, const char* value)
{
  IupHaikuCalendar* cal = (IupHaikuCalendar*)ih->handle;
  if (!cal) return 1;
  LooperLockGuard guard(cal->Looper());
  cal->Grid()->SetWeekNumberHeaderVisible(iupStrBoolean(value) ? true : false);
  return 1;
}

/* Map */

static void haikuCalendarComputeNaturalSizeMethod(Ihandle* ih, int* w, int* h, int* /*children_expand*/)
{
  IupHaikuCalendar* cal = (IupHaikuCalendar*)ih->handle;
  if (cal)
  {
    LooperLockGuard guard(cal->Looper());
    float gw = 0.0f, gh = 0.0f;
    cal->Grid()->GetPreferredSize(&gw, &gh);

    font_height fh;
    cal->GetFontHeight(&fh);
    float nav_h = ceilf(fh.ascent + fh.descent) + 12.0f;
    float month_w = ceilf(be_plain_font->StringWidth("September") + 8.0f);
    float year_w  = ceilf(be_plain_font->StringWidth("9999") + 8.0f);
    float min_nav_w = 4 * nav_h + month_w + year_w + 6 * IupHaikuCalendar::kGap;

    *w = (int)ceilf(iupMAX(gw, min_nav_w) + 2 * IupHaikuCalendar::kPad);
    *h = (int)ceilf(gh + nav_h + IupHaikuCalendar::kGap + 2 * IupHaikuCalendar::kPad);
    return;
  }
  /* Pre-handle fallback: header row + 7-row grid. */
  int cw, ch;
  iupdrvFontGetCharSize(ih, &cw, &ch);
  *w = cw * 3 * 8 + 16;
  *h = (ch + 4) * 7 + (ch + 16) + 16;
}

static int haikuCalendarMapMethod(Ihandle* ih)
{
  IupHaikuCalendar* cal = new IupHaikuCalendar(ih);
  ih->handle = (InativeHandle*)cal;
  iuphaikuAddToParent(ih);
  iuphaikuUpdateWidgetFont(ih, cal);

  char* value = iupAttribGet(ih, "VALUE");
  if (value) haikuCalendarSetValueAttrib(ih, value);
  else
  {
    BDate today = BDate::CurrentDate(B_LOCAL_TIME);
    {
      LooperLockGuard guard(cal->Looper());
      cal->Grid()->SetDate(today.Year(), today.Month(), today.Day());
      cal->UpdateHeader();
    }
  }
  return IUP_NOERROR;
}

static void haikuCalendarUnMapMethod(Ihandle* ih)
{
  IupHaikuCalendar* cal = (IupHaikuCalendar*)ih->handle;
  if (cal) cal->SetIhandle(NULL);
  iupdrvBaseUnMapMethod(ih);
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
  ic->Map = haikuCalendarMapMethod;
  ic->UnMap = haikuCalendarUnMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->ComputeNaturalSize = haikuCalendarComputeNaturalSizeMethod;

  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  iupBaseRegisterCommonCallbacks(ic);
  iupBaseRegisterCommonAttrib(ic);
  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "VALUE", haikuCalendarGetValueAttrib, haikuCalendarSetValueAttrib, NULL, "TODAY", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", haikuCalendarGetTodayAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WEEKDAY", haikuCalendarGetWeekDayAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WEEKNUMBERS", NULL, haikuCalendarSetWeekNumbersAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  return ic;
}

extern "C" IUP_API Ihandle* IupCalendar(void)
{
  return IupCreate("calendar");
}
