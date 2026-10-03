/** \file
 * \brief Qt Quick Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>

#include <QGuiApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QCoreApplication>
#include <QCursor>
#include <QThread>
#include <QElapsedTimer>
#include <QEvent>
#include <QMouseEvent>
#include <QPalette>
#include <QColor>
#include <QQmlContext>
#include <QQmlProperty>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_dialog.h"
#include "iup_dlglist.h"
#include "iup_image.h"
}

#include "iupqml_drv.h"

#include <cstring>


/****************************************************************************
 * Ihandle Association
 ****************************************************************************/

IUP_DRV_API void iupqmlSetIhandle(QObject* obj, Ihandle* ih)
{
  if (obj)
    obj->setProperty("_iup_ih", QVariant::fromValue(reinterpret_cast<void*>(ih)));
}

IUP_DRV_API Ihandle* iupqmlGetIhandle(QObject* obj)
{
  if (!obj)
    return nullptr;
  QVariant v = obj->property("_iup_ih");
  if (!v.isValid())
    return nullptr;
  return static_cast<Ihandle*>(v.value<void*>());
}

IUP_DRV_API QQuickItem* iupqmlGetItem(Ihandle* ih)
{
  if (!ih)
    return nullptr;

  auto* item = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUP_EXTRAPARENT"));
  if (!item)
  {
    if (ih->iclass->nativetype == IUP_TYPEDIALOG)
      return nullptr;
    item = reinterpret_cast<QQuickItem*>(ih->handle);
  }
  return item;
}

static QQuickItem* qmlGetNativeParent(Ihandle* ih)
{
  return reinterpret_cast<QQuickItem*>(iupChildTreeGetNativeParentHandle(ih));
}

IUP_DRV_API QQuickWindow* iupqmlGetParentWindow(Ihandle* ih)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  if (parent)
    return reinterpret_cast<QQuickWindow*>(parent);

  {
    Ihandle* ih_focus = IupGetFocus();
    if (ih_focus)
    {
      Ihandle* dlg = IupGetDialog(ih_focus);
      if (dlg && dlg->handle)
        return reinterpret_cast<QQuickWindow*>(dlg->handle);
    }
  }

  {
    Ihandle* dlg_iter = iupDlgListFirst();
    while (dlg_iter)
    {
      if (dlg_iter->handle && dlg_iter != ih && iupdrvIsVisible(dlg_iter))
        return reinterpret_cast<QQuickWindow*>(dlg_iter->handle);
      dlg_iter = iupDlgListNext();
    }
  }

  return nullptr;
}

IUP_DRV_API QQuickWindow* iupqmlDialogHostWindow(Ihandle* ih, int* owned)
{
  QQuickWindow* parent = iupqmlGetParentWindow(ih);
  if (parent && parent->isVisible())
  {
    *owned = 0;
    return parent;
  }

  auto* host = new QQuickWindow();
  host->setFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
  host->setColor(Qt::transparent);
  host->resize(1, 1);
  iupqmlUpdateWindowPalette(host);

  QScreen* screen = QGuiApplication::primaryScreen();
  QRect area = screen ? screen->availableGeometry() : QRect(0, 0, 800, 600);
  host->setPosition(area.x() + area.width() / 2, area.y() + area.height() / 2);
  host->show();

  *owned = 1;
  return host;
}

/****************************************************************************
 * Item Event Filter
 ****************************************************************************/

class IupQmlItemFilter : public QObject
{
public:
  Ihandle* ih;
  QQuickItem* item;

  IupQmlItemFilter(QQuickItem* parent, Ihandle* handle) : QObject(parent), ih(handle), item(parent) {}

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    if (!iupObjectCheck(ih) || obj != item)
      return false;

