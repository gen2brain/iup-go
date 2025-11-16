/** \file
 * \brief IupDialog - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QMainWindow>
#include <QDialog>
#include <QWindow>
#include <QScreen>
#include <QApplication>
#include <QGuiApplication>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QWindowStateChangeEvent>
#include <QString>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QIcon>
#include <QPixmap>
#include <QBitmap>
#include <QPainterPath>
#include <QRegion>
#include <QStyle>
#include <QStyleHints>
#include <QPalette>
#include <QTimer>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <climits>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_class.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_dlglist.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_focus.h"
#include "iup_str.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"
#include "iup_image.h"
#include "iup_assert.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Qt Dialog Widget with Event Handling
 ****************************************************************************/

class IupQtDialog : public QMainWindow
{
public:
  Ihandle* iup_handle;

  IupQtDialog(Ihandle* ih) : QMainWindow(nullptr), iup_handle(ih)
  {
    /* Initialize with proper window flags */
    updateWindowFlags();
  }

  void updateWindowFlags()
  {
    if (!iup_handle)
      return;

    Qt::WindowFlags flags = Qt::Window;

    /* Check for BORDER/title bar */
    int has_titlebar = iupAttribGet(iup_handle, "TITLE") ||
                       iupAttribGetBoolean(iup_handle, "RESIZE") ||
                       iupAttribGetBoolean(iup_handle, "MAXBOX") ||
                       iupAttribGetBoolean(iup_handle, "MINBOX") ||
                       iupAttribGetBoolean(iup_handle, "MENUBOX");

    if (!has_titlebar && !iupAttribGetBoolean(iup_handle, "BORDER"))
      flags |= Qt::FramelessWindowHint;

    /* Window buttons */
    if (!iupAttribGetBoolean(iup_handle, "MENUBOX"))
      flags |= Qt::WindowSystemMenuHint;
    else
      flags |= Qt::WindowCloseButtonHint;

    if (iupAttribGetBoolean(iup_handle, "MINBOX"))
      flags |= Qt::WindowMinimizeButtonHint;

    if (iupAttribGetBoolean(iup_handle, "MAXBOX"))
      flags |= Qt::WindowMaximizeButtonHint;

    /* Other hints */
    if (iupAttribGetBoolean(iup_handle, "DIALOGHINT"))
      flags |= Qt::Dialog;

    if (iupAttribGetBoolean(iup_handle, "TOOLBOX"))
      flags |= Qt::Tool;

    if (iupAttribGetBoolean(iup_handle, "HIDETITLEBAR"))
      flags |= Qt::FramelessWindowHint;

    setWindowFlags(flags);
  }

protected:
  void closeEvent(QCloseEvent* event) override
  {
    if (iupqtDialogCloseEvent(this, (QEvent*)event, iup_handle))
      event->ignore();
    else
      event->accept();
  }

  void resizeEvent(QResizeEvent* event) override
  {
    QMainWindow::resizeEvent(event);

    if (!iup_handle) {
      return;
    }

    if (iup_handle->data->ignore_resize) {
      return;
    }

    /* Don't update size tracking until after first show is complete.
     * Skip if first_show has not been set yet (0), or if it's currently being shown (1).
     * Only allow updates after first_show completes and gets cleared. */
    if (iup_handle->data->first_show || !iupdrvDialogIsVisible(iup_handle)) {
      return;
    }

    /* Update IUP size tracking */
    int border = 0, caption = 0, menu = 0;
    iupdrvDialogGetDecoration(iup_handle, &border, &caption, &menu);

    int new_width = event->size().width() + 2*border;
    int new_height = event->size().height() + 2*border + caption;

    if (iup_handle->currentwidth == new_width && iup_handle->currentheight == new_height) {
      return;
    }

    iup_handle->currentwidth = new_width;
    iup_handle->currentheight = new_height;

    /* Call RESIZE_CB */
    IFnii cb = (IFnii)IupGetCallback(iup_handle, "RESIZE_CB");
    if (!cb || cb(iup_handle, event->size().width(), event->size().height() - menu) != IUP_IGNORE)
    {
      iup_handle->data->ignore_resize = 1;
      IupRefresh(iup_handle);
      iup_handle->data->ignore_resize = 0;
    }
  }

