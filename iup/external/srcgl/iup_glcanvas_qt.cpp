/** \file
 * \brief Qt helper functions for EGL GLCanvas
 *
 * See Copyright Notice in "iup.h"
 */

#include <QWindow>
#include <QWidget>
#include <QGuiApplication>
#include <QString>
#include <QByteArray>
#include <QThread>
#include <QEventLoop>


#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0) && QT_CONFIG(wayland)
  #include <QtGui/qpa/qplatformnativeinterface.h>
  #include <QtGui/qpa/qplatformwindow_p.h>

  /* Forward declare wl_surface from Wayland client API */
  struct wl_surface;

  /* Forward declare the native interface */
  QT_BEGIN_NAMESPACE
  namespace QNativeInterface {
    struct Q_GUI_EXPORT QWaylandWindow {
      QT_DECLARE_NATIVE_INTERFACE(QWaylandWindow, 1, QWindow)
      virtual struct wl_surface *surface() const = 0;
    };
  }
  QT_END_NAMESPACE
#endif

#include <stdio.h>
#include <string.h>

extern "C" {

#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_drv.h"

const char* iupQtGetPlatformName(void)
{
  static QByteArray platform_bytes;
  QString platform = QGuiApplication::platformName();
  platform_bytes = platform.toUtf8();
  return platform_bytes.constData();
}

int iupQtHasWaylandSupport(void)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0) && QT_CONFIG(wayland)
  return 1;
#else
  return 0;
#endif
}

unsigned long iupQtGetNativeWindow(void* qwindow_ptr)
{
  QWindow* qwindow = static_cast<QWindow*>(qwindow_ptr);
  if (!qwindow)
    return 0;

  /* Use winId() which works for X11 and XWayland */
  return qwindow->winId();
}

void* iupQtGetWindowFromWidget(void* qwidget_or_qwindow_ptr)
{
  if (!qwidget_or_qwindow_ptr)
    return nullptr;

  QWindow* window = nullptr;

  QObject* obj = static_cast<QObject*>(qwidget_or_qwindow_ptr);

  if (obj && obj->inherits("QWindow"))
  {
    window = static_cast<QWindow*>(qwidget_or_qwindow_ptr);
  }
  else if (obj && obj->inherits("QWidget"))
  {
    QWidget* qwidget = static_cast<QWidget*>(qwidget_or_qwindow_ptr);

    window = qwidget->windowHandle();
    if (!window)
    {
      QWidget* topLevel = qwidget->window();
      if (topLevel)
        window = topLevel->windowHandle();
    }
  }

  return window;
}

double iupQtGetDevicePixelRatio(void* qwindow_ptr)
{
  QWindow* qwindow = static_cast<QWindow*>(qwindow_ptr);
  if (!qwindow)
    return 1.0;

  return qwindow->devicePixelRatio();
}

void iupQtGetWidgetSize(void* qwidget_ptr, int* width, int* height)
{
  QWidget* qwidget = static_cast<QWidget*>(qwidget_ptr);
  if (!qwidget)
  {
    *width = 0;
    *height = 0;
    return;
  }

  *width = qwidget->width();
  *height = qwidget->height();
}

/* Get the top-level window's QWindow from a widget */
void* iupQtGetTopLevelWindow(void* qwidget_ptr)
{
  QWidget* qwidget = static_cast<QWidget*>(qwidget_ptr);
  if (!qwidget)
  {
    return nullptr;
  }

  QWidget* topLevel = qwidget->window();
  if (!topLevel)
  {
    return nullptr;
  }

  QWindow* windowHandle = topLevel->windowHandle();

  return windowHandle;
}

/* Qt6 Wayland Support (Qt 6.7+) */
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0) && QT_CONFIG(wayland)

/* Wayland client API */
#include <wayland-client.h>

struct wl_display* iupQtGetWaylandDisplay(void)
{
  using namespace QNativeInterface;
  if (auto* waylandApp = qApp->nativeInterface<QWaylandApplication>())
  {
    wl_display* display = waylandApp->display();
    return display;
  }

  return nullptr;
}

struct wl_compositor* iupQtGetWaylandCompositor(void)
{
  using namespace QNativeInterface;
  if (auto* waylandApp = qApp->nativeInterface<QWaylandApplication>())
  {
    wl_compositor* compositor = waylandApp->compositor();
    return compositor;
  }

  return nullptr;
}

void* iupQtGetWaylandSurface(void* qwindow_ptr)
{
  QWindow* qwindow = static_cast<QWindow*>(qwindow_ptr);
  if (!qwindow)
  {
    return nullptr;
  }

  /* Check if we're actually on Wayland */
  const char* platform = iupQtGetPlatformName();
  if (!platform || strcmp(platform, "wayland") != 0)
  {
    return nullptr;
  }

  struct wl_surface* surf = nullptr;

  //using namespace QNativeInterface;
  //auto waylandWindow = qwindow->nativeInterface<QWaylandWindow>();
  using namespace QNativeInterface::Private;
  auto waylandWindow = qwindow->nativeInterface<QNativeInterface::Private::QWaylandWindow>();

  if (waylandWindow)
  {
    surf = waylandWindow->surface();
  }

  return (void*)surf;
}

/* Get widget position for subsurface placement */
void iupQtGetWidgetPosition(void* qwidget_ptr, int* x, int* y)
{
  QWidget* qwidget = static_cast<QWidget*>(qwidget_ptr);
  if (!qwidget || !x || !y)
  {
    if (x) *x = 0;
    if (y) *y = 0;
    return;
  }

  /* Get position relative to top-level window */
  QPoint pos = qwidget->mapTo(qwidget->window(), QPoint(0, 0));
  *x = pos.x();
  *y = pos.y();
}

/* Get frame margins (for CSD shadow borders) from QWindow */
void iupQtGetWindowFrameMargins(void* qwindow_ptr, int* left, int* top, int* right, int* bottom)
{
  QWindow* qwindow = static_cast<QWindow*>(qwindow_ptr);
  if (!qwindow || !left || !top || !right || !bottom)
  {
    if (left) *left = 0;
    if (top) *top = 0;
    if (right) *right = 0;
    if (bottom) *bottom = 0;
    return;
  }

  /* Get frame margins - these include CSD shadows on Wayland with Mutter */
  QMargins margins = qwindow->frameMargins();
  *left = margins.left();
  *top = margins.top();
  *right = margins.right();
  *bottom = margins.bottom();
}

#else /* QT_VERSION < 6.7.0 */

/* Stub implementations for Qt5 and Qt < 6.7  */
struct wl_display* iupQtGetWaylandDisplay(void)
{
  return nullptr;
}

struct wl_compositor* iupQtGetWaylandCompositor(void)
{
  return nullptr;
}

void* iupQtGetWaylandSurface(void* qwindow_ptr)
{
  (void)qwindow_ptr;
  return nullptr;
}

void iupQtGetWidgetPosition(void* qwidget_ptr, int* x, int* y)
{
  (void)qwidget_ptr;
  if (x) *x = 0;
  if (y) *y = 0;
}

void iupQtGetWindowFrameMargins(void* qwindow_ptr, int* left, int* top, int* right, int* bottom)
{
  (void)qwindow_ptr;
  if (left) *left = 0;
  if (top) *top = 0;
  if (right) *right = 0;
  if (bottom) *bottom = 0;
}

#endif /* QT_VERSION >= QT_VERSION_CHECK(6, 7, 0) && QT_CONFIG(wayland) */

} /* extern "C" */