    switch (event->type())
    {
    case QEvent::KeyPress:
      if (item->property("_iup_keys_handled").toBool())
        return false;
      return iupqmlKeyPressEvent(item, static_cast<QKeyEvent*>(event), ih) != 0;

    case QEvent::KeyRelease:
      if (ih->iclass->nativetype == IUP_TYPECANVAS)
        return iupqmlKeyReleaseEvent(item, static_cast<QKeyEvent*>(event), ih) != 0;
      return false;

    case QEvent::FocusIn:
    case QEvent::FocusOut:
      if (!item->isFocusScope())
        iupqmlFocusInOutEvent(event, ih);
      return false;

    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
      iupqmlEnterLeaveEvent(event, ih);
      return false;

    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
      if (iupqmlMouseButtonEvent(static_cast<QMouseEvent*>(event), ih))
        return true;
      if (strcmp(item->metaObject()->className(), "QQuickItem") == 0)
      {
        event->accept();
        return true;
      }
      return false;

    case QEvent::MouseMove:
      iupqmlMouseMoveEvent(static_cast<QMouseEvent*>(event), ih);
      return false;

    case QEvent::HoverMove:
      {
        auto* hover = static_cast<QHoverEvent*>(event);
        auto cb = reinterpret_cast<IFniis>(IupGetCallback(ih, "MOTION_CB"));
        if (cb)
        {
          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
          iupqmlButtonKeySetStatus(hover->modifiers(), Qt::NoButton, 0, status, 0);
          cb(ih, static_cast<int>(hover->position().x()), static_cast<int>(hover->position().y()), status);
        }
      }
      return false;

    default:
      return false;
    }
  }
};

IUP_DRV_API void iupqmlRemoveFilter(QQuickItem* item)
{
  if (!item)
    return;

  const QObjectList children = item->children();
  for (QObject* child : children)
  {
    auto* filter = dynamic_cast<IupQmlItemFilter*>(child);
    if (filter)
    {
      item->removeEventFilter(filter);
      delete filter;
    }
  }
}

IUP_DRV_API void iupqmlInstallFilter(Ihandle* ih, QQuickItem* item)
{
  if (!item)
    return;

  iupqmlSetIhandle(item, ih);
  iupAttribSet(ih, "_IUPQML_EVENT_ITEM", reinterpret_cast<char*>(item));
  item->setAcceptHoverEvents(true);
  item->installEventFilter(new IupQmlItemFilter(item, ih));

  if (item->isFocusScope())
    iupqmlConnect(item, "activeFocusChanged(bool)", [ih](void** args) {
      iupqmlFocusChanged(ih, *static_cast<bool*>(args[1]));
    });
}

/****************************************************************************
 * Parenting and Positioning
 ****************************************************************************/

IUP_DRV_API QQuickItem* iupqmlButtonFrameInset(QQuickItem* button)
{
  static const char* insets[] = {"leftInset", "rightInset", "topInset", "bottomInset"};
  if (!button)
    return button;
  for (const char* name : insets)
  {
    if (button->property(name).toDouble() < 1)
      button->setProperty(name, 1);
  }
  return button;
}

IUP_DRV_API int iupqmlMenuBarIsNative()
{
#ifdef Q_OS_MACOS
  return !QCoreApplication::testAttribute(Qt::AA_DontUseNativeMenuBar);
#else
  return 0;
#endif
}

IUP_DRV_API void iupqmlReleaseMouseGrab()
{
  const QList<QWindow*> windows = QGuiApplication::topLevelWindows();
  for (QWindow* window : windows)
  {
    auto* quick = qobject_cast<QQuickWindow*>(window);
    QQuickItem* grabber = quick ? quick->mouseGrabberItem() : nullptr;
    if (grabber)
      grabber->ungrabMouse();
  }
}

IUP_DRV_API void iupqmlUnsetArrowCursors(QQuickItem* item)
{
  if (!item)
    return;

  if (item->cursor().shape() == Qt::ArrowCursor)
    item->unsetCursor();

  const QList<QQuickItem*> children = item->childItems();
  for (QQuickItem* child : children)
    iupqmlUnsetArrowCursors(child);
}