  void moveEvent(QMoveEvent* event) override
  {
    QMainWindow::moveEvent(event);

    if (!iup_handle)
      return;

    /* Call MOVE_CB */
    IFnii cb = (IFnii)IupGetCallback(iup_handle, "MOVE_CB");
    if (cb)
    {
      cb(iup_handle, event->pos().x(), event->pos().y());
    }

    /* Save position for retrieval when hidden */
    iupAttribSetInt(iup_handle, "_IUPQT_OLD_X", event->pos().x());
    iupAttribSetInt(iup_handle, "_IUPQT_OLD_Y", event->pos().y());
  }

  void changeEvent(QEvent* event) override
  {
    QMainWindow::changeEvent(event);

    if (!iup_handle)
      return;

    if (event->type() == QEvent::WindowStateChange)
    {
      QWindowStateChangeEvent* state_event = static_cast<QWindowStateChangeEvent*>(event);
      Qt::WindowStates old_state = state_event->oldState();
      Qt::WindowStates new_state = windowState();

      int iup_state = -1;

      if (new_state & Qt::WindowMinimized)
        iup_state = IUP_MINIMIZE;
      else if (new_state & Qt::WindowMaximized)
        iup_state = IUP_MAXIMIZE;
      else if (new_state & Qt::WindowFullScreen)
        iup_state = IUP_MAXIMIZE;  /* Treat fullscreen as maximize */
      else
        iup_state = IUP_RESTORE;

      if (iup_state >= 0 && iup_handle->data->show_state != iup_state)
      {
        IFni cb = (IFni)IupGetCallback(iup_handle, "SHOW_CB");
        iup_handle->data->show_state = iup_state;
        if (cb && cb(iup_handle, iup_state) == IUP_CLOSE)
          IupExitLoop();
      }
    }
    else if (event->type() == QEvent::ActivationChange)
    {
      if (isActiveWindow())
      {
        /* Focus handling */
        iupqtDialogSetFocus(iup_handle);
      }
    }
  }

  void keyPressEvent(QKeyEvent* event) override
  {
    if (!iup_handle)
    {
      QMainWindow::keyPressEvent(event);
      return;
    }

    /* Handle DEFAULTENTER and DEFAULTESC */
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
      Ihandle* button_ih = IupGetAttributeHandle(iup_handle, "DEFAULTENTER");
      if (iupObjectCheck(button_ih))
      {
        iupdrvActivate(button_ih);
        event->accept();
        return;
      }
    }
    else if (event->key() == Qt::Key_Escape)
    {
      Ihandle* button_ih = IupGetAttributeHandle(iup_handle, "DEFAULTESC");
      if (iupObjectCheck(button_ih))
      {
        iupdrvActivate(button_ih);
        event->accept();
        return;
      }
    }

    /* Let key event propagate to focused widget */
    if (iupqtKeyPressEvent(this, event, iup_handle))
    {
      event->accept();
      return;
    }

    QMainWindow::keyPressEvent(event);
  }

  bool event(QEvent* event) override
  {
    /* Monitor theme changes via palette change */
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
    {
      if (iup_handle)
      {
        int dark_mode = iupqtIsSystemDarkMode();
        IFni cb = (IFni)IupGetCallback(iup_handle, "THEMECHANGED_CB");
        if (cb)
          cb(iup_handle, dark_mode);
      }
    }

    return QMainWindow::event(event);
  }
};

/****************************************************************************
 * Dialog Event Handlers
 ****************************************************************************/

extern "C" int iupqtDialogCloseEvent(QWidget *widget, QEvent *evt, Ihandle *ih)
{
  Icallback cb;
  (void)widget;
  (void)evt;

  /* Even when ACTIVE=NO the dialog gets this event */
  if (!iupdrvIsActive(ih))
    return 1;  /* Ignore close */

  cb = IupGetCallback(ih, "CLOSE_CB");
  if (cb)
  {
    int ret = cb(ih);
    if (ret == IUP_IGNORE)
      return 1;  /* Prevent close */
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  IupHide(ih); /* Default: close the window */

  return 0;  /* Allow close */
}

/****************************************************************************
 * Dialog Utilities
 ****************************************************************************/

extern "C" void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* parent)
{
  QWidget* dialog = (QWidget*)ih->handle;
  QWidget* parent_widget = (QWidget*)parent;

  if (dialog && parent_widget)
  {
    /* Set transient relationship */
    QWindow* dialog_window = dialog->windowHandle();
    QWindow* parent_window = parent_widget->windowHandle();

    if (dialog_window && parent_window)
    {
      dialog_window->setTransientParent(parent_window);
    }
  }
}

