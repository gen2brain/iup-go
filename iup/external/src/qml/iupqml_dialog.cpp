/** \file
 * \brief IupDialog - Qt Quick Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickWindow>
#include <QQuickItem>
#include <QGuiApplication>
#include <QTimer>
#include <QScreen>
#include <QIcon>
#include <QPixmap>
#include <QBitmap>
#include <QRegion>
#include <QCloseEvent>
#include <QSurfaceFormat>

#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_class.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_globalattrib.h"
#include "iup_str.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"
#include "iup_image.h"
}

#include "iupqml_drv.h"

#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS) && !defined(Q_OS_HAIKU)
  #include <QtGui/qguiapplication_platform.h>
  #define IUP_QML_HAS_X11_APP 1
#endif

#ifdef IUPX11_USE_DLOPEN
extern "C" {
#include "iupunix_x11.h"
}
#elif !defined(_WIN32) && !defined(__APPLE__) && !defined(__HAIKU__)
#include <X11/Xlib.h>
#endif


static int qmlDialogGetMenuSize(Ihandle* ih)
{
  if (ih->data->menu)
    return iupdrvMenuGetMenuBarSize(ih->data->menu);
  else
    return 0;
}

/****************************************************************************
 * Dialog Window
 ****************************************************************************/

class IupQmlDialog : public QQuickWindow
{
public:
  Ihandle* ih;
  QQuickItem* content;

  IupQmlDialog(Ihandle* handle) : QQuickWindow(), ih(handle), content(nullptr)
  {
    content = iupqmlCreateItem("import QtQuick\nItem { focus: true }");
    content->setParentItem(contentItem());
    content->setPosition(QPointF(0, 0));

    updateWindowFlags();

    QObject::connect(this, &QWindow::windowStateChanged, [this](Qt::WindowState state) {
      if (!ih)
        return;

      int iup_state;
      if (state & Qt::WindowMinimized)
        iup_state = IUP_MINIMIZE;
      else if (state & Qt::WindowMaximized)
        iup_state = IUP_MAXIMIZE;
      else if (state & Qt::WindowFullScreen)
        iup_state = IUP_MAXIMIZE;
      else
        iup_state = IUP_RESTORE;

      if (ih->data->show_state != iup_state)
      {
        IFni cb = reinterpret_cast<IFni>(IupGetCallback(ih, "SHOW_CB"));
        ih->data->show_state = iup_state;
        if (cb && cb(ih, iup_state) == IUP_CLOSE)
          IupExitLoop();
      }
    });

    QObject::connect(this, &QWindow::activeChanged, [this]() {
      if (ih && isActive() && QGuiApplication::focusWindow() == this)
        iupqmlDialogSetFocus(ih);
    });
  }

  void handleResize()
  {
    if (!ih)
      return;

    layoutContent();

    if (ih->data->ignore_resize || !isVisible())
      return;

    int border = 0, caption = 0, menu = 0;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    ih->currentwidth = width() + 2*border;
    ih->currentheight = height() + 2*border + caption;

    auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "RESIZE_CB"));
    if (!cb || cb(ih, width(), height() - menu) != IUP_IGNORE)
    {
      ih->data->ignore_resize = 1;
      IupRefresh(ih);
      ih->data->ignore_resize = 0;
    }
  }

  void updateWindowFlags()
  {
    if (!ih)
      return;

    Qt::WindowFlags flags = iupDialogGetNativeParent(ih) ? Qt::Dialog : Qt::Window;

    int has_titlebar = iupAttribGet(ih, "TITLE") ||
                       iupAttribGetBoolean(ih, "RESIZE") ||
                       iupAttribGetBoolean(ih, "MAXBOX") ||
                       iupAttribGetBoolean(ih, "MINBOX") ||
                       iupAttribGetBoolean(ih, "MENUBOX");

    if (!has_titlebar && !iupAttribGetBoolean(ih, "BORDER"))
      flags |= Qt::FramelessWindowHint;

    if (has_titlebar)
      flags |= Qt::WindowTitleHint | Qt::CustomizeWindowHint;

    if (!iupAttribGetBoolean(ih, "MENUBOX"))
      flags |= Qt::WindowSystemMenuHint;
    else
      flags |= Qt::WindowCloseButtonHint;

    if (iupAttribGetBoolean(ih, "MINBOX"))
      flags |= Qt::WindowMinimizeButtonHint;

    if (iupAttribGetBoolean(ih, "MAXBOX"))
      flags |= Qt::WindowMaximizeButtonHint;

    if (iupAttribGetBoolean(ih, "DIALOGHINT"))
      flags |= Qt::Dialog;

    if (iupAttribGetBoolean(ih, "TOOLBOX"))
      flags |= Qt::Tool;

    if (iupAttribGetBoolean(ih, "HIDETITLEBAR") || iupAttribGetBoolean(ih, "CUSTOMFRAME"))
      flags |= Qt::FramelessWindowHint;

    if (iupAttribGetBoolean(ih, "TOPMOST"))
      flags |= Qt::WindowStaysOnTopHint;

    setFlags(flags);
  }

  void layoutContent()
  {
    if (!ih)
      return;

    int menu = qmlDialogGetMenuSize(ih);
    int w = width(), h = height();

    if (ih->data->menu && ih->data->menu->handle && !iupqmlMenuBarIsNative())
    {
      auto* menubar = reinterpret_cast<QQuickItem*>(ih->data->menu->handle);
      menubar->setPosition(QPointF(0, 0));
      menubar->setSize(QSizeF(w, menu));
    }

    content->setPosition(QPointF(0, menu));
    content->setSize(QSizeF(w, h - menu > 0 ? h - menu : 0));

    auto* background = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_BACKIMAGE"));
    if (background)
      background->setSize(QSizeF(w, h));
  }

