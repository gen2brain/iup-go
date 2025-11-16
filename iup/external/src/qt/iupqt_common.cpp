/** \file
 * \brief Qt Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>

#include <QWidget>
#include <QLayout>
#include <QWindow>
#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleOption>
#include <QMouseEvent>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QKeyEvent>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QEnterEvent>
#endif
#include <QHelpEvent>
#include <QString>
#include <QColor>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QCursor>
#include <QMouseEvent>
#include <QMenuBar>
#include <QMainWindow>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"
#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_focus.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_assert.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Native Container (for absolute positioning like GTK's GtkFixed)
 ****************************************************************************/

/* Qt doesn't have a native "fixed" container like GTK, so we use QWidget with manual layout management */

extern "C" QWidget* iupqtNativeContainerNew(int has_window)
{
  QWidget* widget = new QWidget();

  if (has_window)
  {
    widget->setAttribute(Qt::WA_NativeWindow, true);
  }

  return widget;
}

extern "C" void iupqtNativeContainerAdd(QWidget* container, QWidget* widget)
{
  if (container && widget)
  {
    widget->setParent(container);
    widget->move(0, 0);
  }
}

extern "C" void iupqtNativeContainerMove(QWidget* container, QWidget* widget, int x, int y)
{
  (void)container;

  if (widget)
    widget->move(x, y);
}

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static QWidget* qtGetNativeParent(Ihandle* ih)
{
  QWidget* widget = (QWidget*)iupChildTreeGetNativeParentHandle(ih);

  while (widget && widget->layout())
    widget = widget->parentWidget();

  return widget;
}

extern "C" const char* iupqtGetWidgetClassName(QWidget* widget)
{
  if (widget)
    return widget->metaObject()->className();
  return "NULL";
}

extern "C" void iupqtUpdateMnemonic(Ihandle* ih)
{
  /* Qt handles mnemonics automatically through & in text */
  (void)ih;
}

/****************************************************************************
 * Driver Base Functions
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvActivate(Ihandle* ih)
{
  QWidget* widget = (QWidget*)ih->handle;

  if (widget)
  {
    QWidget* focus_widget = widget->focusWidget();
    if (focus_widget)
      focus_widget->activateWindow();
  }
}

extern "C" IUP_SDK_API void iupdrvReparent(Ihandle* ih)
{
  QWidget* new_parent = qtGetNativeParent(ih);
  QWidget* widget = (QWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  if (!widget)
    widget = (QWidget*)ih->handle;

  if (widget && new_parent)
  {
    QWidget* old_parent = widget->parentWidget();

    if (old_parent != new_parent)
    {
      widget->setParent(new_parent);
      widget->show();
    }
  }
}

extern "C" IUP_DRV_API void iupqtAddToParent(Ihandle* ih)
{
  QWidget* parent = qtGetNativeParent(ih);
  QWidget* widget = (QWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  if (!widget)
    widget = (QWidget*)ih->handle;

  if (parent && widget)
  {
    /* Special handling for QMenuBar - must use setMenuBar() not container add */
    QMenuBar* menubar = qobject_cast<QMenuBar*>(widget);
    if (menubar)
    {
      /* Walk up parent chain to find QMainWindow.
       * qtGetNativeParent might return the central widget, not the QMainWindow itself.
       * QMenuBar must be set on the QMainWindow, not on its central widget. */
      QWidget* window_parent = parent;
      while (window_parent)
      {
        QMainWindow* mainwindow = qobject_cast<QMainWindow*>(window_parent);
        if (mainwindow)
        {
          mainwindow->setMenuBar(menubar);
          /* Don't call show() here - it's already called in qtMenuMapMethod. */
          menubar->adjustSize();

          return;
        }
        window_parent = window_parent->parentWidget();
      }
    }

    iupqtNativeContainerAdd(parent, widget);
  }
}

extern "C" void iupqtSetPosSize(QWidget* parent, QWidget* widget, int x, int y, int width, int height)
{
  (void)parent;

  if (widget)
  {
    const char* className = widget->metaObject()->className();

    iupqtNativeContainerMove(parent, widget, x, y);

    if (width > 0 && height > 0)
    {
      widget->resize(width, height);

      QListWidget* listWidget = qobject_cast<QListWidget*>(widget);
      if (listWidget)
      {
        QSize viewport = listWidget->viewport()->size();
        QRect contentsRect = listWidget->contentsRect();
        int contentHeight = 0;
        for (int i = 0; i < listWidget->count(); i++)
        {
          QListWidgetItem* item = listWidget->item(i);
          if (item)
            contentHeight += listWidget->visualItemRect(item).height();
        }
      }
    }
  }
}

