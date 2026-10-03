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
  QRect clip;

  IupQmlWebHost(Ihandle* handle, QQuickItem* host_item) : QObject(host_item), ih(handle), item(host_item), window(host_item->window())
  {
    if (window)
      connect(window, &QQuickWindow::afterAnimating, this, &IupQmlWebHost::place, Qt::DirectConnection);
    connect(item, &QQuickItem::visibleChanged, this, &IupQmlWebHost::place);
    connect(item, &QQuickItem::windowChanged, this, &IupQmlWebHost::place);
  }

  QRect toNative(const QRectF& r) const
  {
#ifdef Q_OS_WIN
    qreal scale = window ? window->devicePixelRatio() : 1.0;
    int x = qRound(r.left() * scale);
    int y = qRound(r.top() * scale);
    return QRect(x, y, qRound(r.right() * scale) - x, qRound(r.bottom() * scale) - y);
#else
    return r.toAlignedRect();
#endif
  }

  void place()
  {
    if (!ih || !item)
      return;

    QRectF scene = item->mapRectToScene(QRectF(0, 0, item->width(), item->height()));
    QRectF visible;

    if (window && item->window() == window && item->isVisible())
    {
      visible = scene & QRectF(0, 0, window->width(), window->height());
      for (QQuickItem* p = item->parentItem(); p; p = p->parentItem())
      {
        if (p->clip())
          visible &= p->mapRectToScene(QRectF(0, 0, p->width(), p->height()));
      }
    }

    QRect rect = toNative(scene);
    QRect area = visible.isEmpty() ? QRect() : toNative(visible);

    if (rect == bounds && area == clip)
      return;

    bounds = rect;
    clip = area;
    iupwebHostSetBounds(ih, rect.x(), rect.y(), rect.width(), rect.height(), area.x(), area.y(), area.width(), area.height());
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
  {
    iupdrvBaseUnMapMethod(ih);
    return nullptr;
  }

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