protected:
  void closeEvent(QCloseEvent* event) override
  {
    if (ih && iupqmlDialogCloseEvent(ih))
      event->ignore();
    else
      event->accept();
  }

  void resizeEvent(QResizeEvent* event) override
  {
    QQuickWindow::resizeEvent(event);
    handleResize();
  }

  void moveEvent(QMoveEvent* event) override
  {
    QQuickWindow::moveEvent(event);

    if (!ih || QGuiApplication::platformName() == "wayland")
      return;

    QPoint pos = framePosition();

    auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "MOVE_CB"));
    if (cb)
      cb(ih, pos.x(), pos.y());

    iupAttribSetInt(ih, "_IUPQML_OLD_X", pos.x());
    iupAttribSetInt(ih, "_IUPQML_OLD_Y", pos.y());
  }

  bool event(QEvent* event) override
  {
    if (event->type() == QEvent::MouseButtonPress && ih)
      iupAttribSet(ih, "_IUPQML_KEYBOARD_FOCUS", nullptr);

    if (event->type() == QEvent::ThemeChange || event->type() == QEvent::ApplicationPaletteChange)
    {
      if (ih && iupqmlSystemPaletteChanged())
      {
        iupqmlUpdateSystemPalette();
        iupqmlSetGlobalColors();

        iupGlobalNotifyThemeChanged();
        iupGlobalUpdateThemeColors();
      }
    }

    return QQuickWindow::event(event);
  }
};

IUP_DRV_API QQuickWindow* iupqmlDialogGetWindow(Ihandle* ih)
{
  if (!ih || !ih->handle || ih->iclass->nativetype != IUP_TYPEDIALOG)
    return nullptr;
  return reinterpret_cast<QQuickWindow*>(ih->handle);
}

IUP_DRV_API QQuickItem* iupqmlDialogGetContent(Ihandle* ih)
{
  auto* dialog = static_cast<IupQmlDialog*>(iupqmlDialogGetWindow(ih));
  if (!dialog)
    return nullptr;
  return dialog->content;
}

/****************************************************************************
 * Dialog Event Handlers
 ****************************************************************************/

IUP_DRV_API int iupqmlDialogCloseEvent(Ihandle* ih)
{
  Icallback cb;

  if (!iupdrvIsActive(ih))
    return 1;

  cb = IupGetCallback(ih, "CLOSE_CB");
  if (cb)
  {
    int ret = cb(ih);
    if (ret == IUP_IGNORE)
      return 1;
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  IupHide(ih);

  return 0;
}

/****************************************************************************
 * Dialog Utilities
 ****************************************************************************/

static void qmlDialogDisconnectParent(Ihandle* ih)
{
  auto* conn = reinterpret_cast<QMetaObject::Connection*>(iupAttribGet(ih, "_IUPQML_PARENT_DESTROYED"));
  if (conn)
  {
    QObject::disconnect(*conn);
    delete conn;
    iupAttribSet(ih, "_IUPQML_PARENT_DESTROYED", nullptr);
  }
}

extern "C" IUP_SDK_API void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* parent)
{
  auto* dialog = reinterpret_cast<IupQmlDialog*>(ih->handle);
  if (!dialog)
    return;

  dialog->updateWindowFlags();
  qmlDialogDisconnectParent(ih);

  if (parent)
  {
    auto* parent_window = reinterpret_cast<QWindow*>(parent);

    dialog->setTransientParent(parent_window);

    QMetaObject::Connection conn = QObject::connect(parent_window, &QObject::destroyed, [ih]() {
      qmlDialogDisconnectParent(ih);
      iupAttribSet(ih, "PARENTDIALOG", nullptr);
      iupAttribSet(ih, "NATIVEPARENT", nullptr);
      QTimer::singleShot(0, [ih]() {
        if (iupObjectCheck(ih))
          IupDestroy(ih);
      });
    });
    iupAttribSet(ih, "_IUPQML_PARENT_DESTROYED", reinterpret_cast<char*>(new QMetaObject::Connection(conn)));
  }
}

extern "C" IUP_SDK_API int iupdrvDialogIsVisible(Ihandle* ih)
{
  return iupdrvIsVisible(ih);
}

extern "C" IUP_SDK_API void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int* w, int* h)
{
  QWindow* window;
  int border = 0, caption = 0, menu = 0;

  if (!handle)
    handle = ih->handle;

  window = reinterpret_cast<QWindow*>(handle);

  if (window)
  {
    QSize size = window->size();

    if (ih)
      iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    if (w) *w = size.width() + 2*border;
    if (h) *h = size.height() + 2*border + caption;
  }
}