extern "C" int iupdrvDialogIsVisible(Ihandle* ih)
{
  return iupdrvIsVisible(ih);
}

extern "C" void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int *w, int *h)
{
  QWidget* widget;
  int border = 0, caption = 0, menu = 0;

  if (!handle)
    handle = ih->handle;

  widget = (QWidget*)handle;

  if (widget)
  {
    QSize size = widget->size();

    if (ih)
      iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    int result_w = size.width() + 2*border;
    int result_h = size.height() + 2*border + caption;

    if (w) *w = result_w;
    if (h) *h = result_h;
  }
}

extern "C" void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  QWidget* widget = (QWidget*)ih->handle;

  if (widget)
  {
    if (visible)
    {
      if (iupAttribGetBoolean(ih, "SHOWNOACTIVATE"))
        widget->show();
      else
      {
        widget->show();
        widget->activateWindow();
        widget->raise();
      }
    }
    else
    {
      widget->hide();
    }
  }
}

extern "C" void iupdrvDialogGetPosition(Ihandle *ih, InativeHandle* handle, int *x, int *y)
{
  QWidget* widget;

  if (!handle)
    handle = ih->handle;

  widget = (QWidget*)handle;

  if (widget && widget->isVisible())
  {
    QPoint pos = widget->pos();
    if (x) *x = pos.x();
    if (y) *y = pos.y();
  }
  else if (ih)
  {
    /* Return saved position if window is not visible */
    if (x) *x = iupAttribGetInt(ih, "_IUPQT_OLD_X");
    if (y) *y = iupAttribGetInt(ih, "_IUPQT_OLD_Y");
  }
}

extern "C" void iupdrvDialogSetPosition(Ihandle *ih, int x, int y)
{
  QWidget* widget = (QWidget*)ih->handle;

  if (widget)
  {
    widget->move(x, y);

    /* Save position for later retrieval when hidden */
    iupAttribSetInt(ih, "_IUPQT_OLD_X", x);
    iupAttribSetInt(ih, "_IUPQT_OLD_Y", y);
  }
}

static int qtDialogGetMenuSize(Ihandle* ih)
{
  if (ih->data->menu)
    return iupdrvMenuGetMenuBarSize(ih->data->menu);
  else
    return 0;
}

extern "C" void iupdrvDialogGetDecoration(Ihandle* ih, int *border, int *caption, int *menu)
{
  int native_border = iupAttribGetInt(ih, "_IUPQT_NATIVE_BORDER");
  int native_caption = iupAttribGetInt(ih, "_IUPQT_NATIVE_CAPTION");

  int has_titlebar = iupAttribGetBoolean(ih, "RESIZE")  ||
                     iupAttribGetBoolean(ih, "MAXBOX")  ||
                     iupAttribGetBoolean(ih, "MINBOX")  ||
                     iupAttribGetBoolean(ih, "MENUBOX") ||
                     iupAttribGet(ih, "TITLE");

  int has_border = has_titlebar ||
                   iupAttribGetBoolean(ih, "RESIZE") ||
                   iupAttribGetBoolean(ih, "BORDER");

  *menu = qtDialogGetMenuSize(ih);

  /* If we have cached values, prefer them to avoid race conditions during window resize.
   * Only query actual window decorations if we don't have cached values yet. */
  if (native_border > 0 && native_caption > 0)
  {
    *border = has_border ? native_border : 0;
    *caption = has_titlebar ? native_caption : 0;

    if (iupAttribGetBoolean(ih, "HIDETITLEBAR"))
      *caption = 0;

    return;
  }

  /* Try to get actual decorations from window (only on first call) */
  if (ih->handle && iupdrvDialogIsVisible(ih))
  {
    QWidget* widget = (QWidget*)ih->handle;
    QWindow* window = widget->windowHandle();

    if (window)
    {
      /* Get window geometry vs frame geometry to calculate decorations */
      QRect frame_geom = window->frameGeometry();
      QRect window_geom = window->geometry();

      /* Qt may report identical frame and window geometry when window is first shown
       * This means decorations aren't ready yet - fall through to use estimates */
      if (frame_geom.width() != window_geom.width() || frame_geom.height() != window_geom.height())
      {
        int win_border = (frame_geom.width() - window_geom.width()) / 2;
        int win_caption = frame_geom.height() - window_geom.height() - win_border;

        /* Sanity check: reasonable decoration sizes */
        if (win_border >= 0 && win_border < 100 && win_caption >= 0 && win_caption < 200)
        {
          *border = has_border ? win_border : 0;
          *caption = has_titlebar ? win_caption : 0;

          /* Cache values for later use - only cache valid non-zero values */
          if (win_border > 0 && win_border < 100)
            iupAttribSetInt(ih, "_IUPQT_NATIVE_BORDER", win_border);
          if (win_caption > 0 && win_caption < 200)
            iupAttribSetInt(ih, "_IUPQT_NATIVE_CAPTION", win_caption);

          if (iupAttribGetBoolean(ih, "HIDETITLEBAR"))
            *caption = 0;

          return;
        }
      }
    }
  }

  /* Estimate when not visible or invalid values */
  *border = 0;
  if (has_border)
    *border = native_border ? native_border : 5;

  *caption = 0;
  if (has_titlebar)
    *caption = native_caption ? native_caption : 25;

  if (iupAttribGetBoolean(ih, "HIDETITLEBAR"))
    *caption = 0;
}

