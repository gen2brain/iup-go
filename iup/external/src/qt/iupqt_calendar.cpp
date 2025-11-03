/** \file
 * \brief Calendar Control - Qt implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QCalendarWidget>
#include <QDate>
#include <QWidget>
#include <QEvent>
#include <QEnterEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>

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
#include "iup_array.h"
#include "iup_mask.h"
#include "iup_text.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Custom Qt Calendar with Event Handling
 ****************************************************************************/

class IupQtCalendar : public QCalendarWidget
{
public:
  Ihandle* iup_handle;

  IupQtCalendar(Ihandle* ih)
    : QCalendarWidget(), iup_handle(ih)
  {
    setGridVisible(true);
    setNavigationBarVisible(true);
  }

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* event) override
#else
  void enterEvent(QEvent* event) override
#endif
  {
    QCalendarWidget::enterEvent(event);
    iupqtEnterLeaveEvent(this, event, iup_handle);
  }

  void leaveEvent(QEvent* event) override
  {
    QCalendarWidget::leaveEvent(event);
    iupqtEnterLeaveEvent(this, event, iup_handle);
  }

  void focusInEvent(QFocusEvent* event) override
  {
    QCalendarWidget::focusInEvent(event);
    iupqtFocusInOutEvent(this, event, iup_handle);
  }

  void focusOutEvent(QFocusEvent* event) override
  {
    QCalendarWidget::focusOutEvent(event);
    iupqtFocusInOutEvent(this, event, iup_handle);
  }

  void keyPressEvent(QKeyEvent* event) override
  {
    if (iupqtKeyPressEvent(this, event, iup_handle))
    {
      event->accept();
      return;
    }
    QCalendarWidget::keyPressEvent(event);
  }

  void mousePressEvent(QMouseEvent* event) override
  {
    QCalendarWidget::mousePressEvent(event);
    iupqtMouseButtonEvent(this, event, iup_handle);
  }

  void mouseReleaseEvent(QMouseEvent* event) override
  {
    QCalendarWidget::mouseReleaseEvent(event);
    iupqtMouseButtonEvent(this, event, iup_handle);
  }

  void mouseMoveEvent(QMouseEvent* event) override
  {
    QCalendarWidget::mouseMoveEvent(event);
    iupqtMouseMoveEvent(this, event, iup_handle);
  }
};

/****************************************************************************
 * Natural Size Calculation
 ****************************************************************************/

static void qtCalendarComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  (void)children_expand;

  if (ih->handle)
  {
    IupQtCalendar* calendar = (IupQtCalendar*)ih->handle;
    QSize size = calendar->sizeHint();
    *w = size.width();
    *h = size.height();
  }
  else
  {
    iupdrvFontGetMultiLineStringSize(ih, "W8W", w, h);

    *h += 4;

    (*w) *= 7;
    (*h) *= 8;

    *h += 4;

    iupdrvTextAddBorders(ih, w, h);
  }
}

/****************************************************************************
 * Attribute Setters
 ****************************************************************************/

static int qtCalendarSetWeekNumbersAttrib(Ihandle* ih, const char* value)
{
  IupQtCalendar* calendar = (IupQtCalendar*)ih->handle;

  if (calendar)
  {
    if (iupStrBoolean(value))
      calendar->setVerticalHeaderFormat(QCalendarWidget::ISOWeekNumbers);
    else
      calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

    return 1;
  }

  return 0;
}

static int qtCalendarSetValueAttrib(Ihandle* ih, const char* value)
{
  IupQtCalendar* calendar = (IupQtCalendar*)ih->handle;

  if (!calendar)
    return 0;

  if (iupStrEqualNoCase(value, "TODAY"))
  {
    QDate today = QDate::currentDate();
    calendar->setSelectedDate(today);
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

      QDate date(year, month, day);
      if (date.isValid())
        calendar->setSelectedDate(date);
    }
  }

  return 0;
}

static char* qtCalendarGetValueAttrib(Ihandle* ih)
{
  IupQtCalendar* calendar = (IupQtCalendar*)ih->handle;

  if (calendar)
  {
    QDate date = calendar->selectedDate();
    return iupStrReturnStrf("%d/%d/%d", date.year(), date.month(), date.day());
  }

  return nullptr;
}

static char* qtCalendarGetTodayAttrib(Ihandle* ih)
{
  (void)ih;

  QDate today = QDate::currentDate();
  return iupStrReturnStrf("%d/%d/%d", today.year(), today.month(), today.day());
}

static int qtCalendarSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupQtCalendar* calendar = (IupQtCalendar*)ih->handle;

  if (calendar)
  {
    QPalette palette = calendar->palette();
    palette.setColor(QPalette::Window, QColor(r, g, b));
    palette.setColor(QPalette::Base, QColor(r, g, b));
    calendar->setPalette(palette);
    calendar->setAutoFillBackground(true);
    return 1;
  }

  return 0;
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/

static void qtCalendarSelectionChanged(IupQtCalendar* calendar, Ihandle* ih)
{
  (void)calendar;
  iupBaseCallValueChangedCb(ih);
}

static void qtCalendarActivated(IupQtCalendar* calendar, const QDate& date, Ihandle* ih)
{
  (void)calendar;
  (void)date;

  IFns cb = (IFns)IupGetCallback(ih, "SELECT_CB");
  if (cb)
  {
    QDate selected = calendar->selectedDate();
    char date_str[64];
    sprintf(date_str, "%d/%d/%d", selected.year(), selected.month(), selected.day());
    cb(ih, date_str);
  }
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

static int qtCalendarMapMethod(Ihandle* ih)
{
  IupQtCalendar* calendar = new IupQtCalendar(ih);

  if (!calendar)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)calendar;

  if (iupAttribGetBoolean(ih, "WEEKNUMBERS"))
    calendar->setVerticalHeaderFormat(QCalendarWidget::ISOWeekNumbers);

  iupqtAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqtSetCanFocus(calendar, 0);

  QObject::connect(calendar, &QCalendarWidget::selectionChanged, [calendar, ih]() {
    qtCalendarSelectionChanged(calendar, ih);
  });

  QObject::connect(calendar, &QCalendarWidget::activated, [calendar, ih](const QDate& date) {
    qtCalendarActivated(calendar, date, ih);
  });

  return IUP_NOERROR;
}

/****************************************************************************
 * UnMap Method
 ****************************************************************************/

static void qtCalendarUnMapMethod(Ihandle* ih)
{
  if (ih->handle)
  {
    IupQtCalendar* calendar = (IupQtCalendar*)ih->handle;

    iupqtTipsDestroy(ih);

    delete calendar;
    ih->handle = nullptr;
  }
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" Iclass* iupCalendarNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = (char*)"calendar";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  ic->New = iupCalendarNewClass;

  ic->Map = qtCalendarMapMethod;
  ic->UnMap = qtCalendarUnMapMethod;
  ic->ComputeNaturalSize = qtCalendarComputeNaturalSizeMethod;

  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");
  iupClassRegisterCallback(ic, "SELECT_CB", "s");

  iupBaseRegisterCommonCallbacks(ic);
  iupBaseRegisterCommonAttrib(ic);
  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, qtCalendarSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", qtCalendarGetValueAttrib, qtCalendarSetValueAttrib, NULL, "TODAY", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WEEKNUMBERS", NULL, qtCalendarSetWeekNumbersAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", qtCalendarGetTodayAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  return ic;
}

extern "C" IUP_API Ihandle* IupCalendar(void)
{
  return IupCreate("calendar");
}