static void qmlDialogSetResizeInc(Ihandle* ih, const char* value, int min_w, int min_h)
{
  auto* window = reinterpret_cast<QWindow*>(ih->handle);
  int inc_w = 0, inc_h = 0;

  if (!window)
    return;

  if (!iupStrToIntInt(value, &inc_w, &inc_h, 'x') || (inc_w <= 1 && inc_h <= 1))
  {
    window->setSizeIncrement(QSize(0, 0));
    return;
  }

  int border = 0, caption = 0, menu = 0;
  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  int decorwidth = 2*border;
  int decorheight = 2*border + caption;

  window->setBaseSize(QSize(min_w > decorwidth? min_w - decorwidth: 0,
                            min_h > decorheight? min_h - decorheight: 0));
  window->setSizeIncrement(QSize(inc_w > 1? inc_w: 1, inc_h > 1? inc_h: 1));
}

static void qmlDialogSetTaskBarButton(Ihandle* ih, const char* value)
{
#ifdef IUP_QML_HAS_X11_APP
  auto* window = reinterpret_cast<QWindow*>(ih->handle);

  if (!value || !window)
    return;

  if (QGuiApplication::platformName() != QLatin1String("xcb"))
    return;

  auto* x11_app = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
  if (!x11_app)
    return;

  Display* xdisplay = x11_app->display();
  if (!xdisplay)
    return;

#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return;
#endif

  Atom net_wm_state = XInternAtom(xdisplay, "_NET_WM_STATE", 0);
  Atom skip_taskbar = XInternAtom(xdisplay, "_NET_WM_STATE_SKIP_TASKBAR", 0);

  XEvent evt;
  memset(&evt, 0, sizeof(evt));
  evt.xclient.type = ClientMessage;
  evt.xclient.window = static_cast<Window>(window->winId());
  evt.xclient.message_type = net_wm_state;
  evt.xclient.format = 32;
  evt.xclient.data.l[0] = iupStrEqualNoCase(value, "HIDE")? 1: 0;
  evt.xclient.data.l[1] = static_cast<long>(skip_taskbar);
  evt.xclient.data.l[3] = 1;

  XSendEvent(xdisplay, XRootWindow(xdisplay, XDefaultScreen(xdisplay)), 0,
             SubstructureNotifyMask | SubstructureRedirectMask, &evt);
#else
  (void)ih;
  (void)value;
#endif
}

static int qmlDialogSetTaskBarButtonAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle && (reinterpret_cast<QWindow*>(ih->handle))->isVisible())
    qmlDialogSetTaskBarButton(ih, value);
  return 1;
}

extern "C" IUP_SDK_API void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  auto* window = reinterpret_cast<IupQmlDialog*>(ih->handle);

  if (window)
  {
    if (visible)
    {
      window->setVisible(true);
      iupqmlMnemonicUpdate(window->contentItem());

      if (!iupAttribGet(ih, "_IUPQML_SHOWN"))
      {
        iupAttribSet(ih, "_IUPQML_SHOWN", "1");
        window->handleResize();
      }

      if (!iupAttribGetBoolean(ih, "SHOWNOACTIVATE") && !(window->windowState() & Qt::WindowMinimized))
      {
        window->raise();
        if (QGuiApplication::platformName() != "wayland")
          window->requestActivate();
      }

      window->layoutContent();

      qmlDialogSetTaskBarButton(ih, iupAttribGet(ih, "TASKBARBUTTON"));

      if (iupAttribGet(ih, "RESIZEINC"))
      {
        int min_w = 1, min_h = 1;
        iupStrToIntInt(iupAttribGet(ih, "MINSIZE"), &min_w, &min_h, 'x');
        qmlDialogSetResizeInc(ih, iupAttribGet(ih, "RESIZEINC"), min_w, min_h);
      }
    }
    else
    {
      window->hide();
    }
  }
}

extern "C" IUP_SDK_API void iupdrvDialogGetPosition(Ihandle* ih, InativeHandle* handle, int* x, int* y)
{
  QWindow* window;

  if (!handle)
    handle = ih->handle;

  window = reinterpret_cast<QWindow*>(handle);

  if (QGuiApplication::platformName() == "wayland")
  {
    if (x) *x = 0;
    if (y) *y = 0;
  }
  else if (window && window->isVisible())
  {
    QPoint pos = window->framePosition();
    if (x) *x = pos.x();
    if (y) *y = pos.y();
  }
  else if (ih)
  {
    if (x) *x = iupAttribGetInt(ih, "_IUPQML_OLD_X");
    if (y) *y = iupAttribGetInt(ih, "_IUPQML_OLD_Y");
  }
}

extern "C" IUP_SDK_API void iupdrvDialogSetPosition(Ihandle* ih, int x, int y)
{
  auto* window = reinterpret_cast<QWindow*>(ih->handle);

  if (window)
  {
    window->setFramePosition(QPoint(x, y));

    iupAttribSetInt(ih, "_IUPQML_OLD_X", x);
    iupAttribSetInt(ih, "_IUPQML_OLD_Y", y);
  }
}

