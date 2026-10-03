/** \file
 * \brief Web Browser Control - Qt Widgets host of the native web engine
 *
 * See Copyright Notice in "iup.h"
 */

#include <QWidget>
#include <QEvent>
#include <QtMath>
#include <QWindow>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_classbase.h"
}

#include "iupqt_drv.h"
#include "iupweb_host.h"


class IupQtWebHost : public QWidget
{
public:
  Ihandle* ih;

  explicit IupQtWebHost(Ihandle* handle) : ih(handle)
  {
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setFocusPolicy(Qt::StrongFocus);
  }

  void place()
  {
    if (!ih)
      return;

#ifdef Q_OS_WIN
    qreal scale = devicePixelRatioF();
    int w = qCeil(width() * scale), h = qCeil(height() * scale);
#else
    int w = width(), h = height();
#endif
    iupwebHostSetBounds(ih, 0, 0, w, h, 0, 0, w, h);
  }

protected:
  bool event(QEvent* evt) override
  {
    bool ret = QWidget::event(evt);

    switch (evt->type())
    {
      case QEvent::Resize:
      case QEvent::Show:
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
      case QEvent::DevicePixelRatioChange:
#endif
        place();
        break;
      default:
        break;
    }

    return ret;
  }
};

extern "C" void* iupwebHostMap(Ihandle* ih)
{
  auto* host = new IupQtWebHost(ih);
  ih->handle = reinterpret_cast<InativeHandle*>(host);

  iupqtAddToParent(ih);

  return reinterpret_cast<void*>(host->winId());
}

extern "C" void iupwebHostLayoutUpdate(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);

  auto* host = reinterpret_cast<QWidget*>(ih->handle);
  QWindow* window = host ? host->windowHandle() : nullptr;
  if (window && !host->isVisible() && host->nativeParentWidget())
    window->setGeometry(QRect(host->mapTo(host->nativeParentWidget(), QPoint(0, 0)), host->size()));
}

extern "C" void iupwebHostUnMap(Ihandle* ih)
{
  auto* host = static_cast<IupQtWebHost*>(reinterpret_cast<QWidget*>(ih->handle));
  if (host)
    host->ih = nullptr;

  iupdrvBaseUnMapMethod(ih);
}
