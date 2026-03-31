/** \file
 * \brief Calendar Control - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Repeat_Button.H>
#include <FL/fl_draw.H>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_register.h"
}

#include "iupfltk_drv.h"


static int fltkCalendarDaysInMonth(int year, int month)
{
  static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if (month < 1 || month > 12) return 30;
  int d = days[month - 1];
  if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0))
    d = 29;
  return d;
}

static int fltkCalendarDayOfWeek(int year, int month, int day)
{
  struct tm t = {};
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  mktime(&t);
  return t.tm_wday;
}

static int fltkCalendarISOWeek(int year, int month, int day)
{
  struct tm t = {};
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  mktime(&t);

  int yday = t.tm_yday;
  int wday = t.tm_wday;
  if (wday == 0) wday = 7;

  int week = (yday - wday + 10) / 7;
  if (week < 1) week = 52;
  else if (week > 52)
  {
    int jan1_wday = fltkCalendarDayOfWeek(year + 1, 1, 1);
    if (jan1_wday == 0) jan1_wday = 7;
    if (jan1_wday <= 4) week = 1;
  }
  return week;
}


class IupFltkCalendar : public Fl_Group
{
public:
  Ihandle* iup_handle;

  int cur_year, cur_month;
  int sel_year, sel_month, sel_day;
  int focus_row, focus_col;
  int show_week_numbers;

  Fl_Button* btn_prev;
  Fl_Box* lbl_title;
  Fl_Button* btn_next;
  Fl_Box* day_name_labels[7];
  Fl_Box* week_labels[6];
  Fl_Box* day_cells[6][7];

  int grid_day[6][7];
  int grid_month[6][7];
  int grid_year[6][7];

  char title_buf[64];
  char wk_bufs[6][8];
  char day_bufs[6][7][8];

  IupFltkCalendar(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Group(x, y, w, h), iup_handle(ih),
      focus_row(0), focus_col(0), show_week_numbers(0)
  {
    box(FL_FLAT_BOX);

    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    cur_year = t->tm_year + 1900;
    cur_month = t->tm_mon + 1;
    sel_year = cur_year;
    sel_month = cur_month;
    sel_day = t->tm_mday;

    btn_prev = new Fl_Button(0, 0, 1, 1, "@<");
    btn_prev->box(FL_FLAT_BOX);
    btn_prev->callback(prevMonthCB, this);

    lbl_title = new Fl_Box(0, 0, 1, 1);
    lbl_title->box(FL_FLAT_BOX);
    lbl_title->labelfont(FL_HELVETICA_BOLD);

    btn_next = new Fl_Button(0, 0, 1, 1, "@>");
    btn_next->box(FL_FLAT_BOX);
    btn_next->callback(nextMonthCB, this);

    static const char* day_names[] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };
    for (int i = 0; i < 7; i++)
    {
      day_name_labels[i] = new Fl_Box(0, 0, 1, 1, day_names[i]);
      day_name_labels[i]->box(FL_FLAT_BOX);
      day_name_labels[i]->labelfont(FL_HELVETICA_BOLD);
      day_name_labels[i]->labelsize(FL_NORMAL_SIZE - 1);
    }

    for (int r = 0; r < 6; r++)
    {
      week_labels[r] = new Fl_Box(0, 0, 1, 1);
      week_labels[r]->box(FL_FLAT_BOX);
      week_labels[r]->labelsize(FL_NORMAL_SIZE - 2);
      week_labels[r]->labelcolor(FL_INACTIVE_COLOR);
      week_labels[r]->hide();
    }

    for (int r = 0; r < 6; r++)
    {
      for (int c = 0; c < 7; c++)
      {
        day_cells[r][c] = new Fl_Box(0, 0, 1, 1);
        day_cells[r][c]->box(FL_FLAT_BOX);
        grid_day[r][c] = 0;
        grid_month[r][c] = 0;
        grid_year[r][c] = 0;
      }
    }

    end();
    rebuildGrid();
  }

  void rebuildGrid()
  {
    int days_in_month = fltkCalendarDaysInMonth(cur_year, cur_month);
    int first_wday = fltkCalendarDayOfWeek(cur_year, cur_month, 1);

    int prev_month = cur_month - 1;
    int prev_year = cur_year;
    if (prev_month < 1) { prev_month = 12; prev_year--; }
    int days_in_prev = fltkCalendarDaysInMonth(prev_year, prev_month);

    int next_month = cur_month + 1;
    int next_year = cur_year;
    if (next_month > 12) { next_month = 1; next_year++; }

    int day = 1;
    int next_day = 1;

    for (int r = 0; r < 6; r++)
    {
      for (int c = 0; c < 7; c++)
      {
        int cell = r * 7 + c;
        if (cell < first_wday)
        {
          int d = days_in_prev - first_wday + cell + 1;
          grid_day[r][c] = d;
          grid_month[r][c] = prev_month;
          grid_year[r][c] = prev_year;
        }
        else if (day <= days_in_month)
        {
          grid_day[r][c] = day;
          grid_month[r][c] = cur_month;
          grid_year[r][c] = cur_year;
          day++;
        }
        else
        {
          grid_day[r][c] = next_day;
          grid_month[r][c] = next_month;
          grid_year[r][c] = next_year;
          next_day++;
        }
      }
    }

    updateLabels();
    layoutChildren();
  }

  void updateLabels()
  {
    static const char* month_names[] = {
      "January", "February", "March", "April", "May", "June",
      "July", "August", "September", "October", "November", "December"
    };

    snprintf(title_buf, sizeof(title_buf), "%s %d", month_names[cur_month - 1], cur_year);
    lbl_title->label(title_buf);

    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    int today_y = t->tm_year + 1900;
    int today_m = t->tm_mon + 1;
    int today_d = t->tm_mday;

    for (int r = 0; r < 6; r++)
    {
      if (show_week_numbers)
      {
        int wk = fltkCalendarISOWeek(grid_year[r][0], grid_month[r][0], grid_day[r][0]);
        snprintf(wk_bufs[r], sizeof(wk_bufs[r]), "%d", wk);
        week_labels[r]->label(wk_bufs[r]);
        week_labels[r]->show();
      }
      else
        week_labels[r]->hide();

      for (int c = 0; c < 7; c++)
      {
        snprintf(day_bufs[r][c], sizeof(day_bufs[r][c]), "%d", grid_day[r][c]);
        day_cells[r][c]->label(day_bufs[r][c]);

        int is_cur_month = (grid_month[r][c] == cur_month);
        int is_selected = (grid_year[r][c] == sel_year && grid_month[r][c] == sel_month && grid_day[r][c] == sel_day);
        int is_today = (grid_year[r][c] == today_y && grid_month[r][c] == today_m && grid_day[r][c] == today_d);

        if (is_selected)
        {
          day_cells[r][c]->box(FL_FLAT_BOX);
          day_cells[r][c]->color(FL_SELECTION_COLOR);
          day_cells[r][c]->labelcolor(fl_contrast(FL_FOREGROUND_COLOR, FL_SELECTION_COLOR));
          day_cells[r][c]->labelfont(FL_HELVETICA_BOLD);
        }
        else if (is_today)
        {
          day_cells[r][c]->box(FL_THIN_UP_BOX);
          day_cells[r][c]->color(FL_BACKGROUND_COLOR);
          day_cells[r][c]->labelcolor(FL_FOREGROUND_COLOR);
          day_cells[r][c]->labelfont(FL_HELVETICA_BOLD);
        }
        else if (!is_cur_month)
        {
          day_cells[r][c]->box(FL_FLAT_BOX);
          day_cells[r][c]->color(FL_BACKGROUND_COLOR);
          day_cells[r][c]->labelcolor(FL_INACTIVE_COLOR);
          day_cells[r][c]->labelfont(FL_HELVETICA);
        }
        else
        {
          day_cells[r][c]->box(FL_FLAT_BOX);
          day_cells[r][c]->color(FL_BACKGROUND_COLOR);
          day_cells[r][c]->labelcolor(FL_FOREGROUND_COLOR);
          day_cells[r][c]->labelfont(FL_HELVETICA);
        }
      }
    }
  }

  void layoutChildren()
  {
    int X = x(), Y = y(), W = w(), H = h();
    int cols = show_week_numbers ? 8 : 7;
    int cell_w = W / cols;
    int cell_h = H / 8;
    int wk_col_w = show_week_numbers ? cell_w : 0;
    int grid_w = W - wk_col_w;
    cell_w = grid_w / 7;

    btn_prev->resize(X + wk_col_w, Y, cell_w, cell_h);
    lbl_title->resize(X + wk_col_w + cell_w, Y, cell_w * 5, cell_h);
    btn_next->resize(X + wk_col_w + cell_w * 6, Y, cell_w, cell_h);

    for (int i = 0; i < 7; i++)
      day_name_labels[i]->resize(X + wk_col_w + i * cell_w, Y + cell_h, cell_w, cell_h);

    for (int r = 0; r < 6; r++)
    {
      int ry = Y + (r + 2) * cell_h;

      if (show_week_numbers)
        week_labels[r]->resize(X, ry, wk_col_w, cell_h);

      for (int c = 0; c < 7; c++)
        day_cells[r][c]->resize(X + wk_col_w + c * cell_w, ry, cell_w, cell_h);
    }

    redraw();
  }

  void selectDay(int r, int c)
  {
    sel_year = grid_year[r][c];
    sel_month = grid_month[r][c];
    sel_day = grid_day[r][c];

    if (grid_month[r][c] != cur_month)
    {
      cur_month = grid_month[r][c];
      cur_year = grid_year[r][c];
      rebuildGrid();
    }
    else
    {
      updateLabels();
      redraw();
    }

    if (iup_handle)
      iupBaseCallValueChangedCb(iup_handle);
  }

  void prevMonth()
  {
    cur_month--;
    if (cur_month < 1) { cur_month = 12; cur_year--; }
    rebuildGrid();
  }

  void nextMonth()
  {
    cur_month++;
    if (cur_month > 12) { cur_month = 1; cur_year++; }
    rebuildGrid();
  }

protected:
  int handle(int event) override
  {
    switch (event)
    {
      case FL_FOCUS:
      case FL_UNFOCUS:
        if (iup_handle)
          iupfltkFocusInOutEvent(this, iup_handle, event);
        return 1;

      case FL_ENTER:
      case FL_MOVE:
      {
        int ex = Fl::event_x();
        int ey = Fl::event_y();
        int over_day = 0;

        if (day_cells[0][0]->y() > 0)
        {
          for (int r = 0; r < 6 && !over_day; r++)
            for (int c = 0; c < 7 && !over_day; c++)
            {
              Fl_Box* cell = day_cells[r][c];
              if (ex >= cell->x() && ex < cell->x() + cell->w() &&
                  ey >= cell->y() && ey < cell->y() + cell->h() &&
                  grid_day[r][c] > 0)
                over_day = 1;
            }
        }

        window()->cursor(over_day ? FL_CURSOR_HAND : FL_CURSOR_DEFAULT);

        if (event == FL_ENTER && iup_handle)
          iupfltkEnterLeaveEvent(this, iup_handle, event);
        return 1;
      }

      case FL_LEAVE:
        window()->cursor(FL_CURSOR_DEFAULT);
        if (iup_handle)
          iupfltkEnterLeaveEvent(this, iup_handle, event);
        return 1;

      case FL_PUSH:
      {
        int ex = Fl::event_x();
        int ey = Fl::event_y();

        for (int r = 0; r < 6; r++)
        {
          for (int c = 0; c < 7; c++)
          {
            Fl_Box* cell = day_cells[r][c];
            if (ex >= cell->x() && ex < cell->x() + cell->w() &&
                ey >= cell->y() && ey < cell->y() + cell->h())
            {
              focus_row = r;
              focus_col = c;
              selectDay(r, c);
              return 1;
            }
          }
        }

        return Fl_Group::handle(event);
      }

      case FL_KEYBOARD:
      {
        if (iup_handle && iupfltkKeyPressEvent(this, iup_handle))
          return 1;

        int key = Fl::event_key();
        switch (key)
        {
          case FL_Left:
            if (focus_col > 0) focus_col--;
            else if (focus_row > 0) { focus_row--; focus_col = 6; }
            selectDay(focus_row, focus_col);
            return 1;
          case FL_Right:
            if (focus_col < 6) focus_col++;
            else if (focus_row < 5) { focus_row++; focus_col = 0; }
            selectDay(focus_row, focus_col);
            return 1;
          case FL_Up:
            if (focus_row > 0) focus_row--;
            selectDay(focus_row, focus_col);
            return 1;
          case FL_Down:
            if (focus_row < 5) focus_row++;
            selectDay(focus_row, focus_col);
            return 1;
          case FL_Page_Up:
            prevMonth();
            return 1;
          case FL_Page_Down:
            nextMonth();
            return 1;
        }
        break;
      }
    }

    return Fl_Group::handle(event);
  }

  void resize(int X, int Y, int W, int H) override
  {
    Fl_Group::resize(X, Y, W, H);
    layoutChildren();
  }

  static void prevMonthCB(Fl_Widget*, void* data)
  {
    ((IupFltkCalendar*)data)->prevMonth();
  }

  static void nextMonthCB(Fl_Widget*, void* data)
  {
    ((IupFltkCalendar*)data)->nextMonth();
  }
};


static void fltkCalendarComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  (void)children_expand;

  int cols = 7;
  if (iupAttribGetBoolean(ih, "WEEKNUMBERS"))
    cols = 8;

  iupdrvFontGetMultiLineStringSize(ih, "W8W", w, h);
  *h += 4;
  (*w) *= cols;
  (*h) *= 8;
  *h += 4;
}

static int fltkCalendarSetValueAttrib(Ihandle* ih, const char* value)
{
  IupFltkCalendar* cal = (IupFltkCalendar*)ih->handle;
  if (!cal) return 0;

  if (iupStrEqualNoCase(value, "TODAY"))
  {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    cal->sel_year = t->tm_year + 1900;
    cal->sel_month = t->tm_mon + 1;
    cal->sel_day = t->tm_mday;
    cal->cur_year = cal->sel_year;
    cal->cur_month = cal->sel_month;
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
      cal->sel_year = year;
      cal->sel_month = month;
      cal->sel_day = day;
      cal->cur_year = year;
      cal->cur_month = month;
    }
  }

  cal->rebuildGrid();
  return 0;
}

static char* fltkCalendarGetValueAttrib(Ihandle* ih)
{
  IupFltkCalendar* cal = (IupFltkCalendar*)ih->handle;
  if (!cal) return NULL;
  return iupStrReturnStrf("%d/%d/%d", cal->sel_year, cal->sel_month, cal->sel_day);
}

static char* fltkCalendarGetTodayAttrib(Ihandle* ih)
{
  (void)ih;
  time_t now = time(NULL);
  struct tm* t = localtime(&now);
  return iupStrReturnStrf("%d/%d/%d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

static int fltkCalendarSetWeekNumbersAttrib(Ihandle* ih, const char* value)
{
  IupFltkCalendar* cal = (IupFltkCalendar*)ih->handle;
  if (!cal) return 0;

  cal->show_week_numbers = iupStrBoolean(value);
  cal->rebuildGrid();
  return 0;
}

static int fltkCalendarMapMethod(Ihandle* ih)
{
  IupFltkCalendar* cal = new IupFltkCalendar(0, 0, 10, 10, ih);
  ih->handle = (InativeHandle*)cal;

  if (iupAttribGetBoolean(ih, "WEEKNUMBERS"))
    cal->show_week_numbers = 1;

  iupfltkAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupfltkSetCanFocus(cal, 0);

  return IUP_NOERROR;
}

static void fltkCalendarUnMapMethod(Ihandle* ih)
{
  IupFltkCalendar* cal = (IupFltkCalendar*)ih->handle;
  if (cal)
  {
    delete cal;
    ih->handle = NULL;
  }
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
  ic->Map = fltkCalendarMapMethod;
  ic->UnMap = fltkCalendarUnMapMethod;
  ic->ComputeNaturalSize = fltkCalendarComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  iupBaseRegisterCommonCallbacks(ic);
  iupBaseRegisterCommonAttrib(ic);
  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "VALUE", fltkCalendarGetValueAttrib, fltkCalendarSetValueAttrib, NULL, "TODAY", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WEEKNUMBERS", NULL, fltkCalendarSetWeekNumbersAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", fltkCalendarGetTodayAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  return ic;
}

extern "C" IUP_API Ihandle* IupCalendar(void)
{
  return IupCreate("calendar");
}