extern "C" IUP_SDK_API void iupdrvDialogGetDecoration(Ihandle* ih, int* border, int* caption, int* menu)
{
  const int est_border = 5, est_caption = 25;

  *menu = qmlDialogGetMenuSize(ih);

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME") || iupAttribGetBoolean(ih, "HIDETITLEBAR"))
  {
    *border = 0;
    *caption = 0;
    return;
  }

  int native_border = iupAttribGetInt(ih, "_IUPQML_NATIVE_BORDER");
  int native_caption = iupAttribGetInt(ih, "_IUPQML_NATIVE_CAPTION");

  int has_titlebar = iupAttribGetBoolean(ih, "RESIZE")  ||
                     iupAttribGetBoolean(ih, "MAXBOX")  ||
                     iupAttribGetBoolean(ih, "MINBOX")  ||
                     iupAttribGetBoolean(ih, "MENUBOX") ||
                     iupAttribGet(ih, "TITLE");

  int has_border = has_titlebar ||
                   iupAttribGetBoolean(ih, "RESIZE") ||
                   iupAttribGetBoolean(ih, "BORDER");

  if (native_border > 0 && native_caption > 0)
  {
    *border = has_border ? native_border : 0;
    *caption = has_titlebar ? native_caption : 0;
    return;
  }

  if (ih->handle && iupdrvDialogIsVisible(ih))
  {
    auto* window = reinterpret_cast<QWindow*>(ih->handle);
    QMargins margins = window->frameMargins();

    if (!margins.isNull())
    {
      int win_border = (margins.left() + margins.right()) / 2;
      int win_caption = margins.top() + margins.bottom() - win_border;

      if (win_border >= 0 && win_border < 100 && win_caption >= 0 && win_caption < 200)
      {
        *border = has_border ? win_border : 0;
        *caption = has_titlebar ? win_caption : 0;

        if (!native_border && !native_caption && ih->currentheight > 0)
        {
          int est_b = has_border ? est_border : 0, est_c = has_titlebar ? est_caption : 0;
          ih->currentwidth  += 2 * (*border - est_b);
          ih->currentheight += 2 * (*border - est_b) + (*caption - est_c);
        }

        if (win_border > 0)
          iupAttribSetInt(ih, "_IUPQML_NATIVE_BORDER", win_border);
        if (win_caption > 0)
          iupAttribSetInt(ih, "_IUPQML_NATIVE_CAPTION", win_caption);

        return;
      }
    }
  }

  *border = 0;
  if (has_border)
    *border = native_border ? native_border : est_border;

  *caption = 0;
  if (has_titlebar)
    *caption = native_caption ? native_caption : est_caption;
}

static void qmlDialogGetClientSize(Ihandle* ih, int* width, int* height)
{
  auto* window = reinterpret_cast<QWindow*>(ih->handle);

  if (window)
  {
    QSize size = window->size();
    int menu = qmlDialogGetMenuSize(ih);

    *width = size.width();
    *height = size.height() - menu;
  }
}

extern "C" IUP_SDK_API int iupdrvDialogSetPlacement(Ihandle* ih)
{
  auto* window = reinterpret_cast<QWindow*>(ih->handle);
  if (!window)
    return 0;

  char* placement;
  int old_state = ih->data->show_state;

  ih->data->show_state = IUP_SHOW;

  if (iupAttribGetBoolean(ih, "FULLSCREEN"))
  {
    window->setWindowState(Qt::WindowFullScreen);
    return 1;
  }

  placement = iupAttribGet(ih, "PLACEMENT");
  if (!placement)
  {
    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;

    if (window->isVisible())
      window->setWindowState(Qt::WindowNoState);

    if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE") && iupDialogCustomFrameRestore(ih))
    {
      ih->data->show_state = IUP_RESTORE;
      return 1;
    }

    return 0;
  }

  if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE") && iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    iupDialogCustomFrameMaximize(ih);
    iupAttribSet(ih, "PLACEMENT", nullptr);
    ih->data->show_state = IUP_MAXIMIZE;
    return 1;
  }

  if (iupStrEqualNoCase(placement, "MINIMIZED"))
  {
    ih->data->show_state = IUP_MINIMIZE;
    window->setWindowState(Qt::WindowMinimized);
  }
  else if (iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    ih->data->show_state = IUP_MAXIMIZE;
    window->setWindowState(Qt::WindowMaximized);
  }
  else if (iupStrEqualNoCase(placement, "FULL"))
  {
    int width, height, x, y;
    int border, caption, menu;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    x = -(border);
    y = -(border+caption+menu);

    iupdrvGetFullSize(&width, &height);

    height += menu;

    iupdrvDialogSetPosition(ih, x, y);
    window->resize(width, height);

    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;
  }

  iupAttribSet(ih, "PLACEMENT", nullptr);
  return 1;
}

/****************************************************************************
 * Dialog Attributes
 ****************************************************************************/

