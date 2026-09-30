/** \file
 * \brief Qt Quick Driver TIPS (tooltips) management
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickItem>
#include <QQuickWindow>
#include <QEvent>
#include <QHoverEvent>
#include <QTimer>
#include <QCursor>
#include <QString>
#include <QColor>
#include <QFont>

#include <cstdio>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_str.h"
#include "iup_attrib.h"
}

#include "iupqml_drv.h"


static QObject* qmlTipGetPopup(Ihandle* ih, int create)
{
  auto* popup = reinterpret_cast<QObject*>(iupAttribGet(ih, "_IUPQML_TIP"));
  if (popup || !create)
    return popup;

  popup = iupqmlCreateObject(IUPQML_IMPORTS "ToolTip { timeout: -1 }");
  if (!popup)
    return nullptr;

  auto* owner = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_EVENT_ITEM"));
  if (!owner)
    owner = iupqmlGetItem(ih);
  if (owner)
    popup->setParent(owner);

  iupAttribSet(ih, "_IUPQML_TIP", reinterpret_cast<char*>(popup));
  return popup;
}

static void qmlTipUpdateStyle(Ihandle* ih, QObject* popup)
{
  const char* value;
  unsigned char r, g, b;

  value = iupAttribGet(ih, "TIPFONT");
  if (value && !iupStrEqualNoCase(value, "SYSTEM"))
  {
    QFont* qfont = iupqmlGetQFont(value);
    if (qfont)
      popup->setProperty("font", QVariant::fromValue(*qfont));
  }

  value = iupAttribGet(ih, "TIPBGCOLOR");
  if (value && iupStrToRGB(value, &r, &g, &b))
    iupqmlSetPaletteColor(popup, "toolTipBase", QColor(r, g, b));

  value = iupAttribGet(ih, "TIPFGCOLOR");
  if (value && iupStrToRGB(value, &r, &g, &b))
    iupqmlSetPaletteColor(popup, "toolTipText", QColor(r, g, b));
}

static void qmlTipShow(Ihandle* ih, QQuickItem* item, const QPointF& local)
{
  const char* tip = iupAttribGet(ih, "TIP");
  if (!tip)
    return;

  const char* tiprect = iupAttribGet(ih, "TIPRECT");
  if (tiprect)
  {
    int x1, y1, x2, y2;
    if (iupStrToRect(tiprect, &x1, &y1, &x2, &y2))
    {
      if (local.x() < x1 || local.x() > x2 || local.y() < y1 || local.y() > y2)
        return;
    }
  }

  auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "TIPS_CB"));
  if (cb)
    cb(ih, static_cast<int>(local.x()), static_cast<int>(local.y()));

  QObject* popup = qmlTipGetPopup(ih, 1);
  if (!popup)
    return;

  qmlTipUpdateStyle(ih, popup);
  popup->setProperty("parent", QVariant::fromValue(item));
  popup->setProperty("text", QString::fromUtf8(tip));
  popup->setProperty("x", local.x());
  popup->setProperty("y", local.y() + 20);
  popup->setProperty("visible", true);
}

static void qmlTipHide(Ihandle* ih)
{
  QObject* popup = qmlTipGetPopup(ih, 0);
  if (popup)
    popup->setProperty("visible", false);
}

class IupQmlTipFilter : public QObject
{
public:
  Ihandle* ih;
  QQuickItem* item;
  QTimer timer;
  QPointF pos;

  IupQmlTipFilter(QQuickItem* parent, Ihandle* handle) : QObject(parent), ih(handle), item(parent)
  {
    timer.setSingleShot(true);
    timer.setInterval(700);
    QObject::connect(&timer, &QTimer::timeout, [this]() {
      if (iupObjectCheck(ih))
        qmlTipShow(ih, item, pos);
    });
  }

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    (void)obj;
    if (!iupObjectCheck(ih))
      return false;

    switch (event->type())
    {
    case QEvent::HoverEnter:
      pos = static_cast<QHoverEvent*>(event)->position();
      timer.start();
      break;
    case QEvent::HoverMove:
      pos = static_cast<QHoverEvent*>(event)->position();
      if (!timer.isActive() && !(qmlTipGetPopup(ih, 0) && qmlTipGetPopup(ih, 0)->property("visible").toBool()))
        timer.start();
      break;
    case QEvent::HoverLeave:
    case QEvent::MouseButtonPress:
    case QEvent::KeyPress:
      timer.stop();
      qmlTipHide(ih);
      break;
    default:
      break;
    }
    return false;
  }
};

static QQuickItem* qmlTipGetItem(Ihandle* ih)
{
  QQuickItem* item = iupqmlCanvasGetItem(ih);
  if (!item)
    item = iupqmlGetItem(ih);
  return item;
}

extern "C" IUP_SDK_API int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  QQuickItem* item = qmlTipGetItem(ih);
  if (!item)
    return 0;

  if (!iupAttribGet(ih, "_IUPQML_TIPFILTER"))
  {
    auto* filter = new IupQmlTipFilter(item, ih);
    item->setAcceptHoverEvents(true);
    item->installEventFilter(filter);
    iupAttribSet(ih, "_IUPQML_TIPFILTER", reinterpret_cast<char*>(filter));
  }

  if (!value || !value[0])
    qmlTipHide(ih);
  else
  {
    QObject* popup = qmlTipGetPopup(ih, 0);
    if (popup && popup->property("visible").toBool())
      popup->setProperty("text", QString::fromUtf8(value));
  }

  return 1;
}

extern "C" IUP_SDK_API int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  QQuickItem* item = qmlTipGetItem(ih);
  if (!item)
    return 0;

  if (iupStrBoolean(value))
    qmlTipShow(ih, item, item->mapFromGlobal(QPointF(QCursor::pos())));
  else
    qmlTipHide(ih);

  return 0;
}

extern "C" IUP_SDK_API char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  QObject* popup = qmlTipGetPopup(ih, 0);
  return iupStrReturnBoolean(popup && popup->property("visible").toBool());
}

IUP_DRV_API void iupqmlTipsDestroy(Ihandle* ih)
{
  if (!ih)
    return;

  auto* popup = reinterpret_cast<QObject*>(iupAttribGet(ih, "_IUPQML_TIP"));
  if (popup)
  {
    popup->setProperty("visible", false);
    popup->deleteLater();
    iupAttribSet(ih, "_IUPQML_TIP", nullptr);
  }
  iupAttribSet(ih, "_IUPQML_TIPFILTER", nullptr);
}