extern "C" IUP_SDK_API void iupdrvBaseLayoutUpdateMethod(Ihandle *ih)
{
  QWidget* parent = qtGetNativeParent(ih);
  QWidget* widget = (QWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  if (!widget)
    widget = (QWidget*)ih->handle;

  if (widget)
    iupqtSetPosSize(parent, widget, ih->x, ih->y, ih->currentwidth, ih->currentheight);
}

extern "C" IUP_SDK_API void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  /* Skip for types that don't have proper widgets or are managed elsewhere */
  if (ih->iclass->nativetype == IUP_TYPEVOID ||
      ih->iclass->nativetype == IUP_TYPEMENU)
    return;

  QWidget* widget = (QWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  if (!widget)
    widget = (QWidget*)ih->handle;

  if (widget)
  {
    widget->hide();

    /* Only delete if not already being deleted and has no parent
     * (Qt parent-child ownership will delete children automatically) */
    if (!widget->parent())
      delete widget;
    else
      widget->setParent(nullptr);  /* Detach from parent to prevent double-delete */
  }

  ih->handle = nullptr;
}

extern "C" IUP_SDK_API void iupdrvPostRedraw(Ihandle *ih)
{
  QWidget* widget = (QWidget*)ih->handle;

  if (widget)
    widget->update();  /* Schedule repaint */
}

extern "C" IUP_SDK_API void iupdrvRedrawNow(Ihandle *ih)
{
  QWidget* widget = (QWidget*)ih->handle;

  if (widget)
    widget->repaint();  /* Immediate repaint */
}

/****************************************************************************
 * Screen/Client Coordinate Conversion
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvScreenToClient(Ihandle* ih, int *x, int *y)
{
  QWidget* widget = (QWidget*)ih->handle;

  if (widget)
  {
    QPoint global_pos(*x, *y);
    QPoint local_pos = widget->mapFromGlobal(global_pos);

    *x = local_pos.x();
    *y = local_pos.y();
  }
}

extern "C" IUP_SDK_API void iupdrvClientToScreen(Ihandle* ih, int *x, int *y)
{
  QWidget* widget = (QWidget*)ih->handle;

  if (widget)
  {
    QPoint local_pos(*x, *y);
    QPoint global_pos = widget->mapToGlobal(local_pos);

    *x = global_pos.x();
    *y = global_pos.y();
  }
}

/****************************************************************************
 * Event Handlers
 ****************************************************************************/

extern "C" IUP_DRV_API int iupqtEnterLeaveEvent(QWidget *widget, QEvent *evt, Ihandle* ih)
{
  Icallback cb = NULL;
  (void)widget;

  if (evt->type() == QEvent::Enter)
    cb = IupGetCallback(ih, "ENTERWINDOW_CB");
  else if (evt->type() == QEvent::Leave)
    cb = IupGetCallback(ih, "LEAVEWINDOW_CB");

  if (cb)
    cb(ih);

  return 0;
}

extern "C" int iupqtSetMnemonicTitle(Ihandle* ih, QWidget* widget, const char* value)
{
  char c = '&';
  char* str;

  if (!value)
    value = "";

  /* IUP uses & for mnemonics, Qt also uses &, so just pass through */
  str = iupStrProcessMnemonic(value, &c, 1);

  if (str != value)
  {
    /* Has mnemonic */
    QString text = QString::fromUtf8(str);

    /* Set text with mnemonic on appropriate widget type */
    if (qobject_cast<QPushButton*>(widget))
      qobject_cast<QPushButton*>(widget)->setText(text);
    else if (qobject_cast<QLabel*>(widget))
      qobject_cast<QLabel*>(widget)->setText(text);

    free(str);
    return 1;
  }
  else
  {
    /* No mnemonic */
    QString text = QString::fromUtf8(str);

    if (qobject_cast<QPushButton*>(widget))
      qobject_cast<QPushButton*>(widget)->setText(text);
    else if (qobject_cast<QLabel*>(widget))
      qobject_cast<QLabel*>(widget)->setText(text);
  }

  return 0;
}

extern "C" IUP_SDK_API int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  QWidget* widget = (QWidget*)ih->handle;

  if (widget && iupdrvIsVisible(ih))
  {
    if (iupStrEqualNoCase(value, "TOP"))
      widget->raise();
    else
      widget->lower();
  }

  return 0;
}