static void qmlDialogSetMinMax(Ihandle* ih, int min_w, int min_h, int max_w, int max_h)
{
  auto* window = reinterpret_cast<QWindow*>(ih->handle);
  if (!window)
    return;

  int border = 0, caption = 0, menu = 0;
  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  int decorwidth = 2*border;
  int decorheight = 2*border + caption;

  QSize minSize(1, 1);
  if (min_w > decorwidth)
    minSize.setWidth(min_w - decorwidth);
  if (min_h > decorheight)
    minSize.setHeight(min_h - decorheight);

  QSize maxSize((1 << 24) - 1, (1 << 24) - 1);
  if (max_w < 65535 && max_w > decorwidth && max_w > minSize.width())
    maxSize.setWidth(max_w - decorwidth);
  if (max_h < 65535 && max_h > decorheight && max_h > minSize.height())
    maxSize.setHeight(max_h - decorheight);

  window->setMinimumSize(minSize);
  window->setMaximumSize(maxSize);
}

static int qmlDialogSetMinSizeAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return iupBaseSetMinSizeAttrib(ih, value);

  int min_w = 1, min_h = 1;
  int max_w = 65535, max_h = 65535;
  iupStrToIntInt(value, &min_w, &min_h, 'x');

  iupStrToIntInt(iupAttribGet(ih, "MAXSIZE"), &max_w, &max_h, 'x');

  qmlDialogSetMinMax(ih, min_w, min_h, max_w, max_h);

  qmlDialogSetResizeInc(ih, iupAttribGet(ih, "RESIZEINC"), min_w, min_h);

  return iupBaseSetMinSizeAttrib(ih, value);
}

static int qmlDialogSetMaxSizeAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return iupBaseSetMaxSizeAttrib(ih, value);

  int min_w = 1, min_h = 1;
  int max_w = 65535, max_h = 65535;
  iupStrToIntInt(value, &max_w, &max_h, 'x');

  iupStrToIntInt(iupAttribGet(ih, "MINSIZE"), &min_w, &min_h, 'x');

  qmlDialogSetMinMax(ih, min_w, min_h, max_w, max_h);

  return iupBaseSetMaxSizeAttrib(ih, value);
}

static int qmlDialogSetResizeIncAttrib(Ihandle* ih, const char* value)
{
  int min_w = 1, min_h = 1;

  iupStrToIntInt(iupAttribGet(ih, "MINSIZE"), &min_w, &min_h, 'x');
  qmlDialogSetResizeInc(ih, value, min_w, min_h);

  return 1;
}

static char* qmlDialogGetClientSizeAttrib(Ihandle* ih)
{
  if (ih->handle)
  {
    int width, height;
    qmlDialogGetClientSize(ih, &width, &height);
    return iupStrReturnIntInt(width, height, 'x');
  }

  return iupDialogGetClientSizeAttrib(ih);
}

static char* qmlDialogGetClientOffsetAttrib(Ihandle* ih)
{
  (void)ih;
  return const_cast<char*>("0x0");
}

static int qmlDialogSetFlagAttrib(Ihandle* ih, const char* name, const char* value)
{
  iupAttribSetStr(ih, name, value);
  if (ih->handle)
  {
    auto* dialog = reinterpret_cast<IupQmlDialog*>(ih->handle);
    if (dialog->isVisible())
    {
      bool was_active = dialog->isActive();
      QPoint pos = dialog->position();
      dialog->hide();
      dialog->updateWindowFlags();
      dialog->setPosition(pos);
      dialog->setVisible(true);
      if (was_active)
        dialog->requestActivate();
    }
    else
      dialog->updateWindowFlags();
  }
  return 1;
}

static int qmlDialogSetResizeAttrib(Ihandle* ih, const char* value)
{
  return qmlDialogSetFlagAttrib(ih, "RESIZE", value);
}

static int qmlDialogSetMinBoxAttrib(Ihandle* ih, const char* value)
{
  return qmlDialogSetFlagAttrib(ih, "MINBOX", value);
}

static int qmlDialogSetMaxBoxAttrib(Ihandle* ih, const char* value)
{
  return qmlDialogSetFlagAttrib(ih, "MAXBOX", value);
}

static int qmlDialogSetMenuBoxAttrib(Ihandle* ih, const char* value)
{
  return qmlDialogSetFlagAttrib(ih, "MENUBOX", value);
}

static int qmlDialogSetBorderAttrib(Ihandle* ih, const char* value)
{
  return qmlDialogSetFlagAttrib(ih, "BORDER", value);
}

static int qmlDialogSetDialogHintAttrib(Ihandle* ih, const char* value)
{
  return qmlDialogSetFlagAttrib(ih, "DIALOGHINT", value);
}

static int qmlDialogSetToolBoxAttrib(Ihandle* ih, const char* value)
{
  return qmlDialogSetFlagAttrib(ih, "TOOLBOX", value);
}

static int qmlDialogSetHideTitleBarAttrib(Ihandle* ih, const char* value)
{
  return qmlDialogSetFlagAttrib(ih, "HIDETITLEBAR", value);
}

static int qmlDialogSetTopMostAttrib(Ihandle* ih, const char* value)
{
  return qmlDialogSetFlagAttrib(ih, "TOPMOST", value);
}

static int qmlDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
  auto* window = reinterpret_cast<QWindow*>(ih->handle);
  if (window)
    window->setTitle(value ? QString::fromUtf8(value) : QString());

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME") || iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
    return 0;

  return 1;
}

