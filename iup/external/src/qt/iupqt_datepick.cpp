/** \file
 * \brief DatePick Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <QDateEdit>
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
#include "iup_stdcontrols.h"
#include "iup_array.h"
#include "iup_mask.h"
#include "iup_text.h"
}

#include "iupqt_drv.h"


class IupQtDatePick : public QDateEdit
{
public:
  Ihandle* iup_handle;

  IupQtDatePick(Ihandle* ih)
    : QDateEdit(), iup_handle(ih)
  {
    setCalendarPopup(true);
  }

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* event) override
#else
  void enterEvent(QEvent* event) override
#endif
  {
    QDateEdit::enterEvent(event);
    iupqtEnterLeaveEvent(this, event, iup_handle);
  }

  void leaveEvent(QEvent* event) override
  {
    QDateEdit::leaveEvent(event);
    iupqtEnterLeaveEvent(this, event, iup_handle);
  }

  void focusInEvent(QFocusEvent* event) override
  {
    QDateEdit::focusInEvent(event);
    iupqtFocusInOutEvent(this, event, iup_handle);
  }

  void focusOutEvent(QFocusEvent* event) override
  {
    QDateEdit::focusOutEvent(event);
    iupqtFocusInOutEvent(this, event, iup_handle);
  }

  void keyPressEvent(QKeyEvent* event) override
  {
    if (iupqtKeyPressEvent(this, event, iup_handle))
    {
      event->accept();
      return;
    }
    QDateEdit::keyPressEvent(event);
  }

  void mousePressEvent(QMouseEvent* event) override
  {
    QDateEdit::mousePressEvent(event);
    iupqtMouseButtonEvent(this, event, iup_handle);
  }

  void mouseReleaseEvent(QMouseEvent* event) override
  {
    QDateEdit::mouseReleaseEvent(event);
    iupqtMouseButtonEvent(this, event, iup_handle);
  }

  void mouseMoveEvent(QMouseEvent* event) override
  {
    QDateEdit::mouseMoveEvent(event);
    iupqtMouseMoveEvent(this, event, iup_handle);
  }
};

static void qtDatePickUpdateDisplayFormat(Ihandle* ih)
{
  IupQtDatePick* datepick = (IupQtDatePick*)ih->handle;
  if (!datepick)
    return;

  const char* order = iupAttribGetStr(ih, "ORDER");
  const char* separator = iupAttribGetStr(ih, "SEPARATOR");
  int zeropreced = iupAttribGetBoolean(ih, "ZEROPRECED");
  int monthshortnames = iupAttribGetBoolean(ih, "MONTHSHORTNAMES");

  if (!order)
    order = "DMY";
  if (!separator)
    separator = "/";

  QString format;
  for (int i = 0; i < 3 && order[i]; i++)
  {
    if (i > 0)
      format += QString::fromUtf8(separator);

    switch (order[i])
    {
      case 'D':
      case 'd':
        format += zeropreced ? "dd" : "d";
        break;
      case 'M':
      case 'm':
        if (monthshortnames)
          format += "MMM";
        else
          format += zeropreced ? "MM" : "M";
        break;
      case 'Y':
      case 'y':
        format += "yyyy";
        break;
    }
  }

  datepick->setDisplayFormat(format);
}

static void qtDatePickComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  (void)children_expand;

  if (ih->handle)
  {
    IupQtDatePick* datepick = (IupQtDatePick*)ih->handle;
    QSize size = datepick->sizeHint();
    *w = size.width();
    *h = size.height();
  }
  else
  {
    iupdrvFontGetMultiLineStringSize(ih, "WW/WW/WWWW", w, h);
    iupdrvTextAddBorders(ih, w, h);
  }
}

static int qtDatePickSetValueAttrib(Ihandle* ih, const char* value)
{
  IupQtDatePick* datepick = (IupQtDatePick*)ih->handle;

  if (!datepick)
    return 0;

  if (iupStrEqualNoCase(value, "TODAY"))
  {
    datepick->setDate(QDate::currentDate());
  }
  else if (value)
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
        datepick->setDate(date);
    }
  }

  return 0;
}

static char* qtDatePickGetValueAttrib(Ihandle* ih)
{
  IupQtDatePick* datepick = (IupQtDatePick*)ih->handle;

  if (datepick)
  {
    QDate date = datepick->date();
    return iupStrReturnStrf("%d/%d/%d", date.year(), date.month(), date.day());
  }

  return nullptr;
}

static char* qtDatePickGetTodayAttrib(Ihandle* ih)
{
  (void)ih;
  QDate today = QDate::currentDate();
  return iupStrReturnStrf("%d/%d/%d", today.year(), today.month(), today.day());
}

static int qtDatePickSetOrderAttrib(Ihandle* ih, const char* value)
{
  if (!value || strlen(value) != 3)
    return 0;

  for (int i = 0; i < 3; i++)
  {
    char c = value[i];
    if (c != 'D' && c != 'd' && c != 'M' && c != 'm' && c != 'Y' && c != 'y')
      return 0;
  }

  iupAttribSetStr(ih, "ORDER", value);
  qtDatePickUpdateDisplayFormat(ih);
  return 0;
}

static int qtDatePickSetSeparatorAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "SEPARATOR", value);
  qtDatePickUpdateDisplayFormat(ih);
  return 0;
}

static int qtDatePickSetZeroprecedAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "ZEROPRECED", value);
  qtDatePickUpdateDisplayFormat(ih);
  return 0;
}

static int qtDatePickSetMonthshortnamesAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "MONTHSHORTNAMES", value);
  qtDatePickUpdateDisplayFormat(ih);
  return 0;
}

static int qtDatePickSetCalendarweeknumbersAttrib(Ihandle* ih, const char* value)
{
  IupQtDatePick* datepick = (IupQtDatePick*)ih->handle;

  if (datepick)
  {
    QCalendarWidget* calendar = datepick->calendarWidget();
    if (calendar)
    {
      if (iupStrBoolean(value))
        calendar->setVerticalHeaderFormat(QCalendarWidget::ISOWeekNumbers);
      else
        calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    }
  }

  return 1;
}

static int qtDatePickSetFormatAttrib(Ihandle* ih, const char* value)
{
  IupQtDatePick* datepick = (IupQtDatePick*)ih->handle;

  if (datepick && value)
  {
    datepick->setDisplayFormat(QString::fromUtf8(value));
  }

  return 1;
}

static int qtDatePickMapMethod(Ihandle* ih)
{
  IupQtDatePick* datepick = new IupQtDatePick(ih);

  if (!datepick)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)datepick;

  datepick->setDate(QDate::currentDate());

  iupqtAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqtSetCanFocus(datepick, 0);

  QObject::connect(datepick, &QDateEdit::dateChanged, [ih](const QDate& date) {
    (void)date;
    iupBaseCallValueChangedCb(ih);
  });

  qtDatePickUpdateDisplayFormat(ih);

  if (iupAttribGetBoolean(ih, "CALENDARWEEKNUMBERS"))
    qtDatePickSetCalendarweeknumbersAttrib(ih, "YES");

  return IUP_NOERROR;
}

static void qtDatePickUnMapMethod(Ihandle* ih)
{
  if (ih->handle)
  {
    IupQtDatePick* datepick = (IupQtDatePick*)ih->handle;

    iupqtTipsDestroy(ih);

    delete datepick;
    ih->handle = nullptr;
  }
}

extern "C" Iclass* iupDatePickNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = (char*)"datepick";
  ic->cons = (char*)"DatePick";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  ic->New = iupDatePickNewClass;

  ic->Map = qtDatePickMapMethod;
  ic->UnMap = qtDatePickUnMapMethod;
  ic->ComputeNaturalSize = qtDatePickComputeNaturalSizeMethod;

  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  iupBaseRegisterCommonCallbacks(ic);
  iupBaseRegisterCommonAttrib(ic);
  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "VALUE", qtDatePickGetValueAttrib, qtDatePickSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", qtDatePickGetTodayAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SEPARATOR", NULL, qtDatePickSetSeparatorAttrib, IUPAF_SAMEASSYSTEM, "/", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZEROPRECED", NULL, qtDatePickSetZeroprecedAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORDER", NULL, qtDatePickSetOrderAttrib, IUPAF_SAMEASSYSTEM, "DMY", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CALENDARWEEKNUMBERS", NULL, qtDatePickSetCalendarweeknumbersAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MONTHSHORTNAMES", NULL, qtDatePickSetMonthshortnamesAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", NULL, qtDatePickSetFormatAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* Not supported: SHOWDROPDOWN - Qt has no API to programmatically open/close the calendar popup */
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  return ic;
}

extern "C" IUP_API Ihandle* IupDatePick(void)
{
  return IupCreate("datepick");
}