extern "C" void qtDialogUpdateSize(Ihandle* ih)
{
  QWidget* widget = (QWidget*)ih->handle;

  if (widget)
  {
    int width = ih->currentwidth;
    int height = ih->currentheight;

    /* Account for decorations */
    int border = 0, caption = 0, menu = 0;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    width -= 2*border;
    height -= 2*border + caption;

    if (width < 1) width = 1;
    if (height < 1) height = 1;

    widget->resize(width, height);
  }
}

extern "C" void qtDialogGetClientSize(Ihandle* ih, int *width, int *height)
{
  QWidget* widget = (QWidget*)ih->handle;

  if (widget)
  {
    QSize size = widget->size();

    int menu = qtDialogGetMenuSize(ih);

    if (width)  *width = size.width();
    if (height) *height = size.height() - menu;
  }
}

extern "C" int iupdrvDialogSetPlacement(Ihandle* ih)
{
  QWidget* widget = (QWidget*)ih->handle;
  if (!widget)
    return 0;

  char* placement;
  int old_state = ih->data->show_state;

  ih->data->show_state = IUP_SHOW;

  if (iupAttribGetBoolean(ih, "FULLSCREEN"))
  {
    widget->setWindowState(widget->windowState() | Qt::WindowFullScreen);
    return 1;
  }

  placement = iupAttribGet(ih, "PLACEMENT");
  if (!placement)
  {
    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;

    widget->setWindowState(widget->windowState() & ~(Qt::WindowMaximized | Qt::WindowMinimized | Qt::WindowFullScreen));

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
    iupAttribSet(ih, "PLACEMENT", NULL);
    ih->data->show_state = IUP_MAXIMIZE;
    return 1;
  }

  if (iupStrEqualNoCase(placement, "MINIMIZED"))
  {
    ih->data->show_state = IUP_MINIMIZE;
    widget->setWindowState(widget->windowState() | Qt::WindowMinimized);
  }
  else if (iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    ih->data->show_state = IUP_MAXIMIZE;
    widget->setWindowState(widget->windowState() | Qt::WindowMaximized);
  }
  else if (iupStrEqualNoCase(placement, "FULL"))
  {
    int width, height, x, y;
    int border, caption, menu;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    /* Position the decoration outside the screen */
    x = -(border);
    y = -(border+caption+menu);

    /* The dialog client area will cover the task bar */
    iupdrvGetFullSize(&width, &height);

    height += menu; /* menu is inside the client area */

    /* Set the new size and position */
    iupdrvDialogSetPosition(ih, x, y);
    widget->resize(width, height);

    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;
  }

  iupAttribSet(ih, "PLACEMENT", NULL);
  return 1;
}

/****************************************************************************
 * Dialog Attributes
 ****************************************************************************/

static void qtDialogSetMinMax(Ihandle* ih, int min_w, int min_h, int max_w, int max_h)
{
  QWidget* widget = (QWidget*)ih->handle;
  if (!widget)
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

  QSize maxSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
  if (max_w > decorwidth && max_w > minSize.width())
    maxSize.setWidth(max_w - decorwidth);
  if (max_h > decorheight && max_h > minSize.height())
    maxSize.setHeight(max_h - decorheight);

  widget->setMinimumSize(minSize);
  widget->setMaximumSize(maxSize);
}

static int qtDialogSetMinSizeAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return iupBaseSetMinSizeAttrib(ih, value);

  int min_w = 1, min_h = 1;
  int max_w = 65535, max_h = 65535;
  iupStrToIntInt(value, &min_w, &min_h, 'x');

  iupStrToIntInt(iupAttribGet(ih, "MAXSIZE"), &max_w, &max_h, 'x');

  qtDialogSetMinMax(ih, min_w, min_h, max_w, max_h);

  return iupBaseSetMinSizeAttrib(ih, value);
}

static int qtDialogSetMaxSizeAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return iupBaseSetMaxSizeAttrib(ih, value);

  int min_w = 1, min_h = 1;
  int max_w = 65535, max_h = 65535;
  iupStrToIntInt(value, &max_w, &max_h, 'x');

  iupStrToIntInt(iupAttribGet(ih, "MINSIZE"), &min_w, &min_h, 'x');

  qtDialogSetMinMax(ih, min_w, min_h, max_w, max_h);

  return iupBaseSetMaxSizeAttrib(ih, value);
}

static char* qtDialogGetClientSizeAttrib(Ihandle *ih)
{
  if (ih->handle)
  {
    int width, height;
    qtDialogGetClientSize(ih, &width, &height);
    return iupStrReturnIntInt(width, height, 'x');
  }

  return iupDialogGetClientSizeAttrib(ih);
}

static char* qtDialogGetClientOffsetAttrib(Ihandle *ih)
{
  /* In Qt, menu is part of the window, not client area */
  return "0x0";
}

static char* qtDialogGetResizeAttrib(Ihandle* ih)
{
  QWidget* widget = (QWidget*)ih->handle;
  if (!widget)
    return iupAttribGet(ih, "RESIZE");

  Qt::WindowFlags flags = widget->windowFlags();
  return iupStrReturnBoolean(flags & Qt::WindowMaximizeButtonHint);
}

static int qtDialogSetResizeAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    IupQtDialog* dialog = (IupQtDialog*)ih->handle;
    dialog->updateWindowFlags();
  }
  return 1;
}

static int qtDialogSetMinBoxAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    IupQtDialog* dialog = (IupQtDialog*)ih->handle;
    dialog->updateWindowFlags();
  }
  return 1;
}

static int qtDialogSetMaxBoxAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    IupQtDialog* dialog = (IupQtDialog*)ih->handle;
    dialog->updateWindowFlags();
  }
  return 1;
}

static int qtDialogSetMenuBoxAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    IupQtDialog* dialog = (IupQtDialog*)ih->handle;
    dialog->updateWindowFlags();
  }
  return 1;
}

static int qtDialogSetBorderAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    IupQtDialog* dialog = (IupQtDialog*)ih->handle;
    dialog->updateWindowFlags();
  }
  return 1;
}

static int qtDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
  QWidget* widget = (QWidget*)ih->handle;
  if (widget)
  {
    QString title = value ? QString::fromUtf8(value) : QString();
    widget->setWindowTitle(title);
  }

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME") || iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
    return 0;

  return 1;
}

