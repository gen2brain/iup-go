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

void* iupQtGetNativeDisplay(void)
{
  /* Let EGL open its own display connection */
  /* EGL_DEFAULT_DISPLAY handles display connection automatically */
  return nullptr;
}

unsigned long iupQtGetNativeWindow(void* qwindow_ptr)
{
  QWindow* qwindow = static_cast<QWindow*>(qwindow_ptr);
  if (!qwindow)
    return 0;

  /* Use winId() which works for X11 and XWayland */
  return qwindow->winId();
}

void* iupQtGetWindowFromWidget(void* qwidget_ptr)
{
  QWidget* qwidget = static_cast<QWidget*>(qwidget_ptr);
  if (!qwidget)
    return nullptr;

  /* Disable Qt's rendering BEFORE creating native window */
  qwidget->setAutoFillBackground(false);
  qwidget->setAttribute(Qt::WA_OpaquePaintEvent, true);
  qwidget->setAttribute(Qt::WA_PaintOnScreen, true);
  qwidget->setAttribute(Qt::WA_NoSystemBackground, true);

  /* Create the native window */
  qwidget->setAttribute(Qt::WA_NativeWindow, true);

  QWindow* window = qwidget->windowHandle();
  if (!window)
  {
    /* Try top-level window */
    QWidget* topLevel = qwidget->window();
    if (topLevel)
      window = topLevel->windowHandle();
  }

  if (window)
  {
    /* Set OpenGLSurface - works for X11 and XWayland */
    window->setSurfaceType(QSurface::OpenGLSurface);
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

void iupQtGetWindowSize(void* qwindow_ptr, int* width, int* height)
{
  QWindow* qwindow = static_cast<QWindow*>(qwindow_ptr);
  if (!qwindow)
  {
    *width = 0;
    *height = 0;
    return;
  }

  *width = qwindow->width();
  *height = qwindow->height();
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

void iupQtInvokeActionCallback(void* ih_ptr)
{
  Ihandle* ih = (Ihandle*)ih_ptr;
  if (!ih)
    return;

  IFnff cb = (IFnff)IupGetCallback(ih, "ACTION");
  if (cb)
    cb(ih, 0.0f, 0.0f);
}

void iupQtForceWidgetUpdate(void* qwidget_ptr)
{
  QWidget* qwidget = static_cast<QWidget*>(qwidget_ptr);
  if (!qwidget)
    return;

  qwidget->update();
}

} /* extern "C" */
