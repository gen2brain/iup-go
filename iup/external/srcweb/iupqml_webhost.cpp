/** \file
 * \brief Web Browser Control - Qt Quick host of the native web engine
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickItem>
#include <QQuickWindow>
#include <QPointer>
#include <QRect>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_classbase.h"
}

#include "iupqml_drv.h"
#include "iupweb_host.h"


class IupQmlWebHost : public QObject
{
public:
  Ihandle* ih;
  QPointer<QQuickItem> item;
  QPointer<QQuickWindow> window;
  QRect bounds;
  int visible = -1;

  IupQmlWebHost(Ihandle* handle, QQuickItem* host_item) : QObject(host_item), ih(handle), item(host_item), window(host_item->window())
  {
    if (window)
      connect(window, &QQuickWindow::afterAnimating, this, &IupQmlWebHost::place, Qt::DirectConnection);
    connect(item, &QQuickItem::visibleChanged, this, &IupQmlWebHost::place);
    connect(item, &QQuickItem::windowChanged, this, &IupQmlWebHost::place);
  }

  void place()
  {
    if (!ih || !item)
      return;

    int show = window && item->window() == window && item->isVisible() && item->width() > 0 && item->height() > 0;
    QRectF scene = item->mapRectToScene(QRectF(0, 0, item->width(), item->height()));

#ifdef Q_OS_WIN
    qreal scale = window ? window->devicePixelRatio() : 1.0;
    int x = qRound(scene.left() * scale);
    int y = qRound(scene.top() * scale);
    QRect rect(x, y, qRound(scene.right() * scale) - x, qRound(scene.bottom() * scale) - y);
#else
    QRect rect = scene.toAlignedRect();
#endif

    if (rect == bounds && show == visible)
      return;

    bounds = rect;
    visible = show;
    iupwebHostSetBounds(ih, rect.x(), rect.y(), rect.width(), rect.height(), show);
  }
};

extern "C" void* iupwebHostMap(Ihandle* ih)
{
  QQuickItem* item = iupqmlCreateItem("import QtQuick\nItem { }");
  if (!item)
    return nullptr;

  ih->handle = reinterpret_cast<InativeHandle*>(item);
  iupqmlAddToParent(ih);

  QQuickWindow* window = item->window();
  if (!window)
    return nullptr;

  auto* host = new IupQmlWebHost(ih, item);
  iupAttribSet(ih, "_IUPQML_WEBHOST", reinterpret_cast<char*>(host));

  return reinterpret_cast<void*>(window->winId());
}

extern "C" void iupwebHostLayoutUpdate(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
}

extern "C" void iupwebHostUnMap(Ihandle* ih)
{
  auto* host = reinterpret_cast<IupQmlWebHost*>(iupAttribGet(ih, "_IUPQML_WEBHOST"));
  iupAttribSet(ih, "_IUPQML_WEBHOST", nullptr);
  delete host;

  iupdrvBaseUnMapMethod(ih);
}
