/** \file
 * \brief Qt Driver iupdrvSetGlobal/iupdrvGetGlobal
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <QApplication>
#include <QScreen>
#include <QGuiApplication>
#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QString>
#include <QRect>

#ifndef _WIN32
#include <climits>
#include <cstdlib>
#endif

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_key.h"
}

#include "iupqt_drv.h"

extern "C" int iupqtKeyDecode(QEvent *evt);

/****************************************************************************
 * Global Input Event Handler
 ****************************************************************************/

class IupQtEventFilter : public QObject
{
public:
  static IupQtEventFilter* instance()
  {
    static IupQtEventFilter* filter = nullptr;
    if (!filter)
      filter = new IupQtEventFilter();
    return filter;
  }

protected:
  bool eventFilter(QObject *obj, QEvent *event) override
  {
    (void)obj;

    switch(event->type())
    {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonRelease:
      {
        IFiiiis cb = (IFiiiis)IupGetFunction("GLOBALBUTTON_CB");
        if (cb)
        {
          QMouseEvent* mouse_evt = static_cast<QMouseEvent*>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
          int x = mouse_evt->globalPosition().x();
          int y = mouse_evt->globalPosition().y();
#else
          int x = mouse_evt->globalX();
          int y = mouse_evt->globalY();
#endif
          int press = (event->type() != QEvent::MouseButtonRelease) ? 1 : 0;
          int doubleclick = (event->type() == QEvent::MouseButtonDblClick) ? 1 : 0;

          /* Convert Qt button to IUP button */
          int button = IUP_BUTTON1;
          if (mouse_evt->button() == Qt::LeftButton)
            button = IUP_BUTTON1;
          else if (mouse_evt->button() == Qt::MiddleButton)
            button = IUP_BUTTON2;
          else if (mouse_evt->button() == Qt::RightButton)
            button = IUP_BUTTON3;

          /* Build status string */
          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
          iupqtButtonKeySetStatus(mouse_evt->modifiers(), mouse_evt->buttons(), button, status, doubleclick);

          /* Handle double click like GTK (send release before double click) */
          if (doubleclick)
          {
            status[5] = ' '; /* clear double click */
            cb(button, 0, x, y, status);  /* release */
            status[5] = 'D'; /* restore double click */
          }

          cb(button, press, x, y, status);
        }
        break;
      }

    case QEvent::MouseMove:
      {
        IFiis cb = (IFiis)IupGetFunction("GLOBALMOTION_CB");
        if (cb)
        {
          QMouseEvent* mouse_evt = static_cast<QMouseEvent*>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
          int x = mouse_evt->globalPosition().x();
          int y = mouse_evt->globalPosition().y();
#else
          int x = mouse_evt->globalX();
          int y = mouse_evt->globalY();
#endif

          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
          iupqtButtonKeySetStatus(mouse_evt->modifiers(), Qt::NoButton, 0, status, 0);

          cb(x, y, status);
        }
        break;
      }

    case QEvent::Wheel:
      {
        IFfiis cb = (IFfiis)IupGetFunction("GLOBALWHEEL_CB");
        if (cb)
        {
          QWheelEvent* wheel_evt = static_cast<QWheelEvent*>(event);

          /* Get delta (Qt uses 1/8 degree increments, typical mouse wheel is 15 degrees per notch) */
          QPoint angle_delta = wheel_evt->angleDelta();
          float delta = angle_delta.y() / 120.0f;  /* Normalize to notches (120 units per notch) */

          /* Get global position */
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
          QPoint global_pos = wheel_evt->globalPosition().toPoint();
#else
          QPoint global_pos = wheel_evt->globalPos();
#endif

          int x = global_pos.x();
          int y = global_pos.y();

          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
          iupqtButtonKeySetStatus(wheel_evt->modifiers(), Qt::NoButton, 0, status, 0);

          cb(delta, x, y, status);
        }
        break;
      }

    case QEvent::KeyPress:
    case QEvent::KeyRelease:
      {
        IFii cb = (IFii)IupGetFunction("GLOBALKEYPRESS_CB");
        if (cb)
        {
          int pressed = (event->type() == QEvent::KeyPress) ? 1 : 0;
          int code = iupqtKeyDecode(event);

          if (code != 0)
            cb(code, pressed);
        }
        break;
      }

    default:
      break;
    }

    /* Continue processing the event */
    return false;
  }
};