static int qtDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{
  QWidget* widget = (QWidget*)ih->handle;
  if (!widget)
    return 0;

  if (iupStrBoolean(value))
  {
    if (!iupAttribGet(ih, "_IUPQT_FS_STYLE"))
    {
      /* Save current state */
      iupAttribSetStr(ih, "_IUPQT_FS_MAXBOX", iupAttribGet(ih, "MAXBOX"));
      iupAttribSetStr(ih, "_IUPQT_FS_MINBOX", iupAttribGet(ih, "MINBOX"));
      iupAttribSetStr(ih, "_IUPQT_FS_MENUBOX", iupAttribGet(ih, "MENUBOX"));
      iupAttribSetStr(ih, "_IUPQT_FS_RESIZE", iupAttribGet(ih, "RESIZE"));
      iupAttribSetStr(ih, "_IUPQT_FS_BORDER", iupAttribGet(ih, "BORDER"));
      iupAttribSetStr(ih, "_IUPQT_FS_TITLE", iupAttribGet(ih, "TITLE"));

      /* Remove decorations */
      iupAttribSet(ih, "MAXBOX", "NO");
      iupAttribSet(ih, "MINBOX", "NO");
      iupAttribSet(ih, "MENUBOX", "NO");
      IupSetAttribute(ih, "TITLE", NULL);
      iupAttribSet(ih, "RESIZE", "NO");
      iupAttribSet(ih, "BORDER", "NO");

      widget->setWindowState(widget->windowState() | Qt::WindowFullScreen);

      iupAttribSet(ih, "_IUPQT_FS_STYLE", "YES");
    }
  }
  else
  {
    if (iupAttribGet(ih, "_IUPQT_FS_STYLE"))
    {
      iupAttribSet(ih, "_IUPQT_FS_STYLE", NULL);

      /* Restore decorations */
      iupAttribSetStr(ih, "MAXBOX", iupAttribGet(ih, "_IUPQT_FS_MAXBOX"));
      iupAttribSetStr(ih, "MINBOX", iupAttribGet(ih, "_IUPQT_FS_MINBOX"));
      iupAttribSetStr(ih, "MENUBOX", iupAttribGet(ih, "_IUPQT_FS_MENUBOX"));
      IupSetAttribute(ih, "TITLE", iupAttribGet(ih, "_IUPQT_FS_TITLE"));
      iupAttribSetStr(ih, "RESIZE", iupAttribGet(ih, "_IUPQT_FS_RESIZE"));
      iupAttribSetStr(ih, "BORDER", iupAttribGet(ih, "_IUPQT_FS_BORDER"));

      widget->setWindowState(widget->windowState() & ~Qt::WindowFullScreen);

      /* Clean up */
      iupAttribSet(ih, "_IUPQT_FS_MAXBOX", NULL);
      iupAttribSet(ih, "_IUPQT_FS_MINBOX", NULL);
      iupAttribSet(ih, "_IUPQT_FS_MENUBOX", NULL);
      iupAttribSet(ih, "_IUPQT_FS_TITLE", NULL);
      iupAttribSet(ih, "_IUPQT_FS_RESIZE", NULL);
      iupAttribSet(ih, "_IUPQT_FS_BORDER", NULL);
    }
  }

  return 1;
}

static int qtDialogSetDialogHintAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    IupQtDialog* dialog = (IupQtDialog*)ih->handle;
    dialog->updateWindowFlags();
  }
  return 1;
}

static int qtDialogSetHideTitleBarAttrib(Ihandle *ih, const char *value)
{
  if (ih->handle)
  {
    IupQtDialog* dialog = (IupQtDialog*)ih->handle;
    dialog->updateWindowFlags();
  }
  return 1;
}

static char* qtDialogGetActiveWindowAttrib(Ihandle* ih)
{
  QWidget* widget = (QWidget*)ih->handle;
  if (!widget)
    return NULL;

  return iupStrReturnBoolean(widget->isActiveWindow());
}

static int qtDialogSetTopMostAttrib(Ihandle *ih, const char *value)
{
  QWidget* widget = (QWidget*)ih->handle;
  if (!widget)
    return 0;

  Qt::WindowFlags flags = widget->windowFlags();

  if (iupStrBoolean(value))
    flags |= Qt::WindowStaysOnTopHint;
  else
    flags &= ~Qt::WindowStaysOnTopHint;

  widget->setWindowFlags(flags);
  widget->show(); /* Need to show again after changing flags */

  return 1;
}

static int qtDialogSetBringFrontAttrib(Ihandle *ih, const char *value)
{
  if (iupStrBoolean(value))
  {
    QWidget* widget = (QWidget*)ih->handle;
    if (widget)
    {
      widget->raise();
      widget->activateWindow();
    }
  }
  return 0;
}

static int qtDialogSetOpacityAttrib(Ihandle *ih, const char *value)
{
  QWidget* widget = (QWidget*)ih->handle;
  if (!widget)
    return 0;

  int opacity;
  if (iupStrToInt(value, &opacity))
  {
    if (opacity < 0) opacity = 0;
    if (opacity > 255) opacity = 255;
    widget->setWindowOpacity((qreal)opacity / 255.0);
    return 1;
  }
  return 0;
}

static int qtDialogSetIconAttrib(Ihandle* ih, const char *value)
{
  QWidget* widget = (QWidget*)ih->handle;
  if (!widget)
    return 0;

  if (!value)
  {
    widget->setWindowIcon(QIcon());
  }
  else
  {
    QPixmap* pixmap = (QPixmap*)iupImageGetIcon(value);
    if (pixmap)
    {
      QIcon icon(*pixmap);
      widget->setWindowIcon(icon);
      return 1;
    }
  }

  return 0;
}