IUP_DRV_API void iupqmlAddToParent(Ihandle* ih)
{
  QQuickItem* parent = qmlGetNativeParent(ih);
  QQuickItem* item = iupqmlGetItem(ih);

  if (parent && item)
  {
    item->setParentItem(parent);
    iupqmlUnsetArrowCursors(item);
  }
}

IUP_DRV_API void iupqmlSetPosSize(QQuickItem* item, int x, int y, int width, int height)
{
  if (!item)
    return;

  item->setPosition(QPointF(x, y));

  if (width > 0 && height > 0)
    item->setSize(QSizeF(width, height));
}

extern "C" IUP_SDK_API void iupdrvBaseLayoutUpdateMethod(Ihandle* ih)
{
  QQuickItem* item = iupqmlGetItem(ih);
  if (item)
    iupqmlSetPosSize(item, ih->x, ih->y, ih->currentwidth, ih->currentheight);
}

extern "C" IUP_SDK_API void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  if (ih->iclass->nativetype == IUP_TYPEVOID ||
      ih->iclass->nativetype == IUP_TYPEMENU)
    return;

  iupqmlDragDropCleanup(ih);
  iupqmlTipsDestroy(ih);

  auto* event_item = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_EVENT_ITEM"));
  if (event_item)
    iupqmlRemoveFilter(event_item);
  iupAttribSet(ih, "_IUPQML_EVENT_ITEM", nullptr);

  QQuickItem* item = iupqmlGetItem(ih);
  if (item)
  {
    iupqmlRemoveFilter(item);
    item->setVisible(false);
    item->setParentItem(nullptr);
    item->deleteLater();
  }

  iupAttribSet(ih, "_IUP_EXTRAPARENT", nullptr);
  ih->handle = nullptr;
}

extern "C" IUP_SDK_API void iupdrvReparent(Ihandle* ih)
{
  QQuickItem* new_parent = qmlGetNativeParent(ih);
  QQuickItem* item = iupqmlGetItem(ih);

  if (item && new_parent && item->parentItem() != new_parent)
    item->setParentItem(new_parent);
}

extern "C" IUP_SDK_API void iupdrvActivate(Ihandle* ih)
{
  auto* item = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!item)
    return;

  if (ih->iclass->nativetype == IUP_TYPEDIALOG)
    return;

  if (iupqmlCallMethod(item, "click"))
    return;

  if (iupqmlCallMethod(item, "toggle"))
    return;

  QMetaObject::invokeMethod(item, "accepted", Qt::DirectConnection);
}

/****************************************************************************
 * Redraw
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvPostRedraw(Ihandle* ih)
{
  if (iupqmlCanvasGetItem(ih))
  {
    iupqmlCanvasRedraw(ih, 0);
    return;
  }

  QQuickItem* item = iupqmlGetItem(ih);
  if (item)
    item->update();
}

extern "C" IUP_SDK_API void iupdrvRedrawNow(Ihandle* ih)
{
  if (iupqmlCanvasGetItem(ih))
  {
    iupqmlCanvasRedraw(ih, 1);
    return;
  }

  QQuickItem* item = iupqmlGetItem(ih);
  if (item)
    item->update();
}

/****************************************************************************
 * Screen/Client Coordinate Conversion
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvScreenToClient(Ihandle* ih, int* x, int* y)
{
  QQuickItem* item = iupqmlGetItem(ih);
  if (item)
  {
    QPointF local = item->mapFromGlobal(QPointF(*x, *y));
    *x = static_cast<int>(local.x());
    *y = static_cast<int>(local.y());
    return;
  }

  QQuickWindow* window = iupqmlDialogGetWindow(ih);
  if (window)
  {
    QPoint local = window->mapFromGlobal(QPoint(*x, *y));
    *x = local.x();
    *y = local.y();
  }
}

extern "C" IUP_SDK_API void iupdrvClientToScreen(Ihandle* ih, int* x, int* y)
{
  QQuickItem* item = iupqmlGetItem(ih);
  if (item)
  {
    QPointF global = item->mapToGlobal(QPointF(*x, *y));
    *x = static_cast<int>(global.x());
    *y = static_cast<int>(global.y());
    return;
  }

  QQuickWindow* window = iupqmlDialogGetWindow(ih);
  if (window)
  {
    QPoint global = window->mapToGlobal(QPoint(*x, *y));
    *x = global.x();
    *y = global.y();
  }
}

/****************************************************************************
 * Event Handlers
 ****************************************************************************/