/****************************************************************************
 * Visibility and Active State
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvSetVisible(Ihandle* ih, int visible)
{
  /* Skip for types that don't have a QWidget handle (TYPEVOID, TYPEMENU) */
  if (ih->iclass->nativetype == IUP_TYPEVOID || ih->iclass->nativetype == IUP_TYPEMENU)
    return;

  QWidget* container = (QWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  QWidget* widget = (QWidget*)ih->handle;

  if (visible)
  {
    if (container) container->show();
    if (widget) widget->show();
  }
  else
  {
    if (container) container->hide();
    if (widget) widget->hide();
  }
}

extern "C" int iupqtIsVisible(QWidget* widget)
{
  return widget ? widget->isVisible() : 0;
}

extern "C" IUP_SDK_API int iupdrvIsVisible(Ihandle* ih)
{
  /* Skip for types that don't have a QWidget handle (TYPEVOID, TYPEMENU) */
  if (ih->iclass->nativetype == IUP_TYPEVOID || ih->iclass->nativetype == IUP_TYPEMENU)
    return 1;  /* Return 1 (visible) for non-widget types */

  QWidget* widget = (QWidget*)ih->handle;

  if (!widget)
    return 0;

  if (widget->isVisible())
  {
    /* Check parents too */
    Ihandle* parent = ih->parent;
    while (parent)
    {
      if (parent->iclass->nativetype != IUP_TYPEVOID)
      {
        QWidget* parent_widget = (QWidget*)parent->handle;
        if (parent_widget && !parent_widget->isVisible())
          return 0;
      }

      parent = parent->parent;
    }
    return 1;
  }

  return 0;
}

extern "C" IUP_SDK_API int iupdrvIsActive(Ihandle *ih)
{
  /* Skip for types that don't have a QWidget handle (TYPEVOID, TYPEMENU) */
  if (ih->iclass->nativetype == IUP_TYPEVOID || ih->iclass->nativetype == IUP_TYPEMENU)
    return 1;  /* Return 1 (active) for non-widget types */

  QWidget* widget = (QWidget*)ih->handle;
  return widget ? widget->isEnabled() : 0;
}

extern "C" IUP_SDK_API void iupdrvSetActive(Ihandle* ih, int enable)
{
  /* Skip for types that don't have a QWidget handle (TYPEVOID, TYPEMENU) */
  if (ih->iclass->nativetype == IUP_TYPEVOID || ih->iclass->nativetype == IUP_TYPEMENU)
    return;

  QWidget* container = (QWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  QWidget* widget = (QWidget*)ih->handle;

  if (container)
    container->setEnabled(enable);

  if (widget)
    widget->setEnabled(enable);
}

/****************************************************************************
 * Color Management
 ****************************************************************************/

static QColor qtDarkerColor(const QColor& color)
{
  return QColor(
    (color.red() * 9) / 10,
    (color.green() * 9) / 10,
    (color.blue() * 9) / 10
  );
}

static QColor qtLighterColor(const QColor& color)
{
  int r = qMin(255, (color.red() * 11) / 10);
  int g = qMin(255, (color.green() * 11) / 10);
  int b = qMin(255, (color.blue() * 11) / 10);

  return QColor(r, g, b);
}

extern "C" void iupqtColorSetRGB(QColor* color, unsigned char r, unsigned char g, unsigned char b)
{
  if (color)
    color->setRgb(r, g, b);
}

extern "C" void iupqtSetBgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b)
{
  QWidget* widget = (QWidget*)handle;

  if (!widget)
    return;

  QColor color(r, g, b);

  QPalette palette = widget->palette();
  palette.setColor(QPalette::Window, color);
  palette.setColor(QPalette::Base, color);
  palette.setColor(QPalette::Light, qtLighterColor(color));
  palette.setColor(QPalette::Dark, qtDarkerColor(color));

  widget->setPalette(palette);
  widget->setAutoFillBackground(true);
}

extern "C" void iupqtSetFgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b)
{
  QWidget* widget = (QWidget*)handle;

  if (!widget)
    return;

  QColor color(r, g, b);

  QPalette palette = widget->palette();
  palette.setColor(QPalette::WindowText, color);
  palette.setColor(QPalette::Text, color);
  palette.setColor(QPalette::ButtonText, color);

  widget->setPalette(palette);
}

/****************************************************************************
 * Window/Widget Utilities
 ****************************************************************************/

extern "C" void* iupqtGetWindow(QWidget *widget)
{
  if (!widget)
    return NULL;

  QWindow* window = widget->windowHandle();
  return window;
}

extern "C" void iupqtWindowGetPointer(void *window, int *x, int *y, int *mask)
{
  (void)window;
  (void)mask;

  QPoint pos = QCursor::pos();

  if (x) *x = pos.x();
  if (y) *y = pos.y();
}