static int qtDialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  QWidget* widget = (QWidget*)ih->handle;
  if (!widget)
    return 0;

  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    QPalette palette = widget->palette();
    palette.setColor(QPalette::Window, QColor(r, g, b));
    widget->setPalette(palette);
    widget->setAutoFillBackground(true);
    return 1;
  }
  else
  {
    /* Try to use as image */
    QPixmap* pixmap = (QPixmap*)iupImageGetImage(value, ih, 0, NULL);
    if (pixmap)
    {
      QPalette palette = widget->palette();
      palette.setBrush(QPalette::Window, QBrush(*pixmap));
      widget->setPalette(palette);
      widget->setAutoFillBackground(true);
      return 1;
    }
  }

  return 0;
}

static int qtDialogSetShapeImageAttrib(Ihandle *ih, const char *value)
{
  QWidget* widget = (QWidget*)ih->handle;
  if (!widget)
    return 0;

  if (!value)
  {
    widget->clearMask();
    return 1;
  }

  QPixmap* pixmap = (QPixmap*)iupImageGetImage(value, ih, 0, NULL);
  if (pixmap)
  {
    widget->setMask(pixmap->mask());
    return 1;
  }

  return 0;
}

static char* qtDialogGetMaximizedAttrib(Ihandle *ih)
{
  QWidget* widget = (QWidget*)ih->handle;
  if (!widget)
    return NULL;

  return iupStrReturnBoolean(widget->isMaximized());
}

static char* qtDialogGetMinimizedAttrib(Ihandle *ih)
{
  QWidget* widget = (QWidget*)ih->handle;
  if (!widget)
    return NULL;

  return iupStrReturnBoolean(widget->isMinimized());
}

/****************************************************************************
 * Dialog Methods
 ****************************************************************************/

static void qtDialogSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (ih->firstchild)
  {
    char* offset = iupAttribGet(ih, "CHILDOFFSET");

    /* Native container, position is reset */
    x = 0;
    y = 0;

    if (offset) iupStrToIntInt(offset, &x, &y, 'x');

    /* Child coordinates are relative to client left-top corner. */
    if (ih->firstchild)
      iupBaseSetPosition(ih->firstchild, x, y);
  }
}

static void* qtDialogGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  QMainWindow* main_window = (QMainWindow*)ih->handle;
  if (main_window)
    return (void*)main_window->centralWidget();
  return NULL;
}

/* Forward declarations */
extern "C" int qtDialogMapMethod(Ihandle* ih);
extern "C" void qtDialogUnMapMethod(Ihandle* ih);
extern "C" void qtDialogLayoutUpdateMethod(Ihandle *ih);