/****************************************************************************
 * Global Set/Get
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvSetGlobal(const char *name, const char *value)
{
  if (iupStrEqual(name, "INPUTCALLBACKS"))
  {
    if (iupStrBoolean(value))
    {
      /* Install global event filter */
      QApplication* app = iupqtGetApplication();
      if (app)
        app->installEventFilter(IupQtEventFilter::instance());
    }
    else
    {
      /* Remove global event filter */
      QApplication* app = iupqtGetApplication();
      if (app)
        app->removeEventFilter(IupQtEventFilter::instance());
    }
    return 1;
  }

  if (iupStrEqual(name, "UTF8MODE"))
  {
    iupqtStrSetUTF8Mode(iupStrBoolean(value));
    return 1;
  }

  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
    iupqtStrSetUTF8Mode(!iupStrBoolean(value));
    return 0;
  }

  if (iupStrEqual(name, "SHOWMENUIMAGES"))
  {
    /* Qt doesn't have a global setting for menu images */
    /* This would need to be handled per-menu if needed */
    return 1;
  }

  return 1;
}

extern "C" IUP_SDK_API char *iupdrvGetGlobal(const char *name)
{
  if (iupStrEqual(name, "VIRTUALSCREEN"))
  {
    /* Get the virtual screen geometry (all monitors combined) */
    QScreen* primary_screen = QGuiApplication::primaryScreen();
    if (primary_screen)
    {
      QRect virtual_geom = primary_screen->virtualGeometry();
      return iupStrReturnStrf("%d %d %d %d",
                             virtual_geom.x(), virtual_geom.y(),
                             virtual_geom.width(), virtual_geom.height());
    }
    return iupStrReturnStrf("0 0 800 600");
  }

  if (iupStrEqual(name, "MONITORSINFO"))
  {
    QList<QScreen*> screens = QGuiApplication::screens();
    int monitors_count = screens.size();
    const int entry_size = 50;

    char *str = iupStrGetMemory(monitors_count * entry_size);
    char* pstr = str;

    for (int i = 0; i < monitors_count; i++)
    {
      QRect geom = screens[i]->geometry();
      int written = snprintf(pstr, entry_size, "%d %d %d %d\n",
                             geom.x(), geom.y(), geom.width(), geom.height());
      pstr += written;
    }

    return str;
  }

  if (iupStrEqual(name, "MONITORSCOUNT"))
  {
    QList<QScreen*> screens = QGuiApplication::screens();
    return iupStrReturnInt(screens.size());
  }

  if (iupStrEqual(name, "TRUECOLORCANVAS"))
  {
    /* Qt always uses true color in modern systems */
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen)
      return iupStrReturnBoolean(screen->depth() > 8);

    return iupStrReturnBoolean(1);  /* Assume true color */
  }

  if (iupStrEqual(name, "UTF8MODE"))
  {
    return iupStrReturnBoolean(iupqtStrGetUTF8Mode());
  }

  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
    return iupStrReturnBoolean(!iupqtStrGetUTF8Mode());
  }

#ifndef _WIN32
  if (iupStrEqual(name, "EXEFILENAME"))
  {
    char* argv0 = IupGetGlobal("ARGV0");
    if (argv0)
    {
      char* exefilename = realpath(argv0, NULL);
      if (exefilename)
      {
        char* str = iupStrReturnStr(exefilename);
        free(exefilename);
        return str;
      }
    }
  }