IUP_DRV_API int iupqmlEnterLeaveEvent(QEvent* evt, Ihandle* ih)
{
  Icallback cb = nullptr;

  if (evt->type() == QEvent::HoverEnter || evt->type() == QEvent::Enter)
    cb = IupGetCallback(ih, "ENTERWINDOW_CB");
  else if (evt->type() == QEvent::HoverLeave || evt->type() == QEvent::Leave)
    cb = IupGetCallback(ih, "LEAVEWINDOW_CB");

  if (cb)
    cb(ih);

  return 0;
}

IUP_DRV_API int iupqmlMouseMoveEvent(QMouseEvent* mouse_evt, Ihandle* ih)
{
  auto cb = reinterpret_cast<IFniis>(IupGetCallback(ih, "MOTION_CB"));
  if (cb)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupqmlButtonKeySetStatus(mouse_evt->modifiers(), mouse_evt->buttons(), 0, status, 0);
    cb(ih, static_cast<int>(mouse_evt->position().x()), static_cast<int>(mouse_evt->position().y()), status);
  }

  return 0;
}

IUP_DRV_API int iupqmlMouseButtonEvent(QMouseEvent* mouse_evt, Ihandle* ih)
{
  auto cb = reinterpret_cast<IFniiiis>(IupGetCallback(ih, "BUTTON_CB"));
  if (cb)
  {
    int doubleclick = 0, ret, press = 1;
    int button = IUP_BUTTON1;
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

    if (mouse_evt->type() == QEvent::MouseButtonRelease)
      press = 0;

    if (mouse_evt->type() == QEvent::MouseButtonDblClick)
      doubleclick = 1;

    if (mouse_evt->button() == Qt::LeftButton)
      button = IUP_BUTTON1;
    else if (mouse_evt->button() == Qt::MiddleButton)
      button = IUP_BUTTON2;
    else if (mouse_evt->button() == Qt::RightButton)
      button = IUP_BUTTON3;
    else if (mouse_evt->button() == Qt::XButton1)
      button = IUP_BUTTON4;
    else if (mouse_evt->button() == Qt::XButton2)
      button = IUP_BUTTON5;

    iupqmlButtonKeySetStatus(mouse_evt->modifiers(), mouse_evt->button(), button, status, doubleclick);

    int x = static_cast<int>(mouse_evt->position().x());
    int y = static_cast<int>(mouse_evt->position().y());

    if (doubleclick)
    {
      status[5] = ' ';
      ret = cb(ih, button, 0, x, y, status);
      if (ret == IUP_CLOSE)
        IupExitLoop();
      else if (ret == IUP_IGNORE)
        return 1;
      status[5] = 'D';
    }

    ret = cb(ih, button, press, x, y, status);
    if (ret == IUP_CLOSE)
      IupExitLoop();
    else if (ret == IUP_IGNORE)
      return 1;
  }

  return 0;
}

IUP_DRV_API void iupqmlUpdateMnemonic(Ihandle* ih)
{
  (void)ih;
}

IUP_DRV_API int iupqmlSetMnemonicTitle(Ihandle* ih, QObject* item, const char* value)
{
  char c = 0;
  char* str;
  (void)ih;

  if (!value)
    value = "";

  iupqmlSetProperty(item, "text", QString::fromUtf8(value));
  iupqmlMnemonicUpdate(item);

  str = iupStrProcessMnemonic(value, &c, -1);
  if (str != value)
  {
    free(str);
    return c ? 1 : 0;
  }

  return 0;
}