extern "C" void iupdrvDialogInitClass(Iclass* ic)
{
  /* Driver Dependent Class methods */
  ic->Map = qtDialogMapMethod;
  ic->UnMap = qtDialogUnMapMethod;
  ic->LayoutUpdate = qtDialogLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = qtDialogGetInnerNativeContainerHandleMethod;
  ic->SetChildrenPosition = qtDialogSetChildrenPositionMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "TRAYCLICK_CB", "iii");
  iupClassRegisterCallback(ic, "MOVE_CB", "ii");
  iupClassRegisterCallback(ic, "THEMECHANGED_CB", "i");

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* Base Container */
  iupClassRegisterAttribute(ic, "CLIENTSIZE", qtDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", qtDialogGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* IupDialog */
  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, qtDialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ICON", NULL, qtDialogSetIconAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FULLSCREEN", NULL, qtDialogSetFullScreenAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINSIZE", NULL, qtDialogSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXSIZE", NULL, qtDialogSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, qtDialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESIZE", qtDialogGetResizeAttrib, qtDialogSetResizeAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BORDER", NULL, qtDialogSetBorderAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINBOX", NULL, qtDialogSetMinBoxAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXBOX", NULL, qtDialogSetMaxBoxAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MENUBOX", NULL, qtDialogSetMenuBoxAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  /* IupDialog Windows and GTK Only */
  iupClassRegisterAttribute(ic, "ACTIVEWINDOW", qtDialogGetActiveWindowAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPMOST", NULL, qtDialogSetTopMostAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIALOGHINT", NULL, qtDialogSetDialogHintAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITY", NULL, qtDialogSetOpacityAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITYIMAGE", NULL, qtDialogSetShapeImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHAPEIMAGE", NULL, qtDialogSetShapeImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BRINGFRONT", NULL, qtDialogSetBringFrontAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDETITLEBAR", NULL, qtDialogSetHideTitleBarAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXIMIZED", qtDialogGetMaximizedAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINIMIZED", qtDialogGetMinimizedAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* System Tray Support */
  iupClassRegisterAttribute(ic, "TRAY", NULL, iupqtSetTrayAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYIMAGE", NULL, iupqtSetTrayImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIP", NULL, iupqtSetTrayTipAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYMENU", NULL, iupqtSetTrayMenuAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);

  /* Tray Balloon Notification Support */
  iupClassRegisterAttribute(ic, "TRAYBALLOON", NULL, iupqtSetTrayBalloonAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTITLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYBALLOONINFO", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYBALLOONTIMEOUT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* Native Window Handle (platform-specific: XWINDOW, WL_SURFACE, HWND, NSVIEW) */
  iupClassRegisterAttribute(ic, iupqtGetNativeWindowHandleName(), iupqtGetNativeWindowHandleAttrib, NULL, NULL, NULL, IUPAF_NO_STRING|IUPAF_NO_INHERIT);

  /* IupDialog Windows and GTK Only - not implemented yet */
  iupClassRegisterAttribute(ic, "COMPOSITED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOOLBOX", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HELPBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWNOACTIVATE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUSTOMFRAME", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NO_INHERIT);

  /* Not Supported */
  iupClassRegisterAttribute(ic, "SAVEUNDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTROL", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIFRAME", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICLIENT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIMENU", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICHILD", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}

extern "C" int qtDialogMapMethod(Ihandle* ih)
{
  /* Create Qt dialog widget */
  IupQtDialog* dialog = new IupQtDialog(ih);

  if (!dialog)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)dialog;

  /* Create central widget for content */
  QWidget* central = iupqtNativeContainerNew(0);
  dialog->setCentralWidget(central);

  /* Set initial properties */
  const char* title = iupAttribGetStr(ih, "TITLE");
  if (title)
    dialog->setWindowTitle(QString::fromUtf8(title));

  /* Setup menu if present */
  if (ih->data->menu && !ih->data->menu->handle)
  {
    ih->data->menu->parent = ih;

    IupMap(ih->data->menu);
  }

  /* Configure for DRAG&DROP */
  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  /* Add to parent */
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  if (parent)
    iupdrvDialogSetParent(ih, parent);

  /* Configure initial size range */
  qtDialogSetMinMax(ih, 1, 1, 65535, 65535);

  /* Ignore VISIBLE before mapping */
  iupAttribSet(ih, "VISIBLE", NULL);

  return IUP_NOERROR;
}

extern "C" void qtDialogUnMapMethod(Ihandle* ih)
{
  QWidget* widget = (QWidget*)ih->handle;

  if (widget)
  {
    /* Cleanup menu */
    if (ih->data->menu)
    {
      ih->data->menu->handle = NULL;
      IupDestroy(ih->data->menu);
      ih->data->menu = NULL;
    }

    /* Cleanup tray */
    iupqtTrayCleanup(ih);

    /* Qt will handle widget deletion */
    delete widget;
    ih->handle = NULL;
  }
}

extern "C" void qtDialogLayoutUpdateMethod(Ihandle *ih)
{
  int border, caption, menu;
  int width, height;

  if (ih->data->ignore_resize ||
      iupAttribGet(ih, "_IUPQT_FS_STYLE"))
  {
    return;
  }

  ih->data->ignore_resize = 1;

  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  width = ih->currentwidth - 2*border;
  height = ih->currentheight - 2*border - caption;

  if (width <= 0) width = 1;
  if (height <= 0) height = 1;

  QWidget* widget = (QWidget*)ih->handle;
  if (widget)
  {
    QMainWindow* window = qobject_cast<QMainWindow*>(widget);
    widget->resize(width, height);
  }

  /* Update min/max constraints for non-resizable dialogs */
  if (!iupAttribGetBoolean(ih, "RESIZE"))
  {
    int client_width, client_height;

    client_width = ih->currentwidth - 2*border;
    client_height = ih->currentheight - 2*border - caption;

    if (client_width <= 0) client_width = 1;
    if (client_height <= 0) client_height = 1;

    qtDialogSetMinMax(ih, ih->currentwidth, ih->currentheight,
                      ih->currentwidth, ih->currentheight);
  }

  ih->data->ignore_resize = 0;
}
