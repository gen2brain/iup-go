/** \file
 * \brief Calendar Control - Qt Quick implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickItem>
#include <QDate>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QStyleHints>

#include <cstdio>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
}

#include "iupqml_drv.h"


static const char* qmlCalendarQml =
  "import QtQuick\nimport QtQuick.Controls\n"
  "Control { id: cal; objectName: \"cal\"; padding: 0; focusPolicy: Qt.StrongFocus; hoverEnabled: true\n"
  "  property int iupYear: 2000; property int iupMonth: 0; property string iupSelected: \"\"; property bool iupWeeks: false\n"
  "  property font iupFont\n"
  "  signal iupKey(int key)\n"
  "  Keys.onPressed: (event) => { if (event.key === Qt.Key_Left || event.key === Qt.Key_Right || event.key === Qt.Key_Up || event.key === Qt.Key_Down || event.key === Qt.Key_PageUp || event.key === Qt.Key_PageDown || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) { cal.iupKey(event.key); event.accepted = true } }\n"
  "  contentItem: Column { spacing: 2\n"
  "    Row { id: nav; objectName: \"nav\"; width: parent.width; height: prevBtn.implicitHeight\n"
  "      ToolButton { id: prevBtn; text: \"\\u2039\"; font: cal.iupFont; focusPolicy: Qt.NoFocus; onClicked: { if (cal.iupMonth == 0) { cal.iupMonth = 11; cal.iupYear-- } else cal.iupMonth-- } }\n"
  "      Label { width: parent.width - prevBtn.width - nextBtn.width; height: prevBtn.height; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font: cal.iupFont\n"
  "        text: Qt.locale().standaloneMonthName(cal.iupMonth) + \" \" + cal.iupYear }\n"
  "      ToolButton { id: nextBtn; text: \"\\u203a\"; font: cal.iupFont; focusPolicy: Qt.NoFocus; onClicked: { if (cal.iupMonth == 11) { cal.iupMonth = 0; cal.iupYear++ } else cal.iupMonth++ } }\n"
  "    }\n"
  "    Row { width: parent.width; height: dow.implicitHeight\n"
  "      Item { width: weeks.width; height: 1 }\n"
  "      DayOfWeekRow { id: dow; objectName: \"dow\"; width: parent.width - weeks.width; font: cal.iupFont }\n"
  "    }\n"
  "    Row { width: parent.width; height: cal.availableHeight - nav.height - dow.height - 4\n"
  "      WeekNumberColumn { id: weeks; objectName: \"weeks\"; visible: cal.iupWeeks; width: cal.iupWeeks ? implicitWidth : 0; height: parent.height; month: grid.month; year: grid.year; font: cal.iupFont }\n"
  "      MonthGrid { id: grid; objectName: \"grid\"; width: parent.width - weeks.width; height: parent.height; month: cal.iupMonth; year: cal.iupYear; font: cal.iupFont\n"
  "        onClicked: if (cal.focusPolicy !== Qt.NoFocus) cal.forceActiveFocus()\n"
  "        delegate: Rectangle { required property var model; radius: 3\n"
  "          color: Qt.formatDate(model.date, \"yyyy-MM-dd\") === cal.iupSelected ? palette.highlight : \"transparent\"\n"
  "          Text { anchors.centerIn: parent; text: model.day; font: cal.iupFont; opacity: model.month === grid.month ? 1 : 0.3\n"
  "            color: Qt.formatDate(model.date, \"yyyy-MM-dd\") === cal.iupSelected ? palette.highlightedText : palette.text } }\n"
  "      }\n"
  "    }\n"
  "  }\n"
  "}";

struct IupQmlCalendarData
{
  QQuickItem* item = nullptr;
  QDate selected;
  QElapsedTimer click_timer;
  QDate last_click;
};

static IupQmlCalendarData* qmlCalendarGetData(Ihandle* ih)
{
  return reinterpret_cast<IupQmlCalendarData*>(iupAttribGet(ih, "_IUPQML_CALENDAR"));
}

static void qmlCalendarApplySelected(Ihandle* ih)
{
  IupQmlCalendarData* data = qmlCalendarGetData(ih);
  if (!data)
    return;

  data->item->setProperty("iupSelected", data->selected.toString("yyyy-MM-dd"));
  data->item->setProperty("iupMonth", data->selected.month() - 1);
  data->item->setProperty("iupYear", data->selected.year());
}

static void qmlCalendarComputeNaturalSizeMethod(Ihandle* ih, int* w, int* h, int* children_expand)
{
  int cw, ch;
  int nav_h, dow_h, weeks_w = 0;
  (void)children_expand;

  iupdrvFontGetMultiLineStringSize(ih, "W8W", &cw, &ch);
  nav_h = ch + 8;
  dow_h = ch + 4;

  QQuickItem* tpl = iupqmlTemplateItem(qmlCalendarQml);
  if (tpl)
  {
    QFont* font = iupqmlGetIhFont(ih);
    if (font)
      tpl->setProperty("iupFont", QVariant::fromValue(*font));

    auto* nav = tpl->findChild<QQuickItem*>("nav");
    auto* dow = tpl->findChild<QQuickItem*>("dow");
    auto* weeks = tpl->findChild<QQuickItem*>("weeks");
    if (nav)
      nav_h = static_cast<int>(nav->height());
    if (dow)
      dow_h = static_cast<int>(dow->implicitHeight());
    if (weeks && iupAttribGetBoolean(ih, "WEEKNUMBERS"))
      weeks_w = static_cast<int>(weeks->implicitWidth());
  }

  *w = (cw + 8) * 7 + weeks_w;
  *h = nav_h + dow_h + 4 + (ch + 8) * 6;
}

static int qmlCalendarSetWeekNumbersAttrib(Ihandle* ih, const char* value)
{
  IupQmlCalendarData* data = qmlCalendarGetData(ih);
  if (data)
    data->item->setProperty("iupWeeks", iupStrBoolean(value) ? true : false);
  return 1;
}

static int qmlCalendarSetValueAttrib(Ihandle* ih, const char* value)
{
  IupQmlCalendarData* data = qmlCalendarGetData(ih);
  if (!data)
    return 0;

  if (iupStrEqualNoCase(value, "TODAY"))
    data->selected = QDate::currentDate();
  else
  {
    int year, month, day;
    if (iupStrToDate(value, &year, &month, &day))
    {
      if (month < 1) month = 1;
      if (month > 12) month = 12;
      if (day < 1) day = 1;
      if (day > 31) day = 31;

      QDate date(year, month, day);
      if (date.isValid())
        data->selected = date;
    }
  }

  qmlCalendarApplySelected(ih);
  return 0;
}

static char* qmlCalendarGetValueAttrib(Ihandle* ih)
{
  IupQmlCalendarData* data = qmlCalendarGetData(ih);
  if (!data)
    return nullptr;

  return iupStrReturnStrf("%d/%d/%d", data->selected.year(), data->selected.month(), data->selected.day());
}

static char* qmlCalendarGetTodayAttrib(Ihandle* ih)
{
  (void)ih;
  QDate today = QDate::currentDate();
  return iupStrReturnStrf("%d/%d/%d", today.year(), today.month(), today.day());
}

static int qmlCalendarSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  IupQmlCalendarData* data = qmlCalendarGetData(ih);
  if (data)
  {
    QFont* font = iupqmlGetIhFont(ih);
    if (font)
      data->item->setProperty("iupFont", QVariant::fromValue(*font));
  }
  return 1;
}

static void qmlCalendarClicked(Ihandle* ih, const QDateTime& datetime)
{
  IupQmlCalendarData* data = qmlCalendarGetData(ih);
  if (!data)
    return;

  QDate date = datetime.date();
  bool same = (date == data->selected);
  bool dbl = data->click_timer.isValid() && data->last_click == date &&
             data->click_timer.elapsed() < QGuiApplication::styleHints()->mouseDoubleClickInterval();

  data->selected = date;
  data->last_click = date;
  data->click_timer.restart();
  qmlCalendarApplySelected(ih);

  if (!same)
    iupBaseCallValueChangedCb(ih);

  if (dbl)
  {
    IFns cb = reinterpret_cast<IFns>(IupGetCallback(ih, "SELECT_CB"));
    if (cb)
    {
      char date_str[64];
      snprintf(date_str, sizeof(date_str), "%d/%d/%d", date.year(), date.month(), date.day());
      cb(ih, date_str);
    }
  }
}

static void qmlCalendarKey(Ihandle* ih, int key)
{
  IupQmlCalendarData* data = qmlCalendarGetData(ih);
  if (!data)
    return;

  QDate date = data->selected;
  switch (key)
  {
  case Qt::Key_Left: date = date.addDays(-1); break;
  case Qt::Key_Right: date = date.addDays(1); break;
  case Qt::Key_Up: date = date.addDays(-7); break;
  case Qt::Key_Down: date = date.addDays(7); break;
  case Qt::Key_PageUp: date = date.addMonths(-1); break;
  case Qt::Key_PageDown: date = date.addMonths(1); break;
  case Qt::Key_Return:
  case Qt::Key_Enter:
    {
      IFns cb = reinterpret_cast<IFns>(IupGetCallback(ih, "SELECT_CB"));
      if (cb)
      {
        char date_str[64];
        snprintf(date_str, sizeof(date_str), "%d/%d/%d", date.year(), date.month(), date.day());
        cb(ih, date_str);
      }
    }
    return;
  default:
    return;
  }

  if (!date.isValid() || date == data->selected)
    return;

  data->selected = date;
  qmlCalendarApplySelected(ih);
  iupBaseCallValueChangedCb(ih);
}

static int qmlCalendarMapMethod(Ihandle* ih)
{
  QQuickItem* item = iupqmlCreateItem(qmlCalendarQml);
  if (!item)
    return IUP_ERROR;

  auto* data = new IupQmlCalendarData();
  data->item = item;
  data->selected = QDate::currentDate();
  iupAttribSet(ih, "_IUPQML_CALENDAR", reinterpret_cast<char*>(data));

  ih->handle = reinterpret_cast<InativeHandle*>(item);

  QFont* font = iupqmlGetIhFont(ih);
  if (font)
    item->setProperty("iupFont", QVariant::fromValue(*font));

  qmlCalendarApplySelected(ih);

  if (iupAttribGetBoolean(ih, "WEEKNUMBERS"))
    item->setProperty("iupWeeks", true);

  auto* grid = item->findChild<QQuickItem*>("grid");
  if (grid)
  {
    iupqmlConnect(grid, "clicked(QDateTime)", [ih](void** args) {
      qmlCalendarClicked(ih, *static_cast<QDateTime*>(args[1]));
    });
  }

  iupqmlConnect(item, "iupKey(int)", [ih](void** args) {
    qmlCalendarKey(ih, *static_cast<int*>(args[1]));
  });

  iupqmlAddToParent(ih);
  iupqmlInstallFilter(ih, item);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqmlSetCanFocus(item, 0);

  return IUP_NOERROR;
}

static void qmlCalendarUnMapMethod(Ihandle* ih)
{
  IupQmlCalendarData* data = qmlCalendarGetData(ih);
  if (data)
  {
    delete data;
    iupAttribSet(ih, "_IUPQML_CALENDAR", nullptr);
  }
  iupqmlTipsDestroy(ih);
  iupdrvBaseUnMapMethod(ih);
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

  ic->Map = qmlCalendarMapMethod;
  ic->UnMap = qmlCalendarUnMapMethod;
  ic->ComputeNaturalSize = qmlCalendarComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");
  iupClassRegisterCallback(ic, "SELECT_CB", "s");

  iupBaseRegisterCommonCallbacks(ic);
  iupBaseRegisterCommonAttrib(ic);
  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "FONT", nullptr, qmlCalendarSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "VALUE", qmlCalendarGetValueAttrib, qmlCalendarSetValueAttrib, nullptr, "TODAY", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WEEKNUMBERS", nullptr, qmlCalendarSetWeekNumbersAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", qmlCalendarGetTodayAttrib, nullptr, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  return ic;
}

extern "C" IUP_API Ihandle* IupCalendar(void)
{
  return IupCreate("calendar");
}