extern "C" IUP_SDK_API int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  QQuickItem* item = iupqmlGetItem(ih);

  if (item && item->parentItem() && iupdrvIsVisible(ih))
  {
    QList<QQuickItem*> siblings = item->parentItem()->childItems();
    if (iupStrEqualNoCase(value, "TOP"))
    {
      if (!siblings.isEmpty() && siblings.last() != item)
        item->stackAfter(siblings.last());
    }
    else
    {
      if (!siblings.isEmpty() && siblings.first() != item)
        item->stackBefore(siblings.first());
    }
  }

  return 0;
}

/****************************************************************************
 * Visibility and Active State
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvSetVisible(Ihandle* ih, int enable)
{
  if (ih->iclass->nativetype == IUP_TYPEVOID || ih->iclass->nativetype == IUP_TYPEMENU)
    return;

  QQuickItem* item = iupqmlGetItem(ih);
  if (item)
    item->setVisible(enable ? true : false);
}

extern "C" IUP_SDK_API int iupdrvIsVisible(Ihandle* ih)
{
  if (ih->iclass->nativetype == IUP_TYPEVOID || ih->iclass->nativetype == IUP_TYPEMENU)
    return 1;

  if (ih->iclass->nativetype == IUP_TYPEDIALOG)
  {
    QQuickWindow* window = iupqmlDialogGetWindow(ih);
    return (window && window->isVisible()) ? 1 : 0;
  }

  QQuickItem* item = iupqmlGetItem(ih);
  if (!item)
    return 0;

  if (!item->isVisible())
    return 0;

  Ihandle* parent = ih->parent;
  while (parent)
  {
    if (parent->iclass->nativetype != IUP_TYPEVOID && !iupdrvIsVisible(parent))
      return 0;
    parent = parent->parent;
  }

  return 1;
}

extern "C" IUP_SDK_API int iupdrvIsActive(Ihandle* ih)
{
  if (ih->iclass->nativetype == IUP_TYPEVOID || ih->iclass->nativetype == IUP_TYPEMENU)
    return 1;

  if (ih->iclass->nativetype == IUP_TYPEDIALOG)
  {
    QQuickWindow* window = iupqmlDialogGetWindow(ih);
    return (window && window->contentItem()->isEnabled()) ? 1 : 0;
  }

  QQuickItem* item = iupqmlGetItem(ih);
  return item ? (item->isEnabled() ? 1 : 0) : 0;
}

extern "C" IUP_SDK_API void iupdrvSetActive(Ihandle* ih, int enable)
{
  if (ih->iclass->nativetype == IUP_TYPEVOID)
    return;

  if (ih->iclass->nativetype == IUP_TYPEMENU)
  {
    if (ih->handle && (iupStrEqual(ih->iclass->name, "menuitem") || iupStrEqual(ih->iclass->name, "submenu")))
      (reinterpret_cast<QObject*>(ih->handle))->setProperty("enabled", enable ? true : false);
    return;
  }

  if (ih->iclass->nativetype == IUP_TYPEDIALOG)
  {
    QQuickWindow* window = iupqmlDialogGetWindow(ih);
    if (window)
      window->contentItem()->setEnabled(enable ? true : false);
    return;
  }

  QQuickItem* item = iupqmlGetItem(ih);
  if (item)
    item->setEnabled(enable ? true : false);
}

/****************************************************************************
 * Cursor Management
 ****************************************************************************/