static int qmlDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{
  auto* window = reinterpret_cast<QWindow*>(ih->handle);
  if (!window)
    return 0;

  if (iupStrBoolean(value))
  {
    if (!iupAttribGet(ih, "_IUPQML_FS_STYLE"))
    {
      iupAttribSetStr(ih, "_IUPQML_FS_MAXBOX", iupAttribGet(ih, "MAXBOX"));
      iupAttribSetStr(ih, "_IUPQML_FS_MINBOX", iupAttribGet(ih, "MINBOX"));
      iupAttribSetStr(ih, "_IUPQML_FS_MENUBOX", iupAttribGet(ih, "MENUBOX"));
      iupAttribSetStr(ih, "_IUPQML_FS_RESIZE", iupAttribGet(ih, "RESIZE"));
      iupAttribSetStr(ih, "_IUPQML_FS_BORDER", iupAttribGet(ih, "BORDER"));
      iupAttribSetStr(ih, "_IUPQML_FS_TITLE", iupAttribGet(ih, "TITLE"));

      iupAttribSet(ih, "MAXBOX", "NO");
      iupAttribSet(ih, "MINBOX", "NO");
      iupAttribSet(ih, "MENUBOX", "NO");
      IupSetAttribute(ih, "TITLE", nullptr);
      iupAttribSet(ih, "RESIZE", "NO");
      iupAttribSet(ih, "BORDER", "NO");

      window->setWindowState(Qt::WindowFullScreen);

      iupAttribSet(ih, "_IUPQML_FS_STYLE", "YES");
    }
  }
  else
  {
    if (iupAttribGet(ih, "_IUPQML_FS_STYLE"))
    {
      iupAttribSet(ih, "_IUPQML_FS_STYLE", nullptr);

      iupAttribSetStr(ih, "MAXBOX", iupAttribGet(ih, "_IUPQML_FS_MAXBOX"));
      iupAttribSetStr(ih, "MINBOX", iupAttribGet(ih, "_IUPQML_FS_MINBOX"));
      iupAttribSetStr(ih, "MENUBOX", iupAttribGet(ih, "_IUPQML_FS_MENUBOX"));
      IupSetAttribute(ih, "TITLE", iupAttribGet(ih, "_IUPQML_FS_TITLE"));
      iupAttribSetStr(ih, "RESIZE", iupAttribGet(ih, "_IUPQML_FS_RESIZE"));
      iupAttribSetStr(ih, "BORDER", iupAttribGet(ih, "_IUPQML_FS_BORDER"));

      window->setWindowState(Qt::WindowNoState);

      iupAttribSet(ih, "_IUPQML_FS_MAXBOX", nullptr);
      iupAttribSet(ih, "_IUPQML_FS_MINBOX", nullptr);
      iupAttribSet(ih, "_IUPQML_FS_MENUBOX", nullptr);
      iupAttribSet(ih, "_IUPQML_FS_TITLE", nullptr);
      iupAttribSet(ih, "_IUPQML_FS_RESIZE", nullptr);
      iupAttribSet(ih, "_IUPQML_FS_BORDER", nullptr);
    }
  }

  return 1;
}

static char* qmlDialogGetActiveWindowAttrib(Ihandle* ih)
{
  auto* window = reinterpret_cast<QWindow*>(ih->handle);
  if (!window)
    return nullptr;

  return iupStrReturnBoolean(window->isActive());
}

static int qmlDialogSetBringFrontAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    auto* window = reinterpret_cast<QWindow*>(ih->handle);
    if (window)
    {
      window->raise();
      window->requestActivate();
    }
  }
  return 0;
}

static int qmlDialogSetOpacityAttrib(Ihandle* ih, const char* value)
{
  auto* window = reinterpret_cast<QWindow*>(ih->handle);
  if (!window)
    return 0;

  int opacity;
  if (iupStrToInt(value, &opacity))
  {
    if (opacity < 0) opacity = 0;
    if (opacity > 255) opacity = 255;
    window->setOpacity(static_cast<qreal>(opacity) / 255.0);
    return 1;
  }
  return 0;
}

static int qmlDialogSetIconAttrib(Ihandle* ih, const char* value)
{
  auto* window = reinterpret_cast<QWindow*>(ih->handle);
  if (!window)
    return 0;

  if (!value)
  {
    window->setIcon(QIcon());
  }
  else
  {
    auto* pixmap = static_cast<QPixmap*>(iupImageGetIcon(value));
    if (pixmap)
    {
      window->setIcon(QIcon(*pixmap));
      return 1;
    }
  }

  return 0;
}

static QQuickItem* qmlDialogGetBackImage(Ihandle* ih, int create)
{
  auto* image = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_BACKIMAGE"));
  if (image || !create)
    return image;

  auto* dialog = reinterpret_cast<IupQmlDialog*>(ih->handle);
  image = iupqmlCreateItem("import QtQuick\nImage { z: -1; cache: false; horizontalAlignment: Image.AlignLeft; verticalAlignment: Image.AlignTop }");
  if (!image)
    return nullptr;

  image->setParentItem(dialog->contentItem());
  image->setPosition(QPointF(0, 0));
  image->setSize(dialog->size());
  iupAttribSet(ih, "_IUPQML_BACKIMAGE", reinterpret_cast<char*>(image));
  return image;
}

