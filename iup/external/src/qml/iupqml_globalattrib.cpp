/** \file
 * \brief Qt Quick Driver iupdrvSetGlobal/iupdrvGetGlobal
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstdlib>

#include <QCursor>
#include <QKeyEvent>
#include <QQuickStyle>
#include <QString>
#include <QRect>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_key.h"
#include "iup_singleinstance.h"
}

#include "iupqml_drv.h"


/****************************************************************************
 * Global Input Event Handler
 ****************************************************************************/

class IupQmlEventFilter : public QObject
{
public:
  static IupQmlEventFilter* instance()
  {
    static IupQmlEventFilter* filter = nullptr;
    if (!filter)
      filter = new IupQmlEventFilter();
    return filter;
  }

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    (void)obj;

    switch(event->type())
    {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonRelease:
      {
        auto cb = reinterpret_cast<IFiiiis>(IupGetFunction("GLOBALBUTTON_CB"));
        if (cb)
        {
          auto* mouse_evt = static_cast<QMouseEvent*>(event);
          int x = mouse_evt->globalPosition().x();
          int y = mouse_evt->globalPosition().y();
          int press = (event->type() != QEvent::MouseButtonRelease) ? 1 : 0;
          int doubleclick = (event->type() == QEvent::MouseButtonDblClick) ? 1 : 0;

          int button = IUP_BUTTON1;
          if (mouse_evt->button() == Qt::LeftButton)
            button = IUP_BUTTON1;
          else if (mouse_evt->button() == Qt::MiddleButton)
            button = IUP_BUTTON2;
          else if (mouse_evt->button() == Qt::RightButton)
            button = IUP_BUTTON3;

          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
          iupqmlButtonKeySetStatus(mouse_evt->modifiers(), mouse_evt->buttons(), button, status, doubleclick);

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
        auto cb = reinterpret_cast<IFiis>(IupGetFunction("GLOBALMOTION_CB"));
        if (cb)
        {
          auto* mouse_evt = static_cast<QMouseEvent*>(event);
          int x = mouse_evt->globalPosition().x();
          int y = mouse_evt->globalPosition().y();

          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
          iupqmlButtonKeySetStatus(mouse_evt->modifiers(), Qt::NoButton, 0, status, 0);

          cb(x, y, status);
        }
        break;
      }

    case QEvent::Wheel:
      {
        auto cb = reinterpret_cast<IFfiis>(IupGetFunction("GLOBALWHEEL_CB"));
        if (cb)
        {
          auto* wheel_evt = static_cast<QWheelEvent*>(event);

          QPoint angle_delta = wheel_evt->angleDelta();
          float delta = angle_delta.y() / 120.0f;  /* Normalize to notches (120 units per notch) */

          QPoint global_pos = wheel_evt->globalPosition().toPoint();

          int x = global_pos.x();
          int y = global_pos.y();

          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
          iupqmlButtonKeySetStatus(wheel_evt->modifiers(), Qt::NoButton, 0, status, 0);

          cb(delta, x, y, status);
        }
        break;
      }

    case QEvent::KeyPress:
    case QEvent::KeyRelease:
      {
        IFii cb = reinterpret_cast<IFii>(IupGetFunction("GLOBALKEYPRESS_CB"));
        if (cb)
        {
          int pressed = (event->type() == QEvent::KeyPress) ? 1 : 0;
          int code = iupqmlKeyDecode(static_cast<QKeyEvent*>(event));

          if (code != 0)
            cb(code, pressed);
        }
        break;
      }

    default:
      break;
    }

    return false;
  }
};

/****************************************************************************
 * Global Set/Get
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvSetGlobal(const char* name, const char* value)
{
  if (iupStrEqual(name, "SINGLEINSTANCE"))
  {
    if (iupdrvSingleInstanceSet(value))
      return 0;
    else
      return 1;
  }
  if (iupStrEqual(name, "INPUTCALLBACKS"))
  {
    if (iupStrBoolean(value))
    {
      QGuiApplication* app = iupqmlGetApplication();
      if (app)
        app->installEventFilter(IupQmlEventFilter::instance());
    }
    else
    {
      QGuiApplication* app = iupqmlGetApplication();
      if (app)
        app->removeEventFilter(IupQmlEventFilter::instance());
    }
    return 1;
  }

  if (iupStrEqual(name, "UTF8MODE") || iupStrEqual(name, "UTF8AUTOCONVERT"))
    return 1;

  if (iupStrEqual(name, "SHOWMENUIMAGES"))
  {
    /* Qt has no global setting for menu images */
    return 1;
  }

  if (iupStrEqual(name, "QTSTYLE"))
  {
    if (!value || !value[0])
      return 0;
    if (iupqmlStyleLocked())
      return 0;
    QQuickStyle::setStyle(QString::fromUtf8(value));
    return 1;
  }

  return 1;
}

extern "C" IUP_SDK_API char* iupdrvGetGlobal(const char* name)
{
  if (iupStrEqual(name, "VIRTUALSCREEN"))
  {
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

    char* str = iupStrGetMemory(monitors_count * entry_size);
    char* pstr = str;

    for (int i = 0; i < monitors_count; i++)
    {
      int remaining = monitors_count * entry_size - static_cast<int>(pstr - str);
      if (remaining <= 0)
        break;
      QRect geom = screens[i]->geometry();
      int written = snprintf(pstr, remaining, "%d %d %d %d\n",
                             geom.x(), geom.y(), geom.width(), geom.height());
      if (written >= remaining)
        written = remaining - 1;
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
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen)
      return iupStrReturnBoolean(screen->depth() > 8);

    return iupStrReturnBoolean(1);
  }

  if (iupStrEqual(name, "UTF8MODE"))
    return iupStrReturnBoolean(1);

  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
    return iupStrReturnBoolean(0);

  if (iupStrEqual(name, "QTSTYLE"))
    return iupStrReturnStr(QQuickStyle::name().toUtf8().constData());

  if (iupStrEqual(name, "SHOWMENUIMAGES"))
  {
    /* Qt shows menu images by default */
    return iupStrReturnBoolean(1);
  }

  if (iupStrEqual(name, "SANDBOX"))
  {
    if (getenv("FLATPAK_ID"))
      return const_cast<char*>("FLATPAK");
    if (getenv("SNAP"))
      return const_cast<char*>("SNAP");
    if (getenv("APPIMAGE"))
      return const_cast<char*>("APPIMAGE");
    return nullptr;
  }

  return nullptr;
}