static int qmlGetCursorShape(const char* name, Qt::CursorShape* shape)
{
  struct {
    const char* iupname;
    Qt::CursorShape qtshape;
  } table[] = {
    { "NONE",           Qt::BlankCursor },
    { "NULL",           Qt::BlankCursor },
    { "ARROW",          Qt::ArrowCursor },
    { "BUSY",           Qt::WaitCursor },
    { "CROSS",          Qt::CrossCursor },
    { "HAND",           Qt::PointingHandCursor },
    { "HELP",           Qt::WhatsThisCursor },
    { "IUP",            Qt::WhatsThisCursor },
    { "MOVE",           Qt::SizeAllCursor },
    { "PEN",            Qt::CrossCursor },
    { "RESIZE_N",       Qt::SizeVerCursor },
    { "RESIZE_S",       Qt::SizeVerCursor },
    { "RESIZE_NS",      Qt::SizeVerCursor },
    { "SPLITTER_HORIZ", Qt::SizeVerCursor },
    { "RESIZE_W",       Qt::SizeHorCursor },
    { "RESIZE_E",       Qt::SizeHorCursor },
    { "RESIZE_WE",      Qt::SizeHorCursor },
    { "SPLITTER_VERT",  Qt::SizeHorCursor },
    { "RESIZE_NE",      Qt::SizeBDiagCursor },
    { "RESIZE_SE",      Qt::SizeFDiagCursor },
    { "RESIZE_NW",      Qt::SizeFDiagCursor },
    { "RESIZE_SW",      Qt::SizeBDiagCursor },
    { "TEXT",           Qt::IBeamCursor },
    { "UPARROW",        Qt::UpArrowCursor }
  };

  int i, count = sizeof(table)/sizeof(table[0]);

  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(name, table[i].iupname))
    {
      *shape = table[i].qtshape;
      return 1;
    }
  }

  return 0;
}

extern "C" IUP_SDK_API int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value)
    return 0;

  QQuickItem* item = iupqmlGetItem(ih);
  if (!item && ih->iclass->nativetype == IUP_TYPEDIALOG && iupqmlDialogGetWindow(ih))
    item = iupqmlDialogGetWindow(ih)->contentItem();
  if (!item)
    return 0;

  Qt::CursorShape shape;

  if (qmlGetCursorShape(value, &shape))
    item->setCursor(QCursor(shape));
  else
  {
    auto* cursor = static_cast<QCursor*>(iupImageGetCursor(value));
    if (cursor)
      item->setCursor(*cursor);
    else
      item->setCursor(QCursor(Qt::ArrowCursor));
  }

  return 1;
}