static int qmlDialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  auto* dialog = reinterpret_cast<IupQmlDialog*>(ih->handle);
  if (!dialog)
    return 0;

  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    if (iupAttribGet(ih, "OPACITYIMAGE"))
      return 1;
    QQuickItem* image = qmlDialogGetBackImage(ih, 0);
    if (image)
      image->setVisible(false);
    dialog->setColor(QColor(r, g, b));
    return 1;
  }
  else
  {
    auto* pixmap = static_cast<QPixmap*>(iupImageGetImage(value, ih, 0, nullptr));
    if (pixmap)
    {
      QQuickItem* image = qmlDialogGetBackImage(ih, 1);
      if (!image)
        return 0;

      image->setProperty("source", QUrl(iupqmlImageUrl(pixmap)));
      image->setProperty("fillMode", iupAttribGetBoolean(ih, "BACKIMAGEZOOM") ? 0 : 3);
      image->setVisible(true);
      return 1;
    }
  }

  return 0;
}

static int qmlDialogSetBgColorAttrib(Ihandle* ih, const char* value)
{
  auto* dialog = reinterpret_cast<IupQmlDialog*>(ih->handle);
  if (!dialog)
    return 0;

  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (!iupAttribGet(ih, "OPACITYIMAGE"))
    dialog->setColor(QColor(r, g, b));
  iupqmlSetPaletteColor(iupqmlDialogGetContent(ih), "window", QColor(r, g, b));
  return 1;
}

static int qmlDialogSetShapeImageAttrib(Ihandle* ih, const char* value)
{
  auto* window = reinterpret_cast<QWindow*>(ih->handle);
  if (!window)
    return 0;

  if (!value)
  {
    window->setMask(QRegion());
    return 1;
  }

  auto* pixmap = static_cast<QPixmap*>(iupImageGetImage(value, ih, 0, nullptr));
  if (pixmap)
  {
    window->setMask(QRegion(pixmap->mask()));
    return 1;
  }

  return 0;
}

static int qmlDialogSetOpacityImageAttrib(Ihandle* ih, const char* value)
{
  auto* dialog = reinterpret_cast<IupQmlDialog*>(ih->handle);
  if (!dialog)
    return 0;

  if (!value)
  {
    QQuickItem* image = qmlDialogGetBackImage(ih, 0);
    if (image)
      image->setVisible(false);
    return 0;
  }

  auto* pixmap = static_cast<QPixmap*>(iupImageGetImage(value, ih, 0, nullptr));
  if (!pixmap)
    return 0;

  QQuickItem* image = qmlDialogGetBackImage(ih, 1);
  if (!image)
    return 0;

  dialog->setColor(Qt::transparent);
  image->setProperty("source", QUrl(iupqmlImageUrl(pixmap)));
  image->setProperty("fillMode", 6);
  image->setVisible(true);

  return 1;
}

static char* qmlDialogGetMaximizedAttrib(Ihandle* ih)
{
  auto* window = reinterpret_cast<QWindow*>(ih->handle);
  if (!window)
    return nullptr;

  return iupStrReturnBoolean(window->windowState() & Qt::WindowMaximized);
}

static char* qmlDialogGetMinimizedAttrib(Ihandle* ih)
{
  auto* window = reinterpret_cast<QWindow*>(ih->handle);
  if (!window)
    return nullptr;

  return iupStrReturnBoolean(window->windowState() & Qt::WindowMinimized);
}

/****************************************************************************
 * Dialog Methods
 ****************************************************************************/

static void qmlDialogSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (ih->firstchild)
  {
    char* offset = iupAttribGet(ih, "CHILDOFFSET");

    x = 0;
    y = 0;

    if (offset) iupStrToIntInt(offset, &x, &y, 'x');

    iupBaseSetPosition(ih->firstchild, x, y);
  }
}

static void* qmlDialogGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  auto* dialog = reinterpret_cast<IupQmlDialog*>(ih->handle);
  if (dialog)
    return reinterpret_cast<void*>(dialog->content);
  return nullptr;
}

extern "C" int qmlDialogMapMethod(Ihandle* ih)
{
  auto* dialog = new IupQmlDialog(ih);

  ih->handle = reinterpret_cast<InativeHandle*>(dialog);

  iupqmlUpdateWindowPalette(dialog);

  if (iupAttribGet(ih, "OPACITYIMAGE"))
  {
    QSurfaceFormat format = dialog->format();
    format.setAlphaBufferSize(8);
    dialog->setFormat(format);
  }

  iupqmlInstallFilter(ih, dialog->content);

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME"))
    iupDialogCustomFrameSimulateCheckCallbacks(ih);

  const char* title = iupAttribGetStr(ih, "TITLE");
  if (title)
    dialog->setTitle(QString::fromUtf8(title));

  if (ih->data->menu && !ih->data->menu->handle)
  {
    ih->data->menu->parent = ih;
    IupMap(ih->data->menu);
  }

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  InativeHandle* parent = iupDialogGetNativeParent(ih);
  if (parent)
    iupdrvDialogSetParent(ih, parent);

  qmlDialogSetMinMax(ih, 1, 1, 65535, 65535);

  iupAttribSet(ih, "VISIBLE", nullptr);

  return IUP_NOERROR;
}