#endif

  if (iupStrEqual(name, "SHOWMENUIMAGES"))
  {
    /* Qt shows menu images by default */
    return iupStrReturnBoolean(1);
  }

  if (iupStrEqual(name, "DARKMODE"))
  {
    return iupStrReturnBoolean(iupqtIsSystemDarkMode());
  }

  if (iupStrEqual(name, "SANDBOX"))
  {
    if (getenv("FLATPAK_ID"))
      return (char*)"FLATPAK";
    if (getenv("SNAP"))
      return (char*)"SNAP";
    if (getenv("APPIMAGE"))
      return (char*)"APPIMAGE";
    return NULL;
  }

  return NULL;
}

/****************************************************************************
 * Key Decoding
 ****************************************************************************/

extern "C" int iupqtKeyDecode(QEvent *evt)
{
  QKeyEvent* key_evt = static_cast<QKeyEvent*>(evt);

  if (!key_evt)
    return 0;

  int qt_key = key_evt->key();
  int iup_key = 0;

  /* Map Qt keys to IUP keys */
  switch(qt_key)
  {
  case Qt::Key_Escape:       iup_key = K_ESC; break;
  case Qt::Key_Return:
  case Qt::Key_Enter:        iup_key = K_CR; break;
  case Qt::Key_Backspace:    iup_key = K_BS; break;
  case Qt::Key_Tab:          iup_key = K_TAB; break;
  case Qt::Key_Space:        iup_key = K_SP; break;

  /* Navigation */
  case Qt::Key_Home:         iup_key = K_HOME; break;
  case Qt::Key_End:          iup_key = K_END; break;
  case Qt::Key_Left:         iup_key = K_LEFT; break;
  case Qt::Key_Right:        iup_key = K_RIGHT; break;
  case Qt::Key_Up:           iup_key = K_UP; break;
  case Qt::Key_Down:         iup_key = K_DOWN; break;
  case Qt::Key_PageUp:       iup_key = K_PGUP; break;
  case Qt::Key_PageDown:     iup_key = K_PGDN; break;

  /* Function keys */
  case Qt::Key_F1:           iup_key = K_F1; break;
  case Qt::Key_F2:           iup_key = K_F2; break;
  case Qt::Key_F3:           iup_key = K_F3; break;
  case Qt::Key_F4:           iup_key = K_F4; break;
  case Qt::Key_F5:           iup_key = K_F5; break;
  case Qt::Key_F6:           iup_key = K_F6; break;
  case Qt::Key_F7:           iup_key = K_F7; break;
  case Qt::Key_F8:           iup_key = K_F8; break;
  case Qt::Key_F9:           iup_key = K_F9; break;
  case Qt::Key_F10:          iup_key = K_F10; break;
  case Qt::Key_F11:          iup_key = K_F11; break;
  case Qt::Key_F12:          iup_key = K_F12; break;

  /* Editing */
  case Qt::Key_Insert:       iup_key = K_INS; break;
  case Qt::Key_Delete:       iup_key = K_DEL; break;

  /* Modifiers (when pressed alone) */
  case Qt::Key_Shift:        iup_key = 0; break;  /* Ignore */
  case Qt::Key_Control:      iup_key = 0; break;
  case Qt::Key_Alt:          iup_key = 0; break;
  case Qt::Key_Meta:         iup_key = 0; break;

  /* Numpad */
  case Qt::Key_multiply:     iup_key = K_asterisk; break;
  case Qt::Key_Plus:         iup_key = K_plus; break;
  case Qt::Key_Minus:        iup_key = K_minus; break;
  case Qt::Key_Period:       iup_key = K_period; break;
  case Qt::Key_Slash:        iup_key = K_slash; break;

  default:
    /* Try to get the text character */
    QString text = key_evt->text();
    if (!text.isEmpty())
    {
      QChar ch = text[0];
      if (ch.isPrint())
        iup_key = ch.unicode();
    }
    break;
  }

  return iup_key;
}