/****************************************************************************
 * Colors
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value)
    return 0;

  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  auto* obj = reinterpret_cast<QObject*>(ih->handle);
  QColor color(r, g, b);
  iupqmlSetPaletteColor(obj, "window", color);
  iupqmlSetPaletteColor(obj, "base", color);
  iupqmlSetPaletteColor(obj, "button", color);

  return 1;
}

extern "C" IUP_SDK_API int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value)
    return 0;

  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  auto* obj = reinterpret_cast<QObject*>(ih->handle);
  if (ih->iclass->nativetype == IUP_TYPEDIALOG)
    obj = iupqmlDialogGetContent(ih);

  QColor color(r, g, b);
  iupqmlSetPaletteColor(obj, "windowText", color);
  iupqmlSetPaletteColor(obj, "buttonText", color);
  iupqmlSetPaletteColor(obj, "text", color);

  return 1;
}

extern "C" IUP_SDK_API void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
  (void)ic;
}

extern "C" IUP_SDK_API void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "TIPMARKUP", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TIPICON", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
}

/****************************************************************************
 * Scrollbar and Misc
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvGetScrollbarSize(void)
{
  static int size = -1;
  if (size < 0)
  {
    QQuickItem* sb = iupqmlTemplateItem(IUPQML_IMPORTS "ScrollBar { orientation: Qt.Vertical; policy: ScrollBar.AlwaysOn }");
    if (sb)
      size = static_cast<int>(sb->implicitWidth());
    if (size <= 0)
      size = 16;
  }
  return size;
}

extern "C" IUP_SDK_API void iupdrvWarpPointer(int x, int y)
{
  QCursor::setPos(x, y);
}

extern "C" IUP_SDK_API void iupdrvSleep(int time)
{
  QThread::msleep(time);
}

extern "C" IUP_SDK_API unsigned int iupdrvGetTickCount(void)
{
  QElapsedTimer timer;
  timer.start();
  return static_cast<unsigned int>(timer.msecsSinceReference());
}

extern "C" IUP_SDK_API void iupdrvSendKey(int key, int press)
{
  unsigned int keyval = 0, state = 0;
  iupdrvKeyEncode(key, &keyval, &state);
  if (!keyval) return;

  QWindow* receiver = QGuiApplication::focusWindow();
  if (!receiver) return;

  Qt::KeyboardModifiers mods(static_cast<int>(state));
  QString text;
  if (iup_isprint(iup_XkeyBase(key)) && !iup_isCtrlXkey(key) && !iup_isAltXkey(key) && !iup_isSysXkey(key))
    text = QString(QChar(iup_XkeyBase(key)));
  if (press & 0x01)
    QCoreApplication::postEvent(receiver, new QKeyEvent(QEvent::KeyPress, static_cast<int>(keyval), mods, text));
  if (press & 0x02)
    QCoreApplication::postEvent(receiver, new QKeyEvent(QEvent::KeyRelease, static_cast<int>(keyval), mods, text));
}

extern "C" IUP_SDK_API void iupdrvSendMouse(int x, int y, int bt, int status)
{
  QPoint global(x, y);
  QWindow* receiver = QGuiApplication::topLevelAt(global);
  if (!receiver) return;
  QPoint local = receiver->mapFromGlobal(global);

  if (bt == 'W')
  {
    QPoint angle(0, status * 120);
    QCoreApplication::postEvent(receiver,
                                new QWheelEvent(QPointF(local), QPointF(global), QPoint(), angle,
                                  Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false));
    return;
  }

  Qt::MouseButton button = Qt::NoButton;
  switch (bt)
  {
    case IUP_BUTTON1: button = Qt::LeftButton;   break;
    case IUP_BUTTON2: button = Qt::MiddleButton; break;
    case IUP_BUTTON3: button = Qt::RightButton;  break;
    default:
      if (status != -1)
        return;
      break;
  }

  QEvent::Type type;
  Qt::MouseButtons buttons = Qt::NoButton;
  if (status == -1)
  {
    type = QEvent::MouseMove;
    buttons = button;
    button = Qt::NoButton;
  }
  else if (status == 0)
  {
    type = QEvent::MouseButtonRelease;
  }
  else
  {
    type = (status == 2) ? QEvent::MouseButtonDblClick : QEvent::MouseButtonPress;
    buttons = button;
  }

  QCoreApplication::postEvent(receiver,
                              new QMouseEvent(type, QPointF(local), QPointF(global),
                                              button, buttons, Qt::NoModifier));
}

static void qmlSetAccessibleText(Ihandle* ih, const char* name, const char* value)
{
  QQuickItem* item;
  if (ih->iclass->nativetype == IUP_TYPEDIALOG)
    item = iupqmlDialogGetContent(ih);
  else
  {
    item = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_EVENT_ITEM"));
    if (!item)
      item = iupqmlGetItem(ih);
  }

  QQmlContext* context = nullptr;
  for (QQuickItem* parent = item; parent && !context; parent = parent->parentItem())
    context = qmlContext(parent);
  if (!context)
    return;

  QQmlProperty(item, QString::fromLatin1(name), context).write(value ? QString::fromUtf8(value) : QString());
}

extern "C" IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle* ih, const char* title)
{
  qmlSetAccessibleText(ih, "Accessible.name", title);
}

extern "C" IUP_SDK_API void iupdrvSetAccessibleDescription(Ihandle* ih, const char* description)
{
  qmlSetAccessibleText(ih, "Accessible.description", description);
}