extern "C" void qmlDialogUnMapMethod(Ihandle* ih)
{
  iupAttribSet(ih, "_IUPQML_SHOWN", nullptr);
  auto* dialog = reinterpret_cast<IupQmlDialog*>(ih->handle);

  if (dialog)
  {
    if (ih->data->menu)
    {
      ih->data->menu->handle = nullptr;
      IupDestroy(ih->data->menu);
      ih->data->menu = nullptr;
    }

    qmlDialogDisconnectParent(ih);
    iupqmlDragDropCleanup(ih);
    iupqmlTipsDestroy(ih);
    iupqmlRemoveFilter(dialog->content);
    iupAttribSet(ih, "_IUPQML_EVENT_ITEM", nullptr);
    iupAttribSet(ih, "_IUPQML_BACKIMAGE", nullptr);

    dialog->ih = nullptr;
    dialog->hide();
    dialog->deleteLater();
    ih->handle = nullptr;
  }
}

extern "C" void qmlDialogLayoutUpdateMethod(Ihandle* ih)
{
  int border, caption, menu;
  int width, height;

  auto* dialog = reinterpret_cast<IupQmlDialog*>(ih->handle);

  if (ih->data->ignore_resize ||
      iupAttribGet(ih, "_IUPQML_FS_STYLE"))
  {
    return;
  }

  ih->data->ignore_resize = 1;

  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  width = ih->currentwidth - 2*border;
  height = ih->currentheight - 2*border - caption;

  if (width <= 0) width = 1;
  if (height <= 0) height = 1;

  if (dialog && !(dialog->windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen)))
  {
    dialog->resize(width, height);
    dialog->layoutContent();
  }

  if (!iupAttribGetBoolean(ih, "RESIZE"))
  {
    qmlDialogSetMinMax(ih, ih->currentwidth, ih->currentheight,
                       ih->currentwidth, ih->currentheight);
  }

  ih->data->ignore_resize = 0;
}

static int qmlDialogSetBackImageZoomAttrib(Ihandle* ih, const char* value)
{
  QQuickItem* image = qmlDialogGetBackImage(ih, 0);
  if (image && image->isVisible() && !iupAttribGet(ih, "OPACITYIMAGE"))
    image->setProperty("fillMode", iupStrBoolean(value) ? 0 : 3);
  return 1;
}

extern "C" IUP_SDK_API void iupdrvDialogInitClass(Iclass* ic)
{
  ic->Map = qmlDialogMapMethod;
  ic->UnMap = qmlDialogUnMapMethod;
  ic->LayoutUpdate = qmlDialogLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = qmlDialogGetInnerNativeContainerHandleMethod;
  ic->SetChildrenPosition = qmlDialogSetChildrenPositionMethod;

  iupClassRegisterAttribute(ic, "CLIENTSIZE", qmlDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", qmlDialogGetClientOffsetAttrib, nullptr, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, qmlDialogSetBgColorAttrib, "DLGBGCOLOR", nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BACKGROUND", nullptr, qmlDialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGEZOOM", nullptr, qmlDialogSetBackImageZoomAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ICON", nullptr, qmlDialogSetIconAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FULLSCREEN", nullptr, qmlDialogSetFullScreenAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINSIZE", nullptr, qmlDialogSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESIZEINC", nullptr, qmlDialogSetResizeIncAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXSIZE", nullptr, qmlDialogSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", nullptr, qmlDialogSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESIZE", nullptr, qmlDialogSetResizeAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BORDER", nullptr, qmlDialogSetBorderAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINBOX", nullptr, qmlDialogSetMinBoxAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXBOX", nullptr, qmlDialogSetMaxBoxAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MENUBOX", nullptr, qmlDialogSetMenuBoxAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ACTIVEWINDOW", qmlDialogGetActiveWindowAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPMOST", nullptr, qmlDialogSetTopMostAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIALOGHINT", nullptr, qmlDialogSetDialogHintAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITY", nullptr, qmlDialogSetOpacityAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITYIMAGE", nullptr, qmlDialogSetOpacityImageAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHAPEIMAGE", nullptr, qmlDialogSetShapeImageAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BRINGFRONT", nullptr, qmlDialogSetBringFrontAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDETITLEBAR", nullptr, qmlDialogSetHideTitleBarAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXIMIZED", qmlDialogGetMaximizedAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINIMIZED", qmlDialogGetMinimizedAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, iupqmlGetNativeWindowHandleName(), iupqmlGetNativeWindowHandleAttrib, nullptr, nullptr, nullptr, IUPAF_NO_STRING|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COMPOSITED", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOOLBOX", nullptr, qmlDialogSetToolBoxAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
#ifdef IUP_QML_HAS_X11_APP
  iupClassRegisterAttribute(ic, "TASKBARBUTTON", nullptr, qmlDialogSetTaskBarButtonAttrib, IUPAF_SAMEASSYSTEM, nullptr, IUPAF_NO_INHERIT);
#else
  iupClassRegisterAttribute(ic, "TASKBARBUTTON", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
#endif
  iupClassRegisterAttribute(ic, "HELPBUTTON", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWNOACTIVATE", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUSTOMFRAME", nullptr, nullptr, IUPAF_SAMEASSYSTEM, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SAVEUNDER", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTROL", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