extern "C" void iupqtSetMargin(QWidget* widget, int horiz_padding, int vert_padding)
{
  if (widget)
  {
    widget->setContentsMargins(horiz_padding, vert_padding, horiz_padding, vert_padding);
  }
}

/****************************************************************************
 * Mouse Event Handlers
 ****************************************************************************/

extern "C" IUP_DRV_API int iupqtMouseMoveEvent(QWidget *widget, QEvent *evt, Ihandle *ih)
{
  IFniis cb;
  (void)widget;

  QMouseEvent* mouse_evt = static_cast<QMouseEvent*>(evt);

  cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
  if (cb)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    /* Use buttons() to get the state of all pressed buttons during mouse move */
    iupqtButtonKeySetStatus(mouse_evt->modifiers(), mouse_evt->buttons(), 0, status, 0);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    cb(ih, (int)mouse_evt->position().x(), (int)mouse_evt->position().y(), status);
#else
    cb(ih, (int)mouse_evt->x(), (int)mouse_evt->y(), status);
#endif
  }

  return 0;
}

extern "C" IUP_DRV_API int iupqtMouseButtonEvent(QWidget *widget, QEvent *evt, Ihandle *ih)
{
  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb)
  {
    QMouseEvent* mouse_evt = static_cast<QMouseEvent*>(evt);
    int doubleclick = 0, ret, press = 1;
    int button = IUP_BUTTON1;
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

    if (evt->type() == QEvent::MouseButtonRelease)
      press = 0;

    if (evt->type() == QEvent::MouseButtonDblClick)
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

    iupqtButtonKeySetStatus(mouse_evt->modifiers(), mouse_evt->button(), button, status, doubleclick);

    /* Handle double click like GTK (send release before double click) */
    if (doubleclick)
    {
      status[5] = ' '; /* clear double click */

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      ret = cb(ih, button, 0, (int)mouse_evt->position().x(), (int)mouse_evt->position().y(), status);  /* release */
#else
      ret = cb(ih, button, 0, (int)mouse_evt->x(), (int)mouse_evt->y(), status);  /* release */
#endif
      if (ret == IUP_CLOSE)
        IupExitLoop();
      else if (ret == IUP_IGNORE)
        return 1;

      status[5] = 'D'; /* restore double click */
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    ret = cb(ih, button, press, (int)mouse_evt->position().x(), (int)mouse_evt->position().y(), status);
#else
    ret = cb(ih, button, press, (int)mouse_evt->x(), (int)mouse_evt->y(), status);
#endif
    if (ret == IUP_CLOSE)
      IupExitLoop();
    else if (ret == IUP_IGNORE)
      return 1;
  }

  (void)widget;
  return 0;
}

/****************************************************************************
 * Cursor Management
 ****************************************************************************/

static Qt::CursorShape qtGetCursorShape(const char* name)
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
      return table[i].qtshape;
  }

  return Qt::ArrowCursor;  /* Default */
}

extern "C" IUP_SDK_API int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value)
    return 0;

  QWidget* widget = (QWidget*)ih->handle;
  Qt::CursorShape shape = qtGetCursorShape(value);

  widget->setCursor(QCursor(shape));

  return 1;
}

extern "C" IUP_SDK_API int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value)
    return 0;

  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  QWidget* widget = (QWidget*)ih->handle;
  QPalette palette = widget->palette();
  palette.setColor(QPalette::Window, QColor(r, g, b));
  widget->setPalette(palette);
  widget->setAutoFillBackground(true);

  return 1;
}

extern "C" IUP_SDK_API int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value)
    return 0;

  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  QWidget* widget = (QWidget*)ih->handle;
  QPalette palette = widget->palette();
  palette.setColor(QPalette::WindowText, QColor(r, g, b));
  palette.setColor(QPalette::ButtonText, QColor(r, g, b));
  palette.setColor(QPalette::Text, QColor(r, g, b));
  widget->setPalette(palette);

  return 1;
}

extern "C" IUP_SDK_API void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
  /* Qt-specific common attributes can be registered here */
  (void)ic;
}

extern "C" IUP_SDK_API void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
  /* Qt-specific visual attributes can be registered here */
  (void)ic;
}

/****************************************************************************
 * Scrollbar and Menu
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvGetScrollbarSize(void)
{
  QStyle* style = QApplication::style();
  if (style)
  {
    int size = style->pixelMetric(QStyle::PM_ScrollBarExtent);
    return size > 0 ? size : 16;  /* Default fallback */
  }

  return 16;
}
